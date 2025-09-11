#include "../include/managers/H264Movie.h"
#include "../include/decoders/MP4Demuxer.h"

namespace plugin_h264 {

// 辅助函数：将AVCC格式转换为Annex-B格式
// std::vector<uint8_t> convertAVCCToAnnexB(const std::vector<uint8_t>& avcc_data, int nal_length_size = 4) {
//     std::vector<uint8_t> annexb_data;

//     if (avcc_data.size() < nal_length_size) {
//         return annexb_data; // 数据太短，无效
//     }

//     size_t offset = 0;
//     while (offset + nal_length_size <= avcc_data.size()) {
//         // 读取NAL单元长度（大端序）
//         uint32_t nal_length = 0;
//         for (int i = 0; i < nal_length_size; ++i) {
//             nal_length = (nal_length << 8) | avcc_data[offset + i];
//         }

//         offset += nal_length_size;

//         // 检查长度是否有效
//         if (nal_length == 0 || offset + nal_length > avcc_data.size()) {
//             PLUGIN_H264_LOG( ("Invalid NAL length: %u, remaining data: %zu\n", nal_length, avcc_data.size() - offset) );
//             break; // 无效长度，停止处理
//         }

//         // 添加Annex-B起始码
//         annexb_data.insert(annexb_data.end(), {0x00, 0x00, 0x00, 0x01});

//         // 添加NAL单元数据
//         annexb_data.insert(annexb_data.end(),
//                           avcc_data.begin() + offset,
//                           avcc_data.begin() + offset + nal_length);

//         PLUGIN_H264_LOG( ("Converted NAL unit: length=%u, type=0x%02X\n", nal_length,
//                nal_length > 0 ? avcc_data[offset] & 0x1F : 0) );

//         offset += nal_length;
//     }

//     return annexb_data;
// }

H264Movie::H264Movie()
    : is_loaded_(false)
    , is_playing_(false)
    , duration_(0.0)
    , has_new_video_frame_(false)
    , has_new_audio_frame_(false)
    , video_track_finished_(false)
    , audio_track_finished_(false)
    , sps_pps_sent_(false)
    , aac_configured_(false) {

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

    // 预配置H264解码器的SPS/PPS
    auto h264_decoder = decoder_manager_->getH264Decoder();
    auto demuxer = decoder_manager_->getMP4Demuxer();

    if (h264_decoder && demuxer) {
        // 查找H264视频轨道
        for (const auto& track : tracks_) {
            if (track.type == MP4TrackType::VIDEO && track.codec == CodecType::H264) {
                std::vector<uint8_t> sps_data, pps_data;
                if (demuxer->extractSPS(track.track_id, sps_data) &&
                    demuxer->extractPPS(track.track_id, pps_data)) {

                    PLUGIN_H264_LOG( ("Pre-configuring H264 decoder with SPS (size: %zu) and PPS (size: %zu)\n",
                           sps_data.size(), pps_data.size()) );

                    // 为SPS添加NAL起始码
                    std::vector<uint8_t> sps_nal;
                    sps_nal.insert(sps_nal.end(), {0x00, 0x00, 0x00, 0x01});
                    sps_nal.insert(sps_nal.end(), sps_data.begin(), sps_data.end());

                    // 为PPS添加NAL起始码
                    std::vector<uint8_t> pps_nal;
                    pps_nal.insert(pps_nal.end(), {0x00, 0x00, 0x00, 0x01});
                    pps_nal.insert(pps_nal.end(), pps_data.begin(), pps_data.end());

                    // 发送SPS和PPS到解码器
                    VideoFrame dummy_frame;
                    bool sps_result = h264_decoder->decode(sps_nal.data(), sps_nal.size(), dummy_frame);
                    bool pps_result = h264_decoder->decode(pps_nal.data(), pps_nal.size(), dummy_frame);

                    if (sps_result && pps_result) {
                        PLUGIN_H264_LOG( ("H264 decoder pre-configuration successful\n") );
                        sps_pps_sent_ = true;
                    } else {
                        PLUGIN_H264_LOG( ("H264 decoder pre-configuration failed: SPS=%s, PPS=%s\n",
                               sps_result ? "OK" : "FAIL", pps_result ? "OK" : "FAIL") );
                    }
                }
                break; // 只处理第一个H264轨道
            }
        }
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
    video_track_finished_ = false;
    audio_track_finished_ = false;
    sps_pps_sent_ = false;
    aac_configured_ = false;

    clearError();
    return true;
}

bool H264Movie::isPlaying() const {
    return is_playing_;
}

bool H264Movie::replay() {
    if (!is_loaded_ || !decoder_manager_) {
        setError(H264Error::DECODER_INIT_FAILED, "Movie not loaded");
        return false;
    }

    PLUGIN_H264_LOG( ("Starting replay...\n") );

    // 只停止播放，不关闭文件
    is_playing_ = false;

    // 重置所有解码状态，但保持文件加载状态
    has_new_video_frame_ = false;
    has_new_audio_frame_ = false;
    video_track_finished_ = false;
    audio_track_finished_ = false;
    sps_pps_sent_ = false;
    aac_configured_ = false;

    // 清空当前帧数据
    current_video_frame_ = VideoFrame();
    current_audio_frame_ = AudioFrame();

    PLUGIN_H264_LOG( ("Reset decoder states for replay - sps_pps_sent_: %s, aac_configured_: %s\n",
           sps_pps_sent_ ? "true" : "false", aac_configured_ ? "true" : "false") );

    // 重置解码器内部状态
    auto h264_decoder = decoder_manager_->getH264Decoder();
    auto aac_decoder = decoder_manager_->getAACDecoder();

    if (h264_decoder) {
        // 重置H264解码器
        PLUGIN_H264_LOG( ("Resetting H264 decoder...\n") );
        h264_decoder->reset();
    }

    if (aac_decoder) {
        // 重置AAC解码器
        PLUGIN_H264_LOG( ("Resetting AAC decoder...\n") );
        aac_decoder->reset();
    }

    // 重置到文件开头
    if (!seekTo(0.0)) {
        setError(H264Error::DECODE_FAILED, "Failed to seek to beginning for replay");
        return false;
    }

    PLUGIN_H264_LOG( ("Seeked to beginning successfully\n") );

    // 开始播放
    if (!play()) {
        setError(H264Error::DECODE_FAILED, "Failed to start playback for replay");
        return false;
    }

    PLUGIN_H264_LOG( ("Replay started successfully\n") );
    clearError();
    return true;
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

bool H264Movie::hasAudioTrack() const {
    for (const auto& track : tracks_) {
        if (track.type == MP4TrackType::AUDIO) {
            return true;
        }
    }
    return false;
}

bool H264Movie::isVideoTrackFinished() const {
    return video_track_finished_;
}

bool H264Movie::isAudioTrackFinished() const {
    return audio_track_finished_;
}

bool H264Movie::isPlaybackFinished() const {
    // 视频轨道必须完成
    bool video_done = video_track_finished_;

    // 如果有音频轨道，音频也必须完成
    bool audio_done = !hasAudioTrack() || audio_track_finished_;

    return video_done && audio_done;
}

VideoFrame H264Movie::getCurrentVideoFrame() {
    has_new_video_frame_ = false;
    return current_video_frame_;
}

AudioFrame H264Movie::getCurrentAudioFrame() {
    has_new_audio_frame_ = false;
    return current_audio_frame_;
}

bool H264Movie::decodeNextFrame() {
    // 现在使用分离的解码函数，优先解码视频
    bool decoded_video = false;
    bool decoded_audio = false;

    if (!has_new_video_frame_) {
        decoded_video = decodeNextVideoFrame();
    }

    if (!has_new_audio_frame_) {
        decoded_audio = decodeNextAudioFrame();
    }

    return decoded_video || decoded_audio;
}

bool H264Movie::decodeNextVideoFrame() {
    if (!is_loaded_ || !decoder_manager_) {
        setError(H264Error::DECODER_INIT_FAILED, "Movie not loaded");
        return false;
    }

    auto demuxer = decoder_manager_->getMP4Demuxer();
    auto h264_decoder = decoder_manager_->getH264Decoder();

    if (!demuxer || !h264_decoder) {
        setError(H264Error::DECODER_INIT_FAILED, "Video decoder not available");
        return false;
    }

    // 如果已经有新的视频帧，不需要解码
    if (has_new_video_frame_) {
        return true;
    }

    // 获取轨道信息
    auto track_info = demuxer->getTrackInfo();

    for (size_t i = 0; i < track_info.size(); ++i) {
        const auto& track = track_info[i];
        if (track.type == MP4TrackType::VIDEO && track.codec == CodecType::H264) {

            // 优化：只在第一次或seek后需要发送SPS/PPS
            if (!sps_pps_sent_) {
                std::vector<uint8_t> sps_data, pps_data;
                if (demuxer->extractSPS(track.track_id, sps_data) &&
                    demuxer->extractPPS(track.track_id, pps_data)) {

                    PLUGIN_H264_LOG( ("Sending SPS (size: %zu) and PPS (size: %zu) to decoder\n", sps_data.size(), pps_data.size()) );

                    // 为SPS添加NAL起始码
                    std::vector<uint8_t> sps_nal;
                    sps_nal.reserve(sps_data.size() + 4);
                    sps_nal.insert(sps_nal.end(), {0x00, 0x00, 0x00, 0x01});
                    sps_nal.insert(sps_nal.end(), sps_data.begin(), sps_data.end());

                    // 为PPS添加NAL起始码
                    std::vector<uint8_t> pps_nal;
                    pps_nal.reserve(pps_data.size() + 4);
                    pps_nal.insert(pps_nal.end(), {0x00, 0x00, 0x00, 0x01});
                    pps_nal.insert(pps_nal.end(), pps_data.begin(), pps_data.end());

                    // 发送SPS和PPS到解码器
                    VideoFrame dummy_frame;
                    bool sps_result = h264_decoder->decode(sps_nal.data(), sps_nal.size(), dummy_frame);
                    bool pps_result = h264_decoder->decode(pps_nal.data(), pps_nal.size(), dummy_frame);

                    PLUGIN_H264_LOG( ("SPS decode result: %s, PPS decode result: %s\n",
                           sps_result ? "success" : "failed",
                           pps_result ? "success" : "failed") );

                    sps_pps_sent_ = true;
                }
            }

            MP4Sample sample;
            if (demuxer->readNextSample(track.track_id, sample)) {
                // 优化：预分配缓冲区大小，避免动态扩展
                std::vector<uint8_t> annexb_frame;
                annexb_frame.reserve(sample.data.size() + 64); // 预留额外空间给起始码
                
                size_t offset = 0;
                while (offset < sample.data.size()) {
                    if (offset + 4 > sample.data.size()) break;

                    // 读取NAL单元长度（大端序）
                    uint32_t nal_length = (sample.data[offset] << 24) |
                                         (sample.data[offset + 1] << 16) |
                                         (sample.data[offset + 2] << 8) |
                                         sample.data[offset + 3];

                    offset += 4;

                    if (nal_length > sample.data.size() - offset) break;

                    // 使用push_back批量添加，比insert更高效
                    annexb_frame.push_back(0x00);
                    annexb_frame.push_back(0x00);
                    annexb_frame.push_back(0x00);
                    annexb_frame.push_back(0x01);
                    
                    // 添加NAL单元数据
                    annexb_frame.insert(annexb_frame.end(),
                                       sample.data.begin() + offset,
                                       sample.data.begin() + offset + nal_length);

                    offset += nal_length;
                }

                // 检查NAL单元类型
                uint8_t nal_type = (annexb_frame.size() > 4) ? (annexb_frame[4] & 0x1F) : 0;
                PLUGIN_H264_LOG( ("NAL unit type: 0x%02X (%s)\n", nal_type,
                       nal_type == 5 ? "IDR" :
                       nal_type == 1 ? "P-frame" :
                       nal_type == 7 ? "SPS" :
                       nal_type == 8 ? "PPS" : "Other") );

                VideoFrame video_frame;
                if (h264_decoder->decode(annexb_frame.data(), annexb_frame.size(), video_frame)) {
                    current_video_frame_ = video_frame;

                    // 正确转换时间戳：统一使用轨道的timescale
                    if (track.timescale > 0) {
                        current_video_frame_.timestamp = static_cast<double>(sample.timestamp) / track.timescale;
                    } else {
                        // 如果没有timescale信息，假设90kHz（H.264常用）
                        current_video_frame_.timestamp = static_cast<double>(sample.timestamp) / 90000.0;
                    }

                    PLUGIN_H264_LOG( ("Video frame timestamp: %u -> %.3fs (timescale: %u)\n",
                           sample.timestamp, current_video_frame_.timestamp, track.timescale) );

                    has_new_video_frame_ = true;
                    clearError();
                    return true;
                }
            } else {
                // 无法读取更多视频样本，视频轨道已完成
                video_track_finished_ = true;
                PLUGIN_H264_LOG( ("Video track finished - no more samples available\n") );
            }
        }
    }

    return false;
}

bool H264Movie::decodeNextAudioFrame() {
    if (!is_loaded_ || !decoder_manager_) {
        setError(H264Error::DECODER_INIT_FAILED, "Movie not loaded");
        return false;
    }

    auto demuxer = decoder_manager_->getMP4Demuxer();
    auto aac_decoder = decoder_manager_->getAACDecoder();

    if (!demuxer || !aac_decoder) {
        // 没有音频解码器不是错误，可能文件没有音频
        return false;
    }

    // 如果已经有新的音频帧，不需要解码
    if (has_new_audio_frame_) {
        return true;
    }

    // 获取轨道信息
    auto track_info = demuxer->getTrackInfo();

    for (size_t i = 0; i < track_info.size(); ++i) {
        const auto& track = track_info[i];
        if (track.type == MP4TrackType::AUDIO && track.codec == CodecType::AAC) {
            // 首先确保AAC解码器已经配置
            if (!aac_configured_) {
                // 创建基本的AudioSpecificConfig
                std::vector<uint8_t> asc_data;

                // 根据轨道信息构建ASC
                int sample_rate = track.sample_rate;
                int channels = track.channels;

                // AAC-LC (Audio Object Type = 2)
                uint8_t audio_object_type = 2;

                // 采样率索引映射
                uint8_t sampling_freq_index = 4; // 默认44100Hz
                if (sample_rate == 96000) sampling_freq_index = 0;
                else if (sample_rate == 88200) sampling_freq_index = 1;
                else if (sample_rate == 64000) sampling_freq_index = 2;
                else if (sample_rate == 48000) sampling_freq_index = 3;
                else if (sample_rate == 44100) sampling_freq_index = 4;
                else if (sample_rate == 32000) sampling_freq_index = 5;
                else if (sample_rate == 24000) sampling_freq_index = 6;
                else if (sample_rate == 22050) sampling_freq_index = 7;
                else if (sample_rate == 16000) sampling_freq_index = 8;
                else if (sample_rate == 12000) sampling_freq_index = 9;
                else if (sample_rate == 11025) sampling_freq_index = 10;
                else if (sample_rate == 8000) sampling_freq_index = 11;

                // 声道配置
                uint8_t channel_config = std::min(channels, 7);

                // 构建ASC (2字节)
                uint16_t asc = (audio_object_type << 11) | (sampling_freq_index << 7) | (channel_config << 3);
                asc_data.push_back((asc >> 8) & 0xFF);
                asc_data.push_back(asc & 0xFF);

                if (aac_decoder->configureWithASC(asc_data.data(), asc_data.size())) {
                    aac_configured_ = true;
                }
            }

            if (aac_configured_) {
                MP4Sample sample;
                if (demuxer->readNextSample(track.track_id, sample)) {
                    AudioFrame audio_frame;
                    if (aac_decoder->decode(sample.data.data(), sample.data.size(), audio_frame)) {
                        current_audio_frame_ = audio_frame;

                        // 修复音频时间戳：使用与视频相同的timescale逻辑
                        if (track.timescale > 0) {
                            current_audio_frame_.timestamp = static_cast<double>(sample.timestamp) / track.timescale;
                        } else {
                            // 如果没有timescale信息，使用音频采样率作为时间基准
                            current_audio_frame_.timestamp = static_cast<double>(sample.timestamp) / (track.sample_rate > 0 ? track.sample_rate : 44100);
                        }

                        // 音频时间戳已正确设置，用于播放速度控制
                        PLUGIN_H264_LOG( ("Audio frame timestamp: %u -> %.3fs (timescale: %u, samplerate: %d)\n",
                               sample.timestamp, current_audio_frame_.timestamp, track.timescale, track.sample_rate) );

                        has_new_audio_frame_ = true;
                        clearError();
                        return true;
                    }
                } else {
                    // 无法读取更多音频样本，音频轨道已完成
                    audio_track_finished_ = true;
                    PLUGIN_H264_LOG( ("Audio track finished - no more samples available\n") );
                }
            }
        }
    }

    return false;
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

    // 重置必要的解码状态
    has_new_video_frame_ = false;
    has_new_audio_frame_ = false;

    // 对于 seek 操作，重置 SPS/PPS 状态以处理可能的 B-frame 参考帧问题
    // 但保持其他状态不变，避免影响播放连续性
    sps_pps_sent_ = false;

    // 清空当前帧数据，强制获取新位置的帧
    current_video_frame_ = VideoFrame();
    current_audio_frame_ = AudioFrame();

    PLUGIN_H264_LOG( ("Seek completed to %.3fs, reset frame state\n", timestamp) );

    clearError();
    return true;
}

} // namespace plugin_h264
