// Corona types for standalone build compatibility
#pragma once

#include <CoronaLua.h>

// Define missing Corona types for standalone build
typedef void (*CoronaExternalTextureFinalize)(void* context);
typedef unsigned int (*CoronaExternalTextureGetWidth)(void* context);  
typedef unsigned int (*CoronaExternalTextureGetHeight)(void* context);
typedef void (*CoronaExternalTextureGetBitmap)(void* context, void* bitmap);

typedef struct CoronaExternalTextureCallbacks {
    unsigned int size;
    CoronaExternalTextureFinalize onFinalize;
    CoronaExternalTextureGetWidth getWidth;
    CoronaExternalTextureGetHeight getHeight;
    CoronaExternalTextureGetBitmap onRequestBitmap;
} CoronaExternalTextureCallbacks;

// Function declarations
extern "C" {
    int CoronaExternalPushTexture(lua_State *L, const CoronaExternalTextureCallbacks *callbacks, void *context);
    void* CoronaExternalGetUserData(lua_State *L, int index);
}