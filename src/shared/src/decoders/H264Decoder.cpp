#include "../include/decoders/H264Decoder.h"
#include <cstring>
#include <algorithm>
#include <thread>

namespace plugin_h264 {

H264Decoder::H264Decoder()
    : decoder_(nullptr)
    , initialized_(false)
    , frame_width_(0)
    , frame_height_(0)
    , frame_buffer_size_(0)
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
    
    // 暂时禁用多线程解码以排查问题
    // TODO: 调查为什么多线程设置导致解码器初始化失败
    /*
    // 启用多线程解码 - 在Initialize之后设置
    int num_threads = std::thread::hardware_concurrency();
    // 处理hardware_concurrency()返回0的情况
    if (num_threads == 0) {
        num_threads = 4; // 默认使用4线程
    }
    if (num_threads > 1) {
        // 限制最大线程数为8（避免过多线程造成开销）
        num_threads = std::min(num_threads, 8);
        ret = decoder_->SetOption(DECODER_OPTION_NUM_OF_THREADS, &num_threads);
        if (ret != 0) {
            // 如果设置线程失败，不影响解码功能
            PLUGIN_H264_LOG( ("Note: Multi-threading not available, using single thread\n") );
        } else {
            PLUGIN_H264_LOG( ("H264 decoder multi-threading enabled with %d threads\n", num_threads) );
        }
    }
    */

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

    // 解码帧
    DECODING_STATE ret = decoder_->DecodeFrame2(
        nal_data,
        static_cast<int>(nal_size),
        pData,
        &sDstBufInfo
    );

    if (ret != dsErrorFree) {
        PLUGIN_H264_LOG( ("H264 Decoder decode error: %d, forcing decoder flush\n", ret) );

        // 对于所有解码错误，都尝试刷新解码器
        memset(pData, 0, sizeof(pData));
        memset(&sDstBufInfo, 0, sizeof(SBufferInfo));

        // 调用解码器刷新，传入空数据
        DECODING_STATE flush_ret = decoder_->DecodeFrame2(nullptr, 0, pData, &sDstBufInfo);

        PLUGIN_H264_LOG( ("Decoder flush completed, flush result: %d\n", flush_ret) );

        return false;
    }

    // 检查是否有输出帧
    if (sDstBufInfo.iBufferStatus != 1) {
        // 没有输出帧（可能需要更多数据）
        return false;
    }

    // 提取帧信息
    int width = sDstBufInfo.UsrData.sSystemBuffer.iWidth;
    int height = sDstBufInfo.UsrData.sSystemBuffer.iHeight;
    int stride_y = sDstBufInfo.UsrData.sSystemBuffer.iStride[0];
    int stride_uv = sDstBufInfo.UsrData.sSystemBuffer.iStride[1];

    if (width <= 0 || height <= 0) {
        setError(H264Error::DECODE_FAILED, "Invalid frame dimensions");
        return false;
    }

    // 更新解码器尺寸信息
    frame_width_ = width;
    frame_height_ = height;

    // 分配或重新分配帧缓冲区
    if (!allocateFrameBuffer(width, height)) {
        return false;
    }

    // 复制YUV数据到输出帧
    frame.width = width;
    frame.height = height;

    int uv_width = width / 2;
    int uv_height = height / 2;

    // 设置正确的stride（我们复制到紧凑格式）
    frame.y_stride = width;
    frame.uv_stride = uv_width;

    PLUGIN_H264_LOG( ("H264 Decoder output: %dx%d, src_strides: Y=%d UV=%d, dst_strides: Y=%d UV=%d\n",
           width, height, stride_y, stride_uv, frame.y_stride, frame.uv_stride) );

    if (!pData[0] || !pData[1] || !pData[2]) {
        setError(H264Error::DECODE_FAILED, "Decoder returned null plane pointers");
        return false;
    }

    // 必须拷贝数据，因为pData指针在下次解码时会失效
    // Y平面
    frame.y_plane = frame_buffer_.get();
    uint8_t* src_y = pData[0];
    uint8_t* dst_y = frame.y_plane;
    
    // 优化：如果stride匹配，使用单次memcpy
    if (stride_y == width) {
        memcpy(dst_y, src_y, width * height);
    } else {
        for (int i = 0; i < height; ++i) {
            memcpy(dst_y, src_y, width);
            src_y += stride_y;
            dst_y += width;
        }
    }

    // U平面
    frame.u_plane = frame.y_plane + width * height;
    uint8_t* src_u = pData[1];
    uint8_t* dst_u = frame.u_plane;
    
    if (stride_uv == uv_width) {
        memcpy(dst_u, src_u, uv_width * uv_height);
    } else {
        for (int i = 0; i < uv_height; ++i) {
            memcpy(dst_u, src_u, uv_width);
            src_u += stride_uv;
            dst_u += uv_width;
        }
    }

    // V平面
    frame.v_plane = frame.u_plane + uv_width * uv_height;
    uint8_t* src_v = pData[2];
    uint8_t* dst_v = frame.v_plane;
    
    if (stride_uv == uv_width) {
        memcpy(dst_v, src_v, uv_width * uv_height);
    } else {
        for (int i = 0; i < uv_height; ++i) {
            memcpy(dst_v, src_v, uv_width);
            src_v += stride_uv;
            dst_v += uv_width;
        }
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
