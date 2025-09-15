#ifndef PLUGIN_H264_FFMPEG_H264_DECODER_H
#define PLUGIN_H264_FFMPEG_H264_DECODER_H

#include "../utils/Common.h"
#include "../utils/ErrorHandler.h"
#include <vector>
#include <memory>
#include <queue>
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

namespace plugin_h264 {

// 简单的帧缓冲池，复用现有设计
class FrameBufferPool {
public:
    FrameBufferPool(size_t pool_size = 3) : pool_size_(pool_size) {}
    
    std::unique_ptr<uint8_t[]> acquire(size_t size) {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        if (!pool_.empty() && pool_.front().second >= size) {
            auto buffer = std::move(pool_.front().first);
            pool_.pop();
            return buffer;
        }
        return std::make_unique<uint8_t[]>(size);
    }
    
    void release(std::unique_ptr<uint8_t[]> buffer, size_t size) {
        if (!buffer) return;
        std::lock_guard<std::mutex> lock(pool_mutex_);
        if (pool_.size() < pool_size_) {
            pool_.push({std::move(buffer), size});
        }
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        while (!pool_.empty()) {
            pool_.pop();
        }
    }
    
private:
    size_t pool_size_;
    std::queue<std::pair<std::unique_ptr<uint8_t[]>, size_t>> pool_;
    mutable std::mutex pool_mutex_;
};

// FFmpeg H264 解码器类 - 先实现基础单线程版本
class FFmpegH264Decoder : public ErrorHandler {
public:
    FFmpegH264Decoder();
    ~FFmpegH264Decoder();
    
    // 禁用拷贝构造和赋值
    FFmpegH264Decoder(const FFmpegH264Decoder&) = delete;
    FFmpegH264Decoder& operator=(const FFmpegH264Decoder&) = delete;
    
    // 初始化解码器 - 保持与H264Decoder相同接口
    bool initialize();
    
    // 解码H264数据到YUV420 - 保持接口兼容性
    bool decode(const uint8_t* nal_data, size_t nal_size, VideoFrame& frame);
    
    // 获取解码器信息
    bool getDecoderInfo(int& width, int& height) const;
    
    // 重置解码器
    void reset();
    
    // 释放资源
    void destroy();
    
    // 获取内存使用量估计
    size_t getMemoryUsage() const;
    
    // 设置线程数（用于后续多线程测试）
    void setThreadCount(int thread_count);
    
private:
    // FFmpeg 相关成员
    const AVCodec* codec_;
    AVCodecContext* codec_context_;
    AVFrame* av_frame_;
    AVPacket* av_packet_;
    bool initialized_;
    
    // 解码器状态
    int frame_width_;
    int frame_height_;
    int thread_count_;
    
    // 帧缓冲区（重用以减少内存分配）
    std::unique_ptr<uint8_t[]> frame_buffer_;
    size_t frame_buffer_size_;
    
    // 帧缓冲池
    FrameBufferPool buffer_pool_;
    
    // 内部方法
    bool setupFFmpegDecoder();
    bool convertAVFrameToVideoFrame(const AVFrame* av_frame, VideoFrame& video_frame);
    bool allocateFrameBuffer(int width, int height);
    void freeFrameBuffer();
    size_t calculateFrameSize(int width, int height) const;
};

} // namespace plugin_h264

#endif // PLUGIN_H264_FFMPEG_H264_DECODER_H