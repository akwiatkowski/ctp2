//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header file
// Description  :
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
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
//
//----------------------------------------------------------------------------
#ifndef __AUI_MOVIE_H__
#define __AUI_MOVIE_H__

class aui_Movie;
class aui_MovieFormat;
class aui_AviMovieFormat;

#include "ui/aui_common/aui_base.h"
#include "ui/aui_common/aui_memmap.h" // aui_FileFormat

class aui_Surface;

#define k_AUI_MOVIE_PLAYFLAG_LOOP			0x00000001
#define k_AUI_MOVIE_PLAYFLAG_ONSCREEN		0x00000002
#define k_AUI_MOVIE_PLAYFLAG_PLAYANDHOLD	0x00000004


class aui_MovieFormat : public aui_FileFormat
{
public:

	aui_MovieFormat() = default;
	~aui_MovieFormat() override = default;

	virtual AUI_ERRCODE	Load( MBCHAR const *filename, aui_Movie *movie )
		{ return AUI_ERRCODE_OK; }
};


class aui_Movie : public aui_Base
{
public:

	aui_Movie(
		AUI_ERRCODE *retval,
		MBCHAR const * filename = nullptr );
	~aui_Movie() override;

protected:
	AUI_ERRCODE InitCommon( MBCHAR const * filename );

public:

	AUI_ERRCODE Load( );
	AUI_ERRCODE Unload( );

	AUI_ERRCODE	SetFilename( MBCHAR const * filename );
	MBCHAR		*GetFilename( ) const { return (MBCHAR *)m_filename; }

	aui_Surface	*SetDestSurface( aui_Surface *surface );
	aui_Surface	*GetDestSurface( ) const { return m_surface; }

	void SetDestRect( RECT *rect );
	void SetDestRect(
		sint32 x,
		sint32 y,
		sint32 w,
		sint32 h );
	RECT *GetDestRect( ) { return &m_rect; }
	void GetDestRect(
		sint32 *x,
		sint32 *y,
		sint32 *w,
		sint32 *h ) const;

	uint32 SetTimePerFrame( uint32 timePerFrame );
	uint32 GetTimePerFrame( ) const { return m_timePerFrame; }

	virtual AUI_ERRCODE Open(
		uint32 flags = 0,
		aui_Surface *surface = nullptr,
		RECT *rect = nullptr );
	virtual AUI_ERRCODE Close( );

	virtual AUI_ERRCODE Play( );
	virtual AUI_ERRCODE Stop( );

	virtual AUI_ERRCODE Pause( );
	virtual AUI_ERRCODE Resume( );

	virtual AUI_ERRCODE Process( );

	BOOL IsOpen( ) const { return m_isOpen; }
	BOOL IsPlaying( ) const { return m_isPlaying; }
	BOOL IsFinished( ) const { return m_isFinished; }
	BOOL IsPaused( ) const { return m_isPaused; }

	static WNDPROC		m_windowProc;
	static aui_Movie	*m_onScreenMovie;

	uint32 GetFlags() { return m_flags; }

	aui_Surface *GetWindowSurface() { return m_windowSurface; }
	void SetWindowSurface(aui_Surface *surf) { m_windowSurface = surf; }
	RECT *GetWindowRect() { return &m_windowRect; }
	void SetWindowRect(RECT *rect) { m_windowRect = *rect; }

protected:

	AUI_ERRCODE PlayOnScreenMovie( );

	MBCHAR m_filename[ MAX_PATH + 1 ];
	aui_MovieFormat *m_format;

	aui_Surface	*m_surface;
	RECT		m_rect;

	aui_Surface *m_windowSurface;
	RECT		m_windowRect;

	BOOL		m_isOpen;
	BOOL		m_isPlaying;
	BOOL		m_isFinished;
	BOOL		m_isPaused;

	uint32		m_flags;

	uint32		m_timePerFrame;
	uint32		m_lastFrameTime;

	uint32				m_curFrame;
};









class aui_AviMovieFormat : public aui_MovieFormat
{
public:

	aui_AviMovieFormat() = default;
	~aui_AviMovieFormat() override = default;

	AUI_ERRCODE	Load( MBCHAR const * filename, aui_Movie *movie ) override
	{ return AUI_ERRCODE_OK; }
};




LRESULT CALLBACK OnScreenMovieWindowProc(
	HWND hwnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam );

#endif
