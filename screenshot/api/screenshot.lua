---@meta
---@diagnostic disable: lowercase-global
---@diagnostic disable: missing-return
---@diagnostic disable: args-after-dots

---@class defold_api.screenshot
---Functions for capturing full-screen or partial screenshots.
screenshot = {}

---Capture a screenshot as PNG-encoded bytes. On native platforms this is
---synchronous unless a callback is provided. On HTML5, only the asynchronous
---callback form is supported.
--- Examples:
---```
---  -- fullscreen with callback
---  screenshot.png(function(self, image, w, h)
---    -- do something with image
---  end)
---
---  -- partial with callback
---  screenshot.png(0, 0, 100, 200, function(self, image, w, h)
---    -- do something with image
---  end)
---
---  -- fullscreen (native only)
---  local image, w, h = screenshot.png()
---```
---@param x? integer X coordinate (lower-left) for partial capture
---@param y? integer Y coordinate (lower-left) for partial capture
---@param w? integer Width for partial capture
---@param h? integer Height for partial capture
---@param callback? fun(self: userdata, image: string, width: integer, height: integer) Called on post-render (or next frame on HTML5) with the PNG bytes
---@return string|nil Image PNG data as a Lua string (native only, synchronous form)
---@return integer|nil width Width of the captured image in pixels (native only, synchronous form)
---@return integer|nil height Height of the captured image in pixels (native only, synchronous form)
function screenshot.png(x, y, w, h, callback) end

---Capture a screenshot as a Defold buffer. On native platforms this is
---synchronous unless a callback is provided. On HTML5, only the asynchronous
---callback form is supported.
--- Example:
---```
---  local buffer, w, h = screenshot.buffer()
---  resource.set_texture(go.get("cube#model", "texture0"), {
---    type = resource.TEXTURE_TYPE_2D,
---    width = w,
---    height = h,
---    format = resource.TEXTURE_FORMAT_RGBA
---  }, buffer)
---```
---@param x? integer X coordinate (lower-left) for partial capture
---@param y? integer Y coordinate (lower-left) for partial capture
---@param w? integer Width for partial capture
---@param h? integer Height for partial capture
---@param callback? fun(self: userdata, buffer: userdata, width: integer, height: integer) Called on post-render (or next frame on HTML5) with the buffer
---@return userdata|nil buffer Defold buffer containing RGBA pixels (native only, synchronous form)
---@return integer|nil width Width of the captured image in pixels (native only, synchronous form)
---@return integer|nil height Height of the captured image in pixels (native only, synchronous form)
function screenshot.buffer(x, y, w, h, callback) end

---Capture a screenshot as raw RGBA pixels in a Lua string. On native platforms
---this is synchronous unless a callback is provided. On HTML5, only the
---asynchronous callback form is supported.
--- Example:
---```
---  local pixels, w, h = screenshot.pixels()
---```
---@param x? integer X coordinate (lower-left) for partial capture
---@param y? integer Y coordinate (lower-left) for partial capture
---@param w? integer Width for partial capture
---@param h? integer Height for partial capture
---@param callback? fun(self: userdata, pixels: string, width: integer, height: integer) Called on post-render (or next frame on HTML5) with the pixel data
---@return string|nil pixels Raw RGBA bytes as a Lua string (native only, synchronous form)
---@return integer|nil width Width of the captured image in pixels (native only, synchronous form)
---@return integer|nil height Height of the captured image in pixels (native only, synchronous form)
function screenshot.pixels(x, y, w, h, callback) end

---HTML5-only alias for "screenshot.png()". Callback form only.
---@param callback? fun(self: userdata, image: string, width: integer, height: integer) Called on the next frame with PNG bytes
---@return nil
function screenshot.html5(callback) end
