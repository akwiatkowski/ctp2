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



static c3_PopupWindow		*s_gameplayoptionsWindow	= nullptr;
static aui_Switch			*s_tutorialadvice			= nullptr,

							*s_autocycleturn			= nullptr,
							*s_autocycleunits			= nullptr,
							*s_battleview				= nullptr,

							*s_enemyMoves				= nullptr,
							*s_autoCenter				= nullptr,
							*s_autoTabSelect			= nullptr,
							*s_battleViewAlways			= nullptr,
							*s_autoSave					= nullptr,

							*s_EnemyHealth				= nullptr, //emod1

							*s_leftHandedMouse			= nullptr;

static C3Slider				*s_mouseSpeed				= nullptr;
static c3_Static			*s_mouseSpeedN				= nullptr;

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
	auiErr = c3ui_Get()->AddWindow( s_gameplayoptionsWindow );
	Assert( auiErr == AUI_ERRCODE_OK );
	keypress_RegisterHandler(s_gameplayoptionsWindow);

	return retval;
}
sint32 gameplayoptions_removeMyWindow(uint32 action)
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return 0;

	AUI_ERRCODE auiErr;

	auiErr = c3ui_Get()->RemoveWindow( s_gameplayoptionsWindow->Id() );
	Assert( auiErr == AUI_ERRCODE_OK );
	keypress_RemoveHandler(s_gameplayoptionsWindow);

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
	s_gameplayoptionsWindow = new c3_PopupWindow(
		&errcode,
		aui_UniqueId(),
		windowBlock,
		16,
		AUI_WINDOW_TYPE_FLOATING,
		false);
	Assert( AUI_NEWOK(s_gameplayoptionsWindow, errcode) );
	if ( !AUI_NEWOK(s_gameplayoptionsWindow, errcode) ) errcode;

	s_gameplayoptionsWindow->SetStronglyModal(TRUE);

	s_tutorialadvice	= spNew_aui_Switch(&errcode,windowBlock,"TutorialButton",
								gameplayoptions_checkPress,&check[GP_TUTORIALADVICE]);

	s_autocycleturn		= spNew_aui_Switch(&errcode,windowBlock,"AutoCycleTurnButton",
								gameplayoptions_checkPress,&check[GP_AUTOCYCLETURN]);
	s_autocycleunits	= spNew_aui_Switch(&errcode,windowBlock,"AutoCycleUnitsButton",
								gameplayoptions_checkPress,&check[GP_AUTOCYCLEUNITS]);
	s_battleview		= spNew_aui_Switch(&errcode,windowBlock,"BattleViewButton",
								gameplayoptions_checkPress,&check[GP_BATTLEVIEW]);

	s_enemyMoves		= spNew_aui_Switch(&errcode,windowBlock,"EnemyMovesButton",
								gameplayoptions_checkPress,&check[GP_ENEMYMOVES]);
	s_autoCenter		= spNew_aui_Switch(&errcode,windowBlock,"AutoCenterButton",
								gameplayoptions_checkPress,&check[GP_AUTOCENTER]);
	s_autoTabSelect		= spNew_aui_Switch(&errcode,windowBlock,"AutoTabSelectButton",
								gameplayoptions_checkPress,&check[GP_AUTOTABSELECT]);

	s_battleViewAlways	= spNew_aui_Switch(&errcode, windowBlock, "BattleViewAlwaysButton",
								gameplayoptions_checkPress, &check[GP_BATTLEVIEWALWAYS]);

	s_mouseSpeed		 = spNew_C3Slider(&errcode, windowBlock, "MouseSpeedSlider",
								gameplayoptions_mouseSlide);
	s_mouseSpeedN		 = spNew_c3_Static(&errcode, windowBlock, "MouseSpeedName");

	s_autoSave			= spNew_aui_Switch(&errcode, windowBlock, "AutoSaveButton",
								gameplayoptions_checkPress, &check[GP_AUTOSAVE]);

	s_leftHandedMouse	= spNew_aui_Switch(&errcode, windowBlock, "LeftHandedMouseButton",
								gameplayoptions_checkPress, &check[GP_LEFTHANDEDMOUSE]);
	//emod5
	s_EnemyHealth		= spNew_aui_Switch(&errcode, windowBlock, "EnemyHealthButton",
								gameplayoptions_checkPress, &check[GP_ENEMYHEALTH]);







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
#define mycleanup(mypointer) if(mypointer) { delete mypointer; mypointer = NULL; };

	if ( !s_gameplayoptionsWindow  ) return AUI_ERRCODE_OK;

	c3ui_Get()->RemoveWindow( s_gameplayoptionsWindow->Id() );
	keypress_RemoveHandler(s_gameplayoptionsWindow);

	mycleanup(s_tutorialadvice);

	mycleanup(s_autocycleturn);
	mycleanup(s_autocycleunits);
	mycleanup(s_battleview);

	mycleanup(s_enemyMoves);
	mycleanup(s_autoCenter);
	mycleanup(s_autoTabSelect);

	mycleanup(s_battleViewAlways);
	mycleanup(s_mouseSpeed);
	mycleanup(s_mouseSpeedN);
	mycleanup(s_autoSave);
	mycleanup(s_leftHandedMouse);
	mycleanup(s_EnemyHealth); //emod 6

	delete s_gameplayoptionsWindow;
	s_gameplayoptionsWindow = nullptr;

	return AUI_ERRCODE_OK;
#undef mycleanup
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
