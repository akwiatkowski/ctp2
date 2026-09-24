#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __aui_sdl__aui_sdl__h__
#define __aui_sdl__aui_sdl__h__ 1

#include "os/include/ctp2_config.h"

#if defined(__AUI_USE_SDL__)

#include "ui/aui_sdl/aui_sdlcompat.h"
#include "ui/aui_common/camera_window.h"   // P13 step 0: pan budget / safe zoom

#include <map>
#include <tuple>
#include <vector>

class aui_Surface;   // hardware-cursor conversion source
class ModernSpriteAtlas;

class aui_SDL
{
public:

	aui_SDL() { m_SDLRefCount++; m_exclusiveMode = FALSE; }
	virtual ~aui_SDL();

protected:
	AUI_ERRCODE InitCommon( BOOL useExclusiveMode );

public:
	virtual BOOL IsThisA( uint32 classId )
	{
		return classId == m_SDLClassId;
	}

	BOOL GetExclusiveMode() { return m_exclusiveMode; }
	static SDL_Surface *DD() { return m_lpdd; }
	// Read-only access to the GPU present layer, for the test API's
	// presented-frame readback (screenshot_presented).
	static SDL_Renderer *Renderer() { return m_renderer; }
	static SDL_Window *Window() { return m_window; }
	static SDL_Texture *ScreenTexture() { return m_screenTexture; }
	// Smoke-test-only readback of the normal frame, immediately before present.
	static void EnableFrameCapture() { m_captureFrames = true; }
	static void CaptureFrameBeforePresent();
	static SDL_Surface *CapturedFrame() { return m_capturedFrame; }
	static Uint64 CapturedFrameSequence() { return m_capturedFrameSequence; }
	// P11 Stage 2 D: per-layer GPU compositing. Default-on; CTP2_GPU_LAYERS=0 opts out.
	static bool GpuLayersEnabled();
	static SDL_Texture *WorldTexture() { return m_worldTexture; }
	static SDL_Texture *UiTexture() { return m_uiTexture; }
	// P11 Stage 2 C: GPU fog-mask composited over the world texture. Requires
	// GpuLayersEnabled (needs the world on its own texture). Off by default.
	static bool GpuFogEnabled();
	static SDL_Texture *FogTexture() { return m_fogTexture; }
	// P11 Stage 2 F: smooth camera. Pan offset (screen px) + zoom applied to the
	// world + fog layers at present time (UI stays fixed). Requires
	// GpuLayersEnabled. Default-on; CTP2_GPU_CAMERA=0 opts out. Identity (0,0,1) is a no-op.
	static bool GpuCameraEnabled();
	// P11 pan polish — hardware (OS) cursor. In the layered GPU present the
	// legacy software cursor is poison: every move "restores" a pickup of
	// SECONDARY pixels through BltToSecondary, which bakes screen-coord map
	// fragments into the persistent UI layer — stale squares that float over
	// the sliding world (cursor strobing, units apparently in wrong tiles).
	// With layers on, the OS cursor is shown instead and ALL software cursor
	// mixing is skipped (see aui_Mouse).
	static bool HardwareCursorEnabled() { return GpuLayersEnabled(); }
	// Convert a chroma-keyed game cursor image (16 or 32 bpp aui surface)
	// and install it as the OS cursor; null hides the OS cursor.
	static void SetHardwareCursor(aui_Surface *surf, int hotX, int hotY);
	// P11 2a — buttery pan substrate (ADR-001). The world surface + texture are
	// oversized by this margin (px) on each side of the screen so the viewport
	// can pan sub-tile on the GPU (a moving source rect) without revealing a
	// black edge; whole-tile ScrollMap recenters the content underneath. This is
	// the ALLOCATION margin; it must be >= the background window's own margin
	// (94/72 px, see WorldContentOff below) since the world layer is a 1:1
	// mirror of that window. 192px keeps headroom for both axes.
	static int WorldMargin() { return 192; }
	// The pixel offset at which the screen's top-left sits inside the world
	// layer. The world layer is an identity mirror of the legacy background
	// window surface, which is allocated one tile-grid larger than the screen
	// on each side and positioned at (-k_TILE_GRID_WIDTH, -k_TILE_GRID_HEIGHT)
	// — so the offset IS that margin, a fixed property of the window layout
	// (independent of zoom, which only changes how many tiles the margin
	// covers). backgroundWin_Initialize static_asserts these against the
	// tileset constants. The present windows the screen viewport at this
	// offset minus CameraOff; the sub-tile glide may slide at most this far
	// before a whole-tile recenter pulls the offset back.
	static constexpr int WorldContentOffX() { return 94; }   // k_TILE_GRID_WIDTH
	static constexpr int WorldContentOffY() { return 72; }   // k_TILE_GRID_HEIGHT
	// P13 step 0 — the screen size the camera windows out of the world texture.
	// Needed because the pan budget and the safe zoom range both depend on it
	// (see camera_window.h); set once when the layer textures are created.
	static void SetViewportSize(int w, int h) { m_viewportW = w; m_viewportH = h; }
	static float ViewportW() { return static_cast<float>(m_viewportW); }
	static float ViewportH() { return static_cast<float>(m_viewportH); }
	// How far the camera may pan on each axis at the CURRENT zoom before the
	// present samples outside rendered content. At zoom 1 this is just the
	// margin; zooming in eats into it (see camera_window.h for why).
	static float PanBudgetX() { return camera_window::PanBudgetPx(ViewportW(), (float)WorldContentOffX(), m_cameraZoom); }
	static float PanBudgetY() { return camera_window::PanBudgetPx(ViewportH(), (float)WorldContentOffY(), m_cameraZoom); }
	// The lowest zoom the rendered margin can supply on BOTH axes. Below this
	// even a centred window overruns, so every camera-zoom writer clamps to it.
	// P13 step 2.2 — with the whole map in one texture there is no margin to
	// overrun, so the floor is "the whole map fits the viewport" instead of the
	// window-mirror margin bound. Map-size dependent (~0.21 on Gigantic at
	// 1920x1080, ~0.63 on Small), so it is computed, never hardcoded.
	static float WorldmapFitZoom()
	{
		if (m_worldmapW <= 0 || m_worldmapH <= 0) return 1.0f;
		float const zx = ViewportW() / static_cast<float>(m_worldmapW);
		float const zy = ViewportH() / static_cast<float>(m_worldmapH);
		return (zx < zy) ? zx : zy;
	}
	static float MinSafeZoom()
	{
		// Whole-map path: bounded by fitting the map, not by a rendered margin.
		if (GpuWorldmapEnabled() && m_worldmapTexture)
			return WorldmapFitZoom();
		float const zx = camera_window::MinSafeZoom(ViewportW(), (float)WorldContentOffX());
		float const zy = camera_window::MinSafeZoom(ViewportH(), (float)WorldContentOffY());
		return (zx > zy) ? zx : zy;
	}
	static void SetCamera(float offX, float offY, float zoom)
	{
		float const zMin = MinSafeZoom();
		m_cameraOffX = offX; m_cameraOffY = offY;
		m_cameraZoom = (zoom < zMin) ? zMin : zoom;
	}
	// TEMPORARY (P11 pixel-proof debug): set the pan offset directly, bypassing
	// velocity/target logic so a test harness can force a known camera state.
	static void SetCameraOffset(float offX, float offY)
	{ m_cameraOffX = offX; m_cameraOffY = offY; }
	static float CameraOffX() { return m_cameraOffX; }
	static float CameraOffY() { return m_cameraOffY; }
	static float CameraZoom() { return m_cameraZoom; }

