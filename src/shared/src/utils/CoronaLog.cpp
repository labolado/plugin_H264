#include "CoronaLog.h"
#include <cstdio>
#include <cstdarg>

namespace Corona {
    void Log::Printf(LogLevel level, const char* format, ...) {
        const char* level_str;
        switch(level) {
            case kLogLevelVerbose: level_str = "[VERBOSE]"; break;
            case kLogLevelInfo: level_str = "[INFO]"; break;
            case kLogLevelWarning: level_str = "[WARNING]"; break;
            case kLogLevelError: level_str = "[ERROR]"; break;
            default: level_str = "[UNKNOWN]"; break;
        }
        
        printf("%s ", level_str);
        
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
    
    void Log::Print(LogLevel level, const std::string& message) {
        Printf(level, "%s\n", message.c_str());
    }
}