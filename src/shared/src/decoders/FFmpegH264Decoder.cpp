#include "../../include/decoders/FFmpegH264Decoder.h"
#include <cstring>
#include <algorithm>

namespace plugin_h264 {

FFmpegH264Decoder::FFmpegH264Decoder()
    : codec_(nullptr)
    , codec_context_(nullptr)
    , av_frame_(nullptr)
    , av_packet_(nullptr)
    , initialized_(false)
    , frame_width_(0)
    , frame_height_(0)
    , thread_count_(4)
    , frame_buffer_size_(0)
    , buffer_pool_(3) {
}

FFmpegH264Decoder::~FFmpegH264Decoder() {
    destroy();
}

bool FFmpegH264Decoder::initialize() {
    if (initialized_) {
        return true;
    }

    // 查找H.264解码器
    codec_ = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec_) {
        setError(H264Error::DECODER_INIT_FAILED, "Failed to find H.264 decoder");
        return false;
    }

    // 创建解码器上下文
    codec_context_ = avcodec_alloc_context3(codec_);
    if (!codec_context_) {
        setError(H264Error::DECODER_INIT_FAILED, "Failed to allocate codec context");
        return false;
    }

    // 设置基本参数
    if (!setupFFmpegDecoder()) {
        destroy();
        return false;
    }

    // 分配AVFrame和AVPacket
    av_frame_ = av_frame_alloc();
    if (!av_frame_) {
        setError(H264Error::DECODER_INIT_FAILED, "Failed to allocate AVFrame");
        destroy();
        return false;
    }

    av_packet_ = av_packet_alloc();
    if (!av_packet_) {
        setError(H264Error::DECODER_INIT_FAILED, "Failed to allocate AVPacket");
        destroy();
        return false;
    }

    // 打开解码器
    int ret = avcodec_open2(codec_context_, codec_, nullptr);
    if (ret < 0) {
        char error_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, error_buf, sizeof(error_buf));
        setError(H264Error::DECODER_INIT_FAILED, 
                std::string("Failed to open codec: ") + error_buf);
        destroy();
        return false;
    }
    
    // 确认实际使用的线程数
    printf("[FFmpeg] Decoder opened - actual thread_count: %d, thread_type: %d\n", 
           codec_context_->thread_count, codec_context_->thread_type);

    initialized_ = true;
    clearError();
    return true;
}

bool FFmpegH264Decoder::setupFFmpegDecoder() {
    // 设置线程数（用于FFmpeg内部多线程，后续测试用）
    if (thread_count_ > 1) {
        codec_context_->thread_count = thread_count_;
        codec_context_->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;
        
        // 可以通过av_opt_set_int设置线程相关选项
        av_opt_set_int(codec_context_, "threads", thread_count_, 0);
        
        // 调试信息：确认多线程设置
        printf("[FFmpeg] Multi-threading enabled: %d threads (FRAME+SLICE)\n", thread_count_);
    } else {
        // 单线程模式
        codec_context_->thread_count = 1;
        codec_context_->thread_type = 0;
        printf("[FFmpeg] Single-thread mode\n");
    }

    // 设置其他解码参数
    codec_context_->pix_fmt = AV_PIX_FMT_YUV420P; // 期望的像素格式
    codec_context_->flags2 |= AV_CODEC_FLAG2_FAST; // 快速解码模式
    
    return true;
}

bool FFmpegH264Decoder::decode(const uint8_t* nal_data, size_t nal_size, VideoFrame& frame) {
    if (!initialized_ || !codec_context_) {
        setError(H264Error::DECODER_INIT_FAILED, "Decoder not initialized");
        return false;
    }

    if (nal_data == nullptr || nal_size == 0 || nal_size > INT_MAX) {
        setError(H264Error::INVALID_PARAM, "Invalid NAL data or size too large");
        return false;
    }

    // 安全地准备packet数据
    av_packet_unref(av_packet_);
    if (av_new_packet(av_packet_, static_cast<int>(nal_size)) < 0) {
        setError(H264Error::OUT_OF_MEMORY, "Failed to allocate packet");
        return false;
    }
    memcpy(av_packet_->data, nal_data, nal_size);

    // 发送packet到解码器
    int ret = avcodec_send_packet(codec_context_, av_packet_);
    if (ret < 0) {
        char error_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, error_buf, sizeof(error_buf));
        
        if (ret == AVERROR(EAGAIN)) {
            // 解码器需要先输出帧才能接受新输入，这不是错误
            return false;
        } else if (ret == AVERROR_EOF) {
            // 到达流末尾
            return false;
        }
        
        setError(H264Error::DECODE_FAILED, 
                std::string("Failed to send packet to decoder: ") + error_buf);
        return false;
    }

    // 接收解码后的帧
    ret = avcodec_receive_frame(codec_context_, av_frame_);
    if (ret < 0) {
        if (ret == AVERROR(EAGAIN)) {
            // 需要更多输入数据，这不是错误
            return false;
        } else if (ret == AVERROR_EOF) {
            // 解码器已刷新完毕
            return false;
        }
        
        char error_buf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, error_buf, sizeof(error_buf));
        setError(H264Error::DECODE_FAILED,
                std::string("Failed to receive frame from decoder: ") + error_buf);
        return false;
    }

    // 转换AVFrame到VideoFrame
    if (!convertAVFrameToVideoFrame(av_frame_, frame)) {
        return false;
    }

    // 更新解码器尺寸信息
    frame_width_ = av_frame_->width;
    frame_height_ = av_frame_->height;

    clearError();
    return true;
}

