# Plugin H264 - Solar2D Video Plugin
cmake_minimum_required(VERSION 3.10)
project(plugin_h264 VERSION 1.0.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
    set(CMAKE_MACOSX_RPATH ON)
elseif(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 编译选项
option(STANDALONE_BUILD "Build standalone version with Corona compatibility layer" ON)

# 第三方库路径
set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
set(OPENH264_DIR ${THIRD_PARTY_DIR}/openh264)
set(FDKAAC_DIR ${THIRD_PARTY_DIR}/fdk-aac)
set(MINIMP4_DIR ${THIRD_PARTY_DIR}/minimp4)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OPENH264_DIR}/codec/api/wels
    ${FDKAAC_DIR}/libAACdec/include
    ${FDKAAC_DIR}/libSYS/include
    ${MINIMP4_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include/lua
    ${LUA_INCLUDE_DIRS}
)

# 查找OpenH264库
find_library(OPENH264_LIBRARY
    NAMES openh264
    PATHS ${OPENH264_DIR}
    NO_DEFAULT_PATH
)

# 查找FDK-AAC库
find_library(FDKAAC_LIBRARY
    NAMES fdk-aac
    PATHS ${FDKAAC_DIR}/build
    NO_DEFAULT_PATH
)

# 查找Lua库
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(LUA lua5.4 lua)
endif()

if(NOT LUA_FOUND)
    find_library(LUA_LIBRARIES
        NAMES liblua.dylib liblua5.4.dylib lua5.4 lua54 lua
        PATHS /opt/homebrew/lib /usr/local/lib /usr/lib
    )
    find_path(LUA_INCLUDE_DIRS
        NAMES lua.h
        PATHS /opt/homebrew/include/lua /opt/homebrew/include/lua5.4 
              /usr/local/include/lua5.4 /usr/local/include/lua
              /usr/include/lua5.4 /usr/include/lua
    )
endif()

# 源文件
set(PLUGIN_SOURCES
    src/decoders/H264Decoder.cpp
    src/decoders/AACDecoder.cpp
    src/decoders/MP4Demuxer.cpp
    src/managers/DecoderManager.cpp
    src/managers/H264Movie.cpp
    src/utils/ErrorHandler.cpp
    src/utils/MiniMP4Implementation.cpp
    src/lua/H264TextureBinding.cpp
)

# 添加Corona兼容层（仅用于独立测试）
if(STANDALONE_BUILD)
    list(APPEND PLUGIN_SOURCES src/lua/CoronaMinimal.cpp)
    message(STATUS "Building standalone version with Corona compatibility layer")
else()
    message(STATUS "Building Solar2D deployment version (requires Solar2D runtime)")
endif()

# 头文件
set(PLUGIN_HEADERS
    include/decoders/H264Decoder.h
    include/decoders/AACDecoder.h
    include/decoders/MP4Demuxer.h
    include/managers/DecoderManager.h
    include/managers/H264Movie.h
    include/utils/ErrorHandler.h
    include/utils/Common.h
    include/lua/H264TextureBinding.h
)

# 创建静态库
add_library(plugin_h264_static STATIC ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})
target_link_libraries(plugin_h264_static ${OPENH264_LIBRARY} ${FDKAAC_LIBRARY} ${LUA_LIBRARIES})

# 创建动态库 (用于Solar2D插件)
add_library(plugin_h264 SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})
target_link_libraries(plugin_h264 ${OPENH264_LIBRARY} ${FDKAAC_LIBRARY} ${LUA_LIBRARIES})

# 测试选项
option(ENABLE_TESTING "Enable unit tests" ON)
option(ENABLE_COVERAGE "Enable code coverage" OFF)

if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# 代码覆盖率
if(ENABLE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# 安装配置
install(TARGETS plugin_h264_static plugin_h264
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)