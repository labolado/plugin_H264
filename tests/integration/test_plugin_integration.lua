#!/usr/bin/env lua

-- Integration Test Suite for H.264 Plugin
-- This script performs comprehensive testing of the plugin functionality

local plugin_path = "./build/libplugin_h264.dylib"
package.cpath = package.cpath .. ";" .. plugin_path

-- Test configuration
local TEST_CONFIG = {
    TIMEOUT_MS = 5000,
    SAMPLE_VIDEO = "tests/sample_video.mp4",  -- You would need to add this
    INVALID_VIDEO = "tests/invalid_file.mp4"
}

-- Test results tracking
local test_results = {
    total = 0,
    passed = 0,
    failed = 0,
    errors = {}
}

-- Utility functions
local function log(message)
    print("[TEST] " .. message)
end

local function test_assert(condition, message)
    test_results.total = test_results.total + 1
    if condition then
        test_results.passed = test_results.passed + 1
        log("✅ PASS: " .. message)
        return true
    else
        test_results.failed = test_results.failed + 1
        table.insert(test_results.errors, message)
        log("❌ FAIL: " .. message)
        return false
    end
end

local function run_test(name, test_func)
    log("Running test: " .. name)
    local status, error_msg = pcall(test_func)
    if not status then
        test_results.total = test_results.total + 1
        test_results.failed = test_results.failed + 1
        table.insert(test_results.errors, name .. " - " .. tostring(error_msg))
        log("❌ ERROR: " .. name .. " - " .. tostring(error_msg))
    end
    log("")
end

-- Test: Plugin Loading
local function test_plugin_loading()
    local success, plugin = pcall(require, "plugin_h264")
    test_assert(success, "Plugin loads successfully")
    
    if success then
        test_assert(type(plugin) == "table", "Plugin returns a table")
        test_assert(type(plugin.newMovie) == "function", "Plugin has newMovie function")
        test_assert(type(plugin.getVersion) == "function", "Plugin has getVersion function")
        test_assert(type(plugin.getLibraryInfo) == "function", "Plugin has getLibraryInfo function")
        
        return plugin
    end
    return nil
end

-- Test: Plugin Information
local function test_plugin_info(plugin)
    if not plugin then return end
    
    local version = plugin.getVersion()
    test_assert(type(version) == "string", "getVersion returns string")
    test_assert(version:match("%d+%.%d+%.%d+"), "Version follows semantic versioning")
    
    local libInfo = plugin.getLibraryInfo()
    test_assert(type(libInfo) == "table", "getLibraryInfo returns table")
    test_assert(type(libInfo.h264decoder) == "string", "Has h264decoder info")
    test_assert(type(libInfo.aacdecoder) == "string", "Has aacdecoder info")
    test_assert(type(libInfo.mp4demuxer) == "string", "Has mp4demuxer info")
end

-- Test: Movie Object Creation and Methods
local function test_movie_object(plugin)
    if not plugin then return end
    
    local movie = plugin.newMovie()
    test_assert(movie ~= nil, "Movie object created successfully")
    
    if movie then
        -- Test method existence
        test_assert(type(movie.loadVideo) == "function", "Movie has loadVideo method")
        test_assert(type(movie.play) == "function", "Movie has play method")
        test_assert(type(movie.pause) == "function", "Movie has pause method")
        test_assert(type(movie.stop) == "function", "Movie has stop method")
        test_assert(type(movie.seek) == "function", "Movie has seek method")
        test_assert(type(movie.isPlaying) == "function", "Movie has isPlaying method")
        test_assert(type(movie.getDuration) == "function", "Movie has getDuration method")
        test_assert(type(movie.getCurrentTime) == "function", "Movie has getCurrentTime method")
        test_assert(type(movie.addEventListener) == "function", "Movie has addEventListener method")
        test_assert(type(movie.removeEventListener) == "function", "Movie has removeEventListener method")
        
        -- Test initial state
        test_assert(movie:getDuration() == 0.0, "Initial duration is 0")
        test_assert(movie:getCurrentTime() == 0.0, "Initial current time is 0")
        test_assert(movie:isPlaying() == false, "Initial state is not playing")
        
        return movie
    end
    return nil
end

