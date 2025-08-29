#!/usr/bin/env lua

-- Simple Lua test script for plugin H264
-- This tests the basic plugin loading and functionality

print("Testing H.264 Plugin for Solar2D/Corona SDK")
print("============================================")

-- Load the plugin
package.cpath = package.cpath .. ";./build/libplugin_h264.dylib"

local success, plugin = pcall(require, "plugin_h264")
if not success then
    print("❌ Failed to load plugin: " .. tostring(plugin))
    return
end

print("✅ Plugin loaded successfully")

-- Test plugin information
print("\nPlugin Information:")
print("- Version:", plugin.getVersion())

local libInfo = plugin.getLibraryInfo()
print("- Libraries:")
for k, v in pairs(libInfo) do
    print("  -", k .. ":", v)
end

-- Test creating a movie object
print("\nTesting movie object creation:")
local movie = plugin.newMovie()
if movie then
    print("✅ Movie object created successfully")
    
    -- Test basic methods
    print("\nTesting movie methods:")
    
    -- Test duration (should be 0 for unloaded movie)
    local duration = movie:getDuration()
    print("- Duration:", duration .. " seconds")
    
    -- Test current time
    local currentTime = movie:getCurrentTime()
    print("- Current time:", currentTime .. " seconds")
    
    -- Test play state
    local isPlaying = movie:isPlaying()
    print("- Is playing:", tostring(isPlaying))
    
    -- Test event listener
    print("\nTesting event listener:")
    local function movieEventListener(event)
        print("📺 Movie event received:")
        print("  - Type:", event.type)
        print("  - Phase:", event.phase)
        if event.message then
            print("  - Message:", event.message)
        end
    end
    
    local success = movie:addEventListener("videoEvent", movieEventListener)
    print("- Event listener added:", tostring(success))
    
    -- Test loading a non-existent file to trigger error event
    print("\nTesting load (with non-existent file):")
    local loadResult = movie:loadVideo("non_existent_file.mp4")
    print("- Load result:", tostring(loadResult))
    
    print("\n✅ Basic functionality test completed")
else
    print("❌ Failed to create movie object")
end

print("\nTest completed successfully!")