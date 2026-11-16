#include <Script/GpkScriptBinder.h>
#include <Inline/PathDecode.h>
#include <Event/Event.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <cstring>
#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <shobjidl.h> // IFileOpenDialog
    #include <vector>
#endif

extern "C" {
    #include <lauxlib.h>
}

namespace fs = std::filesystem;

namespace {
    constexpr const char* GPK_BINDER_REGISTRY_KEY = "Girigara_GpkScriptBinder";

    GpkScriptBinder* getBinderFromLua(lua_State* L) {
        lua_pushstring(L, GPK_BINDER_REGISTRY_KEY);
        lua_gettable(L, LUA_REGISTRYINDEX);
        auto* binder = static_cast<GpkScriptBinder*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        return binder;
    }
}

GpkScriptBinder::GpkScriptBinder(EventBus& eventBus)
: m_eventBus(eventBus) {
    m_eventBus.subscribe<GpkPackCompletedEvent>(
        [this](GpkPackCompletedEvent& e) { onPackCompleted(e); },
        m_id
    );

    m_eventBus.publish(RegisterScriptBindingEvent{
        m_id,
        GPK_BINDER_REGISTRY_KEY,
        this,
        {
            { "GpkPackDirectory",      &GpkScriptBinder::lua_GpkPackDirectory },
            { "GpkPackDirectoryAsync", &GpkScriptBinder::lua_GpkPackDirectoryAsync },
            { "GpkPackFiles",          &GpkScriptBinder::lua_GpkPackFiles },
            { "GpkList",               &GpkScriptBinder::lua_GpkList },
            { "GpkExtractAll",         &GpkScriptBinder::lua_GpkExtractAll },
            { "OpenFolderDialog",      &GpkScriptBinder::lua_OpenFolderDialog },
            { "OpenFileDialog",        &GpkScriptBinder::lua_OpenFileDialog }
        }
    });
}

GpkScriptBinder::~GpkScriptBinder() {
    // 1. 先清空 workers：std::jthread 解構自動發出 stop_token 並 join()，確保不再有任何背景執行緒存取成員
    {
        std::lock_guard<std::mutex> lock(m_workersMutex);
        m_workers.clear();
    }

    // 2. 在主執行緒釋放所有尚未觸發的 LuaRef
    m_pendingCallbacks.clear();

    // 3. 退訂 EventBus 與 Lua 函式綁定
    m_eventBus.unsubscribeByOwner(m_id);
    m_eventBus.publish(UnregisterScriptBindingEvent{ m_id });
}

GpkArchive::Key256 GpkScriptBinder::parseKey(const std::string& keyStr) {
    GpkArchive::Key256 key{};
    if (keyStr.empty()) return key;

    // 若剛好是 64 個合法 Hex 字元，直接精確解析為 32 bytes
    auto isHex = [](unsigned char c) { return std::isxdigit(c) != 0; };
    if (keyStr.size() == 64 && std::all_of(keyStr.begin(), keyStr.end(), isHex)) {
        for (size_t i = 0; i < 32; ++i) {
            unsigned int byteVal = 0;
            std::sscanf(keyStr.substr(i * 2, 2).c_str(), "%02x", &byteVal);
            key[i] = static_cast<uint8_t>(byteVal);
        }
        return key;
    }

    // 否則將使用者輸入的任意密碼字串以 FNV-1a + 旋轉混合展開成 256-bit 金鑰
    uint64_t h1 = 0xcbf29ce484222325ull;
    uint64_t h2 = 0x84222325cbf29ce4ull;
    uint64_t h3 = 0x100000001b3ull;
    uint64_t h4 = 0x9e3779b97f4a7c15ull;

    for (size_t i = 0; i < keyStr.size(); ++i) {
        uint8_t b = static_cast<uint8_t>(keyStr[i]);
        h1 = (h1 ^ b) * 0x100000001b3ull;
        h2 = ((h2 << 7) | (h2 >> 57)) ^ (b + i * 31u);
        h3 = (h3 + b) * 0xbf58476d1ce4e5b9ull;
        h4 ^= ((h4 << 13) | (h4 >> 51)) + b * 0x94d049bb133111ebull;
    }

    std::memcpy(key.data() + 0,  &h1, 8);
    std::memcpy(key.data() + 8,  &h2, 8);
    std::memcpy(key.data() + 16, &h3, 8);
    std::memcpy(key.data() + 24, &h4, 8);
    return key;
}

