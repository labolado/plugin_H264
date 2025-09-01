// Corona types for standalone build compatibility
#pragma once

#include <CoronaLua.h>

// Define missing Corona types for standalone build
typedef void (*CoronaExternalTextureFinalize)(void* context);
typedef unsigned int (*CoronaExternalTextureGetWidth)(void* context);  
typedef unsigned int (*CoronaExternalTextureGetHeight)(void* context);
typedef void (*CoronaExternalTextureGetBitmap)(void* context, void* bitmap);

// Missing bitmap format enum
typedef enum {
    kExternalBitmapFormat_RGBA = 0
} CoronaExternalBitmapFormat;

// Missing callback types
typedef CoronaExternalBitmapFormat (*CoronaExternalTextureGetFormat)(void* context);
typedef int (*CoronaExternalTextureGetField)(lua_State *L, const char *field, void *context);

typedef struct CoronaExternalTextureCallbacks {
    unsigned int size;
    CoronaExternalTextureFinalize onFinalize;
    CoronaExternalTextureGetWidth getWidth;
    CoronaExternalTextureGetHeight getHeight;
    CoronaExternalTextureGetBitmap onRequestBitmap;
    CoronaExternalTextureGetFormat getFormat;    // 添加格式回调
    CoronaExternalTextureGetField onGetField;    // 添加字段回调
} CoronaExternalTextureCallbacks;

// Function declarations
extern "C" {
    int CoronaExternalPushTexture(lua_State *L, const CoronaExternalTextureCallbacks *callbacks, void *context);
    void* CoronaExternalGetUserData(lua_State *L, int index);
}