	// P11 2c (ADR-001) — buttery pan target. The trackpad accumulates the commanded
	// pan into this TARGET offset (screen px); TickCamera eases the displayed
	// CameraOff toward it every frame, so the motion is smooth at the frame rate and
	// decoupled from the bursty (large-delta) trackpad event stream. On a whole-tile
	// recenter, ShiftPan slides BOTH the displayed offset and the target by the same
	// pixels so the ease continues seamlessly across the ScrollMap.
	static void AddPanTarget(float dx, float dy)
	{
		m_panTargetX += dx;
		m_panTargetY += dy;
	}
	static void SetPanTarget(float x, float y)
	{
		m_panTargetX = x;
		m_panTargetY = y;
	}
	static void ShiftPan(float dx, float dy)
	{
		m_cameraOffX += dx;
		m_cameraOffY += dy;
		m_panTargetX += dx;
		m_panTargetY += dy;
	}
	static float PanTargetX() { return m_panTargetX; }
	static float PanTargetY() { return m_panTargetY; }
	static void ResetPan()
	{
		m_cameraOffX = m_cameraOffY = 0.0f;
		m_panTargetX = m_panTargetY = 0.0f;
		m_panVelX = m_panVelY = 0.0f;
	}
	// P11 Stage 2 F — momentum camera physics. Input adds velocity impulses;
	// TickCamera integrates them each frame. Pan glides to rest under friction
	// and stays put; zoom is spring-loaded toward the "home" zoom so it eases
	// back after a scroll (a rubber-band peek). CameraMoving() is true while the
	// camera is still settling (so the frame loop keeps presenting).
	static void AddZoomImpulse(float delta) { m_zoomVel += delta; }
	static void AddPanImpulse(float dx, float dy) { m_panVelX += dx; m_panVelY += dy; }
	static void SetHomeZoom(float z) { m_homeZoom = z; }
	static void TickCamera(float dtSec);
	static bool CameraMoving();