GpkType GpkScriptBinder::parseType(const std::string& typeStr) {
    std::string lower = typeStr;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (!lower.empty() && lower[0] == '.') lower.erase(0, 1);

    if (lower == "imz" || lower == "image")    return GpkType::Image;
    if (lower == "luz" || lower == "script")   return GpkType::Script;
    if (lower == "dtz" || lower == "database") return GpkType::Database;
    if (lower == "auz" || lower == "audio")    return GpkType::Audio;
    return GpkType::Unknown;
}

std::string GpkScriptBinder::typeToString(GpkType type) {
    switch (type) {
        case GpkType::Image:    return "imz";
        case GpkType::Script:   return "luz";
        case GpkType::Database: return "dtz";
        case GpkType::Audio:    return "auz";
        default:                return "unknown";
    }
}

// @luaFunc GpkPackDirectory(srcDir: string, outPath: string, type: string, key?: string, encrypt?: boolean) -> boolean, string
int GpkScriptBinder::lua_GpkPackDirectory(lua_State* L) {
    const char* srcDirStr  = luaL_checkstring(L, 1);
    const char* outPathStr = luaL_checkstring(L, 2);
    const char* typeStr    = luaL_checkstring(L, 3);
    const char* keyStr     = luaL_optstring(L, 4, "");
    bool encrypt           = lua_isnoneornil(L, 5) ? true : (lua_toboolean(L, 5) != 0);

    // 1. 修正：使用 pathFromUtf8
    fs::path srcDir = pathFromUtf8(srcDirStr);
    if (!fs::exists(srcDir) || !fs::is_directory(srcDir)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Source directory does not exist.");
        return 2;
    }

    GpkType type = parseType(typeStr);
    if (type == GpkType::Unknown) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Invalid archive type (expected imz, luz, dtz, or auz).");
        return 2;
    }

    std::vector<GpkArchive::PackEntry> entries;
    for (const auto& entry : fs::recursive_directory_iterator(srcDir)) {
        if (!entry.is_regular_file()) continue;
        fs::path relPath = fs::relative(entry.path(), srcDir);
        
        // 2. 修正：確保封包內部虛擬路徑也是正確的 UTF-8，並統一正斜線
        std::string virtualPath = pathToUtf8(relPath);
        std::ranges::replace(virtualPath, '\\', '/'); // C++20/23 ranges 寫法 (需 #include <algorithm>)

        // 3. 修正：entities 存的實體路徑若給 std::ifstream 開啟，也必須是 UTF-8
        entries.push_back(GpkArchive::PackEntry{ std::move(virtualPath), pathToUtf8(entry.path()) });
    }

    if (entries.empty()) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "No files found in source directory.");
        return 2;
    }

    // 4. 修正：使用 pathFromUtf8
    fs::path outPath = pathFromUtf8(outPathStr);
    if (outPath.has_parent_path()) {
        std::error_code ec;
        fs::create_directories(outPath.parent_path(), ec);
    }

    GpkArchive::Key256 key = parseKey(keyStr);
    // 注意：請確保 GpkArchive::packFiles 內部開啟 outPathStr 與 entries 裡的路徑時，也有使用 pathFromUtf8！
    if (!GpkArchive::packFiles(outPathStr, type, entries, key, encrypt)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Failed to write encrypted archive.");
        return 2;
    }

    std::string msg = "Successfully packed " + std::to_string(entries.size()) + " files into " + outPathStr;
    lua_pushboolean(L, 1);
    lua_pushstring(L, msg.c_str());
    return 2;
}

