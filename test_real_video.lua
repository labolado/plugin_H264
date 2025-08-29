#!/usr/bin/env lua

-- Real Video File Test
-- This test requires a real MP4 video file

local plugin_path = "./build/libplugin_h264.dylib"
package.cpath = package.cpath .. ";" .. plugin_path

print("Real Video File Test")
print("===================")

-- Load plugin
local success, h264 = pcall(require, "plugin_h264")
if not success then
    print("❌ Plugin failed to load:", h264)
    return
end

print("✅ Plugin loaded")

-- Create movie object
local movie = h264.newMovie()
if not movie then
    print("❌ Failed to create movie object")
    return
end

print("✅ Movie object created")

-- Event tracking
local events_received = {}

local function videoEventListener(event)
    table.insert(events_received, {
        type = event.type,
        phase = event.phase,
        message = event.message,
        timestamp = os.time()
    })
    
    print(string.format("📺 Event: %s - %s", event.phase, event.message or ""))
    
    if event.phase == "loaded" then
        print(string.format("   Duration: %.2f seconds", movie:getDuration()))
        
        -- Auto-play after loading
        print("🎬 Starting playback...")
        movie:play()
        
    elseif event.phase == "play" then
        -- Let it play for 3 seconds then pause
        timer.performWithDelay(3000, function()
            print("⏸️ Pausing playback...")
            movie:pause()
        end)
        
    elseif event.phase == "pause" then
        -- Wait 2 seconds then resume
        timer.performWithDelay(2000, function()
            print("▶️ Resuming playback...")
            movie:play()
        end)
    end
end

-- Add event listener
movie:addEventListener("videoEvent", videoEventListener)

-- Test different video files
local test_videos = {
    "test_video.mp4",  -- You need to provide this
    "sample.mp4",
    "video.mp4",
    -- Add paths to your test videos
}

print("\nTrying to load test video files...")
local loaded = false

for _, video_path in ipairs(test_videos) do
    print(string.format("Trying: %s", video_path))
    
    local result = movie:loadVideo(video_path)
    if result then
        loaded = true
        print(string.format("✅ Successfully loaded: %s", video_path))
        break
    else
        print(string.format("❌ Failed to load: %s", video_path))
    end
end

if not loaded then
    print("\n⚠️  No test video files found!")
    print("To test with real video:")
    print("1. Add a H.264/AAC MP4 file to the current directory")
    print("2. Update the test_videos table with the correct filename")
    print("3. Run this test again")
    
    -- Create a simple timer for demonstration
    local function showStatus()
        print(string.format("Status - Playing: %s, Time: %.1f/%.1f", 
              tostring(movie:isPlaying()), 
              movie:getCurrentTime(), 
              movie:getDuration()))
    end
    
    -- Show status every 2 seconds
    timer.performWithDelay(2000, showStatus, 5)
    
else
    -- Keep the test running for video playback
    print("\n🎬 Video test in progress...")
    print("The video will auto-play, pause after 3s, then resume after 2s")
    
    -- Status updates
    local function updateStatus()
        if movie:isPlaying() then
            local current = movie:getCurrentTime()
            local duration = movie:getDuration()
            local progress = duration > 0 and (current / duration * 100) or 0
            
            print(string.format("⏱️  Playing: %.1fs / %.1fs (%.1f%%)", 
                  current, duration, progress))
        end
    end
    
    timer.performWithDelay(1000, updateStatus, 0)
end

-- Cleanup function
local function cleanup()
    print("\n🧹 Cleaning up...")
    if movie then
        movie:stop()
        movie:removeEventListener("videoEvent", videoEventListener)
    end
    
    print("📊 Events received:")
    for i, event in ipairs(events_received) do
        print(string.format("  %d. %s - %s", i, event.phase, event.message or ""))
    end
    
    print("\n✅ Test completed")
end

-- Run cleanup after 15 seconds
timer.performWithDelay(15000, cleanup)

print("\nTest will run for 15 seconds...")