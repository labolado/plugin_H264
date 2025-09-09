#!/bin/bash

cd $(dirname $0)

current_dir=$(pwd)
openh264_dir=${current_dir}/../../third_party/openh264
fdk_aac_dir=${current_dir}/../../third_party/fdk-aac

CPU_CORES=$(sysctl -n hw.ncpu 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)
echo "CPU_CORES: ${CPU_CORES}"

# 设置 NDK 路径
ANDROID_SDK_HOME=$HOME/Library/Android/sdk
ANDROID_NDK_HOME=$HOME/Library/Android/sdk/ndk
export PATH=$ANDROID_SDK_HOME/tools:$PATH
# export ANDROID_NDK=$ANDROID_NDK_HOME/28.2.13676358
export ANDROID_NDK=$ANDROID_NDK_HOME/21.0.6113669
# export ANDROID_NDK=$ANDROID_NDK_HOME/18.1.5063045
export JAVA_HOME=/Library/Java/JavaVirtualMachines/openjdk-11.jdk/Contents/Home
export ANDROID_SDK_ROOT=$ANDROID_SDK_HOME
export ANDROID_NDK_HOME=$ANDROID_NDK

clean_prev_build_results() {
    cd ${openh264_dir}
    make clean
    rm codec/common/src/cpu-features.o
    ./gradlew clean
}

remove_if_exists() {
    if [ -d ${1} ]
    then
        rm -rf ${1}
    fi
}

# COMMON_BUILD_ARGS="OS=android NDKROOT=${ANDROID_NDK} TARGET=12 NDKLEVEL=21 -j${CPU_CORES}"
# target_lib_name=${openh264_dir}/libopenh264.a

# clean_prev_build_results
# make ${COMMON_BUILD_ARGS} ARCH=arm64
# cp ${target_lib_name} ${current_dir}/jni/arm64-v8a/

# clean_prev_build_results
# make ${COMMON_BUILD_ARGS} ARCH=arm
# cp ${target_lib_name} ${current_dir}/jni/armeabi-v7a/

# clean_prev_build_results
# make ${COMMON_BUILD_ARGS} ARCH=x86
# cp ${target_lib_name} ${current_dir}/jni/x86/

# clean_prev_build_results
# make ${COMMON_BUILD_ARGS} ARCH=x86_64
# cp ${target_lib_name} ${current_dir}/jni/x86_64/

# ANDROID_NDK_DIR=${ANDROID_NDK_HOME}/20.1.5948944
ANDROID_NDK_DIR=${ANDROID_NDK}

build_fdk_aac() {
    cd ${fdk_aac_dir}
    if [ ! -d build/android/${1} ]
    then
        mkdir -p build/android/${1}
    fi
    cd build/android/${1}

    cmake ${fdk_aac_dir} -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_DIR/build/cmake/android.toolchain.cmake \
        -DBUILD_SHARED_LIBS=OFF \
        -DANDROID_ABI=${1} \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INCLUDE_DIRECTORIES_BEFORE=${fdk_aac_dir}/libSBRdec/include \
        -DCMAKE_CXX_FLAGS="-fPIC" \
        -DANDROID_NATIVE_API_LEVEL=16
    make -j${CPU_CORES}

    libs_dir=${current_dir}/jni
    if [ ! -d ${libs_dir}/${1} ]
    then
        mkdir -p ${libs_dir}/${1}
    fi
    cp libfdk-aac.a ${libs_dir}/${1}/libfdk-aac.a
}

cd ${fdk_aac_dir}
if [ ! -d libSBRdec/include/log/ ]
then
    mkdir -p libSBRdec/include/log/
fi
if [ ! -f libSBRdec/include/log/log.h ]
then
    # https://github.com/mstorsjo/fdk-aac/issues/124
    echo "void android_errorWriteLog(int i, const char *string){}" > libSBRdec/include/log/log.h
fi

# remove_if_exists ${fdk_aac_dir}/build/android/arm64-v8a
build_fdk_aac arm64-v8a
build_fdk_aac armeabi-v7a
build_fdk_aac x86_64
build_fdk_aac x86

# 编译所有架构
# for arch in "aarch64" "arm" "i686" "x86_64"; do
#   case $arch in
#     "aarch64")
#       host="aarch64-linux-android"
#       ;;
#     "arm")
#       host="arm-linux-androideabi"
#       ;;
#     "i686")
#       host="i686-linux-android"
#       ;;
#     "x86_64")
#       host="x86_64-linux-android"
#       ;;
#     *)
#       continue
#       ;;
#   esac

#   echo "Building for $arch..."
#   ./configure --host=$host --prefix=$(pwd)/android-build/$arch --enable-static --disable-shared
#   make clean
#   make -j${CPU_CORES}
# done

# echo "Done."
