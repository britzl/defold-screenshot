// copyright britzl

#if !defined(DM_HEADLESS)

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

void getPng(unsigned char** out, size_t* outsize, unsigned int x, unsigned int y, unsigned int w, unsigned int h){

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
    lodepng_encode_memory(out, outsize, pixels, w, h, LCT_RGBA, 8);
    delete pixels;
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

    unsigned char* out = 0;
    size_t outsize = 0;

    getPng(&out,&outsize,x,y,w,h);
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

struct ScreenshotPostRenderLuaListener {
	ScreenshotPostRenderLuaListener() : m_L(0), m_Callback(LUA_NOREF), m_Self(LUA_NOREF),x(0),y(x),w(0),h(0) {}
	lua_State* m_L;
	int m_Callback;
	int m_Self;
	unsigned int x;
	unsigned int y;
	unsigned int w;
	unsigned int h;
};

static ScreenshotPostRenderLuaListener cbkPostRender;

static void UnregisterCallbackPostRender(lua_State* L){
	if(cbkPostRender.m_Callback != LUA_NOREF)
	{
		dmScript::Unref(cbkPostRender.m_L, LUA_REGISTRYINDEX, cbkPostRender.m_Callback);
		dmScript::Unref(cbkPostRender.m_L, LUA_REGISTRYINDEX, cbkPostRender.m_Self);
		cbkPostRender.m_Callback = LUA_NOREF;
	}
}

static dmExtension::Result PostRenderScreenshot(dmExtension::Params* params){
	if(cbkPostRender.m_Callback != LUA_NOREF){
		lua_State* L = cbkPostRender.m_L;
        int top = lua_gettop(L);
		lua_rawgeti(L, LUA_REGISTRYINDEX, cbkPostRender.m_Callback);
        //[-1] - callback
		lua_rawgeti(L, LUA_REGISTRYINDEX, cbkPostRender.m_Self);
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
            UnregisterCallbackPostRender(L);
            dmLogError("Could not run Screenshot postRenderCallback because the instance has been deleted.");
            lua_pop(L, 2);
            assert(top == lua_gettop(L));
        } else {
            unsigned char* out = 0;
            size_t outsize = 0;

            getPng(&out,&outsize,cbkPostRender.x,cbkPostRender.y,cbkPostRender.w,cbkPostRender.h);

            lua_pushlstring(L, (char*)out, outsize);
            lua_pushnumber(L, cbkPostRender.w);
            lua_pushnumber(L, cbkPostRender.h);

            // Assert that there is callback + self + vars
            assert(top +2 + 3 == lua_gettop(L));


            int ret = lua_pcall(L, 4, 0, 0);
            if(ret != 0) {
                dmLogError("Error running callback: %s", lua_tostring(L, -1));
                lua_pop(L, 5);
            }
            UnregisterCallbackPostRender(L);
        }
        assert(top == lua_gettop(L));
    }
    return dmExtension::RESULT_OK;
}

static int CallbackPng(lua_State* L) {
	int top = lua_gettop(L);
    int fn_num = 1;
    unsigned int x, y, w, h;
    if (top == 5) {
        x = luaL_checkint(L, 1);
        y = luaL_checkint(L, 2);
        w = luaL_checkint(L, 3);
        h = luaL_checkint(L, 4);
        fn_num = 5;
    }else{
    	GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        x = viewport[0];
        y = viewport[1];
        w = viewport[2];
        h = viewport[3];
    }

    cbkPostRender.m_L = dmScript::GetMainThread(L);
    luaL_checktype(L, fn_num, LUA_TFUNCTION);
    lua_pushvalue(L, fn_num);

    cbkPostRender.m_Callback = dmScript::Ref(L, LUA_REGISTRYINDEX);
    dmScript::GetInstance(L);
    cbkPostRender.m_Self = dmScript::Ref(L, LUA_REGISTRYINDEX);

    cbkPostRender.x = x;
    cbkPostRender.y = y;
    cbkPostRender.w = w;
    cbkPostRender.h = h;



	assert(top == lua_gettop(L));
	return 0;
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

	int fn_num = 1;
	unsigned int x, y, w, h;
	if (top == 5) {
		x = luaL_checkint(L, 1);
		y = luaL_checkint(L, 2);
		w = luaL_checkint(L, 3);
		h = luaL_checkint(L, 4);
		fn_num = 5;
	}

	cbk.m_L = dmScript::GetMainThread(L);
	luaL_checktype(L, fn_num, LUA_TFUNCTION);
	lua_pushvalue(L, fn_num);
	cbk.m_Callback = dmScript::Ref(L, LUA_REGISTRYINDEX);
	dmScript::GetInstance(L);
	cbk.m_Self = dmScript::Ref(L, LUA_REGISTRYINDEX);

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
	{"callback_png", CallbackPng},
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
        #if !(defined(__EMSCRIPTEN__))
        dmExtension::RegisterCallback(dmExtension::CALLBACK_POST_RENDER, PostRenderScreenshot );
        #endif

		return dmExtension::RESULT_OK;
}

dmExtension::Result InitializeScreenshotExtension(dmExtension::Params* params) {
		// Init Lua
		LuaInit(params->m_L);
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

#else

// dummy implementation
extern "C" void Screenshot() {}

#endif
