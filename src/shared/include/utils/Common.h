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

// 视频帧结构 - 优化版本支持零拷贝模式
struct VideoFrame {
    uint8_t* y_plane;    // Y平面数据指针（支持零拷贝）
    uint8_t* u_plane;    // U平面数据指针
    uint8_t* v_plane;    // V平面数据指针
    int width;           // 帧宽度
    int height;          // 帧高度
    int y_stride;        // Y平面stride（可能 != width）
    int uv_stride;       // UV平面stride（可能 != width/2）
    double timestamp;    // 时间戳
    bool zero_copy_mode; // 是否为零拷贝模式（数据来自解码器内部缓冲区）

    VideoFrame() : y_plane(nullptr), u_plane(nullptr), v_plane(nullptr),
                   width(0), height(0), y_stride(0), uv_stride(0), 
                   timestamp(0.0), zero_copy_mode(false) {}

    bool isValid() const {
        return y_plane != nullptr && u_plane != nullptr && v_plane != nullptr &&
               width > 0 && height > 0;
    }

    // 检查是否为紧凑格式（stride == width）
    bool isCompactFormat() const {
        return y_stride == width && uv_stride == (width / 2);
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
    int sample_rate;   // 音频采样率
    int channels;      // 仅音频
    uint32_t timescale; // 时间戳转换用的时间刻度

    TrackInfo() : type(MP4TrackType::UNKNOWN), codec(CodecType::UNKNOWN),
                  track_id(-1), duration(0.0), width(0), height(0),
                  sample_rate(0), channels(0), timescale(0) {}
};

} // namespace plugin_h264

#endif // PLUGIN_H264_COMMON_H
