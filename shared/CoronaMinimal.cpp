// Minimal Corona Library implementation for compilation testing
// This would be provided by Solar2D runtime in production

#include "../include/CoronaLua.h"
#include <cstring>

extern "C" {

// Minimal texture user data structure
struct TextureUserData {
    CoronaExternalTextureCallbacks callbacks;
    void* context;
};

int CoronaExternalPushTexture(lua_State *L, const CoronaExternalTextureCallbacks *callbacks, void *context) {
    TextureUserData *ud = (TextureUserData*)lua_newuserdata(L, sizeof(TextureUserData));
    ud->callbacks = *callbacks;
    ud->context = context;
    
    if (luaL_newmetatable(L, "CoronaTexture")) {
        // Add __gc metamethod for cleanup
        lua_pushcfunction(L, [](lua_State *L) -> int {
            TextureUserData *ud = (TextureUserData*)luaL_checkudata(L, 1, "CoronaTexture");
            if (ud && ud->callbacks.onFinalize) {
                ud->callbacks.onFinalize(ud->context);
            }
            return 0;
        });
        lua_setfield(L, -2, "__gc");
        
        // Add __index metamethod for field access
        lua_pushcfunction(L, [](lua_State *L) -> int {
            TextureUserData *ud = (TextureUserData*)luaL_checkudata(L, 1, "CoronaTexture");
            const char *field = lua_tostring(L, 2);
            
            if (ud && ud->callbacks.onGetField && field) {
                return ud->callbacks.onGetField(L, field, ud->context);
            }
            
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

int CoronaLibraryNewWithFactory(lua_State *L, lua_CFunction factory, void *context, void *platformContext) {
    // Minimal implementation - just call the factory function
    return factory(L);
}

} // extern "C"