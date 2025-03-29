// copyright britzl

#if !defined(__EMSCRIPTEN__)

#include <dmsdk/sdk.h>
#include "fpng.h"

static dmGraphics::HContext g_GraphicsContext = NULL;

static void* DoReadPixels(unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
	uint32_t size = w * h * 4;
	void* data = malloc(size);
	// ReadPixels return data in BGRA format
	dmGraphics::ReadPixels(g_GraphicsContext, x, y, w, h, data, size);

	// convert BGRA to RGBA
	uint32_t* pixels = (uint32_t*)data;
	for (uint32_t i = 0; i < size / 4; ++i)
	{
		uint32_t pixel = pixels[i];
		uint32_t r = (pixel & 0x000000ff) << 16;
		uint32_t g = (pixel & 0x0000ff00);
		uint32_t b = (pixel & 0x00ff0000) >> 16;
		uint32_t a = (pixel & 0xff000000);
		pixels[i] = r | g | b | a;
	}

	return data;
}

enum ScreenshotFormat
{
	Png,
	Buffer,
	Pixels
};

struct Screenshot
{
	Screenshot()
	{
		memset(this, 0, sizeof(*this));
	}
	dmScript::LuaCallbackInfo* m_Callback;
	unsigned int x;
	unsigned int y;
	unsigned int w;
	unsigned int h;
	ScreenshotFormat format;
};

static Screenshot g_Screenshot;

static int ReadPixelsToPng(lua_State* L, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
	DM_LUA_STACK_CHECK(L, 3);
	void* pixels = DoReadPixels(x, y, w, h);

	// encode to png
	std::vector<uint8_t> out;
	bool result = fpng::fpng_encode_image_to_memory(pixels, w, h, 4, out);
	free(pixels);

	// Put the pixel data onto the stack
	if (result)
	{
		lua_pushlstring(L, (char*)out.data(), out.size());
		lua_pushnumber(L, w);
		lua_pushnumber(L, h);
	}
	else {
		dmLogError("fpng_encode_image_to_memory failed");
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushnil(L);
	}
	return 3;
}

static int ReadPixels(lua_State* L, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
	DM_LUA_STACK_CHECK(L, 3);
	void* pixels = DoReadPixels(x, y, w, h);

	// Put the pixel data onto the stack
	lua_pushlstring(L, (char*)pixels, w * h * 4);
	lua_pushnumber(L, w);
	lua_pushnumber(L, h);
	free(pixels);
	return 3;
}

static int ReadPixelsToBuffer(lua_State* L, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
	DM_LUA_STACK_CHECK(L, 3);
	void* pixels = DoReadPixels(x, y, w, h);

	// create buffer
	dmBuffer::HBuffer buffer;
	dmBuffer::StreamDeclaration streams_decl[] = {
		{ dmHashString64("pixels"), dmBuffer::VALUE_TYPE_UINT8, 1 }
	};
	dmBuffer::Result r = dmBuffer::Create(w * h * 4, streams_decl, 1, &buffer);
	if (r == dmBuffer::RESULT_OK) {
		// copy pixels into buffer
		uint8_t* data = 0;
		uint32_t datasize = 0;
		dmBuffer::GetBytes(buffer, (void**)&data, &datasize);
		memcpy(data, pixels, datasize);
		free(pixels);

		// validate and return
		if (dmBuffer::ValidateBuffer(buffer) == dmBuffer::RESULT_OK) {
			dmScript::LuaHBuffer luabuffer(buffer, dmScript::OWNER_LUA);
			dmScript::PushBuffer(L, luabuffer);
			lua_pushnumber(L, w);
			lua_pushnumber(L, h);
		}
		else {
			lua_pushnil(L);
			lua_pushnil(L);
			lua_pushnil(L);
		}
	}
	// buffer creation failed
	else {
		free(pixels);
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushnil(L);
	}
	return 3;
}


