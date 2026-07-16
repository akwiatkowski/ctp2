//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : User interface
// Id           : $Id$
//
//----------------------------------------------------------------------------
//
// Disclaimer
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
// This material has been developed at apolyton.net by the Apolyton CtP2
// Source Code Project. Contact the authors at ctp2source@apolyton.net.
//
//----------------------------------------------------------------------------
//
// Compiler flags
//
// -None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Import structure changed to compile with Mingw
// - Moved CalculateHash() to aui_Base
// - Prevented processing of uninitialised input
// - Added graphics DirectX built in double buffering and extended it
//   to manual tripple buffering. (1-Jan-2010 Martin G�hmann)
//
//----------------------------------------------------------------------------

#if defined(HAVE_PRAGMA_ONCE)
#pragma once
#endif

#ifndef AUI_UI_H_
#define AUI_UI_H_

//----------------------------------------------------------------------------
// Library imports
//----------------------------------------------------------------------------

#include <windows.h>		// HINSTANCE etc.
#include <algorithm>		// std::min/max (secondary dirty-union)

//----------------------------------------------------------------------------
// Exported names
//----------------------------------------------------------------------------

class			aui_UI;

#define			k_AUI_UI_NOCOLOR	0xff000000

//----------------------------------------------------------------------------
// Project imports
//----------------------------------------------------------------------------

#include "ui/aui_common/aui_action.h"			// aui_Action
#include "ui/aui_common/aui_audiomanager.h"	// aui_AudioManager
#include "ui/aui_common/aui_bitmapfont.h"		// aui_BitmapFont
#include "ui/aui_common/aui_blitter.h"		// aui_Blitter
#include "ui/aui_common/aui_control.h"		// aui_Control
#include "ui/aui_common/aui_cursor.h"			// aui_Cursor
#include "ui/aui_common/aui_dirtylist.h"		// aui_DirtyList
#include "ui/aui_common/aui_image.h"			// aui_Image
#include "ui/aui_common/aui_joystick.h"		// aui_Joystick
#include "ui/aui_common/aui_keyboard.h"		// aui_Keyboard
#include "ui/aui_common/aui_ldl.h"			// aui_Ldl
#include "ui/aui_common/aui_memmap.h"			// aui_MemMap
#include "ui/aui_common/aui_mouse.h"			// aui_Mouse, aui_MouseEvent
#include "ui/aui_common/aui_moviemanager.h"	// aui_MovieManager
#include "ui/aui_common/aui_region.h"			// aui_Region
#include "ui/aui_common/aui_resource.h"		// aui_Resource
#include "ui/aui_common/aui_sound.h"			// aui_Sound
#include "ui/aui_common/aui_static.h"			// aui_Static
#include "ui/aui_common/aui_surface.h"		// aui_Surface
#include "ui/aui_common/aui_window.h"			// aui_Window
#include "ui/aui_common/auitypes.h"			// AUI_...
#include "ctp/c3types.h"			// MBCHAR, sint32, uint32
#include "ui/aui_common/tech_wllist.h"		// tech_WLList

class aui_Movie;

//----------------------------------------------------------------------------
// Class declarations
//----------------------------------------------------------------------------

class aui_UI : public aui_Region
{
public:

	aui_UI(
		AUI_ERRCODE *retval,
		HINSTANCE hinst,
		HWND hwnd,
		sint32 width,
		sint32 height,
		sint32 bpp,
		const MBCHAR *ldlFilename  = nullptr);
	~aui_UI() override;

protected:
	aui_UI()
	:
		aui_Region                  (),
		m_dirtyRectInfoMemory       (nullptr),
		m_dirtyRectInfoList         (nullptr),
		m_hinst                     ((HINSTANCE) INVALID_HANDLE_VALUE),
		m_hwnd                      ((HWND) INVALID_HANDLE_VALUE),
		m_bpp                       (0),
		m_pixelFormat               (AUI_SURFACE_PIXELFORMAT_UNKNOWN),
		m_ldl                       (nullptr),
		m_primary                   (nullptr),
		m_secondary                 (nullptr),
		m_worldSurface              (nullptr),
		m_uiSurface                 (nullptr),
		m_worldWindow               (nullptr),
		m_gpuLayers                 (false),
		m_fogSurface                (nullptr),
		m_gpuFog                    (false),
		m_worldContentVersion       (0),
		m_uiContentVersion          (0),
		m_blitter                   (nullptr),
		m_memmap                    (nullptr),
		m_mouse                     (nullptr),
		m_keyboard                  (nullptr),
		m_joystick                  (nullptr),
		m_dirtyList                 (nullptr),
		m_color                     (k_AUI_UI_NOCOLOR),
		m_image                     (nullptr),
		m_colorAreas                (nullptr),
		m_imageAreas                (nullptr),
		m_virtualFocus              (nullptr),
		m_dxver                     (0),
		m_editMode                  (false),
		m_editRegion                (nullptr),
		m_editWindow                (nullptr),
		m_localRectText             (nullptr),
		m_absoluteRectText          (nullptr),
		m_editModeLdlName           (nullptr),
		m_imageResource             (nullptr),
		m_cursorResource            (nullptr),
		m_bitmapFontResource        (nullptr),
		m_audioManager              (nullptr),
		m_movieManager              (nullptr),
		m_actionList                (nullptr),
		m_destructiveActionList     (nullptr),
		m_winList                   (nullptr),
		m_minimize                  (false),
		m_savedMouseAnimFirstIndex  (0),
		m_savedMouseAnimLastIndex   (0),
		m_savedMouseAnimCurIndex    (0),
		m_savedMouseAnimDelay       (0)
	{};