void GpkScriptBinder::cleanupFinishedWorkers() {
    std::lock_guard<std::mutex> lock(m_workersMutex);
    std::erase_if(m_workers, [](const WorkerSlot& slot) {
        return slot.finished && slot.finished->load(std::memory_order_acquire);
    });
}

void GpkScriptBinder::onPackCompleted(GpkPackCompletedEvent& e) {
    // 順便回收已結束的 jthread
    cleanupFinishedWorkers();

    auto it = m_pendingCallbacks.find(e.taskId);
    if (it == m_pendingCallbacks.end()) {
        return;
    }

    // 取出後立即從 map 移除，確保即使 Lua 回呼內拋錯或再次呼叫 Async 也不會重入或洩漏
    LuaRef callback = std::move(it->second);
    m_pendingCallbacks.erase(it);

    if (callback.isValid() && callback.isFunction()) {
        lua_State* L = callback.getState();
        callback.push();
        lua_pushboolean(L, e.success ? 1 : 0);
        lua_pushstring(L, e.message.c_str());
        lua_pushstring(L, e.outPath.c_str());

        if (lua_pcall(L, 3, 0, 0) != LUA_OK) {
            // 彈出錯誤字串避免污染 Lua 堆疊
            lua_pop(L, 1);
        }
    }
}

// @luaFunc GpkPackDirectoryAsync(srcDir: string, outPath: string, type: string, key?: string, encrypt?: boolean, onComplete?: function) -> boolean, string
int GpkScriptBinder::lua_GpkPackDirectoryAsync(lua_State* L) {
    GpkScriptBinder* self = getBinderFromLua(L);
    if (!self) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Internal error: GpkScriptBinder pointer is null.");
        return 2;
    }

    // 1. 在主執行緒收集並複製所有參數（背景執行緒絕不接觸 lua_State 或 LuaRef）
    std::string srcDirStr  = luaL_checkstring(L, 1);
    std::string outPathStr = luaL_checkstring(L, 2);
    std::string typeStr    = luaL_checkstring(L, 3);
    std::string keyStr     = luaL_optstring(L, 4, "");
    bool encrypt           = lua_isnoneornil(L, 5) ? true : (lua_toboolean(L, 5) != 0);

    LuaRef callbackRef;
    if (lua_isfunction(L, 6)) {
        lua_pushvalue(L, 6);
        callbackRef = LuaRef::fromStack(L);
    }

    GpkType type = parseType(typeStr);
    if (type == GpkType::Unknown) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Invalid archive type (expected imz, luz, dtz, or auz).");
        return 2;
    }

    self->cleanupFinishedWorkers();

    uint64_t taskId = self->m_nextTaskId.fetch_add(1, std::memory_order_relaxed);
    if (callbackRef.isValid()) {
        self->m_pendingCallbacks.emplace(taskId, std::move(callbackRef));
    }

    GpkArchive::Key256 key = parseKey(keyStr);
    auto finishedFlag = std::make_shared<std::atomic<bool>>(false);

    // 2. 啟動 std::jthread 在背景執行掃描與 GpkArchive::packFiles
    std::jthread worker([self,
                         finishedFlag,
                         taskId,
                         srcDirStr = std::move(srcDirStr),
                         outPathStr = std::move(outPathStr),
                         type,
                         key,
                         encrypt](std::stop_token stoken) {
        bool ok = false;
        std::string msg;

        try {
            fs::path srcDir = pathFromUtf8(srcDirStr);
            if (!fs::exists(srcDir) || !fs::is_directory(srcDir)) {
                msg = "Source directory does not exist.";
            } else {
                std::vector<GpkArchive::PackEntry> entries;
                for (const auto& entry : fs::recursive_directory_iterator(srcDir)) {
                    if (stoken.stop_requested()) {
                        msg = "Packing cancelled.";
                        self->m_eventBus.enqueue(GpkPackCompletedEvent{ taskId, false, outPathStr, std::move(msg) });
                        finishedFlag->store(true, std::memory_order_release);
                        return;
                    }
                    if (!entry.is_regular_file()) continue;

                    fs::path relPath = fs::relative(entry.path(), srcDir);
                    std::string virtualPath = pathToUtf8(relPath);
                    std::ranges::replace(virtualPath, '\\', '/');

                    entries.push_back(GpkArchive::PackEntry{ std::move(virtualPath), pathToUtf8(entry.path()) });
                }

                if (entries.empty()) {
                    msg = "No files found in source directory.";
                } else if (!stoken.stop_requested()) {
                    fs::path outPath = pathFromUtf8(outPathStr);
                    if (outPath.has_parent_path()) {
                        std::error_code ec;
                        fs::create_directories(outPath.parent_path(), ec);
                    }

                    if (GpkArchive::packFiles(outPathStr, type, entries, key, encrypt)) {
                        ok = true;
                        msg = "Successfully packed " + std::to_string(entries.size()) + " files into " + outPathStr;
                    } else {
                        msg = "Failed to write encrypted archive.";
                    }
                }
            }
        } catch (const std::exception& e) {
            ok = false;
            msg = std::string("Exception during async pack: ") + e.what();
        }

        // 3. 透過執行緒安全的 EventBus::enqueue 將完成事件排入主執行緒佇列
        self->m_eventBus.enqueue(GpkPackCompletedEvent{ taskId, ok, std::move(outPathStr), std::move(msg) });
        finishedFlag->store(true, std::memory_order_release);
    });

    {
        std::lock_guard<std::mutex> lock(self->m_workersMutex);
        self->m_workers.push_back(WorkerSlot{ std::move(finishedFlag), std::move(worker) });
    }

    lua_pushboolean(L, 1);
    lua_pushstring(L, "Async pack task started.");
    return 2;
}

