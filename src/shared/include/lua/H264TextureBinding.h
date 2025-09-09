// H.264 Texture Binding for Solar2D
#pragma once

#include "CoronaLua.h"

CORONA_EXPORT int CoronaPluginLuaLoad_plugin_h264(lua_State *);

// Main plugin entry point
CORONA_EXPORT int luaopen_plugin_h264(lua_State *L);

// Core texture creation function
static int newMovieTexture(lua_State *L);

static int PushCachedFunction( lua_State *L, lua_CFunction f )
{
    // check cache for the funciton, cache key is function address
    lua_pushlightuserdata(L, (void *)f);
    lua_gettable(L, LUA_REGISTRYINDEX);

    // cahce miss
    if (!lua_iscfunction(L, -1))
    {
        lua_pop(L, 1); // pop nil on top of stack

        // create c function closure on top of stack
        lua_pushcfunction(L, f);

        // push cache key
        lua_pushlightuserdata(L, (void *)f);
        // copy function to be on top of stack as well
        lua_pushvalue(L, -2);
        lua_settable(L, LUA_REGISTRYINDEX);

        // now original function is on top of stack, and cache key and function is in cache
    }

    return 1;
}

static int update(lua_State *L);
static int play(lua_State *L);
static int pause(lua_State *L);
static int stop(lua_State *L);
static int invalidate(lua_State *L);
static int isActive(lua_State *L, void *context);
static int isPlaying(lua_State *L, void *context);
static int currentTime(lua_State *L, void *context);
static int seek(lua_State *L);
static int replay(lua_State *L);
