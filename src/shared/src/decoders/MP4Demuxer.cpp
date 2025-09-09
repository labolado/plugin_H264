#include "../include/decoders/MP4Demuxer.h"
#include <cstring>
#include <algorithm>

namespace plugin_h264 {

MP4Demuxer::MP4Demuxer()
    : is_open_(false)
    , duration_(0.0)
    , current_time_(0.0) {
    memset(&demuxer_, 0, sizeof(demuxer_));
}

MP4Demuxer::~MP4Demuxer() {
    close();
}

bool MP4Demuxer::open(const std::string& file_path) {
    if (is_open_) {
        close();
    }

    // 打开文件
    file_.open(file_path, std::ios::binary);
    if (!file_.is_open()) {
        setError(H264Error::FILE_OPEN_FAILED, "Failed to open file: " + file_path);
        return false;
    }

    // 获取文件大小 - 先定位到文件末尾
    file_.seekg(0, std::ios::end);
    int64_t file_size = file_.tellg();
    if (file_size <= 0) {
        setError(H264Error::UNSUPPORTED_FORMAT, "Invalid file size: " + std::to_string(file_size));
        file_.close();
        return false;
    }

    // 初始化MP4解复用器
    int result = MP4D_open(&demuxer_, readCallback, this, file_size);
    if (result != 1) {
        setError(H264Error::UNSUPPORTED_FORMAT,
                "Failed to open MP4 demuxer, error: " + std::to_string(result));
        file_.close();
        return false;
    }

    // 解析MP4结构
    if (!parseMP4Structure()) {
        close();
        return false;
    }

    // 提取轨道信息
    if (!extractTrackInfo()) {
        close();
        return false;
    }

    // 初始化每个轨道的sample索引
    track_sample_indices_.resize(demuxer_.track_count, 0);

    is_open_ = true;
    current_time_ = 0.0;
    clearError();
    return true;
}

void MP4Demuxer::close() {
    if (is_open_) {
        MP4D_close(&demuxer_);
        is_open_ = false;
    }

    if (file_.is_open()) {
        file_.close();
    }

    tracks_.clear();
    track_sample_indices_.clear();
    duration_ = 0.0;
    current_time_ = 0.0;
    clearError();
}

bool MP4Demuxer::parseMP4Structure() {
    // MP4D_open已经解析了基本结构
    // 这里主要验证文件格式正确性

    if (demuxer_.track_count == 0) {
        setError(H264Error::UNSUPPORTED_FORMAT, "No tracks found in MP4 file");
        return false;
    }

    // 添加调试信息
    PLUGIN_H264_LOG( ("MP4 Demuxer: Found %d tracks\n", demuxer_.track_count) );

    // 计算文件持续时间（基于实际API结构）
    duration_ = 0.0;
    for (unsigned int i = 0; i < demuxer_.track_count; ++i) {
        const MP4D_track_t* track = &demuxer_.track[i];

        // 打印轨道信息用于调试
        PLUGIN_H264_LOG( ("Track %d: handler_type=0x%08X, object_type=0x%02X, samples=%d\n",
               i, track->handler_type, track->object_type_indication, track->sample_count) );

        if (track->timescale > 0) {
            // 计算轨道持续时间（duration分为hi/lo两部分）
            uint64_t track_duration = ((uint64_t)track->duration_hi << 32) | track->duration_lo;
            double track_duration_sec = static_cast<double>(track_duration) / track->timescale;
            if (track_duration_sec > duration_) {
                duration_ = track_duration_sec;
            }

            PLUGIN_H264_LOG( ("  Duration: %llu ticks, %.2f seconds\n",
                   (unsigned long long)track_duration, track_duration_sec) );
        }

        // 检查轨道类型
        if (track->handler_type == MP4D_HANDLER_TYPE_VIDE) {
            PLUGIN_H264_LOG( ("  Type: Video, Size: %dx%d\n",
                   track->SampleDescription.video.width,
                   track->SampleDescription.video.height) );
        } else if (track->handler_type == MP4D_HANDLER_TYPE_SOUN) {
            PLUGIN_H264_LOG( ("  Type: Audio, Channels: %d, SampleRate: %d\n",
                   track->SampleDescription.audio.channelcount,
                   track->SampleDescription.audio.samplerate_hz) );
        }
    }

    return true;
}

bool MP4Demuxer::extractTrackInfo() {
    tracks_.clear();
    tracks_.reserve(demuxer_.track_count);

    for (unsigned int i = 0; i < demuxer_.track_count; ++i) {
        const MP4D_track_t* track = &demuxer_.track[i];
        TrackInfo track_info = createTrackInfo(*track);
        track_info.track_id = i;  // 使用数组索引作为track_id
        tracks_.push_back(track_info);
    }

    return true;
}

TrackInfo MP4Demuxer::createTrackInfo(const MP4D_track_t& track) {
    TrackInfo info;

    // 计算轨道持续时间
    if (track.timescale > 0) {
        uint64_t track_duration = ((uint64_t)track.duration_hi << 32) | track.duration_lo;
        info.duration = static_cast<double>(track_duration) / track.timescale;
    } else {
        info.duration = 0.0;
    }

    // 判断轨道类型（基于验证结果的handler_type）
    if (track.handler_type == MP4D_HANDLER_TYPE_VIDE) {
        info.type = MP4TrackType::VIDEO;

        // 检查视频编码类型
        if (track.object_type_indication == MP4_OBJECT_TYPE_AVC) {
            info.codec = CodecType::H264;
        }

        // 获取视频尺寸
        info.width = track.SampleDescription.video.width;
        info.height = track.SampleDescription.video.height;

        // 设置timescale用于时间戳转换
        info.timescale = track.timescale;

        PLUGIN_H264_LOG( ("Video track: timescale=%u, duration=%llu, calculated_duration=%.3fs\n",
               track.timescale, ((uint64_t)track.duration_hi << 32) | track.duration_lo, info.duration) );

    } else if (track.handler_type == MP4D_HANDLER_TYPE_SOUN) {
        info.type = MP4TrackType::AUDIO;

        // 检查音频编码类型
        if (track.object_type_indication == MP4_OBJECT_TYPE_AUDIO_ISO_IEC_14496_3) {
            info.codec = CodecType::AAC;
        }

        // 获取音频参数
        info.sample_rate = track.SampleDescription.audio.samplerate_hz;
        info.channels = track.SampleDescription.audio.channelcount;
        info.timescale = track.timescale;

        PLUGIN_H264_LOG( ("Audio track: samplerate=%u, timescale=%u, duration=%.3fs\n",
               track.SampleDescription.audio.samplerate_hz, track.timescale, info.duration) );
    }

    return info;
}

std::vector<TrackInfo> MP4Demuxer::getTrackInfo() const {
    return tracks_;
}

bool MP4Demuxer::extractSPS(int track_id, std::vector<uint8_t>& sps_data) const {
    if (!is_open_ || track_id < 0 || track_id >= static_cast<int>(demuxer_.track_count)) {
        return false;
    }

    const MP4D_track_t* track = &demuxer_.track[track_id];

    // 检查是否为H.264视频轨
    if (track->handler_type != MP4D_HANDLER_TYPE_VIDE ||
        track->object_type_indication != MP4_OBJECT_TYPE_AVC) {
        return false;
    }

    // 提取SPS数据（修正API使用方式）
    int sps_size = 0;
    const void* sps = MP4D_read_sps(&demuxer_, track_id, 0, &sps_size);
    if (sps == nullptr || sps_size <= 0) {
        return false;
    }

    // 复制SPS数据
    sps_data.clear();
    sps_data.assign(static_cast<const uint8_t*>(sps), static_cast<const uint8_t*>(sps) + sps_size);

    return true;
}

bool MP4Demuxer::extractPPS(int track_id, std::vector<uint8_t>& pps_data) const {
    if (!is_open_ || track_id < 0 || track_id >= static_cast<int>(demuxer_.track_count)) {
        return false;
    }

    const MP4D_track_t* track = &demuxer_.track[track_id];

    // 检查是否为H.264视频轨
    if (track->handler_type != MP4D_HANDLER_TYPE_VIDE ||
        track->object_type_indication != MP4_OBJECT_TYPE_AVC) {
        return false;
    }

    // 提取PPS数据（修正API使用方式）
    int pps_size = 0;
    const void* pps = MP4D_read_pps(&demuxer_, track_id, 0, &pps_size);
    if (pps == nullptr || pps_size <= 0) {
        return false;
    }

    // 复制PPS数据
    pps_data.clear();
    pps_data.assign(static_cast<const uint8_t*>(pps), static_cast<const uint8_t*>(pps) + pps_size);

    return true;
}

bool MP4Demuxer::readNextSample(int track_id, MP4Sample& sample) {
    if (!is_open_ || track_id < 0 || track_id >= static_cast<int>(demuxer_.track_count)) {
        setError(H264Error::INVALID_PARAM, "Invalid track ID");
        return false;
    }

    // 使用该轨道的专用sample索引
    unsigned int& sample_index = track_sample_indices_[track_id];

    unsigned int frame_bytes = 0;
    unsigned int timestamp = 0;
    unsigned int duration = 0;

    // 获取样本信息
    MP4D_file_offset_t offset = MP4D_frame_offset(&demuxer_, track_id, sample_index, &frame_bytes, &timestamp, &duration);

    if (offset == 0 || frame_bytes == 0) {
        // 该轨道文件结束或错误
        PLUGIN_H264_LOG( ("Track %d: End of track reached at sample %u\n", track_id, sample_index) );
        return false;
    }

    // 读取样本数据（需要从文件中读取）
    file_.seekg(offset, std::ios::beg);
    if (!file_.good()) {
        setError(H264Error::FILE_OPEN_FAILED, "Failed to seek to sample position");
        return false;
    }

    // 分配和读取数据
    sample.data.clear();
    sample.data.resize(frame_bytes);

    file_.read(reinterpret_cast<char*>(sample.data.data()), frame_bytes);
    if (file_.gcount() != static_cast<std::streamsize>(frame_bytes)) {
        setError(H264Error::FILE_OPEN_FAILED, "Failed to read sample data");
        return false;
    }

    // 设置样本信息
    sample.timestamp = timestamp;
    sample.duration = duration;
    sample.is_keyframe = false;  // MiniMP4可能没有直接的关键帧标志

    // 更新当前时间（基于当前轨道）
    const MP4D_track_t* track = &demuxer_.track[track_id];
    if (track->timescale > 0) {
        current_time_ = static_cast<double>(timestamp) / track->timescale;
    }

    // 调试输出
    PLUGIN_H264_LOG( ("Track %d sample %u: timestamp=%u, duration=%u, size=%u\n",
           track_id, sample_index, timestamp, duration, frame_bytes) );

    sample_index++;  // 移动到该轨道的下一个样本
    return true;
}

bool MP4Demuxer::seekToTime(double timestamp) {
    if (!is_open_) {
        setError(H264Error::DECODER_INIT_FAILED, "Demuxer not open");
        return false;
    }

    // MiniMP4可能不支持精确的时间定位
    // 这里提供基础实现

    if (timestamp < 0.0) {
        timestamp = 0.0;
    } else if (timestamp > duration_) {
        timestamp = duration_;
    }

    // 重置所有轨道的sample索引
    for (size_t i = 0; i < track_sample_indices_.size(); ++i) {
        track_sample_indices_[i] = 0;
    }

    // 简单的重置到开头（MiniMP4限制）
    if (timestamp == 0.0) {
        current_time_ = 0.0;
        PLUGIN_H264_LOG( ("Seeked to beginning, reset all track indices\n") );
        return true;
    }

    // 对于非零时间戳，需要为每个轨道计算正确的sample索引
    for (unsigned int track_id = 0; track_id < demuxer_.track_count; ++track_id) {
        const MP4D_track_t* track = &demuxer_.track[track_id];
        if (track->timescale > 0) {
            // 计算对应的sample索引（简化实现）
            uint64_t target_timestamp = static_cast<uint64_t>(timestamp * track->timescale);

            // 遍历样本找到最接近的索引
            unsigned int sample_index = 0;
            unsigned int frame_bytes, sample_timestamp, duration;

            while (sample_index < track->sample_count) {
                MP4D_file_offset_t offset = MP4D_frame_offset(&demuxer_, track_id, sample_index,
                                                             &frame_bytes, &sample_timestamp, &duration);
                if (offset == 0) break;

                if (sample_timestamp >= target_timestamp) {
                    break;
                }
                sample_index++;
            }

            track_sample_indices_[track_id] = sample_index;
            PLUGIN_H264_LOG( ("Track %d: Seeked to sample %u (timestamp %u, target %.3fs)\n",
                   track_id, sample_index, sample_timestamp, timestamp) );
        }
    }

    current_time_ = timestamp;
    return true;
}

double MP4Demuxer::getDuration() const {
    return duration_;
}

double MP4Demuxer::getCurrentTime() const {
    return current_time_;
}

// 静态回调函数（基于验证结果）
int MP4Demuxer::readCallback(int64_t offset, void* buffer, size_t size, void* token) {
    MP4Demuxer* demuxer = static_cast<MP4Demuxer*>(token);
    if (demuxer == nullptr || !demuxer->file_.is_open()) {
        return 1;
    }

    // 定位到指定偏移
    demuxer->file_.seekg(offset, std::ios::beg);
    if (!demuxer->file_.good()) {
        return 1;
    }

    // 读取数据
    demuxer->file_.read(static_cast<char*>(buffer), size);
    // return static_cast<int>(demuxer->file_.gcount());
    return 0;
}

} // namespace plugin_h264