bool FFmpegH264Decoder::convertAVFrameToVideoFrame(const AVFrame* av_frame, VideoFrame& video_frame) {
    if (!av_frame || av_frame->format != AV_PIX_FMT_YUV420P) {
        setError(H264Error::DECODE_FAILED, "Unsupported pixel format or null frame");
        return false;
    }

    int width = av_frame->width;
    int height = av_frame->height;

    // 安全边界检查，防止整数溢出和过大帧
    const int MAX_FRAME_SIZE = 8192;  // 合理的最大帧尺寸
    if (width <= 0 || height <= 0 || width > MAX_FRAME_SIZE || height > MAX_FRAME_SIZE) {
        setError(H264Error::DECODE_FAILED, "Invalid or unsafe frame dimensions");
        return false;
    }
    
    // 检查整数溢出
    if (width > SIZE_MAX / height) {
        setError(H264Error::DECODE_FAILED, "Frame size would cause integer overflow");
        return false;
    }

    // 分配或重新分配帧缓冲区
    if (!allocateFrameBuffer(width, height)) {
        return false;
    }

    // 设置VideoFrame基本信息
    video_frame.width = width;
    video_frame.height = height;
    video_frame.y_stride = width;
    video_frame.uv_stride = width / 2;
    
    // 设置平面指针
    video_frame.y_plane = frame_buffer_.get();
    video_frame.u_plane = video_frame.y_plane + width * height;
    video_frame.v_plane = video_frame.u_plane + (width / 2) * (height / 2);

    // 安全复制Y平面数据
    uint8_t* dst_y = video_frame.y_plane;
    const uint8_t* src_y = av_frame->data[0];
    int src_stride_y = av_frame->linesize[0];
    
    // 边界检查
    if (!src_y || src_stride_y <= 0 || src_stride_y < width) {
        setError(H264Error::DECODE_FAILED, "Invalid Y plane data or stride");
        return false;
    }
    
    if (src_stride_y == width) {
        // 优化：stride匹配时使用单次memcpy，但要检查边界
        size_t copy_size = static_cast<size_t>(width) * height;
        memcpy(dst_y, src_y, copy_size);
    } else {
        // 逐行复制，每行都检查边界
        for (int i = 0; i < height; ++i) {
            memcpy(dst_y, src_y, static_cast<size_t>(width));
            src_y += src_stride_y;
            dst_y += width;
        }
    }

    // 安全复制U平面数据
    uint8_t* dst_u = video_frame.u_plane;
    const uint8_t* src_u = av_frame->data[1];
    int src_stride_u = av_frame->linesize[1];
    int uv_width = width / 2;
    int uv_height = height / 2;
    
    // 边界检查
    if (!src_u || src_stride_u <= 0 || src_stride_u < uv_width) {
        setError(H264Error::DECODE_FAILED, "Invalid U plane data or stride");
        return false;
    }
    
    if (src_stride_u == uv_width) {
        size_t copy_size = static_cast<size_t>(uv_width) * uv_height;
        memcpy(dst_u, src_u, copy_size);
    } else {
        for (int i = 0; i < uv_height; ++i) {
            memcpy(dst_u, src_u, static_cast<size_t>(uv_width));
            src_u += src_stride_u;
            dst_u += uv_width;
        }
    }

    // 安全复制V平面数据
    uint8_t* dst_v = video_frame.v_plane;
    const uint8_t* src_v = av_frame->data[2];
    int src_stride_v = av_frame->linesize[2];
    
    // 边界检查
    if (!src_v || src_stride_v <= 0 || src_stride_v < uv_width) {
        setError(H264Error::DECODE_FAILED, "Invalid V plane data or stride");
        return false;
    }
    
    if (src_stride_v == uv_width) {
        size_t copy_size = static_cast<size_t>(uv_width) * uv_height;
        memcpy(dst_v, src_v, copy_size);
    } else {
        for (int i = 0; i < uv_height; ++i) {
            memcpy(dst_v, src_v, static_cast<size_t>(uv_width));
            src_v += src_stride_v;
            dst_v += uv_width;
        }
    }

    return true;
}

