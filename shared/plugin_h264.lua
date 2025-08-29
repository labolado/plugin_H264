-- Plugin H264 - Solar2D Video Plugin
-- Compatible with plugin_movie API

local Library = require "CoronaLibrary"

-- Create library
local lib = Library:new{ name="plugin.h264", publisherId="com.plugin", version=1 }

-------------------------------------------------------------------------------
-- BEGIN (Insert your implementation starting here)
-------------------------------------------------------------------------------

-- This is called when the library is required
function lib.init()
end

-- newMovieTexture creates the underlying texture object
function lib.newMovieTexture(params)
    if not params then
        print("ERROR: plugin.h264.newMovieTexture() requires parameters")
        return nil
    end
    
    if not params.filename then
        print("ERROR: plugin.h264.newMovieTexture() requires filename parameter")
        return nil
    end
    
    -- Get audio source if provided
    local audioSource = params.audioSource or 0
    
    -- Call native function to create texture
    return lib._newMovieTexture(params.filename, audioSource)
end

-- newMovieRect creates a display object with the movie texture
function lib.newMovieRect(params)
    if not params then
        print("ERROR: plugin.h264.newMovieRect() requires parameters")
        return nil
    end
    
    -- Create the underlying texture
    local texture = lib.newMovieTexture(params)
    if not texture then
        return nil
    end
    
    -- Create display rect with the texture
    local rect = display.newImageRect(texture.filename, texture.baseDir, params.width or 320, params.height or 240)
    if not rect then
        print("ERROR: Failed to create display object")
        return nil
    end
    
    -- Attach texture to rect
    rect.texture = texture
    
    -- Add movie control methods
    rect.play = function(self)
        if self.texture and self.texture.play then
            self.texture:play()
        end
    end
    
    rect.pause = function(self)
        if self.texture and self.texture.pause then
            self.texture:pause()
        end
    end
    
    rect.seek = function(self, time)
        if self.texture and self.texture.seek then
            self.texture:seek(time)
        end
    end
    
    -- Runtime update loop for texture refresh
    rect._updateListener = function(event)
        if rect.texture and rect.texture.update then
            rect.texture:update(event.frame)
            rect.texture:invalidate()
        end
    end
    
    -- Add to runtime
    Runtime:addEventListener("enterFrame", rect._updateListener)
    
    -- Cleanup on finalize
    local originalFinalize = rect._finalize
    rect._finalize = function(self)
        Runtime:removeEventListener("enterFrame", rect._updateListener)
        if originalFinalize then
            originalFinalize(self)
        end
    end
    
    return rect
end

-------------------------------------------------------------------------------
-- END
-------------------------------------------------------------------------------

-- Return an instance
return lib