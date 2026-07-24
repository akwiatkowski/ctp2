#include "os/include/ctp2_config.h"
#include "ctp/c3.h"

#include <cmath>    // std::exp — frame-rate-independent camera ease

#ifdef __AUI_USE_SDL__

#include "ui/aui_common/aui_ui.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_surface.h"
#include "gfx/gfx_utils/pixelutils.h"
#include "gfx/spritesys/ModernSpriteAtlas.h"
#include "gfx/tilesys/tileset.h"
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
float aui_SDL::m_panTargetX = 0.0f;
float aui_SDL::m_panTargetY = 0.0f;
// P11 Stage 3 G1: terrain quad atlas + per-frame draw list.
SDL_Texture *aui_SDL::m_quadAtlasTexture = nullptr;
int aui_SDL::m_quadAtlasW = 0;
int aui_SDL::m_quadAtlasH = 0;
bool aui_SDL::m_quadFrameComplete = true;
char const *aui_SDL::m_quadFrameIncompleteReason = nullptr;
std::vector<aui_SDL::GpuQuad> aui_SDL::m_quadDrawList;
std::map<ModernSpriteAtlas const *, SDL_Texture *> aui_SDL::m_spriteAtlasTextures;
std::map<ModernSpriteAtlas const *, SDL_Texture *> aui_SDL::m_desaturatedSpriteAtlasTextures;
std::map<std::tuple<void const *, int, int, uint16, bool, int>, SDL_Texture *> aui_SDL::m_mapIconTextures;
std::map<uint16, SDL_Texture *> aui_SDL::m_solidColorTextures;
std::vector<aui_SDL::GpuSpriteQuad> aui_SDL::m_spriteDrawList;
uint32 aui_SDL::m_SDLClassId = aui_UniqueId();
sint32 aui_SDL::m_SDLRefCount = 0;

