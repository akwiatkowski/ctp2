//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Single player new game screen
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
// - Clean up any created subscreens when cleaning up the main screen.
// - Always return to main menu, never SP menu (JJB)
// - Repaired memory leaks.
// - Added tribe index check.
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/interface/spnewgamewindow.h"    // spnewgamescreen.h does not exist

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_stringtable.h"
#include "ui/aui_common/aui_switchgroup.h"
#include "ui/aui_common/aui_textfield.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_ctp2/c3_button.h"
#include "ui/aui_ctp2/c3_checkbox.h"
#include "ui/aui_ctp2/c3_dropdown.h"
#include "ui/aui_ctp2/c3_listbox.h"
#include "ui/aui_ctp2/c3_listitem.h"
#include "ui/aui_ctp2/c3_popupwindow.h"
#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/c3_switch.h"
#include "ui/aui_ctp2/c3slider.h"
#include "ui/aui_ctp2/c3textfield.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/c3window.h"
#include "ctp/civ3_main.h"
#include "ctp/civapp.h"
#include "gs/fileio/civscenarios.h"
#include "ui/aui_ctp2/ctp2_button.h"
#include "ui/interface/custommapscreen.h"
#include "gs/fileio/gamefile.h"
#include "gs/utility/gameinit.h"
#include "gs/utility/Globals.h"
#include "ui/interface/hotseatlist.h"
#include "ui/interface/initialplaywindow.h"
#include "ui/interface/loadsavewindow.h"
#include "ui/interface/MessageBoxDialog.h"
#include "gs/database/profileDB.h"                      // profiledb_Get()
#include "ui/interface/scenariowindow.h"
#include "ui/interface/scorewarn.h"
#include "gs/slic/SlicEngine.h"
#include "ui/interface/spnewgamediffscreen.h"
#include "ui/interface/spnewgamemapshapescreen.h"
#include "ui/interface/spnewgamemapsizescreen.h"
#include "ui/interface/spnewgameplayersscreen.h"
#include "ui/interface/spnewgamerandomcustomscreen.h"
#include "ui/interface/spnewgamerulesscreen.h"
#include "ui/interface/spnewgametribescreen.h"
#include "ui/interface/TurnYearStatus.h"

extern c3_PopupWindow       *g_spNewGameTribeScreen;
extern MBCHAR               g_slic_filename[_MAX_PATH];
extern MBCHAR               g_civilisation_filename[_MAX_PATH];

SPNewGameWindow             *g_spNewGameWindow      = nullptr;

BOOL                        g_launchIntoCheatMode = FALSE;

void spnewgamescreen_SetupHotseatOrEmail();

sint32 spnewgamescreen_displayMyWindow()
{

	g_launchIntoCheatMode = FALSE;

	sint32 retval = g_spNewGameWindow ? 0 : spnewgamescreen_Initialize();

	if ( !g_spNewGameTribeScreen ) spnewgametribescreen_Initialize();

	sint32 const    tribeIndex = spnewgametribescreen_getTribeIndex();
	if ((tribeIndex < 0) || (tribeIndex >= INDEX_TRIBE_INVALID))
	{
		spnewgamescreen_setPlayerName(profiledb_Get()->GetLeaderName());
	}
	else
	{
		MBCHAR lname[k_MAX_NAME_LEN + 1];
		spnewgametribescreen_getLeaderName( lname );
		spnewgamescreen_setPlayerName( lname );
	}

	if (g_spNewGameWindow)
	{
		profiledb_Get()->DefaultSettings();
		g_spNewGameWindow->m_useCustomMap=false;
		civpaths_Get()->ClearCurScenarioPath();

		if (slicengine_Get())
		{
			SlicEngine::Reload(g_slic_filename);
		}

		g_spNewGameWindow->Update();
		c3ui_Get()->AddWindow(g_spNewGameWindow);
	}

	return retval;
}

sint32 spnewgamescreen_removeMyWindow(uint32 action)
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return 0;

	AUI_ERRCODE auiErr = c3ui_Get()->RemoveWindow( g_spNewGameWindow->Id() );
	Assert( auiErr == AUI_ERRCODE_OK );

	return 1;
}




