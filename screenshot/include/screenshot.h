#pragma once

#include <dmsdk/sdk.h>

#if defined(DM_PLATFORM_HTML5)

typedef void (*JsCallback)(const char* base64image);

extern "C" {
	void screenshot_on_the_next_frame(JsCallback callback);
}

struct ScreenshotLuaListener {
ScreenshotLuaListener() : m_L(0), m_Callback(LUA_NOREF), m_Self(LUA_NOREF) {}
	lua_State* m_L;
	int m_Callback;
	int m_Self;
};

void UnregisterCallback(lua_State* L);

#endif