bool aui_SDL::GpuLayersEnabled()
{
	// DEFAULT ON (P11, ADR-001/ADR-002 "flip last"): the two-layer GPU
	// present (world texture + UI texture) carries the buttery pan and the
	// hardware cursor. CTP2_GPU_LAYERS=0 opts back into the legacy
	// single-texture present.
	static int s_enabled = -1;
	if (s_enabled < 0)
	{
		char const * e = getenv("CTP2_GPU_LAYERS");
		s_enabled = (e && e[0]) ? (strcmp(e, "0") != 0 ? 1 : 0) : 1;
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
	// DEFAULT ON with the layers (P11): the smooth camera pans/zooms the
	// world+fog layers at present time; identity until input drives it, so
	// with the camera idle the present matches the plain layered copy.
	// CTP2_GPU_CAMERA=0 opts out (or opting out of layers disables both).
	static int s_enabled = -1;
	if (s_enabled < 0)
	{
		char const * e = getenv("CTP2_GPU_CAMERA");
		bool const on = (e && e[0]) ? (strcmp(e, "0") != 0) : true;
		s_enabled = (on && GpuLayersEnabled()) ? 1 : 0;
	}
	return s_enabled != 0;
}

bool aui_SDL::GpuQuadsEnabled()
{
	// Default-on, cached. Terrain-as-GPU-quads renders the world into a render-target
	// texture instead of uploading the CPU world surface, so it requires
	// per-layer compositing (implies GpuLayersEnabled). With the smooth camera it
	// also needs modern sprites; otherwise the GPU world would be terrain-only
	// again and units would not pan with tiles. CTP2_GPU_QUADS=0 opts out while
	// the temporary CPU fallback still exists.
	static int s_enabled = -1;
	if (s_enabled < 0)
	{
		char const * e = getenv("CTP2_GPU_QUADS");
		bool const requested = !e || !e[0] || strcmp(e, "0") != 0;
		bool const spritesSafe = !GpuCameraEnabled() || ModernSpritesEnabled();
		s_enabled = (requested && GpuLayersEnabled() && spritesSafe) ? 1 : 0;
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

void aui_SDL::MarkQuadFrameIncomplete(char const *reason)
{
	m_quadFrameComplete = false;
	if (!m_quadFrameIncompleteReason)
		m_quadFrameIncompleteReason = reason;
}

SDL_Texture *aui_SDL::EnsureSpriteAtlasTexture(ModernSpriteAtlas const *atlas, bool desaturate)
{
	if (!m_renderer || !atlas || atlas->Width() <= 0 || atlas->Height() <= 0)
		return nullptr;

	auto &textures = desaturate ? m_desaturatedSpriteAtlasTextures : m_spriteAtlasTextures;
	auto const found = textures.find(atlas);
	if (found != textures.end())
		return found->second;

	std::vector<uint8_t> rgba;
	uint8_t const *pixels = atlas->Rgba().data();
	if (desaturate)
	{
		rgba = atlas->Rgba();
		for (size_t i = 0; i + 3 < rgba.size(); i += 4)
		{
			uint8_t const ave = static_cast<uint8_t>((static_cast<uint16_t>(rgba[i]) + rgba[i + 1] + rgba[i + 2]) / 3);
			rgba[i] = rgba[i + 1] = rgba[i + 2] = ave;
		}
		pixels = rgba.data();
	}

	SDL_Texture *texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ABGR8888,
		SDL_TEXTUREACCESS_STATIC, atlas->Width(), atlas->Height());
	if (!texture)
		return nullptr;

	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	CTP2_SDL_UpdateTexture(texture, nullptr, pixels, atlas->Width() * 4);
	textures[atlas] = texture;
	return texture;
}

void aui_SDL::ReleaseSpriteAtlasTexture(ModernSpriteAtlas const *atlas)
{
	auto const found = m_spriteAtlasTextures.find(atlas);
	if (found != m_spriteAtlasTextures.end())
	{
		SDL_DestroyTexture(found->second);
		m_spriteAtlasTextures.erase(found);
	}

	auto const desatFound = m_desaturatedSpriteAtlasTextures.find(atlas);
	if (desatFound == m_desaturatedSpriteAtlasTextures.end())
		return;
	SDL_DestroyTexture(desatFound->second);
	m_desaturatedSpriteAtlasTextures.erase(desatFound);
}

SDL_Texture *aui_SDL::EnsureMapIconTexture(void const *data, int w, int h, uint16 color, bool blend, int blendValue)
{
	if (!m_renderer || !data || w <= 0 || h <= 0)
		return nullptr;

	auto const key = std::make_tuple(data, w, h, color, blend, blendValue);
	auto const found = m_mapIconTextures.find(key);
	if (found != m_mapIconTextures.end())
		return found->second;

	std::vector<uint32> rgba(static_cast<size_t>(w) * static_cast<size_t>(h), 0);
	Pixel16 const *encoded = static_cast<Pixel16 const *>(data);
	uint16 const start = static_cast<uint16>(*encoded++);
	uint16 const end = static_cast<uint16>(*encoded++);
	Pixel16 const *table = encoded;
	Pixel16 const *dataStart = table + (end - start + 1);
	Pixel32 const argbColor = pixelutils_16to8888(color) | 0xff000000u;

	for (sint32 j = start; j <= end && j < h; ++j)
	{
		if (static_cast<sint16>(table[j - start]) == -1)
			continue;

		uint32 *dest = rgba.data() + static_cast<size_t>(j) * static_cast<size_t>(w);
		Pixel16 const *rowData = dataStart + table[j - start];
		Pixel16 tag;
		do {
			tag = *rowData++;
			sint32 len = tag & 0x00ff;
			switch ((tag & 0x0f00) >> 8) {
				case k_TILE_SKIP_RUN_ID:
					dest += len;
					break;
				case k_TILE_COPY_RUN_ID:
					while (len-- > 0 && dest < rgba.data() + static_cast<size_t>(j + 1) * static_cast<size_t>(w)) {
						Pixel16 const px = blend ? pixelutils_BlendFast(*rowData, color, blendValue) : *rowData;
						*dest++ = pixelutils_16to8888(px) | 0xff000000u;
						++rowData;
					}
					break;
				case k_TILE_COLORIZE_RUN_ID:
					while (len-- > 0 && dest < rgba.data() + static_cast<size_t>(j + 1) * static_cast<size_t>(w))
						*dest++ = argbColor;
					break;
				case k_TILE_SHADOW_RUN_ID:
					// Shadow runs depend on destination pixels in the CPU blitter; map
					// icons used by GPU unit brackets do not rely on them, so keep them transparent.
					dest += len;
					break;
			}
		} while ((tag & 0xf000) == 0 && dest < rgba.data() + static_cast<size_t>(j + 1) * static_cast<size_t>(w));
	}

	SDL_Texture *texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STATIC, w, h);
	if (!texture)
		return nullptr;
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	CTP2_SDL_UpdateTexture(texture, nullptr, rgba.data(), w * static_cast<int>(sizeof(uint32)));
	m_mapIconTextures[key] = texture;
	return texture;
}

SDL_Texture *aui_SDL::EnsureSolidColorTexture(uint16 color)
{
	if (!m_renderer)
		return nullptr;
	auto const found = m_solidColorTextures.find(color);
	if (found != m_solidColorTextures.end())
		return found->second;
	uint32 const argb = pixelutils_16to8888(color) | 0xff000000u;
	SDL_Texture *texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STATIC, 1, 1);
	if (!texture)
		return nullptr;
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	CTP2_SDL_UpdateTexture(texture, nullptr, &argb, static_cast<int>(sizeof(argb)));
	m_solidColorTextures[color] = texture;
	return texture;
}

