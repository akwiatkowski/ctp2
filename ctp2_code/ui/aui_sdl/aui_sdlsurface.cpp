#include "os/include/ctp2_config.h"
#include "ctp/c3.h"

#ifdef __AUI_USE_SDL__

#include "ui/aui_common/aui_ui.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_sdl/aui_sdlsurface.h"
#include <SDL2/SDL_thread.h>

uint32 aui_SDLSurface::m_SDLSurfaceClassId = aui_UniqueId();

aui_SDLSurface::aui_SDLSurface(
	AUI_ERRCODE *retval,
	sint32 width,
	sint32 height,
	sint32 bpp,
	SDL_Surface* lpdds,
	BOOL isPrimary,
	BOOL useVideoMemory,
	BOOL takeOwnership )
	:
	aui_Surface()
{
	m_bltMutex = SDL_CreateMutex();
	if (lpdds != nullptr && takeOwnership) {
		width = lpdds->w;
		height = lpdds->h;
		bpp = lpdds->format->BitsPerPixel;
	}
	// If wrapping an existing surface (like the window surface), use its actual bpp
	// so m_Bpp and offset calculations are correct
	if (lpdds != nullptr && !takeOwnership) {
		bpp = lpdds->format->BitsPerPixel;
	}
	*retval = aui_Surface::InitCommon( width, height, bpp, isPrimary );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommon();
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	if ( !(m_lpdds = lpdds) )
	{
		// Create the surface with the requested bpp. The game renders in
		// 16-bit (RGB565); the secondary stays 16-bit and SDL_BlitSurface
		// converts on the blit to the primary. The primary is now a standalone
		// 32-bit ARGB8888 surface (NOT the window surface — incompatible with
		// the SDL_Renderer), which Flip() uploads to the GPU screen texture.
		// Explicit ARGB8888 masks avoid SDL_GetWindowSurface (which fails once
		// a renderer exists) and match the SDL_PIXELFORMAT_ARGB8888 texture.
		if (bpp == 16) {
			m_lpdds = SDL_CreateRGBSurface(0, width, height, 16, 0xF800, 0x07E0, 0x001F, 0);
		} else {
			m_lpdds = SDL_CreateRGBSurface(0, width, height, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
		}
		if ( m_lpdds == nullptr )
		{
			*retval = AUI_ERRCODE_MEMALLOCFAILED;
			return;
		}
		m_allocated = TRUE;
		// clear it with black
		if (SDL_FillRect(m_lpdds, nullptr, 0) < 0) {
			fprintf(stderr, "aui_Surface: Failed to erase new surface: %s\n",
			SDL_GetError());
		}
	}
	else
            {
	    m_allocated = takeOwnership;
            }

	// Detect pixel format from the ACTUAL surface, not the window format
	SDL_PixelFormat* actualFmt = m_lpdds->format;
	if ((actualFmt->Rmask >> actualFmt->Rshift == 0x1F) && (actualFmt->Gmask >> actualFmt->Gshift == 0x3F) && (actualFmt->Bmask >> actualFmt->Bshift == 0x1F)) {
            m_pixelFormat = AUI_SURFACE_PIXELFORMAT_565;
            //printf("%s L%d: AUI_SURFACE_PIXELFORMAT_565\n", __FILE__, __LINE__);
            }
        if ((actualFmt->Rmask >> actualFmt->Rshift == 0x1F) && (actualFmt->Gmask >> actualFmt->Gshift == 0x1F) && (actualFmt->Bmask >> actualFmt->Bshift == 0x1F)) {
            m_pixelFormat = AUI_SURFACE_PIXELFORMAT_555;
            //printf("%s L%d: AUI_SURFACE_PIXELFORMAT_555\n", __FILE__, __LINE__);
            }

	m_pitch = m_lpdds->pitch;
	m_size = m_pitch * m_height;
}


AUI_ERRCODE aui_SDLSurface::InitCommon( )
{
	m_lpdds = nullptr;

	return AUI_ERRCODE_OK;
}


aui_SDLSurface::~aui_SDLSurface()
{
	if ( m_allocated && m_lpdds )
	{
		SDL_FreeSurface(m_lpdds);
		m_lpdds = nullptr;
		m_allocated = FALSE;
	}
	SDL_DestroyMutex(m_bltMutex);
}


uint32 aui_SDLSurface::SetChromaKey( uint32 color ) {
    int hr = SDL_SetColorKey(m_lpdds, SDL_TRUE, color); //|SDL_RLEACCEL ?
    //hr == 0 if succeded!
    //printf("%s L%d: SDL_SRCCOLORKEY set to %#X\n", __FILE__, __LINE__, color);

    if ( hr == 0 )
        return aui_Surface::SetChromaKey( color ); //sets aui_Surface.m_chromaKey and returns last value!

    //return AUI_ERRCODE_OK;  //this is not sensible, should retrun last color key!?!
    printf("%s L%d: SDL_SRCCOLORKEY setting failed!\n", __FILE__, __LINE__);
    return (uint32)-1; //better?
    }


BOOL aui_SDLSurface::IsOK( ) const
{
	return TRUE;
}




AUI_ERRCODE aui_SDLSurface::Lock( RECT *rect, LPVOID *buffer, DWORD flags ){

    AUI_ERRCODE errcode = AUI_ERRCODE_OK;

    // must lock the mutex first!
    SDL_LockMutex(m_bltMutex);
    //printf("%s L%d: Locking mutex!\n", __FILE__, __LINE__);
    if (SDL_MUSTLOCK(m_lpdds)) {
        printf("%s L%d: Locking surface! Check this!\n", __FILE__, __LINE__);
        if (SDL_LockSurface(m_lpdds) < 0) {
            fprintf(stderr, "Cannot lock surface: %s\n", SDL_GetError());
            return AUI_ERRCODE_SURFACELOCKFAILED;
            }
        }
    // return a buffer pointer that points to the rectangle
    *buffer = m_lpdds->pixels;
    m_saveBuffer = static_cast<uint8*>(*buffer);
    if (rect != nullptr)
	{
        *buffer = static_cast<char*>(*buffer) +
            rect->top * m_lpdds->pitch +
            rect->left * m_Bpp;
	}
    return ManipulateLockList( rect, buffer, AUI_SURFACE_LOCKOP_ADD );
    }




AUI_ERRCODE aui_SDLSurface::Unlock( LPVOID buffer )
{
	AUI_ERRCODE errcode =
	ManipulateLockList( nullptr, &buffer, AUI_SURFACE_LOCKOP_REMOVE );

	if ( errcode == AUI_ERRCODE_OK )
	{
		if (SDL_MUSTLOCK(m_lpdds)) {
			SDL_UnlockSurface(m_lpdds);
		}
	}
	m_saveBuffer = nullptr;
	SDL_UnlockMutex(m_bltMutex);

	return errcode;
}

AUI_ERRCODE aui_SDLSurface::Blank(const uint32 &color)
{
	int errcode = SDL_FillRect(m_lpdds, nullptr, color);
	if (errcode == 0)
		return AUI_ERRCODE_OK;

	return AUI_ERRCODE_BLTFAILED;
}

void aui_SDLSurface::Flip( )
{
	// Present the composited primary surface through the GPU: upload its
	// pixels to the streaming texture and let the renderer scale/present it
	// (Metal on macOS, GL/Vulkan on Linux). Scaling for free = smooth zoom;
	// a sub-rect source = smooth scroll (wired separately). Falls back to the
	// window-surface present if no renderer (shouldn't happen post-init).
	if ( m_isPrimary && m_lpdds )
	{
		SDL_LockMutex(m_bltMutex);
		if ( m_renderer && m_screenTexture )
		{
			SDL_UpdateTexture( m_screenTexture, nullptr, m_lpdds->pixels, m_lpdds->pitch );
			SDL_RenderClear( m_renderer );
			SDL_RenderCopy( m_renderer, m_screenTexture, nullptr, nullptr );
			SDL_RenderPresent( m_renderer );
		}
		else if ( m_window )
		{
			SDL_UpdateWindowSurface( m_window );
		}
		SDL_UnlockMutex(m_bltMutex);
	}
}

#endif