	AUI_ERRCODE InitCommon(
		HINSTANCE hinst,
		HWND hwnd,
		sint32 bpp,
		const MBCHAR *ldlFilename );
	AUI_ERRCODE CreateScreen( );

public:
	void RegisterObject( aui_Blitter *blitter );
	void RegisterObject( aui_Mouse *mouse );
	void RegisterObject( aui_Keyboard *keyboard );
	void RegisterObject( aui_MemMap *memmap );
	void RegisterObject( aui_AudioManager *audioManager );
	void RegisterObject( aui_MovieManager *movieManager );
	void RegisterObject( aui_Joystick *joystick );

	COLORREF	SetBackgroundColor( COLORREF color );
	aui_Image	*SetBackgroundImage(
		aui_Image *image,
		sint32 x = 0,
		sint32 y = 0 );

	HINSTANCE	TheHINSTANCE( ) const { return m_hinst; }
	HWND		TheHWND( ) const { return m_hwnd; }
	aui_Ldl		*GetLdl( ) const { return m_ldl; }

	AUI_ERRCODE BltToSecondary
	                          (
	                           sint32       destx,
	                           sint32       desty,
	                           aui_Surface *srcSurf,
	                           RECT        *srcRect,
	                           uint32       flags
	                          )
	{
		Assert(m_secondary);
		AccumulateSecondaryDirty(destx, desty,
		                         destx + (srcRect->right - srcRect->left),
		                         desty + (srcRect->bottom - srcRect->top));
		AUI_ERRCODE const rc =
			m_blitter->Blt(m_secondary, destx, desty, srcSurf, srcRect, flags);
		// P11 Stage 2 D: mirror each composite write into a world-only or a
		// UI-only layer so they can be GPU-composited separately (fog on the
		// world, pan/zoom the world). The background window's surface is the
		// world; every other source (UI windows, cursor, fills) is UI. The UI
		// layer is screen-sized and shares the secondary's coordinates.
		if (m_gpuLayers)
		{
			// P11 2b (ADR-001): the world layer is no longer mirrored from the
			// screen background here — TiledMap::RenderWorldLayer renders terrain
			// and actors directly into the oversized m_worldSurface at the wider
			// (margin) view, so the margin holds real content. Only the UI layer
			// is still mirrored from the composite (everything that is NOT the
			// background/world window).
			if (srcSurf && srcSurf == WorldSurfaceKey())
			{
				// Punch a transparent hole in the UI layer (screen coords): in
				// z-order the world is the bottom-most window, so a world write
				// means whatever the UI layer held here (a closed window, a
				// fill) is stale — anything genuinely above will be re-blitted
				// right after by its own window in the same composite pass.
				// Without this, closed windows ghost forever over the world.
				EraseUiLayerRect(destx, desty,
				                 destx + (srcRect->right - srcRect->left),
				                 desty + (srcRect->bottom - srcRect->top));
				++m_uiContentVersion;
			}
			else
			{
				m_blitter->Blt(m_uiSurface, destx, desty, srcSurf, srcRect, flags);
				// Stamp the write so the present can tell "UI pixels changed"
				// from "identical frame" (see the content versions below).
				++m_uiContentVersion;
			}
		}
		return rc;
	};

