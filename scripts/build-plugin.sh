#!/bin/bash
# Solar2D Plugin Build Script for H.264 Plugin
# Based on official Solar2D plugin structure

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
OUTPUT_DIR="$PROJECT_DIR/plugins/2020.3620"

echo "Building H.264 Plugin for Solar2D"
echo "=================================="

# Build the main plugin first
echo "Step 1: Building core plugin..."
cd "$PROJECT_DIR"
mkdir -p build
cd build

# Configure and build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DLUA_LIBRARIES=$(which lua >/dev/null 2>&1 && brew --prefix lua 2>/dev/null || echo "/opt/homebrew")/lib/liblua.dylib \
         -DLUA_INCLUDE_DIRS=$(which lua >/dev/null 2>&1 && brew --prefix lua 2>/dev/null || echo "/opt/homebrew")/include/lua5.4

make -j$(nproc 2>/dev/null || echo 4)

echo "Step 2: Packaging for different platforms..."

# macOS Simulator
echo "Packaging macOS Simulator..."
mkdir -p "$OUTPUT_DIR/mac-sim"
cp libplugin_h264_static.a "$OUTPUT_DIR/mac-sim/libplugin_h264.a"
echo "✅ macOS Simulator package created"

# For iOS/Android, we would need specific build environments
echo "Step 3: Platform-specific builds..."
echo "⚠️  iOS and Android builds require specific toolchains"
echo "   - iOS: Requires Xcode and iOS SDK"
echo "   - Android: Requires Android NDK"

# Create a simple build summary
echo ""
echo "Build Summary:"
echo "=============="
echo "✅ Core plugin built successfully"
echo "✅ macOS Simulator package ready"
echo "📁 Plugin packages available in: $OUTPUT_DIR"

# List what we have
echo ""
echo "Available packages:"
find "$OUTPUT_DIR" -name "*.a" -o -name "*.dylib" -o -name "*.so" -o -name "*.dll" 2>/dev/null | while read file; do
    echo "  📦 $(basename "$(dirname "$file")")/$(basename "$file")"
done

echo ""
echo "🎉 Plugin build completed!"
echo "To use in Solar2D project, add to build.settings:"
echo 'plugins = {'
echo '  ["plugin.h264"] = {'
echo '    publisherId = "com.yourcompany",'
echo '    supportedPlatforms = {'
echo '      macos = true,'
echo '      -- Add other platforms as available'
echo '    }'
echo '  }'
echo '}'