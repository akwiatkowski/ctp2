#include "os/include/ctp2_config.h"
#include "ctp/c3.h"

#ifdef __AUI_USE_SDL__

#include "ui/aui_common/aui_ui.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_sdl/aui_sdl.h"

SDL_Surface *aui_SDL::m_lpdd = nullptr;
SDL_Window *aui_SDL::m_window = nullptr;
SDL_Renderer *aui_SDL::m_renderer = nullptr;
SDL_Texture *aui_SDL::m_screenTexture = nullptr;
// P11 Stage 2 D (per-layer GPU compositing): separate world + UI GPU textures,
// composited on the GPU. Null / unused unless GpuLayersEnabled().
SDL_Texture *aui_SDL::m_worldTexture = nullptr;
SDL_Texture *aui_SDL::m_uiTexture = nullptr;
// P11 Stage 2 C: fog-of-war mask composited over the world texture on the GPU.
SDL_Texture *aui_SDL::m_fogTexture = nullptr;
// P11 Stage 2 F: smooth-camera transform (identity until the camera moves).
float aui_SDL::m_cameraOffX = 0.0f;
float aui_SDL::m_cameraOffY = 0.0f;
float aui_SDL::m_cameraZoom = 1.0f;
uint32 aui_SDL::m_SDLClassId = aui_UniqueId();
sint32 aui_SDL::m_SDLRefCount = 0;

bool aui_SDL::GpuLayersEnabled()
{
	// Opt-in, cached: the two-layer GPU present (world texture + UI texture) is
	// off by default so the verified single-texture present is untouched.
	static int s_enabled = -1;
	if (s_enabled < 0)
	{
		char const * e = getenv("CTP2_GPU_LAYERS");
		s_enabled = (e && e[0] && strcmp(e, "0") != 0) ? 1 : 0;
	}
	return s_enabled != 0;
}

bool aui_SDL::GpuFogEnabled()
{
	// Opt-in, cached. GPU fog composites a mask over the world texture, so it
	// requires per-layer compositing (the world on its own texture) — enabling
	// fog without layers is meaningless, so it implies GpuLayersEnabled().
	static int s_enabled = -1;
	if (s_enabled < 0)
	{
		char const * e = getenv("CTP2_GPU_FOG");
		s_enabled = (e && e[0] && strcmp(e, "0") != 0 && GpuLayersEnabled()) ? 1 : 0;
	}
	return s_enabled != 0;
}

bool aui_SDL::GpuCameraEnabled()
{
	// Opt-in, cached. The smooth camera pans/zooms the world+fog GPU layers at
	// present time, so it requires per-layer compositing (implies
	// GpuLayersEnabled). Identity transform until something drives the camera.
	static int s_enabled = -1;
	if (s_enabled < 0)
	{
		char const * e = getenv("CTP2_GPU_CAMERA");
		s_enabled = (e && e[0] && strcmp(e, "0") != 0 && GpuLayersEnabled()) ? 1 : 0;
	}
	return s_enabled != 0;
}

AUI_ERRCODE aui_SDL::InitCommon(BOOL useExclusiveMode)
{
	m_exclusiveMode = useExclusiveMode;

//	SDL Init is done in CivApp::InitializeApp
/*	int rc = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTTHREAD |
	                  SDL_INIT_AUDIO | SDL_INIT_TIMER);

	if (0 != rc) {
		fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
		return AUI_ERRCODE_CREATEFAILED;
	}
*/
	CTP2_SDL_HideCursor();

	// SDL2: all events are enabled by default; no need to filter
	// SDL_EnableUNICODE removed in SDL2 (always on)

	return AUI_ERRCODE_OK;
}

aui_SDL::~aui_SDL()
{
	if (! --m_SDLRefCount) {
		SDL_Quit();
		m_lpdd = nullptr;
	}
}

#endif
