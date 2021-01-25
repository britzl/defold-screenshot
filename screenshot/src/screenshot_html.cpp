// copyright britzl
#include <dmsdk/sdk.h>

#if defined(__EMSCRIPTEN__)

struct Screenshot
{
	Screenshot()
	{
		memset(this, 0, sizeof(*this));
	}
	dmScript::LuaCallbackInfo* m_Callback;
};

static Screenshot g_Screenshot;

typedef void (*JsCallback)(const char* base64image);

extern "C" void screenshot_on_the_next_frame(JsCallback callback, int x, int y, int w, int h);


static void JsToCCallback(const char* base64image)
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

	lua_pushstring(L, base64image);
	dmScript::PCall(L, 1 + 1, 0);

	dmScript::TeardownCallback(g_Screenshot.m_Callback);
	dmScript::DestroyCallback(g_Screenshot.m_Callback);
	g_Screenshot.m_Callback = 0;
}

int Platform_ScreenshotAsPng(lua_State* L) {
	DM_LUA_STACK_CHECK(L, 0);
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

	if (g_Screenshot.m_Callback)
	{
		dmScript::DestroyCallback(g_Screenshot.m_Callback);
	}
	g_Screenshot.m_Callback = dmScript::CreateCallback(L, fn_num);
	screenshot_on_the_next_frame(JsToCCallback, x, y, w, h);
	return 0;
}

int Platform_ScreenshotAsPixels(lua_State* L)
{
	luaL_error(L, "Capturing screenshot as pixels is not supported on HTML5");
}

int Platform_ScreenshotAsBuffer(lua_State* L)
{
	luaL_error(L, "Capturing screenshot as buffer is not supported on HTML5");
}

void Platform_AppInitializeScreenshotExtension(dmExtension::AppParams* params) { }



#endif
