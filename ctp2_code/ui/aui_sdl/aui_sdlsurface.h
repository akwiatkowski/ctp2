#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __aui_sdl__aui_sdlsurface_h__
#define __aui_sdl__aui_sdlsurface_h__ 1

#include "os/include/ctp2_config.h"

#if defined(__AUI_USE_SDL__)

#include "ui/aui_common/aui_surface.h"
#include "ui/aui_sdl/aui_sdl.h"

class aui_SDLSurface : public aui_Surface, public aui_SDL {
public:
	aui_SDLSurface(
	               AUI_ERRCODE *retval,
	               sint32 width,
	               sint32 height,
	               sint32 bpp,
	               SDL_Surface* lpdds = nullptr,
	               BOOL isPrimary = FALSE,
	               BOOL useVideoMemory = FALSE,
	               BOOL takeOwnership = FALSE );
        ~aui_SDLSurface() override;

protected:
        aui_SDLSurface() : aui_Surface() {}
        AUI_ERRCODE InitCommon( );

public:
	BOOL IsThisA( uint32 classId ) override {
            return ((classId == m_SDLSurfaceClassId)
                    || aui_Surface::IsThisA( classId )
                    || aui_SDL::IsThisA( classId ));
            }

	uint32 SetChromaKey( uint32 color ) override;

	AUI_ERRCODE Lock( RECT *rect, LPVOID *buffer, DWORD flags ) override;
	AUI_ERRCODE Unlock( LPVOID buffer ) override;

	SDL_Surface*    DDS( ) const { return m_lpdds; }
	BOOL                            IsDCGot( ) const { return m_dcIsGot
; }

	BOOL IsOK( ) const override;
	AUI_ERRCODE Blank(const uint32 &color) override;
	void Flip(RECT const *dirty = nullptr) override;

	static uint32 m_SDLSurfaceClassId;

	SDL_mutex* m_bltMutex;

protected:
	SDL_Surface* m_lpdds;
};

typedef aui_SDLSurface aui_NativeSurface;

#endif // defined(__AUI_USE_SDL__)

#endif
