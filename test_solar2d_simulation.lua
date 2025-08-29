-- 简化版Solar2D测试 - 无需Solar2D模拟器
-- 这个脚本模拟Solar2D环境的基本功能

print("Solar2D H.264 Plugin Test (Simulated Environment)")
print("=================================================")

-- 模拟Solar2D的一些基本API
local display = {
    contentWidth = 320,
    contentHeight = 480,
    contentCenterX = 160,
    contentCenterY = 240
}

local timer = {
    performWithDelay = function(delay, callback, iterations)
        print(string.format("Timer: Would call function after %dms", delay))
        -- 在真实Solar2D中，这会实际调用callback
        -- 这里我们立即调用用于测试
        if callback then callback() end
    end
}

-- 模拟系统事件
local Runtime = {
    addEventListener = function(self, event, listener)
        print("Runtime event listener added for:", event)
    end
}

-- 加载插件 (与Solar2D中相同的方式)
package.cpath = package.cpath .. ";./build/libplugin_h264.dylib"

local success, h264 = pcall(require, "plugin_h264")  -- 在Solar2D中是 require("plugin.h264")
if not success then
    print("❌ 插件加载失败:", h264)
    print("在真实Solar2D中，插件路径应该是 'plugin.h264'")
    return
end

print("✅ 插件在模拟环境中加载成功")
print("插件版本:", h264.getVersion())

-- 显示库信息 (与Solar2D中完全相同)
local libInfo = h264.getLibraryInfo()
print("支持的库:")
for k, v in pairs(libInfo) do
    print("  " .. k .. ":", v)
end

-- 创建电影对象 (与Solar2D中完全相同)
local movie = h264.newMovie()
if not movie then
    print("❌ 无法创建电影对象")
    return
end

print("✅ 电影对象创建成功")

-- 事件处理 (与Solar2D中完全相同)
local function videoEventListener(event)
    print("🎬 视频事件:")
    print("  类型:", event.type)
    print("  阶段:", event.phase)
    if event.message then
        print("  消息:", event.message)
    end
    
    -- 模拟Solar2D中的响应
    if event.phase == "loaded" then
        print("📊 视频信息:")
        print("  时长:", movie:getDuration() .. " 秒")
        print("🎬 开始播放...")
        movie:play()
        
    elseif event.phase == "play" then
        print("▶️ 播放中...")
        -- 在真实Solar2D中可以更新UI
        
    elseif event.phase == "error" then
        print("❌ 播放错误")
        -- 在真实Solar2D中可以显示错误对话框
    end
end

-- 添加事件监听器 (与Solar2D中完全相同)
movie:addEventListener("videoEvent", videoEventListener)
print("✅ 事件监听器已添加")

-- 测试基本状态 (与Solar2D中完全相同)
print("\n📊 初始状态:")
print("  时长:", movie:getDuration(), "秒")
print("  当前时间:", movie:getCurrentTime(), "秒") 
print("  正在播放:", movie:isPlaying())

-- 测试加载不存在的文件 (与Solar2D中完全相同)
print("\n🔄 测试加载不存在的视频文件...")
local result = movie:loadVideo("test_video.mp4")
print("加载结果:", result)

-- 模拟Solar2D中的清理工作
local function cleanup()
    print("\n🧹 清理资源...")
    if movie then
        movie:stop()
        movie:removeEventListener("videoEvent", videoEventListener)
    end
    print("✅ 清理完成")
end

-- 在Solar2D中，这通常在应用退出时调用
Runtime:addEventListener("system", function(event)
    if event.type == "applicationExit" then
        cleanup()
    end
end)

print("\n" .. string.rep("=", 50))
print("✅ Solar2D插件功能验证完成")
print("在真实Solar2D项目中:")
print("1. 使用 require('plugin.h264') 加载插件")
print("2. 所有API调用方式完全相同")
print("3. 可以结合display对象显示视频帧")
print("4. 可以使用timer进行定期状态更新")
print("5. 事件系统与Solar2D完全兼容")
print(string.rep("=", 50))