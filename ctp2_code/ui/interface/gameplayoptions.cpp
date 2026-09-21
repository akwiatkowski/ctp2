//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Gameplay options screen
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
// - 7 modifications required to add a button
//
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
// - added citycapture options
// - added show enemy health
// - added show debug AI text
// - Removed city capture options (already in rules window) and debug AI -
//	 (already in scenario editor). (10-Apr-2009 Maq)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include <memory>
#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_ctp2/c3_popupwindow.h"
#include "ui/aui_ctp2/c3_button.h"
#include "ui/aui_ctp2/c3_listitem.h"
#include "ui/aui_ctp2/c3_dropdown.h"
#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/c3slider.h"
#include "ui/aui_common/aui_switch.h"
#include "ui/aui_ctp2/c3_checkbox.h"
#include "ui/aui_ctp2/c3ui.h"
#include "gs/database/profileDB.h"
#include "ui/interface/spnewgamewindow.h"
#include "ui/interface/gameplayoptions.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/interface/screenutils.h"
#include "net/general/network.h"

#include "ui/aui_ctp2/keypress.h"
#include "gs/gameobj/GameSettings.h"



static std::unique_ptr<c3_PopupWindow> s_gameplayoptionsWindow;
static std::unique_ptr<aui_Switch> s_tutorialadvice,

							s_autocycleturn,
							s_autocycleunits,
							s_battleview,

							s_enemyMoves,
							s_autoCenter,
							s_autoTabSelect,
							s_battleViewAlways,
							s_autoSave,

							s_EnemyHealth, //emod1

							s_leftHandedMouse;

static std::unique_ptr<C3Slider> s_mouseSpeed;
static std::unique_ptr<c3_Static> s_mouseSpeedN;

static BOOL					s_leftHandedMouseFlag = FALSE;

enum
{
	GP_TUTORIALADVICE,

	GP_AUTOCYCLETURN,
	GP_AUTOCYCLEUNITS,
	GP_BATTLEVIEW,

	GP_ENEMYMOVES,
	GP_AUTOCENTER,
	GP_AUTOTABSELECT,
	GP_BATTLEVIEWALWAYS,
	GP_AUTOSAVE,
	GP_LEFTHANDEDMOUSE,
	GP_ENEMYHEALTH,
	GP_TOTAL
};

static uint32 check[] =
{
	GP_TUTORIALADVICE,

	GP_AUTOCYCLETURN,
	GP_AUTOCYCLEUNITS,
	GP_BATTLEVIEW,

	GP_ENEMYMOVES,
	GP_AUTOCENTER,
	GP_AUTOTABSELECT,
	GP_BATTLEVIEWALWAYS,
	GP_AUTOSAVE,
	GP_LEFTHANDEDMOUSE,
	GP_ENEMYHEALTH,
	GP_TOTAL
};

sint32 gameplayoptions_updateData()
{
	if ( !profiledb_Get() ) return -1;

	GameSettings *gs = gamesettings_Get();
	sint32 diff = gs ? gs->GetDifficulty() : profiledb_Get()->GetDifficulty();

	if(diff >= 2 || network_Get().IsActive()) {
		s_tutorialadvice->SetState(0);
		s_tutorialadvice->Enable(FALSE);
	} else {
		s_tutorialadvice->SetState( profiledb_Get()->IsTutorialAdvice() );
	}




	s_autocycleturn->SetState( profiledb_Get()->IsAutoTurnCycle() );

	s_autocycleunits->SetState( profiledb_Get()->IsAutoSelectNext() );

	s_battleview->SetState( profiledb_Get()->IsShowZoomedCombat() );

	s_battleViewAlways->SetState(profiledb_Get()->IsZoomedCombatAlways());
	s_autoSave->SetState(profiledb_Get()->IsAutoSave());
	s_mouseSpeed->SetValue(profiledb_Get()->GetMouseSpeed(), 0);

	s_enemyMoves->SetState(profiledb_Get()->IsEnemyMoves());
	s_autoCenter->SetState(profiledb_Get()->IsAutoCenter());
	s_autoTabSelect->SetState( profiledb_Get()->GetAutoSwitchTabs() );
	s_EnemyHealth->SetState( profiledb_Get()->GetShowEnemyHealth() ); //emod4

	return 1;
}


