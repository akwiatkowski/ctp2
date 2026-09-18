//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : SDL user interface handling
//
//----------------------------------------------------------------------------
//
// Disclaimer
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
// This material has been developed at apolyton.net by the Apolyton CtP2
// Source Code Project. Contact the authors at ctp2source@apolyton.net.
//
//----------------------------------------------------------------------------
//
// Compiler flags
//
// __AUI_USE_SDL__
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Prevented crashes
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#ifdef __AUI_USE_SDL__

#include "ctp/civ3_main.h"

#include "ui/aui_common/aui_mouse.h"
#include "ui/aui_common/aui_keyboard.h"
#include "ui/aui_common/aui_joystick.h"
#include "ui/aui_sdl/aui_sdlsurface.h"
#include "ui/aui_sdl/aui_sdlmouse.h"
#include "ui/aui_sdl/aui_sdlcompat.h" // CTP2_SDL_Start/StopTextInput

#include "ui/aui_sdl/aui_sdlui.h"

extern BOOL			g_exclusiveMode;

#include "ctp/civapp.h"

#include "display.h"

extern BOOL					g_createDirectDrawOnSecondary;
extern sint32				g_ScreenWidth;
extern sint32				g_ScreenHeight;
extern DisplayDevice		g_displayDevice;

BOOL g_SDL_flags = FALSE;

aui_SDLUI::aui_SDLUI
(
	AUI_ERRCODE *retval,
	HINSTANCE hinst,
	HWND hwnd,
	sint32 width,
	sint32 height,
	sint32 bpp,
	MBCHAR *ldlFilename,
	BOOL useExclusiveMode
)
:   aui_UI              (),
    aui_SDL             (),
    m_X11Display        (nullptr)
{

	*retval = aui_Region::InitCommon( 0, 0, 0, width, height );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	Assert( aui_Base::GetBaseRefCount() == 2 );
	aui_ui_Set(aui_Base::GetBaseRefCount() == 2 ? this : nullptr);

	*retval = aui_UI::InitCommon( hinst, hwnd, bpp, ldlFilename );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommon();
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = CreateNativeScreen( useExclusiveMode );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

#if defined(HAVE_X11)
	char *dispname = getenv("DISPLAY");
	if (dispname) {
		m_X11Display = XOpenDisplay(dispname);
	} else {
		m_X11Display = XOpenDisplay(":0.0");
	}
	// X11 display is optional (used only for font path discovery).
	// On macOS with SDL2, X11 may not be available.
	// if (!m_X11Display) {
	// 	*retval = AUI_ERRCODE_NOUI;
	// }
#endif
}