// @luaFunc GpkPackFiles(fileList: table, outPath: string, type: string, key?: string, encrypt?: boolean) -> boolean, string
int GpkScriptBinder::lua_GpkPackFiles(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    const char* outPathStr = luaL_checkstring(L, 2);
    const char* typeStr    = luaL_checkstring(L, 3);
    const char* keyStr     = luaL_optstring(L, 4, "");
    bool encrypt           = lua_isnoneornil(L, 5) ? true : (lua_toboolean(L, 5) != 0);

    GpkType type = parseType(typeStr);
    if (type == GpkType::Unknown) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Invalid archive type.");
        return 2;
    }

    std::vector<GpkArchive::PackEntry> entries;
    lua_Integer len = luaL_len(L, 1);
    for (lua_Integer i = 1; i <= len; ++i) {
        lua_rawgeti(L, 1, i);
        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, "src");
            lua_getfield(L, -2, "dst");
            const char* src = lua_tostring(L, -2);
            const char* dst = lua_tostring(L, -1);
            if (src && dst) {
                entries.push_back(GpkArchive::PackEntry{ dst, src });
            }
            lua_pop(L, 2);
        }
        lua_pop(L, 1);
    }

    if (entries.empty()) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "File list is empty.");
        return 2;
    }

    GpkArchive::Key256 key = parseKey(keyStr);
    if (!GpkArchive::packFiles(outPathStr, type, entries, key, encrypt)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Failed to pack specified files.");
        return 2;
    }

    std::string msg = "Packed " + std::to_string(entries.size()) + " files.";
    lua_pushboolean(L, 1);
    lua_pushstring(L, msg.c_str());
    return 2;
}

