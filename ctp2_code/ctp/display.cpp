#include "ctp/c3.h"

#include "ctp/display.h"

#include "ctp/ctp2_utils/pointerlist.h"
#include "ctp/ctp2_utils/appstrings.h"

#define COMPILE_MULTIMON_STUBS
#ifdef __AUI_USE_DIRECTX__
#include <multimon.h>
#elif defined(__AUI_USE_SDL__)
#include <SDL2/SDL.h>
#endif

PointerList<CTPDisplayMode>	*g_displayModes = NULL;
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


	DisplayDevice *p = new DisplayDevice;

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

	g_displayDevices->AddTail(p);

	return TRUE;
}

BOOL display_EnumerateDisplayDevices(void)
{
	HRESULT hres;
	HMODULE hModule;
	LPDIRECTDRAWENUMERATEEX pfnEnum;

	g_displayDevices = new PointerList<DisplayDevice>;

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
		CTPDisplayMode *mode = new CTPDisplayMode;
		mode->width = width;
		mode->height = height;

		g_displayModes->AddTail(mode);
	}

    return S_FALSE;
}
#endif

void display_EnumerateDisplayModes()
{
#ifdef __AUI_USE_DIRECTX__
	HRESULT				hr;
	LPDIRECTDRAW		dd;

	hr = DirectDrawCreate(g_displayDevice.lpGUID, &dd, NULL);
	Assert(hr == DD_OK);
	if (hr != DD_OK) {
		c3errors_FatalDialog(appstrings_GetString(APPSTR_DIRECTX),
								appstrings_GetString(APPSTR_REINSTALLDIRECTX));
		return;
	}

	g_displayModes = new PointerList<CTPDisplayMode>;
#else
	g_displayModes = new PointerList<CTPDisplayMode>;

	int numModes = SDL_GetNumDisplayModes(0);
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
		for (size_t i = 0; i < sizeof(s_commonModes)/sizeof(s_commonModes[0]); i++) {
			CTPDisplayMode *mode = new CTPDisplayMode;
			if (mode) {
				mode->width  = s_commonModes[i].w;
				mode->height = s_commonModes[i].h;
				g_displayModes->AddTail(mode);
			}
		}
		return;
	}

	for (int i = 0; i < numModes; i++) {
		SDL_DisplayMode sdlMode;
		if (SDL_GetDisplayMode(0, i, &sdlMode) != 0)
			continue;

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

		CTPDisplayMode *mode = new CTPDisplayMode;
		if (!mode)
			return;
		mode->width = sdlMode.w;
		mode->height = sdlMode.h;
		g_displayModes->AddTail(mode);
	}
#endif

#ifdef __AUI_USE_DIRECTX__
	hr = dd->EnumDisplayModes(0, NULL, NULL, display_DisplayModeCallback);

	if (g_displayModes->GetCount() < 1) {
		c3errors_FatalDialog(appstrings_GetString(APPSTR_DIRECTX),
							appstrings_GetString(APPSTR_NO16BIT));

		return;
	}

	dd->Release();
#endif
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

#ifdef __AUI_USE_DIRECTX__
BOOL display_InitWindow( HINSTANCE hinst, int cmdshow )
{
	WNDCLASS wc;

	gHInstance = hinst;

	wc.style = CS_DBLCLKS;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hinst;
	wc.hIcon = LoadIcon(hinst, MAKEINTATOM(IDI_APPLICATION));
	wc.hCursor = NULL;
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = gszMainWindowClass;

	if (!RegisterClass(&wc)) return FALSE;

	DWORD	exStyle;

	if (g_exclusiveMode)
		exStyle = WS_EX_TOPMOST;
	else
		exStyle = WS_EX_APPWINDOW;




	HWND hwnd = FindWindow(gszMainWindowClass, gszMainWindowName);
	if(hwnd) {

		if (IsIconic(hwnd)) {
			ShowWindow(hwnd, SW_RESTORE);
		}
		SetForegroundWindow (hwnd);

		exit(0);
	}

	gHwnd = CreateWindowEx(
		exStyle,
		gszMainWindowClass,
		gszMainWindowName,
		WS_POPUP | WS_VISIBLE,
		0,
		0,
		g_ScreenWidth,
		g_ScreenHeight,
		NULL,
		NULL,
		hinst,
		NULL);

	Assert(gHwnd != NULL);
	if (gHwnd == NULL) return FALSE;

    SetFocus(gHwnd);
	ShowWindow(gHwnd, cmdshow);
    UpdateWindow(gHwnd);

	return TRUE;
}
#endif

int display_Initialize(HINSTANCE hInstance, int iCmdShow)
{
#ifdef __AUI_USE_DIRECTX__
	display_EnumerateDisplayDevices();

	DisplayDevice	*device = g_displayDevices->GetTail();

	if (g_createDirectDrawOnSecondary && device != NULL) {

		g_displayDevice = *device;

		MONITORINFO	*monInfo = new MONITORINFO;

		monInfo->cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(device->hMon, monInfo);

		g_ScreenWidth = monInfo->rcMonitor.right - monInfo->rcMonitor.left;
		g_ScreenHeight = monInfo->rcMonitor.bottom - monInfo->rcMonitor.top;

		g_displayDevice.rect = monInfo->rcMonitor;

		delete monInfo;
	} else {

		g_ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
		g_ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

		SetRect(&g_displayDevice.rect, 0, 0, g_ScreenWidth, g_ScreenHeight);
	}
#endif
	display_EnumerateDisplayModes();

	// If user specified --resolution, add it to the list and use it
	if (g_cmdlineResolutionSet && g_ScreenWidth > 0 && g_ScreenHeight > 0) {
		if (!display_IsLegalResolution(g_ScreenWidth, g_ScreenHeight)) {
			CTPDisplayMode *mode = new CTPDisplayMode;
			if (mode) {
				mode->width = g_ScreenWidth;
				mode->height = g_ScreenHeight;
				g_displayModes->AddTail(mode);
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

#ifdef __AUI_USE_DIRECTX__
	display_InitWindow(hInstance, iCmdShow);
#endif

	return 0;
}


void display_Cleanup()
{
#ifdef __AUI_USE_DIRECTX__
	if(g_displayDevices) {
		g_displayDevices->DeleteAll();
		delete g_displayDevices;
		g_displayDevices = NULL;
	}
#endif
	if(g_displayModes) {
		g_displayModes->DeleteAll();
		delete g_displayModes;
		g_displayModes = NULL;
	}
}
