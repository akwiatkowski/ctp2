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
// P13 step 0: viewport size, overwritten by SetViewportSize when the layer
// textures are created. The 1920x1080 default keeps MinSafeZoom sane if the
// camera is queried before that (it only ever tightens the clamp).
int aui_SDL::m_viewportW = 1920;
int aui_SDL::m_viewportH = 1080;
// P13 step 1: whole-map render target (ADR-003). Null unless opted in.
SDL_Texture *aui_SDL::m_worldmapTexture = nullptr;
int aui_SDL::m_worldmapW = 0;
int aui_SDL::m_worldmapH = 0;
int aui_SDL::m_worldmapOriginX = 0;
int aui_SDL::m_lastIconOpaquePixels = 0;
int aui_SDL::m_worldmapOriginY = 0;
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
std::map<std::tuple<void const *, int, int, uint16, bool, int, bool>, SDL_Texture *> aui_SDL::m_mapIconTextures;
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

void aui_SDL::RenderWorldmapSpriteQuads(SDL_Renderer *renderer,
                                        float viewW, float viewH,
                                        float zoom, float offX, float offY)
{
	if (!renderer || !m_worldmapTexture)
		return;

	// The same source rect the terrain present used, so sprites land on the
	// tiles they belong to at every zoom and pan.
	float const srcW = viewW / zoom;
	float const srcH = viewH / zoom;
	float const srcX = (float) m_worldmapOriginX + (viewW - srcW) * 0.5f - offX;
	float const srcY = (float) m_worldmapOriginY + (viewH - srcH) * 0.5f - offY;

	for (GpuSpriteQuad const & q : m_spriteDrawList)
	{
		// Screen-space quads (city names and other overlays pinned to the
		// window) are drawn by the caller after the world, unscaled.
		if (q.screen_space || !q.texture)
			continue;

		float const mapX = (float) q.dx - (float) WorldContentOffX() + (float) m_worldmapOriginX;
		float const mapY = (float) q.dy - (float) WorldContentOffY() + (float) m_worldmapOriginY;

		SDL_SetTextureBlendMode(q.texture, q.additive ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
		SDL_SetTextureColorMod(q.texture, q.red, q.green, q.blue);
		SDL_SetTextureAlphaMod(q.texture, q.alpha);
		CTP2_SDL_RenderTextureSrcDstFlip(renderer, q.texture,
			q.sx, q.sy, q.sw, q.sh,
			(mapX - srcX) * zoom, (mapY - srcY) * zoom,
			(float) q.dw * zoom, (float) q.dh * zoom,
			q.mirror);
	}
}

bool aui_SDL::GpuWorldmapEnabled()
{
	// Opt-in, cached. ADR-003's whole-map target. It reuses the terrain-quad
	// machinery (atlas + tile cache), so it implies GpuQuadsEnabled; without
	// quads there is nothing to draw into it. Default OFF — the ADR-002 window
	// mirror remains the shipping path until this substrate is proven.
	static int s_enabled = -1;
	if (s_enabled < 0)
	{
		char const * e = getenv("CTP2_GPU_WORLDMAP");
		bool const requested = e && e[0] && strcmp(e, "0") != 0;
		s_enabled = (requested && GpuQuadsEnabled()) ? 1 : 0;
	}
	return s_enabled != 0;
}

bool aui_SDL::EnsureWorldmapTexture(int w, int h)
{
	if (!m_renderer || w <= 0 || h <= 0)
		return false;
	if (m_worldmapTexture && m_worldmapW == w && m_worldmapH == h)
		return true;

	DestroyWorldmapTexture();
	m_worldmapTexture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_TARGET, w, h);
	if (!m_worldmapTexture)
		return false;   // driver refused the size; caller falls back

	m_worldmapW = w;
	m_worldmapH = h;
	// Nearest sampling: above 1.0 the camera should show crisp pixel art rather
	// than a blurred upscale (ADR-003).
	CTP2_SDL_SetTextureNearest(m_worldmapTexture);
	SDL_SetTextureBlendMode(m_worldmapTexture, SDL_BLENDMODE_BLEND);

	// Clear once to opaque black — the same base unexplored cells keep, since
	// only explored cells ever draw a tile into it.
	SDL_Texture * const prev = SDL_GetRenderTarget(m_renderer);
	if (CTP2_SDL_SetRenderTarget(m_renderer, m_worldmapTexture))
	{
		SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
		SDL_RenderClear(m_renderer);
	}
	CTP2_SDL_SetRenderTarget(m_renderer, prev);
	return true;
}

