//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Mouse User Interface
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
// - None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Increased k_MOUSE_MAXNUMCURSORS to allow some additional cursors.
//   - April 30th 2005 Martin G�hmann
//
//----------------------------------------------------------------------------

#ifndef __AUI_MOUSE_H__
#define __AUI_MOUSE_H__

#include "ui/aui_common/aui_base.h"
#include "ui/aui_common/aui_input.h"
#include "ui/aui_common/tech_wllist.h"

#ifdef USE_SDL
#include "ui/aui_sdl/aui_sdlcompat.h"
#endif

class aui_Cursor;
class aui_Surface;
class aui_Window;
class aui_Image;
class aui_DirtyList;
class ldl_datablock;

#define k_MOUSE_LDL_NUMCURSORS	"numcursors"
#define k_MOUSE_LDL_ANIM		"anim"
#define k_MOUSE_LDL_CURSOR		"cursor"
#define k_MOUSE_LDL_HOTSPOTX	"hotspotx"
#define k_MOUSE_LDL_HOTSPOTY	"hotspoty"
#define k_MOUSE_LDL_ANIMDELAY	"animdelay"
#define k_MOUSE_LDL_FIRSTINDEX	"firstindex"
#define k_MOUSE_LDL_LASTINDEX	"lastindex"

#define k_MOUSE_EVENT_FLAG_LSHIFT		0x00000001
#define k_MOUSE_EVENT_FLAG_RSHIFT		0x00000002
#define k_MOUSE_EVENT_FLAG_LCONTROL		0x00000004
#define k_MOUSE_EVENT_FLAG_RCONTROL		0x00000008

struct aui_MouseEvent
{
	POINT	position;
	BOOL	lbutton;
	BOOL	rbutton;
	BOOL    mbutton;
	BOOL    tbutton;
	BOOL    ubutton;
	BOOL    vbutton;
	BOOL    wbutton;
	BOOL    xbutton;
	uint32	time;
	sint32	movecount;
	sint32	framecount;
	uint32	flags;
};

#define k_MOUSE_MAXINPUT			48

#define k_MOUSE_MAXSIZE				64

#define k_MOUSE_MAXNUMCURSORS		96

#define k_MOUSE_DEFAULTANIMDELAY	100

#ifdef USE_SDL
// HACK: Halt mouse event handling thread on game exit.  Backing storage
// is `static BOOL g_mouseShouldTerminateThread` in aui_mouse.cpp.
// civ3_main.cpp's shutdown path is the only cross-TU writer.
void aui_mouse_RequestTerminate();
#endif

class aui_Mouse : public aui_Base, public virtual aui_Input
{
public:

	aui_Mouse(
		AUI_ERRCODE *retval,
		MBCHAR *ldlBlock );
	~aui_Mouse() override;

protected:
	aui_Mouse() {}
	AUI_ERRCODE InitCommonLdl( MBCHAR *ldlBlock );
	AUI_ERRCODE InitCommon( );

	sint32 FindNumCursorsFromLdl( ldl_datablock *block );

public:

	AUI_ERRCODE Start( );
	AUI_ERRCODE End( );
	AUI_ERRCODE Suspend( BOOL eraseCursor );
	AUI_ERRCODE Resume( );

	AUI_ERRCODE Show( )
	{
		m_showCount++;
		return AUI_ERRCODE_OK;
	}
	AUI_ERRCODE Hide( )
	{
		if ( !m_showCount )
		{

			Suspend( TRUE );
			m_showCount--;
			Resume();
		}
		return AUI_ERRCODE_OK;
	}

	BOOL IsSuspended( ) const { return m_suspendCount; }
	BOOL IsHidden( ) const { return m_showCount < 0; }

	sint32	X( ) { return m_data.position.x; }
	sint32	Y( ) { return m_data.position.y; }

	void SetClip( sint32 left, sint32 top, sint32 right, sint32 bottom );
	void SetClip( RECT *clip );

