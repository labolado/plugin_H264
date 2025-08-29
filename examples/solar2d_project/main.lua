-- H.264 Video Plugin Example for Solar2D
-- This example demonstrates how to use the H.264 video plugin

-- Import the plugin
local h264 = require("plugin.h264")

print("H.264 Plugin Example for Solar2D")
print("================================")

-- Display plugin information
print("Plugin Version:", h264.getVersion())
local libInfo = h264.getLibraryInfo()
print("Libraries:")
for k, v in pairs(libInfo) do
    print(" -", k .. ":", v)
end
print()

-- Create display objects
local display = require("display")
local widget = require("widget")

-- Background
local background = display.newRect(display.contentCenterX, display.contentCenterY, 
                                   display.contentWidth, display.contentHeight)
background:setFillColor(0.2, 0.2, 0.2)

-- Title
local title = display.newText({
    text = "H.264 Video Player",
    x = display.contentCenterX,
    y = 50,
    font = native.systemFontBold,
    fontSize = 24
})
title:setFillColor(1, 1, 1)

-- Status text
local statusText = display.newText({
    text = "Ready to load video",
    x = display.contentCenterX,
    y = 100,
    font = native.systemFont,
    fontSize = 14,
    align = "center"
})
statusText:setFillColor(0.8, 0.8, 0.8)

-- Video placeholder
local videoPlaceholder = display.newRect(display.contentCenterX, 200, 300, 200)
videoPlaceholder:setFillColor(0.1, 0.1, 0.1)
videoPlaceholder.strokeWidth = 2
videoPlaceholder:setStrokeColor(0.5, 0.5, 0.5)

local placeholderText = display.newText({
    text = "Video Display Area\n(No video loaded)",
    x = display.contentCenterX,
    y = 200,
    font = native.systemFont,
    fontSize = 12,
    align = "center"
})
placeholderText:setFillColor(0.6, 0.6, 0.6)

-- Create movie object
local movie = h264.newMovie()

-- Event listener for video events
local function onVideoEvent(event)
    print("Video Event:", event.type, "Phase:", event.phase)
    
    if event.phase == "loaded" then
        statusText.text = "Video loaded successfully"
        statusText:setFillColor(0, 1, 0)
        placeholderText.text = "Video Ready\\n" .. 
                              "Duration: " .. math.floor(movie:getDuration()) .. "s"
        
    elseif event.phase == "play" then
        statusText.text = "Playing video"
        statusText:setFillColor(0, 0.8, 1)
        
    elseif event.phase == "pause" then
        statusText.text = "Video paused"
        statusText:setFillColor(1, 1, 0)
        
    elseif event.phase == "stop" then
        statusText.text = "Video stopped"
        statusText:setFillColor(0.8, 0.8, 0.8)
        
    elseif event.phase == "error" then
        statusText.text = "Error: " .. (event.message or "Unknown error")
        statusText:setFillColor(1, 0, 0)
        print("Video Error:", event.message)
    end
end

-- Add event listener
movie:addEventListener("videoEvent", onVideoEvent)

-- Control buttons
local buttonY = 350
local buttonSpacing = 80

-- Load button
local loadButton = widget.newButton({
    label = "Load Video",
    x = display.contentCenterX - buttonSpacing * 1.5,
    y = buttonY,
    width = 70,
    height = 30,
    fontSize = 12,
    onRelease = function()
        -- In a real app, you would use system.pathForFile() or select a file
        -- For this example, we'll try to load a sample video
        local videoPath = "sample_video.mp4"  -- Place this file in your project
        print("Attempting to load:", videoPath)
        
        local success = movie:loadVideo(videoPath)
        if not success then
            statusText.text = "Failed to load video file"
            statusText:setFillColor(1, 0, 0)
        end
    end
})

-- Play button
local playButton = widget.newButton({
    label = "Play",
    x = display.contentCenterX - buttonSpacing * 0.5,
    y = buttonY,
    width = 70,
    height = 30,
    fontSize = 12,
    onRelease = function()
        local success = movie:play()
        if not success then
            statusText.text = "Failed to start playback"
            statusText:setFillColor(1, 0.5, 0)
        end
    end
})

-- Pause button
local pauseButton = widget.newButton({
    label = "Pause",
    x = display.contentCenterX + buttonSpacing * 0.5,
    y = buttonY,
    width = 70,
    height = 30,
    fontSize = 12,
    onRelease = function()
        movie:pause()
    end
})

-- Stop button
local stopButton = widget.newButton({
    label = "Stop",
    x = display.contentCenterX + buttonSpacing * 1.5,
    y = buttonY,
    width = 70,
    height = 30,
    fontSize = 12,
    onRelease = function()
        movie:stop()
        placeholderText.text = "Video Display Area\\n(Stopped)"
    end
})

-- Time display
local timeText = display.newText({
    text = "00:00 / 00:00",
    x = display.contentCenterX,
    y = 400,
    font = native.systemFont,
    fontSize = 14
})
timeText:setFillColor(0.9, 0.9, 0.9)

-- Update time display
local function updateTime()
    if movie then
        local current = movie:getCurrentTime()
        local duration = movie:getDuration()
        
        local currentMin = math.floor(current / 60)
        local currentSec = math.floor(current % 60)
        local durationMin = math.floor(duration / 60)
        local durationSec = math.floor(duration % 60)
        
        timeText.text = string.format("%02d:%02d / %02d:%02d", 
                                     currentMin, currentSec, durationMin, durationSec)
        
        -- Update placeholder with playback info if playing
        if movie:isPlaying() and duration > 0 then
            local progress = math.floor((current / duration) * 100)
            placeholderText.text = string.format("Playing Video\\nProgress: %d%%", progress)
        end
    end
end

-- Timer to update time display
timer.performWithDelay(1000, updateTime, 0)

-- Instructions
local instructions = display.newText({
    text = "Instructions:\\n" ..
           "1. Place a sample_video.mp4 file in your project directory\\n" ..
           "2. Click 'Load Video' to load the video\\n" ..
           "3. Use Play/Pause/Stop buttons to control playback\\n\\n" ..
           "Supported formats: MP4 with H.264 video and AAC audio",
    x = display.contentCenterX,
    y = 500,
    width = 350,
    font = native.systemFont,
    fontSize = 10,
    align = "center"
})
instructions:setFillColor(0.7, 0.7, 0.7)

-- Cleanup on app exit
local function onSystemEvent(event)
    if event.type == "applicationExit" then
        if movie then
            movie:removeEventListener("videoEvent", onVideoEvent)
            movie:stop()
        end
    end
end

Runtime:addEventListener("system", onSystemEvent)

print("Solar2D H.264 Video Plugin Example loaded successfully!")
print("Ready to load and play H.264 videos.")