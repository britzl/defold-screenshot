#if !defined(__EMSCRIPTEN__) && defined(DM_DEBUG)

#include "screenshot_debug_webserver.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if defined(_WIN32)
#include <direct.h>
#endif

#include <dmsdk/dlib/time.h>
#include <dmsdk/engine/extension.hpp>

static const char* DEBUG_SCREENSHOT_ENDPOINT = "/screenshot";
static dmWebServer::HServer g_WebServer = 0;
static bool g_DebugEndpointRegistered = false;
static uint32_t g_DebugScreenshotCounter = 0;
static bool g_DebugScreenshotPending = false;
static char g_DebugScreenshotPath[1024];

static void NormalizeSlashes(char* path)
{
	while (*path)
	{
		if (*path == '\\')
		{
			*path = '/';
		}
		++path;
	}
}

#if defined(__ANDROID__)
static bool GetAndroidFilesDirectory(char* directory, uint32_t directory_size)
{
	FILE* file = fopen("/proc/self/cmdline", "rb");
	if (!file)
	{
		return false;
	}

	char package_name[256];
	size_t read = fread(package_name, 1, sizeof(package_name) - 1, file);
	fclose(file);

	if (read == 0)
	{
		return false;
	}

	package_name[read] = 0;
	package_name[sizeof(package_name) - 1] = 0;

	char* process_suffix = strchr(package_name, ':');
	if (process_suffix)
	{
		*process_suffix = 0;
	}

	if (!package_name[0] || strchr(package_name, '/'))
	{
		return false;
	}

	int written = dmSnPrintf(directory, directory_size, "/data/data/%s/files", package_name);
	return written >= 0 && (uint32_t)written < directory_size;
}
#endif

static const char* GetTempDirectory()
{
#if defined(_WIN32)
	const char* temp_dir = getenv("TEMP");
	if (!temp_dir || !temp_dir[0])
	{
		temp_dir = getenv("TMP");
	}
#else
	const char* temp_dir = getenv("TMPDIR");
#endif
	if (!temp_dir || !temp_dir[0])
	{
#if defined(_WIN32)
		temp_dir = ".";
#else
		temp_dir = "/tmp";
#endif
	}
	return temp_dir;
}

static bool GetWritableScreenshotBaseDirectory(char* directory, uint32_t directory_size)
{
#if defined(__ANDROID__)
	if (GetAndroidFilesDirectory(directory, directory_size))
	{
		NormalizeSlashes(directory);
		return true;
	}
#endif

	int written = dmSnPrintf(directory, directory_size, "%s", GetTempDirectory());
	if (written < 0 || (uint32_t)written >= directory_size)
	{
		return false;
	}

	NormalizeSlashes(directory);
	return true;
}

static bool EnsureDebugScreenshotDirectory(char* directory, uint32_t directory_size)
{
	char base_directory[1024];
	if (!GetWritableScreenshotBaseDirectory(base_directory, sizeof(base_directory)))
	{
		return false;
	}

	int written = dmSnPrintf(directory, directory_size, "%s/defold-screenshot", base_directory);
	if (written < 0 || (uint32_t)written >= directory_size)
	{
		return false;
	}

	NormalizeSlashes(directory);

#if defined(_WIN32)
	int result = _mkdir(directory);
#else
	int result = mkdir(directory, 0700);
#endif
	if (result != 0 && errno != EEXIST)
	{
		dmLogError("Unable to create screenshot directory '%s' (%d)", directory, errno);
		return false;
	}

	return true;
}

static bool BuildDebugScreenshotPath(char* path, uint32_t path_size)
{
	char directory[1024];
	if (!EnsureDebugScreenshotDirectory(directory, sizeof(directory)))
	{
		return false;
	}

	uint32_t timestamp = (uint32_t)(dmTime::GetTime() / 1000000);
	uint32_t counter = ++g_DebugScreenshotCounter;
	int written = dmSnPrintf(path, path_size, "%s/screenshot-%u-%u.png", directory, timestamp, counter);
	if (written < 0 || (uint32_t)written >= path_size)
	{
		return false;
	}

	NormalizeSlashes(path);
	return true;
}

