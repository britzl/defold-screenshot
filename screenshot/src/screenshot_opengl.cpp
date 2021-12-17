// copyright britzl

#if !defined(__EMSCRIPTEN__)


#include <dmsdk/sdk.h>

#if defined(_WIN32)
	#include "./lodepng.h"
	#include <gl/GL.h>
#else
	#include "./lodepng.h"
	#include <GLES2/gl2.h>
	#include <GLES2/gl2ext.h>
#endif


static GLubyte* DoReadPixels(unsigned int x, unsigned int y, unsigned int w, unsigned int h) {
	GLubyte* data = new GLubyte[w * h * 4];
#if defined(__MACH__) && !( defined(__arm__) || defined(__arm64__) )
	glBindRenderbuffer(GL_RENDERBUFFER, 1);
#endif
	glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
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
	GLubyte* pixels = DoReadPixels(x, y, w, h);
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

	// encode to png
	unsigned char* out = 0;
	size_t outsize = 0;
	lodepng_encode_memory(&out, &outsize, pixels, w, h, LCT_RGBA, 8);
	delete pixels;

	// Put the pixel data onto the stack
	lua_pushlstring(L, (char*)out, outsize);
	lua_pushnumber(L, w);
	lua_pushnumber(L, h);
	return 3;
}

static int ReadPixels(lua_State* L, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
	DM_LUA_STACK_CHECK(L, 3);
	GLubyte* pixels = DoReadPixels(x, y, w, h);

	// Put the pixel data onto the stack
	lua_pushlstring(L, (char*)pixels, w * h);
	lua_pushnumber(L, w);
	lua_pushnumber(L, h);
	delete pixels;
	return 3;
}

static int ReadPixelsToBuffer(lua_State* L, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
	DM_LUA_STACK_CHECK(L, 3);
	GLubyte* pixels = DoReadPixels(x, y, w, h);

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
		delete pixels;

		// validate and return
		if (dmBuffer::ValidateBuffer(buffer) == dmBuffer::RESULT_OK) {
			dmScript::LuaHBuffer luabuffer{buffer, dmScript::OWNER_LUA};
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
		delete pixels;
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushnil(L);
	}
	return 3;
}


static int ScreenshotWithFormat(lua_State* L, ScreenshotFormat format)
{
	int top = lua_gettop(L);

	unsigned int x, y, w, h;
	if (top >= 4)
	{
		x = luaL_checkint(L, 1);
		y = luaL_checkint(L, 2);
		w = luaL_checkint(L, 3);
		h = luaL_checkint(L, 4);
	}
	else
	{
		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		x = viewport[0];
		y = viewport[1];
		w = viewport[2];
		h = viewport[3];
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
		g_Screenshot.format = Pixels;

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
	dmExtension::RegisterCallback(dmExtension::CALLBACK_POST_RENDER, PostRenderScreenshot );
}

#endif
