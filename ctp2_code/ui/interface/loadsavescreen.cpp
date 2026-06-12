//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Load/save screen
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
// you_want_ai_civs_from_singleplayer_saved_game_showing_up_in_netshell
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Repaired memory leaks.
// - Updated tribe index check.
// - Replaced the old civilisation database by a new one. (Aug 21st 2005 Martin G�hmann)
// - Standardized code (May 21st 2006 Martin G�hmann)
//
//----------------------------------------------------------------------------
//
// Remarks
//
// - This is the only file with some Activision comments left.
// - Fixed scenarios so that the from the civ choser selected civ is used. (2-Jan-2008 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_stringtable.h"
#include "ui/aui_common/aui_textfield.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_ctp2/c3_button.h"
#include "ui/aui_ctp2/c3_dropdown.h"
#include "ui/aui_ctp2/c3_listbox.h"
#include "ui/aui_ctp2/c3_listitem.h"
#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/c3_utilitydialogbox.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/c3window.h"
#include "ctp/civ3_main.h"
#include "ctp/civapp.h"
#include "gs/gameobj/Civilisation.h"
#include "CivilisationRecord.h"
#include "gs/fileio/civscenarios.h"               // g_civScenarios
#include "ui/aui_ctp2/ctp2_button.h"
#ifdef WIN32
#include <direct.h>
#endif // WIN32
#include "gs/fileio/gamefile.h"                   // SAVE_LEADER_NAME_SIZE
#include "gs/utility/gameinit.h"
#include "gs/utility/Globals.h"                    // allocated::reassign
#include "ui/interface/hotseatlist.h"
#include "ui/interface/initialplaywindow.h"
#ifdef WIN32
#include <io.h>
#endif // WIN32
#include "ui/aui_ctp2/keypress.h"
#include "ui/interface/loadsavewindow.h"
#include "ui/interface/MessageBoxDialog.h"
#include "ui/netshell/netshell.h"                   // gamesetup_Get()
#include "ui/netshell/ns_gamesetup.h"
#include "ui/interface/optionswindow.h"
#include "gs/gameobj/Player.h"                     // player_Get()
#include "gs/database/profileDB.h"                  // profiledb_Get()
#include "ui/interface/spnewgamediffscreen.h"
#include "ui/interface/spnewgameplayersscreen.h"
#include "ui/interface/spnewgametribescreen.h"
#include "gs/database/StrDB.h"                      // stringdb_Get()
#include "ui/interface/TurnYearStatus.h"

#ifndef WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#endif // !WIN32
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

extern sint32				g_scenarioUsePlayerNumber;
extern BOOL					g_setDifficultyUponLaunch;
extern sint32				g_difficultyToSetUponLaunch;
extern BOOL					g_setBarbarianRiskUponLaunch;
extern sint32				g_barbarianRiskUponLaunch;

SaveInfo *                  g_savedGameRequest  = nullptr;
static LoadSaveWindow *     g_loadsaveWindow    = nullptr;

LoadSaveWindow * loadsavewindow_Get()
{
    return g_loadsaveWindow;
}

static uint32               s_type              = LSS_TOTAL;
static c3_Static *          s_name				= nullptr;
static aui_StringTable *    s_nameString		= nullptr;




void loadsavescreen_deleteDialog( bool response, void *data )
{
	if ( !response ) return;

	loadsavescreen_delete();
}




sint32	loadsavescreen_displayMyWindow(uint32 type)
{
    AUI_ERRCODE retval = g_loadsaveWindow ? AUI_ERRCODE_OK : loadsavescreen_Initialize();

	g_loadsaveWindow->CleanUpSaveInfo();

	if (retval == AUI_ERRCODE_OK)
    {
		g_loadsaveWindow->SetType(type);
		c3ui_Get()->AddWindow(g_loadsaveWindow);
		keypress_RegisterHandler(g_loadsaveWindow);
	}

	return retval;
}

sint32 loadsavescreen_removeMyWindow(uint32 action)
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return 0;

	AUI_ERRCODE auiErr = c3ui_Get()->RemoveWindow( g_loadsaveWindow->Id() );
	Assert( auiErr == AUI_ERRCODE_OK );
	keypress_RemoveHandler(g_loadsaveWindow);

	return 1;
}


AUI_ERRCODE loadsavescreen_Initialize( aui_Control::ControlActionCallback *callback )
{
	if ( g_loadsaveWindow ) return AUI_ERRCODE_OK;

	MBCHAR		windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	strlcpy(windowBlock, "LoadSaveWindow", sizeof(windowBlock));

	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	g_loadsaveWindow= new LoadSaveWindow(&errcode, aui_UniqueId(), windowBlock, 16 , AUI_WINDOW_TYPE_STANDARD);
	Assert( AUI_NEWOK(g_loadsaveWindow, errcode) );
	if ( !AUI_NEWOK(g_loadsaveWindow, errcode) ) return errcode;

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );

	switch ( g_loadsaveWindow->GetType() )
	{
	case LSS_LOAD_GAME:
	case LSS_LOAD_MP:
	case LSS_LOAD_SCEN:
	case LSS_LOAD_SCEN_MP:
		g_loadsaveWindow->GetOkButton()->Enable( FALSE );
		break;

	default:
		g_loadsaveWindow->GetOkButton()->Enable( TRUE );
		break;
	}

	g_loadsaveWindow->GetDeleteButton()->Enable( FALSE );




	g_loadsaveWindow->GetListOne()->GetHeader()->Enable( FALSE );
	g_loadsaveWindow->GetListTwo()->GetHeader()->Enable( FALSE );

	if ( callback )
		g_loadsaveWindow->GetOkButton()->SetActionFuncAndCookie(
			callback, nullptr );

	return AUI_ERRCODE_OK;
}




