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
			// P11 Stage 2 D: per-layer GPU compositing. The aui_UI chokepoint
			// keeps a world-only and a UI-only composite surface; upload each to
			// its own full-screen ARGB8888 texture and GPU-composite them
			// (world base, then UI in BLENDMODE_BLEND on top). The UI surface is
			// transparent everywhere it has not drawn, so the world shows
			// through. Both surfaces are complete (persisted, dirty-accumulated
			// like m_secondary), so a full upload each present is correct.
			aui_UI * const ui = aui_ui_Get();
			aui_SDLSurface * const worldSurf =
				(ui && ui->GpuLayers()) ? static_cast<aui_SDLSurface *>(ui->WorldSurface()) : nullptr;
			aui_SDLSurface * const uiSurf =
				(ui && ui->GpuLayers()) ? static_cast<aui_SDLSurface *>(ui->UiSurface()) : nullptr;
			bool const layered = aui_SDL::WorldTexture() && aui_SDL::UiTexture()
			                   && worldSurf && uiSurf && worldSurf->DDS() && uiSurf->DDS();

			if (layered)
			{
				SDL_Surface * const ws = worldSurf->DDS();
				SDL_Surface * const us = uiSurf->DDS();

				// P11 2c (ADR-001) — redundant-present elimination. Multiple callers
				// Flip every frame (main loop camera tick, the mouse thread, UI
				// process); each present blocks on vsync, so redundant ones halve or
				// third the achievable frame rate — during a trackpad glide that
				// starved the camera tick down to ~20Hz and broke the ease into
				// visible steps. The composite output is fully determined by the
				// layer content versions + the camera transform + the content
				// offset: if none of them changed since the last present, the frame
				// on screen is already identical — skip the whole present. The
				// texture uploads use the same versions (upload only what changed):
				// during a pure glide neither surface changed, so a present is just
				// two GPU draws + present. Statics are safe under m_bltMutex (all
				// Flips serialize on it) and shared across surfaces by design (the
				// layered composite ignores which surface flipped).
				bool  const camOn   = aui_SDL::GpuCameraEnabled();
				float const camOffX = camOn ? aui_SDL::CameraOffX() : 0.0f;
				float const camOffY = camOn ? aui_SDL::CameraOffY() : 0.0f;
				float const camZoom = camOn ? aui_SDL::CameraZoom() : 1.0f;
				uint32 const worldV = ui->WorldContentVersion();
				uint32 const uiV    = ui->UiContentVersion();
				bool  const fogOn   = ui->GpuFog();

				static uint32 s_shownWorldV  = ~0u;
				static uint32 s_shownUiV     = ~0u;
				static float  s_shownOffX    = 0.0f;
				static float  s_shownOffY    = 0.0f;
				static float  s_shownZoom    = 1.0f;
				static int    s_shownBaseX   = -1;
				static int    s_shownBaseY   = -1;

				bool const worldChanged = (worldV != s_shownWorldV);
				bool const uiChanged    = (uiV    != s_shownUiV);
				bool const camChanged   = (camOffX != s_shownOffX)
				                       || (camOffY != s_shownOffY)
				                       || (camZoom != s_shownZoom)
				                       || (aui_SDL::WorldContentOffX() != s_shownBaseX)
				                       || (aui_SDL::WorldContentOffY() != s_shownBaseY);

				// Fog has no version counter (conservative: never skip while GPU fog
				// is on). Quads re-render the world texture per present, so they
				// always count as a world change.
				if (!worldChanged && !uiChanged && !camChanged && !fogOn
				    && !aui_SDL::GpuQuadsEnabled())
				{
					SDL_UnlockMutex(m_bltMutex);
					return;
				}

				// P11 Stage 3 G1: terrain quad renderer. When enabled, draw the
				// visible terrain cells as GPU textured quads (from the tile
				// atlas) INTO the world texture — which is a render target in
				// quad mode — instead of uploading the CPU world surface. The
				// draw list covers every visible cell every present, so the
				// world texture is always complete (the camera transform below
				// has no holes to reveal — the failure mode that sank the old
				// dirty-rect layer split). Cleared to opaque black first, so any
				// unexplored / unemitted cell is black (as the CPU BlackTile).
				bool const quads = aui_SDL::GpuQuadsEnabled() && aui_SDL::QuadAtlasTexture()
				                && aui_SDL::QuadFrameComplete();
				if (quads)
				{
					SDL_SetRenderTarget( m_renderer, aui_SDL::WorldTexture() );
					SDL_SetRenderDrawColor( m_renderer, 0, 0, 0, 255 );
					SDL_RenderClear( m_renderer );
					SDL_Texture * const atlas = aui_SDL::QuadAtlasTexture();
					for (aui_SDL::GpuQuad const & q : aui_SDL::QuadDrawList())
					{
						CTP2_SDL_RenderTextureSrcDst( m_renderer, atlas,
							q.sx, q.sy, q.sw, q.sh,
							(float)q.dx, (float)q.dy, (float)q.dw, (float)q.dh );
					}
					for (aui_SDL::GpuSpriteQuad const & q : aui_SDL::SpriteDrawList())
					{
						if (q.screen_space)
							continue;
						SDL_SetTextureBlendMode(q.texture, q.additive ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
						SDL_SetTextureColorMod(q.texture, q.red, q.green, q.blue);
						SDL_SetTextureAlphaMod(q.texture, q.alpha);
						CTP2_SDL_RenderTextureSrcDstFlip( m_renderer, q.texture,
							q.sx, q.sy, q.sw, q.sh,
							(float)q.dx, (float)q.dy, (float)q.dw, (float)q.dh,
							q.mirror );
					}
					SDL_SetRenderTarget( m_renderer, nullptr );
				}
				else if (worldChanged)
				{
					// Upload only when the world surface actually changed since the
					// last upload — a camera-only glide present skips the ~6MB copy.
					CTP2_SDL_UpdateTexture( aui_SDL::WorldTexture(), nullptr, ws->pixels, ws->pitch );
				}
				if (uiChanged)
					CTP2_SDL_UpdateTexture( aui_SDL::UiTexture(), nullptr, us->pixels, us->pitch );

				// P11 Stage 2 C: GPU fog. When enabled, upload the fog mask and
				// composite it (BLENDMODE_BLEND, black + per-tile alpha) between
				// the world and UI copies so it darkens ONLY the world (fogged
				// terrain), leaving UI/radar/city-text overlays unfogged.
				aui_SDLSurface * const fogSurf =
					(ui->GpuFog()) ? static_cast<aui_SDLSurface *>(ui->FogSurface()) : nullptr;
				bool const fogged = aui_SDL::FogTexture() && fogSurf && fogSurf->DDS();
				if (fogged)
				{
					SDL_Surface * const fs = fogSurf->DDS();
					CTP2_SDL_UpdateTexture( aui_SDL::FogTexture(), nullptr, fs->pixels, fs->pitch );
				}

				SDL_RenderClear( m_renderer );

				// P11 2a (ADR-001): present a screen-sized viewport windowed into
				// the (oversized) world texture, slid by CameraOff (pan) and scaled
				// by CameraZoom. Windowing by a source rect — rather than stretching
				// the whole texture — is what lets the viewport pan sub-tile into the
				// margin without revealing an edge. At identity (off 0,0 / zoom 1) it
				// samples the centred screen region, reproducing the pre-margin
				// output exactly. World + fog share the window (they move together);
				// the UI layer stays full-screen. The margin is (texW - screenW)/2,
				// so a screen-sized texture (fog) windows to its full extent.
				{
					float const W = static_cast<float>(m_lpdds->w);
					float const H = static_cast<float>(m_lpdds->h);
					bool  const cam  = aui_SDL::GpuCameraEnabled();
					float const z    = cam ? aui_SDL::CameraZoom() : 1.0f;
					float const offX = cam ? aui_SDL::CameraOffX() : 0.0f;
					float const offY = cam ? aui_SDL::CameraOffY() : 0.0f;

					// baseX/baseY = pixel offset of the screen top-left inside the
					// texture. The mirrored world texture is oversized, its screen
					// content at the fixed window-margin content offset; the quad
					// path now uses the same oversized world-space origin as streaming.
					// Zoom centres about the screen middle.
					auto presentWindowed = [&]( SDL_Texture * tex, float baseX, float baseY )
					{
						float const srcW = W / z;
						float const srcH = H / z;
						float const srcX = baseX + (W - srcW) * 0.5f - offX;
						float const srcY = baseY + (H - srcH) * 0.5f - offY;
						CTP2_SDL_RenderTextureWindow( m_renderer, tex,
							srcX, srcY, srcW, srcH, 0.0f, 0.0f, W, H );
					};

					// P13 step 2.1 (ADR-003): with the whole map in one texture the
					// camera is a plain source rect over it -- no margin to run
					// past, so none of the recenter machinery applies. Falls back
					// to the ADR-002 window mirror if the target is not ready.
					if (aui_SDL::GpuWorldmapEnabled() && aui_SDL::WorldmapTexture())
					{
						// P13 step 2.3: filter mode follows zoom DIRECTION.
						// Magnifying (z > 1) must stay NEAREST -- 200% is meant
						// to read as crisp pixel art, and linear would just blur
						// it. Minifying needs linear, since nearest drops pixels
						// and the dropped set changes as the source rect slides,
						// which reads as shimmer during a pan.
						if (z > 1.0f) CTP2_SDL_SetTextureNearest(aui_SDL::WorldmapTexture());
						else          CTP2_SDL_SetTextureLinear(aui_SDL::WorldmapTexture());
						presentWindowed( aui_SDL::WorldmapTexture(),
							(float)aui_SDL::WorldmapOriginX(),
							(float)aui_SDL::WorldmapOriginY() );
						// P13 step 3: units, cities and effects go on top of the
						// windowed terrain. They are NOT composited into the
						// whole-map texture -- it is persistent and
						// dirty-tracked, so anything that moves would leave a
						// trail baked into it.
						aui_SDL::RenderWorldmapSpriteQuads(m_renderer, W, H, z, offX, offY);
					}
					else
						presentWindowed( aui_SDL::WorldTexture(),
							(float)aui_SDL::WorldContentOffX(),
							(float)aui_SDL::WorldContentOffY() );
					if (fogged)
						presentWindowed( aui_SDL::FogTexture(), 0.0f, 0.0f );
				}
				for (aui_SDL::GpuSpriteQuad const & q : aui_SDL::SpriteDrawList())
				{
					if (!q.screen_space)
						continue;
					SDL_SetTextureBlendMode(q.texture, q.additive ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
					SDL_SetTextureColorMod(q.texture, q.red, q.green, q.blue);
					SDL_SetTextureAlphaMod(q.texture, q.alpha);
					CTP2_SDL_RenderTextureSrcDstFlip( m_renderer, q.texture,
						q.sx, q.sy, q.sw, q.sh,
						(float)q.dx, (float)q.dy, (float)q.dw, (float)q.dh,
						q.mirror );
				}
				CTP2_SDL_RenderTexture( m_renderer, aui_SDL::UiTexture() );
				SDL_RenderPresent( m_renderer );

				// Record what this present showed, for the redundancy check above.
				s_shownWorldV = worldV;
				s_shownUiV    = uiV;
				s_shownOffX   = camOffX;
				s_shownOffY   = camOffY;
				s_shownZoom   = camZoom;
				s_shownBaseX  = aui_SDL::WorldContentOffX();
				s_shownBaseY  = aui_SDL::WorldContentOffY();
			}
			else
			{
				if (!dirty || upPtr)
				{
					uint8 const *pixels = static_cast<uint8 const *>(m_lpdds->pixels);
					if (upPtr)
					{
						pixels += static_cast<size_t>(up.y) * m_lpdds->pitch
						        + static_cast<size_t>(up.x) * CTP2_SDL_SurfaceBytesPerPixel(m_lpdds);
					}
					CTP2_SDL_UpdateTexture( m_screenTexture, upPtr, pixels, m_lpdds->pitch );
				}
				SDL_RenderClear( m_renderer );
				CTP2_SDL_RenderTexture( m_renderer, m_screenTexture );
				SDL_RenderPresent( m_renderer );
			}
		}
		else if ( m_window )
		{
			SDL_UpdateWindowSurface( m_window );
		}
		SDL_UnlockMutex(m_bltMutex);
	}
}

#endif
