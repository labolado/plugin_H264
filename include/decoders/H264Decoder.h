#ifndef PLUGIN_H264_H264_DECODER_H
#define PLUGIN_H264_H264_DECODER_H

#include "../utils/Common.h"
#include "../utils/ErrorHandler.h"
#include <codec_def.h>  // OpenH264头文件
#include <codec_api.h>

namespace plugin_h264 {

class H264Decoder : public ErrorHandler {
public:
    H264Decoder();
    ~H264Decoder();
    
    // 禁用拷贝构造和赋值
    H264Decoder(const H264Decoder&) = delete;
    H264Decoder& operator=(const H264Decoder&) = delete;
    
    // 初始化解码器
    bool initialize();
    
    // 解码H264数据到YUV420
    bool decode(const uint8_t* nal_data, size_t nal_size, VideoFrame& frame);
    
    // 获取解码器信息
    bool getDecoderInfo(int& width, int& height) const;
    
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
    
    // 内部辅助方法
    bool setupDecoderOptions();
    bool allocateFrameBuffer(int width, int height);
    void freeFrameBuffer();
    
    // 帧缓冲区（重用以减少内存分配）
    std::unique_ptr<uint8_t[]> frame_buffer_;
    size_t frame_buffer_size_;
};

} // namespace plugin_h264

#endif // PLUGIN_H264_H264_DECODER_H