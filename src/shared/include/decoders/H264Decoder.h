#ifndef PLUGIN_H264_H264_DECODER_H
#define PLUGIN_H264_H264_DECODER_H

#include "../utils/Common.h"
#include "../utils/ErrorHandler.h"
#include "codec_def.h"  // OpenH264头文件
#include "codec_api.h"
#include "codec_app_def.h"  // For DECODER_OPTION_NUM_OF_THREADS
#include <vector>
#include <queue>
#include <mutex>

namespace plugin_h264 {

// 帧缓冲池，用于重用内存
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

class H264Decoder : public ErrorHandler {
public:
    H264Decoder();
    ~H264Decoder();
    
    // 禁用拷贝构造和赋值
    H264Decoder(const H264Decoder&) = delete;
    H264Decoder& operator=(const H264Decoder&) = delete;
    
    // 初始化解码器
    bool initialize();
    
    // 解码H264数据到YUV420 - 支持零拷贝模式
    bool decode(const uint8_t* nal_data, size_t nal_size, VideoFrame& frame);
    
    // 获取解码器信息
    bool getDecoderInfo(int& width, int& height) const;
    
    // 设置是否需要紧凑格式（控制零拷贝模式）
    void setCompactFormatRequired(bool required) { require_compact_format_ = required; }
    
    // 重置解码器
    void reset();
    
    // 释放资源
    void destroy();
    
    // 获取内存使用量（估计值）
    size_t getMemoryUsage() const;
    
private:
    ISVCDecoder* decoder_;
    bool initialized_;
    int frame_width_;
    int frame_height_;
    
    // 零拷贝模式控制
    bool require_compact_format_;  // 是否需要紧凑格式（禁用零拷贝）
    
    // 内部辅助方法
    bool setupDecoderOptions();
    bool tryLenientConfiguration();
    bool tryCompatibilityConfiguration();
    bool tryBaselineConfiguration();
    bool applyAggressiveCompatibilityOptions();
    bool applyStandardCompatibilityOptions();
    bool applyMinimalOptions();
    bool allocateFrameBuffer(int width, int height);
    void freeFrameBuffer();
    
    // 帧缓冲区（重用以减少内存分配）
    std::unique_ptr<uint8_t[]> frame_buffer_;
    size_t frame_buffer_size_;
    
    // 帧缓冲池
    FrameBufferPool buffer_pool_;
};

} // namespace plugin_h264

#endif // PLUGIN_H264_H264_DECODER_H
