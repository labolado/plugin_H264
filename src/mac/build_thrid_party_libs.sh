#!/bin/bash

cd $(dirname $0)

current_dir=$(pwd)
openh264_dir=${current_dir}/../../third_party/openh264
fdk_aac_dir=${current_dir}/../../third_party/fdk-aac

CPU_CORES=$(sysctl -n hw.ncpu 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)
echo "CPU_CORES: ${CPU_CORES}"

clean_prev_build_results() {
    cd ${openh264_dir}
    make clean
    if [ -f codec/common/src/cpu-features.o ]
    then
        rm codec/common/src/cpu-features.o
    fi
}

remove_if_exists() {
    if [ -d ${1} ]
    then
        rm -rf ${1}
    fi
}

create_if_not_exists() {
    if [ ! -d ${1} ]
    then
        mkdir -p ${1}
    fi
}

remove_and_create() {
    remove_if_exists ${1}
    mkdir -p ${1}
}

COMMON_BUILD_ARGS="OS=darwin -j${CPU_CORES}"
target_lib_name=libopenh264.a
build_dir_prefix=build_mac_
build_openh264() {
    cd ${openh264_dir}
    local_build_dir=${openh264_dir}/${build_dir_prefix}${1}
    remove_and_create ${local_build_dir}

    clean_prev_build_results
    make ${COMMON_BUILD_ARGS} ARCH=${1}

    cp ${target_lib_name} ${local_build_dir}
}

build_openh264 arm64
build_openh264 x86_64

cd ${openh264_dir}
libs_dir=${current_dir}/third_party_libs
create_if_not_exists ${libs_dir}
lipo -create ${build_dir_prefix}arm64/${target_lib_name} ${build_dir_prefix}x86_64/${target_lib_name} -output ${libs_dir}/${target_lib_name}

build_fdk_aac() {
    cd ${fdk_aac_dir}
    create_if_not_exists build/mac
    cd build/mac

    cmake ${fdk_aac_dir} \
        -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
        -DCMAKE_BUILD_TYPE=Release
    make -j${CPU_CORES}

    cp libfdk-aac.a ${libs_dir}/libfdk-aac.a
}

build_fdk_aac

ls -la ${libs_dir}

echo "Build thrid party libs done."
