# H.264 Video Plugin for Solar2D - Example Project

This example demonstrates how to use the H.264 video plugin in a Solar2D application.

## Setup

1. **Build the Plugin**: Make sure the plugin has been built successfully:
   ```bash
   cd /path/to/plugin_H264
   mkdir build && cd build
   cmake .. && make
   ```

2. **Install Plugin**: Copy the built plugin to your Solar2D plugins directory or include it in your project.

3. **Add Sample Video**: Place a sample MP4 file (with H.264 video and AAC audio) named `sample_video.mp4` in your project directory.

## Features Demonstrated

- **Plugin Loading**: How to require and initialize the H.264 plugin
- **Movie Object Creation**: Creating a movie player instance
- **Event Handling**: Listening for video events (loaded, play, pause, stop, error)
- **Playback Control**: Loading, playing, pausing, and stopping videos
- **Status Updates**: Displaying current playback time and duration
- **Error Handling**: Graceful handling of loading and playback errors

## File Structure

```
solar2d_project/
├── main.lua           # Main application logic
├── build.settings     # Solar2D build configuration
├── config.lua         # App configuration
├── sample_video.mp4   # Sample video file (you need to add this)
└── README.md          # This file
```

## Usage

1. Launch the Solar2D simulator
2. Open this project
3. Click "Load Video" to load the sample video
4. Use the control buttons to play, pause, or stop the video
5. Monitor the status text for feedback

## Plugin API

The plugin exposes the following methods:

### Global Functions
- `h264.getVersion()` - Get plugin version
- `h264.getLibraryInfo()` - Get information about underlying libraries
- `h264.newMovie()` - Create a new movie player instance

### Movie Object Methods
- `movie:loadVideo(filePath)` - Load a video file
- `movie:play()` - Start playback
- `movie:pause()` - Pause playback
- `movie:stop()` - Stop playback
- `movie:seek(timeMs)` - Seek to specific time
- `movie:isPlaying()` - Check if currently playing
- `movie:getDuration()` - Get total video duration in seconds
- `movie:getCurrentTime()` - Get current playback time in seconds
- `movie:addEventListener(eventType, listener)` - Add event listener
- `movie:removeEventListener(eventType, listener)` - Remove event listener

### Events

The plugin dispatches "videoEvent" events with the following phases:
- `"loaded"` - Video loaded successfully
- `"play"` - Playback started
- `"pause"` - Playback paused
- `"stop"` - Playback stopped
- `"error"` - An error occurred (includes message field)

## Supported Formats

- **Video**: H.264 (AVC)
- **Audio**: AAC
- **Container**: MP4

## Notes

- This is a demonstration project showing basic plugin usage
- In production applications, implement proper error handling and user feedback
- Consider adding progress bars, volume controls, and fullscreen support
- Test with various video files to ensure compatibility