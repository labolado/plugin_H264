#include "decoders/H264Decoder.h"
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
    , buffer_pool_(3)  // 使用3个缓冲区的池
    , timestamp_(0)
    , last_sps_byte_count_(0)
    , enable_multithreading_(true)
    , num_threads_(2) {
}

H264Decoder::~H264Decoder() {
    destroy();
}

bool H264Decoder::initialize(bool enable_multithreading, int num_threads) {
    if (initialized_) {
        return true;
    }
    
    // 紧急修复：暂时禁用多线程以防止崩溃
    enable_multithreading_ = false; // 强制单线程
    num_threads_ = 1;
    timestamp_ = 0;
    last_sps_byte_count_ = 0;
    accumulated_data_.clear();

    // 基于完整验证成功的官方多线程方法，但暂时使用单线程
    long result = WelsCreateDecoder(&decoder_);
    if (result != 0 || decoder_ == nullptr) {
        setError(H264Error::DECODER_INIT_FAILED,
                "Failed to create OpenH264 decoder, error code: " + std::to_string(result));
        return false;
    }

    // 暂时不设置多线程以避免崩溃
    PLUGIN_H264_LOG( ("Using single-thread mode for stability\n") );

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
    // 基于Mozilla和OpenH264官方测试代码的最佳实践
    SDecodingParam sDecParam;
    memset(&sDecParam, 0, sizeof(SDecodingParam));

    // 使用OpenH264官方多线程测试的配置：VIDEO_BITSTREAM_DEFAULT
    // 配合DecodeFrameNoDelay API实现多线程解码
    sDecParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;
    sDecParam.uiTargetDqLayer = UCHAR_MAX;  // 解码所有层
    sDecParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;  // 保持我们验证过的错误隐藏策略
    sDecParam.sVideoProperty.size = sizeof(sDecParam.sVideoProperty);

    PLUGIN_H264_LOG( ("Calling decoder->Initialize with VIDEO_BITSTREAM_AVC\n") );
    int ret = decoder_->Initialize(&sDecParam);
    PLUGIN_H264_LOG( ("decoder->Initialize returned: %d\n", ret) );
    
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

    // 基于官方测试：使用图片级累积解码方式
    accumulated_data_.insert(accumulated_data_.end(), nal_data, nal_data + nal_size);
    
    // 使用官方readPicture方法分割图片
    uint8_t* sps_ptr = nullptr;
    int32_t sps_byte_count = 0;
    int32_t picture_size = readPicture(
        accumulated_data_.data(), 
        accumulated_data_.size(), 
        0, 
        sps_ptr, 
        sps_byte_count
    );
    
    if (picture_size <= 0 || picture_size > (int32_t)accumulated_data_.size()) {
        // 需要更多数据
        return false;
    }
    
    // SPS变化检测（官方逻辑）
    if (last_sps_byte_count_ > 0 && sps_byte_count > 0) {
        if (sps_byte_count != last_sps_byte_count_ || 
            memcmp(sps_ptr, last_sps_buf_, last_sps_byte_count_) != 0) {
            PLUGIN_H264_LOG( ("SPS change detected, may need decoder flush\n") );
            // 这里应该调用FlushFrames，简化处理
        }
    }
    
    // 更新SPS缓存
    if (sps_byte_count > 0 && sps_ptr != nullptr) {
        if (sps_byte_count > 32) sps_byte_count = 32;
        last_sps_byte_count_ = sps_byte_count;
        memcpy(last_sps_buf_, sps_ptr, sps_byte_count);
    }

    uint8_t* pData[3] = {0};
    SBufferInfo sDstBufInfo;
    memset(&sDstBufInfo, 0, sizeof(SBufferInfo));
    sDstBufInfo.uiInBsTimeStamp = ++timestamp_;

    PLUGIN_H264_LOG_DECODE( ("Decode: Picture size=%d, decoder=%p, timestamp=%u\n", 
                      picture_size, decoder_, timestamp_) );

    // 使用官方的DecodeFrameNoDelay方法
    DECODING_STATE ret = decoder_->DecodeFrameNoDelay(
        accumulated_data_.data(),
        picture_size,
        pData,
        &sDstBufInfo
    );

    PLUGIN_H264_LOG_DECODE( ("DecodeFrameNoDelay returned: %d, BufferStatus=%d\n", ret, sDstBufInfo.iBufferStatus) );

    
    // 移除已处理的数据
    accumulated_data_.erase(accumulated_data_.begin(), accumulated_data_.begin() + picture_size);

    if (ret != dsErrorFree) {
        PLUGIN_H264_LOG( ("H264 Decoder decode error: %d\n", ret) );
        return false;
    }

    // 检查是否有输出帧（多线程模式可能需要累积数据）
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

// 从OpenH264官方控制台程序中提取的关键函数
int32_t H264Decoder::readPicture(uint8_t* pBuf, const int32_t& iFileSize, const int32_t& bufPos, 
                                 uint8_t*& pSpsBuf, int32_t& sps_byte_count) {
    int32_t bytes_available = iFileSize - bufPos;
    if (bytes_available < 4) {
        return bytes_available;
    }
    uint8_t* ptr = pBuf + bufPos;
    int32_t read_bytes = 0;
    int32_t sps_count = 0;
    int32_t pps_count = 0;
    int32_t non_idr_pict_count = 0;
    int32_t idr_pict_count = 0;
    int32_t nal_deliminator = 0;
    pSpsBuf = nullptr;
    sps_byte_count = 0;
    
    while (read_bytes < bytes_available - 4) {
        bool has4ByteStartCode = ptr[0] == 0 && ptr[1] == 0 && ptr[2] == 0 && ptr[3] == 1;
        bool has3ByteStartCode = false;
        if (!has4ByteStartCode) {
            has3ByteStartCode = ptr[0] == 0 && ptr[1] == 0 && ptr[2] == 1;
        }
        if (has4ByteStartCode || has3ByteStartCode) {
            int32_t byteOffset = has4ByteStartCode ? 4 : 3;
            uint8_t nal_unit_type = has4ByteStartCode ? (ptr[4] & 0x1F) : (ptr[3] & 0x1F);
            if (nal_unit_type == 1) {
                if (++non_idr_pict_count >= 1 && idr_pict_count >= 1) {
                    return read_bytes;
                }
                if (non_idr_pict_count >= 2) {
                    return read_bytes;
                }
            } else if (nal_unit_type == 5) {
                if (++idr_pict_count >= 1 && non_idr_pict_count >= 1) {
                    return read_bytes;
                }
                if (idr_pict_count >= 2) {
                    return read_bytes;
                }
            } else if (nal_unit_type == 7) {
                if ((++sps_count >= 1) && (non_idr_pict_count >= 1 || idr_pict_count >= 1)) {
                    return read_bytes;
                }
                if (sps_count == 2) return read_bytes;
                // 记录SPS位置
                if (pSpsBuf == nullptr) {
                    pSpsBuf = ptr + byteOffset;
                    sps_byte_count = 0;
                    // 计算SPS大小（到下一个start code）
                    for (int i = byteOffset; i < bytes_available - 4; i++) {
                        if ((ptr[i] == 0 && ptr[i+1] == 0 && ptr[i+2] == 0 && ptr[i+3] == 1) ||
                            (ptr[i] == 0 && ptr[i+1] == 0 && ptr[i+2] == 1)) {
                            break;
                        }
                        sps_byte_count++;
                    }
                }
            } else if (nal_unit_type == 8) {
                if (++pps_count >= 1 && (non_idr_pict_count >= 1 || idr_pict_count >= 1)) return read_bytes;
            } else if (nal_unit_type == 9) {
                if (++nal_deliminator == 2) {
                    return read_bytes;
                }
            }
            if (read_bytes >= bytes_available - 4) {
                return bytes_available;
            }
            read_bytes += 4;
            ptr += 4;
        } else {
            ++ptr;
            ++read_bytes;
        }
    }
    return bytes_available;
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
        initialize(enable_multithreading_, num_threads_);
    }
}

void H264Decoder::destroy() {
    if (decoder_ != nullptr) {
        WelsDestroyDecoder(decoder_);
        decoder_ = nullptr;
    }

    freeFrameBuffer();
    buffer_pool_.clear();  // 清空缓冲池
    accumulated_data_.clear(); // 清空累积数据
    initialized_ = false;
    frame_width_ = 0;
    frame_height_ = 0;
    timestamp_ = 0;
    last_sps_byte_count_ = 0;
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
