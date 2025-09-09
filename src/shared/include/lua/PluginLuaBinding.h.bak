// Solar2D Lua Plugin Interface for H264 Video
#pragma once

#include "CoronaLua.h"
#include "CoronaMacros.h"
#include "CoronaEvent.h"
#include "managers/H264Movie.h"
#include <memory>

// Plugin名称和版本信息
#define PLUGIN_NAME "plugin.h264"
#define PLUGIN_VERSION "1.0.0"

// Lua事件类型
#define EVENT_TYPE_VIDEO "videoEvent"

namespace plugin_h264 {

// Solar2D插件上下文
struct PluginContext {
    std::unique_ptr<H264Movie> movie;
    lua_State* luaState;
    CoronaLuaRef listenerRef;
    int movieRef;
    
    PluginContext() : luaState(nullptr), listenerRef(CoronaLuaRefInvalid), movieRef(LUA_REFNIL) {}
};

// Lua绑定函数声明
extern "C" {
    // 插件初始化
    CORONA_EXPORT int luaopen_plugin_h264(lua_State* L);
    
    // H264电影对象方法
    static int newMovie(lua_State* L);
    static int loadVideo(lua_State* L);
    static int play(lua_State* L);
    static int pause(lua_State* L);
    static int stop(lua_State* L);
    static int seek(lua_State* L);
    static int isPlaying(lua_State* L);
    static int getDuration(lua_State* L);
    static int getCurrentTime(lua_State* L);
    static int setVolume(lua_State* L);
    static int addEventListener(lua_State* L);
    static int removeEventListener(lua_State* L);
    static int __gc(lua_State* L);
    static int __index(lua_State* L);
    
    // 插件级别函数
    static int getVersion(lua_State* L);
    static int getLibraryInfo(lua_State* L);
}

// 辅助函数
void dispatchVideoEvent(PluginContext* context, const std::string& phase, const std::string& message = "");
PluginContext* getPluginContext(lua_State* L, int index);
void setupMovieMetatable(lua_State* L);

} // namespace plugin_h264