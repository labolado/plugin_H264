#ifndef PLUGIN_H264_MP4_DEMUXER_H
#define PLUGIN_H264_MP4_DEMUXER_H

#include "../utils/Common.h"
#include "../utils/ErrorHandler.h"

// MiniMP4单头文件库（仅在一个源文件中定义实现）
#include "minimp4.h"

#include <string>
#include <vector>
#include <fstream>
#include <memory>

namespace plugin_h264 {

struct MP4Sample {
    std::vector<uint8_t> data;
    uint64_t timestamp;
    uint32_t duration;
    bool is_keyframe;

    MP4Sample() : timestamp(0), duration(0), is_keyframe(false) {}
};

class MP4Demuxer : public ErrorHandler {
public:
    MP4Demuxer();
    ~MP4Demuxer();

    // 禁用拷贝构造和赋值
    MP4Demuxer(const MP4Demuxer&) = delete;
    MP4Demuxer& operator=(const MP4Demuxer&) = delete;

    // 打开MP4文件
    bool open(const std::string& file_path);

    // 关闭文件
    void close();

    // 获取轨道信息
    std::vector<TrackInfo> getTrackInfo() const;

    // 提取SPS/PPS数据（H.264）
    bool extractSPS(int track_id, std::vector<uint8_t>& sps_data) const;
    bool extractPPS(int track_id, std::vector<uint8_t>& pps_data) const;

    // 读取下一个样本
    bool readNextSample(int track_id, MP4Sample& sample);

    // 跳转到指定时间
    bool seekToTime(double timestamp);

    // 获取文件持续时间
    double getDuration() const;

    // 获取当前播放时间
    double getCurrentTime() const;

private:
    MP4D_demux_t demuxer_;
    std::ifstream file_;
    std::vector<TrackInfo> tracks_;
    bool is_open_;
    double duration_;
    double current_time_;

    // 为每个轨道维护独立的sample索引
    std::vector<unsigned int> track_sample_indices_;

    // 内部辅助方法
    bool parseMP4Structure();
    bool extractTrackInfo();
    static int readCallback(int64_t offset, void* buffer, size_t size, void* token);
    TrackInfo createTrackInfo(const MP4D_track_t& track);
};

} // namespace plugin_h264

#endif // PLUGIN_H264_MP4_DEMUXER_H
