//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Background screen handling
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
// - None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Prevented crash
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "ui/aui_common/aui.h"

#include "ui/aui_ctp2/background.h"

#include "gfx/tilesys/tiledmap.h"
#include "robot/aibackdoor/dynarr.h"
#include "ui/aui_ctp2/SelItem.h"
#include "ui/interface/cursormanager.h"
#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_ctp2/c3ui.h"

#include "ui/interface/messageactions.h"
#include "ui/interface/messagewindow.h"
#include "ui/aui_ctp2/InfoBar.h"
#include "gs/database/profileDB.h"

#include "ui/interface/controlpanelwindow.h"
#include "ui/interface/StatusBar.h"
#include "ui/interface/scenarioeditor.h"
#include "ui/aui_ctp2/ctp2_Static.h"
#include "ui/aui_common/aui_ldl.h"

extern C3UI				*g_c3ui;
extern MessageWindow	*g_currentMessageWindow;

enum SEV_TYPE {
	SEV_LGRAB,
	SEV_RGRAB,
	SEV_LDROP,
	SEV_LDRAG,
	SEV_LDOUBLE
};

struct SavedMouseEvent {
	aui_MouseEvent event;
	SEV_TYPE type;
};


PointerList<SavedMouseEvent> s_savedEvents;

sint32				doubleClickTimeout = 100;

AUI_ERRCODE Background::DrawThis( aui_Surface *surface, sint32 x, sint32 y )
{
	return m_TheDrawHandler ?
		m_TheDrawHandler( this ) : AUI_ERRCODE_NODRAWHANDLER;
}

void Background::MouseLDragOver(aui_MouseEvent *data)
{
	if (IsDisabled()) return;

	sint16 hold;

	hold = 0;
}

void Background::MouseLGrabInside(aui_MouseEvent *data)
{
	if (IsDisabled()) return;

	if (GetWhichSeesMouse() && GetWhichSeesMouse() != this) return;
	SetWhichSeesMouse(this);

	Assert(tiledmap_Get() != NULL);
	if (tiledmap_Get() == NULL) return;

	data->position.x -= X();
	data->position.y -= Y();

	SavedMouseEvent *ev = new SavedMouseEvent;
	memcpy(&ev->event, data, sizeof(aui_MouseEvent));
	ev->type = SEV_LGRAB;
	s_savedEvents.AddTail(ev);


    m_lbutton_isdown = TRUE;










	data->position.x += X();
	data->position.y += Y();
}

void Background::MouseRGrabInside(aui_MouseEvent *data)
{
	if (IsDisabled()) return;

	if (GetWhichSeesMouse() && GetWhichSeesMouse() != this) return;
	SetWhichSeesMouse(this);


	Assert(tiledmap_Get() != NULL);
	if(tiledmap_Get() == NULL) return;

	data->position.x -= X();
	data->position.y -= Y();

	SavedMouseEvent *ev = new SavedMouseEvent;
	memcpy(&ev->event, data, sizeof(aui_MouseEvent));
	ev->type = SEV_RGRAB;
	s_savedEvents.AddTail(ev);





	m_rbutton_isdown = TRUE;










	data->position.x += X();
	data->position.y += Y();
}

void Background::MouseLDropInside(aui_MouseEvent *data)
{
	if (IsDisabled()) return;

	if (GetWhichSeesMouse() && GetWhichSeesMouse() != this) return;
	SetWhichSeesMouse(this);

	data->position.x -= X();
	data->position.y -= Y();

	SavedMouseEvent *ev = new SavedMouseEvent;
	memcpy(&ev->event, data, sizeof(aui_MouseEvent));
	ev->type = SEV_LDROP;
	s_savedEvents.AddTail(ev);


    m_lbutton_isdown = FALSE;


	data->position.x += X();
	data->position.y += Y();
}


void Background::MouseLDropOutside(aui_MouseEvent *data)
{
	if (IsDisabled()) return;

    m_lbutton_isdown = FALSE;

}

