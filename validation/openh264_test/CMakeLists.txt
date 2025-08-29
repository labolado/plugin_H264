cmake_minimum_required(VERSION 3.10)
project(OpenH264Validation)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OpenH264 library path
set(OPENH264_ROOT ${CMAKE_SOURCE_DIR}/../third_party/openh264)

# Include directories
include_directories(
    ${OPENH264_ROOT}/codec/api/wels
    ${OPENH264_ROOT}/codec/common/inc
)

# Find OpenH264 library
find_library(OPENH264_LIB
    NAMES openh264 libopenh264
    PATHS ${OPENH264_ROOT}
    NO_DEFAULT_PATH
)

if(NOT OPENH264_LIB)
    message(STATUS "OpenH264 library not found, will compile from source")
    
    # Build OpenH264 first
    add_custom_target(build_openh264
        COMMAND make -j4
        WORKING_DIRECTORY ${OPENH264_ROOT}
        COMMENT "Building OpenH264 library"
    )
    
    # Set the expected library path
    if(APPLE)
        set(OPENH264_LIB ${OPENH264_ROOT}/libopenh264.dylib)
    elseif(WIN32)
        set(OPENH264_LIB ${OPENH264_ROOT}/openh264.dll)
    else()
        set(OPENH264_LIB ${OPENH264_ROOT}/libopenh264.so)
    endif()
endif()

# Create test executable
add_executable(test_openh264_api test_openh264_api.cpp)

# Link libraries
target_link_libraries(test_openh264_api ${OPENH264_LIB})

# Add dependency if we're building from source
if(TARGET build_openh264)
    add_dependencies(test_openh264_api build_openh264)
endif()

# Copy library to output directory for runtime
if(APPLE)
    add_custom_command(TARGET test_openh264_api POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${OPENH264_LIB}
        $<TARGET_FILE_DIR:test_openh264_api>
    )
endif()