// Basic Corona SDK/Solar2D Lua API definitions
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "/opt/homebrew/include/lua5.4/lua.h"
#include "/opt/homebrew/include/lua5.4/lauxlib.h"

// Corona Lua引用类型
typedef int CoronaLuaRef;
#define CoronaLuaRefInvalid LUA_REFNIL

// Corona宏定义
#ifndef CORONA_EXPORT
#define CORONA_EXPORT
#endif

// Corona Lua函数声明
CoronaLuaRef CoronaLuaNewRef(lua_State* L, int index);
void CoronaLuaDeleteRef(lua_State* L, CoronaLuaRef ref);
void CoronaLuaNewEvent(lua_State* L, const char* eventType);
void CoronaLuaDispatchEvent(lua_State* L, CoronaLuaRef listener, int nresults);

#ifdef __cplusplus
}
#endif