#ifndef GIRIGARA_SCRIPT_LUA_REF_H
#define GIRIGARA_SCRIPT_LUA_REF_H

extern "C" {
    #include <lua.h>
    #include <lauxlib.h>
}

class LuaRef {
public:
    LuaRef() = default;

    LuaRef(lua_State* L, int refIndex) : m_L(L), m_ref(refIndex) {
        // 若拿到無效 ref，直接將狀態歸零，避免懸空指針
        if (m_ref == LUA_REFNIL || m_ref == LUA_NOREF) {
            m_L = nullptr;
            m_ref = LUA_NOREF;
        }
    }

    ~LuaRef() {
        reset();
    }

    // ★ 支援深層複製：透過 Lua Registry 增加引用計數
    LuaRef(const LuaRef& other) : m_L(other.m_L) {
        if (other.isValid()) {
            other.push();
            m_ref = luaL_ref(m_L, LUA_REGISTRYINDEX);
        }
    }

    LuaRef& operator=(const LuaRef& other) {
        if (this != &other) {
            reset();
            m_L = other.m_L;
            if (other.isValid()) {
                other.push();
                m_ref = luaL_ref(m_L, LUA_REGISTRYINDEX);
            }
        }
        return *this;
    }

    // 移動語意 (維持原樣)
    LuaRef(LuaRef&& other) noexcept : m_L(other.m_L), m_ref(other.m_ref) {
        other.m_L = nullptr;
        other.m_ref = LUA_NOREF;
    }

    LuaRef& operator=(LuaRef&& other) noexcept {
        if (this != &other) {
            reset();
            m_L = other.m_L;
            m_ref = other.m_ref;
            other.m_L = nullptr;
            other.m_ref = LUA_NOREF;
        }
        return *this;
    }

    bool isValid() const {
        return m_L != nullptr && m_ref != LUA_NOREF && m_ref != LUA_REFNIL;
    }

    int getRef() const { return m_ref; }
    lua_State* getState() const { return m_L; }

    // 將此引用壓入棧頂 (維持原樣)
    void push() const {
        if (isValid()) {
            lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_ref);
        } else if (m_L) {
            lua_pushnil(m_L);
        }
    }

    // ★ 新增型別檢查輔助函式
    int type() const {
        if (!isValid()) return LUA_TNIL;
        push();
        int t = lua_type(m_L, -1);
        lua_pop(m_L, 1);
        return t;
    }

    bool isFunction() const { return type() == LUA_TFUNCTION; }
    bool isTable() const    { return type() == LUA_TTABLE; }
    bool isNil() const      { return !isValid() || type() == LUA_TNIL; }

    void reset() {
        if (isValid()) {
            luaL_unref(m_L, LUA_REGISTRYINDEX, m_ref);
            m_ref = LUA_NOREF;
            m_L = nullptr;
        }
    }

    // 從目前棧頂擷取值建立 Ref 並自動彈出棧頂 (維持原樣)
    static LuaRef fromStack(lua_State* L) {
        if (!L) return LuaRef();
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);
        return LuaRef(L, ref);
    }

    // ★ 偷看棧頂值建立 Ref (保留棧頂值不彈出)
    static LuaRef fromStackTop(lua_State* L) {
        if (!L) return LuaRef();
        lua_pushvalue(L, -1);
        return fromStack(L);
    }

private:
    lua_State* m_L = nullptr;
    int m_ref = LUA_NOREF;
};

#endif // GIRIGARA_SCRIPT_LUA_REF_H
