//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Main application
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
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
//----------------------------------------------------------------------------
///
/// \file   civapp.h
/// \brief  Main application

#if defined(HAVE_PRAGMA_ONCE)
#pragma once
#endif

#ifndef CIVAPP_H__
#define CIVAPP_H__

//----------------------------------------------------------------------------
//
// Library imports
//
//----------------------------------------------------------------------------

#include <windows.h>            // HINSTANCE, MBCHAR

//----------------------------------------------------------------------------
//
// Exported names
//
//----------------------------------------------------------------------------

class CivApp;

//----------------------------------------------------------------------------
//
// Project imports
//
//----------------------------------------------------------------------------

#include <memory>                      // std::unique_ptr

#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_action.h"     // aui_Action
#include "os/include/ctp2_inttypes.h"  // sint32, uint32
namespace Ctp2 { class Game; }

//----------------------------------------------------------------------------
//
// Declarations
//
//----------------------------------------------------------------------------

class CivApp
{
public:
	CivApp();
	~CivApp();

	// Non-copyable, non-movable: there's only one CivApp per process,
	// and it owns large session state via Ctp2::Game.
	CivApp(const CivApp&)            = delete;
	CivApp& operator=(const CivApp&) = delete;

	// Access the per-session game container.  Returns nullptr before
	// InitializeGame and after CleanupGame.
	Ctp2::Game *       GetGame()       { return m_game.get(); }
	const Ctp2::Game * GetGame() const { return m_game.get(); }

	void		AutoSave(sint32 player, bool isQuickSave = false);
	void		BeginKeyboardScrolling(sint32 key);
	void		CleanupApp();
	void		CleanupAppDB();
	sint32		EndGame();

	sint32		GetKeyboardScrollingKey() const
    {
        return m_keyboardScrollingKey;
    };

    sint32		InitializeApp(HINSTANCE hInstance, int iCmdShow);
	sint32		InitializeEngine();
	bool		InitializeAppDB();
	// Used directly by headless_main; also called from InitializeGame()
	// when c3ui_Get() is null (i.e. headless save-load).
	sint32		InitializeGameHeadless();
	sint32		InitializeGame();

   	bool		IsGameLoaded() const
    {
        return m_gameLoaded;
    };

    // Forwards to ScenarioEditor::IsShown / IsGivingAdvances so that game
    // logic can ask "are we in scenario-editing mode?" without including
    // ui/interface/scenarioeditor.h.  Defined in civapp.cpp where the editor
    // include is already pulled in.  Headless build links the same source
    // but the editor predicates evaluate to false there because the editor
    // is never instantiated.
    bool		IsScenarioEditorShown() const;
    bool		IsScenarioEditorGivingAdvances() const;

	bool		IsInBackground() const
    {
        return m_inBackground;
    };

	bool		IsKeyboardScrolling() const
    {
        return m_isKeyboardScrolling;
    };

	sint32		LoadSavedGame(MBCHAR const * name);
	sint32		LoadSavedGameMap(MBCHAR const * name);
	sint32		LoadScenarioGame(MBCHAR const * name);
	void		PostEndGameAction();
	void        PostLoadQuickSaveAction(sint32 player);
	void		PostLoadSaveGameAction(MBCHAR const *);
	void		PostLoadScenarioGameAction(MBCHAR const * name);
	void		PostQuitToLobbyAction();
	void		PostQuitToSPShellAction();
	void		PostRestartGameAction();
	void		PostRestartGameSameMapAction();
	void		PostSpriteTestAction();
	void		PostStartGameAction();
	sint32		Process();
	void		ProcessGraphicsCallback();
	sint32		QuickInit(HINSTANCE hInstance, int iCmdShow);
	void		QuitGame();
	sint32		QuitToLobby();
	sint32		QuitToSPShell();
	sint32		RestartGame();
	sint32		RestartGameSameMap();
	bool		SaveDBInGameFile() const
    {
        return m_saveDBInGameFile;
    }
	void		SetInBackground(bool in = true)
    {
        m_inBackground = in;
    }
	sint32		StartSpriteEditor();
	sint32		StartGame();
	void		StopKeyboardScrolling(sint32 key);

private:
	void		CleanupAppUI();
	void		CleanupGame(bool keepScenInfo);
	void		CleanupGameUI();
	void 		InitializeAppUI();
	sint32  	InitializeGameUI();
	sint32		InitializeSpriteEditor();
	void		PostLoadSaveGameMapAction(MBCHAR const *);
	sint32      ProcessAI();
	sint32      ProcessNet(const uint32 target_milliseconds, uint32 &used_milliseconds);
	sint32		ProcessProfile();
	sint32      ProcessRobot(const uint32 target_milliseconds, uint32 &used_milliseconds);
	sint32      ProcessSLIC();
	sint32		ProcessUI(const uint32 target_milliseconds, uint32 &used_milliseconds);
	void		RestoreAutoSave(sint32 player);
	void        StartMessageSystem();

