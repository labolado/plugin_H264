# H.264 Video Plugin for Solar2D - 完整编译指南

## 概述

本插件为Solar2D提供H.264视频解码功能，支持跨平台编译（iOS、Android、macOS、Windows）。

## 系统要求

### 通用要求
- CMake 3.10+
- Git
- 现代C++编译器（支持C++14）

### macOS
```bash
# 安装必要工具
brew install cmake automake lua pkg-config
```

### Ubuntu/Linux
```bash
# 安装必要依赖
sudo apt-get update
sudo apt-get install -y build-essential cmake git nasm lua5.4-dev pkg-config
sudo apt-get install -y autotools-dev autoconf libtool
```

### Windows
```powershell
# 使用 Chocolatey 安装
choco install cmake git
# 需要安装 Visual Studio 2019/2022 with C++ tools
# 手动安装 Lua 5.4
```

## 第三方依赖库

### 1. OpenH264 (Cisco)
```bash
cd third_party
git clone --depth 1 https://github.com/cisco/openh264.git
cd openh264

# macOS/Linux
make -j$(nproc)
sudo make install PREFIX=/usr/local
sudo ldconfig  # Linux only

# Windows (使用 Visual Studio Developer Command Prompt)
# msbuild codec/build/win32/dec/welsdec.vcxproj /p:Configuration=Release
```

### 2. FDK-AAC (Fraunhofer)
```bash
cd third_party
git clone --depth 1 --branch v2.0.3 https://github.com/mstorsjo/fdk-aac.git
cd fdk-aac

# 配置和编译
autoreconf -fiv
./configure
make -j$(nproc)
sudo make install
```

### 3. MiniMP4 (头文件库)
```bash
cd third_party
git clone --depth 1 https://github.com/lieff/minimp4.git
# 头文件库，无需编译
```

## 编译步骤

### 1. 独立构建 (用于测试)
```bash
# 创建构建目录
mkdir -p build && cd build

# 配置CMake
cmake .. -DSTANDALONE_BUILD=ON

# 编译
make -j$(nproc)  # Linux/macOS
# 或 cmake --build . --config Release  # Windows
```

生成文件：
- `libplugin_h264_static.a` - 静态库 (~93KB)
- `libplugin_h264.dylib/.so/.dll` - 动态库

### 2. Solar2D插件构建

#### iOS静态库
```bash
# 复制编译产物到插件目录
cp build/libplugin_h264_static.a plugins/2020.3620/iphone/libplugin_h264.a
cp build/libplugin_h264_static.a plugins/2020.3620/iphone-sim/libplugin_h264.a
```

#### Android AAR
```bash
# 创建Android项目结构
mkdir -p android_build/src/main/java/com/plugin/h264

# 使用提供的 build.gradle 和 AndroidManifest.xml
# 编译AAR
cd android_build
zip -r ../plugins/2020.3620/android/plugin.h264.aar . -x "*.DS_Store*"
```

## 文件结构

```
plugin_H264/
├── plugins/2020.3620/           # Solar2D插件标准目录
│   ├── android/
│   │   ├── metadata.lua         # Android插件元数据
│   │   └── plugin.h264.aar     # Android库文件 (~2.4KB)
│   ├── iphone/
│   │   ├── metadata.lua         # iOS插件元数据
│   │   └── libplugin_h264.a    # iOS静态库 (~93KB)
│   ├── iphone-sim/
│   │   ├── metadata.lua
│   │   └── libplugin_h264.a    # iOS模拟器静态库 (~93KB)
│   ├── mac-sim/
│   │   └── metadata.lua
│   └── win32-sim/
│       └── metadata.lua
├── src/                        # 源代码
│   ├── decoders/              # H.264/AAC解码器
│   ├── managers/              # 解码管理器
│   ├── lua/                   # Lua绑定
│   └── utils/                 # 工具类
├── include/                   # 头文件
├── third_party/              # 第三方库
│   ├── openh264/             # H.264编解码库 (~1.1MB)
│   ├── fdk-aac/              # AAC编解码库 (~1.9MB)
│   └── minimp4/              # MP4容器库
└── shared/plugin_h264.lua    # Lua API封装
```

## API使用示例

```lua
local h264 = require("plugin.h264")

-- 创建视频显示对象
local movie = h264.newMovieRect({
    width = 640,
    height = 480,
    filename = "sample.mp4"
})

-- 添加到显示组
local group = display.newGroup()
group:insert(movie)

-- 播放控制
movie:play()
movie:pause()
movie:stop()
```

## GitHub Actions自动构建

项目配置了多个GitHub Actions工作流：

### 1. 传统构建系统 (.github/workflows/build-test.yml)
- 支持 Linux、macOS、Windows
- 自动下载和编译第三方依赖
- 生成测试报告

### 2. Solar2D插件发布 (.github/workflows/solar2d-publish.yml)
- 使用官方 `solar2d/directory-plugin-action@main`
- 触发条件：`plugins/**` 或 `docs/**` 路径变更
- 自动生成跨平台插件包

## 常见问题

### 1. "nasm: No such file or directory"
```bash
# Ubuntu/Debian
sudo apt-get install nasm

# macOS
brew install nasm

# Windows
# 从 https://www.nasm.us/ 下载安装
```

### 2. "lua: Package not found"
```bash
# Ubuntu
sudo apt-get install lua5.4-dev

# macOS
brew install lua

# Windows - 手动安装或使用预编译包
```

### 3. "Too many retries" (Solar2D Action)
- 检查 GitHub Secrets 中的 `ISSUE_PAT` 配置
- 确保插件结构符合Solar2D标准
- 验证二进制文件大小（不应为占位符）

### 4. 编译产物大小验证
正确的编译产物大小应该为：
- iOS静态库：~93KB (不是38字节占位符)
- Android AAR：~2.4KB (不是36字节占位符)
- OpenH264库：~1.1MB
- FDK-AAC库：~1.9MB

## 开发测试

### 本地测试
```bash
# 编译并运行测试
cd build
make -j$(nproc)
ctest --verbose

# 手动测试
./test_h264_decoder sample.mp4
```

### Solar2D集成测试
1. 将插件复制到Solar2D项目的 `plugins/` 目录
2. 在 `build.settings` 中添加插件依赖
3. 使用提供的Lua API进行功能测试

## 贡献指南

1. Fork项目并创建特性分支
2. 确保所有编译步骤成功
3. 运行完整测试套件
4. 提交Pull Request

---

**注意**：此文档基于当前实现状态编写，随着项目发展可能需要更新。如遇问题请参考项目Issues或提交新的Issue。