	aui_MouseEvent *GetLatestMouseEvent( ) { return &m_data; }

	AUI_ERRCODE SetPosition( sint32 x, sint32 y );
	AUI_ERRCODE SetPosition( POINT *point );

	AUI_ERRCODE	GetHotspot( sint32 *x, sint32 *y, sint32 index = 0 );
	AUI_ERRCODE	SetHotspot( sint32 x, sint32 y, sint32 index = 0 );

	double		&Sensitivity( ) { return m_sensitivity; }

	aui_Cursor *GetCursor( sint32 index ) const { return m_cursors[ index ]; }
	void SetCursor( sint32 index, MBCHAR *cursor );

	aui_Cursor *GetCurrentCursor( ) const { return *m_curCursor; }
	sint32		GetCurrentCursorIndex() ;
	void		SetCurrentCursor( sint32 index );

	uint32 GetAnimDelay( ) const { return m_animDelay; }
	void SetAnimDelay( uint32 animDelay );

	void GetAnimIndexes( sint32 *firstIndex, sint32 *lastIndex )
	{
		if ( firstIndex ) *firstIndex = m_firstIndex;
		if ( lastIndex ) *lastIndex = m_lastIndex;
	}
	void SetAnimIndexes( sint32 firstIndex, sint32 lastIndex );

	void SetAnim( sint32 anim );

	// P11: keep the OS cursor in step with the game cursor when the layered
	// GPU present uses a hardware cursor (no-op otherwise). Called from the
	// per-frame paths (ReactToInput, HandleAnim); converts + installs only
	// when the effective cursor actually changed.
	void SyncHardwareCursor( );

	AUI_ERRCODE ReactToInput( ) override;

	BOOL	ShouldTerminateThread( );

	sint32	ManipulateInputs( aui_MouseEvent *data, BOOL add );

	AUI_ERRCODE HandleAnim( );

	AUI_ERRCODE	BltWindowToPrimary( aui_Window *window );
	AUI_ERRCODE BltDirtyRectInfoToPrimary( );
	AUI_ERRCODE	BltBackgroundColorToPrimary(
		COLORREF color,
		aui_DirtyList *colorAreas );
	AUI_ERRCODE	BltBackgroundImageToPrimary(
		aui_Image *image,
		RECT *imageRect,
		aui_DirtyList *imageAreas );

	CTP2_SDL_Mutex *LPCS() const { return m_lpcs; }

	AUI_ERRCODE CreatePrivateBuffers( );
	void DestroyPrivateBuffers( );

	uint32 GetFlags() { return m_flags;}
	void SetFlags(uint32 flags) { m_flags = flags; }

protected:
	static sint32 m_mouseRefCount;
	static CTP2_SDL_Mutex* m_lpcs;

	virtual AUI_ERRCODE Erase( );

	aui_MouseEvent	m_data;
	double			m_sensitivity;

	aui_MouseEvent	m_inputs[ k_MOUSE_MAXINPUT ];
	aui_Surface		*m_privateMix;
	aui_Surface		*m_pickup;
	aui_Surface		*m_prevPickup;

	RECT		m_clip;
	aui_Cursor	*m_cursors[ k_MOUSE_MAXNUMCURSORS ];
	aui_Cursor	**m_curCursor;
	sint32		m_firstIndex;
	sint32		m_lastIndex;
	uint32		m_animDelay;
	uint32		m_time;

	tech_WLList<POINT>	m_animIndexList;
	tech_WLList<sint32> m_animDelayList;

	sint32		m_suspendCount;
	sint32		m_showCount;
	BOOL		m_reset;
	// P11: cursor last installed as the OS cursor (hardware-cursor mode).
	aui_Cursor	*m_hwCursorShown;

	SDL_Thread     *m_thread;
	uint32          m_threadId;

	uint32		m_flags;
};


int MouseThreadProc(void *param);

#endif
