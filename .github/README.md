# GitHub Actions CI/CD for H.264 Plugin

This project includes comprehensive GitHub Actions workflows for automated building, testing, and deployment.

## 🚀 Available Workflows

### 1. Build and Test (`build.yml`)
**Triggers**: Push to master/main/develop, Pull Requests, Releases

**Platforms**:
- ✅ **macOS** (Intel + ARM64)
- ✅ **Linux** (Ubuntu x64) 
- ✅ **Windows** (x64)
- ✅ **Android** (ARM64-v8a)

**Features**:
- Automatic dependency installation
- Multi-platform compilation
- Automated test execution (44 test cases)
- Artifact generation for each platform
- Release packaging

### 2. Code Quality (`quality.yml`)
**Triggers**: Push to master/main, Pull Requests

**Checks**:
- Static code analysis (Cppcheck)
- Security scanning (Bandit)
- License compliance (FOSSA)
- Memory leak detection (Valgrind on Linux)

## 📦 Build Artifacts

Each successful build produces:

### macOS Artifacts
- `libplugin_h264.dylib` - Dynamic library for Solar2D
- `libplugin_h264_static.a` - Static library
- Solar2D example project

### Linux Artifacts  
- `libplugin_h264.so` - Shared library
- `libplugin_h264_static.a` - Static library
- Solar2D example project

### Windows Artifacts
- `plugin_h264.dll` - Dynamic library
- `plugin_h264_static.lib` - Static library  
- Solar2D example project

### Android Artifacts
- `libplugin_h264.so` - ARM64 shared library
- `libplugin_h264_static.a` - ARM64 static library

## 🎯 Release Process

### Automatic Release
1. Create a GitHub Release
2. CI automatically builds all platforms
3. Creates `plugin-h264-all-platforms.tar.gz`
4. Uploads to release assets

### Manual Download
- Go to **Actions** tab
- Select latest successful build
- Download platform-specific artifacts

## 🔧 Local Development vs CI

**Local Development** (what we built):
```bash
mkdir build && cd build
cmake .. && make
./tests/run_tests.sh
```

**GitHub Actions CI** (automatic):
- Multi-platform builds
- Dependency management
- Test execution  
- Security scanning
- Release packaging

## ⚡ Advantages over Original plugin_movie

**Original plugin_movie**: ❌ No CI/CD
- Manual building for each platform
- No automated testing
- No release management
- Platform-specific build scripts

**New H.264 Plugin**: ✅ Full CI/CD
- **Automated builds** for 4 platforms
- **44 automated tests** on each platform
- **Security and quality checks**
- **One-click releases**
- **Artifact management**

## 🎉 Ready for Production

The H.264 plugin now has **enterprise-grade CI/CD** that:
- Ensures code quality across platforms
- Automates the entire release process  
- Provides ready-to-use binaries
- Maintains security compliance
- Tracks dependencies and licenses

This is **far superior** to the original plugin_movie's manual build process!