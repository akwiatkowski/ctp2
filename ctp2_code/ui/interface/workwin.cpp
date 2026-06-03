//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Worker window (Possibly unsued)
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
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_uniqueid.h"

#include "ui/aui_ctp2/c3_button.h"

#include "ui/aui_ctp2/c3windows.h"
#include "ui/interface/workwindow.h"


#include "ui/aui_ctp2/SelItem.h"

#include "gfx/tilesys/resourcemap.h"
#include "gfx/tilesys/workmap.h"

#include "ui/interface/workwin.h"

extern sint32		g_ScreenWidth;
extern sint32		g_ScreenHeight;

static WorkWindow	*g_workWindow = NULL;

WorkWindow * workwindow_Get()             { return g_workWindow; }
void         workwindow_Set(WorkWindow *p)    { g_workWindow = p; }

static ResourceMap		*g_resourceMap = NULL;

ResourceMap * resourcemap_Get()           { return g_resourceMap; }
void          resourcemap_Set(ResourceMap *p) { g_resourceMap = p; }


WorkMap *g_workMap = NULL;

static c3_Button			*s_exitButton;


extern SelectedItem			*selitem_Get();

void WorkWinCleanupAction::Execute(aui_Control *control,
									uint32 action,
									uint32 data )
{

	workwin_Cleanup();
}

void WorkWinUpdateAction::Execute(aui_Control *control,
									uint32 action,
									uint32 data )
{
	if (!g_workWindow) return;

	if ( c3ui_Get()->GetWindow(g_workWindow->Id()) ) {
		Unit city;
		if ( selitem_Get()->GetSelectedCity(city) ) {
			if ( g_resourceMap ) {
				g_resourceMap->SetUnit( city );
				g_resourceMap->DrawSurface();
			}
		}
	}
}

void WorkExitButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if (!g_workWindow) return;

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	AUI_ERRCODE auiErr;

	auiErr = c3ui_Get()->RemoveWindow( g_workWindow->Id() );
	Assert( auiErr == AUI_ERRCODE_OK );
	if ( auiErr != AUI_ERRCODE_OK ) return;

	WorkWinCleanupAction *tempAction = new WorkWinCleanupAction;
	c3ui_Get()->AddAction( tempAction );
}

sint32 workwin_Initialize( )
{
	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;
	MBCHAR			windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	MBCHAR			controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	if ( g_workWindow ) {
		if ( g_resourceMap ) {
			g_resourceMap->DrawSurface();
		}
		return 0;
	}

























	strcpy(windowBlock, "WorkWindow");

	g_workWindow = new WorkWindow(&errcode, aui_UniqueId(), windowBlock, 16, AUI_WINDOW_TYPE_FLOATING);
	Assert( AUI_NEWOK(g_workWindow, errcode) );
	if ( !AUI_NEWOK(g_workWindow, errcode) ) return -1;

	g_workWindow->GrabRegion()->Move( 0, 0 );
	g_workWindow->GrabRegion()->Resize( g_workWindow->Width(), 20 );
	g_workWindow->SetDraggable( TRUE );

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "ResourceMap" );
	g_resourceMap = new ResourceMap( &errcode, aui_UniqueId(), controlBlock );
	Assert( AUI_NEWOK(g_resourceMap, errcode) );
	if ( !AUI_NEWOK(g_resourceMap, errcode) ) return -3;










	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "ExitButton" );
	s_exitButton = new c3_Button( &errcode, aui_UniqueId(), controlBlock, WorkExitButtonActionCallback );
	Assert( AUI_NEWOK(s_exitButton, errcode) );
	if ( !AUI_NEWOK(s_exitButton, errcode) ) return -5;

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );
	if ( !AUI_SUCCESS(errcode) ) return -1;

	return 0;
}

sint32 workwin_Cleanup( )
{
	if ( !g_workWindow ) return 0;

	c3ui_Get()->RemoveWindow( g_workWindow->Id() );

	delete s_exitButton;
	s_exitButton = NULL;

	delete g_workWindow;
	g_workWindow = NULL;

	if (g_resourceMap) {
		delete g_resourceMap;
		g_resourceMap = NULL;
	}

	return 0;
}

sint32 workwin_Update( )
{
	if ( g_workWindow ) {
		if ( c3ui_Get()->GetWindow(g_workWindow->Id()) ) {
			if ( g_resourceMap ) {
				g_resourceMap->DrawSurface();
			}
		}
	}

	return 1;
}
