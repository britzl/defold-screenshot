// copyright britzl

#if !defined(DM_HEADLESS)

// Extension lib defines
#define LIB_NAME "Screenshot"
#define MODULE_NAME "screenshot"

#include <dmsdk/sdk.h>
#include "screenshot.h"


// Functions exposed to Lua
static const luaL_reg Module_methods[] = {
#if defined(__EMSCRIPTEN__)
	{"html5", Platform_ScreenshotAsPng},
#endif
	{"pixels", Platform_ScreenshotAsPixels},
	{"png", Platform_ScreenshotAsPng},
	{"buffer", Platform_ScreenshotAsBuffer},
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
	Platform_AppInitializeScreenshotExtension(params);
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
