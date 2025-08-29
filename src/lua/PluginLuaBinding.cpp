#include "../include/lua/PluginLuaBinding.h"
#include <cstring>
#include <iostream>

namespace plugin_h264 {

// Metatable名称
static const char* MOVIE_METATABLE = "plugin.h264.movie";

// 创建新的H264电影对象
static int newMovie(lua_State* L) {
    try {
        // 创建插件上下文
        PluginContext* context = (PluginContext*)lua_newuserdata(L, sizeof(PluginContext));
        new(context) PluginContext(); // placement new
        
        context->luaState = L;
        context->movie = std::make_unique<H264Movie>();
        
        // 设置metatable
        luaL_getmetatable(L, MOVIE_METATABLE);
        lua_setmetatable(L, -2);
        
        return 1;
    } catch (const std::exception& e) {
        return luaL_error(L, "Failed to create H264 movie: %s", e.what());
    }
}

// 加载视频文件
static int loadVideo(lua_State* L) {
    PluginContext* context = getPluginContext(L, 1);
    if (!context || !context->movie) {
        return luaL_error(L, "Invalid movie object");
    }
    
    const char* filePath = luaL_checkstring(L, 2);
    if (!filePath) {
        return luaL_error(L, "File path is required");
    }
    
    bool success = context->movie->loadFromFile(filePath);
    lua_pushboolean(L, success);
    
    if (!success && context->movie->hasError()) {
        std::string errorMsg = context->movie->getLastMessage();
        dispatchVideoEvent(context, "error", errorMsg);
    } else if (success) {
        dispatchVideoEvent(context, "loaded", "Video loaded successfully");
    }
    
    return 1;
}

// 播放视频
static int play(lua_State* L) {
    PluginContext* context = getPluginContext(L, 1);
    if (!context || !context->movie) {
        return luaL_error(L, "Invalid movie object");
    }
    
    bool success = context->movie->play();
    lua_pushboolean(L, success);
    
    if (success) {
        dispatchVideoEvent(context, "play", "Video started playing");
    }
    
    return 1;
}

// 暂停视频
static int pause(lua_State* L) {
    PluginContext* context = getPluginContext(L, 1);
    if (!context || !context->movie) {
        return luaL_error(L, "Invalid movie object");
    }
    
    context->movie->pause();
    lua_pushboolean(L, true);
    
    dispatchVideoEvent(context, "pause", "Video paused");
    return 1;
}

// 停止视频
static int stop(lua_State* L) {
    PluginContext* context = getPluginContext(L, 1);
    if (!context || !context->movie) {
        return luaL_error(L, "Invalid movie object");
    }
    
    context->movie->stop();
    lua_pushboolean(L, true);
    
    dispatchVideoEvent(context, "stop", "Video stopped");
    return 1;
}

// 跳转到指定时间
static int seek(lua_State* L) {
    PluginContext* context = getPluginContext(L, 1);
    if (!context || !context->movie) {
        return luaL_error(L, "Invalid movie object");
    }
    
    double timeMs = luaL_checknumber(L, 2);
    bool success = context->movie->seekTo(timeMs);
    lua_pushboolean(L, success);
    
    return 1;
}

// 检查是否正在播放
static int isPlaying(lua_State* L) {
    PluginContext* context = getPluginContext(L, 1);
    if (!context || !context->movie) {
        lua_pushboolean(L, false);
        return 1;
    }
    
    lua_pushboolean(L, context->movie->isPlaying());
    return 1;
}

// 获取视频时长
static int getDuration(lua_State* L) {
    PluginContext* context = getPluginContext(L, 1);
    if (!context || !context->movie) {
        lua_pushnumber(L, 0.0);
        return 1;
    }
    
    lua_pushnumber(L, context->movie->getDuration());
    return 1;
}

// 获取当前播放时间
static int getCurrentTime(lua_State* L) {
    PluginContext* context = getPluginContext(L, 1);
    if (!context || !context->movie) {
        lua_pushnumber(L, 0.0);
        return 1;
    }
    
    lua_pushnumber(L, context->movie->getCurrentTime());
    return 1;
}

// 设置音量
static int setVolume(lua_State* L) {
    PluginContext* context = getPluginContext(L, 1);
    if (!context || !context->movie) {
        return luaL_error(L, "Invalid movie object");
    }
    
    float volume __attribute__((unused)) = (float)luaL_checknumber(L, 2);
    // setVolume方法需要在H264Movie中实现
    // context->movie->setVolume(volume);
    lua_pushboolean(L, true);
    
    return 1;
}

// 添加事件监听器
static int addEventListener(lua_State* L) {
    PluginContext* context = getPluginContext(L, 1);
    if (!context) {
        return luaL_error(L, "Invalid movie object");
    }
    
    const char* eventType = luaL_checkstring(L, 2);
    if (strcmp(eventType, EVENT_TYPE_VIDEO) != 0) {
        return luaL_error(L, "Unsupported event type: %s", eventType);
    }
    
    // 检查第三个参数是否为函数
    if (!lua_isfunction(L, 3)) {
        return luaL_error(L, "Event listener must be a function");
    }
    
    // 释放之前的监听器引用
    if (context->listenerRef != CoronaLuaRefInvalid) {
        CoronaLuaDeleteRef(L, context->listenerRef);
    }
    
    // 存储新的监听器引用
    lua_pushvalue(L, 3);
    context->listenerRef = CoronaLuaNewRef(L, -1);
    lua_pop(L, 1);
    
    lua_pushboolean(L, true);
    return 1;
}

// 移除事件监听器
static int removeEventListener(lua_State* L) {
    PluginContext* context = getPluginContext(L, 1);
    if (!context) {
        return luaL_error(L, "Invalid movie object");
    }
    
    if (context->listenerRef != CoronaLuaRefInvalid) {
        CoronaLuaDeleteRef(L, context->listenerRef);
        context->listenerRef = CoronaLuaRefInvalid;
    }
    
    lua_pushboolean(L, true);
    return 1;
}

// 垃圾回收
static int __gc(lua_State* L) {
    PluginContext* context = (PluginContext*)lua_touserdata(L, 1);
    if (context) {
        // 清理事件监听器
        if (context->listenerRef != CoronaLuaRefInvalid) {
            CoronaLuaDeleteRef(L, context->listenerRef);
        }
        
        // 调用析构函数
        context->~PluginContext();
    }
    return 0;
}

// __index元方法
static int __index(lua_State* L) {
    return luaL_error(L, "Attempt to read undefined property");
}

// 获取插件版本
static int getVersion(lua_State* L) {
    lua_pushstring(L, PLUGIN_VERSION);
    return 1;
}

// 获取库信息
static int getLibraryInfo(lua_State* L) {
    lua_createtable(L, 0, 3);
    
    lua_pushstring(L, "OpenH264");
    lua_setfield(L, -2, "h264decoder");
    
    lua_pushstring(L, "FDK-AAC");
    lua_setfield(L, -2, "aacdecoder");
    
    lua_pushstring(L, "MiniMP4");
    lua_setfield(L, -2, "mp4demuxer");
    
    return 1;
}

// 辅助函数实现
void dispatchVideoEvent(PluginContext* context, const std::string& phase, const std::string& message) {
    if (!context || context->listenerRef == CoronaLuaRefInvalid) {
        return;
    }
    
    lua_State* L = context->luaState;
    
    // 创建事件表
    CoronaLuaNewEvent(L, EVENT_TYPE_VIDEO);
    
    lua_pushstring(L, phase.c_str());
    lua_setfield(L, -2, "phase");
    
    if (!message.empty()) {
        lua_pushstring(L, message.c_str());
        lua_setfield(L, -2, "message");
    }
    
    // 分发事件
    CoronaLuaDispatchEvent(L, context->listenerRef, 0);
}

PluginContext* getPluginContext(lua_State* L, int index) {
    return (PluginContext*)luaL_checkudata(L, index, MOVIE_METATABLE);
}

void setupMovieMetatable(lua_State* L) {
    luaL_newmetatable(L, MOVIE_METATABLE);
    
    // 设置方法
    static const luaL_Reg movieMethods[] = {
        {"loadVideo", loadVideo},
        {"play", play},
        {"pause", pause},
        {"stop", stop},
        {"seek", seek},
        {"isPlaying", isPlaying},
        {"getDuration", getDuration},
        {"getCurrentTime", getCurrentTime},
        {"setVolume", setVolume},
        {"addEventListener", addEventListener},
        {"removeEventListener", removeEventListener},
        {"__gc", __gc},
        {"__index", __index},
        {NULL, NULL}
    };
    
    luaL_setfuncs(L, movieMethods, 0);
    
    // 设置__index指向自己
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);
    
    lua_pop(L, 1); // 弹出metatable
}

// 插件初始化入口点
extern "C" CORONA_EXPORT int luaopen_plugin_h264(lua_State* L) {
    // 设置电影对象的metatable
    setupMovieMetatable(L);
    
    // 创建插件模块表
    static const luaL_Reg pluginFunctions[] = {
        {"newMovie", newMovie},
        {"getVersion", getVersion},
        {"getLibraryInfo", getLibraryInfo},
        {NULL, NULL}
    };
    
    luaL_newlib(L, pluginFunctions);
    
    return 1;
}

} // namespace plugin_h264