static bool WriteBytesToFile(const char* path, const std::vector<uint8_t>& bytes)
{
	if (bytes.empty())
	{
		return false;
	}

	FILE* file = 0;
#if defined(_WIN32)
	if (fopen_s(&file, path, "wb") != 0)
	{
		file = 0;
	}
#else
	file = fopen(path, "wb");
#endif
	if (!file)
	{
		dmLogError("Unable to open screenshot file '%s' for writing", path);
		return false;
	}

	size_t written = fwrite(&bytes[0], 1, bytes.size(), file);
	bool close_ok = fclose(file) == 0;
	if (written != bytes.size() || !close_ok)
	{
		dmLogError("Unable to write screenshot file '%s'", path);
		return false;
	}

	return true;
}

static bool ScheduleDebugScreenshot(const char* path)
{
	if (g_DebugScreenshotPending)
	{
		return false;
	}

	int written = dmSnPrintf(g_DebugScreenshotPath, sizeof(g_DebugScreenshotPath), "%s", path);
	if (written < 0 || (uint32_t)written >= sizeof(g_DebugScreenshotPath))
	{
		return false;
	}

	g_DebugScreenshotPending = true;
	return true;
}

void ProcessPendingDebugScreenshot()
{
	if (!g_DebugScreenshotPending)
	{
		return;
	}

	char path[1024];
	int written = dmSnPrintf(path, sizeof(path), "%s", g_DebugScreenshotPath);
	if (written < 0 || (uint32_t)written >= sizeof(path))
	{
		g_DebugScreenshotPending = false;
		dmLogError("Unable to process pending screenshot path");
		return;
	}

	g_DebugScreenshotPending = false;

	std::vector<uint8_t> png;
	DebugScreenshotCaptureResult capture_result = Platform_CaptureDebugScreenshotToPngMemory(png);
	if (capture_result == DEBUG_SCREENSHOT_CAPTURE_RESULT_NO_GRAPHICS_CONTEXT)
	{
		dmLogError("Unable to capture debug screenshot: graphics context not initialized");
		return;
	}
	else if (capture_result != DEBUG_SCREENSHOT_CAPTURE_RESULT_OK)
	{
		dmLogError("Unable to capture debug screenshot");
		return;
	}

	if (!WriteBytesToFile(path, png))
	{
		dmLogError("Unable to write debug screenshot '%s'", path);
	}
}

static bool JsonEscape(const char* source, char* out, uint32_t out_size)
{
	uint32_t used = 0;
	while (*source)
	{
		const char* replacement = 0;
		switch (*source)
		{
		case '\\': replacement = "\\\\"; break;
		case '"': replacement = "\\\""; break;
		case '\n': replacement = "\\n"; break;
		case '\r': replacement = "\\r"; break;
		case '\t': replacement = "\\t"; break;
		default: break;
		}

		if (replacement)
		{
			uint32_t replacement_length = (uint32_t)strlen(replacement);
			if (used + replacement_length >= out_size)
			{
				return false;
			}
			memcpy(out + used, replacement, replacement_length);
			used += replacement_length;
		}
		else
		{
			if (used + 1 >= out_size)
			{
				return false;
			}
			out[used++] = *source;
		}
		++source;
	}

	out[used] = 0;
	return true;
}

static void SendResponse(dmWebServer::Request* request, int status_code, const char* content_type, const char* response)
{
	dmWebServer::SetStatusCode(request, status_code);
	dmWebServer::SendAttribute(request, "Content-Type", content_type);
	dmWebServer::SendAttribute(request, "Cache-Control", "no-store");
	dmWebServer::Send(request, response, strlen(response));
}

static void SendErrorResponse(dmWebServer::Request* request, int status_code, const char* message)
{
	SendResponse(request, status_code, "text/plain", message);
}

static bool IsCaptureMethod(const char* method)
{
	return method && (strcmp(method, "GET") == 0 || strcmp(method, "POST") == 0);
}

