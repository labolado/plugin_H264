// H.264 Texture Binding for Solar2D
// Integrates H.264 decoder with Solar2D's texture system

#include "CoronaLua.h"
#include "CoronaMacros.h"
#include "CoronaGraphics.h"
#include "CoronaLibrary.h"

#include "lua/H264TextureBinding.h"
#include "managers/H264Movie.h"
#include "utils/Common.h"

#include <memory>
#include <cstring>
#include <algorithm>
#include <cmath>

#include "AL/al.h"
#include "AL/alc.h"

using namespace plugin_h264;

// Number of audio buffers for streaming
#define NUM_BUFFERS 8

// H264MovieTexture wrapper for Solar2D texture integration
struct H264MovieTexture {
    std::unique_ptr<plugin_h264::H264Movie> decoder;
    plugin_h264::VideoFrame current_video_frame;
    plugin_h264::AudioFrame current_audio_frame;

    // Converted RGBA frame data for Corona texture
    std::vector<uint8_t> rgba_data;

    // Playback state - 与plugin_movie字段名完全一致
    bool playing = false;
    bool stopped = false;
    bool audiostarted = false;
    bool audiocompleted = false;

    // Timing - 与plugin_movie字段名完全一致
    unsigned int elapsed = 0;
    unsigned int framems = 0;

    // 音视频同步相关
    double playback_start_time = 0.0;     // 播放开始的系统时间
    double last_audio_timestamp = 0.0;    // 最后一个音频帧的时间戳
    double last_video_timestamp = 0.0;    // 最后一个视频帧的时间戳
    double sync_offset = 0.0;             // 音视频同步偏移量
    double last_sync_report_time = 0.0;   // 上次同步报告的时间

    // Audio (OpenAL integration) - 与plugin_movie字段名完全一致
    ALuint source = 0;
    ALenum audioformat = 0;
    ALuint buffers[NUM_BUFFERS];

    // Default empty pixel data
    unsigned char empty[4] = {0, 0, 0, 0}; // Transparent black RGBA
};

// YUV to RGBA conversion function
void convertYUVtoRGBA(const plugin_h264::VideoFrame& yuv, std::vector<uint8_t>& rgba) {
    if (!yuv.isValid()) {
        PLUGIN_H264_LOG( ("Invalid VideoFrame: y_plane=%p, u_plane=%p, v_plane=%p, size=%dx%d\n",
               yuv.y_plane, yuv.u_plane, yuv.v_plane, yuv.width, yuv.height) );
        return;
    }

    int width = yuv.width;
    int height = yuv.height;
    PLUGIN_H264_LOG( ("Converting YUV frame: %dx%d, y_stride=%d, uv_stride=%d\n",
           width, height, yuv.y_stride, yuv.uv_stride) );

    rgba.resize(width * height * 4);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Y平面索引
            int y_index = y * yuv.y_stride + x;

            // UV平面索引 - YUV420格式中UV是2x2采样
            int uv_y = y / 2;
            int uv_x = x / 2;
            int uv_index = uv_y * yuv.uv_stride + uv_x;

            // 边界检查
            if (y_index < 0 || uv_index < 0) continue;

            // 获取YUV值
            int Y = yuv.y_plane[y_index];
            int U = yuv.u_plane[uv_index] - 128;
            int V = yuv.v_plane[uv_index] - 128;

            // BT.601标准YUV到RGB转换（更精确的系数）
            int R = Y + (1.402f * V);
            int G = Y - (0.344136f * U) - (0.714136f * V);
            int B = Y + (1.772f * U);

            // 限制到0-255范围
            R = std::max(0, std::min(255, R));
            G = std::max(0, std::min(255, G));
            B = std::max(0, std::min(255, B));

            // RGBA输出索引
            int rgba_index = (y * width + x) * 4;

            // 根据Solar2D的期望格式设置RGBA（可能需要BGRA顺序）
            rgba[rgba_index + 0] = static_cast<uint8_t>(R);  // Red
            rgba[rgba_index + 1] = static_cast<uint8_t>(G);  // Green
            rgba[rgba_index + 2] = static_cast<uint8_t>(B);  // Blue
            rgba[rgba_index + 3] = 255;                      // Alpha
        }
    }

    PLUGIN_H264_LOG( ("YUV to RGBA conversion completed for %dx%d frame\n", width, height) );
}

