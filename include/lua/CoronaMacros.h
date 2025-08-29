// Corona SDK macros and definitions
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// 导出宏
#ifndef CORONA_EXPORT
    #if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
        #define CORONA_EXPORT __declspec(dllexport)
    #else
        #define CORONA_EXPORT __attribute__((visibility("default")))
    #endif
#endif

// 平台检测
#ifdef __APPLE__
    #include "TargetConditionals.h"
    #if TARGET_OS_IPHONE
        #define CORONA_PLATFORM_IOS
    #else
        #define CORONA_PLATFORM_OSX  
    #endif
#elif defined(ANDROID) || defined(__ANDROID__)
    #define CORONA_PLATFORM_ANDROID
#elif defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
    #define CORONA_PLATFORM_WIN32
#endif

#ifdef __cplusplus
}
#endif