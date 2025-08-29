# Plugin H264 - Solar2D Video Plugin

A high-performance H.264 video plugin for Solar2D, providing plugin_movie compatible API for seamless video playback integration.

## Supported Video Formats

### Video Codec
- **H.264/AVC**: High-profile, Main-profile, Baseline-profile
- **Container**: MP4, MOV
- **Resolution**: Up to 4K (4096×2160)
- **Frame rates**: 15, 24, 25, 30, 50, 60 fps
- **Bitrate**: Variable bitrate (VBR) and Constant bitrate (CBR)

### Audio Codec  
- **AAC**: Advanced Audio Coding
- **Sample rates**: 8000, 11025, 16000, 22050, 44100, 48000 Hz
- **Channels**: Mono, Stereo, 5.1 surround
- **Bitrate**: 64-320 kbps

### Recommended Video Settings
```
- Video Codec: H.264 High Profile
- Container: MP4
- Resolution: 1920×1080 or lower for mobile devices  
- Frame Rate: 30 fps
- Video Bitrate: 2-8 Mbps
- Audio Codec: AAC
- Audio Bitrate: 128 kbps
- Audio Sample Rate: 44100 Hz
```

## Installation

### For Solar2D Development

1. Place the plugin files in your Solar2D plugins directory
2. Add to your `build.settings`:

```lua
settings = {
    plugins = {
        ["plugin.h264"] = {
            publisherId = "com.plugin",
        },
    },
}
```

### For Standalone Development/Testing

1. Clone this repository
2. Install dependencies:
   ```bash
   # macOS
   brew install lua pkg-config cmake
   
   # Linux
   sudo apt-get install liblua5.4-dev lua5.4 cmake pkg-config
   ```

3. Build third-party dependencies:
   ```bash
   cd third_party/openh264
   make -j4
   sudo make install
   
   cd ../fdk-aac
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j4
   ```

4. Build the plugin:
   ```bash
   mkdir build && cd build
   cmake .. -DSTANDALONE_BUILD=ON
   make -j4
   ```

## Usage

### Basic Usage (plugin_movie compatible)

```lua
local h264 = require("plugin.h264")

-- Create a movie display object
local movieRect = h264.newMovieRect({
    filename = "video.mp4",
    width = 320,
    height = 240,
    x = display.contentCenterX,
    y = display.contentCenterY
})

-- Control playback
movieRect:play()
movieRect:pause()

-- Cleanup
movieRect:removeSelf()
```

### Advanced Usage

```lua
local h264 = require("plugin.h264")

-- Create movie texture directly
local texture = h264.newMovieTexture({
    filename = "video.mp4",
    audioSource = audioSource  -- Optional OpenAL source
})

-- Create display object with the texture
local rect = display.newImageRect(texture.filename, texture.baseDir, 320, 240)
rect.texture = texture

-- Runtime update loop
local function onEnterFrame(event)
    if rect.texture then
        rect.texture:update(event.frame)
        rect.texture:invalidate()
    end
end
Runtime:addEventListener("enterFrame", onEnterFrame)
```

## API Reference

### h264.newMovieRect(options)

Creates a Solar2D display object with video texture.

**Parameters:**
- `options` (table): Configuration options
  - `filename` (string): Path to video file
  - `width` (number): Display width
  - `height` (number): Display height  
  - `x` (number): X position
  - `y` (number): Y position
  - `audioSource` (number, optional): OpenAL audio source ID

**Returns:** Display object with video texture

**Methods:**
- `object:play()`: Start video playback
- `object:pause()`: Pause video playback
- `object:removeSelf()`: Stop and cleanup video

### h264.newMovieTexture(options)

Creates a video texture for manual display object creation.

**Parameters:**
- `options` (table): Configuration options
  - `filename` (string): Path to video file
  - `audioSource` (number, optional): OpenAL audio source ID

**Returns:** Texture object for use with `display.newImageRect()`

## Building

### Prerequisites

- CMake 3.10+
- C++14 compatible compiler
- Git (for submodules)

### Setup

```bash
# Clone with submodules
git clone --recursive https://github.com/user/plugin_H264.git
cd plugin_H264

# Or if already cloned
git submodule update --init --recursive
```

### Cross-Platform Build

The plugin supports dual-mode building:

#### Standalone Mode (Development/Testing)
```bash
cmake .. -DSTANDALONE_BUILD=ON
```
- Includes Corona SDK compatibility layer
- Can run independently for testing
- Links against system Lua

#### Solar2D Deployment Mode  
```bash
cmake .. -DSTANDALONE_BUILD=OFF
```
- For Solar2D plugin deployment
- Requires Solar2D runtime environment
- Minimal dependencies

## File Structure

```
plugin_H264/
├── src/
│   ├── decoders/           # H.264 and AAC decoders
│   ├── managers/           # Video management classes  
│   ├── utils/              # Utility functions
│   └── lua/                # Lua binding layer
├── include/                # Header files
├── shared/                 # Lua wrapper files
├── examples/               # Example projects
├── third_party/            # External dependencies
└── tests/                  # Unit tests
```

## Supported Platforms

- **macOS**: Intel and Apple Silicon
- **Linux**: x86_64, ARM64  
- **Windows**: x64
- **Android**: ARM64, ARMv7
- **iOS**: ARM64 (Solar2D deployment)

## Dependencies

- **OpenH264**: H.264 video decoding
- **FDK-AAC**: AAC audio decoding  
- **minimp4**: MP4 container parsing
- **Lua 5.4**: Scripting interface
- **OpenAL**: Audio playback (optional)

## Performance

- **Hardware acceleration**: Utilizes platform-specific optimizations
- **Memory efficient**: Streaming-based decoding
- **Low latency**: Optimized for real-time playback
- **Thread-safe**: Multi-threaded decoder architecture

## Troubleshooting

### Common Issues

1. **Video not loading**
   - Check if file exists and is readable
   - Verify video format is supported (H.264/AAC in MP4)
   - Check file permissions

2. **Audio not playing**
   - Ensure OpenAL is initialized
   - Verify audio codec is AAC
   - Check audio source configuration

3. **Performance issues**
   - Reduce video resolution/bitrate
   - Check available system memory
   - Monitor CPU usage

### Debug Logging

Enable debug output by setting environment variable:
```bash
export PLUGIN_H264_DEBUG=1
```

## License

- Plugin code: MIT License
- OpenH264: BSD 2-Clause License
- FDK-AAC: Custom license (allows commercial use)
- MiniMP4: MIT License

## Contributing

Please follow the established coding standards and ensure all tests pass before submitting pull requests.