// copyright britzl
// Extension lib defines
#define LIB_NAME "Screenshot"
#define MODULE_NAME "screenshot"

#include <dmsdk/sdk.h>

#if defined(_WIN32)
	#include "./lodepng.h"
	#include <gl/GL.h>
#elif defined(__EMSCRIPTEN__)
#else
	#include "./lodepng.h"
	#include <GLES2/gl2.h>
	#include <GLES2/gl2ext.h>
#endif

#if !defined(__EMSCRIPTEN__)

static GLubyte* ReadPixels(unsigned int x, unsigned int y, unsigned int w, unsigned int h) {
	GLubyte* data = new GLubyte[w * h * 4];
#if defined(__MACH__) && !( defined(__arm__) || defined(__arm64__) )
	glBindRenderbuffer(GL_RENDERBUFFER, 1);
#endif
	glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
	return data;
}


static int Pixels(lua_State* L) {
	int top = lua_gettop(L);

	unsigned int x, y, w, h;
	if (top == 4) {
		x = luaL_checkint(L, 1);
		y = luaL_checkint(L, 2);
		w = luaL_checkint(L, 3);
		h = luaL_checkint(L, 4);
	} else {
		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		x = viewport[0];
		y = viewport[1];
		w = viewport[2];
		h = viewport[3];
	}

	// read pixels
	GLubyte* pixels = ReadPixels(x, y, w, h);

	// Put the pixel data onto the stack
	lua_pushlstring(L, (char*)pixels, w * h);
	lua_pushnumber(L, w);
	lua_pushnumber(L, h);
	delete pixels;

	// Assert that there is three items on the stack.
	assert(top + 3 == lua_gettop(L));

	// Return 3 items
	return 3;
}

static int Png(lua_State* L) {
	int top = lua_gettop(L);

	// read pixels
	unsigned int x, y, w, h;
	if (top == 4) {
		x = luaL_checkint(L, 1);
		y = luaL_checkint(L, 2);
		w = luaL_checkint(L, 3);
		h = luaL_checkint(L, 4);
	} else {
		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		x = viewport[0];
		y = viewport[1];
		w = viewport[2];
		h = viewport[3];
	}
	GLubyte* pixels = ReadPixels(x, y, w, h);
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

	// Assert that there is three items on the stack.
	assert(top + 3 == lua_gettop(L));

	// Return 3 items
	return 3;
}

static int Buffer(lua_State* L) {
	int top = lua_gettop(L);

	// read pixels
	unsigned int x, y, w, h;
	if (top == 4) {
		x = luaL_checkint(L, 1);
		y = luaL_checkint(L, 2);
		w = luaL_checkint(L, 3);
		h = luaL_checkint(L, 4);
	} else {
		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		x = viewport[0];
		y = viewport[1];
		w = viewport[2];
		h = viewport[3];
	}
	GLubyte* pixels = ReadPixels(x, y, w, h);

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
			dmScript::LuaHBuffer luabuffer = { buffer, true };
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

	// Assert that there is three items on the stack.
	assert(top + 3 == lua_gettop(L));

	// Return 3 items
	return 3;
}
#else

struct ScreenshotLuaListener {
	ScreenshotLuaListener() : m_L(0), m_Callback(LUA_NOREF), m_Self(LUA_NOREF) {}
	lua_State* m_L;
	int m_Callback;
	int m_Self;
};

typedef void (*JsCallback)(const char* base64image);

extern "C" void screenshot_on_the_next_frame(JsCallback callback, int x, int y, int w, int h);

static ScreenshotLuaListener cbk;

static void UnregisterCallback(lua_State* L)
{
	if(cbk.m_Callback != LUA_NOREF)
	{
		dmScript::Unref(cbk.m_L, LUA_REGISTRYINDEX, cbk.m_Callback);
		dmScript::Unref(cbk.m_L, LUA_REGISTRYINDEX, cbk.m_Self);
		cbk.m_Callback = LUA_NOREF;
	}
}

static void JsToCCallback(const char* base64image)
{
	if(cbk.m_Callback == LUA_NOREF)
	{
		dmLogInfo("Callback do not exist.");
		return;
	}
	lua_State* L = cbk.m_L;
	int top = lua_gettop(L);
	lua_rawgeti(L, LUA_REGISTRYINDEX, cbk.m_Callback);
	//[-1] - callback
	lua_rawgeti(L, LUA_REGISTRYINDEX, cbk.m_Self);
	//[-1] - self
	//[-2] - callback
	lua_pushvalue(L, -1);
	//[-1] - self
	//[-2] - self
	//[-3] - callback
	dmScript::SetInstance(L);
	//[-1] - self
	//[-2] - callback

	if (!dmScript::IsInstanceValid(L)) {
		UnregisterCallback(L);
		dmLogError("Could not run Screenshot callback because the instance has been deleted.");
		lua_pop(L, 2);
		assert(top == lua_gettop(L));
	} else {
		lua_pushstring(L, base64image);
		int ret = lua_pcall(L, 2, 0, 0);
		if(ret != 0) {
			dmLogError("Error running callback: %s", lua_tostring(L, -1));
			lua_pop(L, 1);
		}
		UnregisterCallback(L);
	}
	assert(top == lua_gettop(L));
}

static int HTML5_screenshot(lua_State* L) {
	int top = lua_gettop(L);
	
	cbk.m_L = dmScript::GetMainThread(L);
	luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_pushvalue(L, 1);
	cbk.m_Callback = dmScript::Ref(L, LUA_REGISTRYINDEX);
	dmScript::GetInstance(L);
	cbk.m_Self = dmScript::Ref(L, LUA_REGISTRYINDEX);

	unsigned int x, y, w, h;
	if (top == 5) {
		x = luaL_checkint(L, 2);
		y = luaL_checkint(L, 3);
		w = luaL_checkint(L, 4);
		h = luaL_checkint(L, 5);
	}
	
	screenshot_on_the_next_frame(JsToCCallback, x, y, w, h);
	
	assert(top == lua_gettop(L));
	return 0;
}
#endif

// Functions exposed to Lua
static const luaL_reg Module_methods[] = {
#if defined(__EMSCRIPTEN__)
	{"html5", HTML5_screenshot},
#else
	{"pixels", Pixels},
	{"png", Png},
	{"buffer", Buffer},
#endif
	{0, 0}
};

static void LuaInit(lua_State* L) {
		int top = lua_gettop(L);

		// Register lua names
		luaL_register(L, MODULE_NAME, Module_methods);

		lua_pop(L, 1);
		assert(top == lua_gettop(L));
}

dmExtension::Result AppInitializeScreenshotExtension(dmExtension::AppParams* params) {
		return dmExtension::RESULT_OK;
}

dmExtension::Result InitializeScreenshotExtension(dmExtension::Params* params) {
		// Init Lua
		LuaInit(params->m_L);
		printf("Registered %s Extension\n", MODULE_NAME);
		return dmExtension::RESULT_OK;
}

dmExtension::Result AppFinalizeScreenshotExtension(dmExtension::AppParams* params) {
		return dmExtension::RESULT_OK;
}

dmExtension::Result FinalizeScreenshotExtension(dmExtension::Params* params) {
		return dmExtension::RESULT_OK;
}


// Defold SDK uses a macro for setting up extension entry points:
//
// DM_DECLARE_EXTENSION(symbol, name, app_init, app_final, init, update, on_event, final)

DM_DECLARE_EXTENSION(Screenshot, LIB_NAME, AppInitializeScreenshotExtension, AppFinalizeScreenshotExtension, InitializeScreenshotExtension, 0, 0, FinalizeScreenshotExtension)
