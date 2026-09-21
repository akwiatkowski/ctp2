//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Single player new game map size selection screen
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
// - Memory leak repaired.
// - Initialized local variables. (Sep 9th 2005 Martin Gühmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_ctp2/c3_popupwindow.h"
#include "ui/aui_ctp2/c3_button.h"
#include "ui/aui_ctp2/c3_listitem.h"
#include "ui/aui_ctp2/c3_dropdown.h"
#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/c3slider.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_common/aui_radio.h"
#include "ui/aui_common/aui_switchgroup.h"
#include "ui/aui_common/aui_uniqueid.h"

#include "gs/database/profileDB.h"

#include "ui/interface/spnewgamemapshapescreen.h"

#include "ui/interface/spnewgamewindow.h"
#include "ui/interface/spnewgamemapsizescreen.h"

#include "ui/aui_ctp2/keypress.h"

#include "gs/utility/Globals.h"

#include <array>
#include <memory>


static std::unique_ptr<c3_PopupWindow>	s_spNewGameMapSizeScreen;


static std::unique_ptr<aui_SwitchGroup>	s_group;
static std::array<std::unique_ptr<aui_Radio>, k_NUM_MAPSIZEBOXES> s_checkBox;

static MBCHAR	checknames[k_NUM_MAPSIZEBOXES][50] = {
	"MapSizeOne",
	"MapSizeTwo",
	"MapSizeThree",
	"MapSizeFour"
};

static sint32 s_useMode = 0;

static sint32 s_mapSizeIndex = 0;
sint32 spnewgamemapsizescreen_getMapSizeIndex( )
{
	return s_mapSizeIndex;
}






void spnewgamemapsizescreen_setMapSizeIndex( sint32 index )
{

	if ( index < 0 || index >= k_NUM_MAPSIZEBOXES )
		return;

	for (sint32 i = 0;i < k_NUM_MAPSIZEBOXES;i++ )
		s_checkBox[ i ]->SetState( 0 );
	s_checkBox[ index ]->SetState( 1 );

	switch ( s_mapSizeIndex = index ) {
	case 0:
		profiledb_Get()->SetMapSize(MAPSIZE_SMALL);

		break;
	case 1:
		profiledb_Get()->SetMapSize(MAPSIZE_MEDIUM);

		break;
	case 2:
		profiledb_Get()->SetMapSize(MAPSIZE_LARGE);

		break;
	case 3:
		profiledb_Get()->SetMapSize(MAPSIZE_GIGANTIC);

		break;
	default:

		Assert( FALSE );
		break;
	}
}





sint32	spnewgamemapsizescreen_displayMyWindow(BOOL viewMode, sint32 useMode)
{
	sint32 retval=0;
	if(!s_spNewGameMapSizeScreen) { retval = spnewgamemapsizescreen_Initialize(); }

	AUI_ERRCODE auiErr;

	for (sint32 i = 0;i < k_NUM_MAPSIZEBOXES;i++ )
		s_checkBox[ i ]->Enable( !viewMode );

	s_useMode = useMode;

	auiErr = c3ui_Get()->AddWindow(s_spNewGameMapSizeScreen.get());
	keypress_RegisterHandler(s_spNewGameMapSizeScreen.get());

	Assert( auiErr == AUI_ERRCODE_OK );

	return retval;
}
sint32 spnewgamemapsizescreen_removeMyWindow(uint32 action)
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return 0;

	uint32 id = s_group->WhichIsSelected();

	if ( id ) {
		for ( sint32 i = 0;i < k_NUM_MAPSIZEBOXES;i ++ ) {
			if ( id == s_checkBox[i]->Id() ) {
				spnewgamemapsizescreen_setMapSizeIndex( i );
			}
		}
	}

	AUI_ERRCODE auiErr;

	auiErr = c3ui_Get()->RemoveWindow( s_spNewGameMapSizeScreen->Id() );
	keypress_RemoveHandler(s_spNewGameMapSizeScreen.get());

	Assert( auiErr == AUI_ERRCODE_OK );

	if ( s_useMode == 1 ) {
		spnewgamemapshapescreen_displayMyWindow( FALSE, 1 );
	}

	spnewgamescreen_update();

	return 1;
}


