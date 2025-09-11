local Movie = require("plugin.h264")
-- local Movie = require("plugin.movie")

local function _releaseTexture(tex)
	if tex and type(tex.releaseself) == "function" then
		tex:releaseself()
	end
end

local MovieRect = class("MovieRect", function(opts)
	local obj = display.newRect(0, 0, opts.width, opts.height)
	return obj
end)

function MovieRect.static.new(self, ...)
	local instance = self:allocate(...)
	instance:addEventListener("finalize")
	instance:initialize(...)
	return instance
end

function MovieRect:initialize(opts)
	local texture = Movie.newMovieTexture(opts)
	self.fill = {type="image", filename = texture.filename, baseDir = texture.baseDir}
	self.options = opts
	self.texture = texture
	self.channel = opts.channel
	if opts.x and opts.y then
		self:translate(opts.x, opts.y)
	end
	self.timerMgr = opts.timerMgr
	self._preserve = opts.preserve
	self._loop = opts.loop
	self.listener = opts.listener
	self._prevtime = -1
	-- self._delta = 0
	-- self._timePast = 0
	self._stop = false
	self.playing = false
	self._started = false
	self._complete = false
end

function MovieRect:enterFrame(event)
	if self.playing then
		if self._prevtime < 0 then
			self._prevtime = event.time
		end
		local delta = event.time - self._prevtime
		-- self._timePast = self._timePast + delta

		if self.texture.isActive then
			self.texture:update(delta)
			self.texture:invalidate()
			-- print("isActive", delta)
		else
			self._complete = true
			if self._loop then
				self:reset()
			else
				self:stop()
			end
		end
	end

	self._prevtime = event.time
end

function MovieRect:reset()
	if not self.playing then return end
	-- self.texture:update(-self._timePast)
	-- self.texture:invalidate()
	-- -- self._prevtime = -1
	-- self._timePast = 0
	self._complete = false
	if type(self.texture.replay) == "function" then
		local result = self.texture:replay()
		print(result, "reset 1111!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
	else
		local prevTexture = self.texture
		if prevTexture ~= nil and type(prevTexture.stop) == "function" then
			prevTexture:stop()
		end
		local texture = Movie.newMovieTexture(self.options)
		self.fill = {type="image", filename = texture.filename, baseDir = texture.baseDir}
		self._prevtime = -1
		self.texture = texture
		texture:play()
		texture:update(0)
		texture:invalidate()
		print("reset 2222!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")

		timer.performWithDelay(1, function()
			_releaseTexture(prevTexture)
		end)
	end
end

function MovieRect:play()
	if self.playing then return end

	self.texture:play()
	self.playing = true

	if not self._started then
		self._started = true
		Runtime:addEventListener('enterFrame', self)
	end
end

function MovieRect:pause()
	if not self.playing then return end

	self.playing = false
	self.texture:pause()
end

function MovieRect:stop()
	if self._stop then return end

	Runtime:removeEventListener('enterFrame', self)

	self.playing = false
	self.texture:stop()
	self._stop = true
	self._started = false

	if self.listener then
		self.listener(
			{
				name = 'movie',
				phase = 'stopped',
				completed = self._complete
			}
		)
	end

	if self._preserve then return end

	self:dispose()
end

function MovieRect:dispose()
	if self.playing then return end

	-- self.timerMgr:setTimeout(1,
	timer.performWithDelay(1,
		function()
			_releaseTexture(self.texture)
			self.texture = nil
			-- self:removeSelf()
		end
	)
	-- self:removeSelf()
end

function MovieRect:onRemove()
	Runtime:removeEventListener('enterFrame', self)
	local texture = self.texture
	-- self.timerMgr:setTimeout(1, function()
	timer.performWithDelay(1, function()
		_releaseTexture(texture)
   end)
end

function MovieRect:finalize(e)
	self:onRemove()
	for k,v in pairs(self) do
		if k:find("^_") == nil then
			self[k] = nil
		end
	end
end

return MovieRect
