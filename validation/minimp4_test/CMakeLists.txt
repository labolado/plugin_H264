cmake_minimum_required(VERSION 3.10)
project(MiniMP4Validation)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MiniMP4 is a single-header library
set(MINIMP4_ROOT ${CMAKE_SOURCE_DIR}/../third_party/minimp4)

# Include directories
include_directories(${MINIMP4_ROOT})

# Create test executable
add_executable(test_minimp4_api test_minimp4_api.cpp)

# MiniMP4 is header-only, so no linking required
# But we need to ensure it compiles as C++