extern SelectedItem *g_selected_item;

void Background::MouseMoveInside(aui_MouseEvent *data)

{
	if (IsDisabled()) return;

	if (GetWhichSeesMouse() && GetWhichSeesMouse() != this) return;
	SetWhichSeesMouse(this);

    Assert(data);

    MapPoint tmp;

    if (tiledmap_Get() && tiledmap_Get()->GetMouseTilePos(tmp)){
        m_current_mouse_tile = tmp;
    }

    if (data->movecount==0) {
        ProcessLastMouseMoveThisFrame(data);
    }

	if (tiledmap_Get() != NULL) {
		tiledmap_Get()->SetHiliteMouseTile(m_current_mouse_tile);
		tiledmap_Get()->DrawHilite( TRUE );
	}
}

void Background::MouseLDragInside( aui_MouseEvent *data )
{
	if ( IsDisabled() ) return;
	if (GetWhichSeesMouse() && GetWhichSeesMouse() != this) return;
	if ( !GetWhichSeesMouse() ) SetWhichSeesMouse( this );

	MapPoint tmp;

	data->position.x -= X();
	data->position.y -= Y();

    if (tiledmap_Get() && tiledmap_Get()->GetMouseTilePos(tmp)){
		if (m_current_mouse_tile != tmp) {
			SavedMouseEvent *ev = new SavedMouseEvent;
			memcpy(&ev->event, data, sizeof(aui_MouseEvent));
			ev->type = SEV_LDRAG;
			s_savedEvents.AddTail(ev);


		}
        m_current_mouse_tile = tmp;
    }

    if (data->movecount==0) {
        ProcessLastMouseMoveThisFrame(data);
    }

	if (tiledmap_Get() != NULL) {
		tiledmap_Get()->SetHiliteMouseTile(m_current_mouse_tile);
		tiledmap_Get()->DrawHilite( TRUE );
	}


	data->position.x += X();
	data->position.y += Y();
}

void Background::MouseMoveOver(aui_MouseEvent *data)

{
	if (IsDisabled()) return;

    Assert(data);

    MapPoint tmp;
	if (tiledmap_Get() && tiledmap_Get()->GetMouseTilePos(tmp))
	{
        m_current_mouse_tile = tmp;
    }

    if (data->movecount==0) {
        ProcessLastMouseMoveThisFrame(data);
    }

	cursormanager_Get()->RestoreCursor();
}

void Background::MouseMoveOutside(aui_MouseEvent *data)

{
	if (IsDisabled()) return;

    Assert(data);

    if (data->movecount==0) {
        ProcessLastMouseMoveThisFrame(data);
    }






	m_lbutton_isdown = FALSE;






}


void Background::MouseMoveAway(aui_MouseEvent *data)

{
	if (IsDisabled()) return;

    Assert(data);

    if (data->movecount==0) {
        ProcessLastMouseMoveThisFrame(data);
    }

	cursormanager_Get()->SaveCursor();
	cursormanager_Get()->SetCursor(CURSORINDEX_DEFAULT);

	m_lbutton_isdown = FALSE;
}

void Background::MouseNoChange(aui_MouseEvent *data)

{
	if (IsDisabled()) return;

    Assert(data);

    if (data->movecount==0) {
        ProcessLastMouseMoveThisFrame(data);
    }

}

