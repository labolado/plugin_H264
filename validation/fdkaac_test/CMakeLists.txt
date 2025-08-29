cmake_minimum_required(VERSION 3.10)
project(FDKAACValidation)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FDK-AAC library path
set(FDKAAC_ROOT ${CMAKE_SOURCE_DIR}/../third_party/fdk-aac)

# Include directories
include_directories(${FDKAAC_ROOT}/libAACdec/include)
include_directories(${FDKAAC_ROOT}/libSYS/include)

# Library directories
link_directories(${FDKAAC_ROOT}/build)

# Create test executable
add_executable(test_fdkaac_api test_fdkaac_api.cpp)

# Link FDK-AAC library
target_link_libraries(test_fdkaac_api fdk-aac)