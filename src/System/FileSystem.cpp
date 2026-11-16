#include <System/FileSystem.h>
#include <Inline/PathDecode.h>
#include <Event/Event.h>
#include <filesystem>
#include <fstream>

extern "C" {
    #include <lauxlib.h>
}

namespace fs = std::filesystem;

namespace {
    constexpr const char* FILE_SYSTEM_REGISTRY_KEY = "GIRIGARA_FILE_SYSTEM_INSTANCE";

    FileSystem* getFileSystemFromLua(lua_State* L) {
        lua_pushstring(L, FILE_SYSTEM_REGISTRY_KEY);
        lua_gettable(L, LUA_REGISTRYINDEX);
        auto* sys = static_cast<FileSystem*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        return sys;
    }
} // namespace

FileSystem::FileSystem(EventBus& eventBus, LoggerSystem& logger)
: m_eventBus(eventBus), m_logger(logger) {
    registerScriptBindings();
    bindEvents();
}

FileSystem::~FileSystem() {
    m_eventBus.unsubscribeByOwner(getID());
    m_dropCallback.reset();
    unregisterScriptBindings();
}

void FileSystem::registerScriptBindings() {
    m_eventBus.publish(RegisterScriptBindingEvent{
        getID(),
                       FILE_SYSTEM_REGISTRY_KEY,
                       this,
                       {
                           { "RegisterFileDropCallback", &FileSystem::lua_RegisterFileDropCallback },
                           { "FsStatPath",               &FileSystem::lua_FsStatPath },
                           { "FsReadBinary",             &FileSystem::lua_FsReadBinary },
                           { "FsWriteBinary",            &FileSystem::lua_FsWriteBinary }
                       }
    });
}

void FileSystem::unregisterScriptBindings() {
    m_eventBus.publish(UnregisterScriptBindingEvent{ getID() });
}

void FileSystem::bindEvents() {
    m_eventBus.subscribe<FileDropEvent>([this](FileDropEvent& e) {
        handleFileDrop(e.paths);
    }, getID());
}

void FileSystem::handleFileDrop(const std::vector<std::string>& paths) {
    if (paths.empty() || !m_dropCallback.isValid() || !m_dropCallback.isFunction()) return;

    lua_State* L = m_dropCallback.getState();
    if (!L) return;

    m_dropCallback.push();
    lua_createtable(L, static_cast<int>(paths.size()), 0);
    for (size_t i = 0; i < paths.size(); ++i) {
        lua_pushlstring(L, paths[i].data(), paths[i].size());
        lua_rawseti(L, -2, static_cast<lua_Integer>(i + 1));
    }

    if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        m_logger.error(std::string("[FileSystem] OnFileDrop 執行錯誤: ") + (err ? err : "Unknown"));
        lua_pop(L, 1);
    }
}

bool FileSystem::exists(const std::string& path) const {
    std::error_code ec;
    return std::filesystem::exists(pathFromUtf8(path));
}

bool FileSystem::isDirectory(const std::string& path) const {
    std::error_code ec;
    return std::filesystem::is_directory(pathFromUtf8(path));
}

bool FileSystem::isRegularFile(const std::string& path) const {
    std::error_code ec;
    return std::filesystem::is_regular_file(pathFromUtf8(path));
}

bool FileSystem::readBinary(const std::string& path, std::vector<uint8_t>& outData) const {
    std::ifstream in(fs::path(path), std::ios::binary | std::ios::ate);
    if (!in.is_open()) return false;

    std::streamsize sz = in.tellg();
    in.seekg(0, std::ios::beg);
    if (sz < 0) return false;

    outData.resize(static_cast<size_t>(sz));
    if (sz > 0) {
        in.read(reinterpret_cast<char*>(outData.data()), sz);
        if (!in.good() && !in.eof()) {
            outData.clear();
            return false;
        }
    }
    return true;
}

bool FileSystem::writeBinary(const std::string& path, const std::vector<uint8_t>& data) const {
    fs::path p(path);
    if (p.has_parent_path()) {
        std::error_code ec;
        fs::create_directories(p.parent_path(), ec);
    }

    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;

    if (!data.empty()) {
        out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    }
    return out.good();
}

int FileSystem::lua_RegisterFileDropCallback(lua_State* L) {
    FileSystem* sys = getFileSystemFromLua(L);
    if (!sys) return 0;

    if (lua_isnoneornil(L, 1)) {
        sys->m_dropCallback.reset();
        return 0;
    }

    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_pushvalue(L, 1);
    sys->m_dropCallback = LuaRef::fromStack(L);
    return 0;
}

int FileSystem::lua_FsStatPath(lua_State* L) {
    const char* pathStr = luaL_checkstring(L, 1);
    FileSystem* sys = getFileSystemFromLua(L);
    if (!sys || !sys->exists(pathStr)) {
        lua_pushnil(L);
        return 1;
    }

    if (sys->isDirectory(pathStr)) {
        lua_pushstring(L, "directory");
    } else if (sys->isRegularFile(pathStr)) {
        lua_pushstring(L, "file");
    } else {
        lua_pushstring(L, "other");
    }
    return 1;
}

int FileSystem::lua_FsReadBinary(lua_State* L) {
    const char* pathStr = luaL_checkstring(L, 1);
    FileSystem* sys = getFileSystemFromLua(L);
    if (!sys) {
        lua_pushnil(L);
        lua_pushstring(L, "FileSystem unavailable.");
        return 2;
    }

    std::vector<uint8_t> data;
    if (!sys->readBinary(pathStr, data)) {
        lua_pushnil(L);
        lua_pushstring(L, "Failed to read binary file.");
        return 2;
    }

    lua_pushlstring(L, reinterpret_cast<const char*>(data.data()), data.size());
    return 1;
}

int FileSystem::lua_FsWriteBinary(lua_State* L) {
    const char* pathStr = luaL_checkstring(L, 1);
    size_t len = 0;
    const char* raw = luaL_checklstring(L, 2, &len);

    FileSystem* sys = getFileSystemFromLua(L);
    if (!sys) {
        lua_pushboolean(L, 0);
        return 1;
    }

    std::vector<uint8_t> data(reinterpret_cast<const uint8_t*>(raw),
                              reinterpret_cast<const uint8_t*>(raw) + len);
    lua_pushboolean(L, sys->writeBinary(pathStr, data) ? 1 : 0);
    return 1;
}
