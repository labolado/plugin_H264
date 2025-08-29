// H.264 Texture Binding for Solar2D
#pragma once

#include <CoronaLua.h>

extern "C" {

// Main plugin entry point
int luaopen_plugin_h264(lua_State *L);

// Core texture creation function
int newMovieTexture(lua_State *L);

}