AUI_ERRCODE spnewgamemapsizescreen_Initialize( aui_Control::ControlActionCallback *callback )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	MBCHAR		controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	MBCHAR		switchBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	sint32 i;

	if ( s_spNewGameMapSizeScreen ) return AUI_ERRCODE_OK;

	strlcpy(windowBlock, "SPNewGameMapSizeScreen", sizeof(windowBlock));

	{
		s_spNewGameMapSizeScreen = std::make_unique<c3_PopupWindow>( &errcode, aui_UniqueId(), windowBlock, 16, AUI_WINDOW_TYPE_FLOATING, false);
		Assert( AUI_NEWOK(s_spNewGameMapSizeScreen, errcode) );
		if ( !AUI_NEWOK(s_spNewGameMapSizeScreen, errcode) ) return errcode;

		s_spNewGameMapSizeScreen->Resize(s_spNewGameMapSizeScreen->Width(),s_spNewGameMapSizeScreen->Height());
		s_spNewGameMapSizeScreen->GrabRegion()->Resize(s_spNewGameMapSizeScreen->Width(),s_spNewGameMapSizeScreen->Height());
		s_spNewGameMapSizeScreen->SetStronglyModal(TRUE);
	}

	if ( !callback ) callback = spnewgamemapsizescreen_backPress;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "Name" );
	s_spNewGameMapSizeScreen->AddTitle( controlBlock );
	s_spNewGameMapSizeScreen->AddClose( callback );


	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "Group" );
	s_group = std::make_unique<aui_SwitchGroup>( &errcode, aui_UniqueId(), controlBlock );
	Assert( AUI_NEWOK(s_group, errcode) );
	if ( !AUI_NEWOK(s_group, errcode) ) return errcode;

	for ( i = 0;i < k_NUM_MAPSIZEBOXES;i++ ) {
		snprintf(switchBlock, sizeof(switchBlock), "%s.%s", controlBlock, checknames[i] );
		s_checkBox[i] = std::make_unique<aui_Radio>( &errcode, aui_UniqueId(), switchBlock );
		Assert( AUI_NEWOK(s_checkBox[i], errcode) );
		if ( !AUI_NEWOK(s_checkBox[i], errcode) ) return errcode;
		s_group->AddSwitch( s_checkBox[i].get() );

	}

	MAPSIZE		size;

	size = profiledb_Get()->GetMapSize();

	switch (size) {
	case MAPSIZE_SMALL:
		s_checkBox[0]->SetState( TRUE );
		break;
	case MAPSIZE_MEDIUM:
		s_checkBox[1]->SetState( TRUE );
		break;
	case MAPSIZE_LARGE:
		s_checkBox[2]->SetState( TRUE );
		break;
	case MAPSIZE_GIGANTIC:
		s_checkBox[3]->SetState( TRUE );
		break;
	}
















	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );

	return AUI_ERRCODE_OK;
}

//----------------------------------------------------------------------------
//
// Name       : spnewgamemapsizescreen_Cleanup
//
// Description: Release the memory of the screen.
//
// Parameters : -
//
// Globals    : s_spNewGameMapSizeScreen
//				s_checkBox
//				s_group
//
// Returns    : AUI_ERRCODE	: always AUI_ERRCODE_OK
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------

AUI_ERRCODE spnewgamemapsizescreen_Cleanup()
{
	if (s_spNewGameMapSizeScreen)
	{
		c3ui_Get()->RemoveWindow(s_spNewGameMapSizeScreen->Id());
		keypress_RemoveHandler(s_spNewGameMapSizeScreen.get());

		for (sint32 i = 0; i < k_NUM_MAPSIZEBOXES; ++i)
		{
			s_checkBox[i].reset();
		}

		s_group.reset();
		s_spNewGameMapSizeScreen.reset();
	}

	return AUI_ERRCODE_OK;
}


void spnewgamemapsizescreen_backPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{


	spnewgamemapsizescreen_removeMyWindow(action);

}
