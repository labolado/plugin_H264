#ifndef PLUGIN_H264_ERROR_HANDLER_H
#define PLUGIN_H264_ERROR_HANDLER_H

#include "Common.h"
#include <string>
#include <sstream>

namespace plugin_h264 {

class ErrorHandler {
public:
    ErrorHandler() : last_error_(H264Error::NONE) {}
    
    void setError(H264Error error, const std::string& message) {
        last_error_ = error;
        last_message_ = message;
    }
    
    void clearError() {
        last_error_ = H264Error::NONE;
        last_message_.clear();
    }
    
    bool hasError() const {
        return last_error_ != H264Error::NONE;
    }
    
    H264Error getLastError() const {
        return last_error_;
    }
    
    const std::string& getLastMessage() const {
        return last_message_;
    }
    
    // 便捷的错误设置方法
    template<typename... Args>
    void setErrorF(H264Error error, const std::string& format, Args... args) {
        std::ostringstream oss;
        formatMessage(oss, format, args...);
        setError(error, oss.str());
    }
    
    static const char* errorToString(H264Error error) {
        switch (error) {
            case H264Error::NONE: return "No error";
            case H264Error::INVALID_PARAM: return "Invalid parameter";
            case H264Error::DECODER_INIT_FAILED: return "Decoder initialization failed";
            case H264Error::DECODE_FAILED: return "Decode operation failed";
            case H264Error::FILE_NOT_FOUND: return "File not found";
            case H264Error::FILE_OPEN_FAILED: return "File open failed";
            case H264Error::UNSUPPORTED_FORMAT: return "Unsupported format";
            case H264Error::OUT_OF_MEMORY: return "Out of memory";
            case H264Error::THREAD_ERROR: return "Thread error";
            default: return "Unknown error";
        }
    }

private:
    H264Error last_error_;
    std::string last_message_;
    
    void formatMessage(std::ostringstream& oss, const std::string& format) {
        oss << format;
    }
    
    template<typename T, typename... Args>
    void formatMessage(std::ostringstream& oss, const std::string& format, T&& t, Args... args) {
        size_t pos = format.find("{}");
        if (pos != std::string::npos) {
            oss << format.substr(0, pos) << t;
            formatMessage(oss, format.substr(pos + 2), args...);
        } else {
            oss << format;
        }
    }
};

} // namespace plugin_h264

#endif // PLUGIN_H264_ERROR_HANDLER_H