	// P11 Stage 3 G1 / P12 — GPU world renderer. When enabled, terrain and the
	// first modern-atlas unit sprites are drawn as GPU textured quads into the
	// world texture instead of uploading the CPU-composited world surface. The
	// D/C/F composite then pans/zooms/fogs that world texture unchanged. Requires
	// GpuLayersEnabled. Default-on; CTP2_GPU_QUADS=0 temporarily opts out.
	static bool GpuQuadsEnabled();
	static SDL_Texture *QuadAtlasTexture() { return m_quadAtlasTexture; }
	// Lazily create the atlas texture (ARGB8888 streaming, alpha-blended so the
	// transparent diamond surround tessellates). No-op if already sized to match.
	static void EnsureQuadAtlas(int atlasW, int atlasH);
	// Upload one composited tile's pixels (ARGB8888) into an atlas slot rect.
	static void UploadQuadAtlasSlot(int x, int y, int w, int h,
	                                void const *pixels, int pitch);

	// P14 (GPU rasterisation): the whole-map terrain composite moves off the
	// 1999 CPU rasteriser. Instead of compositing base + transition strips into
	// a scratch surface per distinct cell appearance, the tileset itself is
	// decoded ONCE into this static atlas — base tiles with their transition
	// markers left transparent, plus transition strips pre-splatted into
	// diamond positions — and a cell becomes 1-5 GPU quads. The space is
	// additive (tiles + strips), not multiplicative (combinations), so nothing
	// here is ever evicted. Opt-in via CTP2_GPU_RASTER until parity is proven.
	static bool GpuRasterEnabled();
	static SDL_Texture *TilesetAtlasTexture() { return m_tilesetAtlasTexture; }
	static bool EnsureTilesetAtlas(int w, int h);
	static void UploadTilesetAtlasRect(int x, int y, int w, int h,
	                                   void const *pixels, int pitch);