AUI_ERRCODE spnewgamescreen_Initialize( )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;

	if (!g_spNewGameWindow)
	{
		MBCHAR windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
		strlcpy(windowBlock, "SPNewGameWindow", sizeof(windowBlock));

		g_spNewGameWindow= new SPNewGameWindow(&errcode, aui_UniqueId(), windowBlock, 16 );
		Assert(AUI_NEWOK(g_spNewGameWindow, errcode));
		if (AUI_NEWOK(g_spNewGameWindow, errcode))
		{
			errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
			Assert(AUI_SUCCESS(errcode));
		}
	}

	return errcode;
}

//----------------------------------------------------------------------------
//
// Name       : spnewgamescreen_Cleanup
//
// Description: Release the memory of the single player new game screen(s).
//
// Parameters : -
//
// Globals    : g_spNewGameWindow
//              c3ui_Get()
//
// Returns    : -
//
// Remark(s)  : Clean up the subscreens as well.
//
//----------------------------------------------------------------------------
void spnewgamescreen_Cleanup()
{
	// Clean up subscreens.
	spnewgamediffscreen_Cleanup();
	spnewgamemapshapescreen_Cleanup();
	spnewgamemapsizescreen_Cleanup();
	spnewgameplayersscreen_Cleanup();
	spnewgamerandomcustomscreen_Cleanup();
	spnewgamerulesscreen_Cleanup();
	spnewgametribescreen_Cleanup();
	custommapscreen_Cleanup();
	scenarioscreen_Cleanup();

	hotseatlist_Cleanup();

	// Clean up main screen
	if (g_spNewGameWindow)
	{
		c3ui_Get()->RemoveWindow(g_spNewGameWindow->Id());
		allocated::clear(g_spNewGameWindow);
	}
}

sint32 spnewgamescreen_setPlayerName( const MBCHAR *name )
{

	if ( g_spNewGameWindow )
		g_spNewGameWindow->m_spName->SetFieldText( name );

	return 1;
}

sint32 spnewgamescreen_update( )
{

	if(g_spNewGameWindow)
		g_spNewGameWindow->Update();

	return 1;
}





sint32 callbackSetSelected(aui_Control *control, void *cookie)
{
	if(control && cookie) {
		c3_DropDown	*mydrop		= (c3_DropDown*)cookie;
		c3_ListBox	*mylist		= (c3_ListBox*) control;
		uint32		index		= mylist->GetSelectedItemIndex();

		mydrop->SetSelectedItem(index);
		return index;
	}
	return -1;
}





void
spnewgamescreen_instaPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{

}

void
spnewgamescreen_startPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action == uint32(AUI_BUTTON_ACTION_EXECUTE) )
	{
		if (c3files_HasLegalCD())
		{
			MBCHAR fieldText[k_MAX_NAME_LEN];
			g_spNewGameWindow->m_spName->GetFieldText(fieldText, k_MAX_NAME_LEN);
			profiledb_Get()->SetLeaderName(fieldText);

			if (gameinit_IsEmailGame() || gameinit_IsHotseatGame())
			{
				spnewgamescreen_SetupHotseatOrEmail();
			}
			else
			{
				spnewgamescreen_removeMyWindow(action);

				profiledb_Get()->SetSaveNote("");
				profiledb_Get()->SetTutorialAdvice(FALSE);
				civapp_Get()->PostStartGameAction();
			}
		}
	}
}

void
spnewgamescreen_returnPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;


	gameinit_SetHotseatGame(FALSE);
	gameinit_SetEmailGame(FALSE);
	is_scenario_Set(FALSE);
	memset(scenario_name_buf(), '\0', k_SCENARIO_NAME_MAX);
	civpaths_Get()->ClearCurScenarioPath();
	civpaths_Get()->ClearCurScenarioPackPath();

	if (spnewgamescreen_removeMyWindow(action))
	{
		// In the new interface there is no SP window
		initialplayscreen_displayMyWindow();
	}

	ScenarioWindow::Hide();
}

