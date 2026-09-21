#include "ctp/c3.h"

#include "ctp/display.h"

#include "ctp/ctp2_utils/pointerlist.h"
#include "ctp/ctp2_utils/appstrings.h"
#include "gs/utility/Globals.h"                    // allocated::clear

#include <memory>

#define COMPILE_MULTIMON_STUBS
#include "ui/aui_sdl/aui_sdlcompat.h"

PointerList<CTPDisplayMode>	*g_displayModes = nullptr;
#ifdef WIN32
PointerList<DisplayDevice>	*g_displayDevices = NULL;

DisplayDevice				g_displayDevice;
#endif

extern LPCSTR				gszMainWindowClass;
extern LPCSTR				gszMainWindowName;
extern HINSTANCE			gHInstance;
extern HWND					gHwnd;
extern LRESULT CALLBACK		WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
extern sint32				g_ScreenWidth;
extern sint32				g_ScreenHeight;
extern BOOL					g_cmdlineResolutionSet;
extern BOOL					g_exclusiveMode;
extern BOOL					g_createDirectDrawOnSecondary;

#include "gs/database/profileDB.h"

#ifdef WIN32
BOOL CALLBACK display_FindDeviceCallbackEx(GUID* lpGUID, LPSTR szName,
								   LPSTR szDevice, LPVOID lParam, HMONITOR hMonitor)
{


	auto p = std::make_unique<DisplayDevice>();

	if (lpGUID)
	{
		p->DisplayGUID = *lpGUID;
		p->szName = szName;
		p->lpGUID = &p->DisplayGUID;
		p->szDevice = szDevice;
	}
	else
	{
		p->lpGUID = NULL;
	}

	p->hMon = hMonitor;

	g_displayDevices->AddTail(p.release());

	return TRUE;
}

BOOL display_EnumerateDisplayDevices(void)
{
	HRESULT hres;
	HMODULE hModule;
	LPDIRECTDRAWENUMERATEEX pfnEnum;

	g_displayDevices = std::make_unique<PointerList<DisplayDevice>>().release();

	memset(&g_displayDevice, 0, sizeof(DisplayDevice));

	hModule = GetModuleHandle("ddraw.dll");
	pfnEnum = (LPDIRECTDRAWENUMERATEEX)GetProcAddress(hModule, "DirectDrawEnumerateExA");

	if (pfnEnum != NULL)
	{


		hres = (*pfnEnum)(	display_FindDeviceCallbackEx,
							NULL,
							DDENUM_ATTACHEDSECONDARYDEVICES);
	}
	else
	{

		Assert(FALSE);
	}

	return TRUE;
}

HRESULT CALLBACK display_DisplayModeCallback(LPDDSURFACEDESC pdds, LPVOID lParam)
{
    sint32 width  = pdds->dwWidth;
    sint32 height = pdds->dwHeight;
    sint32 bpp    = pdds->ddpfPixelFormat.dwRGBBitCount;

	BOOL	legalSize = TRUE;

	if (width < 640 || height < 480)
		legalSize = FALSE;

	if (legalSize) {
		if (width < 1024 || height < 768) {
			if (
				!(width == 800 && height == 600))
				legalSize = FALSE;
		}
	}

	if (bpp == 16 && legalSize) {
	auto mode = std::make_unique<CTPDisplayMode>();
		mode->width = width;
		mode->height = height;

		g_displayModes->AddTail(mode.release());
	}

    return S_FALSE;
}
#endif

