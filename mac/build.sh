#!/bin/bash

path=`dirname $0`

OUTPUT_DIR=$1
TARGET_NAME=h264
OUTPUT_SUFFIX=dylib
CONFIG=Release

#
# Checks exit value for error
# 
checkError() {
    if [ $? -ne 0 ]
    then
        echo "Exiting due to errors (above)"
        exit -1
    fi
}

# 
# Canonicalize relative paths to absolute paths
# 
pushd $path > /dev/null
dir=`pwd`
path=$dir
popd > /dev/null

if [ -z "$OUTPUT_DIR" ]
then
    OUTPUT_DIR=.
fi

pushd $OUTPUT_DIR > /dev/null
dir=`pwd`
OUTPUT_DIR=$dir
popd > /dev/null

echo "OUTPUT_DIR: $OUTPUT_DIR"

# Build with CMake instead of Xcode to maintain consistency
cd "$path/.."
mkdir -p build
cd build

echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=$CONFIG
checkError

echo "Building plugin..."
make -j4
checkError

PLUGINS_DIR=~/Solar2DPlugins/com.plugin/plugin.$TARGET_NAME/mac-sim

mkdir -p "$PLUGINS_DIR"

# Copy the built dylib
cp "libplugin_h264.dylib" "$OUTPUT_DIR/plugin_$TARGET_NAME.$OUTPUT_SUFFIX"

# Create data.tgz for Solar2D plugin deployment
tar -czf "data.tgz" "./plugin_$TARGET_NAME.$OUTPUT_SUFFIX"

cp "data.tgz" "$PLUGINS_DIR/data.tgz"

echo "Build completed successfully!"
echo "Plugin deployed to: $PLUGINS_DIR"