static bool DrainRequestBody(dmWebServer::Request* request)
{
	char buffer[256];
	uint32_t remaining = request->m_ContentLength;

	while (remaining > 0)
	{
		uint32_t received = 0;
		uint32_t to_read = remaining < sizeof(buffer) ? remaining : (uint32_t)sizeof(buffer);
		dmWebServer::Result result = dmWebServer::Receive(request, buffer, to_read, &received);
		if (result != dmWebServer::RESULT_OK || received == 0)
		{
			dmLogWarning("Unable to drain screenshot request body (%d)", result);
			return false;
		}
		remaining -= received;
	}

	return true;
}

static void DebugScreenshotHandler(void* user_data, dmWebServer::Request* request)
{
	(void)user_data;

	if (!IsCaptureMethod(request->m_Method))
	{
		DrainRequestBody(request);
		dmWebServer::SetStatusCode(request, 405);
		dmWebServer::SendAttribute(request, "Allow", "GET, POST");
		dmWebServer::SendAttribute(request, "Content-Type", "text/plain");
		dmWebServer::SendAttribute(request, "Cache-Control", "no-store");
		dmWebServer::Send(request, "method not allowed\n", strlen("method not allowed\n"));
		return;
	}

	if (request->m_ContentLength > 0)
	{
		DrainRequestBody(request);
		SendErrorResponse(request, 400, "request body not supported\n");
		return;
	}

	char path[1024];
	if (!BuildDebugScreenshotPath(path, sizeof(path)))
	{
		SendErrorResponse(request, 500, "failed to create screenshot path\n");
		return;
	}

	if (!ScheduleDebugScreenshot(path))
	{
		SendErrorResponse(request, 500, "failed to schedule screenshot\n");
		return;
	}

	char escaped_path[2048];
	if (!JsonEscape(path, escaped_path, sizeof(escaped_path)))
	{
		SendErrorResponse(request, 500, "failed to encode screenshot response\n");
		return;
	}

	char response[2304];
	int written = dmSnPrintf(response, sizeof(response), "{\"path\":\"%s\"}\n", escaped_path);
	if (written < 0 || (uint32_t)written >= sizeof(response))
	{
		SendErrorResponse(request, 500, "failed to create screenshot response\n");
		return;
	}

	SendResponse(request, 200, "application/json", response);
}

void RegisterDebugScreenshotEndpoint(dmExtension::AppParams* params)
{
	g_WebServer = dmEngine::GetWebServer(params);
	if (!g_WebServer)
	{
		return;
	}

	dmWebServer::HandlerParams handler_params;
	handler_params.m_Userdata = 0;
	handler_params.m_Handler = DebugScreenshotHandler;

	dmWebServer::Result result = dmWebServer::AddHandler(g_WebServer, DEBUG_SCREENSHOT_ENDPOINT, &handler_params);
	if (result == dmWebServer::RESULT_OK)
	{
		g_DebugEndpointRegistered = true;
		dmLogInfo("Screenshot debug endpoint registered at `%s` (https://127.0.0.1:engine_service_port%s)", DEBUG_SCREENSHOT_ENDPOINT, DEBUG_SCREENSHOT_ENDPOINT);
	}
	else if (result != dmWebServer::RESULT_HANDLER_ALREADY_REGISTRED)
	{
		dmLogWarning("Unable to register screenshot debug endpoint '%s' (%d)", DEBUG_SCREENSHOT_ENDPOINT, result);
	}
}

void UnregisterDebugScreenshotEndpoint()
{
	if (!g_WebServer || !g_DebugEndpointRegistered)
	{
		g_WebServer = 0;
		g_DebugEndpointRegistered = false;
		return;
	}

	dmWebServer::Result result = dmWebServer::RemoveHandler(g_WebServer, DEBUG_SCREENSHOT_ENDPOINT);
	if (result != dmWebServer::RESULT_OK && result != dmWebServer::RESULT_HANDLER_NOT_REGISTRED)
	{
		dmLogWarning("Unable to remove screenshot debug endpoint '%s' (%d)", DEBUG_SCREENSHOT_ENDPOINT, result);
	}

	g_WebServer = 0;
	g_DebugEndpointRegistered = false;
}

#endif