// Audio streaming functions - 匹配plugin_movie逻辑
bool startAudioStream(H264MovieTexture *movie) {
    if (!movie->current_audio_frame.isValid()) {
        PLUGIN_H264_LOG( ("startAudioStream failed: invalid audio frame\n") );
        return false;
    }

    PLUGIN_H264_LOG( ("Starting audio stream: %d channels, %d samples, %d Hz\n",
           movie->current_audio_frame.channels,
           (int)movie->current_audio_frame.samples.size(),
           movie->current_audio_frame.sample_rate) );

    // 确保音频缓冲区存在（可能在 stopAudioStream 中被删除了）
    ALboolean buffers_valid = alIsBuffer(movie->buffers[0]);
    if (!buffers_valid) {
        PLUGIN_H264_LOG( ("Recreating audio buffers after stop/seek\n") );

        // 先清理任何残留的错误状态
        alGetError(); // 清除之前的错误

        // 确保 source 处于正确状态
        alSourceStop(movie->source);
        alSourceRewind(movie->source);
        alSourcei(movie->source, AL_BUFFER, 0);

        // 初始化缓冲区 ID 数组
        for (int i = 0; i < NUM_BUFFERS; i++) {
            movie->buffers[i] = 0;
        }

        alGenBuffers(NUM_BUFFERS, movie->buffers);
        ALenum error = alGetError();
        if (error != AL_NO_ERROR) {
            PLUGIN_H264_LOG( ("Failed to generate audio buffers: %d\n", error) );

            return false;
        }

        PLUGIN_H264_LOG( ("Successfully created %d audio buffers\n", NUM_BUFFERS) );
    }

    ALsizei i;
    for(i = 0; i < NUM_BUFFERS; i++) {
        ALsizei size = movie->current_audio_frame.samples.size() * sizeof(int16_t);

        // 详细检查参数
        PLUGIN_H264_LOG( ("Buffer %d: audioformat=%d, size=%d, sample_rate=%d, samples_ptr=%p\n",
               i, movie->audioformat, size, movie->current_audio_frame.sample_rate,
               movie->current_audio_frame.samples.data()) );

        // 检查参数有效性
        if (size <= 0) {
            PLUGIN_H264_LOG( ("Invalid buffer size: %d\n", size) );
            return false;
        }

        if (movie->current_audio_frame.sample_rate <= 0) {
            PLUGIN_H264_LOG( ("Invalid sample rate: %d\n", movie->current_audio_frame.sample_rate) );
            return false;
        }

        if (movie->current_audio_frame.samples.empty()) {
            PLUGIN_H264_LOG( ("Empty audio samples\n") );
            return false;
        }

        alBufferData(movie->buffers[i], movie->audioformat,
                    movie->current_audio_frame.samples.data(), size,
                    movie->current_audio_frame.sample_rate);

        // 检查 OpenAL 错误
        ALenum error = alGetError();
        if (error != AL_NO_ERROR) {
            PLUGIN_H264_LOG( ("OpenAL buffer error %d at buffer %d (format=%d, size=%d, rate=%d)\n",
                   error, i, movie->audioformat, size, movie->current_audio_frame.sample_rate) );
            return false;
        }

        // 获取下一个音频帧（使用分离的音频解码）
        if (!movie->decoder->hasNewAudioFrame()) {
            movie->decoder->decodeNextAudioFrame();
        }

        if(movie->decoder->hasNewAudioFrame()) {
            movie->current_audio_frame = movie->decoder->getCurrentAudioFrame();
        } else {
            PLUGIN_H264_LOG( ("Only %d audio buffers filled (expected %d)\n", i+1, NUM_BUFFERS) );
            break;
        }
    }

    alSourceQueueBuffers(movie->source, i, movie->buffers);
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        PLUGIN_H264_LOG( ("OpenAL queue buffers error: %d\n", error) );
        return false;
    }

    alSourcePlay(movie->source);
    error = alGetError();
    if (error != AL_NO_ERROR) {
        PLUGIN_H264_LOG( ("OpenAL source play error: %d\n", error) );
        return false;
    }

    PLUGIN_H264_LOG( ("Audio stream started successfully with %d buffers\n", i) );
    return true;
}

void stopAudioStream(H264MovieTexture *movie) {
    alSourceStop(movie->source);
    alSourceRewind(movie->source);
    alSourcei(movie->source, AL_BUFFER, 0);
    alDeleteBuffers(NUM_BUFFERS, movie->buffers);
}