	// One terrain cell to draw: atlas source rect -> world-texture dest rect.
	// tex: which texture the quad samples. nullptr means the shared quad atlas
	// (the overwhelmingly common case, and the only one the window-mirror path
	// ever uses). The GPU-raster path (P14) sets it to the tileset atlas so a
	// cell can composite as base + transition-strip quads with no per-cell
	// upload at all.
	enum class QuadOperation { Draw, BeginCell, EndCell };
	struct GpuQuad { int sx, sy, sw, sh; int dx, dy, dw, dh;
	                 SDL_Texture *tex = nullptr;
	                 SDL_BlendMode blend = SDL_BLENDMODE_BLEND;
	                 QuadOperation operation = QuadOperation::Draw; };
	// The per-frame draw list is rebuilt by the tile pass (BeginQuadFrame +
	// AddQuad) and consumed by the present (QuadDrawList). It persists between
	// presents so camera-only frames reuse it without a rebuild.
	// P13 close-out: quad/sprite lists are rebuilt every Draw but nothing
	// versioned them, so Flip's redundant-present check skipped presents
	// after list-only changes (new selection brackets, fresh fixtures) and
	// the oracle read stale textures. Bump on every mutation; Flip treats a
	// version change like a content change.
	static uint32 QuadListVersion() { return m_quadListVersion; }
	static void AddQuad(GpuQuad const &q) { m_quadDrawList.push_back(q); ++m_quadListVersion; }
	static void BeginQuadFrame() { m_quadDrawList.clear(); m_quadFrameComplete = true; m_quadFrameIncompleteReason = nullptr; ++m_quadListVersion; }
	// P13 step 1 (ADR-003) — whole-map GPU render target. Opt-in via
	// CTP2_GPU_WORLDMAP; the ADR-002 window mirror stays the default path.
	// Instead of a screen+margin texture rebuilt as the view scrolls, the ENTIRE
	// map lives in one render target in absolute map-pixel coordinates, and only
	// tiles whose content actually changed are redrawn. Pan and zoom then never
	// touch it. Measured feasible: Gigantic is 6,580x5,040 (~133MB) against a
	// 16,384^2 limit (debug_gpu_worldmap_probe).
	static bool GpuWorldmapEnabled();
	static SDL_Texture *WorldmapTexture() { return m_worldmapTexture; }
	// Where the screen's top-left sits inside the whole-map texture, in map
	// pixels. Published each build by TiledMap; the present windows here.
	static void SetWorldmapOrigin(int x, int y) { m_worldmapOriginX = x; m_worldmapOriginY = y; }
	// View margin, in whole-map pixels: how far the visible (screen-centred)
	// window sits inside the engine view. The view carries scroll margins, so
	// its corner is NOT the screen corner; every windowing consumer (present,
	// sprites, pick, readback) must add this after the origin. Published with
	// the origin; zero when the view fits the screen.
	static void SetWorldmapMargin(int x, int y) { m_worldmapMarginX = x; m_worldmapMarginY = y; }
	static int WorldmapMarginX() { return m_worldmapMarginX; }
	static int WorldmapMarginY() { return m_worldmapMarginY; }
	// Whole-map pixel that view-relative (0,0) maps to, for SPRITES.
	//
	// Same projection as WorldmapOrigin, published separately because it answers
	// a different question: where the tile builder DREW, versus where the present
	// WINDOWS. They used to differ by k_TILE_PIXEL_HEADROOM (a phantom -- see
	// BuildWorldmapQuads) and by a quarter tile in X (a real stride bug). Both
	// are fixed, so the two now agree; keeping them separate means a future
	// divergence stays observable in query_gpu_world instead of silently
	// misplacing every sprite by the difference.
	static void SetWorldmapSpriteBase(int x, int y) { m_worldmapSpriteBaseX = x; m_worldmapSpriteBaseY = y; }
	static int WorldmapSpriteBaseX() { return m_worldmapSpriteBaseX; }
	static int WorldmapSpriteBaseY() { return m_worldmapSpriteBaseY; }
	static int WorldmapOriginX() { return m_worldmapOriginX; }
	// Test hook: turn the whole-map sprite pass off so a single run can
	// capture the same map with and without it. New games generate a random
	// map and loading one loses good actors, so an A/B across processes
	// cannot hold the map fixed.
	static void SetWorldmapSprites(bool on) { m_worldmapSpritesOn = on; }
	static bool m_worldmapSpritesOn;
	static int LastIconOpaquePixels() { return m_lastIconOpaquePixels; }
	static int m_lastIconOpaquePixels;
	static int WorldmapOriginY() { return m_worldmapOriginY; }
	static int WorldmapW() { return m_worldmapW; }
	static int WorldmapH() { return m_worldmapH; }
	// Horizontal period of the whole-map texture: mapWidth column strides. NOT
	// WorldmapW(), which is one stride wider so the half-stride overhang of odd
	// rows has somewhere to land. The map wraps in X, so texture content
	// satisfies content(x) == content(x + wrap) and any sampling must be taken
	// modulo this.
	static void SetWorldmapWrap(int w) { m_worldmapWrapW = w; }
	static int WorldmapWrapW() { return m_worldmapWrapW; }
	// Present the whole-map target into the current render target, windowed at
	// WorldmapOrigin, zoomed and panned by the camera, wrapping across the map's
	// X seam.
	//
	// This exists as ONE function on purpose. Both aui_SDLSurface::Flip and the
	// screenshot_presented readback have to produce identical pixels -- the
	// readback is the only pixel oracle for this path, and when it last drifted
	// from Flip it re-composited the wrong texture entirely, leaving every
	// screenshot test blind to the whole-map path for a whole phase. Two copies
	// of this arithmetic is how that happens; there is now one.
    static bool WindowQuadsReady() { return GpuQuadsEnabled() && QuadAtlasTexture() && QuadFrameComplete(); }
    static bool WholeMapReady() { return GpuWorldmapEnabled() && WorldmapTexture() && !SpriteFrameIncompleteReason(); }
    static void PresentWorldFrame(SDL_Renderer *renderer, float w, float h, float zoom, float offX, float offY);
	// True when Flip presents the layered GPU composite (world/UI textures)
	// instead of the software secondary surface. The secondary-to-primary
	// mirror then serves only the screenshot oracle, which refreshes it
	// synchronously on demand (see the screenshot handlers in civapp.cpp) —
	// per-frame mirroring may be skipped. Mirrors Flip's `layered` condition.
	static bool LayeredPresentActive();
	static void PresentWorldmapWindow(SDL_Renderer *renderer,
	                                  float viewW, float viewH,
	                                  float zoom, float offX, float offY);
	// Create (or resize) the whole-map target and clear it to opaque black, the
	// same "unexplored" base the window-mirror path clears to. Returns false if
	// the driver refuses the size, so callers can fall back rather than draw
	// into nothing.
	static bool EnsureWorldmapTexture(int w, int h);
	// Draw a batch of atlas->map-space quads into the whole-map target. Separate
	// from the present: these land when content changes, not when the camera
	// moves. Returns false if the target is missing or cannot be bound.
	//
	// The two lists are deliberately DIFFERENT sets. `clears` is the cells whose
	// own content changed; `quads` is those plus every cell whose diamond
	// overlaps one of the cleared rects. Using one list for both is what made an
	// incremental update lose 11% of the map: whatever set gets cleared, the
	// cells just outside it painted into those rects too, so a set that clears
	// exactly what it redraws always erases its own border. Widening such a set
	// cannot help — it only moves the border outwards.
	static bool DrawWorldmapQuads(std::vector<GpuQuad> const &clears,
	                              std::vector<GpuQuad> const &quads);
	// Count non-black samples on a grid x grid lattice over the whole-map target,
	// sampled while the target stays bound (see the .cpp for why that matters).
	// Returns -1 if there is no target.
	static int SampleWorldmapCoverage(int grid);
	static void DestroyWorldmapTexture();
	static void MarkQuadFrameIncomplete(char const *reason = nullptr);
	static char const *SpriteFrameIncompleteReason() { return m_spriteFrameIncompleteReason; }
	static bool QuadFrameComplete() { return m_quadFrameComplete && !m_spriteFrameIncompleteReason; }
	static char const *QuadFrameIncompleteReason() { return m_quadFrameIncompleteReason ? m_quadFrameIncompleteReason : m_spriteFrameIncompleteReason; }
	static void MarkSpriteFrameIncomplete(char const *reason) { if (!m_spriteFrameIncompleteReason) m_spriteFrameIncompleteReason = reason; }
	static std::vector<GpuQuad> const &QuadDrawList() { return m_quadDrawList; }
	struct GpuSpriteQuad { SDL_Texture *texture; int sx, sy, sw, sh; int dx, dy, dw, dh; bool mirror; uint8 alpha; uint8 red = 255, green = 255, blue = 255; bool additive = false; bool screen_space = false; };
	static void BeginSpriteFrame() { m_spriteDrawList.clear(); m_spriteFrameIncompleteReason = nullptr; ++m_spriteListVersion; }
	static void AddSpriteQuad(GpuSpriteQuad const &q) { m_spriteDrawList.push_back(q); ++m_spriteListVersion; }
	static uint32 SpriteListVersion() { return m_spriteListVersion; }
	static SDL_Texture *EnsureSpriteAtlasTexture(ModernSpriteAtlas const *atlas, bool desaturate = false);
	static void ReleaseSpriteAtlasTexture(ModernSpriteAtlas const *atlas);
	static SDL_Texture *EnsureMapIconTexture(void const *data, int w, int h, uint16 color, bool blend = false, int blendValue = 0, bool dither = false);
	static SDL_Texture *EnsureSolidColorTexture(uint16 color);
	static std::vector<GpuSpriteQuad> const &SpriteDrawList() { return m_spriteDrawList; }

