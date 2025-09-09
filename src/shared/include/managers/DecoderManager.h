#ifndef PLUGIN_H264_DECODER_MANAGER_H
#define PLUGIN_H264_DECODER_MANAGER_H

#include "../utils/Common.h"
#include "../utils/ErrorHandler.h"
#include "../decoders/H264Decoder.h"
#include "../decoders/AACDecoder.h"
#include "../decoders/MP4Demuxer.h"
#include <memory>

namespace plugin_h264 {

class DecoderManager : public ErrorHandler {
public:
    DecoderManager();
    ~DecoderManager();
    
    // 禁用拷贝构造和赋值
    DecoderManager(const DecoderManager&) = delete;
    DecoderManager& operator=(const DecoderManager&) = delete;
    
    // 初始化管理器
    bool initialize();
    
    // 打开媒体文件
    bool openFile(const std::string& file_path);
    
    // 关闭文件
    void closeFile();
    
    // 获取文件信息
    bool getFileInfo(std::vector<TrackInfo>& tracks, double& duration) const;
    
    // 获取解码器
    H264Decoder* getH264Decoder() { return h264_decoder_.get(); }
    AACDecoder* getAACDecoder() { return aac_decoder_.get(); }
    MP4Demuxer* getMP4Demuxer() { return mp4_demuxer_.get(); }
    
    // 释放资源
    void destroy();
    
private:
    std::unique_ptr<H264Decoder> h264_decoder_;
    std::unique_ptr<AACDecoder> aac_decoder_;
    std::unique_ptr<MP4Demuxer> mp4_demuxer_;
    bool initialized_;
    bool file_open_;
};

} // namespace plugin_h264

#endif // PLUGIN_H264_DECODER_MANAGER_H