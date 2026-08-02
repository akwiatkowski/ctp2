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
	static SDL_Texture *ScreenTexture() { return m_screenTexture; }
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
	static float MinSafeZoom()
	{
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
	static void AddPanTarget(float dx, float dy) { m_panTargetX += dx; m_panTargetY += dy; }
	static void SetPanTarget(float x, float y)   { m_panTargetX = x;  m_panTargetY = y; }
	static void ShiftPan(float dx, float dy)
	{ m_cameraOffX += dx; m_cameraOffY += dy; m_panTargetX += dx; m_panTargetY += dy; }
	static float PanTargetX() { return m_panTargetX; }
	static float PanTargetY() { return m_panTargetY; }
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

	// One terrain cell to draw: atlas source rect -> world-texture dest rect.
	struct GpuQuad { int sx, sy, sw, sh; int dx, dy, dw, dh; };
	// The per-frame draw list is rebuilt by the tile pass (BeginQuadFrame +
	// AddQuad) and consumed by the present (QuadDrawList). It persists between
	// presents so camera-only frames reuse it without a rebuild.
	static void BeginQuadFrame() { m_quadDrawList.clear(); m_quadFrameComplete = true; m_quadFrameIncompleteReason = nullptr; }
	static void AddQuad(GpuQuad const &q) { m_quadDrawList.push_back(q); }
	static void MarkQuadFrameIncomplete(char const *reason = nullptr);
	static bool QuadFrameComplete() { return m_quadFrameComplete; }
	static char const *QuadFrameIncompleteReason() { return m_quadFrameIncompleteReason; }
	static std::vector<GpuQuad> const &QuadDrawList() { return m_quadDrawList; }

	struct GpuSpriteQuad { SDL_Texture *texture; int sx, sy, sw, sh; int dx, dy, dw, dh; bool mirror; uint8 alpha; uint8 red = 255, green = 255, blue = 255; bool additive = false; bool screen_space = false; };
	static SDL_Texture *EnsureSpriteAtlasTexture(ModernSpriteAtlas const *atlas, bool desaturate = false);
	static void ReleaseSpriteAtlasTexture(ModernSpriteAtlas const *atlas);
	static SDL_Texture *EnsureMapIconTexture(void const *data, int w, int h, uint16 color, bool blend = false, int blendValue = 0, bool dither = false);
	static SDL_Texture *EnsureSolidColorTexture(uint16 color);
	static void BeginSpriteFrame() { m_spriteDrawList.clear(); }
	static void AddSpriteQuad(GpuSpriteQuad const &q) { m_spriteDrawList.push_back(q); }
	static std::vector<GpuSpriteQuad> const &SpriteDrawList() { return m_spriteDrawList; }

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
	// P11 G1: terrain quad atlas (source) + the per-frame cell draw list.
	static SDL_Texture *	m_quadAtlasTexture;
	static int		m_quadAtlasW;
	static int		m_quadAtlasH;
	static bool		m_quadFrameComplete;
	static char const *	m_quadFrameIncompleteReason;
	static std::vector<GpuQuad> m_quadDrawList;
	static std::map<ModernSpriteAtlas const *, SDL_Texture *> m_spriteAtlasTextures;
	static std::map<ModernSpriteAtlas const *, SDL_Texture *> m_desaturatedSpriteAtlasTextures;
	static std::map<std::tuple<void const *, int, int, uint16, bool, int, bool>, SDL_Texture *> m_mapIconTextures;
	static std::map<uint16, SDL_Texture *> m_solidColorTextures;
	static std::vector<GpuSpriteQuad> m_spriteDrawList;

private:
	static sint32		m_SDLRefCount;
protected:
	static uint32           m_SDLClassId;
};

typedef aui_SDL aui_Native;

#endif // defined(__AUI_USE_SDL__)

#endif
