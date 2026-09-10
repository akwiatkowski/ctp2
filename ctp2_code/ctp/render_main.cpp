#include "ctp/c3.h"

#include "ctp/civapp.h"
#include "ctp/civ3_main.h"
#include "ctp/crash_handler.h"
#include "ctp/ctp2_utils/appstrings.h"
#include "ctp/ctp2_utils/civlog.h"

#include <clocale>
#include <chrono>
#include <cstdio>
#include <thread>

#ifdef __AUI_USE_SDL__
#include "ui/aui_sdl/aui_sdlcompat.h"
#endif

extern BOOL gDone;
extern BOOL g_smokeTest;

void main_InitializeLogs();

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	crash_handler::Install("/tmp/ctp2-render-crash.log");
	civlog::Init();
	appstrings_Initialize();
	setlocale(LC_COLLATE, appstrings_GetString(APPSTR_LOCALE));

#if defined(_DEBUG) || defined(USE_LOGGING)
	main_InitializeLogs();
#endif

	g_smokeTest = TRUE;
	civapp_Set(new CivApp());

	if (civapp_Get()->InitializeApp(nullptr, 0) != 0) {
		std::fprintf(stderr, "ctp2_render: InitializeApp failed\n");
		return 1;
	}

	for (gDone = FALSE; !gDone; ) {
#ifdef __AUI_USE_SDL__
		SDL_PumpEvents();
		SDL_Event event;
		while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_QUIT, SDL_QUIT) > 0) {
			gDone = TRUE;
		}
#endif
		civapp_Get()->ProcessRenderTool();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	DoFinalCleanup(0);
	return 0;
}