bool FFmpegH264Decoder::allocateFrameBuffer(int width, int height) {
    // 使用安全的帧大小计算
    size_t total_size = calculateFrameSize(width, height);
    if (total_size == 0) {
        setError(H264Error::INVALID_PARAM, "Invalid frame size would cause overflow");
        return false;
    }

    if (frame_buffer_size_ < total_size) {
        // 安全释放旧缓冲区到池中
        freeFrameBuffer();
        
        try {
            // 从池中获取新缓冲区
            frame_buffer_ = buffer_pool_.acquire(total_size);
            frame_buffer_size_ = total_size;
        } catch (const std::bad_alloc&) {
            frame_buffer_size_ = 0;  // 确保状态一致
            setError(H264Error::OUT_OF_MEMORY,
                    "Failed to allocate frame buffer of size: " + std::to_string(total_size));
            return false;
        } catch (...) {
            frame_buffer_size_ = 0;
            setError(H264Error::OUT_OF_MEMORY, "Unknown error allocating frame buffer");
            return false;
        }
    }

    return true;
}

void FFmpegH264Decoder::freeFrameBuffer() {
    if (frame_buffer_) {
        buffer_pool_.release(std::move(frame_buffer_), frame_buffer_size_);
        frame_buffer_.reset();  // 确保指针被清空
    }
    frame_buffer_size_ = 0;
}

bool FFmpegH264Decoder::getDecoderInfo(int& width, int& height) const {
    if (!initialized_) {
        return false;
    }

    width = frame_width_;
    height = frame_height_;
    return frame_width_ > 0 && frame_height_ > 0;
}

void FFmpegH264Decoder::reset() {
    if (initialized_ && codec_context_) {
        // 刷新解码器
        avcodec_flush_buffers(codec_context_);
    }
    
    // 清理帧缓冲区
    if (av_frame_) {
        av_frame_unref(av_frame_);
    }
    if (av_packet_) {
        av_packet_unref(av_packet_);
    }
    
    clearError();
}

void FFmpegH264Decoder::destroy() {
    if (av_frame_) {
        av_frame_free(&av_frame_);
        av_frame_ = nullptr;
    }

    if (av_packet_) {
        av_packet_free(&av_packet_);
        av_packet_ = nullptr;
    }

    if (codec_context_) {
        avcodec_free_context(&codec_context_);
        codec_context_ = nullptr;
    }

    freeFrameBuffer();
    buffer_pool_.clear();
    
    codec_ = nullptr;
    initialized_ = false;
    frame_width_ = 0;
    frame_height_ = 0;
    clearError();
}

size_t FFmpegH264Decoder::getMemoryUsage() const {
    size_t base_usage = sizeof(*this);
    size_t buffer_usage = frame_buffer_size_;
    size_t decoder_usage = initialized_ ? 1024 * 1024 : 0; // 估计1MB
    
    return base_usage + buffer_usage + decoder_usage;
}

void FFmpegH264Decoder::setThreadCount(int thread_count) {
    thread_count_ = std::max(1, std::min(thread_count, 16)); // 限制1-16线程
    
    // 如果解码器已初始化，需要重新初始化以应用线程设置
    if (initialized_) {
        bool was_initialized = initialized_;
        destroy();
        if (was_initialized) {
            initialize();
        }
    }
}

size_t FFmpegH264Decoder::calculateFrameSize(int width, int height) const {
    if (width <= 0 || height <= 0) {
        return 0;
    }
    
    // 检查整数溢出 - Y平面
    if (width > SIZE_MAX / height) {
        return 0;
    }
    size_t y_size = static_cast<size_t>(width) * height;
    
    // UV平面大小计算
    size_t uv_width = width / 2;
    size_t uv_height = height / 2;
    
    // 检查UV平面溢出
    if (uv_width > 0 && uv_height > SIZE_MAX / uv_width) {
        return 0;
    }
    size_t uv_size = uv_width * uv_height;
    
    // 检查总大小溢出 (Y + U + V)
    if (y_size > SIZE_MAX - (uv_size * 2)) {
        return 0;
    }
    
    return y_size + uv_size * 2;
}

} // namespace plugin_h264