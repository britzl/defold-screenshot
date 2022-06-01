// copyright britzl

#if defined(__EMSCRIPTEN__)


#include <dmsdk/sdk.h>
#include "fpng.h"

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
	ScreenshotFormat format;
};

static Screenshot g_Screenshot;

typedef void (*JsCallback)(const char* image, const int w, const int h);

extern "C" void screenshot_on_the_next_frame(JsCallback callback, int x, int y, int w, int h);

static void FlipRawImage(const char *pixels, unsigned int w, unsigned int h)
{
	unsigned int *p = (unsigned int*)pixels;

	// flip vertically
	for (int yi=0; yi < (h / 2); yi++) {
		for (int xi=0; xi < w; xi++) {
			unsigned int offset1 = xi + (yi * w);
			unsigned int offset2 = xi + ((h - 1 - yi) * w);
			unsigned int pixel1 = p[offset1];
			unsigned int pixel2 = p[offset2];
			p[offset1] = pixel2;
			p[offset2] = pixel1;
		}
	}
}

static int RawImageToPng(lua_State* L, const char *pixels, unsigned int w, unsigned int h)
{
	DM_LUA_STACK_CHECK(L, 3);
	unsigned int *p = (unsigned int*)pixels;

	// encode to png
	std::vector<uint8_t> out;
	bool result = fpng::fpng_encode_image_to_memory(pixels, w, h, 4, out);

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

static int RawImageToPixels(lua_State* L, const char *pixels, unsigned int w, unsigned int h)
{
	DM_LUA_STACK_CHECK(L, 3);

	FlipRawImage(pixels, w, h);

	// Put the pixel data onto the stack
	lua_pushlstring(L, (char*)pixels, w * h * 4);
	lua_pushnumber(L, w);
	lua_pushnumber(L, h);

	return 3;
}

static int RawImageToBuffer(lua_State* L, const char *pixels, unsigned int w, unsigned int h)
{
	DM_LUA_STACK_CHECK(L, 3);

	FlipRawImage(pixels, w, h);

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
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushnil(L);
	}
	return 3;
}

static void JsToCCallback(const char* image, const int w, const int h)
{
	if (g_Screenshot.m_Callback == 0)
	{
		return;
	}

	lua_State* L = dmScript::GetCallbackLuaContext(g_Screenshot.m_Callback);

	if (!dmScript::IsCallbackValid(g_Screenshot.m_Callback))
	{
		g_Screenshot.m_Callback = 0;
		return;
	}

	if (!dmScript::SetupCallback(g_Screenshot.m_Callback))
	{
		dmScript::DestroyCallback(g_Screenshot.m_Callback);
		g_Screenshot.m_Callback = 0;
		return;
	}

	int num_values = 0;
	if (g_Screenshot.format == Pixels)
	{
		num_values = RawImageToPixels(L, image, w, h);
	}
	else if (g_Screenshot.format == Png)
	{
		num_values = RawImageToPng(L, image, w, h);
	}
	else if (g_Screenshot.format == Buffer)
	{
		num_values = RawImageToBuffer(L, image, w, h);
	}
	dmScript::PCall(L, 1 + num_values, 0);

	dmScript::TeardownCallback(g_Screenshot.m_Callback);
	dmScript::DestroyCallback(g_Screenshot.m_Callback);
	g_Screenshot.m_Callback = 0;
}

static int ScreenshotWithFormat(lua_State* L, ScreenshotFormat format)
{
	DM_LUA_STACK_CHECK(L, 0);
	int top = lua_gettop(L);

	int fn_num = 1;
	unsigned int x = 0, y = 0, w = 0, h = 0;
	if (top == 5) {
		x = luaL_checkint(L, 1);
		y = luaL_checkint(L, 2);
		w = luaL_checkint(L, 3);
		h = luaL_checkint(L, 4);
		fn_num = 5;
	}

	if (g_Screenshot.m_Callback)
	{
		dmScript::DestroyCallback(g_Screenshot.m_Callback);
	}
	g_Screenshot.m_Callback = dmScript::CreateCallback(L, fn_num);
	g_Screenshot.format = format;
	screenshot_on_the_next_frame(JsToCCallback, x, y, w, h);
	return 0;
}

int Platform_ScreenshotAsPng(lua_State* L)
{
	return ScreenshotWithFormat(L, Png);
}

int Platform_ScreenshotAsPixels(lua_State* L)
{
	return ScreenshotWithFormat(L, Pixels);
}

int Platform_ScreenshotAsBuffer(lua_State* L)
{
	return ScreenshotWithFormat(L, Buffer);
}

void Platform_AppInitializeScreenshotExtension(dmExtension::AppParams* params) { }



#endif