// @luaFunc GpkList(archivePath: string, key?: string) -> table|nil, string
int GpkScriptBinder::lua_GpkList(lua_State* L) {
    const char* archivePath = luaL_checkstring(L, 1);
    const char* keyStr      = luaL_optstring(L, 2, "");

    GpkArchive arc;
    if (!arc.open(archivePath, parseKey(keyStr))) {
        lua_pushnil(L);
        lua_pushstring(L, "Failed to open archive (invalid file or wrong key).");
        return 2;
    }

    auto files = arc.listFiles();
    lua_createtable(L, static_cast<int>(files.size()), 0);
    for (size_t i = 0; i < files.size(); ++i) {
        lua_pushstring(L, files[i].c_str());
        lua_rawseti(L, -2, static_cast<lua_Integer>(i + 1));
    }

    std::string typeStr = typeToString(arc.getType());
    lua_pushstring(L, typeStr.c_str());
    return 2;
}

// @luaFunc GpkExtractAll(archivePath: string, outDir: string, key?: string) -> boolean, string
int GpkScriptBinder::lua_GpkExtractAll(lua_State* L) {
    const char* archivePath = luaL_checkstring(L, 1);
    const char* outDirStr   = luaL_checkstring(L, 2);
    const char* keyStr      = luaL_optstring(L, 3, "");

    GpkArchive arc;
    // 注意：請確保 arc.open 內部開啟 archivePath 時也有使用 pathFromUtf8
    if (!arc.open(archivePath, parseKey(keyStr))) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Failed to open archive (invalid file or wrong key).");
        return 2;
    }

    // 1. 修正：使用 pathFromUtf8
    fs::path outRoot = pathFromUtf8(outDirStr);
    auto files = arc.listFiles();
    size_t extractedCount = 0;

    for (const auto& virtualPath : files) {
        std::vector<uint8_t> data;
        if (!arc.extractFile(virtualPath, data)) {
            lua_pushboolean(L, 0);
            lua_pushfstring(L, "Failed to extract entry: %s", virtualPath.c_str());
            return 2;
        }

        // 2. 修正：virtualPath 也可能包含中文，必須用 pathFromUtf8 轉換後再拼接
        fs::path targetPath = outRoot / pathFromUtf8(virtualPath);
        if (targetPath.has_parent_path()) {
            std::error_code ec;
            fs::create_directories(targetPath.parent_path(), ec);
        }

        std::ofstream out(targetPath, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            lua_pushboolean(L, 0);
            // 3. 修正：錯誤訊息回傳給 Lua 時使用 pathToUtf8，避免路徑在 Lua 端變成亂碼
            lua_pushfstring(L, "Cannot write output file: %s", pathToUtf8(targetPath).c_str());
            return 2;
        }

        if (!data.empty()) {
            out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        }
        ++extractedCount;
    }

    std::string msg = "Extracted " + std::to_string(extractedCount) + " files to " + outDirStr;
    lua_pushboolean(L, 1);
    lua_pushstring(L, msg.c_str());
    return 2;
}

