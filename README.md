# Defold-Screenshot
Screenshot extension for the [Defold game engine](http://www.defold.com)

## Installation
You can use Defold-Screenshot in your own project by adding this project as a [Defold library dependency](http://www.defold.com/manuals/libraries/). Open your game.project file and in the dependencies field under project add:

	https://github.com/britzl/defold-screenshot/archive/master.zip

Or point to the ZIP file of a [specific release](https://github.com/britzl/defold-screenshot/releases).

## Usage
The extensions will declare a new module, exposed to Lua as `screenshot`. The extension has three functions:

    -- Take screenshot and encode to a PNG
    -- Write it to foo.png
    local png = screenshot.png(x, y, w, h)
    local f = io.open("screenshot1.png", "wb")
    f:write(png)
    f:flush()
    f:close()
    
    -- Take screenshot and return it as a Defold buffer
    -- Set buffer as texture on a model
    local buffer = screenshot.buffer(x, y, w, h)
    local url = go.get("cube#model", "texture0")
    resource.set_texture(url, { type = resource.TEXTURE_TYPE_2D, width = w, height = h, format = resource.TEXTURE_FORMAT_RGBA }, buffer)
    
    -- Take screenshot and return pixels as a Lua string
    local pixels = screenshot.pixels(x, y, w, h)

## Credits
* [LodePNG](http://lodev.org/lodepng/) for PNG encoding
* Graphics in example by [Kenney](http://www.kenney.nl)
