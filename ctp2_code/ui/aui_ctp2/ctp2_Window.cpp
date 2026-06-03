#include "ctp/c3.h"
#include "ui/aui_ctp2/ctp2_Window.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_control.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_ui.h"
#include "ui/aui_ctp2/pattern.h"

#include "ui/aui_utils/primitives.h"

#include "ui/aui_common/aui_action.h"

#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/ctp2_Static.h"
#include "gs/database/StrDB.h"

#include "ui/ldl/ldl_data.hpp"


#define k_DOCK_SNAP_MARGIN 10

#define k_STANDARD_MAIN_WINDOW_X 224
#define k_STANDARD_MAIN_WINDOW_Y 28

ctp2_Window::ctp2_Window(
	AUI_ERRCODE *retval,
	uint32 id,
	MBCHAR *ldlBlock,
	sint32 bpp,
	AUI_WINDOW_TYPE type,
	bool bevel)
	:
	aui_Window( retval, id, ldlBlock, bpp, type ),
	PatternBase( ldlBlock, nullptr )
{
	m_bevel = bevel;

	*retval = InitCommon();
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	m_weaklyModalCancelCallback = nullptr;
	m_weaklyModalCancelCookie = nullptr;
	m_dockedTo = nullptr;
	m_dock = nullptr;
}


ctp2_Window::ctp2_Window(
	AUI_ERRCODE *retval,
	uint32 id,
	sint32 x,
	sint32 y,
	sint32 width,
	sint32 height,
	sint32 bpp,
	MBCHAR *pattern,
	AUI_WINDOW_TYPE type,
	bool bevel)
	:
	aui_Window( retval, id, x, y, width, height, bpp, type ),
	PatternBase( pattern )
{
	m_bevel = bevel;

	*retval = InitCommon();
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	m_weaklyModalCancelCallback = nullptr;
	m_weaklyModalCancelCookie = nullptr;
	m_dockedTo = nullptr;
	m_dock = nullptr;
}


