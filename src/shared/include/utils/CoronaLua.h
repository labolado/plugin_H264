// Minimal Corona SDK headers for compilation testing
// In production, these would be provided by Solar2D SDK

#ifndef _CORONA_LUA_H_
#define _CORONA_LUA_H_

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

// Corona external texture callback types
typedef void (*CoronaExternalTextureFinalize)(void* context);
typedef unsigned int (*CoronaExternalTextureGetWidth)(void* context);
typedef unsigned int (*CoronaExternalTextureGetHeight)(void* context);
typedef const void* (*CoronaExternalTextureGetBitmap)(void* context);

typedef enum {
    kExternalBitmapFormat_RGBA = 0
} CoronaExternalBitmapFormat;

typedef CoronaExternalBitmapFormat (*CoronaExternalTextureGetFormat)(void* context);
typedef int (*CoronaExternalTextureGetField)(lua_State *L, const char *field, void *context);

typedef struct CoronaExternalTextureCallbacks {
    unsigned int size;
    CoronaExternalTextureFinalize onFinalize;
    CoronaExternalTextureGetWidth getWidth;
    CoronaExternalTextureGetHeight getHeight;
    CoronaExternalTextureGetBitmap onRequestBitmap;
    CoronaExternalTextureGetFormat getFormat;
    CoronaExternalTextureGetField onGetField;
} CoronaExternalTextureCallbacks;

// Corona library functions
extern "C" {
    int CoronaExternalPushTexture(lua_State *L, const CoronaExternalTextureCallbacks *callbacks, void *context);
    void* CoronaExternalGetUserData(lua_State *L, int index);
    
    // Minimal Corona Library support
    typedef lua_CFunction (*CoronaLuaOpenFunction)(lua_State *L);
    int CoronaLibraryNewWithFactory(lua_State *L, lua_CFunction factory, void *context, void *platformContext);
}

#endif // _CORONA_LUA_H_