void
spnewgamescreen_quitPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	spnewgamescreen_removeMyWindow(action);

	ExitGame();
}




void spnewgamescreen_tribePress( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;


	spnewgametribescreen_displayMyWindow( nullptr, TRUE );

	sint32 index = (sint32)profiledb_Get()->GetCivIndex();

	const sint32 size = k_MAX_NAME_LEN;
	MBCHAR lname[ size + 1 ];
	g_spNewGameWindow->m_spName->GetFieldText( lname, size );

	spnewgametribescreen_setTribeIndex( index, lname );
}
void spnewgamescreen_malePress( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;
}

void spnewgamescreen_femalePress( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;
}

void spnewgamescreen_difficultyPress( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	spnewgamediffscreen_displayMyWindow(FALSE,TRUE);
}

void spnewgamescreen_mapSizePress( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	spnewgamemapsizescreen_displayMyWindow( FALSE, 0 );
}

void spnewgamescreen_playersPress( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	spnewgameplayersscreen_Cleanup();
	spnewgameplayersscreen_displayMyWindow();
}

void spnewgamescreen_mapPress( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	custommapscreen_displayMyWindow();
}

void spnewgamescreen_rulesPress( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	spnewgamerulesscreen_displayMyWindow();
}

void spnewgamescreen_editorPress( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	g_launchIntoCheatMode = TRUE;

	disclaimer_Initialize(spnewgamescreen_startPress);

}





void spnewgamescreen_scenarioExitCallback(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	MBCHAR	tempPath[_MAX_PATH];
	snprintf(tempPath, sizeof(tempPath), "%s%s%s",
	        civpaths_Get()->GetCurScenarioPath(),
	        FILE_SEP,
	        k_SCENARIO_DEFAULT_SAVED_GAME_NAME
	       );

	if (c3files_PathIsValid(tempPath)) {
		if(!c3files_HasLegalCD())
			exit(0);

		SaveInfo *saveInfo = new SaveInfo;

		strlcpy(saveInfo->fileName, k_SCENARIO_DEFAULT_SAVED_GAME_NAME, sizeof(saveInfo->fileName));

		strlcpy(saveInfo->pathName, tempPath, sizeof(saveInfo->pathName));

		if (GameFile::FetchExtendedSaveInfo(tempPath, saveInfo)) {
			MBCHAR scenPath[_MAX_PATH];
			strlcpy(scenPath, civpaths_Get()->GetCurScenarioPath(), sizeof(scenPath));
			start_info_type_Set(saveInfo->startInfoType);
			loadsavescreen_BeginLoadProcess(saveInfo, scenPath);
		}

		delete saveInfo;

		return;
	}

	g_spNewGameWindow->Update();
}





void spnewgamescreen_scenarioPress(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	if (civpaths_Get()->GetCurScenarioPath())
	{
		civpaths_Get()->ClearCurScenarioPath();
		g_spNewGameWindow->Update();
	}
	else
	{
		scenarioscreen_displayMyWindow();
		scenarioscreen_SetExitCallback(spnewgamescreen_scenarioExitCallback);
	}
}




void spnewgamescreen_mapTypePress( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	spnewgamerandomcustomscreen_displayMyWindow();
}

void spnewgamescreen_worldShapePress( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	spnewgamemapshapescreen_displayMyWindow();
}


void spnewgamescreen_clanSelect(aui_Control *control, uint32 action, uint32 data, void *cookie )
{








}
void spnewgamescreen_genderSelect(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_LISTBOX_ACTION_SELECT  ) return;

	if(g_spNewGameWindow) callbackSetSelected(control,cookie);
}
void
spnewgamescreen_preferencePress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{

}
void
spnewgamescreen_pCustomPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{

}

