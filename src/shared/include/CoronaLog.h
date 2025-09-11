#ifndef CORONA_LOG_H
#define CORONA_LOG_H

#include <iostream>
#include <string>

// Simple logging implementation for plugin_h264
namespace Corona {
    enum LogLevel {
        kLogLevelVerbose,
        kLogLevelInfo,
        kLogLevelWarning,
        kLogLevelError
    };
    
    class Log {
    public:
        static void Printf(LogLevel level, const char* format, ...);
        static void Print(LogLevel level, const std::string& message);
    };
}

// Convenience macros
#define CORONA_LOG_VERBOSE(format, ...) Corona::Log::Printf(Corona::kLogLevelVerbose, format, ##__VA_ARGS__)
#define CORONA_LOG_INFO(format, ...) Corona::Log::Printf(Corona::kLogLevelInfo, format, ##__VA_ARGS__)
#define CORONA_LOG_WARNING(format, ...) Corona::Log::Printf(Corona::kLogLevelWarning, format, ##__VA_ARGS__)
#define CORONA_LOG_ERROR(format, ...) Corona::Log::Printf(Corona::kLogLevelError, format, ##__VA_ARGS__)

#endif // CORONA_LOG_H