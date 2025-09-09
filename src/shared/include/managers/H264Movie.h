#ifndef PLUGIN_H264_H264_MOVIE_H
#define PLUGIN_H264_H264_MOVIE_H

#include "../utils/Common.h"
#include "../utils/ErrorHandler.h"
#include "../managers/DecoderManager.h"
#include <memory>
#include <string>

namespace plugin_h264 {

class H264Movie : public ErrorHandler {
public:
    H264Movie();
    ~H264Movie();

    // 禁用拷贝构造和赋值
    H264Movie(const H264Movie&) = delete;
    H264Movie& operator=(const H264Movie&) = delete;

    // 加载电影文件
    bool loadFromFile(const std::string& file_path);

    // 播放控制
    bool play();
    bool pause();
    bool stop();
    bool replay();
    bool isPlaying() const;

    // 获取电影信息
    double getDuration() const;
    double getCurrentTime() const;
    bool hasNewVideoFrame() const;
    bool hasNewAudioFrame() const;
    bool hasAudioTrack() const;
    bool isVideoTrackFinished() const;
    bool isAudioTrackFinished() const;
    bool isPlaybackFinished() const;

    // 获取当前帧
    VideoFrame getCurrentVideoFrame();
    AudioFrame getCurrentAudioFrame();

    // 解码下一帧
    bool decodeNextFrame();

    // 分离的解码函数
    bool decodeNextVideoFrame();
    bool decodeNextAudioFrame();

    // 定位
    bool seekTo(double timestamp);

private:
    std::unique_ptr<DecoderManager> decoder_manager_;
    bool is_loaded_;
    bool is_playing_;
    double duration_;
    std::vector<TrackInfo> tracks_;

    // 内部状态
    VideoFrame current_video_frame_;
    AudioFrame current_audio_frame_;
    bool has_new_video_frame_;
    bool has_new_audio_frame_;

    // 轨道完成状态
    bool video_track_finished_;
    bool audio_track_finished_;

    // 解码器配置状态
    bool sps_pps_sent_;
    bool aac_configured_;
};

} // namespace plugin_h264

#endif // PLUGIN_H264_H264_MOVIE_H