namespace {
#if defined(_WIN32)
    std::wstring utf8ToWide(const std::string& utf8) {
        if (utf8.empty()) return L"";
        int len = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
        if (len <= 0) return L"";
        std::wstring wide(static_cast<size_t>(len), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), len);
        return wide;
    }

    std::string wideToUtf8(const wchar_t* wide) {
        if (!wide || *wide == L'\0') return "";
        int len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
        if (len <= 1) return "";
        std::string utf8(static_cast<size_t>(len - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8.data(), len, nullptr, nullptr);
        return utf8;
    }

    // 透過 Win32 COM IFileOpenDialog 顯示現代系統對話框（folderMode = true 選資料夾，false 選檔案）
    std::string showWin32OpenDialog(const std::string& titleUtf8, bool folderMode, const std::string& filterPatternUtf8 = "") {
        std::string selectedUtf8;

        HRESULT hrInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        bool needUninit = SUCCEEDED(hrInit);

        IFileOpenDialog* pDialog = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pDialog));
        if (SUCCEEDED(hr) && pDialog) {
            DWORD options = 0;
            if (SUCCEEDED(pDialog->GetOptions(&options))) {
                options |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST;
                if (folderMode) {
                    options |= FOS_PICKFOLDERS;
                } else {
                    options |= FOS_FILEMUSTEXIST;
                }
                pDialog->SetOptions(options);
            }

            std::wstring wTitle = utf8ToWide(titleUtf8);
            if (!wTitle.empty()) {
                pDialog->SetTitle(wTitle.c_str());
            }

            std::wstring wSpec;
            if (!folderMode) {
                wSpec = filterPatternUtf8.empty()
                    ? L"*.imz;*.luz;*.dtz;*.auz"
                    : utf8ToWide(filterPatternUtf8);
                COMDLG_FILTERSPEC fileTypes[] = {
                    { L"Girigara Archive", wSpec.c_str() },
                    { L"All Files (*.*)",  L"*.*" }
                };
                pDialog->SetFileTypes(2, fileTypes);
                pDialog->SetFileTypeIndex(1);
            }

            // 取得目前前景視窗作為 Owner，避免對話框跑到遊戲視窗後方
            HWND ownerHwnd = GetActiveWindow();
            if (SUCCEEDED(pDialog->Show(ownerHwnd))) {
                IShellItem* pItem = nullptr;
                if (SUCCEEDED(pDialog->GetResult(&pItem)) && pItem) {
                    PWSTR pszFilePath = nullptr;
                    if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)) && pszFilePath) {
                        selectedUtf8 = wideToUtf8(pszFilePath);
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pDialog->Release();
        }

        if (needUninit) {
            CoUninitialize();
        }
        return selectedUtf8;
    }
#else
    std::string runDialogCommand(const std::string& cmd) {
        std::string result;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return result;
        char buf[512];
        while (fgets(buf, sizeof(buf), pipe)) {
            result += buf;
        }
        pclose(pipe);
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
            result.pop_back();
        }
        return result;
    }
#endif
} // namespace

// @luaFunc OpenFolderDialog(title?: string) -> string|nil
int GpkScriptBinder::lua_OpenFolderDialog(lua_State* L) {
    const char* title = luaL_optstring(L, 1, "選擇目標資料夾");
#if defined(_WIN32)
    std::string path = showWin32OpenDialog(title, true);
#else
    std::string cmd = "zenity --file-selection --directory --title=\"" + std::string(title) +
                      "\" 2>/dev/null || kdialog --getexistingdirectory . --title \"" + std::string(title) + "\" 2>/dev/null";
    std::string path = runDialogCommand(cmd);
#endif
    if (path.empty()) lua_pushnil(L);
    else lua_pushstring(L, path.c_str());
    return 1;
}

// @luaFunc OpenFileDialog(title?: string, filter?: string) -> string|nil
int GpkScriptBinder::lua_OpenFileDialog(lua_State* L) {
    const char* title  = luaL_optstring(L, 1, "選擇封包檔案");
    const char* filter = luaL_optstring(L, 2, "");
#if defined(_WIN32)
    std::string path = showWin32OpenDialog(title, false, filter);
#else
    std::string fileFilter = (filter && filter[0] != '\0') ? filter : "*.imz *.luz *.dtz *.auz";
    std::string cmd = "zenity --file-selection --title=\"" + std::string(title) +
                      "\" --file-filter='Girigara Archive | " + fileFilter + "' --file-filter='All files | *' 2>/dev/null "
                      "|| kdialog --getopenfilename . '" + fileFilter + "' --title \"" + std::string(title) + "\" 2>/dev/null";
    std::string path = runDialogCommand(cmd);
#endif
    if (path.empty()) lua_pushnil(L);
    else lua_pushstring(L, path.c_str());
    return 1;
}