void loadsavescreen_Cleanup()
{
	if (g_loadsaveWindow)
    {
        if (c3ui_Get())
        {
	        c3ui_Get()->RemoveWindow(g_loadsaveWindow->Id());
        }
	    keypress_RemoveHandler(g_loadsaveWindow);

        allocated::clear(g_loadsaveWindow);
    }
}

void loadsavescreen_PostCleanupAction()
{
	c3ui_Get()->AddAction(new LSCleanupAction);
}

void LSCleanupAction::Execute(aui_Control *control, uint32 action, uint32 data)
{
	loadsavescreen_Cleanup();
}




static MBCHAR s_tempPath[_MAX_PATH];
static SaveInfo	*s_tempSaveInfo = nullptr;





void loadsavescreen_HotseatCallback(sint32 launch, sint32 player,
									 sint32 civ, BOOL human,
									 MBCHAR *name, MBCHAR *email)
{
	if(launch) {
		civapp_Get()->PostLoadSaveGameAction(s_tempPath);





















	} else {

		hs_player_setup_buf()[player].civ = civ;
		hs_player_setup_buf()[player].isHuman = human;
		hs_player_setup_buf()[player].name = name;
		hs_player_setup_buf()[player].email = email;
	}
}





void loadsavescreen_SetupHotseatOrEmail()
{
	hs_player_setup_Clear();

	hotseatlist_DisplayWindow(
		(HotseatListCallback *)loadsavescreen_HotseatCallback);
}




void loadsavescreen_DifficultyScreenActionCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	if (spnewgamediffscreen_removeMyWindow(action)) {
		sint32 diff = spnewgamediffscreen_getDifficulty1();
		sint32 risk = spnewgamediffscreen_getDifficulty2();

		g_setDifficultyUponLaunch = TRUE;
		g_difficultyToSetUponLaunch = diff;

		g_setBarbarianRiskUponLaunch = TRUE;
		g_barbarianRiskUponLaunch = risk;

		civapp_Get()->PostLoadSaveGameAction(s_tempPath);
	}
}




void loadsavescreen_TribeScreenActionCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	MBCHAR tempNameStr[k_MAX_NAME_LEN];

	spnewgametribescreen_removeMyWindow(action, tempNameStr );

	sint32	tribeIndex = spnewgametribescreen_getTribeIndex();

	if(tribeIndex < 0 || tribeIndex >= g_theCivilisationDB->NumRecords())
	{
		tribeIndex = 1;
	}

	profiledb_Get()->SetCivIndex(tribeIndex);

	if(s_tempSaveInfo
	&& s_tempSaveInfo->startInfoType == STARTINFOTYPE_NOLOCS
	){
		bool noCivsInList = true;
		for(int i : s_tempSaveInfo->playerCivIndexList)
		{
			if(i > 0)
			{
				noCivsInList = false;
				break;
			}
		}

		if(noCivsInList)
		{
			bool foundOne = false;

			for(sint32 i = 0; i < k_MAX_PLAYERS; i++)
			{
				MBCHAR		*civName;
				MBCHAR		*dbString;

				civName = s_tempSaveInfo->civList[i];
				dbString = (MBCHAR *)stringdb_Get()->GetNameStr(g_theCivilisationDB->Get(tribeIndex)->GetPluralCivName());
				if(strlen(civName) > 0)
				{
					if(!stricmp(dbString, civName))
					{
						g_scenarioUsePlayerNumber = i;
						foundOne = true;
						break;
					}
				}
			}

			if (!foundOne)
			{
				g_scenarioUsePlayerNumber = 1;
			}
		}
		else
		{
			for (sint32 i=0; i<k_MAX_PLAYERS; i++)
			{
				if (s_tempSaveInfo->playerCivIndexList[i] == tribeIndex)
				{
					g_scenarioUsePlayerNumber = i;
					break;
				}
			}
		}

		allocated::clear(s_tempSaveInfo);
	}

	spnewgamediffscreen_Initialize(loadsavescreen_DifficultyScreenActionCallback);
	spnewgamediffscreen_displayMyWindow();
}