	// P13 step 3 (ADR-003): draw the world-space sprite quads -- units, cities,
	// effects -- for the whole-map path.
	//
	// They cannot be composited INTO the whole-map texture the way terrain is.
	// That texture is persistent and dirty-tracked, redrawing only cells whose
	// signature changed, so anything that moves would smear a permanent trail
	// across it. Sprites are therefore drawn at present time, on top of the
	// windowed terrain, every frame.
	//
	// Their coordinates arrive in WORLD-TEXTURE space (view-relative, plus the
	// fixed content margin). Whole-map space is the same scale, because the
	// engine is pinned at zoom 1 on this path and the camera owns zoom, so the
	// conversion is a single offset:
	//
	//     worldmap = worldTexture - WorldContentOff + WorldmapOrigin
	//
	// That is the identity picking already depends on -- MousePointToTilePos
	// reads "texture position minus the published origin is the view-relative
	// pixel". Screen position then applies the same windowing the terrain used,
	// so sprites and tiles pan and zoom together by construction.
	//
	// Shared by the real present and by screenshot_presented's readback. Those
	// two drifting apart is exactly how the whole-map path stayed invisible to
	// every pixel test until 5769503f.
	static void RenderWorldmapSpriteQuads(SDL_Renderer *renderer,
	                                      float viewW, float viewH,
	                                      float zoom, float offX, float offY);

protected:
	BOOL			m_exclusiveMode;
	static SDL_Surface *	m_lpdd;
	static SDL_Window *	m_window;
	// GPU present layer (hybrid): the engine still renders into the software
	// `primary` surface; Flip() uploads it to m_screenTexture and the renderer
	// presents/scales it on the GPU (Metal on macOS, GL/Vulkan on Linux —
	// portable SDL2, no per-platform code). Statics: one window/renderer.
	static SDL_Renderer *	m_renderer;
	static SDL_Texture *	m_screenTexture;
	static SDL_Texture *	m_worldTexture;   // P11 D: world layer (terrain+units)
	static SDL_Texture *	m_uiTexture;      // P11 D: UI layer (alpha over world)
	static SDL_Texture *	m_fogTexture;     // P11 C: fog mask (alpha over world)
	// P11 F: smooth-camera transform for the world+fog layers (identity = no-op).
	static float		m_cameraOffX;
	static float		m_cameraOffY;
	static float		m_cameraZoom;
	// P11 F: momentum physics state (velocities + spring anchor).
	static float		m_panVelX;
	static float		m_panVelY;
	static float		m_zoomVel;
	static float		m_homeZoom;
	// P11 2c: buttery-pan follow target (the displayed CameraOff eases toward this).
	static float		m_panTargetX;
	static float		m_panTargetY;
	// P13 step 0: screen size the camera windows out of the world texture.
	static int		m_viewportW;
	static int		m_viewportH;
	// P13 step 1: whole-map render target (ADR-003). Null unless opted in.
	static SDL_Texture *	m_worldmapTexture;
	static int		m_worldmapW;
	static int		m_worldmapH;
	static int		m_worldmapWrapW;
	static int		m_worldmapOriginX;
	static int		m_worldmapMarginX;
	static int		m_worldmapMarginY;
	static int m_worldmapSpriteBaseX;
	static int m_worldmapSpriteBaseY;
	static int		m_worldmapOriginY;
	// P11 G1: terrain quad atlas (source) + the per-frame cell draw list.
	static SDL_Texture *	m_quadAtlasTexture;
	static int		m_quadAtlasW;
	static int		m_quadAtlasH;
	// P14: static decoded-tileset atlas (base layers + pre-splatted strips).
	static SDL_Texture *	m_tilesetAtlasTexture;
	static int		m_tilesetAtlasW;
	static int		m_tilesetAtlasH;
	static bool		m_quadFrameComplete;
	static char const *	m_quadFrameIncompleteReason;
	static inline char const *m_spriteFrameIncompleteReason = nullptr;
	static std::vector<GpuQuad> m_quadDrawList;
	static uint32 m_quadListVersion;
	static SDL_Texture *m_rasterCellTexture;
	static int m_rasterCellW, m_rasterCellH;
	static std::map<ModernSpriteAtlas const *, SDL_Texture *> m_spriteAtlasTextures;
	static std::map<ModernSpriteAtlas const *, SDL_Texture *> m_desaturatedSpriteAtlasTextures;
	static std::map<std::tuple<void const *, int, int, uint16, bool, int, bool>, SDL_Texture *> m_mapIconTextures;
	static std::map<uint16, SDL_Texture *> m_solidColorTextures;
	static std::vector<GpuSpriteQuad> m_spriteDrawList;

	static uint32 m_spriteListVersion;
private:
	static inline bool m_captureFrames = false;
	static inline SDL_Surface *m_capturedFrame = nullptr;
	static inline Uint64 m_capturedFrameSequence = 0;
	static sint32		m_SDLRefCount;
protected:
	static uint32           m_SDLClassId;
};

typedef aui_SDL aui_Native;

#endif // defined(__AUI_USE_SDL__)

#endif