	// useAccumulatedDirty: the per-frame mouse presents pass true — every
	// write they composite goes through BltToSecondary/ColorBltToSecondary
	// above, so the accumulated union covers exactly what changed, and the
	// software mirror copy + GPU texture upload are scoped to it. Direct
	// Secondary() writers (splash text, movie playback) keep the default
	// full-frame present. An empty union falls back to full-frame.
	AUI_ERRCODE BltSecondaryToPrimary
	                        (
	                         uint32       flags,
	                         bool         useAccumulatedDirty = false
	                        );

	AUI_ERRCODE ColorBltToSecondary
	                             (
	                              RECT     *destRect,
	                              COLORREF  color,
	                              uint32    flags
	                             )
	{
		AccumulateSecondaryDirty(destRect->left, destRect->top,
		                         destRect->right, destRect->bottom);
		AUI_ERRCODE const rc = m_blitter->ColorBlt(m_secondary, destRect, color, flags);
		// P11 Stage 2 D: color/image fills are background/UI chrome, never the
		// world window — mirror them into the UI layer (see BltToSecondary).
		if (m_gpuLayers)
		{
			m_blitter->ColorBlt(m_uiSurface, destRect, color, flags);
			++m_uiContentVersion;
		}
		return rc;
	};

	sint32 PrimaryHeight()  { return m_primary->Height(); };
	sint32 PrimaryWidth()   { return m_primary->Width(); };
	sint32 SecondaryHeight(){ return m_secondary->Height(); };
	sint32 SecondaryWidth() { return m_secondary->Width(); };
	bool HasPrimary()       { return m_primary   != nullptr; };
	bool HasSecondary()     { return m_secondary != nullptr; };

	AUI_ERRCODE BlackScreen()
	{
		if(m_primary != nullptr)
		{
			RECT rect = {0, 0, PrimaryWidth(), PrimaryHeight()};
			return m_blitter->ColorBlt(m_primary, &rect, RGB(0,0,0), 0);
		}
		else
		{
			return AUI_ERRCODE_OK;
		}
	}

	AUI_ERRCODE ClearSecondary()
	{
		RECT rect = {0, 0, SecondaryWidth(), SecondaryHeight()};
		AccumulateSecondaryDirty(rect.left, rect.top, rect.right, rect.bottom);
		return m_blitter->ColorBlt(m_secondary, &rect, RGB(0,0,0), 0);
	}

	aui_Surface		*Secondary( ) const { return m_secondary; }
	aui_Surface		*Primary( ) const { return m_primary; }
	// P11 Stage 2 D: per-layer GPU compositing surfaces + configuration.
	aui_Surface		*WorldSurface( ) const { return m_worldSurface; }
	aui_Surface		*UiSurface( ) const { return m_uiSurface; }
	bool			GpuLayers( ) const { return m_gpuLayers; }
	void			SetWorldWindow( aui_Window *w ) { m_worldWindow = w; }
	// The world window's CURRENT surface (or null) — the source-surface key
	// BltToSecondary classifies world writes by. Resolved live because windows
	// create their surfaces lazily and drop/rebuild them on hide/resize.
	aui_Surface		*WorldSurfaceKey( ) const
	{ return m_worldWindow ? m_worldWindow->TheSurface() : nullptr; }
	// Zero (ARGB 0x00000000 = transparent) a rect of the UI layer, clamped to
	// the surface. See the world-blit hole punch in BltToSecondary.
	void			EraseUiLayerRect( sint32 l, sint32 t, sint32 r, sint32 b );
	aui_Surface		*FogSurface( ) const { return m_fogSurface; }
	bool			GpuFog( ) const { return m_gpuFog; }
	// P11 2c (ADR-001) — layer content versions. Monotonic counters bumped on
	// every write to the UI layer (the BltToSecondary/ColorBltToSecondary
	// chokepoints above) and to the world layer (TiledMap::RenderWorldLayer).
	// The GPU present compares them against what it last uploaded/showed: an
	// unchanged version means the texture upload can be skipped, and an entirely
	// unchanged frame (same versions + same camera) can skip the vsync-blocking
	// present altogether. That matters because the mouse thread also presents;
	// during a trackpad glide the cursor is still, and without the skip its
	// redundant presents each block on vsync and starve the 60fps camera tick.
	uint32			WorldContentVersion( ) const { return m_worldContentVersion; }
	uint32			UiContentVersion( ) const { return m_uiContentVersion; }
	void			BumpWorldContentVersion( ) { ++m_worldContentVersion; }
	aui_Blitter		*TheBlitter( ) const { return m_blitter; }
	aui_MemMap		*TheMemMap( ) const { return m_memmap; }
	aui_Mouse		*TheMouse( ) const { return m_mouse; }
	aui_Keyboard	*TheKeyboard( ) const { return m_keyboard; }
	aui_Joystick	*TheJoystick( ) const { return m_joystick; }

