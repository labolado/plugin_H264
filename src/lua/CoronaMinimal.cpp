// 最小Corona SDK兼容层 - 仅用于独立编译测试
#include <CoronaLua.h>
// CoronaGraphics.h not available in standalone build - using local definitions
#include <cstring>
#include "../../include/lua/CoronaTypes.h"

extern "C" {

// 最小纹理用户数据结构
struct TextureUserData {
    CoronaExternalTextureCallbacks callbacks;
    void* context;
};

int CoronaExternalPushTexture(lua_State *L, const CoronaExternalTextureCallbacks *callbacks, void *context) {
    TextureUserData *ud = (TextureUserData*)lua_newuserdata(L, sizeof(TextureUserData));
    ud->callbacks = *callbacks;
    ud->context = context;
    
    if (luaL_newmetatable(L, "CoronaTexture")) {
        lua_pushcfunction(L, [](lua_State *L) -> int {
            TextureUserData *ud = (TextureUserData*)luaL_checkudata(L, 1, "CoronaTexture");
            if (ud && ud->callbacks.onFinalize) {
                ud->callbacks.onFinalize(ud->context);
            }
            return 0;
        });
        lua_setfield(L, -2, "__gc");
        
        lua_pushcfunction(L, [](lua_State *L) -> int {
            // Simple __index - just return nil for now
            lua_pushnil(L);
            return 1;
        });
        lua_setfield(L, -2, "__index");
    }
    lua_setmetatable(L, -2);
    
    return 1;
}

void* CoronaExternalGetUserData(lua_State *L, int index) {
    TextureUserData *ud = (TextureUserData*)luaL_checkudata(L, index, "CoronaTexture");
    return ud ? ud->context : nullptr;
}

} // extern "C"