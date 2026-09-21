//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : User interface background window
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
// __USING_SPANS
// unknown
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Event handlers declared in a notation that is more standard C++.
// - #pragma once commented out.
//
//----------------------------------------------------------------------------

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __BACKGROUND_H__
#define __BACKGROUND_H__

#include "ui/aui_common/aui_window.h"
#include <memory>
#include "gs/world/MapPoint.h"






typedef AUI_ERRCODE (DrawHandler)(LPVOID);


class Background : public aui_Window
{

    MapPoint m_current_mouse_tile;

public:
	Background(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		sint32 bpp,
		DrawHandler *TheDrawHandler )
		:
		aui_Window( retval, id, x, y, width, height, bpp, AUI_WINDOW_TYPE_BACKGROUND ),
		m_TheDrawHandler( TheDrawHandler ) {
            m_lbutton_isdown = FALSE;
            m_rbutton_isdown = FALSE;
            m_current_mouse_tile.Set(-1,-1);
#ifdef __USING_SPANS__

			m_dirtyList = std::make_unique<aui_DirtyList>( TRUE, width, height );
#endif
        }

	~Background() override = default;

	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

	AUI_ERRCODE Idle() override;

protected:
    sint32 m_lbutton_isdown;
    sint32 m_rbutton_isdown;

    void	MouseLDragOver(aui_MouseEvent * data) override;
    void	MouseLGrabInside(aui_MouseEvent * data) override;
    void	MouseLDropInside(aui_MouseEvent * data) override;
    void	MouseLDropOutside(aui_MouseEvent * data) override;
    void	MouseLDragInside(aui_MouseEvent * data) override;
    void	MouseRGrabInside(aui_MouseEvent * data) override;
    void	MouseMoveOver(aui_MouseEvent * data) override;
    void	MouseMoveInside(aui_MouseEvent * data) override;
    void	MouseMoveAway(aui_MouseEvent * data) override;
    void	MouseMoveOutside(aui_MouseEvent * data) override;
    void	MouseNoChange(aui_MouseEvent * data) override;
    void	MouseLDoubleClickInside(aui_MouseEvent * data) override;
    void	MouseRDoubleClickInside(aui_MouseEvent * data) override;

    void ProcessLastMouseMoveThisFrame(aui_MouseEvent *data);

	DrawHandler *m_TheDrawHandler;
};

#endif