void loadsavescreen_PlayersScreenActionCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	spnewgameplayersscreen_removeMyWindow(AUI_BUTTON_ACTION_EXECUTE);

	scenario_civs_Set(profiledb_Get()->GetNPlayers()-1);

	if (s_tempSaveInfo->startInfoType != STARTINFOTYPE_NOLOCS)
	{
		if (scenario_civs_Get() > s_tempSaveInfo->numPositions)
		{
			scenario_civs_Set(s_tempSaveInfo->numPositions);

			profiledb_Get()->SetNPlayers(scenario_civs_Get()+1);
		}
	}

	if (s_tempSaveInfo->startInfoType == STARTINFOTYPE_CIVSFIXED)
	{
		if (gameinit_IsEmailGame() || gameinit_IsHotseatGame())
		{
			hotseatlist_ClearOptions();
			for (sint32 i=0; i<profiledb_Get()->GetNPlayers(); i++)
			{
				sint32 civIndex = s_tempSaveInfo->playerCivIndexList[i];
				hotseatlist_SetPlayerCiv(i, civIndex);
			}
			hotseatlist_LockCivs();
			loadsavescreen_SetupHotseatOrEmail();
		}
		else
		{
			spnewgamediffscreen_Initialize(loadsavescreen_DifficultyScreenActionCallback);
			spnewgamediffscreen_displayMyWindow();
		}
	}
	else
	{
		if (gameinit_IsEmailGame() || gameinit_IsHotseatGame())
		{
			if (s_tempSaveInfo->startInfoType == STARTINFOTYPE_POSITIONSFIXED)
			{
				hotseatlist_ClearOptions();
				hotseatlist_EnableAllCivs();
				loadsavescreen_SetupHotseatOrEmail();
			}
			else
			{
				if (s_tempSaveInfo->startInfoType == STARTINFOTYPE_NOLOCS)
				{
					hotseatlist_ClearOptions();
					hotseatlist_LockCivs();

					for (int i : s_tempSaveInfo->playerCivIndexList)
					{
						hotseatlist_EnableCiv(i);
					}
					loadsavescreen_SetupHotseatOrEmail();
				}
				else
				{
					hotseatlist_ClearOptions();
					hotseatlist_DisableAllCivs();
					for (sint32 i=0; i<s_tempSaveInfo->numPositions; i++)
					{
						hotseatlist_EnableCiv(s_tempSaveInfo->positions[i].civIndex);
					}
					loadsavescreen_SetupHotseatOrEmail();
				}
			}
		}
		else
		{
			spnewgametribescreen_Cleanup();
			spnewgametribescreen_Initialize( loadsavescreen_TribeScreenActionCallback );

			if (s_tempSaveInfo->startInfoType == STARTINFOTYPE_POSITIONSFIXED)
			{
				spnewgametribescreen_setTribeIndex(1 + rand() % (g_theCivilisationDB->NumRecords() - 1));
			}
			else
			{
				spnewgametribescreen_clearTribes();

				if (s_tempSaveInfo->startInfoType == STARTINFOTYPE_NOLOCS)
				{

					if (save_file_version_Get() >= 50 && s_tempSaveInfo->startingPlayer != -1)
					{
						// Only one player is enabled
						spnewgametribescreen_addTribe(s_tempSaveInfo->playerCivIndexList[s_tempSaveInfo->startingPlayer]);
					}
					else
					{
						s_tempSaveInfo->startingPlayer = -1;
						s_tempSaveInfo->showLabels=FALSE;

						BOOL noCivsInList = TRUE;
						sint32 i;
						for (i=0; i<k_MAX_PLAYERS; i++) {
							if (s_tempSaveInfo->playerCivIndexList[i] > 0) {
								noCivsInList = FALSE;
								break;
							}
						}

						if (noCivsInList)
						{
							BOOL foundOne = FALSE;

							for (auto civName : s_tempSaveInfo->civList)
							{
									if (strlen(civName) > 0)
								{
									for (sint32 j=0; j<g_theCivilisationDB->NumRecords(); j++)
									{
										MBCHAR const *  dbString =
										    stringdb_Get()->GetNameStr(g_theCivilisationDB->Get(j)->GetPluralCivName());

										if (!stricmp(dbString, civName))
										{
											spnewgametribescreen_addTribeNoDuplicate(j);
											foundOne = TRUE;
											break;
										}
									}
								}
							}

							if (!foundOne)
							{
								spnewgametribescreen_Cleanup();

								spnewgamediffscreen_Initialize(loadsavescreen_DifficultyScreenActionCallback);
								spnewgamediffscreen_displayMyWindow();

								return;
							}
						}
						else
						{
							for (int i : s_tempSaveInfo->playerCivIndexList)
							{
								spnewgametribescreen_addTribeNoDuplicate(i);
							}
						}
					}

					spnewgametribescreen_displayMyWindow(nullptr, TRUE);

					return;
				}
				else
				{
					for ( sint32 i = 0; i < s_tempSaveInfo->numPositions; i++ )
					{
						spnewgametribescreen_addTribeNoDuplicate(s_tempSaveInfo->positions[i].civIndex);
					}

					sint32 index = rand() % s_tempSaveInfo->numPositions;
					spnewgametribescreen_setTribeIndex(s_tempSaveInfo->positions[index].civIndex);
				}
			}

			spnewgametribescreen_displayMyWindow(nullptr, TRUE);
		}
	}

	allocated::clear(s_tempSaveInfo);
}