// Texture callback implementations
static unsigned int GetWidth(void *context) {
    H264MovieTexture *movie = (H264MovieTexture*)context;
    return movie->current_video_frame.width > 0 ? movie->current_video_frame.width : 1;
}

static unsigned int GetHeight(void *context) {
    H264MovieTexture *movie = (H264MovieTexture*)context;
    return movie->current_video_frame.height > 0 ? movie->current_video_frame.height : 1;
}

static const void* GetImage(void *context) {
    H264MovieTexture *movie = (H264MovieTexture*)context;

    PLUGIN_H264_LOG( ("GetImage called: playing=%s, frame_valid=%s\n",
           movie->playing ? "true" : "false",
           movie->current_video_frame.isValid() ? "true" : "false") );

    if (movie->current_video_frame.isValid()) {
        // 总是转换YUV到RGBA，确保获得最新的帧数据
        convertYUVtoRGBA(movie->current_video_frame, movie->rgba_data);

        PLUGIN_H264_LOG( ("GetImage: Converted YUV to RGBA, size=%zu bytes\n", movie->rgba_data.size()) );

        // 调试：检查RGBA数据的前几个像素（减少输出频率）
        static int debug_counter = 0;
        if (movie->rgba_data.size() >= 16 && (debug_counter++ % 30 == 0)) {
            PLUGIN_H264_LOG( ("RGBA data first 4 pixels: ") );
            for (int i = 0; i < 16; i += 4) {
                PLUGIN_H264_LOG( ("(%d,%d,%d,%d) ",
                       movie->rgba_data[i], movie->rgba_data[i+1],
                       movie->rgba_data[i+2], movie->rgba_data[i+3]) );
            }
            PLUGIN_H264_LOG( ("\n") );
        }

        return movie->rgba_data.data();
    } else {
        PLUGIN_H264_LOG( ("GetImage: Invalid video frame, returning empty (black) data\n") );
        return movie->empty;
    }
}

// CRITICAL: onGetField callback - 动态提供方法，与plugin_movie完全一致
static int GetField(lua_State *L, const char *field, void *context) {
    int result = 0;

    if(strcmp(field, "update") == 0) {
        result = PushCachedFunction(L, update);
    }
    else if(strcmp(field, "play") == 0) {
        result = PushCachedFunction(L, play);
    }
    else if(strcmp(field, "pause") == 0) {
        result = PushCachedFunction(L, pause);
    }
    else if(strcmp(field, "stop") == 0) {
        result = PushCachedFunction(L, stop);
    }
    else if(strcmp(field, "replay") == 0) {
        result = PushCachedFunction(L, replay);
    }
    // seek is not correctly implemented
    // else if(strcmp(field, "seek") == 0) {
    //     result = PushCachedFunction(L, seek);
    // }
    else if(strcmp(field, "invalidate") == 0) {
        result = PushCachedFunction(L, invalidate);
    }
    else if(strcmp(field, "isActive") == 0)
        result = isActive(L, context);
    else if(strcmp(field, "isPlaying") == 0)
        result = isPlaying(L, context);
    else if(strcmp(field, "currentTime") == 0)
        result = currentTime(L, context);

    return result;
}

static CoronaExternalBitmapFormat GetFormat(void *context) {
    return kExternalBitmapFormat_RGBA;
}