void aui_SDL::SetHardwareCursor(aui_Surface *surf, int hotX, int hotY)
{
	// The installed SDL_Cursor must stay alive while set; keep the current
	// one and destroy it only after its replacement is installed.
	static SDL_Cursor *s_current = nullptr;

	if (!surf)
	{
		CTP2_SDL_HideCursor();
		return;
	}

	LPVOID buffer = nullptr;
	if (surf->Lock(nullptr, &buffer, 0) != AUI_ERRCODE_OK || !buffer)
		return;

	int const     w      = surf->Width();
	int const     h      = surf->Height();
	sint32 const  pitch  = surf->Pitch();
	sint32 const  bpp    = surf->BitsPerPixel();
	uint32 const  chroma = surf->GetChromaKey();
	uint8 const * base   = static_cast<uint8 const *>(buffer);

	// Chroma key -> alpha: the game blits cursors with
	// k_AUI_BLITTER_FLAG_CHROMAKEY (pixels equal to the surface's key are
	// skipped); the OS cursor wants that as real transparency instead.
	// Pixels are assembled byte-wise (little-endian, as everywhere in the
	// codebase) so no pointer reinterpretation is needed.
	std::vector<uint32> argb(static_cast<size_t>(w) * h);
	for (int y = 0; y < h; ++y)
	{
		uint32 *out = argb.data() + static_cast<size_t>(y) * w;
		uint8 const *row = base + static_cast<size_t>(y) * pitch;
		if (bpp == 16)
		{
			for (int x = 0; x < w; ++x)
			{
				uint16 const p = static_cast<uint16>(
					row[2 * x] | (row[2 * x + 1] << 8));
				if (p == static_cast<uint16>(chroma))
				{
					out[x] = 0;
					continue;
				}
				// RGB565 -> 888, replicating high bits into low so pure
				// white/black stay pure.
				uint32 const r = (p >> 11) & 0x1f;
				uint32 const g = (p >> 5) & 0x3f;
				uint32 const b = p & 0x1f;
				out[x] = 0xff000000u
				       | ((r << 3 | r >> 2) << 16)
				       | ((g << 2 | g >> 4) << 8)
				       |  (b << 3 | b >> 2);
			}
		}
		else // 32bpp ARGB
		{
			for (int x = 0; x < w; ++x)
			{
				uint32 const p = static_cast<uint32>(row[4 * x])
				               | (static_cast<uint32>(row[4 * x + 1]) << 8)
				               | (static_cast<uint32>(row[4 * x + 2]) << 16)
				               | (static_cast<uint32>(row[4 * x + 3]) << 24);
				out[x] = (p == chroma) ? 0 : (p | 0xff000000u);
			}
		}
	}
	surf->Unlock(buffer);

	SDL_Surface *ss = CTP2_SDL_CreateARGBSurfaceFrom(
		argb.data(), w, h, w * static_cast<int>(sizeof(uint32)));
	if (!ss)
		return;
	SDL_Cursor *cursor = SDL_CreateColorCursor(ss, hotX, hotY);
	SDL_FreeSurface(ss);
	if (!cursor)
		return;

	SDL_SetCursor(cursor);
	CTP2_SDL_ShowCursor();
	if (s_current)
		SDL_FreeCursor(s_current);
	s_current = cursor;
}