void spnewgamescreen_mapSizeSelect(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_LISTBOX_ACTION_SELECT  ) return;

	if(g_spNewGameWindow) {
		callbackSetSelected(control,cookie);

		switch(((c3_ListBox*)control)->GetSelectedItemIndex()) {
		case 0:
			profiledb_Get()->SetMapSize( MAPSIZE_SMALL);

			break;
		case 1:
			profiledb_Get()->SetMapSize( MAPSIZE_MEDIUM);

			break;
		case 2:
			profiledb_Get()->SetMapSize( MAPSIZE_LARGE);

			break;
		case 3:
			profiledb_Get()->SetMapSize( MAPSIZE_GIGANTIC);

			break;
		default: Assert(0);
		}
	}
}
void spnewgamescreen_worldTypeSelect(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_LISTBOX_ACTION_SELECT  ) return;

	if(g_spNewGameWindow) callbackSetSelected(control,cookie);
}
void spnewgamescreen_worldShapeSelect(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_LISTBOX_ACTION_SELECT  ) return;

	if(g_spNewGameWindow) callbackSetSelected(control,cookie);
}
void spnewgamescreen_difficultySelect(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_LISTBOX_ACTION_SELECT  ) return;

	if(g_spNewGameWindow) {
		callbackSetSelected(control,cookie);

		profiledb_Get()->SetDifficulty( ((c3_ListBox*)control)->GetSelectedItemIndex());
	}
}
void spnewgamescreen_riskLevelSelect(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_LISTBOX_ACTION_SELECT  ) return;

	if(g_spNewGameWindow) {
		callbackSetSelected(control,cookie);

		profiledb_Get()->SetRiskLevel( ((c3_ListBox*)control)->GetSelectedItemIndex() );
	}
}
void
spnewgamescreen_opponentSelect(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_LISTBOX_ACTION_SELECT  ) return;

	if(g_spNewGameWindow) {
		callbackSetSelected(control,cookie);

		profiledb_Get()->SetNPlayers( ((c3_ListBox*)control)->GetSelectedItemIndex()+3 );
	}
}
void
spnewgamescreen_wCustomPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

		custommapscreen_displayMyWindow();
}

void
spnewgamescreen_genocidePress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action == uint32(AUI_SWITCH_ACTION_PRESS) ) {
		uint32 state = data;
//		c3_CheckBox *mycheckbox = (c3_CheckBox*)control;

		profiledb_Get()->SetGenocideRule( state ? FALSE : TRUE );
	}
}
void
spnewgamescreen_tradePress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action == uint32(AUI_SWITCH_ACTION_PRESS) ) {
		uint32 state = data;
//		c3_CheckBox *mycheckbox = (c3_CheckBox*)control;

		profiledb_Get()->SetTradeRule( state ? FALSE : TRUE );
	}
}
void
spnewgamescreen_combatPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action == uint32(AUI_SWITCH_ACTION_PRESS) ) {
		uint32 state = data;
//		c3_CheckBox *mycheckbox = (c3_CheckBox*)control;

		profiledb_Get()->SetSimpleCombatRule( state ? FALSE : TRUE );
	}
}
void
spnewgamescreen_pollutionPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action == uint32(AUI_SWITCH_ACTION_PRESS) ) {
		uint32 state = data;
//		c3_CheckBox *mycheckbox = (c3_CheckBox*)control;

		profiledb_Get()->SetPollutionRule( state ? FALSE : TRUE );
	}
}






c3_Button* spNew_c3_Button(AUI_ERRCODE *errcode, MBCHAR *ldlParent,MBCHAR *ldlMe,
					void (*callback)(aui_Control*,uint32,uint32,void*))
{
	MBCHAR			textBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	snprintf(textBlock, sizeof(textBlock), "%s.%s", ldlParent, ldlMe );

	return new c3_Button(errcode, aui_UniqueId(), textBlock, callback);
}

ctp2_Button* spNew_ctp2_Button(AUI_ERRCODE *errcode, MBCHAR *ldlParent,MBCHAR *ldlMe,
							   void (*callback)(aui_Control*,uint32,uint32,void*))
{
	MBCHAR			textBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	snprintf(textBlock, sizeof(textBlock), "%s.%s", ldlParent, ldlMe );

	return new ctp2_Button(errcode, aui_UniqueId(), textBlock, callback);
}