	sint32 BitsPerPixel( ) const { return m_bpp; }
	AUI_SURFACE_PIXELFORMAT PixelFormat( ) { return m_pixelFormat; }

	uint32			DXVer( ) const { return m_dxver; }

	aui_DirtyList	*GetDirtyList( ) { return m_dirtyList; }

	AUI_ERRCODE		FlushDirtyList( );

	aui_Resource<aui_Image> *GetImageResource( ) const
		{ return m_imageResource; }

	aui_Image	*LoadImage( const MBCHAR *name )
		{ return m_imageResource->Load( name, C3DIR_PICTURES ); }

	AUI_ERRCODE	UnloadImage( aui_Image *resource )
		{ return m_imageResource->Unload( resource ); }
	AUI_ERRCODE	UnloadImage( const MBCHAR *name )
		{ return m_imageResource->Unload( name ); }

	AUI_ERRCODE	AddImageSearchPath( const MBCHAR *path )
		{ return m_imageResource->AddSearchPath( path ); }
	AUI_ERRCODE	RemoveImageSearchPath( const MBCHAR *path )
		{ return m_imageResource->RemoveSearchPath( path ); }

	aui_Resource<aui_Cursor> *GetCursorResource( ) const
		{ return m_cursorResource; }

	aui_Cursor	*LoadCursor( const MBCHAR *name )
		{ return m_cursorResource->Load( name, C3DIR_CURSORS ); }

	AUI_ERRCODE	UnloadCursor( aui_Cursor *resource )
		{ return m_cursorResource->Unload( resource ); }
	AUI_ERRCODE	UnloadCursor( const MBCHAR *name )
		{ return m_cursorResource->Unload( name ); }

	AUI_ERRCODE	AddCursorSearchPath( const MBCHAR *path )
		{ return m_cursorResource->AddSearchPath( path ); }
	AUI_ERRCODE	RemoveCursorSearchPath( const MBCHAR *path )
		{ return m_cursorResource->RemoveSearchPath( path ); }

	aui_Resource<aui_BitmapFont> *GetBitmapFontResource( ) const
		{ return m_bitmapFontResource; }




	aui_BitmapFont	*LoadBitmapFont( const MBCHAR *name, uint32 size = 0 )
		{ return m_bitmapFontResource->Load( name, C3DIR_DIRECT, size ); }

	AUI_ERRCODE	UnloadBitmapFont( aui_BitmapFont *resource )
		{ return m_bitmapFontResource->Unload( resource ); }
	AUI_ERRCODE	UnloadBitmapFont( const MBCHAR *name )
		{ return m_bitmapFontResource->Unload( name ); }

	AUI_ERRCODE	AddBitmapFontSearchPath( const MBCHAR *path )
		{ return m_bitmapFontResource->AddSearchPath( path ); }
	AUI_ERRCODE	RemoveBitmapFontSearchPath( const MBCHAR *path )
		{ return m_bitmapFontResource->RemoveSearchPath( path ); }

	aui_AudioManager *TheAudioManager( ) const { return m_audioManager; }

	aui_Sound	*LoadSound( const MBCHAR *name )
		{ return m_audioManager ? m_audioManager->Load( name ) : nullptr; }

	AUI_ERRCODE	UnloadSound( aui_Sound *resource )
		{ return m_audioManager ? m_audioManager->Unload( resource ) : AUI_ERRCODE_HACK; }
	AUI_ERRCODE	UnloadSound( const MBCHAR *name )
		{ return m_audioManager ? m_audioManager->Unload( name ) : AUI_ERRCODE_HACK; }

	AUI_ERRCODE	AddSoundSearchPath( const MBCHAR *path )
		{ return m_audioManager ? m_audioManager->AddSearchPath( path ) : AUI_ERRCODE_HACK; }
	AUI_ERRCODE	RemoveSoundSearchPath( const MBCHAR *path )
		{ return m_audioManager ? m_audioManager->RemoveSearchPath( path ) : AUI_ERRCODE_HACK; }

	aui_MovieManager *TheMovieManager( ) const { return m_movieManager; }

	aui_Movie	*LoadMovie( const MBCHAR *name)
		{ return m_movieManager ? m_movieManager->Load( name, C3DIR_VIDEOS  ) : nullptr; }

