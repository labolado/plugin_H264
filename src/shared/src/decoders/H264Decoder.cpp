#include "../include/decoders/H264Decoder.h"
#include <cstring>
#include <algorithm>

namespace plugin_h264 {

H264Decoder::H264Decoder()
    : decoder_(nullptr)
    , initialized_(false)
    , frame_width_(0)
    , frame_height_(0)
    , frame_buffer_size_(0)
    , require_compact_format_(false)  // 默认使用零拷贝模式
    , buffer_pool_(3) {  // 使用3个缓冲区的池
}

H264Decoder::~H264Decoder() {
    destroy();
}

bool H264Decoder::initialize() {
    if (initialized_) {
        return true;
    }

    // 基于验证结果：使用WelsCreateDecoder创建解码器
    long result = WelsCreateDecoder(&decoder_);
    if (result != 0 || decoder_ == nullptr) {
        setError(H264Error::DECODER_INIT_FAILED,
                "Failed to create OpenH264 decoder, error code: " + std::to_string(result));
        return false;
    }

    // 设置解码器选项
    if (!setupDecoderOptions()) {
        destroy();
        return false;
    }

    initialized_ = true;
    clearError();
    return true;
}

bool H264Decoder::setupDecoderOptions() {
    // 基于验证结果：使用DECODER_OPTION_ERROR_CON_IDC而不是DECODER_OPTION_DATAFORMAT
    SDecodingParam sDecParam;
    memset(&sDecParam, 0, sizeof(SDecodingParam));

    sDecParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_AVC;
    sDecParam.uiTargetDqLayer = UCHAR_MAX;  // 解码所有层
    sDecParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;  // 错误隐藏
    sDecParam.sVideoProperty.size = sizeof(sDecParam.sVideoProperty);

    int ret = decoder_->Initialize(&sDecParam);
    if (ret != 0) {
        setError(H264Error::DECODER_INIT_FAILED,
                "Failed to initialize decoder parameters, error: " + std::to_string(ret));
        return false;
    }

    // 设置错误隐藏选项（基于验证通过的API）
    unsigned int uiEcIdc = ERROR_CON_SLICE_COPY;
    ret = decoder_->SetOption(DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
    if (ret != 0) {
        // 这不是致命错误，只是记录警告
        setError(H264Error::NONE, "Warning: Failed to set error concealment option");
    }

    return true;
}

bool H264Decoder::decode(const uint8_t* nal_data, size_t nal_size, VideoFrame& frame) {
    if (!initialized_ || decoder_ == nullptr) {
        setError(H264Error::DECODER_INIT_FAILED, "Decoder not initialized");
        return false;
    }

    if (nal_data == nullptr || nal_size == 0) {
        setError(H264Error::INVALID_PARAM, "Invalid NAL data");
        return false;
    }

    uint8_t* pData[3] = {0};
    SBufferInfo sDstBufInfo;
    memset(&sDstBufInfo, 0, sizeof(SBufferInfo));

    // 调用 OpenH264 解码
    DECODING_STATE ret = decoder_->DecodeFrame2(
        nal_data,
        static_cast<int>(nal_size),
        pData,
        &sDstBufInfo
    );

    // 仅在严重错误时 flush（比如解码器状态崩坏）
    if (ret == dsNoParamSets || ret == dsDataErrorConcealed) {
        PLUGIN_H264_LOG(("H264 Decoder warning (recoverable): %d\n", ret));
        return false; // 忽略这帧
    } else if (ret != dsErrorFree) {
        PLUGIN_H264_LOG(("H264 Decoder fatal error: %d, flushing\n", ret));
        memset(pData, 0, sizeof(pData));
        memset(&sDstBufInfo, 0, sizeof(SBufferInfo));
        decoder_->DecodeFrame2(nullptr, 0, pData, &sDstBufInfo); // flush 一次
        return false;
    }

    // 如果没有输出帧（需要更多 NAL 累积），直接返回
    if (sDstBufInfo.iBufferStatus != 1) {
        return false;
    }

    // 读取帧参数
    int width = sDstBufInfo.UsrData.sSystemBuffer.iWidth;
    int height = sDstBufInfo.UsrData.sSystemBuffer.iHeight;
    int stride_y = sDstBufInfo.UsrData.sSystemBuffer.iStride[0];
    int stride_uv = sDstBufInfo.UsrData.sSystemBuffer.iStride[1];

    if (width <= 0 || height <= 0) {
        setError(H264Error::DECODE_FAILED, "Invalid frame dimensions");
        return false;
    }

    frame_width_ = width;
    frame_height_ = height;

    // ✅ 直接引用解码器输出（零拷贝模式）
    // 如果调用方能接受 stride ≠ width，就直接用
    frame.width = width;
    frame.height = height;
    frame.y_stride = stride_y;
    frame.uv_stride = stride_uv;
    frame.y_plane = pData[0];
    frame.u_plane = pData[1];
    frame.v_plane = pData[2];
    frame.zero_copy_mode = true;

    // 如果调用方 **必须要紧凑格式**，再做一次 memcpy
    if (require_compact_format_) {
        if (!allocateFrameBuffer(width, height)) {
            return false;
        }

        // Y plane
        for (int i = 0; i < height; i++) {
            memcpy(frame_buffer_.get() + i * width,
                   pData[0] + i * stride_y,
                   width);
        }

        // U/V planes
        int uv_width = width / 2;
        int uv_height = height / 2;
        for (int i = 0; i < uv_height; i++) {
            memcpy(frame_buffer_.get() + width * height + i * uv_width,
                   pData[1] + i * stride_uv,
                   uv_width);
            memcpy(frame_buffer_.get() + width * height + uv_width * uv_height + i * uv_width,
                   pData[2] + i * stride_uv,
                   uv_width);
        }

        // 更新 frame 指针为紧凑格式
        frame.y_plane = frame_buffer_.get();
        frame.u_plane = frame_buffer_.get() + width * height;
        frame.v_plane = frame.u_plane + uv_width * uv_height;
        frame.y_stride = width;
        frame.uv_stride = uv_width;
        frame.zero_copy_mode = false;
    }

    clearError();
    return true;
}

bool H264Decoder::allocateFrameBuffer(int width, int height) {
    // 计算YUV420帧大小
    size_t y_size = width * height;
    size_t uv_size = (width / 2) * (height / 2);
    size_t total_size = y_size + uv_size * 2;

    if (frame_buffer_size_ < total_size) {
        // 释放旧缓冲区到池中
        if (frame_buffer_) {
            buffer_pool_.release(std::move(frame_buffer_), frame_buffer_size_);
        }
        
        try {
            // 从池中获取新缓冲区
            frame_buffer_ = buffer_pool_.acquire(total_size);
            frame_buffer_size_ = total_size;
        } catch (const std::bad_alloc&) {
            setError(H264Error::OUT_OF_MEMORY,
                    "Failed to allocate frame buffer of size: " + std::to_string(total_size));
            return false;
        }
    }

    return true;
}

void H264Decoder::freeFrameBuffer() {
    if (frame_buffer_) {
        buffer_pool_.release(std::move(frame_buffer_), frame_buffer_size_);
    }
    frame_buffer_size_ = 0;
}

bool H264Decoder::getDecoderInfo(int& width, int& height) const {
    if (!initialized_) {
        return false;
    }

    width = frame_width_;
    height = frame_height_;
    return frame_width_ > 0 && frame_height_ > 0;
}

void H264Decoder::reset() {
    if (initialized_ && decoder_ != nullptr) {
        // 重置解码器状态（如果API支持）
        // OpenH264可能需要重新初始化
        destroy();
        initialize();
    }
}

void H264Decoder::destroy() {
    if (decoder_ != nullptr) {
        WelsDestroyDecoder(decoder_);
        decoder_ = nullptr;
    }

    freeFrameBuffer();
    buffer_pool_.clear();  // 清空缓冲池
    initialized_ = false;
    frame_width_ = 0;
    frame_height_ = 0;
    clearError();
}

size_t H264Decoder::getMemoryUsage() const {
    // 估计内存使用量
    size_t base_usage = sizeof(*this);  // 对象本身
    size_t buffer_usage = frame_buffer_size_;  // 帧缓冲区
    size_t decoder_usage = initialized_ ? 1024 * 1024 : 0;  // 解码器内部状态（估计1MB）

    return base_usage + buffer_usage + decoder_usage;
}

} // namespace plugin_h264