ctp2_Button*
spNew_ctp2_Button(AUI_ERRCODE *errcode,
				  MBCHAR *ldlParent,
				  MBCHAR *ldlMe,
				  MBCHAR *default_text,
				  void (*callback)(aui_Control*,uint32,uint32,void*),
				  MBCHAR *buttonFlavor)
{
	MBCHAR		textBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	if (ldlParent==nullptr)
		snprintf(textBlock, sizeof(textBlock), "%s",ldlMe );
	else
		snprintf(textBlock, sizeof(textBlock), "%s.%s", ldlParent, ldlMe );

	return new ctp2_Button
        (errcode, aui_UniqueId(), textBlock,
         buttonFlavor,
         500, 10,
         100, 20,
         nullptr,
         reinterpret_cast<void *>(callback)
        );
}


c3_Switch* spNew_c3_Switch(AUI_ERRCODE *errcode, MBCHAR *ldlParent,MBCHAR *ldlMe,
					void (*callback)(aui_Control*,uint32,uint32,void*), void *cookie)
{
	MBCHAR			textBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	snprintf(textBlock, sizeof(textBlock), "%s.%s", ldlParent, ldlMe );

	return new c3_Switch( errcode, aui_UniqueId(), textBlock, callback, cookie );
}

aui_Switch* spNew_aui_Switch(
	AUI_ERRCODE *errcode,
	MBCHAR *ldlParent,MBCHAR *ldlMe,
	void (*callback)(aui_Control*,uint32,uint32,void*),
	void *cookie)
{
	MBCHAR			textBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	snprintf(textBlock, sizeof(textBlock), "%s.%s", ldlParent, ldlMe );

	return new aui_Switch( errcode, aui_UniqueId(), textBlock, callback, cookie );
}

c3_ListBox* spNew_c3_ListBox(AUI_ERRCODE *errcode, MBCHAR *ldlParent,MBCHAR *ldlMe,
					void (*callback)(aui_Control*,uint32,uint32,void*),
					void *cookie)
{
	MBCHAR			textBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	snprintf(textBlock, sizeof(textBlock), "%s.%s", ldlParent, ldlMe );

	return new c3_ListBox(errcode,aui_UniqueId(), textBlock, callback,cookie );
}

c3_DropDown* spNew_c3_DropDown(AUI_ERRCODE *errcode, MBCHAR *ldlParent,MBCHAR *ldlMe,
					void (*callback)(aui_Control*,uint32,uint32,void*))
{
	MBCHAR			textBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	snprintf(textBlock, sizeof(textBlock), "%s.%s", ldlParent, ldlMe);

	c3_DropDown * myDropDown = new c3_DropDown( errcode, aui_UniqueId(), textBlock);
	myDropDown->GetListBox()->SetActionFuncAndCookie(callback,myDropDown);

	return myDropDown;
}

aui_StringTable* spNewStringTable(AUI_ERRCODE *errcode, MBCHAR *ldlme)
{
	return new aui_StringTable(errcode, ldlme);
}

void spFillDropDown(AUI_ERRCODE *retval, c3_DropDown *mydrop, aui_StringTable *mytable, MBCHAR *listitemparent, MBCHAR *listitemme)
{
	for (sint32 i = 0; i < mytable->GetNumStrings(); ++i)
    {
		mydrop->AddItem
            (new SPDropDownListItem
                (retval, listitemparent, listitemme, mytable->GetString(i))
            );
	}
}
void spFillListBox(AUI_ERRCODE *retval, c3_ListBox *mylist, aui_StringTable *mytable, MBCHAR *listitemparent, MBCHAR *listitemme)
{
	for (sint32 i = 0; i < mytable->GetNumStrings(); i++)
		{
		mylist->AddItem
		    (new SPDropDownListItem
		        (retval, listitemparent, listitemme, mytable->GetString(i))
		    );
	}
}

c3_Static* spNew_c3_Static(AUI_ERRCODE *errcode, MBCHAR *ldlParent,MBCHAR *ldlMe)
{
	MBCHAR			textBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	snprintf(textBlock, sizeof(textBlock), "%s.%s", ldlParent, ldlMe );

	return new c3_Static(errcode, aui_UniqueId(), textBlock);
}

