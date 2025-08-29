#include "../include/lua/CoronaLua.h"
#include <map>
#include <cstring>

// Corona Lua API基础实现
static std::map<int, int> g_luaRefs;
static int g_refCounter = 1;

extern "C" {

CoronaLuaRef CoronaLuaNewRef(lua_State* L, int index) {
    if (lua_isnil(L, index)) {
        return CoronaLuaRefInvalid;
    }
    
    lua_pushvalue(L, index);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    int coronaRef = g_refCounter++;
    g_luaRefs[coronaRef] = ref;
    
    return coronaRef;
}

void CoronaLuaDeleteRef(lua_State* L, CoronaLuaRef ref) {
    if (ref == CoronaLuaRefInvalid) {
        return;
    }
    
    auto it = g_luaRefs.find(ref);
    if (it != g_luaRefs.end()) {
        luaL_unref(L, LUA_REGISTRYINDEX, it->second);
        g_luaRefs.erase(it);
    }
}

void CoronaLuaNewEvent(lua_State* L, const char* eventType) {
    lua_createtable(L, 0, 2);
    
    lua_pushstring(L, "event");
    lua_setfield(L, -2, "name");
    
    lua_pushstring(L, eventType);
    lua_setfield(L, -2, "type");
}

void CoronaLuaDispatchEvent(lua_State* L, CoronaLuaRef listener, int nresults) {
    if (listener == CoronaLuaRefInvalid) {
        lua_pop(L, 1); // 弹出事件表
        return;
    }
    
    auto it = g_luaRefs.find(listener);
    if (it == g_luaRefs.end()) {
        lua_pop(L, 1); // 弹出事件表
        return;
    }
    
    // 获取监听器函数
    lua_rawgeti(L, LUA_REGISTRYINDEX, it->second);
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 2); // 弹出函数和事件表
        return;
    }
    
    // 交换栈顶的函数和事件表
    lua_insert(L, -2);
    
    // 调用监听器函数
    int result = lua_pcall(L, 1, nresults, 0);
    if (result != 0) {
        // 处理错误
        const char* errorMsg = lua_tostring(L, -1);
        printf("Corona event dispatch error: %s\n", errorMsg ? errorMsg : "Unknown error");
        lua_pop(L, 1);
    }
}

} // extern "C"