-- Test: Event System
local function test_event_system(movie)
    if not movie then return end
    
    local event_received = false
    local event_data = {}
    
    local function testListener(event)
        event_received = true
        event_data = event
    end
    
    -- Test adding event listener
    local success = movie:addEventListener("videoEvent", testListener)
    test_assert(success == true, "Event listener added successfully")
    
    -- Test removing event listener
    success = movie:removeEventListener("videoEvent", testListener)
    test_assert(success == true, "Event listener removed successfully")
    
    -- Test invalid event type
    local status, error_msg = pcall(function()
        movie:addEventListener("invalidEvent", testListener)
    end)
    test_assert(status == false, "Invalid event type properly rejected")
end

-- Test: Error Handling
local function test_error_handling(movie)
    if not movie then return end
    
    local event_received = false
    local error_event = nil
    
    local function errorListener(event)
        event_received = true
        error_event = event
    end
    
    movie:addEventListener("videoEvent", errorListener)
    
    -- Test loading invalid file
    local result = movie:loadVideo(TEST_CONFIG.INVALID_VIDEO)
    test_assert(result == false, "Loading invalid file returns false")
    
    -- Give some time for event to be dispatched
    local start_time = os.time()
    while not event_received and (os.time() - start_time) < 2 do
        -- Wait for event
    end
    
    test_assert(event_received, "Error event was dispatched")
    if event_received then
        test_assert(error_event.type == "videoEvent", "Event has correct type")
        test_assert(error_event.phase == "error", "Event has error phase")
        test_assert(type(error_event.message) == "string", "Event has error message")
    end
    
    movie:removeEventListener("videoEvent", errorListener)
end

-- Test: Memory Management
local function test_memory_management(plugin)
    if not plugin then return end
    
    -- Create multiple movie objects and let them be garbage collected
    local movies = {}
    for i = 1, 5 do
        movies[i] = plugin.newMovie()
        test_assert(movies[i] ~= nil, "Movie object " .. i .. " created")
    end
    
    -- Clear references
    movies = nil
    collectgarbage("collect")
    
    test_assert(true, "Memory management test completed")
end

-- Test: Concurrent Operations
local function test_concurrent_operations(plugin)
    if not plugin then return end
    
    local movie1 = plugin.newMovie()
    local movie2 = plugin.newMovie()
    
    test_assert(movie1 ~= nil and movie2 ~= nil, "Multiple movie objects created")
    
    if movie1 and movie2 then
        -- Test that objects are independent
        test_assert(movie1 ~= movie2, "Movie objects are independent")
        
        -- Test basic operations on both
        test_assert(movie1:isPlaying() == false, "Movie1 initial state")
        test_assert(movie2:isPlaying() == false, "Movie2 initial state")
        
        -- Test that operations on one don't affect the other
        movie1:play()  -- Will fail since no video loaded, but shouldn't affect movie2
        test_assert(movie2:isPlaying() == false, "Operations are isolated between objects")
    end
end

-- Main test execution
local function run_all_tests()
    print("H.264 Plugin Integration Test Suite")
    print("===================================")
    print()
    
    local plugin = nil
    
    -- Run tests
    run_test("Plugin Loading", function()
        plugin = test_plugin_loading()
    end)
    
    run_test("Plugin Information", function()
        test_plugin_info(plugin)
    end)
    
    local movie = nil
    run_test("Movie Object Creation", function()
        movie = test_movie_object(plugin)
    end)
    
    run_test("Event System", function()
        test_event_system(movie)
    end)
    
    run_test("Error Handling", function()
        test_error_handling(movie)
    end)
    
    run_test("Memory Management", function()
        test_memory_management(plugin)
    end)
    
    run_test("Concurrent Operations", function()
        test_concurrent_operations(plugin)
    end)
    
    -- Print results
    print("Test Results:")
    print("=============")
    print(string.format("Total: %d, Passed: %d, Failed: %d", 
          test_results.total, test_results.passed, test_results.failed))
    
    if test_results.failed > 0 then
        print()
        print("Failed Tests:")
        for _, error in ipairs(test_results.errors) do
            print("  - " .. error)
        end
    end
    
    print()
    if test_results.failed == 0 then
        print("🎉 All tests passed!")
        os.exit(0)
    else
        print("💥 Some tests failed!")
        os.exit(1)
    end
end

-- Run the test suite
run_all_tests()