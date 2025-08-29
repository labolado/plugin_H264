#ifndef PLUGIN_H264_AAC_DECODER_H
#define PLUGIN_H264_AAC_DECODER_H

#include "../utils/Common.h"
#include "../utils/ErrorHandler.h"
#include <aacdecoder_lib.h>  // FDK-AAC头文件
#include <vector>

namespace plugin_h264 {

class AACDecoder : public ErrorHandler {
public:
    AACDecoder();
    ~AACDecoder();
    
    // 禁用拷贝构造和赋值
    AACDecoder(const AACDecoder&) = delete;
    AACDecoder& operator=(const AACDecoder&) = delete;
    
    // 初始化解码器
    bool initialize();
    
    // 解码AAC数据到PCM
    bool decode(const uint8_t* aac_data, size_t aac_size, AudioFrame& frame);
    
    // 获取解码器信息
    bool getDecoderInfo(int& sample_rate, int& channels, int& bits_per_sample) const;
    
    // 重置解码器
    void reset();
    
    // 释放资源
    void destroy();
    
    // 获取库信息
    std::vector<LIB_INFO> getLibraryInfo() const;
    
    // 获取内存使用量（估计值）
    size_t getMemoryUsage() const;
    
private:
    HANDLE_AACDECODER decoder_;
    bool initialized_;
    int sample_rate_;
    int channels_;
    int bits_per_sample_;
    
    // 输出缓冲区
    std::vector<INT_PCM> output_buffer_;
    
    // 内部辅助方法
    bool setupDecoderConfig();
    void updateDecoderInfo();
};

} // namespace plugin_h264

#endif // PLUGIN_H264_AAC_DECODER_H