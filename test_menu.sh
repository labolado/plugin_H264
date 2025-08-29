#!/bin/bash

echo "H.264 Plugin Testing Guide"
echo "========================="
echo

echo "Available Test Options:"
echo "1. Basic Plugin Test (no video file needed)"
echo "2. Integration Test Suite (comprehensive)"
echo "3. Real Video File Test (requires MP4 file)"
echo "4. Solar2D Project Test"
echo

# Set library path
export DYLD_LIBRARY_PATH="./third_party/openh264:./third_party/fdk-aac/build:$DYLD_LIBRARY_PATH"

while true; do
    echo "Select test to run:"
    echo "1) Basic plugin functionality test"
    echo "2) Full integration test suite"
    echo "3) Real video file test"
    echo "4) Show Solar2D project setup"
    echo "5) All tests (except real video)"
    echo "0) Exit"
    echo
    read -p "Enter your choice [0-5]: " choice
    
    case $choice in
        1)
            echo "Running basic plugin test..."
            echo "============================="
            lua test_lua_binding.lua
            echo
            ;;
        2)
            echo "Running integration test suite..."
            echo "=================================="
            lua tests/integration/test_plugin_integration.lua
            echo
            ;;
        3)
            echo "Running real video test..."
            echo "=========================="
            echo "⚠️  This test requires a H.264/AAC MP4 video file"
            echo "Place your test video in the current directory and name it 'test_video.mp4'"
            echo
            read -p "Press Enter to continue (or Ctrl+C to cancel): "
            lua test_real_video.lua
            echo
            ;;
        4)
            echo "Solar2D Project Setup"
            echo "===================="
            echo "1. Copy the built plugin to your Solar2D plugins directory:"
            echo "   cp build/libplugin_h264.dylib ~/Solar2D/Plugins/"
            echo
            echo "2. Or use the example project:"
            echo "   cd examples/solar2d_project"
            echo "   # Open this directory in Solar2D Simulator"
            echo
            echo "3. The example project includes:"
            echo "   - main.lua (example app)"
            echo "   - build.settings (plugin configuration)"
            echo "   - config.lua (app settings)"
            echo
            echo "4. To test with real video in Solar2D:"
            echo "   - Add a 'sample_video.mp4' file to the project"
            echo "   - Run the project in Solar2D Simulator"
            echo "   - Click 'Load Video' button"
            echo
            ;;
        5)
            echo "Running all tests..."
            echo "==================="
            ./tests/run_tests.sh
            echo
            ;;
        0)
            echo "Exiting..."
            exit 0
            ;;
        *)
            echo "Invalid choice. Please try again."
            echo
            ;;
    esac
    
    read -p "Press Enter to continue..."
    echo
done