bool aui_SDL::DrawWorldmapQuads(std::vector<GpuQuad> const &quads, bool clearFirst)
{
	if (!m_renderer || !m_worldmapTexture || !m_quadAtlasTexture)
		return false;
	if (quads.empty())
		return true;   // nothing dirty is a success, not a failure

	SDL_Texture * const prev = SDL_GetRenderTarget(m_renderer);
	if (!CTP2_SDL_SetRenderTarget(m_renderer, m_worldmapTexture))
		return false;

	// Clearing is a separate pass, never interleaved with drawing: a per-quad
	// clear would erase the overlapping part of a cell already drawn this batch.
	if (clearFirst)
	{
		SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
		for (GpuQuad const & q : quads)
		{
			SDL_Rect const clearRect = { q.dx, q.dy, q.dw, q.dh };
			CTP2_SDL_RenderFillRectI(m_renderer, &clearRect);
		}
	}

	for (GpuQuad const & q : quads)
	{
		// NO per-quad clear. Rows step half a tile height but the quad rect is a
		// full tile tall, so clearing a cell's rect erases the bottom half of the
		// row above it -- drawing top-to-bottom, each row wiped its predecessor
		// and only slivers survived. Tiles are alpha-blended and the target starts
		// opaque black, so they tessellate correctly without one. (A changed cell
		// therefore needs its overlapping neighbours redrawn too; that belongs to
		// the dirty-update path, not here.)
		CTP2_SDL_RenderTextureWindow(m_renderer, m_quadAtlasTexture,
			(float)q.sx, (float)q.sy, (float)q.sw, (float)q.sh,
			(float)q.dx, (float)q.dy, (float)q.dw, (float)q.dh);
	}

	CTP2_SDL_SetRenderTarget(m_renderer, prev);
	return true;
}

int aui_SDL::SampleWorldmapCoverage(int grid)
{
	// Count non-black samples on a grid x grid lattice over the whole-map
	// target, WITHOUT unbinding it between reads. Sampling from a separate
	// command would interleave with the frame loop's SDL_RenderPresent, and
	// render-target contents are not guaranteed to survive a present — so a
	// low count read later cannot distinguish "drew wrong" from "was discarded".
	if (!m_renderer || !m_worldmapTexture || grid <= 0)
		return -1;

	SDL_Texture * const prev = SDL_GetRenderTarget(m_renderer);
	if (!CTP2_SDL_SetRenderTarget(m_renderer, m_worldmapTexture))
		return -1;

	int hits = 0;
	for (int yi = 0; yi < grid; ++yi)
	{
		for (int xi = 0; xi < grid; ++xi)
		{
			int const x = (int)((int64_t)m_worldmapW * xi / grid);
			int const y = (int)((int64_t)m_worldmapH * yi / grid);
			uint32 pixel = 0;
			if (CTP2_SDL_RenderReadPixelARGB(m_renderer, x, y, &pixel)
			    && (pixel & 0x00FFFFFFu) != 0)
				++hits;
		}
	}
	CTP2_SDL_SetRenderTarget(m_renderer, prev);
	return hits;
}