void Background::ProcessLastMouseMoveThisFrame(aui_MouseEvent *data)
{
	Assert(data);
	MapPoint cur, old;

	g_selected_item->GetOldMouseTilePos(old);
	if(m_current_mouse_tile.x != -1
	&& m_current_mouse_tile   != old
	){
		if((   m_lbutton_isdown == FALSE
		    || g_theProfileDB->IsUseCTP2Mode()
		   )
		&& (   g_selected_item->IsLocalArmy()
		    || g_selected_item->GetState() == SELECT_TYPE_LOCAL_ARMY_UNLOADING
		    ||(g_selected_item->IsLocalCity() && g_theProfileDB->IsDebugCityAstar())
		   )
		){
			g_selected_item->SetDrawablePathDest(m_current_mouse_tile);
		}
		else
		{
			g_selected_item->SetCurMouseTile(m_current_mouse_tile);
		}

		InfoBar *ib = infobar_Get();
		ib->SetTextFromMap(m_current_mouse_tile);

		if(!GetWhichSeesMouse() || GetWhichSeesMouse() == this)
			StatusBar::SetText(ib->GetText());

		if(ScenarioEditor::IsShown())
		{
			sint32 x,y;
			char lemurpoo[_MAX_PATH];
			x = m_current_mouse_tile.x;
			y = m_current_mouse_tile.y;
			snprintf(lemurpoo, sizeof(lemurpoo), "x: %d, y: %d", x, y);
			ctp2_Static *tf = (ctp2_Static *)aui_Ldl::GetObject("ScenarioEditor.WorldControls.PosField");
			tf->SetText(lemurpoo);
		}
	}
}

AUI_ERRCODE Background::Idle(void)
{
	if(tiledmap_Get()) {
		tiledmap_Get()->Idle();
	}

	while(s_savedEvents.GetCount() > 0) {
		SavedMouseEvent *ev = s_savedEvents.GetHead();
		switch(ev->type) {
			case SEV_LGRAB:
			{
				uint32 curTicks = GetTickCount();
				if (curTicks > (ev->event.time + doubleClickTimeout)) {
					tiledmap_Get()->Click(&ev->event, FALSE);
				} else {
					return AUI_ERRCODE_OK;
				}
				break;
			}
			case SEV_RGRAB:
				tiledmap_Get()->Click(&ev->event, FALSE);
				break;
			case SEV_LDROP:
				tiledmap_Get()->Drop(&ev->event);
				break;
			case SEV_LDRAG:
				tiledmap_Get()->MouseDrag(&ev->event);
				break;
			case SEV_LDOUBLE:
				tiledmap_Get()->Click(&ev->event, TRUE);
				break;
		}
		delete s_savedEvents.RemoveHead();
	}

#if 0
	if (hasSavedEvent) {
		uint32 curTicks = GetTickCount();
		if (curTicks > (savedEvent.time + doubleClickTimeout)) {
			tiledmap_Get()->Click(&savedEvent, FALSE);
			hasSavedEvent = FALSE;
		}
	}
#endif
	return AUI_ERRCODE_OK;
}

void Background::MouseLDoubleClickInside(aui_MouseEvent *data)
{
	if (IsDisabled()) return;

	if (GetWhichSeesMouse() && GetWhichSeesMouse() != this) return;
	SetWhichSeesMouse(this);

	Assert(tiledmap_Get() != NULL);
	if (tiledmap_Get() == NULL) return;

	data->position.x -= X();
	data->position.y -= Y();

	if (s_savedEvents.GetCount() > 0) {
		SavedMouseEvent *ev = s_savedEvents.GetTail();
		if(ev->type == SEV_LGRAB) {
			ev->type = SEV_LDOUBLE;
			memcpy(&ev->event, data, sizeof(aui_MouseEvent));
		} else {
			ev = new SavedMouseEvent;
			memcpy(&ev->event, data, sizeof(aui_MouseEvent));
			ev->type = SEV_LDOUBLE;
			s_savedEvents.AddTail(ev);
		}
	} else {
		tiledmap_Get()->Click(data, TRUE);
	}

    m_lbutton_isdown = TRUE;

}

void Background::MouseRDoubleClickInside(aui_MouseEvent *data)
{
	if (IsDisabled()) return;

	if (GetWhichSeesMouse() && GetWhichSeesMouse() != this) return;
	SetWhichSeesMouse(this);

	Assert(tiledmap_Get() != NULL);
	if (tiledmap_Get() == NULL) return;

	data->position.x -= X();
	data->position.y -= Y();

	tiledmap_Get()->Click(data, TRUE);

    m_lbutton_isdown = TRUE;

}
