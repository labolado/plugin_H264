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

# 添加FFmpeg支持选项
option(USE_FFMPEG "Use FFmpeg instead of OpenH264 for decoding" OFF)

# 查找FFmpeg库
if(USE_FFMPEG)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG_AVCODEC REQUIRED libavcodec)
    pkg_check_modules(FFMPEG_AVUTIL REQUIRED libavutil)
    add_definitions(-DUSE_FFMPEG)
    message(STATUS "Using FFmpeg for H264 decoding")
else()
    message(STATUS "Using OpenH264 for H264 decoding")
endif()

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

# 添加FFmpeg包含目录
if(USE_FFMPEG)
    include_directories(${FFMPEG_AVCODEC_INCLUDE_DIRS})
    include_directories(${FFMPEG_AVUTIL_INCLUDE_DIRS})
endif()

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

# 查找OpenAL框架（macOS系统框架）
if(APPLE)
    find_library(OPENAL_LIBRARY OpenAL)
    if(NOT OPENAL_LIBRARY)
        message(FATAL_ERROR "OpenAL framework not found")
    endif()
else()
    find_package(OpenAL REQUIRED)
    set(OPENAL_LIBRARY ${OPENAL_LIBRARY})
endif()

# 源文件
if(USE_FFMPEG)
    set(PLUGIN_SOURCES
        src/shared/src/decoders/FFmpegH264Decoder.cpp
        src/shared/src/decoders/AACDecoder.cpp
        src/shared/src/decoders/MP4Demuxer.cpp
        src/shared/src/managers/DecoderManager.cpp
        src/shared/src/managers/H264Movie.cpp
        src/shared/src/utils/ErrorHandler.cpp
        src/shared/src/utils/MiniMP4Implementation.cpp
        src/lua/H264TextureBinding.cpp
    )
else()
    set(PLUGIN_SOURCES
        src/shared/src/decoders/H264Decoder.cpp
        src/shared/src/decoders/AACDecoder.cpp
        src/shared/src/decoders/MP4Demuxer.cpp
        src/shared/src/managers/DecoderManager.cpp
        src/shared/src/managers/H264Movie.cpp
        src/shared/src/utils/ErrorHandler.cpp
        src/shared/src/utils/MiniMP4Implementation.cpp
        src/lua/H264TextureBinding.cpp
    )
endif()

# 添加Corona兼容层（仅用于独立测试）
if(STANDALONE_BUILD)
    list(APPEND PLUGIN_SOURCES src/lua/CoronaMinimal.cpp)
    message(STATUS "Building standalone version with Corona compatibility layer")
else()
    message(STATUS "Building Solar2D deployment version (requires Solar2D runtime)")
endif()

# 头文件
if(USE_FFMPEG)
    set(PLUGIN_HEADERS
        src/shared/include/decoders/FFmpegH264Decoder.h
        src/shared/include/decoders/AACDecoder.h
        src/shared/include/decoders/MP4Demuxer.h
        src/shared/include/managers/DecoderManager.h
        src/shared/include/managers/H264Movie.h
        src/shared/include/utils/ErrorHandler.h
        src/shared/include/utils/Common.h
        include/lua/H264TextureBinding.h
    )
else()
    set(PLUGIN_HEADERS
        src/shared/include/decoders/H264Decoder.h
        src/shared/include/decoders/AACDecoder.h
        src/shared/include/decoders/MP4Demuxer.h
        src/shared/include/managers/DecoderManager.h
        src/shared/include/managers/H264Movie.h
        src/shared/include/utils/ErrorHandler.h
        src/shared/include/utils/Common.h
        include/lua/H264TextureBinding.h
    )
endif()

# 创建静态库
add_library(plugin_h264_static STATIC ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

# 链接库
if(USE_FFMPEG)
    target_link_libraries(plugin_h264_static 
        ${FFMPEG_AVCODEC_LIBRARIES} 
        ${FFMPEG_AVUTIL_LIBRARIES}
        ${FDKAAC_LIBRARY} 
        ${LUA_LIBRARIES} 
        ${OPENAL_LIBRARY}
    )
else()
    target_link_libraries(plugin_h264_static 
        ${OPENH264_LIBRARY} 
        ${FDKAAC_LIBRARY} 
        ${LUA_LIBRARIES} 
        ${OPENAL_LIBRARY}
    )
endif()

# 创建动态库 (用于Solar2D插件)
add_library(plugin_h264 SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

if(USE_FFMPEG)
    target_link_libraries(plugin_h264 
        ${FFMPEG_AVCODEC_LIBRARIES} 
        ${FFMPEG_AVUTIL_LIBRARIES}
        ${FDKAAC_LIBRARY} 
        ${LUA_LIBRARIES} 
        ${OPENAL_LIBRARY}
    )
else()
    target_link_libraries(plugin_h264 
        ${OPENH264_LIBRARY} 
        ${FDKAAC_LIBRARY} 
        ${LUA_LIBRARIES} 
        ${OPENAL_LIBRARY}
    )
endif()

# 测试选项
option(ENABLE_TESTING "Enable unit tests" ON)
option(ENABLE_COVERAGE "Enable code coverage" OFF)

# Skip tests to avoid GTest dependency for now
# if(ENABLE_TESTING)
#     enable_testing()
#     add_subdirectory(tests)
# endif()

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