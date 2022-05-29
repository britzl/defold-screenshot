![](logo.png)

# Defold-Screenshot
Screenshot extension for the [Defold game engine](http://www.defold.com)

## Installation
You can use Defold-Screenshot in your own project by adding this project as a [Defold library dependency](http://www.defold.com/manuals/libraries/). Open your game.project file and in the dependencies field under project add:

	https://github.com/britzl/defold-screenshot/archive/master.zip

Or point to the ZIP file of a [specific release](https://github.com/britzl/defold-screenshot/releases).

## API
The screenshot API provides four versions of each function:

* Partial screenshot - synchronous. Not available on HTML5.
* Fullscreen screenshot - synchronous. Not available on HTML5.
* Partial screenshot - asynchronous
* Fullscreen screenshot - asynchronous

The asynchronous versions will capture the screenshot on POST RENDER. The synchronous versions will capture the screenshot immediately. On some mobile devices this may result in a blank capture.


### screenshot.png([x, y, w, h, [callback]])
Capture a full screen or partial screenshot as a PNG. Example:

```
-- fullscreen with callback
screenshot.png(function(self, image, w, h)
	-- do something with image
end)

-- partial with callback
-- 100x200 pixels starting from 0,0 (lower left corner)
screenshot.png(0, 0, 100, 200, function(self, image, w, h)
	-- do something with image
end)

-- fullscreen
local image, w, h = screenshot.png()

-- partial
-- 100x200 pixels starting from 0,0 (lower left corner)
local image, w, h = screenshot.png(0, 0, 100, 200)
```

### screenshot.buffer([x, y, w, h, [callback]])
Capture a full screen or partial screenshot as a Defold buffer.

```
-- Take screenshot and return it as a Defold buffer
-- Set buffer as texture on a model
local buffer, w, h = screenshot.buffer()
local url = go.get("cube#model", "texture0")
resource.set_texture(url, { type = resource.TEXTURE_TYPE_2D, width = w, height = h, format = resource.TEXTURE_FORMAT_RGBA }, buffer)
```



### screenshot.pixels([x, y, w, h, [callback]])
Capture a full screen or partial screenshot as raw pixels.

```
-- Take screenshot and return pixels as a Lua string
local pixels, w, h = screenshot.pixels()
```


## Credits
* [fpng](https://github.com/richgel999/fpng) for PNG encoding
* Graphics in example by [Kenney](http://www.kenney.nl)
