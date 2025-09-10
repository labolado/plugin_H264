#!/bin/bash

path=`dirname $0`

# ./build_thrid_party_libs.sh
# cd $path

OUTPUT_DIR=$1
TARGET_NAME=h264
OUTPUT_SUFFIX=a
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

# Clean
xcodebuild -project "$path/Plugin.xcodeproj" -configuration $CONFIG clean
checkError

# iOS
xcodebuild -project "$path/Plugin.xcodeproj" -configuration $CONFIG -sdk iphoneos
checkError

# iOS-sim
xcodebuild -project "$path/Plugin.xcodeproj" -configuration $CONFIG -sdk iphonesimulator
checkError

# create universal binary
# lipo -create "$path"/build/$CONFIG-iphoneos/lib$TARGET_NAME.$OUTPUT_SUFFIX "$path"/build/$CONFIG-iphonesimulator/lib$TARGET_NAME.$OUTPUT_SUFFIX -output "$OUTPUT_DIR"/libplugin_$TARGET_NAME.$OUTPUT_SUFFIX
# checkError

# echo "$OUTPUT_DIR"/libplugin_$TARGET_NAME.$OUTPUT_SUFFIX

lib_version=2020.3627
lib_name=libplugin_$TARGET_NAME.$OUTPUT_SUFFIX``

copy_lib() {
    dst_dir=${path}/../../plugins/${1}/${lib_version}
    if [ ! -d ${dst_dir} ]
    then
        mkdir -p ${dst_dir}
    fi
    cp "${path}"/metadata.lua "${dst_dir}/metadata.lua"
    cp "${path}"/build/$CONFIG-${2}/lib$TARGET_NAME.$OUTPUT_SUFFIX "${dst_dir}/${lib_name}"

    echo "Packing binaries..."
    cd ${path}
    tar -czvf ${lib_version}-${1}.tgz -C ${dst_dir} ${lib_name} metadata.lua
    echo ${path}/${lib_version}-${1}.tgz.
}

copy_lib iphone iphoneos
copy_lib iphone-sim iphonesimulator

echo "Done."
