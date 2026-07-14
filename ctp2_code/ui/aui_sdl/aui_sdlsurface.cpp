#include "os/include/ctp2_config.h"
#include "ctp/c3.h"

#ifdef __AUI_USE_SDL__

#include "ui/aui_common/aui_ui.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_sdl/aui_sdlcompat.h"
#include "ui/aui_sdl/aui_sdlsurface.h"
#include "ui/aui_sdl/aui_sdl.h"   // P11 D: GpuLayersEnabled + world/UI textures
#include <algorithm>   // std::min/max (dirty-rect clamp in Flip)

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
		bpp = CTP2_SDL_SurfaceBitsPerPixel(lpdds);
	}
	// If wrapping an existing surface (like the window surface), use its actual bpp
	// so m_Bpp and offset calculations are correct
	if (lpdds != nullptr && !takeOwnership) {
		bpp = CTP2_SDL_SurfaceBitsPerPixel(lpdds);
	}
	*retval = aui_Surface::InitCommon( width, height, bpp, isPrimary );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommon();
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	if ( !(m_lpdds = lpdds) )
	{
		// Create the surface with the requested bpp. Legacy asset/window surfaces
		// may still be 16-bit RGB565, but the SDL primary and secondary screen
		// surfaces are 32-bit ARGB8888 so the present path uploads pixels without
		// a per-frame 565->8888 conversion.
		if (bpp == 16) {
			m_lpdds = CTP2_SDL_CreateRGBSurface(width, height, 16, 0xF800, 0x07E0, 0x001F, 0);
		} else {
			m_lpdds = CTP2_SDL_CreateRGBSurface(width, height, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
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
	if (CTP2_SDL_SurfaceBitsPerPixel(m_lpdds) == 32) {
		m_pixelFormat = AUI_SURFACE_PIXELFORMAT_888;
	}
	if ((CTP2_SDL_SurfaceRMask(m_lpdds) >> CTP2_SDL_SurfaceRShift(m_lpdds) == 0x1F) && (CTP2_SDL_SurfaceGMask(m_lpdds) >> CTP2_SDL_SurfaceGShift(m_lpdds) == 0x3F) && (CTP2_SDL_SurfaceBMask(m_lpdds) >> CTP2_SDL_SurfaceBShift(m_lpdds) == 0x1F)) {
            m_pixelFormat = AUI_SURFACE_PIXELFORMAT_565;
            //printf("%s L%d: AUI_SURFACE_PIXELFORMAT_565\n", __FILE__, __LINE__);
            }
		if ((CTP2_SDL_SurfaceRMask(m_lpdds) >> CTP2_SDL_SurfaceRShift(m_lpdds) == 0x1F) && (CTP2_SDL_SurfaceGMask(m_lpdds) >> CTP2_SDL_SurfaceGShift(m_lpdds) == 0x1F) && (CTP2_SDL_SurfaceBMask(m_lpdds) >> CTP2_SDL_SurfaceBShift(m_lpdds) == 0x1F)) {
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
		CTP2_SDL_DestroySurface(m_lpdds);
		m_lpdds = nullptr;
		m_allocated = FALSE;
	}
	SDL_DestroyMutex(m_bltMutex);
}


uint32 aui_SDLSurface::SetChromaKey( uint32 color ) {
    //printf("%s L%d: SDL_SRCCOLORKEY set to %#X\n", __FILE__, __LINE__, color);

    if ( CTP2_SDL_SetColorKey(m_lpdds, true, color) )
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

void aui_SDLSurface::Flip(RECT const *dirty)
{
	// Present the composited primary surface through the GPU: upload its
	// pixels to the streaming texture and let the renderer scale/present it
	// (Metal on macOS, GL/Vulkan on Linux). When a dirty rect is given only
	// that region is uploaded — the texture persists between frames, so a
	// partial upload + full RenderCopy still shows the complete frame.
	// Falls back to the window-surface present if no renderer (shouldn't
	// happen post-init).
	if ( m_isPrimary && m_lpdds )
	{
		SDL_LockMutex(m_bltMutex);
		if ( m_renderer && m_screenTexture )
		{
			SDL_Rect up;
			SDL_Rect const *upPtr = nullptr;
			if (dirty)
			{
				// Clamp to surface bounds; empty -> nothing to upload,
				// but still present (callers only Flip when something
				// changed, so keep the present unconditional).
				sint32 const l = std::max<sint32>(dirty->left, 0);
				sint32 const t = std::max<sint32>(dirty->top, 0);
				sint32 const r = std::min<sint32>(dirty->right, m_lpdds->w);
				sint32 const b = std::min<sint32>(dirty->bottom, m_lpdds->h);
				if (l < r && t < b)
				{
					up.x = l;  up.y = t;  up.w = r - l;  up.h = b - t;
					upPtr = &up;
				}
			}
			// P11 Stage 2 D: when per-layer GPU compositing is enabled, the
			// composited frame becomes the world (base) layer and a second
			// alpha texture is composited over it on the GPU. Both textures are
			// full-screen ARGB8888; the UI texture is in BLENDMODE_BLEND so its
			// transparent pixels leave the world layer untouched. Until the UI
			// composite is redirected into it (next increment) the UI texture
			// is fully transparent, so the presented frame is byte-identical to
			// the single-texture present — keeping the slice-ui oracle green.
			bool const layered = aui_SDL::GpuLayersEnabled()
			                   && aui_SDL::WorldTexture() && aui_SDL::UiTexture();
			SDL_Texture * const baseTex = layered ? aui_SDL::WorldTexture()
			                                      : m_screenTexture;
			// The world/base texture accumulates the frame from per-dirty-rect
			// uploads exactly as m_screenTexture does (the full m_lpdds surface
			// is never a complete frame — only its freshly composited regions
			// are valid, the rest holds stale init content). Since layers are
			// enabled from frame 0, the world texture builds up identically.
			if (!dirty || upPtr)
			{
				uint8 const *pixels = static_cast<uint8 const *>(m_lpdds->pixels);
				if (upPtr)
				{
					pixels += static_cast<size_t>(up.y) * m_lpdds->pitch
					        + static_cast<size_t>(up.x) * CTP2_SDL_SurfaceBytesPerPixel(m_lpdds);
				}
				CTP2_SDL_UpdateTexture( baseTex, upPtr, pixels, m_lpdds->pitch );
			}
			SDL_RenderClear( m_renderer );
			CTP2_SDL_RenderTexture( m_renderer, baseTex );
			if (layered)
				CTP2_SDL_RenderTexture( m_renderer, aui_SDL::UiTexture() );
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
