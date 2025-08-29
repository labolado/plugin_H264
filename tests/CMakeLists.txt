cmake_minimum_required(VERSION 3.10)

# 查找GoogleTest
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/openh264/codec/api/wels)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/fdk-aac/libAACdec/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/fdk-aac/libSYS/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/minimp4)

# 设置库路径
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/openh264)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/fdk-aac/build)

# 单元测试源文件
set(TEST_SOURCES
    unit/test_h264_decoder.cpp
    unit/test_error_handler.cpp
)

# 创建测试可执行文件
add_executable(plugin_h264_tests ${TEST_SOURCES})

# 链接库
target_link_libraries(plugin_h264_tests
    plugin_h264_static
    ${GTEST_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    openh264
    fdk-aac
)

# 添加测试
add_test(NAME plugin_h264_unit_tests COMMAND plugin_h264_tests)