// H.264 Texture Binding for Solar2D
// Integrates H.264 decoder with Solar2D's texture system

#include <CoronaLua.h>
#include <CoronaMacros.h>
// CoronaLibrary.h and CoronaGraphics.h not available in standalone build
// Using our local Corona compatibility layer instead
#include "../../include/lua/CoronaTypes.h"

#include "../../include/lua/H264TextureBinding.h"
#include "../../include/managers/H264Movie.h"  
#include "../../include/utils/Common.h"

#include <memory>
#include <cstring>
#include <algorithm>

#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

using namespace plugin_h264;

// Number of audio buffers for streaming
#define NUM_BUFFERS 8

// Forward declarations for texture callback methods
static int update(lua_State *L);
static int play(lua_State *L);
static int pause(lua_State *L);
static int stop(lua_State *L);
static int isActive(lua_State *L, void *context);
static int isPlaying(lua_State *L, void *context);
static int currentTime(lua_State *L, void *context);

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
    
    // Audio (OpenAL integration) - 与plugin_movie字段名完全一致
    ALuint source = 0;
    ALenum audioformat = 0;
    ALuint buffers[NUM_BUFFERS];
    
    // Default empty pixel data
    unsigned char empty[4] = {0, 0, 0, 255}; // Transparent black RGBA
};

// YUV to RGBA conversion function
void convertYUVtoRGBA(const plugin_h264::VideoFrame& yuv, std::vector<uint8_t>& rgba) {
    if (!yuv.isValid()) return;
    
    int width = yuv.width;
    int height = yuv.height;
    rgba.resize(width * height * 4);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int y_index = y * yuv.y_stride + x;
            int uv_index = (y / 2) * yuv.uv_stride + (x / 2);
            
            int Y = yuv.y_plane[y_index];
            int U = yuv.u_plane[uv_index] - 128;
            int V = yuv.v_plane[uv_index] - 128;
            
            // YUV to RGB conversion
            int R = Y + (1.402 * V);
            int G = Y - (0.344 * U) - (0.714 * V);
            int B = Y + (1.772 * U);
            
            // Clamp values
            R = std::max(0, std::min(255, R));
            G = std::max(0, std::min(255, G));
            B = std::max(0, std::min(255, B));
            
            int rgba_index = (y * width + x) * 4;
            rgba[rgba_index + 0] = R;
            rgba[rgba_index + 1] = G;
            rgba[rgba_index + 2] = B;
            rgba[rgba_index + 3] = 255; // Alpha
        }
    }
}

