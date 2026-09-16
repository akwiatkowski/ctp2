//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : User interface control window
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
// _MSC_VER
// - Use Microsoft C++ extensions when set.
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Event handlers declared in a notation that is more standard C++.
//
//----------------------------------------------------------------------------

#ifndef __AUI_WIN_H__
#define __AUI_WIN_H__

#include "ui/aui_common/aui_control.h"


class aui_Win : public aui_Control
{
public:

	aui_Win(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR const *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	aui_Win(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	~aui_Win() override;

protected:
	aui_Win() : aui_Control() {}
	AUI_ERRCODE InitCommonLdl( MBCHAR const *ldlBlock );
	AUI_ERRCODE InitCommon( );

public:
	BOOL	IsRegistered( ) const { return m_registered; }
	MBCHAR	*GetWindowClass( ) const { return m_windowClass; }

	HWND	TheHWND( ) const { return m_hwnd; }

	aui_Control	*SetKeyboardFocus( ) override;

	static aui_Win *GetWinFromHWND( HWND hwnd );

	AUI_ERRCODE	DrawThis(
		aui_Surface *surface,
		sint32 x,
		sint32 y ) override;

protected:
	static BOOL		m_registered;
	static MBCHAR	*m_windowClass;
	static sint32	m_winRefCount;

	HWND			m_hwnd;

	POINT			m_offscreen;

	HDC				m_memdc;
	HBITMAP			m_hbitmap;
	HBITMAP			m_hbitmapOld;

	static tech_WLList<aui_Win *> *m_winList;

	void			WinMouseMove(aui_MouseEvent * mouseData);
	void			WinMouseLDrag(aui_MouseEvent * mouseData);
	void			WinMouseRDrag(aui_MouseEvent * mouseData);

	void	MouseMoveOver(aui_MouseEvent * mouseData) override;
	void	MouseMoveAway(aui_MouseEvent * mouseData) override;
	void	MouseMoveInside(aui_MouseEvent * mouseData) override;
	void	MouseMoveOutside(aui_MouseEvent * mouseData) override;

	void	MouseLDragOver(aui_MouseEvent * mouseData) override;
	void	MouseLDragAway(aui_MouseEvent * mouseData) override;
	void	MouseLDragInside(aui_MouseEvent * mouseData) override;
	void	MouseLDragOutside(aui_MouseEvent * mouseData) override;
	void	MouseRDragOver(aui_MouseEvent * mouseData) override;
	void	MouseRDragAway(aui_MouseEvent * mouseData) override;
	void	MouseRDragInside(aui_MouseEvent * mouseData) override;
	void	MouseRDragOutside(aui_MouseEvent * mouseData) override;

	void	MouseLGrabInside(aui_MouseEvent * mouseData) override;
	void	MouseLGrabOutside(aui_MouseEvent * mouseData) override;
	void	MouseLDropInside(aui_MouseEvent * mouseData) override;
	void	MouseLDropOutside(aui_MouseEvent * mouseData) override;
	void	MouseRGrabInside(aui_MouseEvent * mouseData) override;
	void	MouseRGrabOutside(aui_MouseEvent * mouseData) override;
	void	MouseRDropInside(aui_MouseEvent * mouseData) override;
	void	MouseRDropOutside(aui_MouseEvent * mouseData) override;

	void	MouseLDoubleClickInside(aui_MouseEvent * mouseData) override;
	void	MouseRDoubleClickInside(aui_MouseEvent * mouseData) override;
};

#endif
