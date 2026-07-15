#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __aui_sdl__aui_sdl__h__
#define __aui_sdl__aui_sdl__h__ 1

#include "os/include/ctp2_config.h"

#if defined(__AUI_USE_SDL__)

#include "ui/aui_sdl/aui_sdlcompat.h"

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
	static void SetCamera(float offX, float offY, float zoom)
	{ m_cameraOffX = offX; m_cameraOffY = offY; m_cameraZoom = zoom; }
	static float CameraOffX() { return m_cameraOffX; }
	static float CameraOffY() { return m_cameraOffY; }
	static float CameraZoom() { return m_cameraZoom; }

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

private:
	static sint32		m_SDLRefCount;
protected:
	static uint32           m_SDLClassId;
};

typedef aui_SDL aui_Native;

#endif // defined(__AUI_USE_SDL__)

#endif