void loadsavescreen_BeginLoadProcess(SaveInfo *saveInfo, MBCHAR *directoryPath)
{
	MBCHAR		path[_MAX_PATH];
	snprintf(path, sizeof(path), "%s%s%s", directoryPath, FILE_SEP, saveInfo->fileName);

	if (saveInfo->startInfoType != STARTINFOTYPE_NONE) {







		if (!saveInfo->scenarioName.empty()) {

			Scenario		*scen;
			ScenarioPack	*pack;

			if (civscenarios_Get()->FindScenario(const_cast<MBCHAR*>(saveInfo->scenarioName.c_str()), &pack, &scen)) {

				civpaths_Get()->SetCurScenarioPath(scen->m_path);
				civpaths_Get()->SetCurScenarioPackPath(pack->m_path);
				profiledb_Get()->SetIsScenario(TRUE);

				// TODO(strlcpy): unknown dst size
				strcpy(scenario_name_buf(), saveInfo->scenarioName.c_str());
			} else {

				MBCHAR tempStr[_MAX_PATH];
				snprintf(tempStr, sizeof(tempStr), "%s%s", stringdb_Get()->GetNameStr("str_ERR_CANT_LOCATE_SCEN"), saveInfo->scenarioName.c_str());

				MessageBoxDialog::Information(tempStr,"CantLoadScenario",nullptr, nullptr, "str_ldl_MB_OK", false);
				return;
			}
		} else {

			civpaths_Get()->SetCurScenarioPath(directoryPath);
			profiledb_Get()->SetIsScenario(TRUE);

			is_scenario_Set(true);
		}
	} else {

		BOOL wasScenario = !saveInfo->scenarioName.empty();








		civpaths_Get()->ClearCurScenarioPath();
		profiledb_Get()->SetIsScenario(FALSE);

		if (wasScenario) {

			Scenario *scen;
			ScenarioPack *pack;
			if(civscenarios_Get()->FindScenario(const_cast<MBCHAR*>(saveInfo->scenarioName.c_str()), &pack, &scen)) {
				civpaths_Get()->SetCurScenarioPath(scen->m_path);
				civpaths_Get()->SetCurScenarioPackPath(pack->m_path);
			} else {

				MBCHAR tempStr[_MAX_PATH];
				snprintf(tempStr, sizeof(tempStr), "%s%s", stringdb_Get()->GetNameStr("str_ERR_CANT_LOCATE_SCEN"), saveInfo->scenarioName.c_str());

				MessageBoxDialog::Information(tempStr,"CantLoadScenarioData",nullptr, nullptr, "str_ldl_MB_OK", false);
				return;
			}

			civapp_Get()->CleanupAppDB();
			civapp_Get()->InitializeAppDB();
		}
	}

	if (saveInfo->isScenario &&
		(saveInfo->startInfoType == STARTINFOTYPE_CIVS ||
		 saveInfo->startInfoType == STARTINFOTYPE_POSITIONSFIXED ||
		 saveInfo->startInfoType == STARTINFOTYPE_CIVSFIXED ||
		 saveInfo->startInfoType == STARTINFOTYPE_NOLOCS)) {






		strlcpy(s_tempPath, path, sizeof(s_tempPath));
		allocated::reassign(s_tempSaveInfo, new SaveInfo(saveInfo));

		if (s_tempSaveInfo->numPositions <= 3 || saveInfo->startInfoType == STARTINFOTYPE_NOLOCS)
		{
			loadsavescreen_PlayersScreenActionCallback(nullptr, AUI_BUTTON_ACTION_EXECUTE, 0, nullptr);
		}
		else
		{
			spnewgameplayersscreen_Cleanup();
			spnewgameplayersscreen_Initialize( loadsavescreen_PlayersScreenActionCallback );
			spnewgameplayersscreen_SetMaxPlayers(s_tempSaveInfo->numPositions);
			spnewgameplayersscreen_displayMyWindow();
		}
	} else {

		civapp_Get()->PostLoadSaveGameAction(path);
	}
}


void loadsavescreen_LoadGame()
{
	GameInfo *  gameInfo = g_loadsaveWindow->GetGameInfo();

	Assert(gameInfo);
	if (!gameInfo) return;
	if (!c3files_HasLegalCD()) return;

	if (SaveInfo * saveInfo = g_loadsaveWindow->GetSaveInfo())
	{
		loadsavescreen_BeginLoadProcess(saveInfo, gameInfo->path);
	}
	else
	{
		loadsavescreen_displayMyWindow(0);
	}
}

void loadsavescreen_SaveGame(MBCHAR *usePath, MBCHAR *useName)
{

	SaveInfo		*saveInfo = g_loadsaveWindow->GetSaveInfoToSave();

	Assert( saveInfo != nullptr );
	if ( !saveInfo ) return;

	if (!g_loadsaveWindow->GetGameName(saveInfo->gameName)) return;
	if (!g_loadsaveWindow->GetSaveName(saveInfo->fileName)) return;
	if (!g_loadsaveWindow->GetNote(saveInfo->note)) return;

	saveInfo->isScenario = is_scenario_Get();

	if (strlen(saveInfo->gameName) == 0) {

		if (g_loadsaveWindow->GetGameInfo() != nullptr) {
			strlcpy(saveInfo->gameName, g_loadsaveWindow->GetGameInfo()->name, sizeof(saveInfo->gameName));
		} else {
			g_loadsaveWindow->BuildDefaultSaveName(nullptr, saveInfo->gameName);
			saveInfo->gameName[SAVE_LEADER_NAME_SIZE] = '\0';
		}
	}

	nf_GameSetup gs;

	sint32 j = 0;
	uint32 i;
	for(i = 0; i < k_MAX_PLAYERS; i++)
	{
		if ( player_Get(i) )
		{
			TribeSlot ts;

			ts.key = 0;

#if you_want_ai_civs_from_singleplayer_saved_game_showing_up_in_netshell
			ts.isAI = player_Get(i)->IsRobot();
#else
			ts.isAI = 0;
#endif


			ts.tribe = player_Get(i)->m_civilisation->GetCivilisation() + 1;

			ts.isFemale = (player_Get(i)->m_civilisation->GetGender() == GENDER_FEMALE);


			if ( 1 < ts.tribe )
			{

				if ( j >= k_NS_MAX_PLAYERS ) break;

				gs.GetSavedTribeSlots()[ j++ ] = ts;
			}
		}
	}
	saveInfo->gameSetup = gs;

	MBCHAR	path[_MAX_PATH];
	MBCHAR	fullPath[_MAX_PATH];

	if(usePath) {
		strlcpy(path, usePath, sizeof(path));
	} else {
		if (!civpaths_Get()->GetSavePath(C3SAVEDIR_GAME, path)) return;
	}

	if(!useName) {

		char *testchars="\\*\"/:|?<>";
		bool charschanged=false;
		for(i=0; i<strlen(saveInfo->gameName); i++)
		{
			if(strchr(testchars,saveInfo->gameName[i]))
			{
				saveInfo->gameName[i]='#';
				charschanged=true;
			}
		}
		if(charschanged)
		{
			MessageBoxDialog::Information("str_ldl_InvalidCharsFixed", "InfoInvalidCharsFixed");
		}

		snprintf(fullPath, sizeof(fullPath), "%s%s%s", path, FILE_SEP, saveInfo->gameName);

		// Verify that this directory exists, and if it doesn't, create it
		if (!c3files_PathIsValid(fullPath)) {
			if (!c3files_CreateDirectory(fullPath)) {
				Assert(FALSE);
				// FIXME
				// unable to create the directory for the game, should report
				// to the user, here
				return;
			}
		}

		// Check for invalid characters in filename.
		bool charschanged2=false;
		for(i=0; i<strlen(saveInfo->fileName); i++)
		{
			if(strchr(testchars,saveInfo->fileName[i]))
			{
				saveInfo->fileName[i]='#';
				charschanged2=true;
			}
		}
		if(charschanged2 && !charschanged)
		{
			MessageBoxDialog::Information("str_ldl_InvalidCharsFixed", "InfoInvalidCharsFixed");
		}

		// Full path, including the save file's filename
		snprintf(saveInfo->pathName, sizeof(saveInfo->pathName), "%s%s%s", fullPath, FILE_SEP, saveInfo->fileName);
	} else {
		snprintf(fullPath, sizeof(fullPath), "%s", path);
		strlcpy(saveInfo->fileName, useName, sizeof(saveInfo->fileName));
		snprintf(saveInfo->pathName, sizeof(saveInfo->pathName), "%s%s%s", fullPath, FILE_SEP, useName);
	}

	// Build a power graph from the UI
	g_loadsaveWindow->GetPowerGraph(saveInfo);
	g_loadsaveWindow->GetRadarMap(saveInfo);

	// SAM021899 changed to make a save request
	allocated::reassign(g_savedGameRequest, new SaveInfo(saveInfo));

//	GameFile::SaveGame(saveInfo->pathName, saveInfo);
}