AUI_ERRCODE aui_SDLUI::InitCommon()
{
	m_savedMouseAnimFirstIndex = 0;
	m_savedMouseAnimLastIndex = 0;
	m_savedMouseAnimCurIndex = 0;

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE aui_SDLUI::DestroyNativeScreen()
{
	if (m_primary)
	{
		delete m_primary;
		m_primary   = nullptr;
		m_lpdds     = nullptr;
	}

	return AUI_ERRCODE_OK;
}

AUI_ERRCODE aui_SDLUI::CreateNativeScreen( BOOL useExclusiveMode )
{
	AUI_ERRCODE errcode = aui_SDL::InitCommon( useExclusiveMode );
	Assert( AUI_SUCCESS(errcode) );
	assert( AUI_SUCCESS(errcode) );
	if ( !AUI_SUCCESS(errcode) ) return errcode;

	// SDL2: create window instead of SDL_SetVideoMode.
	// ALLOW_HIGHDPI: on Retina the drawable becomes 2x the logical size, so the
	//   GPU present (RenderSetLogicalSize below) scales the game surface onto a
	//   full-resolution backing instead of an OS-upscaled blurry one.
	// RESIZABLE: the renderer's logical size letterboxes/scales the game res to
	//   any window size; mouse coords are mapped back via SDL_RenderWindowToLogical
	//   in aui_SDLMouse so input stays correct when the window is not 1:1.
	Uint64 windowFlags = CTP2_SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
#if defined(RENDER_TOOL_BUILD)
	windowFlags |= SDL_WINDOW_HIDDEN;
#else
	windowFlags |= CTP2_SDL_WINDOW_SHOWN;
#endif
	if (g_SDL_flags) {
		windowFlags |= SDL_WINDOW_FULLSCREEN;
	}
	m_window = CTP2_SDL_CreateWindow("Call to Power 2", m_width, m_height, windowFlags);
	if (!m_window) {
		// "%s": SDL_GetError() is runtime text, not a format template.
		c3errors_FatalDialog("aui_SDLUI", "%s", SDL_GetError());
	}
	// Text input for city names/chat: enable once per window. ASCII keeps
	// arriving via KEYDOWN; SDL_TEXTINPUT carries the rest (see handler).
	CTP2_SDL_StartTextInput(m_window);
	fprintf(stderr, "[SDLUI] display scale: %.2f (1.0 = no Retina scaling)\n",
	        CTP2_SDL_GetWindowDisplayScale(m_window));

	// Ensure cursor is hidden inside the window (macOS may need this after window creation)
	CTP2_SDL_HideCursor();

	// GPU present layer (portable SDL2 — Metal on macOS, GL/Vulkan on Linux):
	// create an accelerated renderer + a streaming texture sized to the game
	// resolution. The engine still composites into the software `primary`
	// surface; Flip() uploads that to the texture and the renderer scales/
	// presents it on the GPU. Logical size = game res, so resized/HiDPI
	// windows scale on the GPU. NOTE: SDL_GetWindowSurface and SDL_Renderer are
	// mutually exclusive on one window — the primary is therefore a standalone
	// surface, not the window surface.
	m_renderer = CTP2_SDL_CreateRenderer(m_window, false);
	if (!m_renderer) {
		// Software-renderer fallback keeps display-less / unusual-GPU setups
		// (some Linux CI) alive rather than aborting.
		m_renderer = CTP2_SDL_CreateRenderer(m_window, true);
	}
	if (!m_renderer) {
		// "%s": SDL_GetError() is runtime text, not a format template.
		c3errors_FatalDialog("aui_SDLUI", "%s", SDL_GetError());
	}
	CTP2_SDL_SetRenderLogicalSize(m_renderer, m_width, m_height);
	SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
	m_screenTexture = SDL_CreateTexture(m_renderer,
		SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
		m_width, m_height);
	if (!m_screenTexture) {
		// "%s": SDL_GetError() is runtime text, not a format template.
		c3errors_FatalDialog("aui_SDLUI", "%s", SDL_GetError());
	}

	// P11 Stage 2 D: when per-layer GPU compositing is enabled, create separate
	// world + UI layer textures (world composited under, UI alpha-blended over).
	// Dormant until the two-layer present is wired; the default single-texture
	// present is unaffected.
	if (aui_SDL::GpuLayersEnabled()) {
		// P12: both streaming and full-GPU paths use one oversized world texture.
		// Screen top-left lives at WorldContentOffX/Y inside it; the camera pans by
		// moving the source window. Quad mode only changes HOW the world texture is
		// filled (render target instead of CPU upload), not its coordinate system.
		int const worldMargin = aui_SDL::WorldMargin();
		// P13 step 0: the camera's pan budget and safe zoom range are both
		// functions of the screen size it windows out of the world texture.
		aui_SDL::SetViewportSize(m_width, m_height);
		m_worldTexture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888,
			aui_SDL::GpuQuadsEnabled()
				? SDL_TEXTUREACCESS_TARGET : SDL_TEXTUREACCESS_STREAMING,
			m_width + 2 * worldMargin, m_height + 2 * worldMargin);
		m_uiTexture = SDL_CreateTexture(m_renderer,
			SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, m_width, m_height);
		if (m_uiTexture) {
			SDL_SetTextureBlendMode(m_uiTexture, SDL_BLENDMODE_BLEND);
			// A freshly created streaming texture has undefined contents; clear
			// it to fully transparent (ARGB 0x00000000) so, until the UI
			// composite is redirected into it, the alpha overlay contributes
			// nothing and the presented frame matches the single-texture path.
			// Lock the streaming texture and zero its pixels. Guard on the
			// returned pixels pointer, not the return code: SDL_LockTexture
			// yields int(0)=ok on SDL2 but bool(true)=ok on SDL3, so a `== 0`
			// test would misfire on the SDL3 default backend.
			void * pixels = nullptr;
			int    pitch  = 0;
			SDL_LockTexture(m_uiTexture, nullptr, &pixels, &pitch);
			if (pixels) {
				// Clear the whole locked region (all rows at full pitch,
				// padding included) so every pixel is fully transparent
				// (ARGB 0x00000000). memset takes the pointer directly.
				memset(pixels, 0, static_cast<size_t>(m_height) * pitch);
				SDL_UnlockTexture(m_uiTexture);
			}
		}
		if (!m_worldTexture || !m_uiTexture) {
			// "%s": SDL_GetError() is runtime text, not a format template.
		c3errors_FatalDialog("aui_SDLUI", "%s", SDL_GetError());
		}
	}

	// P11 Stage 2 C: fog-of-war mask texture, composited over the world texture
	// on the GPU (between the world and UI copies) to darken fogged terrain.
	// Requires GpuLayersEnabled (implied by GpuFogEnabled). Created transparent.
	if (aui_SDL::GpuFogEnabled()) {
		m_fogTexture = SDL_CreateTexture(m_renderer,
			SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, m_width, m_height);
		if (m_fogTexture) {
			SDL_SetTextureBlendMode(m_fogTexture, SDL_BLENDMODE_BLEND);
			void * pixels = nullptr;
			int    pitch  = 0;
			SDL_LockTexture(m_fogTexture, nullptr, &pixels, &pitch);
			if (pixels) {
				memset(pixels, 0, static_cast<size_t>(m_height) * pitch);
				SDL_UnlockTexture(m_fogTexture);
			}
		} else {
			// "%s": SDL_GetError() is runtime text, not a format template.
		c3errors_FatalDialog("aui_SDLUI", "%s", SDL_GetError());
		}
	}

	fprintf(stderr, "[SDLUI] Requested screen: %dx%d @ %dbpp; renderer + ARGB8888 streaming texture\n",
		m_width, m_height, m_bpp);

	// Primary: standalone 32-bit ARGB8888 surface (bpp=32, no wrapped window
	// surface). It is only a software mirror for tests/screenshot capture — the
	// secondary is the surface presented to the GPU (see below), so the primary
	// is created NOT-primary: aui_SDLSurface::Flip() presents only when
	// m_isPrimary is set, and exactly one surface (the secondary) must present.
	m_primary = new aui_SDLSurface(
		&errcode,
		m_width,
		m_height,
		32,
		nullptr,
		FALSE );
	Assert( AUI_NEWOK(m_primary,errcode) );
	assert( AUI_NEWOK(m_primary,errcode) );
	if ( !AUI_NEWOK(m_primary,errcode) ) return AUI_ERRCODE_MEMALLOCFAILED;

	// Keep aui_SDLUI::m_lpdds pointing at the primary's SDL surface for any
	// consumers that read it (it used to be the window surface).
	m_lpdds = static_cast<aui_SDLSurface *>(m_primary)->DDS();

	fprintf(stderr, "[SDLUI] Primary surface: %dx%d @ 32bpp\n", m_primary->Width(), m_primary->Height());

	// Secondary: the live 32-bit ARGB8888 composite AND the surface presented to
	// the GPU. BltSecondaryToPrimary() mirrors it into the primary (for the
	// pixel oracle) and then calls m_secondary->Flip(), which uploads these
	// pixels to the screen texture and presents. That present path runs only
	// when m_isPrimary is set, so the presenting surface must be created primary.
	m_secondary = new aui_SDLSurface(
		&errcode,
		m_width,
		m_height,
		32,
		nullptr,
		TRUE );
	Assert( AUI_NEWOK(m_secondary,errcode) );
	assert( AUI_NEWOK(m_secondary,errcode) );
	if ( !AUI_NEWOK(m_secondary,errcode) ) return AUI_ERRCODE_MEMALLOCFAILED;

	fprintf(stderr, "[SDLUI] Secondary surface: %dx%d @ 32bpp\n", m_secondary->Width(), m_secondary->Height());

	m_pixelFormat = m_primary->PixelFormat();

	// P11 Stage 2 D: per-layer GPU compositing. Create the world-only and
	// UI-only composite surfaces (32-bit, screen-sized, NOT-primary — they are
	// uploaded to their own GPU textures in Flip, never self-present). The UI
	// surface starts fully transparent so the world shows through everywhere the
	// UI has not drawn; the aui_UI chokepoint mirrors each composite write into
	// one of these two layers. Gated so the default single-texture path is
	// untouched.
	if (aui_SDL::GpuLayersEnabled()) {
		// P12: keep the CPU mirror oversized too while the migration is hybrid; the
		// GPU quad path may stop depending on it later, but the layer dimensions stay
		// identical across renderers.
		int const worldMargin = aui_SDL::WorldMargin();
		m_worldSurface = new aui_SDLSurface(&errcode, m_width + 2 * worldMargin,
			m_height + 2 * worldMargin, 32, nullptr, FALSE);
		if (!AUI_NEWOK(m_worldSurface, errcode)) return AUI_ERRCODE_MEMALLOCFAILED;
		m_uiSurface = new aui_SDLSurface(&errcode, m_width, m_height, 32, nullptr, FALSE);
		if (!AUI_NEWOK(m_uiSurface, errcode)) return AUI_ERRCODE_MEMALLOCFAILED;

		// Zero both surfaces (ARGB 0x00000000). The world layer is fully
		// repainted by the opaque background window each frame; the UI layer
		// stays transparent until UI composites into it.
		SDL_Surface *ws = static_cast<aui_SDLSurface *>(m_worldSurface)->DDS();
		SDL_Surface *us = static_cast<aui_SDLSurface *>(m_uiSurface)->DDS();
		if (ws && ws->pixels) memset(ws->pixels, 0, static_cast<size_t>(ws->h) * ws->pitch);
		if (us && us->pixels) memset(us->pixels, 0, static_cast<size_t>(us->h) * us->pitch);

		m_gpuLayers = true;
		fprintf(stderr, "[SDLUI] Per-layer GPU compositing ON: world + UI surfaces %dx%d @ 32bpp\n",
			m_width, m_height);
	}

	// P11 Stage 2 C: fog mask surface — TiledMap stamps fogged-tile diamonds
	// here (50% black); Flip uploads it to m_fogTexture. Transparent to start.
	if (aui_SDL::GpuFogEnabled()) {
		m_fogSurface = new aui_SDLSurface(&errcode, m_width, m_height, 32, nullptr, FALSE);
		if (!AUI_NEWOK(m_fogSurface, errcode)) return AUI_ERRCODE_MEMALLOCFAILED;
		SDL_Surface *fs = static_cast<aui_SDLSurface *>(m_fogSurface)->DDS();
		if (fs && fs->pixels) memset(fs->pixels, 0, static_cast<size_t>(fs->h) * fs->pitch);
		m_gpuFog = true;
		fprintf(stderr, "[SDLUI] GPU fog mask ON: fog surface %dx%d @ 32bpp\n", m_width, m_height);
	}

	return AUI_ERRCODE_OK;
}