C3TextField* spNewTextEntry(AUI_ERRCODE *errcode, MBCHAR *ldlParent,MBCHAR *ldlMe,
					void (*callback)(aui_Control*,uint32,uint32,void*),void *cookie )

{
	MBCHAR			textBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	snprintf(textBlock, sizeof(textBlock), "%s.%s", ldlParent, ldlMe );

	return new C3TextField( errcode, aui_UniqueId(), textBlock, callback, cookie);
}

TwoChoiceButton* spNewTwoChoiceButton(AUI_ERRCODE *errcode, MBCHAR* ldlParent, MBCHAR *ldlMe,
					MBCHAR *ldlstringtable,uint32 state,
					void (*callback)(aui_Control*,uint32,uint32,void*))
{
	MBCHAR			textBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	MBCHAR *c0= nullptr;
	MBCHAR *c1= nullptr;
	aui_StringTable * choices = spNewStringTable(errcode,ldlstringtable);
	if(choices && choices->GetNumStrings()==2)
	{ c0 = choices->GetString(0); c1 = choices->GetString(1); }

	snprintf(textBlock, sizeof(textBlock), "%s.%s", ldlParent, ldlMe );
	TwoChoiceButton * mybutton = new TwoChoiceButton(errcode,aui_UniqueId(),textBlock,c0,c1,state,callback);

	delete choices;
	return mybutton;
}

C3Slider* spNew_C3Slider(AUI_ERRCODE *errcode, MBCHAR *ldlParent, MBCHAR *ldlMe,
	 					void (*callback)(aui_Control*,uint32,uint32,void*))
{
	MBCHAR			textBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	snprintf(textBlock, sizeof(textBlock), "%s.%s", ldlParent, ldlMe );

    return new C3Slider(errcode, aui_UniqueId(), textBlock, callback);
}
c3_CheckBox* spNew_c3_CheckBox(AUI_ERRCODE *errcode, MBCHAR* ldlParent, MBCHAR *ldlMe,
					uint32 state, void (*callback)(aui_Control*,uint32,uint32,void*), void*cookie)
{
	MBCHAR			textBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	snprintf(textBlock, sizeof(textBlock), "%s.%s", ldlParent, ldlMe );

	c3_CheckBox * mycheck = new c3_CheckBox(errcode,aui_UniqueId(),textBlock,callback,cookie);
	mycheck->SetState(state);

	return mycheck;
}
aui_SwitchGroup* spNew_aui_SwitchGroup( AUI_ERRCODE *errcode, MBCHAR *ldlParent, MBCHAR *ldlMe )
{
	MBCHAR			textBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	snprintf(textBlock, sizeof(textBlock), "%s.%s", ldlParent, ldlMe );

	return new aui_SwitchGroup(errcode, aui_UniqueId(), textBlock);
}

void spnewgamescreen_HotseatCallback(sint32 launch, sint32 player,
									 sint32 civ, BOOL human,
									 MBCHAR *name, MBCHAR *email)
{
	if(launch) {
		DPRINTF(k_DBG_GAMESTATE, ("Hotseat callback: %d, %d, %d, %s\n", player, civ, human, email));

		MBCHAR fieldText[k_MAX_NAME_LEN];

		g_spNewGameWindow->m_spName->GetFieldText(fieldText, k_MAX_NAME_LEN);
		profiledb_Get()->SetLeaderName(fieldText);





		profiledb_Get()->SetSaveNote("");


		profiledb_Get()->SetTutorialAdvice(FALSE);

		civapp_Get()->PostStartGameAction();
	} else {

		hs_player_setup_buf()[player].civ = civ;
		hs_player_setup_buf()[player].isHuman = human;
		hs_player_setup_buf()[player].name = name;
		hs_player_setup_buf()[player].email = email;
	}
}

void spnewgamescreen_SetupHotseatOrEmail()
{
	hs_player_setup_Clear();

	hotseatlist_ClearOptions();
	hotseatlist_EnableAllCivs();

	hotseatlist_DisplayWindow(
		(HotseatListCallback *)spnewgamescreen_HotseatCallback);
}