/////////////////////////////////////////////////////////////

void loadsavescreen_LoadMPGame()
{
	if(!g_loadsaveWindow) {
		Assert(netfunc_Get() && !netfunc_Get()->IsHost());
		// HACK FIXME I still don't understand what's going on, but now
		// this happens on the client instead.
		return;
	}

	// SAM042099 check for a valid CD-ROM before allowing a game to be loaded
	if ((!netfunc_Get() || NETFunc::IsHost()) && !c3files_HasLegalCD())
		return;

	GameInfo *  gameInfo = g_loadsaveWindow->GetGameInfo();

	// HACK FIXME this happens on the client, which probably shouldn't call
	// this function at all, but it wasn't immediately obvious how to tell
	// that this is a client where this is being called.
	//Assert(gameInfo);
	// EAS02161999 - Must also check to see if you're not the host.
	// 'Cause in single player mode, somebody might've already created
	// a ligitimate *single*player gameInfo that's still lying around.
	if (!gameInfo || (netfunc_Get() && !NETFunc::IsHost())) {
		civapp_Get()->PostStartGameAction();
		return;
	}

	SaveInfo *  saveInfo = g_loadsaveWindow->GetSaveInfo();
	Assert(saveInfo);
	if (!saveInfo) return;

	MBCHAR		path[_MAX_PATH];

	snprintf(path, sizeof(path), "%s%s%s", gameInfo->path, FILE_SEP, saveInfo->fileName);
	civapp_Get()->PostLoadSaveGameAction(path);
}

/////////////////////////////////////////////////////////////

void loadsavescreen_SaveMPGame()
{
	SaveInfo		*saveInfo = g_loadsaveWindow->GetSaveInfoToSave();

	Assert( saveInfo != nullptr );
	if ( !saveInfo ) return;

	if (!g_loadsaveWindow->GetGameName(saveInfo->gameName)) return;
	if (!g_loadsaveWindow->GetSaveName(saveInfo->fileName)) return;
	if (!g_loadsaveWindow->GetNote(saveInfo->note)) return;

	// SAM050399
	saveInfo->isScenario = is_scenario_Get();

	if (strlen(saveInfo->gameName) == 0) {
		// Empty game name in the save info, copy it over from the game info
		if (g_loadsaveWindow->GetGameInfo() != nullptr) {
			strlcpy(saveInfo->gameName, g_loadsaveWindow->GetGameInfo()->name, sizeof(saveInfo->gameName));
		} else {
			g_loadsaveWindow->BuildDefaultSaveName(nullptr, saveInfo->gameName);
			saveInfo->gameName[SAVE_LEADER_NAME_SIZE] = '\0';
		}
	}

	saveInfo->gameSetup = gamesetup_Get();

	memcpy(
		saveInfo->gameSetup.GetSavedTribeSlots(),
		saveInfo->gameSetup.GetTribeSlots(),
		k_NS_MAX_PLAYERS * sizeof( TribeSlot ) );
	memset(
		saveInfo->gameSetup.GetTribeSlots(),
		0,
		k_NS_MAX_PLAYERS * sizeof( TribeSlot ) );

	MBCHAR	path[_MAX_PATH];
	if (!civpaths_Get()->GetSavePath(C3SAVEDIR_MP, path)) return;

	MBCHAR	fullPath[_MAX_PATH];
	snprintf(fullPath, sizeof(fullPath), "%s%s%s", path, FILE_SEP, saveInfo->gameName);

	// Verify that this directory exists, and if it doesn't, create it
	if (!c3files_PathIsValid(fullPath)) {
		if (!c3files_CreateDirectory(fullPath)) {
			Assert(FALSE);
			// FIXME
			// unable to create the directory for the game, should report
			// to the user, here
			return;
		}
	}

	// Full path, including the save file's filename
	snprintf(saveInfo->pathName, sizeof(saveInfo->pathName), "%s%s%s", fullPath, FILE_SEP, saveInfo->fileName);

	// Build a power graph from the UI
	g_loadsaveWindow->GetPowerGraph(saveInfo);
	g_loadsaveWindow->GetRadarMap(saveInfo);

	// SAM021899 changed to make a save request
	allocated::reassign(g_savedGameRequest, new SaveInfo(saveInfo));

//	GameFile::SaveGame(saveInfo->pathName, saveInfo);

}