void aui_SDL::DestroyWorldmapTexture()
{
	if (m_worldmapTexture)
	{
		SDL_DestroyTexture(m_worldmapTexture);
		m_worldmapTexture = nullptr;
	}
	m_worldmapW = m_worldmapH = 0;
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
		CTP2_SDL_SetTextureNearest(m_quadAtlasTexture);
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

SDL_Texture *aui_SDL::EnsureMapIconTexture(void const *data, int w, int h, uint16 color, bool blend, int blendValue, bool dither)
{
	if (!m_renderer || !data || w <= 0 || h <= 0)
		return nullptr;

	auto const key = std::make_tuple(data, w, h, color, blend, blendValue, dither);
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
						if (!dither || (((int)(dest - rgba.data()) % w + j) & 1)) {
							Pixel16 const px = blend ? pixelutils_BlendFast(*rowData, color, blendValue) : *rowData;
							*dest = pixelutils_16to8888(px) | 0xff000000u;
						}
						++dest;
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

	m_lastIconOpaquePixels = 0;
	for (uint32 px : rgba)
		if ((px >> 24) != 0)
			++m_lastIconOpaquePixels;

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
	// P13 step 0: the budget is the margin MINUS what the zoomed source window
	// already spends overshooting the screen region — not the raw margin. Using
	// the raw margin let the pan run past rendered content whenever the camera
	// was zoomed, and SDL's srcrect clipping turned that into a visible jump.
	// P13: on the whole-map path the limit is the texture edge, not a margin --
	// the camera traverses the entire map, so a +/-94px budget would pin it.
	if (GpuWorldmapEnabled() && m_worldmapTexture)
	{
		m_panTargetX = camera_window::ClampPan(m_panTargetX, ViewportW(),
			(float)m_worldmapW, (float)m_worldmapOriginX, m_cameraZoom);
		m_panTargetY = camera_window::ClampPan(m_panTargetY, ViewportH(),
			(float)m_worldmapH, (float)m_worldmapOriginY, m_cameraZoom);
		m_cameraOffX = camera_window::ClampPan(m_cameraOffX, ViewportW(),
			(float)m_worldmapW, (float)m_worldmapOriginX, m_cameraZoom);
		m_cameraOffY = camera_window::ClampPan(m_cameraOffY, ViewportH(),
			(float)m_worldmapH, (float)m_worldmapOriginY, m_cameraZoom);
	}
	else
	{
	float const limX = PanBudgetX();
	float const limY = PanBudgetY();
	auto clampPanAxis = [](float &off, float &tgt, float lim)
	{
		if (tgt >  lim) tgt =  lim;
		if (tgt < -lim) tgt = -lim;
		if (off >  lim) off =  lim;
		if (off < -lim) off = -lim;
	};
	clampPanAxis(m_cameraOffX, m_panTargetX, limX);
	clampPanAxis(m_cameraOffY, m_panTargetY, limY);
	}

	// --- Zoom: damped spring toward the home zoom (the "gravity"). ---
	// Impulses (scroll) push m_zoomVel; the spring pulls zoom back to home and
	// the damping bleeds off velocity so it settles rather than oscillating.
	// P13: NO spring on the whole-map path. The spring is the legacy rubber-band
	// peek -- zoom springs back to home because the ENGINE owned the real zoom
	// and the camera was only a transient preview. Here the camera owns zoom
	// outright, so a pull toward home would drag every zoom back to 100% and
	// make any other zoom impossible to hold. Velocity still decays so a flick
	// coasts to rest.
	if (GpuWorldmapEnabled() && m_worldmapTexture)
	{
		float const k_ZOOM_DAMPING = 9.0f;
		m_zoomVel   -= k_ZOOM_DAMPING * m_zoomVel * dtSec;
		m_cameraZoom += m_zoomVel * dtSec;
	}
	else
	{
	float const k_ZOOM_SPRING  = 55.0f;   // pull strength toward home (1/s^2)
	float const k_ZOOM_DAMPING = 9.0f;    // velocity damping (1/s)
	float const accel = k_ZOOM_SPRING * (m_homeZoom - m_cameraZoom)
	                  - k_ZOOM_DAMPING * m_zoomVel;
	m_zoomVel   += accel * dtSec;
	m_cameraZoom += m_zoomVel * dtSec;
	}

	// Clamp zoom to a sane peek range. The lower bound is not a taste choice:
	// below MinSafeZoom the source window is wider than screen+margin, so even
	// a centred window samples outside rendered content (P13 step 0). ADR-003's
	// whole-map texture removes the limit; until then it is a hard floor.
	// P13 step 2.2: ceiling is 2.0 on the whole-map path (the requested 200%,
	// shown with NEAREST as crisp pixel art); the legacy peek range keeps 2.5.
	float const k_ZOOM_MIN = MinSafeZoom();
	float const k_ZOOM_MAX = GpuWorldmapEnabled() ? 2.0f : 2.5f;
	if (m_cameraZoom < k_ZOOM_MIN) { m_cameraZoom = k_ZOOM_MIN; if (m_zoomVel < 0.0f) m_zoomVel = 0.0f; }
	if (m_cameraZoom > k_ZOOM_MAX) { m_cameraZoom = k_ZOOM_MAX; if (m_zoomVel > 0.0f) m_zoomVel = 0.0f; }

	// Settle: once motion is negligible, snap to rest so CameraMoving() clears
	// and the frame loop stops presenting.
	if (!CameraMoving())
	{
		// P13 step 2.6: on the whole-map path the camera owns zoom, so settling
		// means landing on the user's zoom -- with a detent so 100% is easy to
		// hit exactly (pixel-exact terrain). The legacy path still springs back
		// to its home zoom, which is what its rubber-band peek is for.
		if (GpuWorldmapEnabled())
			m_cameraZoom = camera_window::ZoomDetent(m_cameraZoom);
		else
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
	float const panErrX = m_panTargetX - m_cameraOffX;
	float const panErrY = m_panTargetY - m_cameraOffY;
	// P13: on the whole-map path a zoom away from home is a RESTING state, not
	// motion -- the camera owns zoom, so 1.5x held steady is where the user put
	// it. Counting it as movement would keep the frame loop presenting forever
	// and stop the settle (and its detent) from ever running.
	if (GpuWorldmapEnabled() && m_worldmapTexture)
		return (m_zoomVel > 0.01f) || (m_zoomVel < -0.01f)
		    || (panErrX  > 0.5f)   || (panErrX  < -0.5f)
		    || (panErrY  > 0.5f)   || (panErrY  < -0.5f);
	float const zoomErr = m_cameraZoom - m_homeZoom;
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