// Core texture creation function
int newMovieTexture(lua_State *L) {
    H264MovieTexture *movie = new H264MovieTexture;

    const char *path = lua_tostring(L, 1);

    if(!path) {
        delete movie;
        lua_pushnil(L);
        return 1;
    }

    // Create H.264 decoder instance
    movie->decoder = std::make_unique<plugin_h264::H264Movie>();

    // Load video file
    if(!movie->decoder->loadFromFile(path)) {
        delete movie;
        lua_pushnil(L);
        return 1;
    }

    // 解码第一帧以便立即显示
    bool result = movie->decoder->decodeNextFrame();
    PLUGIN_H264_LOG( ("Attempting to decode first frame: %s...\n", result ? "true" : "false") );

    if(movie->decoder->hasNewVideoFrame()) {
        movie->current_video_frame = movie->decoder->getCurrentVideoFrame();
        PLUGIN_H264_LOG( ("First video frame decoded successfully: %dx%d, timestamp=%.3fs\n",
               movie->current_video_frame.width, movie->current_video_frame.height,
               movie->current_video_frame.timestamp) );
    } else {
        PLUGIN_H264_LOG( ("WARNING: No video frame available after decoding!\n") );
    }

    if(movie->decoder->hasNewAudioFrame()) {
        movie->current_audio_frame = movie->decoder->getCurrentAudioFrame();
        PLUGIN_H264_LOG( ("First audio frame decoded: %d channels, %d samples\n",
               movie->current_audio_frame.channels, (int)movie->current_audio_frame.samples.size()) );
    } else {
        PLUGIN_H264_LOG( ("No audio frame (this is normal for video-only files)\n") );
    }

    // Setup audio source
    movie->source = (ALuint)lua_tonumber(L, 2);
    alSourceRewind(movie->source);
    alSourcei(movie->source, AL_BUFFER, 0);
    alGenBuffers(NUM_BUFFERS, movie->buffers);

    // Setup Solar2D texture callbacks - 关键：添加onGetField和getFormat
    CoronaExternalTextureCallbacks callbacks = {};
    callbacks.size = sizeof(CoronaExternalTextureCallbacks);
    callbacks.getWidth = GetWidth;
    callbacks.getHeight = GetHeight;
    callbacks.onRequestBitmap = GetImage;
    callbacks.getFormat = GetFormat;        // 提供RGBA格式信息
    callbacks.onGetField = GetField;        // 关键：动态提供methods和properties
    callbacks.onFinalize = [](void* context) {
        H264MovieTexture *movie = (H264MovieTexture*)context;

        // 完整的资源清理 - 匹配plugin_movie的Dispose逻辑
        if(!movie->stopped) {
            movie->stopped = true;
            movie->playing = false;

            // 停止音频流并清理OpenAL资源
            if(movie->audiostarted) {
                stopAudioStream(movie);
            }

            // 停止decoder
            if(movie->decoder) {
                movie->decoder->stop();
            }
        }

        // 清理动态分配的资源
        movie->rgba_data.clear();
        movie->rgba_data.shrink_to_fit();

        delete movie;
    };

    return CoronaExternalPushTexture(L, &callbacks, movie);
}

