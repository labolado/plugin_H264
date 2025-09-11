// Minimal Corona SDK macros for compilation testing
#ifndef _CORONA_MACROS_H_
#define _CORONA_MACROS_H_

#ifdef __cplusplus
#define CORONA_API extern "C"
#define CORONA_EXPORT extern "C" __attribute__ ((visibility ("default")))
#else
#define CORONA_API
#define CORONA_EXPORT __attribute__ ((visibility ("default")))
#endif

// Corona Lua namespace
namespace Corona {
    namespace Lua {
        template<lua_CFunction f>
        int Open(lua_State *L) {
            return f(L);
        }
    }
}

#endif // _CORONA_MACROS_H_