sint32	gameplayoptions_displayMyWindow()
{
	s_leftHandedMouseFlag = profiledb_Get()->GetLeftHandedMouse();

	sint32 retval=0;
	if(!s_gameplayoptionsWindow) { retval = gameplayoptions_Initialize(); }

	s_leftHandedMouse->SetState(s_leftHandedMouseFlag);

	gameplayoptions_updateData();

	AUI_ERRCODE auiErr;
	auiErr = c3ui_Get()->AddWindow( s_gameplayoptionsWindow.get() );
	Assert( auiErr == AUI_ERRCODE_OK );
	keypress_RegisterHandler(s_gameplayoptionsWindow.get());

	return retval;
}
sint32 gameplayoptions_removeMyWindow(uint32 action)
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return 0;

	AUI_ERRCODE auiErr;

	auiErr = c3ui_Get()->RemoveWindow( s_gameplayoptionsWindow->Id() );
	Assert( auiErr == AUI_ERRCODE_OK );
	keypress_RemoveHandler(s_gameplayoptionsWindow.get());

	return 1;
}




AUI_ERRCODE gameplayoptions_Initialize( )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	if ( s_gameplayoptionsWindow ) {








		return AUI_ERRCODE_OK;
	}

	strlcpy(windowBlock, "GamePlayOptionsWindow", sizeof(windowBlock));
	s_gameplayoptionsWindow = std::make_unique<c3_PopupWindow>(
		&errcode,
		aui_UniqueId(),
		windowBlock,
		16,
		AUI_WINDOW_TYPE_FLOATING,
		false);
	Assert( AUI_NEWOK(s_gameplayoptionsWindow, errcode) );
	if ( !AUI_NEWOK(s_gameplayoptionsWindow, errcode) ) return errcode;

	s_gameplayoptionsWindow->SetStronglyModal(TRUE);

	s_tutorialadvice.reset(spNew_aui_Switch(&errcode,windowBlock,"TutorialButton",
								gameplayoptions_checkPress,&check[GP_TUTORIALADVICE]));

	s_autocycleturn.reset(spNew_aui_Switch(&errcode,windowBlock,"AutoCycleTurnButton",
								gameplayoptions_checkPress,&check[GP_AUTOCYCLETURN]));
	s_autocycleunits.reset(spNew_aui_Switch(&errcode,windowBlock,"AutoCycleUnitsButton",
								gameplayoptions_checkPress,&check[GP_AUTOCYCLEUNITS]));
	s_battleview.reset(spNew_aui_Switch(&errcode,windowBlock,"BattleViewButton",
								gameplayoptions_checkPress,&check[GP_BATTLEVIEW]));

	s_enemyMoves.reset(spNew_aui_Switch(&errcode,windowBlock,"EnemyMovesButton",
								gameplayoptions_checkPress,&check[GP_ENEMYMOVES]));
	s_autoCenter.reset(spNew_aui_Switch(&errcode,windowBlock,"AutoCenterButton",
								gameplayoptions_checkPress,&check[GP_AUTOCENTER]));
	s_autoTabSelect.reset(spNew_aui_Switch(&errcode,windowBlock,"AutoTabSelectButton",
								gameplayoptions_checkPress,&check[GP_AUTOTABSELECT]));

	s_battleViewAlways.reset(spNew_aui_Switch(&errcode, windowBlock, "BattleViewAlwaysButton",
								gameplayoptions_checkPress, &check[GP_BATTLEVIEWALWAYS]));

	s_mouseSpeed.reset(spNew_C3Slider(&errcode, windowBlock, "MouseSpeedSlider",
								gameplayoptions_mouseSlide));
	s_mouseSpeedN.reset(spNew_c3_Static(&errcode, windowBlock, "MouseSpeedName"));

	s_autoSave.reset(spNew_aui_Switch(&errcode, windowBlock, "AutoSaveButton",
								gameplayoptions_checkPress, &check[GP_AUTOSAVE]));

	s_leftHandedMouse.reset(spNew_aui_Switch(&errcode, windowBlock, "LeftHandedMouseButton",
								gameplayoptions_checkPress, &check[GP_LEFTHANDEDMOUSE]));
	//emod5
	s_EnemyHealth.reset(spNew_aui_Switch(&errcode, windowBlock, "EnemyHealthButton",
								gameplayoptions_checkPress, &check[GP_ENEMYHEALTH]));







	gameplayoptions_updateData();

	MBCHAR block[ k_AUI_LDL_MAXBLOCK + 1 ];
	snprintf(block, sizeof(block), "%s.%s", windowBlock, "Name" );
	s_gameplayoptionsWindow->AddTitle( block );
	s_gameplayoptionsWindow->AddClose( gameplayoptions_exitPress );

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE gameplayoptions_Cleanup()
{
	if ( !s_gameplayoptionsWindow  ) return AUI_ERRCODE_OK;

	c3ui_Get()->RemoveWindow( s_gameplayoptionsWindow->Id() );
	keypress_RemoveHandler(s_gameplayoptionsWindow.get());

	// Same release order the mycleanup macro used.
	s_tutorialadvice.reset();

	s_autocycleturn.reset();
	s_autocycleunits.reset();
	s_battleview.reset();

	s_enemyMoves.reset();
	s_autoCenter.reset();
	s_autoTabSelect.reset();

	s_battleViewAlways.reset();
	s_mouseSpeed.reset();
	s_mouseSpeedN.reset();
	s_autoSave.reset();
	s_leftHandedMouse.reset();
	s_EnemyHealth.reset(); //emod 6

	s_gameplayoptionsWindow.reset();

	return AUI_ERRCODE_OK;
}