static int ScreenshotWithFormat(lua_State* L, ScreenshotFormat format)
{
	int top = lua_gettop(L);

	int32_t x, y;
	uint32_t w, h;
	if (top >= 4)
	{
		x = luaL_checkint(L, 1);
		y = luaL_checkint(L, 2);
		w = luaL_checkint(L, 3);
		h = luaL_checkint(L, 4);
	}
	else
	{
		dmGraphics::GetViewport(g_GraphicsContext, &x, &y, &w, &h);
	}

	// callback?
	if (top == 1 || top == 5)
	{
		if (g_Screenshot.m_Callback)
		{
			dmScript::DestroyCallback(g_Screenshot.m_Callback);
		}
		g_Screenshot.m_Callback = dmScript::CreateCallback(L, top);
		g_Screenshot.x = x;
		g_Screenshot.y = y;
		g_Screenshot.w = w;
		g_Screenshot.h = h;
		g_Screenshot.format = format;

		assert(top == lua_gettop(L));
		return 0;
	}
	else if (format == Pixels)
	{
		return ReadPixels(L, x, y, w, h);
	}
	else if (format == Png)
	{
		return ReadPixelsToPng(L, x, y, w, h);
	}
	else if (format == Buffer)
	{
		return ReadPixelsToBuffer(L, x, y, w, h);
	}
	dmLogError("Unknown format");
	return 0;
}

int Platform_ScreenshotAsPixels(lua_State* L) {
	return ScreenshotWithFormat(L, Pixels);
}

int Platform_ScreenshotAsPng(lua_State* L) {
	return ScreenshotWithFormat(L, Png);
}

int Platform_ScreenshotAsBuffer(lua_State* L) {
	return ScreenshotWithFormat(L, Buffer);
}

static dmExtension::Result PostRenderScreenshot(dmExtension::Params* params) {
	if (g_Screenshot.m_Callback == 0)
	{
		return dmExtension::RESULT_OK;
	}

	lua_State* L = dmScript::GetCallbackLuaContext(g_Screenshot.m_Callback);

	if (!dmScript::IsCallbackValid(g_Screenshot.m_Callback))
	{
		g_Screenshot.m_Callback = 0;
		return dmExtension::RESULT_OK;
	}

	if (!dmScript::SetupCallback(g_Screenshot.m_Callback))
	{
		dmScript::DestroyCallback(g_Screenshot.m_Callback);
		g_Screenshot.m_Callback = 0;
		return dmExtension::RESULT_OK;
	}

	int num_values = 0;
	if (g_Screenshot.format == Pixels)
	{
		num_values = ReadPixels(L, g_Screenshot.x, g_Screenshot.y, g_Screenshot.w, g_Screenshot.h);
	}
	else if (g_Screenshot.format == Png)
	{
		num_values = ReadPixelsToPng(L, g_Screenshot.x, g_Screenshot.y, g_Screenshot.w, g_Screenshot.h);
	}
	else if (g_Screenshot.format == Buffer)
	{
		num_values = ReadPixelsToBuffer(L, g_Screenshot.x, g_Screenshot.y, g_Screenshot.w, g_Screenshot.h);
	}

	dmScript::PCall(L, num_values + 1, 0);

	dmScript::TeardownCallback(g_Screenshot.m_Callback);
	dmScript::DestroyCallback(g_Screenshot.m_Callback);
	g_Screenshot.m_Callback = 0;

	return dmExtension::RESULT_OK;
}

void Platform_AppInitializeScreenshotExtension(dmExtension::AppParams* params)
{
	fpng::fpng_init();

	dmExtension::RegisterCallback(dmExtension::CALLBACK_POST_RENDER, (FExtensionCallback)PostRenderScreenshot);
}

void Platform_InitializeScreenshotExtension(dmExtension::Params* params)
{
	g_GraphicsContext = ExtensionParamsGetContextByName(params, "graphics");
	assert(g_GraphicsContext != NULL);
}

#endif
