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
float aui_SDL::m_panVelX = 0.0f;
float aui_SDL::m_panVelY = 0.0f;
float aui_SDL::m_zoomVel = 0.0f;
float aui_SDL::m_homeZoom = 1.0f;
// P11 Stage 3 G1: terrain quad atlas + per-frame draw list.
SDL_Texture *aui_SDL::m_quadAtlasTexture = nullptr;
int aui_SDL::m_quadAtlasW = 0;
int aui_SDL::m_quadAtlasH = 0;
std::vector<aui_SDL::GpuQuad> aui_SDL::m_quadDrawList;
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

bool aui_SDL::GpuQuadsEnabled()
{
	// Opt-in, cached. Terrain-as-GPU-quads renders the world into a render-target
	// texture instead of uploading the CPU world surface, so it requires
	// per-layer compositing (implies GpuLayersEnabled). Off by default.
	static int s_enabled = -1;
	if (s_enabled < 0)
	{
		char const * e = getenv("CTP2_GPU_QUADS");
		s_enabled = (e && e[0] && strcmp(e, "0") != 0 && GpuLayersEnabled()) ? 1 : 0;
	}
	return s_enabled != 0;
}

void aui_SDL::EnsureQuadAtlas(int atlasW, int atlasH)
{
	// Lazily create the atlas texture the terrain quads sample from. It is a
	// streaming ARGB8888 texture (CPU-composited tiles are uploaded slot by slot
	// via UploadQuadAtlasSlot) with alpha blending so a tile's transparent
	// diamond surround does not clobber its neighbours when quads tessellate.
	if (!m_renderer) return;
	if (m_quadAtlasTexture && m_quadAtlasW == atlasW && m_quadAtlasH == atlasH)
		return;
	if (m_quadAtlasTexture)
	{
		SDL_DestroyTexture(m_quadAtlasTexture);
		m_quadAtlasTexture = nullptr;
	}
	m_quadAtlasTexture = SDL_CreateTexture(m_renderer,
		SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, atlasW, atlasH);
	if (m_quadAtlasTexture)
	{
		SDL_SetTextureBlendMode(m_quadAtlasTexture, SDL_BLENDMODE_BLEND);
		m_quadAtlasW = atlasW;
		m_quadAtlasH = atlasH;
	}
}

void aui_SDL::UploadQuadAtlasSlot(int x, int y, int w, int h,
                                  void const *pixels, int pitch)
{
	if (!m_quadAtlasTexture) return;
	SDL_Rect rect = { x, y, w, h };
	CTP2_SDL_UpdateTexture(m_quadAtlasTexture, &rect, pixels, pitch);
}

void aui_SDL::TickCamera(float dtSec)
{
	// Clamp dt so a stall (e.g. a long modal) doesn't fling the camera.
	if (dtSec > 0.05f) dtSec = 0.05f;
	if (dtSec <= 0.0f) return;

	// --- Pan: velocity + exponential friction, no spring (stays where left). ---
	// k_PAN_FRICTION per-second decay rate: higher = stops sooner.
	float const k_PAN_FRICTION = 6.0f;
	float const panDecay = 1.0f - k_PAN_FRICTION * dtSec;
	m_cameraOffX += m_panVelX * dtSec;
	m_cameraOffY += m_panVelY * dtSec;
	m_panVelX *= (panDecay > 0.0f) ? panDecay : 0.0f;
	m_panVelY *= (panDecay > 0.0f) ? panDecay : 0.0f;

	// --- Zoom: damped spring toward the home zoom (the "gravity"). ---
	// Impulses (scroll) push m_zoomVel; the spring pulls zoom back to home and
	// the damping bleeds off velocity so it settles rather than oscillating.
	float const k_ZOOM_SPRING  = 55.0f;   // pull strength toward home (1/s^2)
	float const k_ZOOM_DAMPING = 9.0f;    // velocity damping (1/s)
	float const accel = k_ZOOM_SPRING * (m_homeZoom - m_cameraZoom)
	                  - k_ZOOM_DAMPING * m_zoomVel;
	m_zoomVel   += accel * dtSec;
	m_cameraZoom += m_zoomVel * dtSec;

	// Clamp zoom to a sane peek range.
	float const k_ZOOM_MIN = 0.5f, k_ZOOM_MAX = 2.5f;
	if (m_cameraZoom < k_ZOOM_MIN) { m_cameraZoom = k_ZOOM_MIN; if (m_zoomVel < 0.0f) m_zoomVel = 0.0f; }
	if (m_cameraZoom > k_ZOOM_MAX) { m_cameraZoom = k_ZOOM_MAX; if (m_zoomVel > 0.0f) m_zoomVel = 0.0f; }

	// Settle: once motion is negligible, snap to rest so CameraMoving() clears
	// and the frame loop stops presenting.
	if (!CameraMoving())
	{
		m_cameraZoom = m_homeZoom;
		m_zoomVel = m_panVelX = m_panVelY = 0.0f;
	}
}

bool aui_SDL::CameraMoving()
{
	float const zoomErr = m_cameraZoom - m_homeZoom;
	return (zoomErr >  0.002f) || (zoomErr < -0.002f)
	    || (m_zoomVel > 0.01f) || (m_zoomVel < -0.01f)
	    || (m_panVelX > 0.5f)  || (m_panVelX < -0.5f)
	    || (m_panVelY > 0.5f)  || (m_panVelY < -0.5f);
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
