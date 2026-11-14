#include <Script/GpkScriptBinder.h>
#include <Event/Event.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <cstring>

extern "C" {
    #include <lauxlib.h>
}

namespace fs = std::filesystem;

GpkScriptBinder::GpkScriptBinder(EventBus& eventBus)
: m_eventBus(eventBus) {
    m_eventBus.publish(RegisterScriptBindingEvent{
        m_id,
        "Girigara_GpkScriptBinder",
        this,
        {
            { "GpkPackDirectory", &GpkScriptBinder::lua_GpkPackDirectory },
            { "GpkPackFiles",     &GpkScriptBinder::lua_GpkPackFiles },
            { "GpkList",          &GpkScriptBinder::lua_GpkList },
            { "GpkExtractAll",    &GpkScriptBinder::lua_GpkExtractAll },
            { "OpenFolderDialog", &GpkScriptBinder::lua_OpenFolderDialog },
            { "OpenFileDialog",   &GpkScriptBinder::lua_OpenFileDialog }
        }
    });
}

GpkScriptBinder::~GpkScriptBinder() {
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

int GpkScriptBinder::lua_GpkPackDirectory(lua_State* L) {
    const char* srcDirStr  = luaL_checkstring(L, 1);
    const char* outPathStr = luaL_checkstring(L, 2);
    const char* typeStr    = luaL_checkstring(L, 3);
    const char* keyStr     = luaL_optstring(L, 4, "");
    bool encrypt           = lua_isnoneornil(L, 5) ? true : (lua_toboolean(L, 5) != 0);

    fs::path srcDir(srcDirStr);
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
        // 統一轉換為正斜線 '/' 作為封包內部虛擬路徑（跨 Linux / Windows 相容）
        std::string virtualPath = relPath.generic_string();
        entries.push_back(GpkArchive::PackEntry{ virtualPath, entry.path().string() });
    }

    if (entries.empty()) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "No files found in source directory.");
        return 2;
    }

    fs::path outPath(outPathStr);
    if (outPath.has_parent_path()) {
        std::error_code ec;
        fs::create_directories(outPath.parent_path(), ec);
    }

    GpkArchive::Key256 key = parseKey(keyStr);
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

int GpkScriptBinder::lua_GpkExtractAll(lua_State* L) {
    const char* archivePath = luaL_checkstring(L, 1);
    const char* outDirStr   = luaL_checkstring(L, 2);
    const char* keyStr      = luaL_optstring(L, 3, "");

    GpkArchive arc;
    if (!arc.open(archivePath, parseKey(keyStr))) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Failed to open archive (invalid file or wrong key).");
        return 2;
    }

    fs::path outRoot(outDirStr);
    auto files = arc.listFiles();
    size_t extractedCount = 0;

    for (const auto& virtualPath : files) {
        std::vector<uint8_t> data;
        if (!arc.extractFile(virtualPath, data)) {
            lua_pushboolean(L, 0);
            lua_pushfstring(L, "Failed to extract entry: %s", virtualPath.c_str());
            return 2;
        }

        fs::path targetPath = outRoot / fs::path(virtualPath);
        if (targetPath.has_parent_path()) {
            std::error_code ec;
            fs::create_directories(targetPath.parent_path(), ec);
        }

        std::ofstream out(targetPath, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            lua_pushboolean(L, 0);
            lua_pushfstring(L, "Cannot write output file: %s", targetPath.string().c_str());
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
    std::string runDialogCommand(const std::string& cmd) {
        std::string result;
        #if defined(_WIN32)
        FILE* pipe = _popen(cmd.c_str(), "r");
        #else
        FILE* pipe = popen(cmd.c_str(), "r");
        #endif
        if (!pipe) return result;
        char buf[512];
        while (fgets(buf, sizeof(buf), pipe)) {
            result += buf;
        }
        #if defined(_WIN32)
        _pclose(pipe);
        #else
        pclose(pipe);
        #endif
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
            result.pop_back();
        }
        return result;
    }
} // namespace

int GpkScriptBinder::lua_OpenFolderDialog(lua_State* L) {
    const char* title = luaL_optstring(L, 1, "選擇目標資料夾");
    #if defined(_WIN32)
    std::string cmd = "powershell -NoProfile -Command \"Add-Type -AssemblyName System.Windows.Forms; "
    "$f = New-Object System.Windows.Forms.FolderBrowserDialog; "
    "$f.Description = '" + std::string(title) + "'; "
    "if($f.ShowDialog() -eq 'OK'){ $f.SelectedPath }\"";
    #else
    std::string cmd = "zenity --file-selection --directory --title=\"" + std::string(title) +
    "\" 2>/dev/null || kdialog --getexistingdirectory . --title \"" + std::string(title) + "\" 2>/dev/null";
    #endif
    std::string path = runDialogCommand(cmd);
    if (path.empty()) lua_pushnil(L);
    else lua_pushstring(L, path.c_str());
    return 1;
}

int GpkScriptBinder::lua_OpenFileDialog(lua_State* L) {
    const char* title = luaL_optstring(L, 1, "選擇封包檔案");
    #if defined(_WIN32)
    std::string cmd = "powershell -NoProfile -Command \"Add-Type -AssemblyName System.Windows.Forms; "
    "$f = New-Object System.Windows.Forms.OpenFileDialog; "
    "$f.Filter = 'Girigara Archive (*.imz;*.luz;*.dtz;*.auz)|*.imz;*.luz;*.dtz;*.auz|All files (*.*)|*.*'; "
    "if($f.ShowDialog() -eq 'OK'){ $f.FileName }\"";
    #else
    std::string cmd = "zenity --file-selection --title=\"" + std::string(title) +
    "\" --file-filter='Girigara Archive | *.imz *.luz *.dtz *.auz' --file-filter='All files | *' 2>/dev/null "
    "|| kdialog --getopenfilename . '*.imz *.luz *.dtz *.auz' --title \"" + std::string(title) + "\" 2>/dev/null";
    #endif
    std::string path = runDialogCommand(cmd);
    if (path.empty()) lua_pushnil(L);
    else lua_pushstring(L, path.c_str());
    return 1;
}
