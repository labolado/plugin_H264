#ifndef PLUGIN_H264_COMMON_H
#define PLUGIN_H264_COMMON_H

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

namespace plugin_h264 {

// 版本信息
#define PLUGIN_H264_VERSION_MAJOR 1
#define PLUGIN_H264_VERSION_MINOR 0
#define PLUGIN_H264_VERSION_PATCH 0

// 错误码定义
enum class H264Error {
    NONE = 0,
    INVALID_PARAM,
    DECODER_INIT_FAILED,
    DECODE_FAILED,
    FILE_NOT_FOUND,
    FILE_OPEN_FAILED,
    UNSUPPORTED_FORMAT,
    OUT_OF_MEMORY,
    THREAD_ERROR
};

// 视频帧结构
struct VideoFrame {
    uint8_t* y_plane;
    uint8_t* u_plane;
    uint8_t* v_plane;
    int width;
    int height;
    int y_stride;
    int uv_stride;
    double timestamp;
    
    VideoFrame() : y_plane(nullptr), u_plane(nullptr), v_plane(nullptr),
                   width(0), height(0), y_stride(0), uv_stride(0), timestamp(0.0) {}
    
    bool isValid() const {
        return y_plane != nullptr && u_plane != nullptr && v_plane != nullptr &&
               width > 0 && height > 0;
    }
};

// 音频帧结构
struct AudioFrame {
    std::vector<int16_t> samples;
    int sample_rate;
    int channels;
    double timestamp;
    
    AudioFrame() : sample_rate(0), channels(0), timestamp(0.0) {}
    
    bool isValid() const {
        return !samples.empty() && sample_rate > 0 && channels > 0;
    }
};

// MP4轨道类型
enum class MP4TrackType {
    UNKNOWN = 0,
    VIDEO,
    AUDIO
};

// 编解码器类型
enum class CodecType {
    UNKNOWN = 0,
    H264,
    AAC
};

// 轨道信息
struct TrackInfo {
    MP4TrackType type;
    CodecType codec;
    int track_id;
    double duration;
    uint32_t width;    // 仅视频
    uint32_t height;   // 仅视频
    int sample_rate;   // 仅音频
    int channels;      // 仅音频
    
    TrackInfo() : type(MP4TrackType::UNKNOWN), codec(CodecType::UNKNOWN),
                  track_id(-1), duration(0.0), width(0), height(0),
                  sample_rate(0), channels(0) {}
};

} // namespace plugin_h264

#endif // PLUGIN_H264_COMMON_H