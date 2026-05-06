#pragma once

#if !defined(__EMSCRIPTEN__) && defined(DM_DEBUG)

#include <vector>

#include <dmsdk/sdk.h>

enum DebugScreenshotCaptureResult
{
	DEBUG_SCREENSHOT_CAPTURE_RESULT_OK,
	DEBUG_SCREENSHOT_CAPTURE_RESULT_NO_GRAPHICS_CONTEXT,
	DEBUG_SCREENSHOT_CAPTURE_RESULT_ENCODE_FAILED
};

DebugScreenshotCaptureResult Platform_CaptureDebugScreenshotToPngMemory(std::vector<uint8_t>& out);

void RegisterDebugScreenshotEndpoint(dmExtension::AppParams* params);

void UnregisterDebugScreenshotEndpoint();

void ProcessPendingDebugScreenshot();

#endif
