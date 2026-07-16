#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __aui_sdl__aui_sdl__h__
#define __aui_sdl__aui_sdl__h__ 1

#include "os/include/ctp2_config.h"

#if defined(__AUI_USE_SDL__)

#include "ui/aui_sdl/aui_sdlcompat.h"

#include <vector>

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
	// P11 Stage 2 D: per-layer GPU compositing. Off by default.
	static bool GpuLayersEnabled();
	static SDL_Texture *WorldTexture() { return m_worldTexture; }
	static SDL_Texture *UiTexture() { return m_uiTexture; }
	// P11 Stage 2 C: GPU fog-mask composited over the world texture. Requires
	// GpuLayersEnabled (needs the world on its own texture). Off by default.
	static bool GpuFogEnabled();
	static SDL_Texture *FogTexture() { return m_fogTexture; }
	// P11 Stage 2 F: smooth camera. Pan offset (screen px) + zoom applied to the
	// world + fog layers at present time (UI stays fixed). Requires
	// GpuLayersEnabled. Off by default; identity (0,0,1) is a no-op.
	static bool GpuCameraEnabled();
	// P11 2a — buttery pan substrate (ADR-001). The world surface + texture are
	// oversized by this margin (px) on each side of the screen so the viewport
	// can pan sub-tile on the GPU (a moving source rect) without revealing a
	// black edge; whole-tile ScrollMap recenters the content underneath. This is
	// the ALLOCATION margin (generous, fixed); the actual content offset the
	// world-layer render uses is whole-tile-aligned (<= this) and published via
	// SetWorldContentOffset so the present + oracle window to the exact centre.
	// 192px (4 tile-columns / 8 half-rows at default zoom): the buttery pan lets
	// the GPU camera glide this far into the margin before a whole-tile recenter,
	// so a bigger margin = fewer recenters (fewer synchronous world re-renders),
	// most noticeably on the horizontal axis where tiles are twice as wide.
	static int WorldMargin() { return 192; }
	// The whole-tile-aligned pixel offset at which TiledMap::RenderWorldLayer
	// places the screen's top-left inside the oversized world surface. The
	// present windows the screen viewport at this offset (minus CameraOff). Set
	// each frame by the render; 0 until the first oversized render.
	static void SetWorldContentOffset(int x, int y) { m_worldContentOffX = x; m_worldContentOffY = y; }
	static int WorldContentOffX() { return m_worldContentOffX; }
	static int WorldContentOffY() { return m_worldContentOffY; }
	static void SetCamera(float offX, float offY, float zoom)
	{ m_cameraOffX = offX; m_cameraOffY = offY; m_cameraZoom = zoom; }
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

	// P11 Stage 3 G1 — terrain quad renderer. When enabled, terrain is drawn as
	// GPU textured quads from a tile atlas into the world texture (a render
	// target) instead of uploading the CPU-composited world surface. The D/C/F
	// composite then pans/zooms/fogs that world texture unchanged. Requires
	// GpuLayersEnabled. Off by default.
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
	static void BeginQuadFrame() { m_quadDrawList.clear(); }
	static void AddQuad(GpuQuad const &q) { m_quadDrawList.push_back(q); }
	static std::vector<GpuQuad> const &QuadDrawList() { return m_quadDrawList; }

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
	// P11 2b: whole-tile-aligned pixel offset of the screen's top-left inside the
	// oversized world surface (published by RenderWorldLayer; consumed by present).
	static int		m_worldContentOffX;
	static int		m_worldContentOffY;
	// P11 F: momentum physics state (velocities + spring anchor).
	static float		m_panVelX;
	static float		m_panVelY;
	static float		m_zoomVel;
	static float		m_homeZoom;
	// P11 2c: buttery-pan follow target (the displayed CameraOff eases toward this).
	static float		m_panTargetX;
	static float		m_panTargetY;
	// P11 G1: terrain quad atlas (source) + the per-frame cell draw list.
	static SDL_Texture *	m_quadAtlasTexture;
	static int		m_quadAtlasW;
	static int		m_quadAtlasH;
	static std::vector<GpuQuad> m_quadDrawList;

private:
	static sint32		m_SDLRefCount;
protected:
	static uint32           m_SDLClassId;
};

typedef aui_SDL aui_Native;

#endif // defined(__AUI_USE_SDL__)

#endif
