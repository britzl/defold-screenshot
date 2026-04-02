#pragma once
#include <dmsdk/sdk.h>

int Platform_ScreenshotAsPixels(lua_State* L);

int Platform_ScreenshotAsPng(lua_State* L);

int Platform_ScreenshotAsBuffer(lua_State* L);

void Platform_AppInitializeScreenshotExtension(dmExtension::AppParams* params);

void Platform_InitializeScreenshotExtension(dmExtension::Params* params);
