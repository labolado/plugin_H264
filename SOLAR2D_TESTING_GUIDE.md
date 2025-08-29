# Solar2D H.264 插件测试指南

## 🎯 针对Solar2D的正确测试方法

### 1. 插件安装测试

首先需要将插件正确安装到Solar2D中：

```bash
# 方法1: 复制到Solar2D插件目录
cp build/libplugin_h264.dylib ~/Documents/Solar2D/Plugins/

# 方法2: 在项目中直接使用
mkdir -p your_project/Plugins/
cp build/libplugin_h264.dylib your_project/Plugins/
```

### 2. Solar2D模拟器测试

**步骤1: 使用示例项目**
```bash
cd examples/solar2d_project
# 在Solar2D Simulator中打开这个目录
```

**步骤2: 添加测试视频**
- 将MP4视频文件重命名为 `sample_video.mp4`
- 放到 `examples/solar2d_project/` 目录中

**步骤3: 运行测试**
- 在Solar2D Simulator中运行项目
- 观察控制台输出
- 测试加载、播放、暂停等功能

### 3. 在现有Solar2D项目中测试

如果你有现有的Solar2D项目：

1. **修改build.settings**
```lua
settings = {
    plugins = {
        ["plugin.h264"] = {
            publisherId = "com.yourcompany"
        }
    }
}
```

2. **在main.lua中测试**
```lua
local h264 = require("plugin.h264")

print("Plugin version:", h264.getVersion())

local movie = h264.newMovie()
movie:addEventListener("videoEvent", function(event)
    print("Video event:", event.phase)
end)

-- 测试加载视频
movie:loadVideo("your_video.mp4")
```

### 4. 设备测试

对于真机测试：
- iOS: 需要编译iOS版本的插件
- Android: 需要编译Android版本的插件
- macOS: 当前版本可以直接用

## ⚠️ 当前测试方法的问题

之前的Lua直接测试只能验证：
- ✅ 插件C++代码正确性
- ✅ Lua绑定功能
- ❌ 无法测试Solar2D集成
- ❌ 无法测试display对象
- ❌ 无法测试实际应用场景

## 🎬 推荐测试流程

1. **开发阶段**: 用直接Lua测试验证核心功能
2. **集成阶段**: 用Solar2D示例项目测试
3. **部署阶段**: 在真实Solar2D应用中测试