	bool		m_appLoaded;
	bool		m_dbLoaded;
	bool		m_gameLoaded;
	bool		m_saveDBInGameFile;
	bool		m_aiFinishedThisTurn;
	bool		m_inBackground;
	bool		m_isKeyboardScrolling;
	sint32		m_keyboardScrollingKey;

	// Session-state container.  Allocated by InitializeGame and reset
	// by CleanupGame.  Holds TurnCount today; subsystems migrate in
	// per the long-running globals refactor.
	std::unique_ptr<Ctp2::Game>  m_game;
};

AUI_ACTION_BASIC(EndGameAction);
AUI_ACTION_BASIC(QuitToSPShellAction);
AUI_ACTION_BASIC(QuitToLobbyAction);
AUI_ACTION_BASIC(RestartGameAction);
AUI_ACTION_BASIC(RestartGameSameMapAction);
AUI_ACTION_BASIC(StartGameAction);
AUI_ACTION_BASIC(SpriteTestAction);

class LoadSaveGameAction : public aui_Action
{
public:
	LoadSaveGameAction(MBCHAR const * name = nullptr)
    :   aui_Action  ()
    {
        if (name)
        {
            strncpy(m_filename, name, k_AUI_LDL_MAXBLOCK);
        }
        else
        {
            m_filename[0] = 0;
        }
    };

	void	Execute
	(
		aui_Control	*	control,
		uint32			action,
		uint32			data
	) override;

private:
	MBCHAR m_filename[k_AUI_LDL_MAXBLOCK + 1];
};

/*   // never used
class LoadSaveGameMapAction : public aui_Action
{
public:
	LoadSaveGameMapAction(MBCHAR const * name = NULL)
    :   aui_Action  ()
    {
        if (name)
        {
            strncpy(m_filename, name, k_AUI_LDL_MAXBLOCK);
        }
        else
        {
            m_filename[0] = 0;
        }
    };

	virtual void	Execute
	(
		aui_Control	*	control,
		uint32			action,
		uint32			data
	);

private:
	MBCHAR m_filename[k_AUI_LDL_MAXBLOCK + 1];
};
*/

class LoadScenarioGameAction : public aui_Action
{
public:
	LoadScenarioGameAction(MBCHAR const * name)
    :   aui_Action  ()
    {
        if (name)
        {
            strncpy(m_filename, name, _MAX_PATH);
        }
        else
        {
            m_filename[0] = 0;
        }
    };

	void	Execute
	(
		aui_Control	*	control,
		uint32			action,
		uint32			data
	) override;

private:
	MBCHAR m_filename[_MAX_PATH];
};

// App-singleton accessor pair, mirroring world_Get / c3ui_Get /
// tiledmap_Get.  The legacy g_civApp pointer is now file-static
// in ctp/civ3_main.cpp; outside callers must go through these accessors.
CivApp * civapp_Get();
void     civapp_Set(CivApp *p);

#endif
