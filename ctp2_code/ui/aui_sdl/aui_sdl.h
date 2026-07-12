#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __aui_sdl__aui_sdl__h__
#define __aui_sdl__aui_sdl__h__ 1

#include "os/include/ctp2_config.h"

#if defined(__AUI_USE_SDL__)

#include <SDL2/SDL.h>

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

private:
	static sint32		m_SDLRefCount;
protected:
	static uint32           m_SDLClassId;
};

typedef aui_SDL aui_Native;

#endif // defined(__AUI_USE_SDL__)

#endif