	AUI_ERRCODE	UnloadMovie( aui_Movie *resource )
		{ return m_movieManager ? m_movieManager->Unload( resource ) : AUI_ERRCODE_HACK; }
	AUI_ERRCODE	UnloadMovie( const MBCHAR *name )
		{ return m_movieManager ? m_movieManager->Unload( name ) : AUI_ERRCODE_HACK; }

	AUI_ERRCODE	AddMovieSearchPath( const MBCHAR *path )
		{ return m_movieManager ? m_movieManager->AddSearchPath( path ) : AUI_ERRCODE_HACK; }
	AUI_ERRCODE	RemoveMovieSearchPath( const MBCHAR *path )
		{ return m_movieManager ? m_movieManager->RemoveSearchPath( path ) : AUI_ERRCODE_HACK; }

	aui_Window		*TopWindow( ) const
	{ return m_childList->L() ? (aui_Window *)m_childList->GetHead() : nullptr; }
	aui_Window		*BringWindowToTop( uint32 windowId );
	aui_Window		*BringWindowToTop( aui_Window *window );

	virtual AUI_ERRCODE DrawOne( aui_Window *window );

	AUI_ERRCODE Draw( );

	AUI_ERRCODE	AddWindow( aui_Window *window )
	{ return AddChild( (aui_Region *)window ); }
	AUI_ERRCODE	RemoveWindow( uint32 windowId )
	{ return RemoveChild( windowId ); }
	aui_Window	*GetWindow( uint32 windowId )
	{ return (aui_Window *)GetChild( windowId ); }
	AUI_ERRCODE	AddChild( aui_Region *child ) override;
	AUI_ERRCODE	RemoveChild( uint32 windowId ) override;

	AUI_ERRCODE	ShowWindow( uint32 windowId );
	AUI_ERRCODE	HideWindow( uint32 windowId );

	virtual AUI_ERRCODE	Idle( aui_Region *recurse = nullptr );

	AUI_ERRCODE	Invalidate( RECT *rect = nullptr );

	AUI_ERRCODE AddDirtyRect( RECT *rect );
	AUI_ERRCODE AddDirtyRect( sint32 left, sint32 top, sint32 right, sint32 bottom );

	AUI_ERRCODE HandleMouseEvents(
		sint32 numEvents = 0,
		aui_MouseEvent *events = nullptr );
	AUI_ERRCODE HandleKeyboardEvents( );
	AUI_ERRCODE HandleJoystickEvents( );
	virtual AUI_ERRCODE HandleWindowsMessage(
		HWND hwnd,
		UINT message,
		WPARAM wParam,
		LPARAM lParam );
	virtual AUI_ERRCODE Process( );

	void AddAction( aui_Action *action );
	void HandleActions( );







	void AddDestructiveAction(aui_Action *action);
	void HandleDestructiveActions( );

	virtual AUI_ERRCODE AltTabOut( );
	virtual AUI_ERRCODE AltTabIn( );
	BOOL	MinimizeOnAltTabOut( BOOL minimize );

	BOOL		IsChildWin( HWND hwnd ) const
	{ return (m_winList->Find(hwnd) ? TRUE : FALSE); }
	AUI_ERRCODE	AddWin( HWND hwnd );
	void		RemoveWin( HWND hwnd );

	aui_Region	*TheEditRegion( ) const { return m_editRegion; }
	void		SetEditRegion( aui_Region *region );
	void		SetEditMode( BOOL mode );
	BOOL		GetEditMode( ) { return m_editMode; }
	AUI_ERRCODE ShowSelectedRegion( aui_Region *region );
	RECT		TheEditRect( ) const { return m_editRect; }
	AUI_ERRCODE	CreateEditModeDialog( BOOL bMake );

	struct DirtyRectInfo
	{
		RECT rect;
		aui_Window *window;
	};

	tech_WLList<DirtyRectInfo *> *GetDirtyRectInfoList( )
	{ return m_dirtyRectInfoList; }

protected:
	AUI_ERRCODE	TagMouseEvents( sint32 numEvents, aui_MouseEvent *events );

	virtual AUI_ERRCODE ClipAndConsolidate( );

	AUI_ERRCODE InsertDirtyRectInfo( RECT *rect, aui_Window *window );
	void FlushDirtyRectInfoList( );

	tech_Memory<DirtyRectInfo>		*m_dirtyRectInfoMemory;
	tech_WLList<DirtyRectInfo *>	*m_dirtyRectInfoList;