AUI_ERRCODE ctp2_Window::InitCommon( )
{
	GrabRegion()->Move( 0, 0 );
	GrabRegion()->Resize( m_width, 20 );

	SetDynamic(TRUE);

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE ctp2_Window::DrawThis( aui_Surface *surface, sint32 x, sint32 y )
{

	if ( IsHidden() ) return AUI_ERRCODE_OK;

	RECT rect = { 0, 0, m_width, m_height };

	fprintf(stderr, "[C3W] DrawThis: id=%u pattern=%p surface=%p w=%d h=%d\n",
		Id(), (void*)m_pattern, (void*)m_surface, m_width, m_height);

	if (m_pattern)
		m_pattern->Draw( m_surface, &rect );
	else
		fprintf(stderr, "[C3W] DrawThis: id=%u NO PATTERN - will be black\n", Id());

	m_dirtyList->AddRect( &rect );

	return AUI_ERRCODE_OK;
}

class WeaklyModalCloseAction : public aui_Action
{
public:
	WeaklyModalCloseAction(ctp2_Window *win)
    :   aui_Action  (),
        m_window    (win)
    { ; };

	virtual void	Execute
	(
		aui_Control	*	control,
		uint32			action,
		uint32			data
	)
    {
        if (aui_ui_Get() && m_window)
        {
	        aui_ui_Get()->RemoveWindow(m_window->Id());
        }
    }

protected:
	ctp2_Window *   m_window;
};

void ctp2_Window::MouseLGrabOutside( aui_MouseEvent *mouseData )
{
	if ( IsDisabled() ) return;
	if ( IsWeaklyModal() )
	{


		aui_ui_Get()->AddAction(new WeaklyModalCloseAction(this));

		bool passEventOn = false;
		if(m_weaklyModalCancelCallback) {
			m_weaklyModalCancelCallback(mouseData, this, m_weaklyModalCancelCookie, passEventOn);
		}
		if ( !passEventOn && m_mouseCode == AUI_ERRCODE_UNHANDLED )

			m_mouseCode = AUI_ERRCODE_HANDLEDEXCLUSIVE;

	}
}

void    ctp2_Window::ResetCurrentMouseState()
{
	m_mouseState = *aui_ui_Get()->TheMouse()->GetLatestMouseEvent();
}


AUI_ERRCODE ctp2_Window::DoneInstantiatingThis(const MBCHAR *ldlBlock)
{
	ctp2_Static *background = (ctp2_Static *)aui_Ldl::GetObject((MBCHAR *)ldlBlock, "Background");
	if(background) {
		background->Enable(FALSE);
	}

	ldl_datablock * block = aui_Ldl::FindDataBlock((MBCHAR *)ldlBlock);
	if(block) {
		MBCHAR *title = block->GetString("title");
		if(title) {
			ctp2_Static *titleBar = (ctp2_Static *)aui_Ldl::GetObject((MBCHAR *)ldlBlock, "TitleBar");
			if(!titleBar) {
				titleBar = (ctp2_Static *)aui_Ldl::GetObject((MBCHAR *)ldlBlock, "Background.TitleBar");
			}
			if(titleBar) {
				if(stringdb_Get()->GetNameStr(title)) {
					titleBar->SetText((MBCHAR *)stringdb_Get()->GetNameStr(title));
				}
				SetDraggable(TRUE);
				titleBar->Move(0, 0);
				titleBar->Resize(m_width, titleBar->Height());
				titleBar->SetBlindness(TRUE);
				titleBar->Enable(FALSE);
			}
		}
	}

	if(block->GetBool("centeredwindow")) {
		sint32 x, y;
		x = c3ui_Get()->Width() / 2 - Width() / 2;
		y = c3ui_Get()->Height() / 2 - Height() / 2;
		if(x < k_STANDARD_MAIN_WINDOW_X) x = k_STANDARD_MAIN_WINDOW_X;
		if(y < k_STANDARD_MAIN_WINDOW_Y) y = k_STANDARD_MAIN_WINDOW_Y;

		if(x + Width() > c3ui_Get()->Width()) {
			x = c3ui_Get()->Width() - Width();
		}

		if(y + Height() > c3ui_Get()->Height()) {
			y = c3ui_Get()->Height() - Height();
		}
		Move(x, y);
	}

	return AUI_ERRCODE_OK;
}

AUI_ERRCODE
ctp2_Window::Move(sint32 x, sint32 y)
{
	sint32 oldx = X(), oldy = Y();

	AUI_ERRCODE err = aui_Window::Move(x, y);
	PointerList<ctp2_Window>::Walker walk(&m_dockedWindows);
	while(walk.IsValid()) {
		walk.GetObj()->aui_Window::Offset(x - oldx, y - oldy);
		walk.Next();
	}
	return err;

}

AUI_ERRCODE
ctp2_Window::Offset(sint32 dx, sint32 dy)
{
	AUI_ERRCODE err = aui_Window::Offset(dx, dy);

	PointerList<ctp2_Window>::Walker walk(&m_dockedWindows);
	while(walk.IsValid()) {
		walk.GetObj()->aui_Window::Offset(dx, dy);
		walk.Next();
	}

	return err;
}

void ctp2_Window::AddDockedWindow(ctp2_Window *window)
{
	if(window->m_dockedTo) {

		window->m_dockedTo->RemoveDockedWindow(window);
	}

	if(!m_dockedWindows.Find(window)) {
		m_dockedWindows.AddTail(window);
	}

	window->m_dockedTo = this;
}

void ctp2_Window::RemoveDockedWindow(ctp2_Window *window)
{
	if(window->m_dockedTo != this)
		return;

	PointerList<ctp2_Window>::Walker walk(&m_dockedWindows);
	while(walk.IsValid()) {
		if(walk.GetObj() == window) {
			walk.Remove();
		} else {
			walk.Next();
		}
	}

	window->m_dockedTo = nullptr;
}

void ctp2_Window::MouseLDropOutside(aui_MouseEvent *mouseData)
{
	if(IsDisabled()) return;

	if(m_isDragging) {
		if(m_dock) {

			bool docked = false;
			if(abs(m_dock->X() - (X() + Width())) < k_DOCK_SNAP_MARGIN) {
				if((Y() + Height() >= m_dock->Y() - k_DOCK_SNAP_MARGIN) &&
				   (Y() <= m_dock->Y() + m_dock->Height() + k_DOCK_SNAP_MARGIN)) {

					sint32 newy;
					if(abs(Y() - m_dock->Y()) < k_DOCK_SNAP_MARGIN) {
						newy = m_dock->Y();
					} else {
						newy = Y();
					}

					Move(m_dock->X() - Width(), newy);
					m_dock->AddDockedWindow(this);
					docked = true;
				}
			} else if(abs((m_dock->X() + m_dock->Width()) - X()) < k_DOCK_SNAP_MARGIN) {
				if((Y() + Height() >= m_dock->Y() - k_DOCK_SNAP_MARGIN) &&
				   (Y() <= m_dock->Y() + m_dock->Height() + k_DOCK_SNAP_MARGIN)) {

					sint32 newy;
					if(abs(Y() - m_dock->Y()) < k_DOCK_SNAP_MARGIN) {
						newy = m_dock->Y();
					} else {
						newy = Y();
					}

					Move(m_dock->X() + m_dock->Width(), newy);
					m_dock->AddDockedWindow(this);
					docked = true;
				}
			} else if(abs((Y() + Height()) - m_dock->Y()) < k_DOCK_SNAP_MARGIN) {
				if((X() + Width() >= m_dock->X() - k_DOCK_SNAP_MARGIN) &&
				   (X() <= m_dock->X() + m_dock->Width())) {

					Move(X(), m_dock->Y() - Height());
					m_dock->AddDockedWindow(this);
					docked = true;
				}
			} else if(abs(Y() - (m_dock->Y() + m_dock->Height())) < k_DOCK_SNAP_MARGIN) {
				if((X() + Width() >= m_dock->X() - k_DOCK_SNAP_MARGIN) &&
				   (X() <= m_dock->X() + m_dock->Width())) {

					Move(X(), m_dock->Y() + m_dock->Height());
					m_dock->AddDockedWindow(this);
					docked = true;
				}
			}

			if(!docked) {
				m_dock->RemoveDockedWindow(this);
			}

		}
	}

	aui_Window::MouseLDropOutside(mouseData);
}

void ctp2_Window::SetDock(ctp2_Window *window)
{
	m_dock = window;
	if(m_dockedTo && m_dockedTo != m_dock) {
		m_dockedTo->RemoveDockedWindow(this);
	}
}

bool ctp2_Window::HandleKey(uint32 wParam)
{
	if(aui_Window::HandleKey(wParam)) {
		return true;
	}

	if(m_dockedTo) {
		return m_dockedTo->HandleKey(wParam);
	}

	return false;
}