/////////////////////////////////////////////////////////////

void loadsavescreen_LoadSCENGame()
{
	// SAM042099 check for a valid CD-ROM before allowing a game to be loaded
	if (!c3files_HasLegalCD()) return;

	GameInfo	*gameInfo = g_loadsaveWindow->GetGameInfo();

	Assert(gameInfo);
	if (!gameInfo) return;

	SaveInfo	*saveInfo = g_loadsaveWindow->GetSaveInfo();
	Assert(saveInfo);
	if (!saveInfo) return;

	MBCHAR		path[_MAX_PATH];

	snprintf(path, sizeof(path), "%s%s%s", gameInfo->path, FILE_SEP, saveInfo->fileName);
//	civapp_Get()->PostLoadSaveGameAction(path);

	civpaths_Get()->SetCurScenarioPath(gameInfo->path);

	profiledb_Get()->SetIsScenario(TRUE);

	civapp_Get()->PostLoadScenarioGameAction(saveInfo->fileName);
}

/////////////////////////////////////////////////////////////

void loadsavescreen_SaveSCENGame()
{
	SaveInfo		*saveInfo = g_loadsaveWindow->GetSaveInfoToSave();

	Assert( saveInfo != nullptr );
	if ( !saveInfo ) return;

	if (!g_loadsaveWindow->GetGameName(saveInfo->gameName)) return;
	if (!g_loadsaveWindow->GetSaveName(saveInfo->fileName)) return;
	if (!g_loadsaveWindow->GetNote(saveInfo->note)) return;

	if (strlen(saveInfo->gameName) == 0) {
		// Empty game name in the save info, copy it over from the game info
		if (g_loadsaveWindow->GetGameInfo() != nullptr) {
			strlcpy(saveInfo->gameName, g_loadsaveWindow->GetGameInfo()->name, sizeof(saveInfo->gameName));
		} else {
			g_loadsaveWindow->BuildDefaultSaveName(nullptr, saveInfo->gameName);
			saveInfo->gameName[SAVE_LEADER_NAME_SIZE] = '\0';
		}
	}

	// Create a default gamesetup.
	nf_GameSetup gs;

	// Must save which tribes were used.
	sint32 j = 0;
	for ( sint32 i = 0; i < k_NS_MAX_PLAYERS; i++ )
	{
		if ( player_Get(i) )
		{
			TribeSlot ts;

			// These fields don't matter for the savedtribeslots.
			ts.isAI = 0;
			ts.key  = 0;

			// This is the only important field.
			// +1 because netshell treats zero as "none" w/ barbarians == 1.
			ts.tribe = player_Get(i)->m_civilisation->GetCivilisation() + 1;

			ts.isFemale = player_Get(i)->m_civilisation->GetGender() == GENDER_FEMALE;

			// We don't want to store the barbarians.  The netshell skips them.
			if ( 1 < ts.tribe )
				gs.GetSavedTribeSlots()[ j++ ] = ts;
		}
	}

	saveInfo->gameSetup = gs;

	MBCHAR	path[_MAX_PATH];

	if (!civpaths_Get()->GetSavePath(C3SAVEDIR_SCEN, path)) return;

	MBCHAR	fullPath[_MAX_PATH];
	snprintf(fullPath, sizeof(fullPath), "%s%s%s", path, FILE_SEP, saveInfo->gameName);

	// Verify that this directory exists, and if it doesn't, create it
	if (!c3files_PathIsValid(fullPath)) {
		if (!c3files_CreateDirectory(fullPath)) {
			Assert(FALSE);
			// FIXME
			// unable to create the directory for the game, should report
			// to the user, here
			return;
		}
	}

	// Full path, including the save file's filename
	snprintf(saveInfo->pathName, sizeof(saveInfo->pathName), "%s%s%s", fullPath, FILE_SEP, saveInfo->fileName);

	// Build a power graph from the UI
	g_loadsaveWindow->GetPowerGraph(saveInfo);
	g_loadsaveWindow->GetRadarMap(saveInfo);

	// SAM021899 changed to make a save request
	allocated::reassign(g_savedGameRequest, new SaveInfo(saveInfo));

//	GameFile::SaveGame(saveInfo->pathName, saveInfo);
}

BOOL loadsavescreen_CheckOverwrite( );

/////////////////////////////////////////////////////////////
// CallBacks
/////////////////////////////////////////////////////////////

void loadsavescreen_executePress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	// calling function should do what ever necessary to continue game state
	// this is all this callback should do

	// need to check for file overwrite before closing the window if we're saving

	if ( action == (uint32)AUI_BUTTON_ACTION_EXECUTE ) {
		switch ( g_loadsaveWindow->GetType() ) {
		case LSS_SAVE_GAME:
		case LSS_SAVE_MP:
			if ( g_loadsaveWindow->NoName() ) return;
			if ( loadsavescreen_CheckOverwrite() ) return;
			break;
		}
	}

	// remove window and do operation
	if(loadsavescreen_removeMyWindow(action)) {
		switch(g_loadsaveWindow->GetType()) {
			case LSS_LOAD_GAME:		loadsavescreen_LoadGame();			break;
			case LSS_SAVE_GAME:		loadsavescreen_SaveGame();	break;

// EAS100698 - we don't want to immediately load when picking a saved game from w/i netshell.
			case LSS_LOAD_MP:
				/*loadsavescreen_LoadMPGame();*/
				break;
			case LSS_SAVE_MP:		loadsavescreen_SaveMPGame();		break;

			case LSS_LOAD_SCEN:		loadsavescreen_LoadSCENGame();		break;
			case LSS_LOAD_SCEN_MP:	break;	// in this case, do nothing here, LoadSCENGame() is called elsewhere
			case LSS_SAVE_SCEN:		loadsavescreen_SaveSCENGame();		break;
			default:
				Assert(0);
				break;
		}
		loadsavescreen_PostCleanupAction();
	}
}