// Audio streaming functions - 匹配plugin_movie逻辑
bool startAudioStream(H264MovieTexture *movie) {
    if (!movie->current_audio_frame.isValid()) return false;
    
    ALsizei i;
    for(i = 0; i < NUM_BUFFERS; i++) {
        ALsizei size = movie->current_audio_frame.samples.size() * sizeof(int16_t);
        alBufferData(movie->buffers[i], movie->audioformat, 
                    movie->current_audio_frame.samples.data(), size, 
                    movie->current_audio_frame.sample_rate);

        // 获取下一个音频帧
        if(movie->decoder->hasNewAudioFrame()) {
            movie->current_audio_frame = movie->decoder->getCurrentAudioFrame();
        } else {
            break;
        }
    }
    
    alSourceQueueBuffers(movie->source, i, movie->buffers);
    alSourcePlay(movie->source);
    
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

static void GetImage(void *context, void *bitmap) {
    H264MovieTexture *movie = (H264MovieTexture*)context;
    
    if (movie->current_video_frame.isValid() && bitmap) {
        // Convert YUV to RGBA if we have a new frame
        if (movie->rgba_data.empty() || 
            movie->rgba_data.size() != movie->current_video_frame.width * movie->current_video_frame.height * 4) {
            convertYUVtoRGBA(movie->current_video_frame, movie->rgba_data);
        }
        // Copy RGBA data to the provided bitmap buffer
        memcpy(bitmap, movie->rgba_data.data(), movie->rgba_data.size());
    }
}

// CRITICAL: onGetField callback - 动态提供方法，与plugin_movie完全一致
static int GetField(lua_State *L, const char *field, void *context) {
    int result = 0;
    
    if(strcmp(field, "update") == 0) {
        lua_pushcfunction(L, update);
        result = 1;
    }
    else if(strcmp(field, "play") == 0) {
        lua_pushcfunction(L, play);
        result = 1;
    }
    else if(strcmp(field, "pause") == 0) {
        lua_pushcfunction(L, pause);
        result = 1;
    }
    else if(strcmp(field, "stop") == 0) {
        lua_pushcfunction(L, stop);
        result = 1;
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
    
    if(movie->playing && movie->decoder) {
MAINLOOP:
        unsigned int delta = luaL_checkinteger(L, 2);
        
        // 获取音频帧（如果没有）
        if(!movie->current_audio_frame.isValid()) {
            movie->current_audio_frame = movie->decoder->getCurrentAudioFrame();
        }
        
        // 获取视频帧（如果没有）
        if(!movie->current_video_frame.isValid()) {
            movie->current_video_frame = movie->decoder->getCurrentVideoFrame();
        }
        
        if(delta > 0 && (movie->current_audio_frame.isValid() || movie->current_video_frame.isValid())) {
            unsigned int currentTime = movie->elapsed + delta;
            
            // 音频处理 - 完整逻辑匹配plugin_movie
            if(movie->current_audio_frame.isValid()) {
                if(!movie->audioformat) {
                    movie->audioformat = (movie->current_audio_frame.channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
                }
                
                if(!movie->audiostarted) {
                    movie->audiostarted = startAudioStream(movie);
                }
                
                ALint state, processed;
                alGetSourcei(movie->source, AL_SOURCE_STATE, &state);
                alGetSourcei(movie->source, AL_BUFFERS_PROCESSED, &processed);
                
                while(processed > 0) {
                    ALuint buffID;
                    alSourceUnqueueBuffers(movie->source, 1, &buffID);
                    processed--;
                    
                    ALsizei size = movie->current_audio_frame.samples.size() * sizeof(int16_t);
                    alBufferData(buffID, movie->audioformat, 
                                movie->current_audio_frame.samples.data(), size, 
                                movie->current_audio_frame.sample_rate);
                    
                    alSourceQueueBuffers(movie->source, 1, &buffID);
                    
                    // 获取下一个音频帧
                    if(movie->decoder->hasNewAudioFrame()) {
                        movie->current_audio_frame = movie->decoder->getCurrentAudioFrame();
                    } else {
                        movie->current_audio_frame = AudioFrame(); // 清空
                        break;
                    }
                }
                
                if(state == AL_STOPPED) {
                    if(movie->decoder->hasNewAudioFrame()) {
                        alSourcePlay(movie->source);
                    } else {
                        movie->audiocompleted = true;
                    }
                }
            }
            
            // 视频处理 - 帧率控制逻辑
            if(movie->current_video_frame.isValid()) {
                if(movie->framems == 0) {
                    // 从decoder获取帧率，默认30fps
                    movie->framems = (unsigned int)(1000.0 / 30.0);
                }
                
                while(currentTime >= (movie->current_video_frame.timestamp + movie->framems)) {
                    if(movie->decoder->hasNewVideoFrame()) {
                        movie->current_video_frame = movie->decoder->getCurrentVideoFrame();
                        // 清空RGBA缓存强制重新转换
                        movie->rgba_data.clear();
                    } else {
                        break;
                    }
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

static int isActive(lua_State *L, void *context) {
    H264MovieTexture *movie = (H264MovieTexture*)context;
    
    bool active = (movie->audiostarted && !movie->audiocompleted) || 
                  (movie->decoder && !movie->stopped);
    lua_pushboolean(L, active);
    return 1;
}

static int isPlaying(lua_State *L, void *context) {
    H264MovieTexture *movie = (H264MovieTexture*)context;
    
    lua_pushboolean(L, movie->playing);
    return 1;
}

static int currentTime(lua_State *L, void *context) {
    H264MovieTexture *movie = (H264MovieTexture*)context;
    
    lua_pushnumber(L, movie->elapsed * 0.001);
    return 1;
}

// Main plugin entry point
extern "C" int luaopen_plugin_h264(lua_State *L) {
    // Register the main texture creation function
    const luaL_Reg kFunctions[] = {
        {"newMovieTexture", newMovieTexture},
        {"_newMovieTexture", newMovieTexture}, // Internal alias for Lua wrapper
        {NULL, NULL}
    };

    // Create the plugin table
    luaL_newlib(L, kFunctions);
    
    return 1;
}