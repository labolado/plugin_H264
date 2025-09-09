#include "../include/decoders/AACDecoder.h"
#include <cstring>
#include <algorithm>

namespace plugin_h264 {

AACDecoder::AACDecoder()
    : decoder_(nullptr)
    , initialized_(false)
    , sample_rate_(0)
    , channels_(0)
    , bits_per_sample_(16) {
}

AACDecoder::~AACDecoder() {
    destroy();
}

bool AACDecoder::initialize() {
    if (initialized_) {
        return true;
    }

    // 创建AAC解码器（基于验证结果）
    decoder_ = aacDecoder_Open(TT_MP4_RAW, 1);
    if (decoder_ == nullptr) {
        setError(H264Error::DECODER_INIT_FAILED, "Failed to create FDK-AAC decoder");
        return false;
    }

    // 设置解码器配置
    if (!setupDecoderConfig()) {
        destroy();
        return false;
    }

    // 预分配输出缓冲区（立体声，16位，2048样本）
    output_buffer_.resize(2048 * 2);

    initialized_ = true;
    clearError();
    return true;
}

bool AACDecoder::setupDecoderConfig() {
    // 设置解码器参数（基于验证结果的配置）
    AAC_DECODER_ERROR err;

    // 设置输出声道数限制（支持立体声到7.1）
    UINT max_channels = 8;
    err = aacDecoder_SetParam(decoder_, AAC_PCM_MAX_OUTPUT_CHANNELS, max_channels);
    if (err != AAC_DEC_OK) {
        setError(H264Error::DECODER_INIT_FAILED,
                "Failed to set max output channels: " + std::to_string(err));
        return false;
    }

    // 设置输出限制器（防止削波）
    err = aacDecoder_SetParam(decoder_, AAC_PCM_LIMITER_ENABLE, 1);
    if (err != AAC_DEC_OK) {
        // 这不是致命错误，只记录警告
        setError(H264Error::NONE, "Warning: Failed to enable limiter");
    }

    // 设置DRC（动态范围压缩）
    err = aacDecoder_SetParam(decoder_, AAC_DRC_ATTENUATION_FACTOR, 127); // 最大衰减
    if (err != AAC_DEC_OK) {
        // 这不是致命错误
        setError(H264Error::NONE, "Warning: Failed to set DRC parameters");
    }

    return true;
}

bool AACDecoder::configureWithASC(const uint8_t* asc_data, size_t asc_size) {
    if (!initialized_ || decoder_ == nullptr) {
        setError(H264Error::DECODER_INIT_FAILED, "Decoder not initialized");
        return false;
    }

    if (asc_data == nullptr || asc_size == 0) {
        setError(H264Error::INVALID_PARAM, "Invalid ASC data");
        return false;
    }

    // 配置解码器使用AudioSpecificConfig数据
    UCHAR* config_buffer = const_cast<UCHAR*>(asc_data);
    UINT config_size = static_cast<UINT>(asc_size);

    AAC_DECODER_ERROR err = aacDecoder_ConfigRaw(decoder_, &config_buffer, &config_size);
    if (err != AAC_DEC_OK) {
        setError(H264Error::DECODER_INIT_FAILED,
                "Failed to configure AAC decoder with ASC: " + std::to_string(err));
        return false;
    }

    // 更新解码器信息
    updateDecoderInfo();

    clearError();
    return true;
}

bool AACDecoder::decode(const uint8_t* aac_data, size_t aac_size, AudioFrame& frame) {
    if (!initialized_ || decoder_ == nullptr) {
        setError(H264Error::DECODER_INIT_FAILED, "Decoder not initialized");
        return false;
    }

    if (aac_data == nullptr || aac_size == 0) {
        setError(H264Error::INVALID_PARAM, "Invalid AAC data");
        return false;
    }

    // 准备输入数据
    UCHAR* input_buffer = const_cast<UCHAR*>(aac_data);
    UINT input_size = static_cast<UINT>(aac_size);
    UINT bytes_valid = input_size;

    // 填充解码器缓冲区
    AAC_DECODER_ERROR err = aacDecoder_Fill(decoder_, &input_buffer, &input_size, &bytes_valid);
    if (err != AAC_DEC_OK) {
        setError(H264Error::DECODE_FAILED,
                "Failed to fill decoder buffer: " + std::to_string(err));
        return false;
    }

    // 解码帧
    INT_PCM* output_ptr = output_buffer_.data();
    err = aacDecoder_DecodeFrame(decoder_, output_ptr, output_buffer_.size(), 0);

    if (err != AAC_DEC_OK) {
        if (err == AAC_DEC_NOT_ENOUGH_BITS) {
            // 需要更多数据，这不是错误
            return false;
        }
        setError(H264Error::DECODE_FAILED,
                "AAC Frame decode failed: " + std::to_string(err));
        return false;
    }

    // 更新解码器信息
    updateDecoderInfo();

    // 获取输出帧信息
    CStreamInfo* stream_info = aacDecoder_GetStreamInfo(decoder_);
    if (stream_info == nullptr) {
        setError(H264Error::DECODE_FAILED, "Failed to get stream info");
        return false;
    }

    // 填充输出帧
    frame.sample_rate = stream_info->sampleRate;
    frame.channels = stream_info->numChannels;

    // 复制PCM数据
    int frame_size = stream_info->frameSize;
    int total_samples = frame_size * stream_info->numChannels;

    frame.samples.clear();
    frame.samples.reserve(total_samples);

    // FDK-AAC输出INT_PCM（通常是int16_t），转换为我们的格式
    for (int i = 0; i < total_samples; ++i) {
        frame.samples.push_back(static_cast<int16_t>(output_ptr[i]));
    }

    // 更新内部状态
    sample_rate_ = frame.sample_rate;
    channels_ = frame.channels;

    clearError();
    return true;
}

void AACDecoder::updateDecoderInfo() {
    if (!initialized_ || decoder_ == nullptr) {
        return;
    }

    CStreamInfo* info = aacDecoder_GetStreamInfo(decoder_);
    if (info != nullptr) {
        sample_rate_ = info->sampleRate;
        channels_ = info->numChannels;
    }
}

bool AACDecoder::getDecoderInfo(int& sample_rate, int& channels, int& bits_per_sample) const {
    if (!initialized_) {
        return false;
    }

    sample_rate = sample_rate_;
    channels = channels_;
    bits_per_sample = bits_per_sample_;

    return sample_rate_ > 0 && channels_ > 0;
}

void AACDecoder::reset() {
    if (initialized_ && decoder_ != nullptr) {
        // FDK-AAC的重置方法
        aacDecoder_SetParam(decoder_, AAC_TPDEC_CLEAR_BUFFER, 1);
    }
}

void AACDecoder::destroy() {
    if (decoder_ != nullptr) {
        aacDecoder_Close(decoder_);
        decoder_ = nullptr;
    }

    output_buffer_.clear();
    initialized_ = false;
    sample_rate_ = 0;
    channels_ = 0;
    clearError();
}

std::vector<LIB_INFO> AACDecoder::getLibraryInfo() const {
    std::vector<LIB_INFO> lib_info_vector;

    // 获取库信息（基于验证结果修正的API）
    LIB_INFO lib_info;

    // 直接获取AAC解码器模块信息
    if (FDKlibInfo_getCapabilities(&lib_info, FDK_AACDEC) == 0) {
        lib_info_vector.push_back(lib_info);
    }

    return lib_info_vector;
}

size_t AACDecoder::getMemoryUsage() const {
    // 估计内存使用量
    size_t base_usage = sizeof(*this);  // 对象本身
    size_t buffer_usage = output_buffer_.capacity() * sizeof(INT_PCM);  // 输出缓冲区
    size_t decoder_usage = initialized_ ? 512 * 1024 : 0;  // 解码器内部状态（估计512KB）

    return base_usage + buffer_usage + decoder_usage;
}

} // namespace plugin_h264