void aui_SDL::TickCamera(float dtSec)
{
	// Clamp dt so a stall (e.g. a long modal) doesn't fling the camera.
	if (dtSec > 0.05f) dtSec = 0.05f;
	if (dtSec <= 0.0f) return;

	// --- Pan: exponential follow toward the commanded target (buttery glide). ---
	// The trackpad sets m_panTarget (in big, bursty jumps); the displayed CameraOff
	// eases a fraction of the remaining distance each frame, so the motion is smooth
	// at the frame rate no matter how chunky the event stream is. macOS keeps sending
	// decaying momentum wheel events after the finger lifts, so following the target
	// yields natural inertia for free. k_PAN_FOLLOW: higher = snappier (less lag),
	// lower = floatier.
	//
	// The factor uses the EXACT exponential form 1-e^(-k*dt), not the linear k*dt
	// approximation: the linear form saturates to a 100% snap-to-target once
	// dt >= 1/k (0.045s at k=22) — precisely what happens when presents get
	// starved to 20Hz — turning the ease back into per-event stepping. The exact
	// form converges to but never reaches 1, and composes correctly across any
	// frame rate (two 8ms ticks advance exactly as far as one 16ms tick).
	float const k_PAN_FOLLOW = 22.0f;
	float const panFollow = 1.0f - std::exp(-k_PAN_FOLLOW * dtSec);
	m_cameraOffX += (m_panTargetX - m_cameraOffX) * panFollow;
	m_cameraOffY += (m_panTargetY - m_cameraOffY) * panFollow;

	// Hard-clamp the pan (offset AND target) to the rendered world margin. The
	// present windows the source rect at (contentOff - off), which is only valid
	// while |off| <= contentOff — beyond it the window samples unrendered (black)
	// texture. This is also the safety bound that keeps the recenter's ScrollMap
	// requests small: an unclamped ease once walked the offset out to ~1000px —
	// which then asked ScrollMap for a scroll taller than its surface (the
	// 2026-07-16 ScrollPixels crash). Recenter shifts offset+target back inside
	// the margin as the pan travels, so this cap never limits sustained
	// scrolling — only the queued overshoot.
	float const limX = static_cast<float>(WorldContentOffX());
	float const limY = static_cast<float>(WorldContentOffY());
	auto clampPanAxis = [](float &off, float &tgt, float lim)
	{
		if (tgt >  lim) tgt =  lim;
		if (tgt < -lim) tgt = -lim;
		if (off >  lim) off =  lim;
		if (off < -lim) off = -lim;
	};
	clampPanAxis(m_cameraOffX, m_panTargetX, limX);
	clampPanAxis(m_cameraOffY, m_panTargetY, limY);

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
		// Snap the displayed offset exactly onto the target so no sub-pixel error
		// lingers (and the next recenter math starts clean).
		m_cameraOffX = m_panTargetX;
		m_cameraOffY = m_panTargetY;
	}
}

bool aui_SDL::CameraMoving()
{
	float const zoomErr = m_cameraZoom - m_homeZoom;
	float const panErrX = m_panTargetX - m_cameraOffX;
	float const panErrY = m_panTargetY - m_cameraOffY;
	return (zoomErr >  0.002f) || (zoomErr < -0.002f)
	    || (m_zoomVel > 0.01f) || (m_zoomVel < -0.01f)
	    || (panErrX  > 0.5f)   || (panErrX  < -0.5f)
	    || (panErrY  > 0.5f)   || (panErrY  < -0.5f);
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
	// Layered mode uses the OS cursor (see HardwareCursorEnabled); the
	// default arrow shows until the game installs its themed cursor.
	if (!HardwareCursorEnabled())
		CTP2_SDL_HideCursor();

	// SDL2: all events are enabled by default; no need to filter
	// SDL_EnableUNICODE removed in SDL2 (always on)

	return AUI_ERRCODE_OK;
}

aui_SDL::~aui_SDL()
{
	if (! --m_SDLRefCount) {
		for (auto &entry : m_spriteAtlasTextures)
			SDL_DestroyTexture(entry.second);
		m_spriteAtlasTextures.clear();
		for (auto &entry : m_desaturatedSpriteAtlasTextures)
			SDL_DestroyTexture(entry.second);
		m_desaturatedSpriteAtlasTextures.clear();
		for (auto &entry : m_mapIconTextures)
			SDL_DestroyTexture(entry.second);
		m_mapIconTextures.clear();
		for (auto &entry : m_solidColorTextures)
			SDL_DestroyTexture(entry.second);
		m_solidColorTextures.clear();
		m_spriteDrawList.clear();
		if (m_quadAtlasTexture) {
			SDL_DestroyTexture(m_quadAtlasTexture);
			m_quadAtlasTexture = nullptr;
		}
		m_quadDrawList.clear();
		m_quadFrameComplete = true;
		m_quadFrameIncompleteReason = nullptr;
		SDL_Quit();
		m_lpdd = nullptr;
	}
}

#endif