/////////////////////////////////////////////////////////////

void loadsavescreen_backPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	// FIXED EMPTY CONTROLLED STATEMENT
	// ILD. 6/2000.
//	if(loadsavescreen_removeMyWindow(action)) ;
	loadsavescreen_removeMyWindow(action) ;

	loadsavescreen_PostCleanupAction();
}

void loadsavescreen_delete( )
{
	GameInfo	*gameInfo = g_loadsaveWindow->GetGameInfo();

	Assert(gameInfo);
	if (!gameInfo) return;

//	Assert(saveInfo);
//	if (!saveInfo) return;

	SaveInfo	*saveInfo = g_loadsaveWindow->GetSaveInfo();
	if(saveInfo)
	{
		MBCHAR		path[_MAX_PATH];

		snprintf(path, sizeof(path), "%s%s%s", gameInfo->path, FILE_SEP, saveInfo->fileName);

#ifdef WIN32
		if ( DeleteFile( path ) )
#elif defined(HAVE_UNISTD_H)
		if ( !unlink( path ) )
#endif
		{
			// FIXME ? Do we want to worry about deleting .gw files?

			// Refill list two.
			sint32 one = g_loadsaveWindow->GetListOne()->GetSelectedItemIndex();
			sint32 two = g_loadsaveWindow->GetListTwo()->GetSelectedItemIndex();

			g_loadsaveWindow->SetType( g_loadsaveWindow->GetType() );

			g_loadsaveWindow->GetListOne()->SelectItem( one );

			if ( two && two == g_loadsaveWindow->GetListTwo()->NumItems() ) --two;
			g_loadsaveWindow->GetListTwo()->SelectItem( two );

	// This wasn't working.
	//  		aui_Item *item = g_loadsaveWindow->GetListTwo()->GetSelectedItem();
	//  		if ( item )
	//  		{
	//  			g_loadsaveWindow->GetListTwo()->RemoveItem( item->Id() );
	//  			delete item;
	//  		}
		}
		else
		{
			Assert( "Couldn't delete file." == nullptr );
		}
	}
	else
	{
#ifdef WIN32
		MBCHAR		path[_MAX_PATH];
		snprintf(path, sizeof(path), "%s%s*.*", gameInfo->path, FILE_SEP);

		_finddata_t findData;
		int fileHandle=_findfirst(path,&findData);
		while(fileHandle)
		{
			snprintf(path, sizeof(path), "%s%s%s", gameInfo->path, FILE_SEP, findData.name);
			DeleteFile(path);
			if(_findnext(fileHandle,&findData))
			{
				_findclose(fileHandle);
				fileHandle=0;
			}
		}
		snprintf(path, sizeof(path), "%s", gameInfo->path);
		int retval=_rmdir(path);
		assert(!retval);
#else
		// Delete all files in the directory, then the directory itself.
		DIR *dir = opendir(gameInfo->path);
		if (dir) {
			struct dirent *entry;
			while ((entry = readdir(dir))) {
				if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
					continue;
				char fpath[_MAX_PATH];
				snprintf(fpath, sizeof(fpath), "%s%s%s", gameInfo->path, FILE_SEP, entry->d_name);
				struct stat st;
				if (stat(fpath, &st) == 0 && !S_ISDIR(st.st_mode)) {
					unlink(fpath);
				}
			}
			closedir(dir);
		}
		rmdir(gameInfo->path);
#endif // WIN32
		g_loadsaveWindow->FillListTwo(NULL);
		g_loadsaveWindow->SetType(g_loadsaveWindow->GetType());
	}
}

void loadsavescreen_deletePress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	if (g_loadsaveWindow->GetGameInfo())
    {
		MessageBoxDialog::Query(stringdb_Get()->GetNameStr("DELETE_SAVE_CONFIRM"),
                                "ConfirmLoadSaveDelete",
                                loadsavescreen_deleteDialog
                               );
	}
}

/////////////////////////////////////////////////////////////

void loadsavescreen_ListOneHandler(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	// We only care about select actions
	if ( action != (uint32)AUI_LISTBOX_ACTION_SELECT ) return;

	c3_ListBox	*list = (c3_ListBox *)control;
	if (list == nullptr) return;

	// check if there is a selected item in the list box
	LSGamesListItem *item = (LSGamesListItem *)list->GetSelectedItem();
	if (item == nullptr)
	{
		g_loadsaveWindow->SetGameInfo(nullptr);

		// Force the default names to come back.
		g_loadsaveWindow->SetType( g_loadsaveWindow->GetType() );

		g_loadsaveWindow->GetDeleteButton()->Enable( FALSE );
	}
	else
	{
		// Set the current game info
		g_loadsaveWindow->SetGameInfo(item->GetGameInfo());

		g_loadsaveWindow->GetDeleteButton()->Enable( TRUE );
	}

	if ( !g_loadsaveWindow->GetListTwo()->GetSelectedItem() )
	{
		switch ( g_loadsaveWindow->GetType() )
		{
		case LSS_LOAD_GAME:
		case LSS_LOAD_MP:
		case LSS_LOAD_SCEN:
		case LSS_LOAD_SCEN_MP:
			g_loadsaveWindow->GetOkButton()->Enable( FALSE );
			break;

		default:
			g_loadsaveWindow->GetOkButton()->Enable( TRUE );
			break;
		}

//		g_loadsaveWindow->GetDeleteButton()->Enable( FALSE );
	}
}