#ifdef HAVE_X11
Display *
aui_SDLUI::getDisplay()
{
	return m_X11Display;
}
#endif

aui_SDLUI::~aui_SDLUI( )
{
	if ( m_screenTexture ) {
		SDL_DestroyTexture(m_screenTexture);
		m_screenTexture = nullptr;
	}
	if ( m_renderer ) {
		SDL_DestroyRenderer(m_renderer);
		m_renderer = nullptr;
	}
	if ( m_window ) {
		CTP2_SDL_StopTextInput(m_window);
		SDL_DestroyWindow(m_window);
		m_window = nullptr;
		m_lpdds = nullptr;
	}
#ifdef HAVE_X11
	if (m_X11Display) {
		XCloseDisplay(m_X11Display);
		m_X11Display = nullptr;
	}
#endif
}

AUI_ERRCODE aui_SDLUI::TearDownMouse()
{

	if (m_mouse) {
		m_mouse->GetAnimIndexes(&m_savedMouseAnimFirstIndex, &m_savedMouseAnimLastIndex);
		m_savedMouseAnimCurIndex = m_mouse->GetCurrentCursorIndex();
		m_savedMouseAnimDelay = (sint32)m_mouse->GetAnimDelay();

		if ( m_minimize || m_exclusiveMode )
		{
		}

		m_mouse->End();
		delete m_mouse;
		m_mouse = nullptr;
	}

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE aui_SDLUI::RestoreMouse()
{
	AUI_ERRCODE		auiErr;
	BOOL			exclusive = TRUE;

	aui_SDLMouse *mouse = new aui_SDLMouse( &auiErr, const_cast<MBCHAR *>("CivMouse"), exclusive );
	Assert(mouse != nullptr);
	if ( !mouse ) return AUI_ERRCODE_MEMALLOCFAILED;

	delete m_mouse;
	m_mouse = mouse;

	m_mouse->SetAnimIndexes(m_savedMouseAnimFirstIndex, m_savedMouseAnimLastIndex);
	m_mouse->SetCurrentCursor(m_savedMouseAnimCurIndex);
	m_mouse->SetAnimDelay((uint32)m_savedMouseAnimDelay);

	auiErr = m_mouse->Start();
	Assert(auiErr == AUI_ERRCODE_OK);
	if ( auiErr != AUI_ERRCODE_OK ) return auiErr;

	if ( m_minimize || m_exclusiveMode )
	{
		POINT point;
		//GetCursorPos( &point );
		m_mouse->SetPosition( &point );
	}

	return AUI_ERRCODE_OK;
}

AUI_ERRCODE aui_SDLUI::AltTabOut( )
{
	assert(0);

	if(m_keyboard) m_keyboard->Unacquire();
	if ( m_joystick ) m_joystick->Unacquire();

	if (m_mouse) {
		if (g_exclusiveMode) {
			TearDownMouse();
		} else {
			main_RestoreTaskBar();

			if (!m_mouse->IsSuspended()) {
				m_mouse->Suspend(FALSE);
				m_mouse->Unacquire();
			}
		}
	}

	if ( m_minimize || m_exclusiveMode )
	{
		DestroyNativeScreen();
	}

	if (civapp_Get())
	{
		civapp_Get()->SetInBackground(TRUE);
	}

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE aui_SDLUI::AltTabIn( )
{
	assert(0);

	if ( !m_primary ) CreateNativeScreen( m_exclusiveMode );

	if ( m_joystick ) m_joystick->Acquire();
	if (m_keyboard) m_keyboard->Acquire();

	if (civapp_Get())
    {
        civapp_Get()->SetInBackground(FALSE);
    }

	return FlushDirtyList();
}

#endif  // __AUI_USE_SDL__
