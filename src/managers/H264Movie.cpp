#include "../include/managers/H264Movie.h"

namespace plugin_h264 {

H264Movie::H264Movie()
    : is_loaded_(false)
    , is_playing_(false)
    , duration_(0.0)
    , has_new_video_frame_(false)
    , has_new_audio_frame_(false) {
    
    decoder_manager_ = std::make_unique<DecoderManager>();
}

H264Movie::~H264Movie() {
    stop();
}

bool H264Movie::loadFromFile(const std::string& file_path) {
    if (is_loaded_) {
        stop();
    }
    
    // 初始化并打开文件
    if (!decoder_manager_->openFile(file_path)) {
        setError(H264Error::FILE_OPEN_FAILED, "Failed to load movie: " + decoder_manager_->getLastMessage());
        return false;
    }
    
    // 获取文件信息
    if (!decoder_manager_->getFileInfo(tracks_, duration_)) {
        setError(H264Error::UNSUPPORTED_FORMAT, "Failed to get movie info");
        return false;
    }
    
    is_loaded_ = true;
    clearError();
    return true;
}

bool H264Movie::play() {
    if (!is_loaded_) {
        setError(H264Error::DECODER_INIT_FAILED, "Movie not loaded");
        return false;
    }
    
    is_playing_ = true;
    clearError();
    return true;
}

bool H264Movie::pause() {
    if (!is_loaded_) {
        setError(H264Error::DECODER_INIT_FAILED, "Movie not loaded");
        return false;
    }
    
    is_playing_ = false;
    clearError();
    return true;
}

bool H264Movie::stop() {
    is_playing_ = false;
    
    if (is_loaded_ && decoder_manager_) {
        decoder_manager_->closeFile();
    }
    
    is_loaded_ = false;
    duration_ = 0.0;
    tracks_.clear();
    
    has_new_video_frame_ = false;
    has_new_audio_frame_ = false;
    
    clearError();
    return true;
}

bool H264Movie::isPlaying() const {
    return is_playing_;
}

double H264Movie::getDuration() const {
    return duration_;
}

double H264Movie::getCurrentTime() const {
    if (!is_loaded_ || !decoder_manager_) {
        return 0.0;
    }
    
    auto demuxer = decoder_manager_->getMP4Demuxer();
    return demuxer ? demuxer->getCurrentTime() : 0.0;
}

bool H264Movie::hasNewVideoFrame() const {
    return has_new_video_frame_;
}

bool H264Movie::hasNewAudioFrame() const {
    return has_new_audio_frame_;
}

VideoFrame H264Movie::getCurrentVideoFrame() {
    has_new_video_frame_ = false;
    return current_video_frame_;
}

AudioFrame H264Movie::getCurrentAudioFrame() {
    has_new_audio_frame_ = false;
    return current_audio_frame_;
}

bool H264Movie::seekTo(double timestamp) {
    if (!is_loaded_ || !decoder_manager_) {
        setError(H264Error::DECODER_INIT_FAILED, "Movie not loaded");
        return false;
    }
    
    auto demuxer = decoder_manager_->getMP4Demuxer();
    if (!demuxer) {
        setError(H264Error::DECODER_INIT_FAILED, "Demuxer not available");
        return false;
    }
    
    if (!demuxer->seekToTime(timestamp)) {
        setError(H264Error::DECODE_FAILED, "Seek failed: " + demuxer->getLastMessage());
        return false;
    }
    
    clearError();
    return true;
}

} // namespace plugin_h264