void display_EnumerateDisplayModes()
{
	g_displayModes = std::make_unique<PointerList<CTPDisplayMode>>().release();

	int numModes = 0;
	SDL_DisplayMode **sdlModes = SDL_GetFullscreenDisplayModes(
		SDL_GetPrimaryDisplay(), &numModes);
	if (!sdlModes) {
		numModes = -1;
	}
	if (numModes < 0) {
		// Fallback: pick common resolutions
		static const struct { sint32 w; sint32 h; } s_commonModes[] = {
			{ 800, 600 },
			{ 1024, 768 },
			{ 1280, 720 },
			{ 1280, 800 },
			{ 1366, 768 },
			{ 1440, 900 },
			{ 1600, 900 },
			{ 1680, 1050 },
			{ 1920, 1080 },
			{ 1920, 1200 },
			{ 2560, 1440 },
			{ 2560, 1600 },
			{ 3840, 2160 },
		};
		for (auto s_commonMode : s_commonModes) {
			auto mode = std::make_unique<CTPDisplayMode>();
			if (mode) {
				mode->width  = s_commonMode.w;
				mode->height = s_commonMode.h;
				g_displayModes->AddTail(mode.release());
			}
		}
		return;
	}

	for (int i = 0; i < numModes; i++) {
		SDL_DisplayMode sdlMode;
		if (!sdlModes[i])
			continue;
		sdlMode = *sdlModes[i];

		// Only consider modes with at least 16bpp equivalent
		if (SDL_BITSPERPIXEL(sdlMode.format) < 16)
			continue;

		// Filter: minimum 640x480, skip weird aspect ratios
		if (sdlMode.w < 640 || sdlMode.h < 480)
			continue;

		// Skip duplicate modes (check against already-added list)
		bool alreadyAdded = false;
		PointerList<CTPDisplayMode>::PointerListNode *node = g_displayModes->GetHeadNode();
		while (node) {
			CTPDisplayMode *existing = node->GetObj();
			if (existing->width == sdlMode.w && existing->height == sdlMode.h) {
				alreadyAdded = true;
				break;
			}
			node = node->GetNext();
		}
		if (alreadyAdded)
			continue;

		auto mode = std::make_unique<CTPDisplayMode>();
		if (!mode) {
			SDL_free(sdlModes);
			return;
		}
		mode->width = sdlMode.w;
		mode->height = sdlMode.h;
		g_displayModes->AddTail(mode.release());
	}
	SDL_free(sdlModes);

}


BOOL display_IsLegalResolution(sint32 width, sint32 height)
{

	PointerList<CTPDisplayMode>::PointerListNode	*node;

	node = g_displayModes->GetHeadNode();
	while (node) {
		CTPDisplayMode *mode = node->GetObj();
		if (mode->width == width && mode->height == height) {

			return TRUE;
		}
		node = node->GetNext();
	}
	return FALSE;
}


int display_Initialize(HINSTANCE hInstance, int iCmdShow)
{
	display_EnumerateDisplayModes();

	// If user specified --resolution, add it to the list and use it
	if (g_cmdlineResolutionSet && g_ScreenWidth > 0 && g_ScreenHeight > 0) {
		if (!display_IsLegalResolution(g_ScreenWidth, g_ScreenHeight)) {
			auto mode = std::make_unique<CTPDisplayMode>();
			if (mode) {
				mode->width = g_ScreenWidth;
				mode->height = g_ScreenHeight;
				g_displayModes->AddTail(mode.release());
			}
		}
	}

	BOOL foundRes = FALSE;

	if (g_cmdlineResolutionSet) {
		// Command-line resolution always takes precedence
		foundRes = display_IsLegalResolution(g_ScreenWidth, g_ScreenHeight);
	} else if (profiledb_Get()->IsTryWindowsResolution()) {
		if (display_IsLegalResolution(g_ScreenWidth, g_ScreenHeight))
			foundRes = TRUE;
	}

	if (!foundRes) {
		if (display_IsLegalResolution(profiledb_Get()->GetScreenResWidth(),
									profiledb_Get()->GetScreenResHeight())) {
			g_ScreenWidth = profiledb_Get()->GetScreenResWidth();
			g_ScreenHeight = profiledb_Get()->GetScreenResHeight();
		} else {

			CTPDisplayMode *mode = g_displayModes->GetHead();

			g_ScreenWidth = mode->width;
			g_ScreenHeight = mode->height;
		}
	}


	if (g_createDirectDrawOnSecondary) {
		g_ScreenWidth = 800;
		g_ScreenHeight = 600;
	}


	return 0;
}


void display_Cleanup()
{
	if(g_displayModes) {
		g_displayModes->DeleteAll();
		allocated::clear(g_displayModes);
	}
}