	HINSTANCE		m_hinst;
	HWND			m_hwnd;
	sint32			m_bpp;
	AUI_SURFACE_PIXELFORMAT m_pixelFormat;

	aui_Ldl			*m_ldl;

	aui_Surface		*m_primary;
	aui_Surface		*m_secondary;

	// P11 Stage 2 D: per-layer GPU compositing. When m_gpuLayers is set (by the
	// SDL UI when CTP2_GPU_LAYERS is on), every BltToSecondary/ColorBltToSecondary
	// is mirrored into a world-only or UI-only screen-sized surface so the two
	// layers can be GPU-composited independently. m_worldWindow is the background
	// window; a write whose source is that window's CURRENT surface is world,
	// everything else is UI. The window pointer (not its surface) is stored
	// because windows create their surfaces lazily and drop/rebuild them on
	// hide/resize — a surface pointer captured once goes stale (or is null).
	aui_Surface		*m_worldSurface;
	aui_Surface		*m_uiSurface;
	// P11 2c: content versions for the layer surfaces (see accessors above).
	uint32			m_worldContentVersion;
	uint32			m_uiContentVersion;
	aui_Window		*m_worldWindow;
	bool			m_gpuLayers;
	// P11 Stage 2 C: fog-of-war mask surface (32-bit, screen-sized, transparent
	// except fogged tiles = 50% black). Composited over the world layer on the
	// GPU. Built by TiledMap from vision state. Set when m_gpuFog is on.
	aui_Surface		*m_fogSurface;
	bool			m_gpuFog;

	// Running union of every rect written into m_secondary via
	// BltToSecondary/ColorBltToSecondary since the last present.
	// Consumed + reset by BltSecondaryToPrimary(useAccumulatedDirty=true).
	RECT			m_secondaryDirtyUnion;
	BOOL			m_secondaryDirtyValid;

	void AccumulateSecondaryDirty(sint32 l, sint32 t, sint32 r, sint32 b)
	{
		if (r <= l || b <= t) return;
		if (m_secondaryDirtyValid)
		{
			m_secondaryDirtyUnion.left   = std::min<sint32>(m_secondaryDirtyUnion.left,   l);
			m_secondaryDirtyUnion.top    = std::min<sint32>(m_secondaryDirtyUnion.top,    t);
			m_secondaryDirtyUnion.right  = std::max<sint32>(m_secondaryDirtyUnion.right,  r);
			m_secondaryDirtyUnion.bottom = std::max<sint32>(m_secondaryDirtyUnion.bottom, b);
		}
		else
		{
			m_secondaryDirtyUnion = { l, t, r, b };
			m_secondaryDirtyValid = TRUE;
		}
	}

	aui_Blitter		*m_blitter;
	aui_MemMap		*m_memmap;
	aui_Mouse		*m_mouse;
	aui_Keyboard	*m_keyboard;
	aui_Joystick	*m_joystick;
	aui_DirtyList	*m_dirtyList;

	COLORREF		m_color;
	aui_Image		*m_image;
	RECT			m_imageRect;
	aui_DirtyList	*m_colorAreas;
	aui_DirtyList	*m_imageAreas;

	aui_Control		*m_virtualFocus;

	DWORD			m_dxver;

	BOOL			m_editMode;
	aui_Region		*m_editRegion;
	RECT			m_editRect;
	aui_Window		*m_editWindow;
	aui_Static		*m_localRectText;
	aui_Static		*m_absoluteRectText;
	aui_Static		*m_editModeLdlName;

	aui_Resource<aui_Image>			*m_imageResource;
	aui_Resource<aui_Cursor>		*m_cursorResource;
	aui_Resource<aui_BitmapFont>	*m_bitmapFontResource;
	aui_AudioManager				*m_audioManager;
	aui_MovieManager				*m_movieManager;

	tech_WLList<aui_Action *>	*m_actionList;

	tech_WLList<aui_Action *>	*m_destructiveActionList;

	tech_WLList<HWND>			*m_winList;

	BOOL m_minimize;




	sint32			m_savedMouseAnimFirstIndex;
	sint32			m_savedMouseAnimLastIndex;
	sint32			m_savedMouseAnimCurIndex;
	sint32			m_savedMouseAnimDelay;
};

// App-singleton accessor pair for the legacy g_ui pointer.  g_ui is
// file-static in aui_ui.cpp; external consumers go through these
// accessors.
aui_UI * aui_ui_Get();
void     aui_ui_Set(aui_UI *p);

#endif
