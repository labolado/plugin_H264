#!/bin/bash

# Test runner script for H.264 Plugin
# This script runs all available tests

set -e

echo "H.264 Plugin Test Suite Runner"
echo "=============================="
echo

# Check if build exists
if [ ! -f "build/libplugin_h264.dylib" ]; then
    echo "❌ Plugin not built. Building now..."
    mkdir -p build
    cd build
    cmake ..
    make
    cd ..
    echo "✅ Plugin built successfully"
    echo
fi

# Set library path for dependencies
export DYLD_LIBRARY_PATH="./third_party/openh264:./third_party/fdk-aac/build:$DYLD_LIBRARY_PATH"

echo "Running tests..."
echo "=================="

# Run basic Lua binding test
echo "1. Running basic Lua binding test..."
lua test_lua_binding.lua
echo

# Run integration tests
echo "2. Running integration test suite..."
lua tests/integration/test_plugin_integration.lua
echo

# Run any unit tests if they exist
if [ -d "tests/unit" ]; then
    echo "3. Running unit tests..."
    for test_file in tests/unit/*.lua; do
        if [ -f "$test_file" ]; then
            echo "Running: $test_file"
            lua "$test_file"
        fi
    done
    echo
fi

echo "🎉 All tests completed successfully!"
echo
echo "Test Summary:"
echo "- Basic Lua binding: ✅ PASSED"
echo "- Integration tests: ✅ PASSED"
echo "- Plugin functionality verified"
echo
echo "The H.264 plugin is ready for use in Solar2D projects!"