void gameplayoptions_checkPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_SWITCH_ACTION_PRESS ) return;

	uint32 checkbox = *((uint32*)cookie);
	void (ProfileDB::*func)(BOOL) = nullptr;
	uint32 state = data;

	switch(checkbox) {
	case GP_TUTORIALADVICE:	func = &ProfileDB::SetTutorialAdvice; break;

	case GP_AUTOCYCLETURN:	func = &ProfileDB::SetAutoTurnCycle; break;
	case GP_AUTOCYCLEUNITS:
		state = !state;
		profiledb_Get()->SetAutoSelectFirstUnit(state);
		profiledb_Get()->SetAutoSelectNext(state);
		profiledb_Get()->SetAutoDeselect(state);
		break;
	case GP_BATTLEVIEW:		func = &ProfileDB::SetShowZoomedCombat; break;
	case GP_ENEMYMOVES:		func = &ProfileDB::SetEnemyMoves; break;
	case GP_AUTOCENTER:		func = &ProfileDB::SetAutoCenter; break;
	case GP_AUTOTABSELECT:	func = &ProfileDB::SetAutoSwitchTabs; break;
	case GP_TOTAL:  break;
	case GP_BATTLEVIEWALWAYS: func = &ProfileDB::SetZoomedCombatAlways; break;
	case GP_AUTOSAVE: func = &ProfileDB::SetAutoSave; break;
	case GP_ENEMYHEALTH: func = &ProfileDB::SetEnemyHealth; break; //emod7
	case GP_LEFTHANDEDMOUSE:
		s_leftHandedMouseFlag = !state;
		func = nullptr;

		break;

	default:  Assert(0); break;
	};

	if(func)
		(profiledb_Get()->*func)(state ? FALSE : TRUE);
}
void gameplayoptions_exitPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	profiledb_Get()->SetLeftHandedMouse(s_leftHandedMouseFlag);

	profiledb_Get()->Save();

	gameplayoptions_removeMyWindow(action);
}

void gameplayoptions_mouseSlide(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != AUI_RANGER_ACTION_VALUECHANGE ) return;

	profiledb_Get()->SetMouseSpeed(s_mouseSpeed->GetValueX());

	if (c3ui_Get()->TheMouse()) {
		double sensitivity = 0.0;

		sensitivity = 0.25 * (1 + profiledb_Get()->GetMouseSpeed());

		c3ui_Get()->TheMouse()->Sensitivity() = sensitivity;

	}
}
