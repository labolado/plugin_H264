#include "../include/managers/DecoderManager.h"

namespace plugin_h264 {

DecoderManager::DecoderManager()
    : initialized_(false)
    , file_open_(false) {
}

DecoderManager::~DecoderManager() {
    destroy();
}

bool DecoderManager::initialize() {
    if (initialized_) {
        return true;
    }
    
    // 创建解码器实例
    h264_decoder_ = std::make_unique<FFmpegH264Decoder>();
    aac_decoder_ = std::make_unique<AACDecoder>();
    mp4_demuxer_ = std::make_unique<MP4Demuxer>();
    
    // 初始化解码器
    if (!h264_decoder_->initialize()) {
        setError(H264Error::DECODER_INIT_FAILED, "Failed to initialize H264 decoder: " + h264_decoder_->getLastMessage());
        return false;
    }
    
    if (!aac_decoder_->initialize()) {
        setError(H264Error::DECODER_INIT_FAILED, "Failed to initialize AAC decoder: " + aac_decoder_->getLastMessage());
        return false;
    }
    
    initialized_ = true;
    clearError();
    return true;
}

bool DecoderManager::openFile(const std::string& file_path) {
    if (!initialized_) {
        if (!initialize()) {
            return false;
        }
    }
    
    if (file_open_) {
        closeFile();
    }
    
    // 打开MP4文件
    if (!mp4_demuxer_->open(file_path)) {
        setError(H264Error::FILE_OPEN_FAILED, "Failed to open MP4 file: " + mp4_demuxer_->getLastMessage());
        return false;
    }
    
    file_open_ = true;
    clearError();
    return true;
}

void DecoderManager::closeFile() {
    if (file_open_ && mp4_demuxer_) {
        mp4_demuxer_->close();
        file_open_ = false;
    }
}

bool DecoderManager::getFileInfo(std::vector<TrackInfo>& tracks, double& duration) const {
    if (!file_open_ || !mp4_demuxer_) {
        return false;
    }
    
    tracks = mp4_demuxer_->getTrackInfo();
    duration = mp4_demuxer_->getDuration();
    return true;
}

void DecoderManager::destroy() {
    closeFile();
    
    h264_decoder_.reset();
    aac_decoder_.reset();
    mp4_demuxer_.reset();
    
    initialized_ = false;
    clearError();
}

} // namespace plugin_h264