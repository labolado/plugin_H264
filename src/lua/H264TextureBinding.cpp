// H.264 Texture Binding for Solar2D
// Integrates H.264 decoder with Solar2D's texture system

#include <CoronaLua.h>
#include <CoronaMacros.h>
#include <CoronaLibrary.h>
#include <CoronaGraphics.h>

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

// H264MovieTexture wrapper for Solar2D texture integration  
struct H264MovieTexture {
    std::unique_ptr<plugin_h264::H264Movie> decoder;
    plugin_h264::VideoFrame current_video_frame;
    plugin_h264::AudioFrame current_audio_frame;
    
    // Converted RGBA frame data for Corona texture
    std::vector<uint8_t> rgba_data;
    
    // Playback state
    bool playing = false;
    bool stopped = false;
    bool audio_started = false;
    bool audio_completed = false;
    
    // Timing
    unsigned int elapsed = 0;
    unsigned int frame_ms = 0;
    
    // Audio (OpenAL integration)
    ALuint source = 0;
    ALenum audio_format = 0;
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
    
    if (movie->current_video_frame.isValid()) {
        // Convert YUV to RGBA if we have a new frame
        if (movie->rgba_data.empty() || 
            movie->rgba_data.size() != movie->current_video_frame.width * movie->current_video_frame.height * 4) {
            convertYUVtoRGBA(movie->current_video_frame, movie->rgba_data);
        }
        return movie->rgba_data.data();
    }
    
    return movie->empty;
}

// Core texture creation function
static int newMovieTexture(lua_State *L) {
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
    
    // Setup Solar2D texture callbacks
    CoronaExternalTextureCallbacks callbacks = {};
    callbacks.size = sizeof(CoronaExternalTextureCallbacks);
    callbacks.getWidth = GetWidth;
    callbacks.getHeight = GetHeight;
    callbacks.onRequestBitmap = GetImage;
    callbacks.onFinalize = [](void* context) {
        H264MovieTexture *movie = (H264MovieTexture*)context;
        delete movie;
    };
    
    return CoronaExternalPushTexture(L, &callbacks, movie);
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