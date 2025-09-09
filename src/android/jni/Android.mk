LOCAL_PATH := $(call my-dir)

CORONA_NATIVE := /Applications/CoronaEnterprise
CORONA_ROOT := $(CORONA_NATIVE)/Corona
LUA_API_DIR := $(CORONA_ROOT)/shared/include/lua
LUA_API_CORONA := $(CORONA_ROOT)/shared/include/Corona

SRC_DIR := ../../shared

HEADER_DIR := $(SRC_DIR)/include
HEADER_DIR += $(SRC_DIR)/../../third_party/minimp4
OPENH264_API_DIR := $(SRC_DIR)/../../third_party/openh264/codec/api/wels
FDKAAC_API_DIR := $(SRC_DIR)/../../third_party/fdk-aac/libAACdec/include
FDKAAC_API_DIR += $(SRC_DIR)/../../third_party/fdk-aac/libSYS/include

######################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := liblua
LOCAL_SRC_FILES := ../corona-libs/jni/$(TARGET_ARCH_ABI)/liblua.so
LOCAL_EXPORT_C_INCLUDES := $(LUA_API_DIR)
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libcorona
LOCAL_SRC_FILES := ../corona-libs/jni/$(TARGET_ARCH_ABI)/libcorona.so
LOCAL_EXPORT_C_INCLUDES := $(LUA_API_CORONA)
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libopenal
LOCAL_SRC_FILES := ../corona-libs/jni/$(TARGET_ARCH_ABI)/libopenal.so
include $(PREBUILT_SHARED_LIBRARY)

######################################################################

include $(CLEAR_VARS)
LOCAL_MODULE := libopenh264
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libopenh264.a
LOCAL_EXPORT_C_INCLUDES := $(OPENH264_API_DIR)
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libfdk-aac
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libfdk-aac.a
LOCAL_EXPORT_C_INCLUDES := $(FDKAAC_API_DIR)
include $(PREBUILT_STATIC_LIBRARY)

######################################################################

include $(CLEAR_VARS)

LOCAL_MODULE := libplugin.h264

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(HEADER_DIR)

LOCAL_SRC_FILES := $(SRC_DIR)/src/lua/H264TextureBinding.cpp \
    $(SRC_DIR)/src/decoders/H264Decoder.cpp \
    $(SRC_DIR)/src/decoders/AACDecoder.cpp \
    $(SRC_DIR)/src/decoders/MP4Demuxer.cpp \
    $(SRC_DIR)/src/managers/DecoderManager.cpp \
    $(SRC_DIR)/src/managers/H264Movie.cpp \
    $(SRC_DIR)/src/utils/ErrorHandler.cpp \
    $(SRC_DIR)/src/utils/MiniMP4Implementation.cpp \
    $(SRC_DIR)/generated/plugin_h264.c


LOCAL_CFLAGS := \
    -DANDROID_NDK \
    -DNDEBUG \
    -D_REENTRANT \
    -DRtt_ANDROID_ENV \
    -ffast-math \
    -Ofast \
    -fPIC \
    -DPIC

LOCAL_CPPFLAGS := -fexceptions -fPIC

LOCAL_LDLIBS := -s

LOCAL_LDFLAGS += "-Wl,-z,max-page-size=16384"
LOCAL_LDFLAGS += "-Wl,-z,common-page-size=16384"

LOCAL_SHARED_LIBRARIES := liblua libcorona libopenal

LOCAL_STATIC_LIBRARIES := libopenh264 libfdk-aac

ifeq ($(TARGET_ARCH), arm)
    LOCAL_CFLAGS+= -D_ARM_ASSEM_
endif

ifeq ($(TARGET_ARCH), x86)
    LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true
endif


LOCAL_ARM_MODE := arm
include $(BUILD_SHARED_LIBRARY)