/////////////////////////////////////////////////////////////

void loadsavescreen_ListTwoHandler(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	// We only care about select actions
	if ( action != (uint32)AUI_LISTBOX_ACTION_SELECT ) return;

	c3_ListBox	*list = (c3_ListBox *)control;
	if (list == nullptr) return;

	// If another list item was previously selected, make sure to dump its
	// extended info
	tech_WLList<sint32> *lastList = list->GetSelectedListLastTime();
	for (uint32 i=0; i<lastList->L(); i++) {
		sint32 index = lastList->GetAtIndex(i);
		LSSavesListItem *oldItem = (LSSavesListItem *)list->GetItemByIndex(index);
		if (oldItem != nullptr) {
			SaveInfo *oldSaveInfo = oldItem->GetSaveInfo();
			if (oldSaveInfo) {
				// Dump power graph
				oldSaveInfo->powerGraphWidth = 0;
				oldSaveInfo->powerGraphHeight = 0;
				if (oldSaveInfo->powerGraphData) {
					delete[] oldSaveInfo->powerGraphData;
					oldSaveInfo->powerGraphData = nullptr;
				}

				// Dump radar map
				oldSaveInfo->radarMapWidth = 0;
				oldSaveInfo->radarMapHeight = 0;
				if (oldSaveInfo->radarMapData) {
					delete[] oldSaveInfo->radarMapData;
					oldSaveInfo->radarMapData = nullptr;
				}

				// Set load type back to basic
				oldSaveInfo->loadType = SAVEINFOLOAD_BASIC;
			}
		}
	}

	// check if there is a selected item in the list box
	LSSavesListItem *item = (LSSavesListItem *)list->GetSelectedItem();
	if (item == nullptr)
	{
		switch ( g_loadsaveWindow->GetType() )
		{
		case LSS_LOAD_GAME:
		case LSS_LOAD_MP:
		case LSS_LOAD_SCEN:
		case LSS_LOAD_SCEN_MP:
			g_loadsaveWindow->GetOkButton()->Enable( FALSE );
			break;

		default:
			g_loadsaveWindow->GetOkButton()->Enable( TRUE );
			break;
		}

		g_loadsaveWindow->SetSaveInfo(nullptr);

		// Force the default names to come back.
		g_loadsaveWindow->SetType( g_loadsaveWindow->GetType() );

		g_loadsaveWindow->GetDeleteButton()->Enable( FALSE );
	}
	else
	{
		SaveInfo	*info = item->GetSaveInfo();
		if (info == nullptr) return;

		// Set the current save info
		if (info->loadType == SAVEINFOLOAD_BASIC)
			GameFile::FetchExtendedSaveInfo(info->pathName, info);

		g_loadsaveWindow->SetSaveInfo(info);
		g_loadsaveWindow->GetOkButton()->Enable( TRUE );
		g_loadsaveWindow->GetDeleteButton()->Enable( TRUE );
	}
}

/////////////////////////////////////////////////////////////

void loadsavescreen_CivListHandler(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_LISTBOX_ACTION_SELECT )
		return;
}

/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////
void loadsavescreen_OverwriteCallback( bool response, void *data )
{
	if ( response ) {

		switch(g_loadsaveWindow->GetType()) {
			case LSS_SAVE_GAME:		loadsavescreen_SaveGame();			break;
			case LSS_SAVE_MP:		loadsavescreen_SaveMPGame();		break;
		}

		// FIXED EMPTY CONTROLLED STATEMENT
		// ILD. 6/2000.
	//	if(loadsavescreen_removeMyWindow((uint32)AUI_BUTTON_ACTION_EXECUTE)) ;
		loadsavescreen_removeMyWindow((uint32)AUI_BUTTON_ACTION_EXECUTE) ;

		loadsavescreen_PostCleanupAction();
	}
}

/////////////////////////////////////////////////////////////

BOOL loadsavescreen_CheckOverwrite( )
{
	// EAS012199 - save the one we originally set out to save.
	SaveInfo *  saveInfo = g_loadsaveWindow->GetSaveInfoToSave();

	Assert( saveInfo != nullptr );
	if ( !saveInfo ) return FALSE;

	if (!g_loadsaveWindow->GetGameName(saveInfo->gameName)) return FALSE;
	if (!g_loadsaveWindow->GetSaveName(saveInfo->fileName)) return FALSE;
	if (!g_loadsaveWindow->GetNote(saveInfo->note)) return FALSE;

	GameInfo *  gameInfo = g_loadsaveWindow->GetGameInfo();
	if (gameInfo)
    {
		for
        (
            PointerList<SaveInfo>::Walker walker = PointerList<SaveInfo>::Walker(gameInfo->files);
            walker.IsValid();
            walker.Next()
        )
        {
			SaveInfo * info = walker.GetObj();
			if ( !strcmp(info->fileName, saveInfo->fileName) )
            {
				//c3_TextMessage( s, k_UTILITY_TEXTMESSAGE_YESNO, loadsavescreen_OverwriteCallback );
				MessageBoxDialog::Query(stringdb_Get()->GetNameStr("SAVE_OVERWRITE"),
                                        "ConfirmSaveOverwrite",
                                        loadsavescreen_OverwriteCallback
                                       );
				return TRUE;
			}
		}
	}

	return FALSE;
}
