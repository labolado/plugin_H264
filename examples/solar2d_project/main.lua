-- H.264 Video Plugin Example for Solar2D
-- This example demonstrates how to use the H.264 video plugin (plugin_movie compatible API)
class = require("lib.system.middleclass_ext")

local MovieRect = require("movie_rect")

-- Import the plugin
-- local h264 = require("plugin.h264")

print("H.264 Plugin Example for Solar2D")
print("================================")

-- Create display objects
local display = require("display")
local widget = require("widget")

-- Background
local background = display.newRect(display.contentCenterX, display.contentCenterY,
                                   display.contentWidth, display.contentHeight)
background:setFillColor(0.2, 0.2, 0.2)

local group = display.newGroup()

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

-- Video display object (will replace placeholder when loaded)
local movieRect = nil

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
        -- local videoPath = "file_example_MP4_640_3MG.mp4"  -- Place this file in your project
        local videoPath = "ad_mp4"  -- Place this file in your project
        -- local videoPath = "ad2_mp4"  -- Place this file in your project
        -- local videoPath = "test2_mp4"  -- Place this file in your project
        -- local videoPath = "test2.mp4"  -- Place this file in your project
        -- local videoPath = "ad.mp4"  -- Place this file in your project
        -- local videoPath = "test_short.mp4"  -- Place this file in your project
        -- local videoPath = "test_short2.mp4"  -- Place this file in your project
        -- local videoPath = "test3.mp4"  -- Place this file in your project
        -- local videoPath = "output.mp4"  -- Place this file in your project
        -- local videoPath = "auido_only_output.mp4"  -- Place this file in your project
        -- local videoPath = "ad.ogv"  -- Place this file in your project
        -- local videoPath = "output2.ogv"  -- Place this file in your project
        print("Attempting to load:", videoPath)

        -- Remove old movie if exists
        if movieRect then
            movieRect:removeSelf()
            movieRect = nil
        end

        -- Create new movie display object using plugin_movie compatible API
        -- movieRect = h264.newMovieRect({
        movieRect = MovieRect:new({
            filename = videoPath,
            baseDir = system.ResourceDirectory,
            width = 300,
            height = 200,
            loop = true,
            preserve = true,
            x = display.contentCenterX,
            y = 200
        })
        if movieRect then
            group:insert(movieRect)
            statusText.text = "Video loaded successfully"
            statusText:setFillColor(0, 1, 0)
            placeholderText.text = "Video Ready"
            -- Hide placeholder
            videoPlaceholder.isVisible = false
            placeholderText.isVisible = false

        else
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
        if movieRect and movieRect.play then
            movieRect:play()
            statusText.text = "Playing video"
            statusText:setFillColor(0, 0.8, 1)
        else
            statusText.text = "No video loaded"
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
        if movieRect and movieRect.pause then
            movieRect:pause()
            -- movieRect.texture:replay()
            statusText.text = "Video paused"
            statusText:setFillColor(1, 1, 0)
        end
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
        if movieRect then
            movieRect:removeSelf()
            movieRect = nil
            statusText.text = "Video stopped"
            statusText:setFillColor(0.8, 0.8, 0.8)
            -- Show placeholder again
            videoPlaceholder.isVisible = true
            placeholderText.isVisible = true
            placeholderText.text = "Video Display Area\n(Stopped)"
        end
    end
})

-- Instructions
local instructions = display.newText({
    text = "Instructions:\n" ..
           "1. Place a sample_video.mp4 file in your project directory\n" ..
           "2. Click 'Load Video' to load the video\n" ..
           "3. Use Play/Pause/Stop buttons to control playback\n\n" ..
           "Supported formats: MP4 with H.264 video and AAC audio",
    x = display.contentCenterX,
    y = 450,
    width = 350,
    font = native.systemFont,
    fontSize = 10,
    align = "center"
})
instructions:setFillColor(0.7, 0.7, 0.7)

-- Runtime:addEventListener("key", function(event)
--     if event.keyName == "space" then
--         if movieRect then
--             movieRect:play()
--         end
--     end
--     if event.keyName == "q" then
--         if movieRect then
--             movieRect:stop()
--         end
--     end
--     if event.keyName == "s" then
--         if movieRect then
--             movieRect:pause()
--         end
--     end
--     if event.keyName == "r" then
--         if movieRect then
--             movieRect.texture:replay()
--         end
--     end
--     if event.keyName == "right" then
--         if movieRect then
--             movieRect.texture:seek(movieRect.texture.currentTime + 10)
--             -- print(movieRect.texture.currentTime)
--         end
--     end
-- end)

-- Cleanup on app exit
local function onSystemEvent(event)
    if event.type == "applicationExit" then
        if movieRect then
            movieRect:removeSelf()
            movieRect = nil
        end
    end
end

Runtime:addEventListener("system", onSystemEvent)

print("Solar2D H.264 Video Plugin Example loaded successfully!")
print("Ready to load and play H.264 videos using plugin_movie compatible API.")


local frame = 0
local fps = display.fps
local lastTime = system.getTimer()
local memoryUsageText
local function monitorMem()
    frame = frame + 1
    local currTime = system.getTimer()
    if currTime-lastTime >= 1000 then
        lastTime = currTime
        fps = frame
        frame = 0
    end
    memoryUsageText.text = string.format("FPS:%s", fps)
end

if memoryUsageText == nil then
    memoryUsageText = display.newText("fps", display.contentCenterX, 15, native.systemoFont, 30)
    memoryUsageText.alpha = .5
    memoryUsageText:setFillColor(1, 0, 0, 1)
    Runtime:addEventListener("enterFrame", monitorMem)
end