// 实现所有texture方法 - 与plugin_movie逻辑完全一致
static int update(lua_State *L) {
    H264MovieTexture *movie = (H264MovieTexture*)CoronaExternalGetUserData(L, 1);

    PLUGIN_H264_LOG( ("Update called - playing: %s, decoder: %s, stopped: %s\n",
           movie->playing ? "true" : "false",
           movie->decoder ? "exists" : "null",
           movie->stopped ? "true" : "false") );

    if(movie->playing && movie->decoder) {
// MAINLOOP:
        unsigned int delta = luaL_checkinteger(L, 2);

        PLUGIN_H264_LOG( ("Update main loop - delta: %u, current_video_frame.isValid(): %s\n",
               delta, movie->current_video_frame.isValid() ? "true" : "false") );

        // 独立解码视频帧
        if (!movie->current_video_frame.isValid()) {
            if (!movie->decoder->hasNewVideoFrame()) {
                movie->decoder->decodeNextVideoFrame();
            }
            if (movie->decoder->hasNewVideoFrame()) {
                movie->current_video_frame = movie->decoder->getCurrentVideoFrame();
            }
        }

        // 独立解码音频帧（仅当文件包含音频时）
        if (!movie->current_audio_frame.isValid() && movie->decoder->hasAudioTrack()) {
            if (!movie->decoder->hasNewAudioFrame()) {
                movie->decoder->decodeNextAudioFrame();
            }
            if (movie->decoder->hasNewAudioFrame()) {
                movie->current_audio_frame = movie->decoder->getCurrentAudioFrame();
            }
        }

        if(delta > 0 && (movie->current_audio_frame.isValid() || movie->current_video_frame.isValid())) {
            unsigned int currentTime = movie->elapsed + delta;

            // 音频处理 - 改进的音频播放控制，只有当文件包含音频时才处理
            if(movie->current_audio_frame.isValid() && movie->decoder->hasAudioTrack()) {
                if(!movie->audioformat) {
                    movie->audioformat = (movie->current_audio_frame.channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
                }

                if(!movie->audiostarted) {
                    movie->audiostarted = startAudioStream(movie);
                    if (movie->audiostarted && movie->playback_start_time == 0.0) {
                        // 记录播放开始时间
                        movie->playback_start_time = currentTime / 1000.0;
                        PLUGIN_H264_LOG( ("Audio playback started at time %.3fs\n", movie->playback_start_time) );
                    } else if (!movie->audiostarted) {
                        // 音频启动失败，记录错误并继续视频播放
                        PLUGIN_H264_LOG( ("Audio failed to start, continuing with video-only playback\n") );
                        // 设置音频完成标志，避免继续尝试启动音频
                        movie->audiocompleted = true;
                        // 如果还没有设置播放时间基准，现在设置
                        if (movie->playback_start_time == 0.0) {
                            movie->playback_start_time = currentTime / 1000.0;
                            PLUGIN_H264_LOG( ("Video-only playback started at time %.3fs\n", movie->playback_start_time) );
                        }
                    }
                }

                ALint state, processed;
                alGetSourcei(movie->source, AL_SOURCE_STATE, &state);
                alGetSourcei(movie->source, AL_BUFFERS_PROCESSED, &processed);

                // 计算期望的播放时间（基于播放开始时间）
                double expected_time = (currentTime / 1000.0) - movie->playback_start_time;
                double audio_timestamp = movie->current_audio_frame.timestamp;

                // 更新音频时间戳记录
                movie->last_audio_timestamp = audio_timestamp;

                // 计算音视频偏移量（音频时间戳 - 期望时间）
                double audio_offset = audio_timestamp - expected_time;

                while(processed > 0) {
                    ALuint buffID;
                    alSourceUnqueueBuffers(movie->source, 1, &buffID);
                    processed--;

                    ALsizei size = movie->current_audio_frame.samples.size() * sizeof(int16_t);
                    alBufferData(buffID, movie->audioformat,
                                movie->current_audio_frame.samples.data(), size,
                                movie->current_audio_frame.sample_rate);

                    alSourceQueueBuffers(movie->source, 1, &buffID);

                    // 获取下一个音频帧（使用分离的音频解码）
                    if (!movie->decoder->hasNewAudioFrame()) {
                        movie->decoder->decodeNextAudioFrame();
                    }

                    if(movie->decoder->hasNewAudioFrame()) {
                        movie->current_audio_frame = movie->decoder->getCurrentAudioFrame();
                        movie->last_audio_timestamp = movie->current_audio_frame.timestamp;
                    } else {
                        movie->current_audio_frame = AudioFrame(); // 清空
                        break;
                    }
                }

                // 打印同步信息（仅当差距较大时）
                if (fabs(audio_offset) > 0.1) {
                    PLUGIN_H264_LOG( ("Audio sync: expected=%.3fs, audio=%.3fs, offset=%.3fs\n",
                           expected_time, audio_timestamp, audio_offset) );
                }

                if(state == AL_STOPPED) {
                    if(movie->decoder->hasNewAudioFrame()) {
                        alSourcePlay(movie->source);
                    } else {
                        movie->audiocompleted = true;
                    }
                }
            }

            // 视频处理 - 基于播放时间基准的同步控制
            if(movie->current_video_frame.isValid()) {
                double currentTimeSeconds = currentTime / 1000.0;
                double frameTime = movie->current_video_frame.timestamp;

                // 更新视频时间戳记录
                movie->last_video_timestamp = frameTime;

                // 如果播放已开始，计算期望的播放时间
                double expected_time = 0.0;
                double video_offset = 0.0;
                bool should_advance_frame = false;

                if (movie->playback_start_time > 0.0) {
                    expected_time = currentTimeSeconds - movie->playback_start_time;
                    video_offset = frameTime - expected_time;

                    // 视频等待音频：基于音频时间戳来控制视频进度
                    if (movie->last_audio_timestamp > 0.0) {
                        // 只有当视频落后于音频时才推进视频帧
                        if (frameTime < movie->last_audio_timestamp - 0.040) { // 视频滞后于音频40ms以上
                            should_advance_frame = true;
                        }
                        // 如果视频超前于音频，则等待（不推进）
                        // 这样确保视频不会超前于音频播放
                    } else {
                        // 如果还没有音频时间戳，使用期望时间
                        if (video_offset < -0.050) {
                            should_advance_frame = true;
                        }
                    }
                } else {
                    // 如果还没开始播放，初始化播放时间基准
                    if (movie->playback_start_time == 0.0) {
                        movie->playback_start_time = currentTimeSeconds;
                        PLUGIN_H264_LOG( ("Video playback started at time %.3fs (first frame: %.3fs)\n",
                               movie->playback_start_time, frameTime) );
                        // 立即推进到下一帧，因为当前帧已经显示
                        should_advance_frame = true;
                    } else {
                        // 使用传统的时间戳比较
                        if (currentTimeSeconds > frameTime + 0.040) {
                            should_advance_frame = true;
                        }
                    }
                }

                // 获取视频总时长用于进度显示
                double totalDuration = movie->decoder->getDuration();
                double progress = totalDuration > 0 ? (frameTime / totalDuration) * 100.0 : 0.0;

                // 打印同步信息（显示视频相对于音频的状态）
                if (movie->playback_start_time > 0.0 && movie->last_audio_timestamp > 0.0) {
                    double av_diff = frameTime - movie->last_audio_timestamp;
                    if (fabs(av_diff) > 0.05 || should_advance_frame) {
                        PLUGIN_H264_LOG( ("Video waiting for audio: video=%.3fs, audio=%.3fs, V-A=%.3fs, advance=%s\n",
                               frameTime, movie->last_audio_timestamp, av_diff,
                               should_advance_frame ? "YES" : "NO") );
                    }
                } else if (movie->playback_start_time > 0.0 && fabs(video_offset) > 0.1) {
                    PLUGIN_H264_LOG( ("Video sync (no audio ref): expected=%.3fs, video=%.3fs, offset=%.3fs\n",
                           expected_time, frameTime, video_offset) );
                }

                if (should_advance_frame) {
                    // 尝试解码下一视频帧（使用分离的视频解码）
                    if (!movie->decoder->hasNewVideoFrame()) {
                        movie->decoder->decodeNextVideoFrame();
                    }

                    if(movie->decoder->hasNewVideoFrame()) {
                        auto next_frame = movie->decoder->getCurrentVideoFrame();
                        if(next_frame.isValid()) {
                            movie->current_video_frame = next_frame;
                            movie->last_video_timestamp = movie->current_video_frame.timestamp;
                            // 清空RGBA缓存强制重新转换
                            movie->rgba_data.clear();

                            if (movie->playback_start_time > 0.0) {
                                if (movie->last_audio_timestamp > 0.0) {
                                    PLUGIN_H264_LOG( ("Video frame advanced: %.3fs -> %.3fs (audio at: %.3fs)\n",
                                           frameTime, movie->current_video_frame.timestamp, movie->last_audio_timestamp) );
                                } else {
                                    PLUGIN_H264_LOG( ("Video frame advanced: %.3fs -> %.3fs (expected: %.3fs)\n",
                                           frameTime, movie->current_video_frame.timestamp, expected_time) );
                                }
                            }
                        }
                    }
                }
            }

            // 周期性同步状态报告（每2秒报告一次）
            if (movie->playback_start_time > 0.0 && movie->decoder->hasAudioTrack()) {
                double current_time_sec = currentTime / 1000.0;
                if (current_time_sec - movie->last_sync_report_time > 2.0) {
                    double av_sync_diff = movie->last_audio_timestamp - movie->last_video_timestamp;
                    PLUGIN_H264_LOG( ("=== SYNC STATUS ===\n") );
                    PLUGIN_H264_LOG( ("Audio: %.3fs, Video: %.3fs, A-V diff: %.3fs\n",
                           movie->last_audio_timestamp, movie->last_video_timestamp, av_sync_diff) );
                    PLUGIN_H264_LOG( ("Playback time: %.3fs\n", current_time_sec - movie->playback_start_time) );
                    PLUGIN_H264_LOG( ("==================\n") );
                    movie->last_sync_report_time = current_time_sec;
                }
            }

            movie->elapsed = currentTime;
        }
    }
    // 处理解码完成后剩余音频
    else if(movie->audiostarted && !movie->audiocompleted) {
        // 简化的完成检查
        if(!movie->decoder || (!movie->decoder->hasNewAudioFrame() && !movie->decoder->hasNewVideoFrame())) {
            movie->audiocompleted = true;
        }
    }

    return 0;
}

static int play(lua_State *L) {
    H264MovieTexture *movie = (H264MovieTexture*)CoronaExternalGetUserData(L, 1);

    if(!movie->playing) {
        movie->playing = true;
        if(movie->audiostarted && !movie->audiocompleted) {
            alSourcePlay(movie->source);
        }
    }

    return 0;
}

static int pause(lua_State *L) {
    H264MovieTexture *movie = (H264MovieTexture*)CoronaExternalGetUserData(L, 1);

    if(movie->playing) {
        movie->playing = false;
        if(movie->audiostarted && !movie->audiocompleted) {
            alSourcePause(movie->source);
        }
    }

    return 0;
}

static int stop(lua_State *L) {
    H264MovieTexture *movie = (H264MovieTexture*)CoronaExternalGetUserData(L, 1);

    movie->stopped = true;
    movie->playing = false;

    // 使用统一的stopAudioStream函数
    if(movie->audiostarted) {
        stopAudioStream(movie);
        movie->audiostarted = false;
        movie->audiocompleted = true;
    }

    // 停止decoder
    if(movie->decoder) {
        movie->decoder->stop();
    }

    return 0;
}

static int replay(lua_State *L) {
    H264MovieTexture *movie = (H264MovieTexture*)CoronaExternalGetUserData(L, 1);

    PLUGIN_H264_LOG( ("Lua replay called\n") );

    if (!movie->decoder) {
        PLUGIN_H264_LOG( ("ERROR: No decoder available for replay\n") );
        lua_pushboolean(L, false);
        return 1;
    }

    // 停止音频
    if(movie->audiostarted) {
        PLUGIN_H264_LOG( ("Stopping audio stream for replay\n") );
        stopAudioStream(movie);
        movie->audiostarted = false;
        movie->audiocompleted = false;
    }

    // 重置播放状态
    movie->stopped = false;
    movie->playing = false;
    movie->elapsed = 0;
    movie->playback_start_time = 0.0;
    movie->last_audio_timestamp = 0.0;
    movie->last_video_timestamp = 0.0;
    movie->sync_offset = 0.0;
    movie->last_sync_report_time = 0.0;

    // 清空当前帧数据
    movie->current_video_frame = plugin_h264::VideoFrame();
    movie->current_audio_frame = plugin_h264::AudioFrame();

    PLUGIN_H264_LOG( ("Reset H264MovieTexture state for replay\n") );

    // 调用解码器的 replay 方法
    bool success = movie->decoder->replay();

    PLUGIN_H264_LOG( ("H264Movie replay result: %s\n", success ? "true" : "false") );

    // 如果 replay 成功，设置播放状态为 true
    if (success) {
        movie->playing = true;
        PLUGIN_H264_LOG( ("Set movie->playing = true after successful replay\n") );
    }

    if (success) {
        // 解码第一帧以便立即显示（就像初始化时一样）
        // 可能需要多次尝试，因为第一次可能只是发送SPS/PPS
        int decode_attempts = 0;
        const int max_attempts = 3;

        while (decode_attempts < max_attempts && !movie->decoder->hasNewVideoFrame()) {
            bool decode_result = movie->decoder->decodeNextFrame();
            decode_attempts++;

            PLUGIN_H264_LOG( ("Decode attempt %d after replay: %s\n", decode_attempts, decode_result ? "true" : "false") );

            if (movie->decoder->hasNewVideoFrame()) {
                break;
            }
        }

        if(movie->decoder->hasNewVideoFrame()) {
            movie->current_video_frame = movie->decoder->getCurrentVideoFrame();
            PLUGIN_H264_LOG( ("First video frame decoded after replay (attempt %d): %dx%d, timestamp=%.3fs\n",
                   decode_attempts, movie->current_video_frame.width, movie->current_video_frame.height,
                   movie->current_video_frame.timestamp) );
        } else {
            PLUGIN_H264_LOG( ("WARNING: No video frame available after %d decode attempts!\n", decode_attempts) );
        }

        if(movie->decoder->hasNewAudioFrame()) {
            movie->current_audio_frame = movie->decoder->getCurrentAudioFrame();
            PLUGIN_H264_LOG( ("First audio frame decoded after replay: %d channels, %d samples\n",
                   movie->current_audio_frame.channels, (int)movie->current_audio_frame.samples.size()) );
        }
    }

    lua_pushboolean(L, success);
    return 1;
}

static int isActive(lua_State *L, void *context) {
    H264MovieTexture *movie = (H264MovieTexture*)context;

    // 检查是否已停止
    if (movie->stopped || !movie->decoder) {
        lua_pushboolean(L, false);
        return 1;
    }

    // 检查播放是否完成
    if (movie->decoder->isPlaybackFinished()) {
        lua_pushboolean(L, false);
        return 1;
    }

    // 如果没有完成，说明还在播放中
    lua_pushboolean(L, true);
    return 1;
}

static int isPlaying(lua_State *L, void *context) {
    H264MovieTexture *movie = (H264MovieTexture*)context;

    lua_pushboolean(L, movie->playing);
    return 1;
}

static int invalidate(lua_State *L) {
    // Solar2D 应该会自动检测纹理数据变化
    // 这个方法主要是为了兼容 plugin_movie 的 API
    return 0;
}

static int currentTime(lua_State *L, void *context) {
    H264MovieTexture *movie = (H264MovieTexture*)context;

    lua_pushnumber(L, movie->elapsed * 0.001);
    return 1;
}

// seek is not correctly implemented
static int seek(lua_State *L) {
    H264MovieTexture *movie = (H264MovieTexture*)CoronaExternalGetUserData(L, 1);

    double timestamp = luaL_checknumber(L, 2);

    PLUGIN_H264_LOG( ("Seek to timestamp: %.3fs\n", timestamp) );

    if (!movie->decoder) {
        lua_pushboolean(L, false);
        return 1;
    }

    // 调用解码器 seek
    bool seek_success = movie->decoder->seekTo(timestamp);
    if (!seek_success) {
        PLUGIN_H264_LOG( ("Decoder seek failed\n") );
        lua_pushboolean(L, false);
        return 1;
    }

    // 更新播放时间位置
    movie->elapsed = (unsigned int)(timestamp * 1000.0);

    // 清空当前帧，强制获取新位置的帧
    movie->current_video_frame = plugin_h264::VideoFrame();
    movie->current_audio_frame = plugin_h264::AudioFrame();

    // 尝试解码新位置的第一帧，处理 B-frame 参考帧丢失问题
    bool decode_success = false;
    int decode_failures = 0;
    const int max_decode_attempts = 10;

    for (int attempt = 0; attempt < max_decode_attempts && !decode_success; attempt++) {
        bool frame_decoded = movie->decoder->decodeNextFrame();

        if (frame_decoded && movie->decoder->hasNewVideoFrame()) {
            auto frame = movie->decoder->getCurrentVideoFrame();
            if (frame.isValid() && frame.width > 0 && frame.height > 0) {
                movie->current_video_frame = frame;
                // 强制更新纹理
                movie->rgba_data.clear();
                decode_success = true;
                PLUGIN_H264_LOG( ("Decoded valid frame: %.3fs (attempt %d)\n", frame.timestamp, attempt + 1) );
                break;
            }
        }

        if (!frame_decoded) {
            decode_failures++;
            // 如果连续解码失败，可能遇到了 B-frame 参考帧问题
            if (decode_failures >= 3) {
                PLUGIN_H264_LOG( ("Multiple decode failures (%d), seeking to nearby keyframe\n", decode_failures) );

                // 向前寻找最近的关键帧位置（通常在GOP开始）
                double keyframe_seek = timestamp - 1.0; // 向前1秒寻找关键帧
                if (keyframe_seek < 0) keyframe_seek = 0;

                if (movie->decoder->seekTo(keyframe_seek)) {
                    PLUGIN_H264_LOG( ("Seeked to keyframe position: %.3fs\n", keyframe_seek) );
                    // 更新 elapsed 到实际 seek 位置
                    movie->elapsed = (unsigned int)(keyframe_seek * 1000.0);
                    decode_failures = 0; // 重置失败计数
                } else {
                    PLUGIN_H264_LOG( ("Keyframe seek failed, continuing with current position\n") );
                    break;
                }
            }
        }
    }

    if (!decode_success) {
        PLUGIN_H264_LOG( ("Warning: Could not decode valid frame after %d attempts\n", max_decode_attempts) );
    }

    // 获取音频帧（如果有）
    if (movie->decoder->hasNewAudioFrame()) {
        movie->current_audio_frame = movie->decoder->getCurrentAudioFrame();
    }

    lua_pushboolean(L, true);
    return 1;
}

// Main plugin entry point
CORONA_EXPORT int luaopen_plugin_h264(lua_State *L) {
    lua_CFunction factory = Corona::Lua::Open<CoronaPluginLuaLoad_plugin_h264>;
    int result = CoronaLibraryNewWithFactory(L, factory, NULL, NULL);

    if(result) {
        const luaL_Reg kFunctions[] = {
            {"_newMovieTexture", newMovieTexture},
            {NULL, NULL}
        };

        luaL_register(L, NULL, kFunctions);
    }

    return result;
}
