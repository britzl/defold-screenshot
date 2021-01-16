![](logo.png)

# Defold-Screenshot
Screenshot extension for the [Defold game engine](http://www.defold.com)

## Installation
You can use Defold-Screenshot in your own project by adding this project as a [Defold library dependency](http://www.defold.com/manuals/libraries/). Open your game.project file and in the dependencies field under project add:

	https://github.com/britzl/defold-screenshot/archive/master.zip

Or point to the ZIP file of a [specific release](https://github.com/britzl/defold-screenshot/releases).

## Usage
The extensions will declare a new module, exposed to Lua as `screenshot`. The extension has the following functionality:
```lua
	-- The extension has an alternative implementation for HTML5
	-- On HTML5 screenshot will return as base64 encoded image in a callback
	--
	if screenshot.html5 then
		screenshot.html5(function(self, base64_img)
			-- share your base64 png
		end)
		-- Capture screenshots of a portion of the screen:
		-- screenshot.html5(x, y, width, height, callback)
	else
		-- IMPORTANT! The functionality below is NOT available on HTML5

		-- Take screenshot and encode to a PNG
		-- Write it to foo.png
		local png, w, h = screenshot.png()
		local f = io.open("screenshot1.png", "wb")
		f:write(png)
		f:flush()
		f:close()

		-- Take screenshot and return it as a Defold buffer
		-- Set buffer as texture on a model
		local buffer, w, h = screenshot.buffer()
		local url = go.get("cube#model", "texture0")
		resource.set_texture(url, { type = resource.TEXTURE_TYPE_2D, width = w, height = h, format = resource.TEXTURE_FORMAT_RGBA }, buffer)

		-- Take screenshot on POST_RENDER callback(https://github.com/britzl/defold-screenshot/issues/12)
		screenshot.callback_png(function(self,png,w,h)
			local f = io.open("screenshot1_cb.png", "wb")
			f:write(png)
			f:flush()
			f:close()
		end)

		-- Take screenshot and return pixels as a Lua string
		local pixels, w, h = screenshot.pixels()
		
		-- Capture screenshots of a portion of the screen
		screenshot.png(x, y, w, h)
		screenshot.buffer(x, y, w, h)
		screenshot.pixels(x, y, w, h)
		screenshot.callback_png(x, y, w, h, function(self,png,w,h) end)
	end

```

## Credits
* [LodePNG](http://lodev.org/lodepng/) for PNG encoding
* Graphics in example by [Kenney](http://www.kenney.nl)
