//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Main application initialisation, processing, and cleanup.
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
// _DEBUG
// - Generates debug information when set.
//
// _DEBUG_MEMORY
// _MEMORYLOGGING
// - Generates extra memory debug information when both set, and _DEBUG set.
//
// _NO_GAME_WATCH
// - Generates a game watch file when not set.
//
// USE_SDL
// - Use SDL as replacement for DirectX.
//
// HAVE_UNISTD_H
// WIN32
// LINUX
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Keep the user's leader name when the data is consistent.
// - Skip begin turn handling when loading from a file.
// - Fixed a repetitive memory leak in the Great Libary caused by
//   scenario loading, by Martin G�hmann.
// - Removed some redundant code, because it is already done
//   somewhere else, by Martin G�hmann.
//
//----------------------------------------------------------------------------
//
// - Implemented GovernmentsModified subclass (allowing cdb files including
//   a GovernmentsModified record to produce parsers capable of reading and
//   storing subrecords for Government types.)
//   See http://apolyton.net/forums/showthread.php?s=&threadid=107916 for
//   more details  _____ by MrBaggins Jan-04
//
//   * Reordered parsing of CTPDatabase templated classes to ensure that
//     parsing Advances and Governments would occur before other DBs using
//     GovernmentsModified, to ensure that Governments would be able to be
//     inspected by those other database classes.
//
//----------------------------------------------------------------------------
//
// - When quitting to New Game, go to main menu rather than SP screen
// - Removed cleanup code for SP screen (JJB)
// - Removed some of Martin's library cleanup code, after correcting the
//   problem at the root in GreatLibrary.cpp.
// - Used the new ColorSet option to select civilisation colors.
// - Memory leak repaired: clean up the turn counter override information.
// - Hot seat handling improved.
// - Static member of StatusBar is now deleted correctly, by Martin G�hmann.
// - Cleaned up music screen.
// - The civilisation index from the profile is now reset if it is too high.
//   This prevents the game from crashing. - April 12th 2005 Martin G�hmann
// - Added crash prevention during game loading.
// - Added another civilisation index check.
// - Option added to include multiple data directories.
// - Added Slic segment cleanup.
// - Replaced old civilisation database by new one. (Aug 22nd 2005 Martin G�hmann)
// - Made progress bar more fluently. (Aug 22nd 2005 Martin G�hmann)
// - Removed the old endgame and installation databases. (Aug 29th 2005 Martin G�hmann)
// - Removed old sprite state databases, removed olf good's icon
//   database (unused), replaced old risk database by new one. (Aug 29th 2005 Martin G�hmann)
// - The right color set is now selected afterwards the ProfileDB is available. (Aug 29th 2005 Martin G�hmann)
// - Added cleanup of gaia controller and info window. (Sep 13th 2005 Martin G�hmann)
// - Added ArmyData and Network cleanup. (Sep 25th 2005 Martin G�hmann)
// - Added graphicsresscreen_Cleanup. (Sep 25th 2005 Martin G�hmann)
// - Replaced old difficulty database by new one. (April 29th 2006 Martin G�hmann)
// - Replaced old pollution database by new one. (July 15th 2006 Martin G�hmann)
// - Replaced old global warming database by new one. (July 15th 2006 Martin G�hmann)
// - Added new map icon database. (3-Mar-2007 Martin G�hmann)
// - Replaced old map database by new one. (27-Mar-2007 Martin G�hmann)
// - Replaced old concept database by new one. (31-Mar-2007 Martin G�hmann)
// - Replaced old const database by new one. (5-Aug-2007 Martin G�hmann)
// - Fixed PBEM BeginTurn event execution. (27-Oct-2007 Martin G�hmann)
// - Games can now be saved if the visible player is a robot. (30-Jan-2008 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ctp/civapp.h"
#include "gs/core/game.h"     // Ctp2::Game (owned by CivApp)
#include "gs/gameobj/Diffcly.h"  // diffutil_GetYearFromTurn
#include "ctp/ctp2_utils/civlog.h"
#include "gs/core/game_observer.h"     // g_gameObservers init in InitializeEngine
#include "gs/core/game_observer_registration.h"  // RegisterUIGameObserver + RegisterUIPlayerView
#include "robot/utility/RoboInit.h"             // roboinit_Initalize
#include "ai/ctpai.h"                           // CtpAi::Initialize
#include "ui/aui_ctp2/ui_events.h"     // ui_events_Initialize / _Cleanup

#ifdef __AUI_USE_SDL__
#include <SDL2/SDL.h>
#include "ui/aui_sdl/aui_sdlkeyboard.h"
#endif

#include "AdvanceBranchRecord.h"
#include "AdvanceListRecord.h"
#include "AdvanceRecord.h"
#include "gs/gameobj/Advances.h"
#include "gs/gameobj/advanceutil.h"
#include "AgeCityStyleRecord.h"
#include "AgeRecord.h"
#include <algorithm>                    // std::find
#include "ui/interface/ancientwindows.h"
#include "ctp/ctp2_utils/appstrings.h"
#include "gs/gameobj/ArmyData.h"                   // ArmyData::Cleanup
#include "ui/interface/armymanagerwindow.h"
#include "ui/interface/AttractWindow.h"
#include "ui/aui_common/aui_blitter.h"
#include "ui/aui_ctp2/background.h"
#include "ui/interface/backgroundwin.h"
#include "ui/interface/battleview.h"
#include "ui/aui_ctp2/bevellesswindow.h"
#include "BuildingBuildListRecord.h"
#include "BuildingRecord.h"
#include "gs/gameobj/buildingutil.h"
#include "BuildListSequenceRecord.h"
#include "ui/aui_ctp2/c3_button.h"
#include "ui/aui_ctp2/c3_checkbox.h"
#include "ui/aui_ctp2/c3_dropdown.h"
#include "ui/aui_ctp2/c3_listbox.h"
#include "ui/aui_ctp2/c3_listitem.h"
#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_ctp2/c3windows.h"
#include "ui/interface/chatbox.h"
#include "CitySizeRecord.h"
#include "CityStyleRecord.h"
#include "ui/interface/citywindow.h"
#include "ctp/civ3_main.h"
#include "robot/aibackdoor/civarchive.h"
#include "CivilisationRecord.h"
#include "gs/fileio/CivPaths.h"
#include "gs/fileio/civscenarios.h"
#include "ConceptRecord.h"
#include "ConstRecord.h"                    // g_theConstDB
#include "ui/interface/controlpanelwindow.h"
#include "ai/ctpai.h"
#include "ctp/fingerprint/ctp_finger.h"
#include "ui/interface/cursormanager.h"
#include "gs/database/DB.h"
#include "gs/newdb/DBLexer.h"
#include "ctp/debugtools/debugmemory.h"
#include "DifficultyRecord.h"
#include "ui/interface/DiplomacyDetails.h"
#include "DiplomacyProposalRecord.h"
#include "DiplomacyRecord.h"
#include "DiplomacyThreatRecord.h"
#include "ai/diplomacy/diplomacyutil.h"
#include "ui/interface/diplomacywindow.h"
#include "ui/interface/dipwizard.h"
#include "gfx/spritesys/director.h"                   // director_Get()
#include "ctp/display.h"
#include "ui/interface/DomesticManagementDialog.h"
#include "ui/interface/EditQueue.h"
#include "EndGameObjectRecord.h"
#include "ui/interface/EndgameWindow.h"
#include "gs/gameobj/Events.h"
#include "gs/gameobj/Exclusions.h"
#include "FeatRecord.h"
#include "gs/database/filenamedb.h"
#include "gs/events/GameEventManager.h"
#include "gs/fileio/gamefile.h"                   // SAVE_LEADER_NAME_SIZE
#include "gs/utility/gameinit.h"
#include "ui/interface/gameplayoptions.h"
#include "gs/gameobj/GameSettings.h"
#include "sound/gamesounds.h"
#include "gfx/gfx_utils/gfx_options.h"
#include "gs/utility/Globals.h"                    // allocated::clear, allocated::reassign
#include "GlobalWarmingRecord.h"
#include "GoalRecord.h"
#include "GovernmentRecord.h"
#include "ui/aui_ctp2/grabitem.h"
#include "ui/interface/graphicsresscreen.h"          // graphicsresscreen_Cleanup
#include "ui/interface/graphicsscreen.h"
#include "ui/interface/greatlibrarywindow.h"
#include "ui/interface/helptile.h"
#include "IconRecord.h"
#include "ImprovementListRecord.h"
#include "ui/aui_ctp2/InfoBar.h"
#include "ui/interface/infowin.h"
#include "ui/interface/infowindow.h"                 // Info Window cleanup
#include "ui/interface/initialplaywindow.h"
#include "ui/interface/intelligencewindow.h"
#include "ui/interface/IntroMovieWin.h"
#include "ui/aui_ctp2/keymap.h"
#include "ui/aui_ctp2/keypress.h"
#include "ui/interface/km_screen.h"
#include "ui/interface/loadsavewindow.h"
#include "ui/interface/MainControlPanel.h"
#include "MapIconRecord.h"
#include "MapRecord.h"
#include "gs/gameobj/message.h"
#include "gs/gameobj/MessagePool.h"                // messagepool_Get()
#include "ui/interface/messagewin.h"
#include "gs/database/moviedb.h"
#include "ui/interface/musicscreen.h"
#include "ui/interface/NationalManagementDialog.h"
#include "ctp/ctp2_utils/netconsole.h"
#include "ui/netshell/netshell.h"
#include "ui/netshell/netshell_game.h"
#include "net/general/network.h"
#include "ui/interface/optionswindow.h"
#include "ui/interface/optionwarningscreen.h"
#include "OrderRecord.h"
#include "PersonalityRecord.h"
#include "gs/gameobj/Player.h"                     // player_Get
#include "gs/database/PlayListDB.h"
#include "PollutionRecord.h"
#include "PopRecord.h"
#include "gs/fileio/prjfile.h"
#include "gs/database/profileDB.h"                  // g_theProfileDB
#include "ui/interface/ProfileEdit.h"
#include "ui/interface/progresswindow.h"
#include "ui/aui_ctp2/radarmap.h"                   // radar_map_Get()
#include "ui/interface/radarwindow.h"
#include "gs/utility/RandGen.h"                    // rand_ptr()
#include "ResourceRecord.h"
#include "RiskRecord.h"
#include "robot/utility/RoboInit.h"
#include "ui/interface/scenariowindow.h"
#include "ui/interface/sci_advancescreen.h"
#include "ui/interface/ScienceManagementDialog.h"
#include "ui/interface/sciencevictorydialog.h"       // Gaia controller window cleanup
#include "ui/interface/sciencewin.h"
#include "gfx/spritesys/screenmanager.h"
#include "ui/interface/screenutils.h"
#include "ui/aui_ctp2/SelItem.h"                    // selitem_Get()
#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicSegment.h"                // SlicSegment::Cleanup
#include "sound/soundmanager.h"               // g_soundManager
#include "SoundRecord.h"
#include "ui/interface/soundscreen.h"
#include "SpecialAttackInfoRecord.h"
#include "SpecialEffectRecord.h"
#include "ui/interface/scenarioeditor.h"
#include "ui/interface/splash.h"						// g_splash_old
#include "ui/interface/spnewgametribescreen.h"
#include "ui/interface/spnewgamewindow.h"
#include "test/smoketest_server.h"
#include "ui/interface/spriteeditor.h"
#include "SpriteRecord.h"
#include "ui/interface/statswindow.h"
#include "ui/aui_ctp2/statuswindow.h"
#include "StrategyRecord.h"
#include "gs/database/StrDB.h"
#include <string>                       // std::string
#include "gs/database/thronedb.h"                   // g_theThroneDB
#include "TerrainImprovementRecord.h"
#include "TerrainRecord.h"
#include "gs/gameobj/terrainutil.h"
#include "gfx/tilesys/tiledmap.h"
#include "ui/interface/trademanager.h"
#include "gs/utility/TurnCnt.h"                    // g_turn
#include "ui/interface/tutorialwin.h"
#include "gs/fileio/gamefile.h"
#ifdef USE_SDL
#include "ui/aui_sdl/aui_sdlsurface.h"
#endif
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/UnitData.h"
#include "UnitBuildListRecord.h"
#include "gs/utility/UnitDynArr.h"
#include "ui/interface/unitmanager.h"
#include "UnitRecord.h"
#include "gs/gameobj/unitutil.h"
#include "gs/database/UVDB.h"
#include "ui/interface/victorywin.h"
#include "WonderBuildListRecord.h"
#include "WonderRecord.h"
#include "WonderMovieRecord.h"
#include "ui/interface/workwin.h"

#ifndef _NO_GAME_WATCH
#include "GameWatch/gamewatch/GameWatch.h"
#include "GameWatch/gwciv/GWCiv.h"
#endif

#include <thread>
#include <chrono>

extern ScreenManager *          g_screenManager;
extern OzoneDatabase            *g_theUVDB;
extern MovieDB                  *g_theVictoryMovieDB;
extern FilenameDB               *g_theMessageIconFileDB;
extern PlayListDB               *g_thePlayListDB;
extern C3UI                 *g_c3ui;
extern Background           *g_background;
extern StatsWindow          *g_statsWindow;
extern StatusWindow         *g_statusWindow;
extern ControlPanelWindow   *g_controlPanel;
extern SpriteEditWindow     *g_spriteEditWindow;
extern aui_Surface          *g_sharedSurface;
extern sint32               g_modalWindow;
extern CivApp               *g_civApp;
extern ChatBox              *g_chatBox;

extern TutorialWin          *g_tutorialWin;
extern SaveInfo *           g_savedGameRequest;

// User options
extern sint32               g_fog_toggle;
extern sint32               g_god;
extern sint32               g_isCheatModeOn;
extern BOOL                 g_launchIntoCheatMode;
extern BOOL                 g_launchScenario;
extern BOOL                 g_no_exit_action;
extern BOOL                 g_no_shell;
extern BOOL                 g_no_timeslice;
extern BOOL                 g_runInBackground;
extern BOOL                 g_smokeTest;
extern sint32               g_scenarioUsePlayerNumber;
extern BOOL                 g_use_profile_process;
extern BOOL                 g_useIntroMovie;

// File names
extern MBCHAR g_improve_filename[_MAX_PATH];
extern MBCHAR g_pollution_filename[_MAX_PATH];
extern MBCHAR g_global_warming_filename[_MAX_PATH];
extern MBCHAR g_ozone_filename[_MAX_PATH];
extern MBCHAR g_terrain_filename[_MAX_PATH];
extern MBCHAR g_installation_filename[_MAX_PATH];
extern MBCHAR g_government_filename[_MAX_PATH];
extern MBCHAR g_governmenticondb_filename[_MAX_PATH]; // Empty slot
extern MBCHAR g_wonder_filename[_MAX_PATH];
extern MBCHAR g_constdb_filename[_MAX_PATH];
extern MBCHAR g_pop_filename[_MAX_PATH];
extern MBCHAR g_civilisation_filename[_MAX_PATH];
extern MBCHAR g_agedb_filename[_MAX_PATH];
extern MBCHAR g_thronedb_filename[_MAX_PATH];
extern MBCHAR g_conceptdb_filename[_MAX_PATH];
extern MBCHAR g_terrainicondb_filename[_MAX_PATH]; // Empty slot
extern MBCHAR g_advanceicondb_filename[_MAX_PATH]; // Empty slot
extern MBCHAR g_advancedb_filename[_MAX_PATH];
extern MBCHAR g_mapicondb_filename[_MAX_PATH]; // New map icon database
extern MBCHAR g_tileimprovementdb_filename[_MAX_PATH];
extern MBCHAR g_spritestatedb_filename[_MAX_PATH];
extern MBCHAR g_specialeffectdb_filename[_MAX_PATH];
extern MBCHAR g_specialattackinfodb_filename[_MAX_PATH];
extern MBCHAR g_goodsspritestatedb_filename[_MAX_PATH];
extern MBCHAR g_cityspritestatedb_filename[_MAX_PATH];
extern MBCHAR g_uniticondb_filename[_MAX_PATH]; // Used by the icon db
extern MBCHAR g_unitsdb_filename[_MAX_PATH];
extern MBCHAR g_wondericondb_filename[_MAX_PATH]; // Empty slot
extern MBCHAR g_improveicondb_filename[_MAX_PATH]; // Empty slot
extern MBCHAR g_difficultydb_filename[_MAX_PATH];
extern MBCHAR g_stringdb_filename[_MAX_PATH];
extern MBCHAR g_slic_filename[_MAX_PATH];
extern MBCHAR g_tutorial_filename[_MAX_PATH];
extern MBCHAR g_unitdb_filename[_MAX_PATH];
extern MBCHAR g_sounddb_filename[_MAX_PATH];
extern MBCHAR g_goods_filename[_MAX_PATH];
extern MBCHAR g_risk_filename[_MAX_PATH];
extern MBCHAR g_wondermoviedb_filename[_MAX_PATH];
extern MBCHAR g_victorymoviedb_filename[_MAX_PATH];
extern MBCHAR g_endgame_filename[_MAX_PATH]; // Free slot
extern MBCHAR g_messageiconfdb_filename[_MAX_PATH];
extern MBCHAR g_goodsicondb_filename[_MAX_PATH]; // Empty slot
extern MBCHAR g_orderdb_filename[_MAX_PATH];
extern MBCHAR g_mapdb_filename[_MAX_PATH];
extern MBCHAR g_playlistdb_filename[_MAX_PATH];
extern MBCHAR g_branchdb_filename[_MAX_PATH];
extern MBCHAR g_endgameicondb_filename[_MAX_PATH]; // Empty slot
extern MBCHAR g_citysize_filename[_MAX_PATH];
extern MBCHAR g_featdb_filename[_MAX_PATH];
extern MBCHAR g_endgameobject_filename[_MAX_PATH];
extern MBCHAR g_city_style_db_filename[_MAX_PATH];
extern MBCHAR g_age_city_style_db_filename[_MAX_PATH];
extern MBCHAR g_goal_db_filename[_MAX_PATH];
extern MBCHAR g_personality_db_filename[_MAX_PATH];
extern MBCHAR g_strategy_db_filename[_MAX_PATH];
extern MBCHAR g_buildlist_sequence_db_filename[_MAX_PATH];
extern MBCHAR g_unit_buildlist_db_filename[_MAX_PATH];
extern MBCHAR g_wonder_buildlist_db_filename[_MAX_PATH];
extern MBCHAR g_building_buildlist_db_filename[_MAX_PATH];
extern MBCHAR g_improvement_list_db_filename[_MAX_PATH];
extern MBCHAR g_diplomacy_db_filename[_MAX_PATH];
extern MBCHAR g_advance_list_db_filename[_MAX_PATH];
extern MBCHAR g_diplomacy_proposal_filename[_MAX_PATH];
extern MBCHAR g_diplomacy_threat_filename[_MAX_PATH];

ProjectFile *g_GreatLibPF = NULL;
ProjectFile *g_ImageMapPF = NULL;
ProjectFile *g_SoundPF = NULL;

sint32 g_logCrashes = 1;

sint32 g_oldRandSeed = FALSE;

ProgressWindow *g_theProgressWindow = NULL;

static bool g_headlessMode = false;

bool is_headless(void)      { return g_headlessMode; }
void set_headless(bool v)   { g_headlessMode = v; }

// Null-safe wrapper around ProgressTo.  In headless
// mode the global stays NULL (ProgressWindow::BeginProgress short-circuits),
// so the unconditional `ProgressTo(...)` callsites
// peppered through InitializeAppDB would UB-fault on member-call entry.  Use
// this helper at every loading-progress call site.
static inline void ProgressTo(sint32 val, MBCHAR const * msg = NULL)
{
	if (g_theProgressWindow) g_theProgressWindow->StartCountingTo(val, msg);
}

bool    g_tempLeakCheck = false;

#ifndef _NO_GAME_WATCH
int g_gameWatchID = -1;
#endif

// File-static loggers.  Anonymous namespace = internal linkage; one
// shared_ptr per translation unit, captured once at startup.  Both
// names appear as [civapp] / [smoke] in the spdlog pattern, matching
// the historical [CIVAPP] / [SMOKE] fprintf prefixes.
namespace {
auto civapp_log = civlog::Get("civapp");
auto smoke_log = civlog::Get("smoke");
}  // namespace

void InitializeGreatLibrary();
void InitializeSoundPF();
void InitializeImageMaps();

namespace
{
#if defined(_DEBUG)
/// Allocated memory at the start
size_t  g_allocatedAtStart  = 0;
#endif

/// Add search directories to a project file
/// @param  a_ProjectFile   Project file to add to
/// @param  a_Type          Type of search items
/// @param  a_PackFileName  Name of a packed (.zfs) file to add to the lookup
/// @remarks Both the directories (for individual files) and the packed files in
///          the directories are added to the search items.
void AddSearchDirectories
(
    ProjectFile *   a_ProjectFile,
    C3DIR const &   a_Type,
    MBCHAR const *  a_PackFileName
)
{
    MBCHAR          path[_MAX_PATH];

    for (int i = 0; g_civPaths->FindPath(a_Type, i++, path); )
    {
        if (path[0])
        {
            // The directory, for looking up "loose" files
            a_ProjectFile->addPath(path);

            // A predefined packed file in the directory
            strncat(path, FILE_SEP, sizeof(path) - strlen(path) - 1);
            strncat(path, a_PackFileName, sizeof(path) - strlen(path) - 1);
            a_ProjectFile->addPath(path);
        }
    }
}

/// Add search "pack files" to a project file
/// @param  a_ProjectFile   Project file to add to
/// @param  a_Type          Type of search path
/// @param  a_PackFileName  Name of a packed (.zfs) file to add to the lookup
/// @remarks Only the packed files are added to the search items.
void AddSearchPacks
(
    ProjectFile *   a_ProjectFile,
    C3DIR const &   a_Type,
    MBCHAR const *  a_PackFileName
)
{
    MBCHAR          path[_MAX_PATH];

    for (int i = 0; g_civPaths->FindPath(a_Type, i++, path); )
    {
        if (path[0])
        {
            // A predefined packed file in the directory
            strncat(path, FILE_SEP, sizeof(path) - strlen(path) - 1);
            strncat(path, a_PackFileName, sizeof(path) - strlen(path) - 1);
            a_ProjectFile->addPath(path, TRUE);
        }
    }
}

//----------------------------------------------------------------------------
//
// Name       : InitDataIncludePath
//
// Description: Add an 'include'-style path to lookup data files.
//
// Parameters : -
//
// Globals    : g_theProfileDB:   user preferences (read)
//              g_civPaths:       (updated)
//
// Returns    : -
//
// Remark(s)  : The top directories (ctp2_data-style) are read from a
//              semicolon-separated string 'Rulesets' in userprofile.txt.
//
//----------------------------------------------------------------------------
void InitDataIncludePath(void)
{
	MBCHAR                  ruleSets[MAX_PATH];
	strncpy(ruleSets, g_theProfileDB->GetRuleSets(), MAX_PATH - 1);
	ruleSets[MAX_PATH - 1] = '\0';

	std::vector<MBCHAR *>   pathStarts;
	MBCHAR *                nextPath    = ruleSets;
    size_t const            ruleSetSize = strlen(ruleSets);

	for (size_t	toDo = ruleSetSize; toDo > 0; )
	{
		pathStarts.push_back(nextPath);
		nextPath = std::find(nextPath, nextPath + toDo, PATH_SEPC);

		if (nextPath < ruleSets + ruleSetSize)
		{
			*nextPath++	= 0;
			toDo		= strlen(nextPath);
		}
		else
		{
			toDo		= 0;
		}
	}

	for
	(
	    std::vector<MBCHAR *>::reverse_iterator p  = pathStarts.rbegin();
	    p != pathStarts.rend();
	    ++p
	)
	{
		g_civPaths->InsertExtraDataPath(*p);
	}
}

//----------------------------------------------------------------------------
//
// Name       : SelectColorSet
//
// Description: Select which color set (colors##.txt file) to use.
//
// Parameters : -
//
// Globals    : g_theProfileDB  : user preferences (read)
//
// Returns    : -
//
// Remark(s)  : - When the user preference is invalid, color set 0 is used.
//              - The existence of the file is not checked.
//
//----------------------------------------------------------------------------
void SelectColorSet(void)
{
	Assert(g_theProfileDB);
	ColorSet::Initialize(g_theProfileDB->GetValueByName("ColorSet"));
}

} // namespace

//----------------------------------------------------------------------------
//
// Name       : Os
//
// Description: Wrapper for some operating system specific functions
//
// Remark(s)  : TODO: move to a better location
//
//----------------------------------------------------------------------------
namespace Os
{
	uint32 GetTicks(void)
	{
#if defined(USE_SDL)
		return SDL_GetTicks();
#else
		return GetTickCount();
#endif
	}

	void Sleep(uint32 milliSeconds)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(milliSeconds));
	}
} // namespace Os

void check_leak()
{
#if defined(_DEBUG) && defined(WIN32)
	if (g_tempLeakCheck)
	{
		_CrtMemState new_state;
		_CrtMemCheckpoint(&new_state);
		Assert(g_allocatedAtStart == new_state.lSizes[1]);
		g_allocatedAtStart = new_state.lSizes[1];
	}
#endif
}

CivApp::CivApp()
:
    m_appLoaded             (false),
    m_dbLoaded              (false),
    m_gameLoaded            (false),
    m_saveDBInGameFile      (false),
    m_aiFinishedThisTurn    (true),
    m_inBackground          (false),
    m_isKeyboardScrolling   (false),
    m_keyboardScrollingKey  (0),
    m_game                  (nullptr)
{
}

// Out-of-line dtor — Ctp2::Game is forward-declared in civapp.h, so
// the unique_ptr's deleter needs to see the full type here.
CivApp::~CivApp() = default;

void CivApp::InitializeAppUI(void)
{
	civapp_log->info("InitializeAppUI: called");
	// Set CTP2 specific data for the Anet library (multiplayer only)
	NETFunc::GameType	= GAMEID;				// CTP2 game id for Anet
	NETFunc::DllPath	= "dll" FILE_SEP "net";	// Anet DLLs are in dll\net (relative to executable)

	if (g_useIntroMovie && !g_no_shell)
	{
#if defined(__AUI_USE_SDL__)
		// SDL builds: skip intro movie since video playback is not yet supported
		civapp_log->info("InitializeAppUI: skipping intro movie on SDL");
#else
		civapp_log->info("InitializeAppUI: intro movie branch");
  		intromoviewin_Initialize();
    	intromoviewin_DisplayIntroMovie();
#endif
	}

	if (!g_useIntroMovie || g_no_shell
#if defined(__AUI_USE_SDL__)
		|| TRUE  // SDL: always show main menu after skipping intro
#endif
		)
	{
		civapp_log->info("InitializeAppUI: main menu branch");
		if (g_soundManager)
		{
			g_soundManager->EnableMusic();
			g_soundManager->PickNextTrack();
			g_soundManager->StartMusic();
		}

		AUI_ERRCODE errcode = initialplayscreen_Initialize();
		Assert(errcode == AUI_ERRCODE_OK);
		civapp_log->info("InitializeAppUI: initialplayscreen_Initialize returned {}", (int)errcode);

		if(!g_no_shell && !g_launchScenario)
		{
			civapp_log->info("InitializeAppUI: calling displayMyWindow");
			initialplayscreen_displayMyWindow();
		}
		else
		{
			civapp_log->info("InitializeAppUI: skipping displayMyWindow (no_shell={} launchScenario={})",
				(int)g_no_shell, (int)g_launchScenario);
		}
	}
	civapp_log->info("InitializeAppUI: done");
}

#ifdef _DEBUG
void print_dip_details(FILE *dipFile, const DiplomacyProposalRecord::StrengthStrings *strings1,
					   const DiplomacyProposalRecord *rec2)
{
	const DiplomacyProposalRecord::StrengthStrings *strings2;
	StringId str1, str2;

	if(strings1->GetDetailsEven(str1)) {
		if(rec2->GetDetailsEx0(strings2)) {
			if(strings2->GetDetailsEven(str2)) {
				fprintf(dipFile, "%s %s (%s/%s)\n\n",
						g_theStringDB->GetNameStr(str1), g_theStringDB->GetNameStr(str2),
						g_theStringDB->GetIdStr(str1), g_theStringDB->GetIdStr(str2));
			}
		}
		if(rec2->GetDetailsEx1(strings2)) {
			if(strings2->GetDetailsEven(str2)) {
				fprintf(dipFile, "%s %s (%s/%s)\n\n",
						g_theStringDB->GetNameStr(str1), g_theStringDB->GetNameStr(str2),
						g_theStringDB->GetIdStr(str1), g_theStringDB->GetIdStr(str2));
			}
		}
		if(rec2->GetDetailsEx2(strings2)) {
			if(strings2->GetDetailsEven(str2)) {
				fprintf(dipFile, "%s %s (%s/%s)\n\n",
						g_theStringDB->GetNameStr(str1), g_theStringDB->GetNameStr(str2),
						g_theStringDB->GetIdStr(str1), g_theStringDB->GetIdStr(str2));
			}
		}
		if(rec2->GetDetailsEx3(strings2)) {
			if(strings2->GetDetailsEven(str2)) {
				fprintf(dipFile, "%s %s (%s/%s)\n\n",
						g_theStringDB->GetNameStr(str1), g_theStringDB->GetNameStr(str2),
						g_theStringDB->GetIdStr(str1), g_theStringDB->GetIdStr(str2));
			}
		}
		if(rec2->GetDetailsEx4(strings2)) {
			if(strings2->GetDetailsEven(str2)) {
				fprintf(dipFile, "%s %s (%s/%s)\n\n",
						g_theStringDB->GetNameStr(str1), g_theStringDB->GetNameStr(str2),
						g_theStringDB->GetIdStr(str1), g_theStringDB->GetIdStr(str2));
			}
		}
	}
}
#endif

// Old style interface - parameter not used any more
sint32 CivApp::InitializeAppDB(CivArchive & /* archive */)
{
	return InitializeAppDB() ? TRUE : FALSE;
}

/// Initialize databases
bool CivApp::InitializeAppDB(void)
{
	ProgressWindow::BeginProgress
		(g_theProgressWindow, "InitProgressWindow", 520);

    // Create a set of empty databases
	g_theAdvanceDB              = new CTPDatabase<AdvanceRecord>;
	g_theAdvanceBranchDB        = new CTPDatabase<AdvanceBranchRecord>;
	g_theAdvanceListDB          = new CTPDatabase<AdvanceListRecord>;
	g_theAgeDB                  = new CTPDatabase<AgeRecord>;
	g_theAgeCityStyleDB         = new CTPDatabase<AgeCityStyleRecord>;
	g_theBuildingDB             = new CTPDatabase<BuildingRecord>;
	g_theBuildingBuildListDB    = new CTPDatabase<BuildingBuildListRecord>;
	g_theBuildListSequenceDB    = new CTPDatabase<BuildListSequenceRecord>;
	g_theCitySizeDB             = new CTPDatabase<CitySizeRecord>;
	g_theCityStyleDB            = new CTPDatabase<CityStyleRecord>;
	g_theCivilisationDB         = new CTPDatabase<CivilisationRecord>;
	g_theConceptDB              = new CTPDatabase<ConceptRecord>;
    g_theConstDB                = new CTPDatabase<ConstRecord>;
	g_theDifficultyDB           = new CTPDatabase<DifficultyRecord>;
	g_theDiplomacyDB            = new CTPDatabase<DiplomacyRecord>;
	g_theDiplomacyProposalDB    = new CTPDatabase<DiplomacyProposalRecord>;
	g_theDiplomacyThreatDB      = new CTPDatabase<DiplomacyThreatRecord>;
	g_theEndGameObjectDB        = new CTPDatabase<EndGameObjectRecord>;
	g_theFeatDB                 = new CTPDatabase<FeatRecord>;
	g_theGlobalWarmingDB        = new CTPDatabase<GlobalWarmingRecord>;
	g_theGoalDB                 = new CTPDatabase<GoalRecord>;
	g_theGovernmentDB           = new CTPDatabase<GovernmentRecord>;
	g_theIconDB                 = new CTPDatabase<IconRecord>;
	g_theImprovementListDB      = new CTPDatabase<ImprovementListRecord>;
	g_theMapDB                  = new CTPDatabase<MapRecord>;
	g_theMapIconDB              = new CTPDatabase<MapIconRecord>;
	g_theMessageIconFileDB      = new FilenameDB();
	g_theOrderDB                = new CTPDatabase<OrderRecord>;
	g_thePersonalityDB          = new CTPDatabase<PersonalityRecord>;
	g_thePlayListDB             = new PlayListDB();
	g_thePollutionDB            = new CTPDatabase<PollutionRecord>;
	g_thePopDB                  = new CTPDatabase<PopRecord>;
	g_theResourceDB             = new CTPDatabase<ResourceRecord>;
	g_theRiskDB                 = new CTPDatabase<RiskRecord>;
	g_theSoundDB                = new CTPDatabase<SoundRecord>;
	g_theSpecialAttackInfoDB    = new CTPDatabase<SpecialAttackInfoRecord>;
	g_theSpecialEffectDB        = new CTPDatabase<SpecialEffectRecord>;
	g_theSpriteDB               = new CTPDatabase<SpriteRecord>;
	g_theStrategyDB             = new CTPDatabase<StrategyRecord>;
	g_theStringDB               = new StringDB();
	g_theTerrainDB              = new CTPDatabase<TerrainRecord>;
	g_theTerrainImprovementDB   = new CTPDatabase<TerrainImprovementRecord>;
	thronedb_Set(new ThroneDB());
	g_theUVDB                   = new OzoneDatabase();
	g_theUnitBuildListDB        = new CTPDatabase<UnitBuildListRecord>;
	g_theUnitDB                 = new CTPDatabase<UnitRecord>;
	g_theVictoryMovieDB         = new MovieDB();
	g_theWonderDB               = new CTPDatabase<WonderRecord>;
	g_theWonderBuildListDB      = new CTPDatabase<WonderBuildListRecord>;
	g_theWonderMovieDB          = new CTPDatabase<WonderMovieRecord>;

    // Firstly get the string database up and running - so we can display texts
	civapp_log->info("InitializeAppDB: Parsing StringDB");
    if (!g_theStringDB->Parse(g_stringdb_filename))
    {
        return false;
    }

	// Has to be done after the initialization of the string database
	ProgressTo(10, g_theStringDB->GetNameStr("LOADING"));

    // Fill the databases from file

	civapp_log->info("InitializeAppDB: Parsing SoundDB");
	if (!g_theSoundDB->Parse(C3DIR_GAMEDATA, g_sounddb_filename))
    {
		return false;
	}

	ProgressTo( 20 );

	if (strcmp(g_mapicondb_filename, ""))   // May not exist for mods
	{
		civapp_log->info("InitializeAppDB: Parsing MapIconDB");
		if (!g_theMapIconDB->Parse(C3DIR_GAMEDATA, g_mapicondb_filename))
		{
			return false;
		}
	}

	ProgressTo( 30 );

	civapp_log->info("InitializeAppDB: Parsing IconDB");
	if (!g_theIconDB->Parse(C3DIR_GAMEDATA, g_uniticondb_filename))
    {
		return false;
	}

	ProgressTo( 40 );

	civapp_log->info("InitializeAppDB: Parsing ConstDB");
	if (!g_theConstDB->Parse(C3DIR_GAMEDATA, g_constdb_filename))
    {
		return false;
	}

	ProgressTo( 50 );

	civapp_log->info("InitializeAppDB: Parsing WonderMovieDB");
	if (!g_theWonderMovieDB->Parse(C3DIR_GAMEDATA, g_wondermoviedb_filename))
    {
		return false;
	}

	ProgressTo( 60 );

	civapp_log->info("InitializeAppDB: Parsing VictoryMovieDB");
	if (!g_theVictoryMovieDB->Parse(g_victorymoviedb_filename))
    {
	    return false;
	}

	ProgressTo( 70 );

	civapp_log->info("InitializeAppDB: Parsing PlayListDB");
	if (!g_thePlayListDB->Parse(g_playlistdb_filename))
    {
		return false;
	}

	ProgressTo( 80 );

	civapp_log->info("InitializeAppDB: Parsing SpriteDB");
	if (!g_theSpriteDB->Parse(C3DIR_GAMEDATA, "newsprite.txt"))
    {
		return false;
	}

	ProgressTo( 90 );

	civapp_log->info("InitializeAppDB: Parsing SpecialEffectDB");
	if (!g_theSpecialEffectDB->Parse(C3DIR_GAMEDATA, g_specialeffectdb_filename))
    {
		return false;
	}

	ProgressTo( 100 );

	civapp_log->info("InitializeAppDB: Parsing SpecialAttackInfoDB");
	if (!g_theSpecialAttackInfoDB->Parse(C3DIR_GAMEDATA, g_specialattackinfodb_filename))
    {
		return false;
	}

	ProgressTo( 110 );

	civapp_log->info("InitializeAppDB: Parsing AdvanceBranchDB");
	if (!g_theAdvanceBranchDB->Parse(C3DIR_GAMEDATA, g_branchdb_filename))
    {
		return false;
	}

	ProgressTo( 120 );

	civapp_log->info("InitializeAppDB: Parsing AdvanceDB");
	if (!g_theAdvanceDB->Parse(C3DIR_GAMEDATA, g_advancedb_filename))
    {
		return false;
	}

	ProgressTo( 130 );

	civapp_log->info("InitializeAppDB: Parsing GovernmentDB");
	if (!g_theGovernmentDB->Parse(C3DIR_GAMEDATA, g_government_filename))
    {
	    return false;
	}

	ProgressTo( 140 );

	civapp_log->info("InitializeAppDB: Parsing UnitDB");
	if (!g_theUnitDB->Parse(C3DIR_GAMEDATA, g_unitdb_filename))
    {
		return false;
	}

	ProgressTo( 150 );

	civapp_log->info("InitializeAppDB: Parsing DifficultyDB");
	if (!g_theDifficultyDB->Parse(C3DIR_GAMEDATA, g_difficultydb_filename))
    {
		ExitGame();
		return false;
	}

	ProgressTo( 160 );

	civapp_log->info("InitializeAppDB: Parsing AgeDB");
	if (!g_theAgeDB->Parse(C3DIR_GAMEDATA, g_agedb_filename))
    {
		ExitGame();
		return false;
	}

	ProgressTo( 170 );

	civapp_log->info("InitializeAppDB: Parsing ThroneDB");
	thronedb_Get()->Init(g_thronedb_filename );

	ProgressTo( 180 );

	civapp_log->info("InitializeAppDB: Parsing ConceptDB");
	if (!g_theConceptDB->Parse(C3DIR_GAMEDATA, g_conceptdb_filename))
    {
		ExitGame();
		return false;
	}

	ProgressTo( 190 );

	civapp_log->info("InitializeAppDB: Parsing TerrainImprovementDB");
	if (!g_theTerrainImprovementDB->Parse(C3DIR_GAMEDATA, g_tileimprovementdb_filename))
    {
		ExitGame();
		return false;
	}

	ProgressTo( 200 );

	civapp_log->info("InitializeAppDB: Parsing ResourceDB");
	if (!g_theResourceDB->Parse(C3DIR_GAMEDATA, g_goods_filename))
    {
		ExitGame();
		return false;
	}

	ProgressTo( 210 );

	civapp_log->info("InitializeAppDB: Parsing TerrainDB");
	if (!g_theTerrainDB->Parse(C3DIR_GAMEDATA, g_terrain_filename))
    {
		ExitGame();
		return false;
	}

	ProgressTo( 220 );

	civapp_log->info("InitializeAppDB: Parsing BuildingDB");
	if (!g_theBuildingDB->Parse(C3DIR_GAMEDATA, g_improve_filename))
    {
		return false;
	}

	ProgressTo( 230 );

	civapp_log->info("InitializeAppDB: Parsing PollutionDB");
	if (!g_thePollutionDB->Parse(C3DIR_GAMEDATA, g_pollution_filename))
    {
		return false;
	}

	ProgressTo( 240 );

	civapp_log->info("InitializeAppDB: Parsing GlobalWarmingDB");
	if (!g_theGlobalWarmingDB->Parse(C3DIR_GAMEDATA, g_global_warming_filename))
    {
		return false;
	}

	ProgressTo( 250 );

	civapp_log->info("InitializeAppDB: Parsing UVDB");
	if (!g_theUVDB->Initialise(g_ozone_filename, C3DIR_GAMEDATA))
	{
		ExitGame();
		return false;
	}

	ProgressTo( 260 );

	civapp_log->info("InitializeAppDB: Parsing CivilisationDB");
	if (!g_theCivilisationDB->Parse(C3DIR_GAMEDATA, g_civilisation_filename))
    {
		return false;
	}

	// Fix the player index if it is out of range:
	if(g_theProfileDB->GetCivIndex() >= g_theCivilisationDB->NumRecords())
		g_theProfileDB->SetCivIndex(1); // Set to first non-Barbarian civ

	ProgressTo( 270 );

	civapp_log->info("InitializeAppDB: Parsing WonderDB");
	if (!g_theWonderDB->Parse(C3DIR_GAMEDATA, g_wonder_filename))
    {
		return false;
	}

	ProgressTo( 280 );

	civapp_log->info("InitializeAppDB: Parsing RiskDB");
	if (!g_theRiskDB->Parse(C3DIR_GAMEDATA, g_risk_filename))
    {
		return false;
	}

	ProgressTo( 290 );

	civapp_log->info("InitializeAppDB: Parsing MessageIconFileDB");
	if (!g_theMessageIconFileDB->Parse(g_messageiconfdb_filename))
    {
		ExitGame();
		return false;
	}

	ProgressTo( 300 );

	civapp_log->info("InitializeAppDB: Parsing MapDB");
	if (!g_theMapDB->Parse(C3DIR_GAMEDATA, g_mapdb_filename))
    {
		return false;
	}

	ProgressTo( 310 );

	civapp_log->info("InitializeAppDB: Parsing OrderDB");
	if (!g_theOrderDB->Parse(C3DIR_GAMEDATA, g_orderdb_filename))
    {
		return false;
	}

	ProgressTo( 320 );

	civapp_log->info("InitializeAppDB: Parsing FeatDB");
	if (!g_theFeatDB->Parse(C3DIR_GAMEDATA, g_featdb_filename))
    {
		return false;
	}

	ProgressTo( 330 );

	civapp_log->info("InitializeAppDB: Parsing EndGameObjectDB");
	if (!g_theEndGameObjectDB->Parse(C3DIR_GAMEDATA, g_endgameobject_filename))
    {
		return false;
	}

	ProgressTo( 340 );

	civapp_log->info("InitializeAppDB: Parsing GoalDB");
	if (!g_theGoalDB->Parse(C3DIR_AIDATA, g_goal_db_filename))
    {
		return false;
	}

	ProgressTo( 350 );

	civapp_log->info("InitializeAppDB: Parsing PersonalityDB");
	if (!g_thePersonalityDB->Parse(C3DIR_AIDATA, g_personality_db_filename))
    {
		return false;
	}

	ProgressTo( 360 );

	civapp_log->info("InitializeAppDB: Parsing UnitBuildListDB");
	if (!g_theUnitBuildListDB->Parse(C3DIR_AIDATA, g_unit_buildlist_db_filename))
    {
		return false;
	}

	ProgressTo( 370 );

	civapp_log->info("InitializeAppDB: Parsing WonderBuildListDB");
	if (!g_theWonderBuildListDB->Parse(C3DIR_AIDATA, g_wonder_buildlist_db_filename))
    {
		return false;
	}

	ProgressTo( 380 );

	civapp_log->info("InitializeAppDB: Parsing BuildingBuildListDB");
	if (!g_theBuildingBuildListDB->Parse(C3DIR_AIDATA, g_building_buildlist_db_filename))
    {
		return false;
	}

	ProgressTo( 390 );

	civapp_log->info("InitializeAppDB: Parsing ImprovementListDB");
	if (!g_theImprovementListDB->Parse(C3DIR_AIDATA, g_improvement_list_db_filename))
    {
		return false;
	}

	ProgressTo( 400 );

	civapp_log->info("InitializeAppDB: Parsing StrategyDB");
	if (!g_theStrategyDB->Parse(C3DIR_AIDATA, g_strategy_db_filename))
    {
		return false;
	}

	ProgressTo( 410 );

	civapp_log->info("InitializeAppDB: Parsing BuildListSequenceDB");
	if (!g_theBuildListSequenceDB->Parse(C3DIR_AIDATA, g_buildlist_sequence_db_filename))
    {
		return false;
	}

	ProgressTo( 420 );

	civapp_log->info("InitializeAppDB: Parsing DiplomacyDB");
	if (!g_theDiplomacyDB->Parse(C3DIR_AIDATA, g_diplomacy_db_filename))
    {
		return false;
	}

	ProgressTo( 430 );

	civapp_log->info("InitializeAppDB: Parsing DiplomacyProposalDB");
	if (!g_theDiplomacyProposalDB->Parse(C3DIR_AIDATA, g_diplomacy_proposal_filename))
    {
		return false;
	}

	ProgressTo( 440 );

	civapp_log->info("InitializeAppDB: Parsing DiplomacyThreatDB");
	if (!g_theDiplomacyThreatDB->Parse(C3DIR_AIDATA, g_diplomacy_threat_filename))
    {
		return false;
	}

	ProgressTo( 450 );

	civapp_log->info("InitializeAppDB: Parsing AdvanceListDB");
	if (!g_theAdvanceListDB->Parse(C3DIR_AIDATA, g_advance_list_db_filename))
    {
		return false;
	}

	ProgressTo( 460 );

	civapp_log->info("InitializeAppDB: Parsing CityStyleDB");
	if (!g_theCityStyleDB->Parse(C3DIR_GAMEDATA, g_city_style_db_filename))
    {
		return false;
	}

	ProgressTo( 470 );

	civapp_log->info("InitializeAppDB: Parsing AgeCityStyleDB");
	if (!g_theAgeCityStyleDB->Parse(C3DIR_GAMEDATA, g_age_city_style_db_filename))
    {
		return false;
	}

	ProgressTo( 480 );

    {
        char *lastdot = strrchr(g_citysize_filename, '.');
		Assert(lastdot);

		if(!lastdot)
			lastdot = g_citysize_filename + strlen(g_citysize_filename);

		snprintf(lastdot, _MAX_PATH - (lastdot - g_citysize_filename), "%d.txt", 0);

		civapp_log->info("InitializeAppDB: Parsing CitySizeDB");
		if (!g_theCitySizeDB->Parse(C3DIR_GAMEDATA, g_citysize_filename))
			return false;

		strcpy(lastdot, ".txt");
	}

	ProgressTo( 490 );

	civapp_log->info("InitializeAppDB: Parsing PopDB");
	if (!g_thePopDB->Parse(C3DIR_GAMEDATA, g_pop_filename))
    {
		return false;
	}

	ProgressTo( 500 );

	civapp_log->info("InitializeAppDB: Creating Exclusions, resolving references");
    exclusions_Set(new Exclusions());

	if(!g_theUnitDB->ResolveReferences())               return false;
	if(!g_theAdvanceDB->ResolveReferences())            return false;
	if(!g_theIconDB->ResolveReferences())               return false;
	if(!g_theConstDB->ResolveReferences())              return false;
	if(!g_theSpriteDB->ResolveReferences())             return false;
	if(!g_theSoundDB->ResolveReferences())              return false;
	if(!g_theSpecialEffectDB->ResolveReferences())      return false;
	if(!g_theSpecialAttackInfoDB->ResolveReferences())  return false;
	if(!g_theGovernmentDB->ResolveReferences())         return false;
	if(!g_theAdvanceBranchDB->ResolveReferences())      return false;
	if(!g_theAgeDB->ResolveReferences())                return false;
	if(!g_theTerrainDB->ResolveReferences())            return false;
	if(!g_theResourceDB->ResolveReferences())           return false;
	if(!g_theTerrainImprovementDB->ResolveReferences()) return false;
	if(!g_theMapDB->ResolveReferences())                return false;
	if(!g_theOrderDB->ResolveReferences())              return false;
	if(!g_theMapIconDB->ResolveReferences())            return false;
	if(!g_theCitySizeDB->ResolveReferences())           return false;
	if(!g_thePopDB->ResolveReferences())                return false;
	if(!g_theBuildingDB->ResolveReferences())           return false;
	if(!g_thePollutionDB->ResolveReferences())          return false;
	if(!g_theCityStyleDB->ResolveReferences())          return false;
	if(!g_theAgeCityStyleDB->ResolveReferences())       return false;
	if(!g_theGoalDB->ResolveReferences())               return false;
	if(!g_thePersonalityDB->ResolveReferences())        return false;
	if(!g_theUnitBuildListDB->ResolveReferences())      return false;
	if(!g_theWonderBuildListDB->ResolveReferences())    return false;
	if(!g_theBuildingBuildListDB->ResolveReferences())  return false;
	if(!g_theConceptDB->ResolveReferences())            return false;
	if(!g_theImprovementListDB->ResolveReferences())    return false;
	if(!g_theStrategyDB->ResolveReferences())           return false;
	if(!g_theBuildListSequenceDB->ResolveReferences())  return false;
	if(!g_theDiplomacyDB->ResolveReferences())          return false;
	if(!g_theDiplomacyProposalDB->ResolveReferences())  return false;
	if(!g_theDiplomacyThreatDB->ResolveReferences())    return false;
	if(!g_theAdvanceListDB->ResolveReferences())        return false;
	if(!g_theCivilisationDB->ResolveReferences())       return false;
	if(!g_theWonderDB->ResolveReferences())             return false;
	if(!g_theWonderMovieDB->ResolveReferences())        return false;
	if(!g_theFeatDB->ResolveReferences())               return false;
	if(!g_theEndGameObjectDB->ResolveReferences())      return false;
	if(!g_theRiskDB->ResolveReferences())               return false;
	if(!g_theDifficultyDB->ResolveReferences())         return false;
	if(!g_theGlobalWarmingDB->ResolveReferences())      return false;

	ProgressTo( 510 );

	unitutil_Initialize();
	advanceutil_Initialize();
	diplomacyutil_Initialize();
	terrainutil_Initialize();
	buildingutil_Initialize();

	ProgressTo( 520 );

#ifdef _DEBUG
	FILE *dipFile = fopen("dipcombo.txt", "w");
	if(dipFile) {
		sint32 i,j;

		for(i = 1; i < g_theDiplomacyProposalDB->NumRecords(); i++) {
			const DiplomacyProposalRecord *rec1 = g_theDiplomacyProposalDB->Get(i);
			for(j = 1; j < g_theDiplomacyProposalDB->NumRecords(); j++) {
				if(j == i) continue;

				const DiplomacyProposalRecord *rec2 = g_theDiplomacyProposalDB->Get(j);
				if(rec1->GetExcludes() & rec2->GetClass()) continue;

				StringId str1, str2;
				const DiplomacyProposalRecord::StrengthStrings *strings1, *strings2;

				if(rec1->GetDetails0(strings1)) {
					if(strings1->GetDetailsEven(str1)) {
						if(rec2->GetDetailsEx0(strings2)) {
							if(strings2->GetDetailsEven(str2)) {
								fprintf(dipFile, "%s %s (%s/%s)\n\n",
										g_theStringDB->GetNameStr(str1), g_theStringDB->GetNameStr(str2),
										g_theStringDB->GetIdStr(str1), g_theStringDB->GetIdStr(str2));
							}
						}
					}
				}
				if(rec1->GetDetails1(strings1)) {
					if(strings1->GetDetailsEven(str1)) {
						if(rec2->GetDetailsEx1(strings2)) {
							if(strings2->GetDetailsEven(str2)) {
								fprintf(dipFile, "%s %s (%s/%s)\n\n",
										g_theStringDB->GetNameStr(str1), g_theStringDB->GetNameStr(str2),
										g_theStringDB->GetIdStr(str1), g_theStringDB->GetIdStr(str2));
							}
						}
					}
				}
				if(rec1->GetDetails2(strings1)) {
					if(strings1->GetDetailsEven(str1)) {
						if(rec2->GetDetailsEx2(strings2)) {
							if(strings2->GetDetailsEven(str2)) {
								fprintf(dipFile, "%s %s (%s/%s)\n\n",
										g_theStringDB->GetNameStr(str1), g_theStringDB->GetNameStr(str2),
										g_theStringDB->GetIdStr(str1), g_theStringDB->GetIdStr(str2));
							}
						}
					}
				}
				if(rec1->GetDetails3(strings1)) {
					if(strings1->GetDetailsEven(str1)) {
						if(rec2->GetDetailsEx3(strings2)) {
							if(strings2->GetDetailsEven(str2)) {
								fprintf(dipFile, "%s %s (%s/%s)\n\n",
										g_theStringDB->GetNameStr(str1), g_theStringDB->GetNameStr(str2),
										g_theStringDB->GetIdStr(str1), g_theStringDB->GetIdStr(str2));
							}
						}
					}
				}
				if(rec1->GetDetails4(strings1)) {
					if(strings1->GetDetailsEven(str1)) {
						if(rec2->GetDetailsEx4(strings2)) {
							if(strings2->GetDetailsEven(str2)) {
								fprintf(dipFile, "%s %s (%s/%s)\n\n",
										g_theStringDB->GetNameStr(str1), g_theStringDB->GetNameStr(str2),
										g_theStringDB->GetIdStr(str1), g_theStringDB->GetIdStr(str2));
							}
						}
					}
				}
			}
		}
		fclose(dipFile);
	}
#endif
	ProgressWindow::EndProgress( g_theProgressWindow );

	m_dbLoaded = true;

	return true;
}








bool CivApp::IsScenarioEditorShown(void) const
{
	return ScenarioEditor::IsShown();
}

bool CivApp::IsScenarioEditorGivingAdvances(void) const
{
	return ScenarioEditor::IsGivingAdvances();
}

sint32 CivApp::InitializeEngine(void)
{
	civapp_log->info("InitializeEngine: started");

	// Wire the game-observer registry global before anything else that might
	// fire Notify*().  See game_observer.cpp for why this is deferred from
	// static-init time.
	g_gameObservers = &GameObserverRegistry::Instance();

	Splash::Initialize();

	CivPaths_InitCivPaths();

	g_theProfileDB = new ProfileDB;
	if (!g_theProfileDB->Init(FALSE)) {
		c3errors_FatalDialog("CivApp", "Unable to init the ProfileDB.");
		return -1;
	}

	g_logCrashes = g_theProfileDB->GetEnableLogs();

	InitDataIncludePath();
	c3files_InitializeCD();
	g_civPaths->InitCDPath();
	GreatLibrary::Initialize_Great_Library_Data();

#ifndef _NO_GAME_WATCH
	gameWatch.DeliverySystem("gwfile", g_theProfileDB->GetGameWatchDirectory());
	gameWatch.RecordingSystem("gwciv");
#endif

	civapp_log->info("InitializeEngine: done");
	return 0;
}

sint32 CivApp::InitializeApp(HINSTANCE hInstance, int iCmdShow)
{
	civapp_log->info("InitializeApp: started");
#ifdef WIN32
    // COM needed for DirectX/Movies
	CoInitialize(NULL);
#endif

	sint32 err = InitializeEngine();
	if (err != 0) return err;

	civapp_log->info("InitializeApp: display_Initialize");
	display_Initialize(hInstance, iCmdShow);
	civapp_log->info("InitializeApp: init_keymap");
	init_keymap();
	civapp_log->info("InitializeApp: ui_Initialize");
	(void) ui_Initialize();
	civapp_log->info("InitializeApp: SoundManager::Initialize");
	SoundManager::Initialize();

	civapp_log->info("InitializeApp: sharedsurface_Initialize");
	if ( sharedsurface_Initialize() != AUI_ERRCODE_OK ) {
		c3errors_FatalDialog( "CivApp", "Unable to init shared surface." );
		return -1;
	}

	civapp_log->info("InitializeApp: CursorManager::Initialize");
	CursorManager::Initialize();

	InitializeImageMaps();

	civapp_log->info("InitializeApp: ProgressWindow");
	ProgressWindow::BeginProgress(
		g_theProgressWindow,
		"InitProgressWindow",
		620 );

	ProgressTo( 10 );

	civapp_log->info("InitializeApp: gameinit_InitializeGameFiles");
	if (!gameinit_InitializeGameFiles())
    {
        ExitGame();
        return -1;
    }

    SPLASH_STRING(g_theProfileDB->IsAIOn() ? "AI is ON" : "AI is OFF");

	ProgressTo( 20 );

	ProgressTo( 540 );

	civapp_log->info("InitializeApp: InitializeAppDB");
	if (!InitializeAppDB())
    {
		c3errors_FatalDialog("CivApp", "Unable to Init the Databases.");
		return -1;
	}

	ProgressTo( 550 );

	civapp_log->info("InitializeApp: InitializeGreatLibrary");
	InitializeGreatLibrary();

	ProgressTo( 560 );

	civapp_log->info("InitializeApp: InitializeSoundPF");
	InitializeSoundPF();

	ProgressTo( 570 );

	civapp_log->info("InitializeApp: calling InitializeAppUI");
	InitializeAppUI();
	civapp_log->info("InitializeApp: InitializeAppUI returned");

	ProgressTo( 580 );

	CivScenarios::Initialize();

	{
		// Maintain consistency between the CivIndex and CivName entries.
		// When inconsistent, the CivIndex is leading.

		sint32 const userCivIndex = g_theProfileDB->GetCivIndex();

		if (static_cast<int>(userCivIndex) < g_theCivilisationDB->NumRecords())
		{
			MBCHAR const * const    dbCivName =
			    g_theStringDB->GetNameStr
			        (g_theCivilisationDB->Get(userCivIndex)->GetPluralCivName());

			if (0 == strcmp(dbCivName, g_theProfileDB->GetCivName()))
			{
				// No action: keep the leader name of the user.
			}
			else
			{
				// Restore civilisation default country and leader names.
				g_theProfileDB->DefaultSettings();
			}
		}
		else
		{
			// Possible after using a mod with less civilisations
			g_theProfileDB->SetCivIndex(1);
			g_theProfileDB->DefaultSettings();
		}
	}

	ProgressTo( 590 );

	StartMessageSystem();

	ProgressTo( 600 );

	SPLASH_STRING("Initializing Messaging System...");
	AUI_ERRCODE errcode = messagewin_InitializeMessages();
	Assert(errcode == 0);
	if (errcode != 0) return 7;

	ProgressTo( 610 );

	if (g_theProfileDB->IsUseFingerprinting())
		if (!ctpfinger_Check()) {

			c3errors_FatalDialog(appstrings_GetString(APPSTR_INITIALIZE),
									appstrings_GetString(APPSTR_CANTFINDFILE));
		}

	ProgressTo( 620 );

	if (g_c3ui->TheMouse())
    {
		double const sensitivity = 0.25 * (1 + g_theProfileDB->GetMouseSpeed());
		g_c3ui->TheMouse()->Sensitivity() = sensitivity;
	}

	SelectColorSet(); // Select the right color set.

	ProgressWindow::EndProgress( g_theProgressWindow );

	// Wire UI hooks for the IGameObserver registry and player_view bindings
	// here, after the registry is initialised by InitializeEngine.  Previously
	// these were called from civ3_main.cpp only in the default-launch branch,
	// leaving --load / --launchScenario / --no-shell paths with a null
	// selitem_Get() (player_view::Init was a no-op because s_init was null).
	RegisterUIGameObserver();
	RegisterUIPlayerView();
	RegisterProgressWindowObserver();
	RegisterTextObserverAdapter();
	RegisterBattleObserverAdapter();
	RegisterDipWizardObserverAdapter();

	m_appLoaded = true;

	if (g_smokeTest) {
		smoke_log->info("Smoke test mode enabled, starting command server");
		smoketest_server_init();
	}

	return 0;
}






sint32 CivApp::QuickInit(HINSTANCE hInstance, int iCmdShow)
{
	InitializeApp(hInstance, iCmdShow);
	StartGame();

	return 0;
}

void CivApp::CleanupAppUI(void)
{
	NetShell::Leave( k_NS_FLAGS_DESTROY );

	// Clean up any opened screens
	greatlibrary_Cleanup();
	spnewgamescreen_Cleanup();
	spnewgametribescreen_Cleanup();
	initialplayscreen_Cleanup();
	scenarioscreen_Cleanup();
	optionsscreen_Cleanup();
	optionwarningscreen_Cleanup();
	graphicsscreen_Cleanup();
	gameplayoptions_Cleanup();
	soundscreen_Cleanup();
	musicscreen_Cleanup();
	StatusBar::CleanUp();
	loadsavescreen_Cleanup();
	graphicsresscreen_Cleanup();
	km_screen_Cleanup();

    allocated::clear(g_c3ui);

#if defined(_DEBUG) && !defined(__AUI_USE_SDL__)
	sint32 const cleanBaseRefCount = aui_Base::GetBaseRefCount();
	Assert(0 == cleanBaseRefCount);
#endif

	allocated::clear(g_GreatLibPF);
	allocated::clear(g_ImageMapPF);
	allocated::clear(g_SoundPF);
}






void CivApp::CleanupAppDB(void)
{
    allocated::clear(g_theMapDB);
    { Exclusions *p = exclusions_Get(); delete p; exclusions_Set(NULL); }
    allocated::clear(g_theMessageIconFileDB);
    allocated::clear(g_theRiskDB);
    allocated::clear(g_theWonderDB);
    allocated::clear(g_theCivilisationDB);
    allocated::clear(g_thePollutionDB);
    allocated::clear(g_theUVDB);
    allocated::clear(g_theGlobalWarmingDB);
    allocated::clear(g_theBuildingDB);
    allocated::clear(g_theTerrainDB);
    allocated::clear(g_theResourceDB);
    allocated::clear(g_theGovernmentDB);
    allocated::clear(g_theConceptDB);
    { ThroneDB *p = thronedb_Get(); delete p; thronedb_Set(NULL); };
    allocated::clear(g_theAgeDB);
    allocated::clear(g_theCityStyleDB);
    allocated::clear(g_theAgeCityStyleDB);
    allocated::clear(g_theDifficultyDB);
    allocated::clear(g_theUnitDB);
    allocated::clear(g_theAdvanceDB);
    allocated::clear(g_theAdvanceBranchDB);
    allocated::clear(g_theSpecialEffectDB);
    allocated::clear(g_theSpriteDB);
    allocated::clear(g_theSpecialAttackInfoDB);
    allocated::clear(g_thePlayListDB);
    allocated::clear(g_theVictoryMovieDB);
    allocated::clear(g_theWonderMovieDB);
    allocated::clear(g_theIconDB);
    allocated::clear(g_theSoundDB);
    allocated::clear(g_theStringDB);
    allocated::clear(g_theTerrainImprovementDB);
    allocated::clear(g_theOrderDB);
    allocated::clear(g_theCitySizeDB);
    allocated::clear(g_thePopDB);
    allocated::clear(g_theBuildingDB);
    allocated::clear(g_theFeatDB);
    allocated::clear(g_theEndGameObjectDB);
    allocated::clear(g_theGoalDB);
    allocated::clear(g_thePersonalityDB);
    allocated::clear(g_theUnitBuildListDB);
    allocated::clear(g_theWonderBuildListDB);
    allocated::clear(g_theBuildingBuildListDB);
    allocated::clear(g_theImprovementListDB);
    allocated::clear(g_theStrategyDB);
    allocated::clear(g_theBuildListSequenceDB);
    allocated::clear(g_theDiplomacyDB);
    allocated::clear(g_theAdvanceListDB);
    allocated::clear(g_theDiplomacyProposalDB);
    allocated::clear(g_theDiplomacyThreatDB);
    allocated::clear(g_theMapIconDB);
    allocated::clear(g_theConstDB);

    m_dbLoaded = false;
}




void CivApp::CleanupApp(void)
{
	if (m_appLoaded)
	{
		g_network.Cleanup();
		GreatLibrary::Shutdown_Great_Library_Data();
		Splash::Cleanup();
		messagewin_Cleanup();

		delete slicengine_Get();
		slicengine_Set(NULL);
		delete messagepool_Get(); messagepool_Set(NULL);

		CivScenarios::Cleanup();
		SoundManager::Cleanup();

		allocated::clear(g_theProfileDB);

		gameinit_Cleanup();
		ui_events_Cleanup();
		events_Cleanup();
		gameEventManager_Cleanup();
		g_network.Cleanup();
		CursorManager::Cleanup();
		sharedsurface_Cleanup();
		CleanupAppUI();
		cleanup_keymap();
		CleanupAppDB();
		CivPaths_CleanupCivPaths();
		SlicSegment::Cleanup();

#ifdef WIN32
		// COM needed for DirectX Moviestuff
		CoUninitialize();
#endif

		display_Cleanup();
	}

	if (g_smokeTest) {
		smoketest_server_shutdown();
	}

	m_appLoaded = false;
}







sint32 CivApp::InitializeGameUI(void)
{
	SelectColorSet();
	spnewgamescreen_Cleanup();
	spnewgametribescreen_Cleanup();
	initialplayscreen_Cleanup();
	scenarioscreen_Cleanup();

	ProgressWindow::BeginProgress(
		g_theProgressWindow,
		"InitProgressWindow",
		130 );

	ProgressTo(10, g_theStringDB->GetNameStr("LOADING"));

	SPLASH_STRING("Creating Main Windows...");
#if defined(_DEBUG)
	splash_MarkOld();
#endif

	SPLASH_STRING("Creating Status Window...");
	sint32 errcode = c3windows_MakeStatusWindow(TRUE);
	Assert(errcode == 0);
	if (errcode != 0) return 7;
	g_statusWindow->Hide(); // Maybe should be removed entirely

	ProgressTo( 20 );

	SPLASH_STRING("Creating Game Window...");
	errcode = backgroundWin_Initialize();
	Assert(errcode == 0);
	if (errcode != 0) return 7;

	ProgressTo( 30 );

	SPLASH_STRING("Creating Tile Help Window...");
	errcode = helptile_Initialize();
	Assert(errcode == 0);
	if (errcode != 0) return 7;

	ProgressTo( 40 );

	SPLASH_STRING("Creating Debug Window...");
	errcode = c3windows_MakeDebugWindow(TRUE);

	ProgressTo( 50 );

	AncientWindows_PreInitialize();

	ProgressTo( 60 );

	SPLASH_STRING("Creating Radar Window...");
	errcode = radarwindow_Initialize();
	Assert(errcode == 0);
	if (errcode != 0) return 7;

	ProgressTo( 70 );

	SPLASH_STRING("Creating Info Bar...");
	InfoBar::Initialize();

	ProgressTo( 80 );

	SPLASH_STRING("Creating Control Panel Window...");
	errcode = controlpanelwindow_Initialize();

	ProgressTo( 90 );

    AUI_ERRCODE auiErr = g_c3ui->AddWindow( g_background );
	Assert(auiErr == AUI_ERRCODE_OK);
	if ( auiErr != AUI_ERRCODE_OK ) return 11;

	ProgressTo( 100 );

	auiErr = g_c3ui->AddWindow( g_statusWindow );
	Assert(auiErr == AUI_ERRCODE_OK);
	if ( auiErr != AUI_ERRCODE_OK ) return 11;

	ProgressTo( 110 );

	radarwindow_Display();

	ProgressTo( 120 );

	errcode = AncientWindows_Initialize();

	g_modalWindow = 0;

	ProgressTo( 130 );

	AttractWindow::Initialize();

	ProgressWindow::EndProgress( g_theProgressWindow );

	return 0;
}






sint32 CivApp::InitializeGame(CivArchive *archive)
{
	// Headless: g_c3ui is null and every helper below (c3windows_*,
	// ChatBox, GrabItem, MainControlPanel, director_Get()->*, scenario UI
	// reload, etc.) crashes or no-ops on UI singletons.  Reroute to
	// the headless path which does only the game-state restore +
	// minimal subsystem init (AI, gevManager).  Both --new-game and
	// --load-game share this entry point now.
	if (!g_c3ui) return InitializeGameHeadless(archive);

#ifndef _NO_GAME_WATCH
	SPLASH_STRING("Initializing Game Watch...");
	g_gameWatchID = gameWatch.StartGame();
#endif // _NO_GAME_WATCH

	ProgressWindow::BeginProgress(
		g_theProgressWindow,
		"InitProgressWindow",
		770 );

	ProgressTo
	    (10, g_theStringDB->GetNameStr("LOADING"));

	init_keymap();

	ProgressTo( 20 );

	SPLASH_STRING("Initializing Sprite Engine...");

	sprite_Initialize();

	ProgressTo( 540 );

    if (m_dbLoaded && g_theProfileDB->IsScenario())
    {
        CleanupAppDB();
        InitializeAppDB();

        // The cached patterns of the allocated windows have to be unloaded to
        // be able to reinitialise the image maps safely.
        // InitializeGameUI will take care of this by closing most windows, but
        // the progress window stays open. It has to be handled manually to
        // prevent possible corruption of its background and border patterns.
        if (g_theProgressWindow) g_theProgressWindow->PatternInfoSave();

        InitializeImageMaps();
        InitializeSoundPF();

		// Restore the progress window background
        if (g_theProgressWindow) g_theProgressWindow->PatternInfoRestore();

        InitializeGameUI();
        greatlibrary_Cleanup();
        InitializeGreatLibrary();
        GreatLibrary::Initialize_Great_Library_Data();
    }
    else
    {
    	InitializeGameUI();
    }

	ProgressTo( 560 );

	ChatBox::Initialize();

	ProgressTo( 570 );

	GrabItem::Init();

	ProgressTo( 580 );

	if (m_dbLoaded && g_theProfileDB->IsScenario()) {
		if(g_controlPanel)
			g_controlPanel->CreateTileImpBanks();
	}

	ProgressTo( 590 );

	gameEventManager_Initialize();

	// Prevent the event handler corrupting the (diplomacy) data in the
	// middle of a file restore operation.
	gevmanager_Get()->Pause();

	events_Initialize();
	ui_events_Initialize();

	ProgressTo( 600 );

	g_fog_toggle = FALSE;
	g_god = FALSE;

	if (!gameinit_Initialize(-1, -1, archive)) {
		gevmanager_Get()->Resume();
		return FALSE;
	}

	ProgressTo( 610 );

	if(is_scenario_Get() && (archive != NULL &&
	   (start_info_type_Get() != STARTINFOTYPE_NONE ||
		save_file_version_Get() < gamefile_CurrentVersion()))) {

		for(sint32 i = 0; i < k_MAX_PLAYERS; i++) {
			if(player_Get(i)) {
				player_Get(i)->m_messages->Clear();
			}
		}

        delete messagepool_Get();
        messagepool_Set(new MessagePool());

		SlicEngine::Reload(g_slic_filename);

		if(g_scenarioUsePlayerNumber > 0 && player_Get(g_scenarioUsePlayerNumber) &&
		   player_Get(g_scenarioUsePlayerNumber)->m_civilisation &&
		   g_theCivilisationDB && g_theProfileDB) {
			Player *        p       = player_Get(g_scenarioUsePlayerNumber);
			StringId        id      =
                (p->m_civilisation->GetDBRec())->GetLeaderNameMale();
			const MBCHAR *name = g_theStringDB->GetNameStr(id);
			if(name) {
				p->m_civilisation->AccessData()->SetLeaderName(name);
				g_theProfileDB->SetLeaderName(name);
			}
		}
	}

	ProgressTo( 620 );

	if(is_scenario_Get() && !g_oldRandSeed) {






		civrand().Initialize(static_cast<sint32>(time(0)));
	}

	ProgressTo( 630 );

	if (is_scenario_Get())
	{
		for (size_t p = 0; p < k_MAX_PLAYERS; ++p)
		{
			if (player_Get(p))
			{
				player_Get(p)->m_civilisation->ResetCiv
					(player_Get(p)->m_civilisation->GetCivilisation(),
					 player_Get(p)->m_civilisation->GetGender()
					);
			}
		}
	}

	ProgressTo( 640 );

#ifdef _DEBUG
	gevmanager_Get()->Dump();
#endif

	ProgressTo( 650 );

	GraphicsOptions::Initialize();

	SPLASH_STRING("Initializing Tile Engine...");
	tile_Initialize(archive != NULL);

	ProgressTo( 660 );

	if (is_scenario_Get()) {
		tiledmap_Get()->PostProcessMap();
	}

	ProgressTo( 670 );

	radar_Initialize();

	ProgressTo( 680 );

	Splash::Cleanup();

	ProgressTo( 690 );

	// Allocate per-session Ctp2::Game container.  gameinit_Initialize
	// has already created the legacy globals (g_turn, g_thePollution,
	// topten_Get(), etc.); the container's own NewGame() instantiates
	// its TurnCount alongside.  During the long-running globals refactor
	// they coexist; callers will migrate to game.GetTurn() incrementally.
	m_game = std::make_unique<Ctp2::Game>();
	m_game->NewGame(
		g_theProfileDB->GetNPlayers(),
		diffutil_GetYearFromTurn(gamesettings_Get()->GetDifficulty(), 0)
	);

	m_gameLoaded = TRUE;
	//gevmanager_Get()->Resume();
	//gevmanager_Get()->Process();

	ProgressTo( 700 );

	director_Get()->CatchUp();

  gevmanager_Get()->Resume();
  gevmanager_Get()->Process();

	ProgressTo( 710 );

	if(!g_network.IsActive() && !g_network.IsNetworkLaunch())
	{
		if ((archive == NULL) ||										// launch button
			((start_info_type_Get() != STARTINFOTYPE_NONE) && is_scenario_Get())	// scenario start
		   )
		{
			gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
				GEV_BeginTurn,
				GEA_Player, selitem_Get()->GetCurPlayer(),
				GEA_Int, player_Get(selitem_Get()->GetCurPlayer())->m_current_round,
				GEA_End);
		}
	}

	ProgressTo( 720 );

	if(!g_network.IsActive()) {
		if (archive == NULL ||
			(save_file_version_Get() >= 42 &&





			(is_scenario_Get() && start_info_type_Get() != STARTINFOTYPE_NOLOCS)))
        {
			if (director_Get())
				director_Get()->AddCopyVision();
		}
	}

	ProgressTo( 730 );

	director_Get()->ReloadAllSprites();

	ProgressTo( 740 );

	if(g_turn->IsEmail() && archive != NULL) {
		selitem_Get()->KeyboardSelectFirstUnit();
		if(selitem_Get()->GetState() != SELECT_TYPE_LOCAL_ARMY &&
		   (player_Get(selitem_Get()->GetVisiblePlayer())->m_all_cities->Num() > 0)) {
			selitem_Get()->SetSelectCity(player_Get(selitem_Get()->GetVisiblePlayer())->m_all_cities->Access(0));
			director_Get()->AddCenterMap(player_Get(selitem_Get()->GetVisiblePlayer())->m_all_cities->Access(0).RetPos());
		}
		director_Get()->AddCenterMap(selitem_Get()->GetCurSelectPos());

		slicengine_Get()->CheckPendingResearch();
	}

	ProgressTo( 750 );

	if (!g_turn->IsHotSeat())
	{
		MainControlPanel::UpdateCityList();
	}

	g_scenarioUsePlayerNumber = 0;

	ProgressTo( 760 );

	if ((archive) && g_turn->IsHotSeat())
    {
	    // Indicate the resuming player when loading a saved hotseat game
	    g_turn->SendNextPlayerMessage();
    }
	else if (selitem_Get())
    {
        if (!archive)
        {
            selitem_Get()->Refresh();
        }

		if (director_Get())
			director_Get()->AddCenterMap(selitem_Get()->GetCurSelectPos());
	}

	ProgressTo( 770 );

	g_oldRandSeed = FALSE;

	if (!g_turn->IsHotSeat())
	{
		MainControlPanel::UpdatePlayer(selitem_Get()->GetCurPlayer());
	}

    if (g_launchIntoCheatMode)
    {
        ScenarioEditor::Display();
    }

	if (    g_turn->IsEmail()
	     && player_Get(selitem_Get()->GetCurPlayer())->IsTurnOver()
	){
		gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
		                       GEV_BeginTurn,
		                       GEA_Player, selitem_Get()->GetCurPlayer(),
		                       GEA_Int,    player_Get(selitem_Get()->GetCurPlayer())->GetCurRound() + 1,
		                       GEA_End);
	}

	ProgressWindow::EndProgress( g_theProgressWindow );

	return 0;
}





sint32 InitializeSpriteEditorUI(void)
{
	SelectColorSet();

	spnewgamescreen_Cleanup();
	spnewgametribescreen_Cleanup();
	initialplayscreen_Cleanup();
	scenarioscreen_Cleanup();

	ProgressWindow::BeginProgress(
		g_theProgressWindow,
		"InitProgressWindow",
		120 );

	ProgressTo(10, g_theStringDB->GetNameStr("LOADING"));

	SPLASH_STRING("Creating Main Windows...");
#if defined(_DEBUG)
	splash_MarkOld();
#endif

	ProgressTo( 20 );

	SPLASH_STRING("Creating Status Window...");
	sint32          errcode;

	errcode = c3windows_MakeStatusWindow(TRUE);
	Assert(errcode == 0);
	if (errcode != 0) return 7;

	ProgressTo( 30 );

	SPLASH_STRING("Creating Game Window...");
	errcode = backgroundWin_Initialize(false);
	Assert(errcode == 0);
	if (errcode != 0) return 7;

	ProgressTo( 40 );

	SPLASH_STRING("Creating Tile Help Window...");
	errcode = helptile_Initialize();
	Assert(errcode == 0);
	if (errcode != 0) return 7;

	ProgressTo( 50 );
	SPLASH_STRING("Creating Debug Window...");
	errcode = c3windows_MakeDebugWindow(TRUE);

	ProgressTo( 60 );

	AncientWindows_PreInitialize();

	ProgressTo( 70 );

	SPLASH_STRING("Creating Control Panel Window...");

	errcode = controlpanelwindow_Initialize();

	ProgressTo( 80 );

	SPLASH_STRING("Creating Radar Window...");
	errcode = radarwindow_Initialize();
	Assert(errcode == 0);
	if (errcode != 0) return 7;

	ProgressTo( 90 );

	AUI_ERRCODE	auiErr = g_c3ui->AddWindow(g_background);
	Assert(auiErr == AUI_ERRCODE_OK);
	if ( auiErr != AUI_ERRCODE_OK ) return 11;

	SPLASH_STRING("Creating Info Bar...");
	InfoBar::Initialize();

	ProgressTo( 100 );

	g_modalWindow = 0;
	AttractWindow::Initialize();

	ProgressTo( 110 );

	HideControlPanel();
	g_statusWindow->Hide();

	ProgressTo( 120 );

	errcode = SpriteEditWindow_Initialize();
	auiErr = g_c3ui->AddWindow( g_spriteEditWindow );
	Assert(auiErr == AUI_ERRCODE_OK);
	if ( auiErr != AUI_ERRCODE_OK ) return 11;

	ProgressWindow::EndProgress( g_theProgressWindow );

	return 0;
}





sint32 CivApp::InitializeSpriteEditor(CivArchive *archive)
{
	ProgressWindow::BeginProgress(
		g_theProgressWindow,
		"InitProgressWindow",
		860 );

	ProgressTo(10, g_theStringDB->GetNameStr("LOADING"));

	g_fog_toggle = TRUE;

	init_keymap();

	ProgressTo( 20 );

	SPLASH_STRING("Initializing Sprite Engine...");

	sprite_Initialize();

	ProgressTo( 140 );

	InitializeSpriteEditorUI();

	ProgressTo( 150 );

	ChatBox::Initialize();

	ProgressTo( 160 );

	GrabItem::Init();

	ProgressTo( 680 );

	if (m_dbLoaded && g_theProfileDB->IsScenario())
	{
		CleanupAppDB();
		InitializeAppDB();
	}

	ProgressTo( 690 );

	gameEventManager_Initialize();

	ProgressTo( 700 );

	events_Initialize();
	ui_events_Initialize();

	ProgressTo( 710 );

	if (!spriteEditor_Initialize(20,15))
		return FALSE;

	ProgressTo( 720 );

	if (    (archive != NULL)
         && (start_info_type_Get() != STARTINFOTYPE_NONE ||
		     save_file_version_Get() < gamefile_CurrentVersion()
            )
       )
    {
        SlicEngine::Reload(g_slic_filename);
	}
	slicengine_Get()->RunTrigger(TRIGGER_LIST_GAME_LOADED, ST_END);

	ProgressTo( 730 );

#ifdef _DEBUG
	gevmanager_Get()->Dump();
#endif

	ProgressTo( 740 );

	SPLASH_STRING("Initializing AI...");
	roboinit_Initalize(archive);
	CtpAi::Initialize();

	ProgressTo( 750 );

	SPLASH_STRING("Initializing Tile Engine...");
	tile_Initialize(archive != NULL);

	ProgressTo( 760 );

	radar_Initialize();

	ProgressTo( 770 );

	Splash::Cleanup();

	ProgressTo( 780 );

	// See comment at the matching site in InitializeGameHeadless.
	m_game = std::make_unique<Ctp2::Game>();
	m_game->NewGame(
		g_theProfileDB->GetNPlayers(),
		diffutil_GetYearFromTurn(gamesettings_Get()->GetDifficulty(), 0)
	);

	m_gameLoaded = TRUE;

	director_Get()->CatchUp();

	ProgressTo( 790 );

	g_turn->BeginNewTurn(FALSE);

	if(!g_network.IsActive()) {
		if (archive == NULL ||
			(save_file_version_Get() >= 42 &&





			(is_scenario_Get() && start_info_type_Get() != STARTINFOTYPE_NOLOCS))) {





			if(g_scenarioUsePlayerNumber == 0 && !g_turn->IsHotSeat() &&
				!g_turn->IsEmail()) {
				selitem_Get()->SetPlayerOnScreen(1);
			}
			if (director_Get())
				director_Get()->AddCopyVision();
		}
    }

	ProgressTo( 800 );

	if(g_turn->IsEmail() && archive != NULL) {
		selitem_Get()->KeyboardSelectFirstUnit();
		if(selitem_Get()->GetState() != SELECT_TYPE_LOCAL_ARMY &&
		   (player_Get(selitem_Get()->GetVisiblePlayer())->m_all_cities->Num() > 0)) {
			selitem_Get()->SetSelectCity(player_Get(selitem_Get()->GetVisiblePlayer())->m_all_cities->Access(0));
			director_Get()->AddCenterMap(player_Get(selitem_Get()->GetVisiblePlayer())->m_all_cities->Access(0).RetPos());
		}
		director_Get()->AddCenterMap(selitem_Get()->GetCurSelectPos());

		slicengine_Get()->CheckPendingResearch();
	}

	ProgressTo( 810 );

	g_scenarioUsePlayerNumber = 0;

	if(selitem_Get()) {
		selitem_Get()->Refresh();
		if(director_Get())
			director_Get()->AddCenterMap(selitem_Get()->GetCurSelectPos());
	}

	g_oldRandSeed = FALSE;


	if (g_launchIntoCheatMode) {
		ScenarioEditor::Display();







	}

	ProgressWindow::EndProgress( g_theProgressWindow );

	return 0;
}


void CivApp::CleanupGameUI(void)
{
	AttractWindow::Cleanup();
	GrabItem::Cleanup();
	greatlibrary_Cleanup();
	spnewgamescreen_Cleanup();
	spnewgametribescreen_Cleanup();
	musicscreen_Cleanup();
	initialplayscreen_Cleanup();
	scenarioscreen_Cleanup();
	optionsscreen_Cleanup();
	optionwarningscreen_Cleanup();
	graphicsscreen_Cleanup();
	gameplayoptions_Cleanup();
	soundscreen_Cleanup();
	AncientWindows_Cleanup();
	controlpanelwindow_Cleanup();

	c3windows_MakeDebugWindow(FALSE);

	messagewin_PurgeMessages();

	helptile_Cleanup();
	backgroundWin_Cleanup();
	radarwindow_Cleanup();
	c3windows_MakeStatusWindow(FALSE);

	InfoBar::Cleanup();
	InfoWindow::Cleanup();
	ScienceVictoryDialog::Cleanup();

	sci_advancescreen_Cleanup();
	infowin_Cleanup();
	infowin_Cleanup_Controls();
	ScienceManagementDialog::Cleanup();

	NationalManagementDialog::Cleanup();
	tutorialwin_Cleanup();

	workwin_Cleanup();





	ScenarioEditor::Cleanup();
	CityWindow::Cleanup();
	DiplomacyWindow::Cleanup();
	ArmyManagerWindow::Cleanup();
	TradeManager::Cleanup();
	IntelligenceWindow::Cleanup();
	UnitManager::Cleanup();
	EditQueue::Cleanup();
	ProfileEdit::Cleanup();


	DipWizard::Cleanup();
	DomesticManagementDialog::Cleanup();
	initialplayscreen_Cleanup();

	DiplomacyDetails::Cleanup();
}






void CivApp::CleanupGame(bool keepScenInfo)
{
	// Reset the Ctp2::Game container before the legacy gameinit cleanup
	// runs.  Game::Cleanup() resets only the subsystems Game owns
	// (TurnCount today); the legacy globals are torn down by
	// gameinit_Cleanup() below.
	if (m_game) {
		m_game->Cleanup();
		m_game.reset();
	}

	gameinit_CleanupMessages();
	uint32 target_milliseconds = 100000;
	uint32 used_milliseconds;
	ProcessUI(target_milliseconds, used_milliseconds);

	g_network.Cleanup();

	radarwindow_Cleanup();


	GraphicsOptions::Cleanup();

	tile_Cleanup();

	ChatBox::Cleanup();

	CleanupGameUI();

	ProcessUI(target_milliseconds, used_milliseconds);

	gameinit_Cleanup();

	ui_events_Cleanup();
	events_Cleanup();

	gameEventManager_Cleanup();

	sprite_Cleanup();

	if (g_soundManager)
		g_soundManager->DumpAllSounds();








	if(!keepScenInfo) {

		g_theProfileDB->SetIsScenario(FALSE);
		g_civPaths->ClearCurScenarioPath();
		g_civPaths->ClearCurScenarioPackPath();
		memset(scenario_name_buf(), '\0', k_SCENARIO_NAME_MAX);
		CleanupAppDB();
		InitializeAppDB();
	}

#if defined(_DEBUG) && defined(_MEMORYLOGGING) && defined(_DEBUG_MEMORY)
	DebugMemory_LeaksShow(8675209);
#endif

#ifndef _NO_GAME_WATCH

	if (!g_no_exit_action) {
		char userName[256];
		DWORD size = 256;
		userName[0] = '\0';
		GetUserName(userName, &size);

		char computerName[256];
		size = 256;
		computerName[0] = '\0';
		GetComputerName(computerName, &size);

		SYSTEMTIME localTime;
		memset(&localTime, 0, sizeof(localTime));
		GetLocalTime(&localTime);

		char stamp[1024];
		snprintf(stamp, sizeof(stamp), "Civilization III CTP - %s on %s at %d/%d/%d %d:%d:%d", userName, computerName,
		        localTime.wMonth, localTime.wDay, localTime.wYear, localTime.wHour,
		        localTime.wMinute, localTime.wSecond);

		gameWatch.EndGame(g_gameWatchID, stamp);
	}
#endif

	m_gameLoaded            = false;

	g_launchIntoCheatMode   = FALSE;
	g_god                   = FALSE;
	g_isCheatModeOn         = FALSE;

	g_c3ui->BlackScreen();
}

void CivApp::StartMessageSystem()
{
	if (!m_dbLoaded)
    {
		InitializeAppDB();
    }

    delete messagepool_Get();
    messagepool_Set(new MessagePool());
    SlicEngine::Reload(g_slic_filename);
}

void CivApp::BeginKeyboardScrolling(sint32 key)
{
	m_keyboardScrollingKey  = key;
	m_isKeyboardScrolling   = true;
}

void CivApp::StopKeyboardScrolling(sint32 key)
{
	m_keyboardScrollingKey  = 0;
	m_isKeyboardScrolling   = false;
}








void CivApp::ProcessGraphicsCallback(void)
{
    static bool s_inCallback = false;

	if (s_inCallback)   return;

	if (!tiledmap_Get())    return;
	if (!g_background)  return;
	if (!director_Get())    return;
	if (!g_background)  return;
	if (!g_c3ui)        return;

	s_inCallback = true;

	tiledmap_Get()->RestoreMixFromMap(g_background->TheSurface());
	g_background->Draw();
	g_c3ui->Process();

	if (!g_network.IsActive() || g_network.ReadyToStart())
    {
		director_Get()->Process();
    }

	s_inCallback = false;
}






sint32 CivApp::ProcessUI(const uint32 target_milliseconds, uint32 &used_milliseconds)
{
	uint32          start_time_ms   = Os::GetTicks();
	uint32          curTicks        = Os::GetTicks();
	static uint32	lastTicks       = curTicks;

	if (g_c3ui->TheMouse()) {
		if (g_c3ui->TheMouse()->IsSuspended() )
		{
			used_milliseconds = Os::GetTicks() - start_time_ms;

			if(g_runInBackground
			|| g_theProfileDB->GetValueByName("RunInBackground")
			){
				if (m_gameLoaded)
				{
					if (director_Get())
					{
						director_Get()->GarbageCollectItems();
						director_Get()->Process();
					}
				}
			}
			return 0;
		}

		bool netGameLoading = (g_network.IsNetworkLaunch() || g_network.IsActive())
			                  && !g_network.ReadyToStart();

		if (m_gameLoaded && !g_modalWindow) {
			if (!netGameLoading && ui_CheckForScroll()) {
				uint32 scroll_loop_last_tick = Os::GetTicks();
				// Use while (not do-while) so ui_CheckForScroll runs FIRST each
				// iteration. If the user moves the mouse away, we exit immediately
				// without another heavy ScrollMap + RepaintTiles pass.
				while (ui_CheckForScroll()) {
					// Frame-limit scroll loop: cap at ~20 fps (50 ms) to give
					// ScrollMap / RepaintTiles more time on unexplored terrain.
					uint32 scroll_loop_now = Os::GetTicks();
					uint32 scroll_loop_elapsed = scroll_loop_now - scroll_loop_last_tick;
					if (scroll_loop_elapsed < 50) {
						Os::Sleep(50 - scroll_loop_elapsed);
					}
					scroll_loop_last_tick = Os::GetTicks();

					tiledmap_Get()->CopyMixDirtyRects(g_background->GetDirtyList());

					// Pump SDL keyboard events so KEYUP gets processed while
					// we're in this loop. Without this, keyboard scroll state
					// never updates because CivMain's SDL_PeepEvents isn't running.
					#ifdef __AUI_USE_SDL__
					SDL_PumpEvents();
					SDL_Event sdlEvent;
					while (SDL_PeepEvents(&sdlEvent, 1, SDL_GETEVENT, SDL_KEYDOWN, SDL_KEYUP) > 0) {
						aui_sdlkbd_PushQueueEvent(sdlEvent);
					}
					#endif

					g_c3ui->Process();

					uint32 target_milliseconds=30;
					uint32 used_milliseconds;

					ProcessNet(target_milliseconds, used_milliseconds);
				}

				tiledmap_Get()->RetargetTileSurface(NULL);
				tiledmap_Get()->Refresh();
				tiledmap_Get()->InvalidateMap();
				tiledmap_Get()->ValidateMix();
			}
            else
            {
				if(tiledmap_Get()) {
					tiledmap_Get()->RestoreMixFromMap(g_background->TheSurface());
				}
				if(g_background)
					g_background->Draw();

			}

			lastTicks = curTicks;
		}
	}

	if (m_appLoaded) {
		g_c3ui->Process();
	}

	if (m_gameLoaded && !g_modalWindow && tiledmap_Get()) {

		if (director_Get())
			director_Get()->Process();
	}

	// Smoke test command dispatch
	if (g_smokeTest) {
		char cmd[256];
		if (smoketest_poll_command(cmd, sizeof(cmd))) {
			smoke_log->info("Executing command: {}", cmd);

			if (strcmp(cmd, "new_game") == 0) {
				if (m_appLoaded && !m_gameLoaded) {
					initialplayscreen_newgamePress(NULL, AUI_BUTTON_ACTION_EXECUTE, 0, NULL);
					smoketest_send_response("ok", cmd, NULL);
				} else {
					smoketest_send_response("error", cmd, "not_on_main_menu");
				}
			}
			else if (strcmp(cmd, "start_game") == 0) {
				if (m_appLoaded && !m_gameLoaded) {
					spnewgamescreen_startPress(NULL, AUI_BUTTON_ACTION_EXECUTE, 0, NULL);
					smoketest_send_response("ok", cmd, NULL);
				} else {
					smoketest_send_response("error", cmd, "not_on_new_game_screen");
				}
			}
			else if (strcmp(cmd, "end_turn") == 0) {
				if (m_gameLoaded) {
					director_Get()->AddEndTurn();
					smoketest_send_response("ok", cmd, NULL);
				} else {
					smoketest_send_response("error", cmd, "game_not_loaded");
				}
			}
			else if (strcmp(cmd, "build_city") == 0) {
				if (m_gameLoaded) {
					Player *human = NULL;
					for (sint32 p = 0; p < k_MAX_PLAYERS; p++) {
						if (player_Get(p) && player_Get(p)->IsHuman()) {
							human = player_Get(p);
							break;
						}
					}

					if (!human) {
						smoketest_send_response("error", cmd, "no_human_player");
					} else {
						DynamicArray<Army> *armies = human->GetAllArmiesList();
						bool found = false;
						for (sint32 i = 0; i < armies->Num(); i++) {
							Army army = armies->Access(i);
							if (army.IsValid() && army.CanSettle()) {
								smoke_log->info("Found settler army {} for player {}, queueing settle",
									i, (int)human->GetOwner());
								army.AccessData()->Settle();
								found = true;
								break;
							}
						}

						if (found) {
							smoketest_send_response("ok", cmd, NULL);
						} else {
							smoketest_send_response("error", cmd, "no_settler_found");
						}
					}
				} else {
					smoketest_send_response("error", cmd, "game_not_loaded");
				}
			}
			else if (strcmp(cmd, "enable_autoplay") == 0) {
				if (m_gameLoaded) {
					sint32 flipped = 0;
					for (sint32 p = 0; p < k_MAX_PLAYERS; p++) {
						if (player_Get(p)) {
							player_Get(p)->m_playerType = PLAYER_TYPE_ROBOT;
							flipped++;
						}
					}
					char detail[64];
					snprintf(detail, sizeof(detail), "flipped=%d", flipped);
					smoketest_send_response("ok", cmd, detail);
				} else {
					smoketest_send_response("error", cmd, "game_not_loaded");
				}
			}
			else if (strncmp(cmd, "set_production ", 15) == 0) {
				if (m_gameLoaded) {
					int city_idx = 0;
					char unit_keyword[64];
					if (sscanf(cmd + 15, "%d %63s", &city_idx, unit_keyword) != 2) {
						smoketest_send_response("error", cmd, "bad_args");
					} else {
						Player *human = NULL;
						for (sint32 p = 0; p < k_MAX_PLAYERS; p++) {
							if (player_Get(p) && player_Get(p)->IsHuman()) {
								human = player_Get(p);
								break;
							}
						}

						if (!human) {
							smoketest_send_response("error", cmd, "no_human_player");
						} else if (city_idx < 0 || city_idx >= human->GetAllCitiesList()->Num()) {
							smoketest_send_response("error", cmd, "bad_city_index");
						} else {
							Unit city = human->GetAllCitiesList()->Access(city_idx);
							if (!city.IsValid() || !city.GetData()->GetCityData()) {
								smoketest_send_response("error", cmd, "invalid_city");
							} else {
								sint32 unit_type = -1;
								sint32 gov_type = human->GetGovernmentType();

								if (strcmp(unit_keyword, "cheapest_military") == 0) {
									sint32 best_cost = 0x7fffffff;
									for (sint32 i = 0; i < g_theUnitDB->NumRecords(); i++) {
										const UnitRecord *rec = g_theUnitDB->Get(i, gov_type);
										if (!rec || rec->GetCantBuild()) continue;
										if (rec->GetAttack() <= 0.0) continue;
										if (!city.GetData()->GetCityData()->CanBuildUnit(i)) continue;

										sint32 cost = rec->GetShieldCost();
										if (cost > 0 && cost < best_cost) {
											best_cost = cost;
											unit_type = i;
										}
									}
								} else if (strcmp(unit_keyword, "settler") == 0) {
									sint32 best_cost = 0x7fffffff;
									for (sint32 i = 0; i < g_theUnitDB->NumRecords(); i++) {
										const UnitRecord *rec = g_theUnitDB->Get(i, gov_type);
										if (!rec || rec->GetCantBuild()) continue;
										if (!rec->GetSettle() && rec->GetNumCanSettleOn() <= 0) continue;
										if (!city.GetData()->GetCityData()->CanBuildUnit(i)) continue;

										sint32 cost = rec->GetShieldCost();
										if (cost > 0 && cost < best_cost) {
											best_cost = cost;
											unit_type = i;
										}
									}
								} else {
									unit_type = atoi(unit_keyword);
								}

								if (unit_type < 0 || unit_type >= g_theUnitDB->NumRecords()) {
									smoketest_send_response("error", cmd, "unit_not_found");
								} else if (!city.GetData()->GetCityData()->CanBuildUnit(unit_type)) {
									smoketest_send_response("error", cmd, "cannot_build_unit");
								} else {
									smoke_log->info("Setting city {} to build unit {}",
										(int)city_idx, (int)unit_type);
									city.GetData()->GetCityData()->BuildUnit(unit_type);
									smoketest_send_response("ok", cmd, NULL);
								}
							}
						}
					}
				} else {
					smoketest_send_response("error", cmd, "game_not_loaded");
				}
			}
			else if (strncmp(cmd, "enable_governor ", 16) == 0) {
				if (m_gameLoaded) {
					char target[32];
					char gov_name[32];
					if (sscanf(cmd + 16, "%31s %31s", target, gov_name) != 2) {
						smoketest_send_response("error", cmd, "bad_args");
					} else {
						sint32 gov_idx = -1;
						if (strcmp(gov_name, "production") == 0) gov_idx = 0;
						else if (strcmp(gov_name, "growth") == 0) gov_idx = 1;
						else if (strcmp(gov_name, "offense") == 0) gov_idx = 2;
						else if (strcmp(gov_name, "defense") == 0) gov_idx = 3;
						else if (strcmp(gov_name, "science") == 0) gov_idx = 4;
						else if (strcmp(gov_name, "gold") == 0) gov_idx = 5;
						else if (strcmp(gov_name, "wonders") == 0) gov_idx = 6;
						else if (strcmp(gov_name, "happiness") == 0) gov_idx = 7;
						else if (strcmp(gov_name, "default") == 0) gov_idx = 8;
						else gov_idx = atoi(gov_name);

						if (gov_idx < 0 || gov_idx >= g_theBuildListSequenceDB->NumRecords()) {
							smoketest_send_response("error", cmd, "bad_governor_type");
						} else {
							Player *human = NULL;
							for (sint32 p = 0; p < k_MAX_PLAYERS; p++) {
								if (player_Get(p) && player_Get(p)->IsHuman()) {
									human = player_Get(p);
									break;
								}
							}

							if (!human) {
								smoketest_send_response("error", cmd, "no_human_player");
							} else {
								bool all_cities = (strcmp(target, "all") == 0);
								sint32 city_idx = all_cities ? -1 : atoi(target);

								if (!all_cities && (city_idx < 0 || city_idx >= human->GetAllCitiesList()->Num())) {
									smoketest_send_response("error", cmd, "bad_city_index");
								} else {
									if (all_cities) {
										for (sint32 i = 0; i < human->GetAllCitiesList()->Num(); i++) {
											Unit city = human->GetAllCitiesList()->Access(i);
											if (city.IsValid() && city.GetData()->GetCityData()) {
												city.GetData()->GetCityData()->SetUseGovernor(true);
												city.GetData()->GetCityData()->SetBuildListSequenceIndex(gov_idx);
											}
										}
									} else {
										Unit city = human->GetAllCitiesList()->Access(city_idx);
										if (city.IsValid() && city.GetData()->GetCityData()) {
											city.GetData()->GetCityData()->SetUseGovernor(true);
											city.GetData()->GetCityData()->SetBuildListSequenceIndex(gov_idx);
										}
									}
									smoke_log->info("Enabled {} governor for {}",
										gov_name, all_cities ? "all cities" : target);
									smoketest_send_response("ok", cmd, NULL);
								}
							}
						}
					}
				} else {
					smoketest_send_response("error", cmd, "game_not_loaded");
				}
			}
			else if (strcmp(cmd, "turn_counter") == 0) {
				if (m_gameLoaded && g_turn) {
					char detail[64];
					snprintf(detail, sizeof(detail), "round=%d", g_turn->GetRound());
					smoketest_send_response("ok", cmd, detail);
				} else {
					smoketest_send_response("error", cmd, "game_not_loaded");
				}
			}
			else if (strncmp(cmd, "set_research ", 13) == 0) {
				if (m_gameLoaded) {
					const char *adv_name = cmd + 13;
					if (!adv_name[0]) {
						smoketest_send_response("error", cmd, "bad_args");
					} else {
						Player *human = NULL;
						for (sint32 p = 0; p < k_MAX_PLAYERS; p++) {
							if (player_Get(p) && player_Get(p)->IsHuman()) {
								human = player_Get(p);
								break;
							}
						}
						if (!human) {
							smoketest_send_response("error", cmd, "no_human_player");
						} else {
							StringId str_id;
							if (!g_theStringDB->GetStringID(adv_name, str_id)) {
								smoketest_send_response("error", cmd, "advance_name_not_found");
							} else {
								sint32 adv_idx = -1;
								if (!g_theAdvanceDB->GetNamedItem(str_id, adv_idx)) {
									smoke_log->info("Setting research to advance {} ({})",
										(int)adv_idx, adv_name);
									human->SetResearching(adv_idx);
									smoketest_send_response("ok", cmd, NULL);
								} else {
									smoketest_send_response("error", cmd, "advance_not_in_db");
								}
							}
						}
					}
				} else {
					smoketest_send_response("error", cmd, "game_not_loaded");
				}
			}
			else if (strncmp(cmd, "diplomacy_status ", 17) == 0) {
				if (m_gameLoaded) {
					int target_player = atoi(cmd + 17);
					if (target_player < 0 || target_player >= k_MAX_PLAYERS) {
						smoketest_send_response("error", cmd, "bad_player_index");
					} else {
						Player *human = NULL;
						for (sint32 p = 0; p < k_MAX_PLAYERS; p++) {
							if (player_Get(p) && player_Get(p)->IsHuman()) {
								human = player_Get(p);
								break;
							}
						}
						if (!human) {
							smoketest_send_response("error", cmd, "no_human_player");
						} else if (!player_Get(target_player)) {
							smoketest_send_response("error", cmd, "player_not_active");
						} else {
							DIPLOMATIC_STATE state = human->GetDiplomaticState(target_player);
							const char *state_str = "unknown";
							switch(state) {
								case DIPLOMATIC_STATE_WAR:       state_str = "war"; break;
								case DIPLOMATIC_STATE_CEASEFIRE: state_str = "ceasefire"; break;
								case DIPLOMATIC_STATE_NEUTRAL:   state_str = "neutral"; break;
								case DIPLOMATIC_STATE_ALLIED:    state_str = "allied"; break;
							}
							char detail[64];
							snprintf(detail, sizeof(detail), "player=%d,state=%s", target_player, state_str);
							smoketest_send_response("ok", cmd, detail);
						}
					}
				} else {
					smoketest_send_response("error", cmd, "game_not_loaded");
				}
			}
			else if (strncmp(cmd, "save_game ", 10) == 0) {
				if (m_gameLoaded) {
					const char *path = cmd + 10;
					if (!path[0]) {
						smoketest_send_response("error", cmd, "bad_args");
					} else {
						smoke_log->info("Saving game to {}", path);
						GameFile::SaveGame(path, NULL);
						smoketest_send_response("ok", cmd, NULL);
					}
				} else {
					smoketest_send_response("error", cmd, "game_not_loaded");
				}
			}
			else if (strncmp(cmd, "load_game ", 10) == 0) {
				const char *path = cmd + 10;
				if (!path[0]) {
					smoketest_send_response("error", cmd, "bad_args");
				} else {
					smoke_log->info("Loading game from {}", path);
					GameFile::RestoreGame(path);
					smoketest_send_response("ok", cmd, NULL);
				}
			}
			else if (strncmp(cmd, "screenshot ", 11) == 0) {
				const char *path = cmd + 11;
				if (!path[0]) {
					smoketest_send_response("error", cmd, "bad_args");
				} else {
#ifdef USE_SDL
					aui_SDLSurface *sdlSurf = static_cast<aui_SDLSurface*>(g_c3ui->Primary());
					if (sdlSurf && sdlSurf->DDS()) {
						if (SDL_SaveBMP(sdlSurf->DDS(), path) == 0) {
							smoke_log->info("Screenshot saved to {}", path);
							smoketest_send_response("ok", cmd, NULL);
						} else {
							smoketest_send_response("error", cmd, "sdl_save_failed");
						}
					} else {
						smoketest_send_response("error", cmd, "no_surface");
					}
#else
					smoketest_send_response("error", cmd, "not_sdl");
#endif
				}
			}
			else if (strncmp(cmd, "move_unit ", 10) == 0) {
				if (m_gameLoaded) {
					int city_idx = 0, dx = 0, dy = 0;
					if (sscanf(cmd + 10, "%d %d %d", &city_idx, &dx, &dy) != 3) {
						smoketest_send_response("error", cmd, "bad_args");
					} else {
						Player *human = NULL;
						for (sint32 p = 0; p < k_MAX_PLAYERS; p++) {
							if (player_Get(p) && player_Get(p)->IsHuman()) {
								human = player_Get(p);
								break;
							}
						}
						if (!human) {
							smoketest_send_response("error", cmd, "no_human_player");
						} else if (city_idx < 0 || city_idx >= human->GetAllCitiesList()->Num()) {
							smoketest_send_response("error", cmd, "bad_city_index");
						} else {
							Unit city = human->GetAllCitiesList()->Access(city_idx);
							if (!city.IsValid()) {
								smoketest_send_response("error", cmd, "invalid_city");
							} else {
								MapPoint city_pos;
								city.GetPos(city_pos);
								MapPoint dest(city_pos.x + dx, city_pos.y + dy);

								Cell *cell = world_Get()->GetCell(city_pos);
									bool moved = false;
								if (cell) {
									for (sint32 i = 0; i < cell->GetNumUnits(); i++) {
										Unit u = cell->AccessUnit(i);
										if (u.IsValid() && !u.IsCity() && u.GetOwner() == human->GetOwner()) {
											Army army = u.GetArmy();
											if (army.IsValid()) {
												army.AddOrders(UNIT_ORDER_MOVE_TO, dest);
												smoke_log->info("Moving unit from ({},{}) to ({},{})",
												(int)city_pos.x, (int)city_pos.y, (int)dest.x, (int)dest.y);
												moved = true;
												break;
											}
										}
									}
								}
								if (moved) {
									smoketest_send_response("ok", cmd, NULL);
								} else {
									smoketest_send_response("error", cmd, "no_movable_unit");
								}
							}
						}
					}
				} else {
					smoketest_send_response("error", cmd, "game_not_loaded");
				}
			}
			else if (strncmp(cmd, "auto_explore ", 13) == 0) {
				// Put a unit (by city-relative-slot, same convention as
				// move_unit) on auto-explore.  The army receives ORDER_EXPLORE;
				// the per-turn hook keeps re-picking new targets.
				if (m_gameLoaded) {
					int city_idx = 0;
					if (sscanf(cmd + 13, "%d", &city_idx) != 1) {
						smoketest_send_response("error", cmd, "bad_args");
					} else {
						Player *human = NULL;
						for (sint32 p = 0; p < k_MAX_PLAYERS; p++) {
							if (player_Get(p) && player_Get(p)->IsHuman()) {
								human = player_Get(p);
								break;
							}
						}
						if (!human) {
							smoketest_send_response("error", cmd, "no_human_player");
						} else if (city_idx < 0 || city_idx >= human->GetAllCitiesList()->Num()) {
							smoketest_send_response("error", cmd, "bad_city_index");
						} else {
							Unit city = human->GetAllCitiesList()->Access(city_idx);
							MapPoint city_pos;
							city.GetPos(city_pos);
							Cell *cell = world_Get()->GetCell(city_pos);
								bool issued = false;
							if (cell) {
								for (sint32 i = 0; i < cell->GetNumUnits(); i++) {
									Unit u = cell->AccessUnit(i);
									if (u.IsValid() && !u.IsCity() && u.GetOwner() == human->GetOwner()) {
										Army army = u.GetArmy();
										if (army.IsValid()) {
											gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
											                       GEV_ExploreOrder,
											                       GEA_Army, army,
											                       GEA_End);
											smoke_log->info("Auto-explore queued for army at ({},{})",
											        (int)city_pos.x, (int)city_pos.y);
											issued = true;
											break;
										}
									}
								}
							}
							smoketest_send_response(issued ? "ok" : "error", cmd,
							                        issued ? NULL : "no_movable_unit");
						}
					}
				} else {
					smoketest_send_response("error", cmd, "game_not_loaded");
				}
			}
			else if (strcmp(cmd, "list_visible_units") == 0) {
				if (m_gameLoaded) {
					Player *human = NULL;
					for (sint32 p = 0; p < k_MAX_PLAYERS; p++) {
						if (player_Get(p) && player_Get(p)->IsHuman()) {
							human = player_Get(p);
							break;
						}
					}
					if (!human) {
						smoketest_send_response("error", cmd, "no_human_player");
					} else {
						sint32 vis_player = selitem_Get()->GetVisiblePlayer();
						char detail[512];
						detail[0] = '\0';
						int count = 0;
						for (sint32 p = 0; p < k_MAX_PLAYERS && count < 10; p++) {
							if (!player_Get(p)) continue;
							for (sint32 i = 0; i < player_Get(p)->m_all_units->Num() && count < 10; i++) {
								Unit u = player_Get(p)->m_all_units->Access(i);
								if (u.IsValid() && (u.GetVisibility() & (1 << vis_player))) {
									MapPoint pos;
									u.GetPos(pos);
									char entry[64];
									snprintf(entry, sizeof(entry), "%s%d@(%d,%d)",
										count > 0 ? "," : "",
										p, pos.x, pos.y);
									if (strlen(detail) + strlen(entry) < sizeof(detail) - 1) {
										strcat(detail, entry);
										count++;
									}
								}
							}
						}
						char full_detail[576];
						snprintf(full_detail, sizeof(full_detail), "count=%d,%s", count, detail);
						smoketest_send_response("ok", cmd, full_detail);
					}
				} else {
					smoketest_send_response("error", cmd, "game_not_loaded");
				}
			}
			else if (strcmp(cmd, "quit") == 0) {
				smoketest_send_response("ok", cmd, NULL);
				// Give socket thread time to send response before we tear down
				Os::Sleep(100);
				ExitGame();
			}
			else {
				smoketest_send_response("error", cmd, "unknown_command");
			}
		}
	}

	return 0;
}

sint32 CivApp::ProcessAI()
{
	if((g_network.IsActive() || g_network.IsNetworkLaunch()) &&
		!g_network.ReadyToStart())
		return 0;

	if(victorywin_IsOnScreen())
		return 0;

	if (g_c3ui->TheMouse()) {
		if( g_c3ui->TheMouse()->IsSuspended()
		&& !g_runInBackground
		&& !g_theProfileDB->GetValueByName("RunInBackground")
		){
			return 0;
		}
	}
	return 1;
}

sint32 CivApp::ProcessRobot(const uint32 target_milliseconds, uint32 &used_milliseconds)
{
	if((g_network.IsActive() || g_network.IsNetworkLaunch()) &&
		!g_network.ReadyToStart())
		return 0;

	if(victorywin_IsOnScreen())
		return 0;

	uint32 const    start_time_ms = Os::GetTicks();

	if (g_c3ui->TheMouse())
	{
		if (g_c3ui->TheMouse()->IsSuspended() && !g_runInBackground && !g_theProfileDB->GetValueByName("RunInBackground"))
		{
			// Probably there was something here.
		}
	}

	used_milliseconds = Os::GetTicks() - start_time_ms;
	return 0;
}

sint32 CivApp::ProcessNet(const uint32 target_milliseconds, uint32 &used_milliseconds)
{
	uint32 const        start_time = Os::GetTicks();

	if (m_gameLoaded)
	{
		g_network.Process();
	}

	used_milliseconds   = Os::GetTicks() - start_time;
	return 0;
}

sint32 CivApp::ProcessSLIC(void)
{
	if (!slicengine_Get())
		return 0;

	slicengine_Get()->ProcessUITriggers();

	static time_t   lastRanSlicTimers   = 0;
    time_t          now                 = time(0);
	if (now > lastRanSlicTimers + slicengine_Get()->GetTimerGranularity())
    {
		slicengine_Get()->RunTimerTriggers();
        /// @todo Check lastRanSlicTimers = now;
	}

	if (slicengine_Get()->WaitingForLoad())
    {
		main_RestoreGame(slicengine_Get()->GetLoadName());
	}

	return 0;
}

sint32 CivApp::ProcessProfile(void)
{
    uint32 target_milliseconds = (g_no_timeslice) ? 10000000 : 30;
	uint32 used_milliseconds;

	ProcessNet(target_milliseconds, used_milliseconds);

	ProcessUI(target_milliseconds, used_milliseconds);


	ProcessSLIC();

	ProcessRobot(target_milliseconds, used_milliseconds);

	if (g_soundManager)
		g_soundManager->Process(target_milliseconds, used_milliseconds);

    return 0;
}

sint32 CivApp::Process(void)
{
#if defined(_DEBUG) && defined(WIN32)
	if(g_tempLeakCheck) {
		_CrtMemState new_state;
		_CrtMemCheckpoint(&new_state);

		g_allocatedAtStart = new_state.lSizes[1];
	}
#endif


	if (NetConsole *nc = netconsole_Get()) {
		static uint32 last_tick = 0;

		if (Os::GetTicks() > last_tick + 250)
		{
			nc->Idle();
			last_tick = Os::GetTicks();
		}
	}

	// Sleep problem if the game is supposed to run in background
	if (m_inBackground)
	{
		Os::Sleep(50);
		return 0;
	}

	if (g_use_profile_process) {
		ProcessProfile();
		return 0;
	}

    uint32 target_milliseconds = (g_no_timeslice) ? 10000000 : 30;
	uint32 used_milliseconds;

	ProcessNet(target_milliseconds, used_milliseconds);

	ProcessUI(target_milliseconds, used_milliseconds);

	if(AttractWindow *aw = attractwindow_Get())
		aw->AppIdle();


	ProcessSLIC();

	ProcessRobot(target_milliseconds, used_milliseconds);

	if (g_soundManager)
		g_soundManager->Process(target_milliseconds, used_milliseconds);

	if(gevmanager_Get())
		gevmanager_Get()->Process();


	if (m_gameLoaded && g_savedGameRequest && selitem_Get())
    {
        Player *    p = player_Get(selitem_Get()->GetCurPlayer());
		if(p && !p->IsRobot()
		|| selitem_Get()->GetVisiblePlayer() == selitem_Get()->GetCurPlayer())
        {
			if (director_Get())
				director_Get()->CatchUp();

			if (g_launchIntoCheatMode)
				gamesettings_Get()->SetKeepScore(TRUE);

    		GameFile::SaveGame(g_savedGameRequest->pathName, g_savedGameRequest);

			if (g_launchIntoCheatMode)
				gamesettings_Get()->SetKeepScore(FALSE);

			allocated::clear(g_savedGameRequest);
		}
	}

    return 0;
}

sint32 CivApp::StartGame(void)
{
	return InitializeGame(NULL);
}

sint32 CivApp::InitializeGameHeadless(CivArchive *archive)
{
	civapp_log->info("InitializeGameHeadless: started (archive={})",
	          archive ? "load" : "new");

	civapp_log->debug("calling sprite_Initialize()");
	sprite_Initialize();

	civapp_log->debug("calling gameEventManager_Initialize()");
	gameEventManager_Initialize();

	civapp_log->debug("calling gevmanager_Get()->Pause()");
	gevmanager_Get()->Pause();

	civapp_log->debug("calling events_Initialize()");
	events_Initialize();

	civapp_log->debug("calling gameinit_Initialize(archive={})", (void*)archive);
	if (!gameinit_Initialize(-1, -1, archive)) {
		gevmanager_Get()->Resume();
		civapp_log->error("InitializeGameHeadless: gameinit_Initialize failed");
		return FALSE;
	}
	civapp_log->debug("gameinit_Initialize returned OK");

	// Create TiledMap without UI window (normally done in tile_Initialize).
	// Skip LoadTileset entirely — this function is the headless-only path,
	// and LoadTileset needs g_ImageMapPF which is initialized in
	// InitializeImageMaps (UI-only).  Map data is sufficient for logic;
	// tile graphics are not needed.
	if (world_Get() && !tiledmap_Get()) {
		civapp_log->debug("creating headless TiledMap (world={}x{})",
		           world_Get()->GetXWidth(), world_Get()->GetYHeight());
		MapPoint mapsize(world_Get()->GetXWidth(), world_Get()->GetYHeight());
		tiledmap_Set(new TiledMap(mapsize));
	}

	// See comment at the matching site in InitializeGameHeadless.
	m_game = std::make_unique<Ctp2::Game>();
	m_game->NewGame(
		g_theProfileDB->GetNPlayers(),
		diffutil_GetYearFromTurn(gamesettings_Get()->GetDifficulty(), 0)
	);

	m_gameLoaded = TRUE;

	civapp_log->debug("calling gevmanager_Get()->Resume + Process");
	gevmanager_Get()->Resume();
	gevmanager_Get()->Process();

	// Initialize AI subsystems (pathfinder, governors, scheduler, diplomat).
	// The interactive game does this in InitializeGame() via roboinit_Initalize
	// and CtpAi::Initialize(); the headless path must do the same or the AI
	// never makes decisions (settlers never settle, score stays flat).
	civapp_log->debug("calling roboinit_Initalize / CtpAi::Initialize");
	roboinit_Initalize(NULL);
	CtpAi::Initialize();

	civapp_log->info("InitializeGameHeadless: done (game loaded)");
	return 0;
}

sint32 CivApp::StartSpriteEditor(void)
{
	return InitializeSpriteEditor(NULL);
}







sint32 CivApp::EndGame(void)
{
	if (m_gameLoaded) {
		CleanupGame(false);
		StartMessageSystem();
	}

	AUI_ERRCODE errcode = initialplayscreen_Initialize();
	Assert(errcode == AUI_ERRCODE_OK);

	return initialplayscreen_displayMyWindow();
}

sint32 CivApp::LoadSavedGame(MBCHAR const * name)
{

	ProgressWindow::BeginProgress(
		g_theProgressWindow,
		"InitProgressWindow",
		1300 );

	ProgressTo( 10, g_theStringDB->GetNameStr("LOADING") );

	FILE * fin = fopen(name, "r");
	if (fin == NULL) {
		c3errors_ErrorDialog("Load save game", "Could not open %s", name);
		return 0;
	}
	fclose(fin);

	ProgressTo( 20 );

	if (m_gameLoaded) {
		CleanupGame(true);

		ProgressTo( 30 );

		CleanupAppDB();

		ProgressTo( 550 );

		InitializeAppDB();

		ProgressTo( 560 );

		greatlibrary_Cleanup();
		GreatLibrary::Initialize_Great_Library_Data();

		ProgressTo( 570 );

		StartMessageSystem();
	}

	ProgressTo( 1280 );

	GameFile::RestoreGame(name);

	ProgressTo( 1290 );

	tiledmap_Get()->InvalidateMap();

	ProgressTo( 1300 );

	if (!g_turn->IsHotSeat())
	{
		selitem_Get()->NextUnmovedUnit(TRUE, TRUE);
	}

	ProgressWindow::EndProgress( g_theProgressWindow );

	return 0;
}

sint32 CivApp::LoadSavedGameMap(MBCHAR const * name)
{
	FILE * fin = fopen(name, "r");

    if (fin)
    {
    	fclose(fin);
	    GameMapFile::RestoreGameMap(name);
	    tiledmap_Get()->InvalidateMap();
	    selitem_Get()->NextUnmovedUnit(TRUE, FALSE);
    }
    else
    {
		c3errors_ErrorDialog("Load save game map", "Could not open %s", name);
	}

	return 0;
}


sint32 CivApp::LoadScenarioGame(MBCHAR const * file)
{
	if (m_gameLoaded) {
		CleanupGame(true);
		StartMessageSystem();
	}

	GameFile::RestoreScenarioGame(file);

	if (tiledmap_Get())
		tiledmap_Get()->InvalidateMap();

	selitem_Get()->NextUnmovedUnit(TRUE, FALSE);

	return 0;
}

sint32 CivApp::RestartGame(void)
{
	if (m_gameLoaded) {
		CleanupGame(true);
		StartMessageSystem();
	}

	return StartGame();
}

sint32 CivApp::RestartGameSameMap(void)
{
	Assert(rand_ptr());
	g_oldRandSeed = rand_ptr() ? civrand().GetSeed() : 0;

	if (m_gameLoaded)
	{
		CleanupGame(true);
		StartMessageSystem();
	}

	if (g_theProfileDB->IsScenario())
	{
		spnewgamescreen_scenarioExitCallback(NULL, AUI_BUTTON_ACTION_EXECUTE, 0, NULL);
		return 0;
	}
	else
	{
		return StartGame();
	}
}

sint32 CivApp::QuitToSPShell(void)
{
	if (m_gameLoaded)
	{
		CleanupGame(false);
		StartMessageSystem();
	}

	// We've removed that so go to main menu instead
	// (Change by JJB)
	return initialplayscreen_displayMyWindow();
}

sint32 CivApp::QuitToLobby(void)
{
	if (m_gameLoaded) {
		CleanupGame(false);
		StartMessageSystem();
	}

	return NetShell::Enter( k_NS_FLAGS_RETURN );
}

void CivApp::QuitGame(void)
{
	if (m_gameLoaded)
		CleanupGame(true);

	CleanupApp();
}


void CivApp::AutoSave(sint32 player, bool isQuickSave)
{
	if ((g_network.IsActive() && !g_network.IsHost()) || g_network.IsNetworkLaunch())
		return;

	MBCHAR const *  autosaveItem    = (isQuickSave) ? "QUICKSAVE_NAME" : "AUTOSAVE_NAME";
	MBCHAR const *  autosaveName    = g_theStringDB->GetNameStr(autosaveItem);

	MBCHAR			leaderName[k_MAX_NAME_LEN];
	strncpy(leaderName, g_theProfileDB->GetLeaderName(), SAVE_LEADER_NAME_SIZE);
	leaderName[SAVE_LEADER_NAME_SIZE] = '\0';
	c3files_StripSpaces(leaderName);

	MBCHAR			filename[_MAX_PATH];
	snprintf(filename, sizeof(filename), "%s-%s", autosaveName, leaderName);

	C3SAVEDIR       dir = (g_network.IsActive()) ? C3SAVEDIR_MP : C3SAVEDIR_GAME;

	MBCHAR			path[_MAX_PATH];
	g_civPaths->GetSavePath(dir, path);

	MBCHAR			fullpath[_MAX_PATH];
	snprintf(fullpath, sizeof(fullpath), "%s%s%s", path, FILE_SEP, leaderName);

	if (c3files_PathIsValid(fullpath) || c3files_CreateDirectory(fullpath))
	{
		strcat(fullpath, FILE_SEP);
		strcat(fullpath, filename);

		is_scenario_Set(FALSE);
		// Route through the public SaveGame dispatcher so the JSON
		// default (g_useJsonSave) applies to autosave too.  The UTF-8
		// cleanliness bug that previously kept this on the binary path
		// is fixed by utf8_safe() in json_save.cpp.
		GameFile::SaveGame(fullpath, NULL);
	}
	else
	{
		Assert(false);
	}
}

void CivApp::RestoreAutoSave(sint32 player)
{
	MBCHAR		filename[_MAX_PATH];
	snprintf(filename, sizeof(filename), "auto%d.sav", player);

	g_c3ui->AddAction(new LoadSaveGameAction(filename));
}


void CivApp::PostStartGameAction(void)
{
	g_c3ui->AddAction(new StartGameAction());
}

void CivApp::PostSpriteTestAction(void)
{
	g_c3ui->AddAction(new SpriteTestAction());
}

void CivApp::PostLoadSaveGameAction(MBCHAR const * name)
{
	g_c3ui->AddAction(new LoadSaveGameAction(name));
}

void CivApp::PostLoadQuickSaveAction(sint32 player)
{
	/// @todo Check g_network.IsHost? See AutoSave. Otherwise, the dir assignment
	///       becomes constant.
	if (g_network.IsActive())
	{
		return;
	}

	MBCHAR			leaderName[SAVE_LEADER_NAME_SIZE + 1];
	strncpy(leaderName, g_theProfileDB->GetLeaderName(), SAVE_LEADER_NAME_SIZE);
	leaderName[SAVE_LEADER_NAME_SIZE] = '\0';
	c3files_StripSpaces(leaderName);

	MBCHAR			filename[_MAX_PATH];
	snprintf(filename, sizeof(filename), "%s-%s", g_theStringDB->GetNameStr("QUICKSAVE_NAME"), leaderName);

	C3SAVEDIR       dir = (g_network.IsActive()) ? C3SAVEDIR_MP : C3SAVEDIR_GAME;

	MBCHAR			path[_MAX_PATH];
	g_civPaths->GetSavePath(dir, path);

	MBCHAR			fullpath[_MAX_PATH];
	snprintf(fullpath, sizeof(fullpath), "%s%s%s", path, FILE_SEP, leaderName);

	if (c3files_PathIsValid(fullpath) || c3files_CreateDirectory(fullpath))
	{
		strcat(fullpath, FILE_SEP);
		strcat(fullpath, filename);

		FILE * f = fopen(fullpath, "r");
		if (f)
		{
			fclose(f);
			PostLoadSaveGameAction(fullpath);
		}
	}
	else
	{
		Assert(FALSE);
	}
}

#if 0   // never used
void CivApp::PostLoadSaveGameMapAction(MBCHAR const * name)
{
	g_c3ui->AddAction(new LoadSaveGameMapAction(name));
}
#endif

void CivApp::PostRestartGameAction(void)
{
	g_c3ui->AddAction(new RestartGameAction());
}

void CivApp::PostRestartGameSameMapAction(void)
{
	Player * p = player_Get(selitem_Get()->GetVisiblePlayer());

	if (p && g_theProfileDB)
	{
		g_theProfileDB->SetLeaderName(p->GetLeaderName());
		g_theProfileDB->SetCivIndex(p->GetCivilisation()->GetCivilisation());
	}

	g_c3ui->AddAction(new RestartGameSameMapAction());
}

void CivApp::PostQuitToSPShellAction(void)
{
	g_c3ui->AddAction(new QuitToSPShellAction());
}

void CivApp::PostQuitToLobbyAction(void)
{
	g_c3ui->AddAction(new QuitToLobbyAction());
}

void CivApp::PostEndGameAction(void)
{
	g_c3ui->AddAction(new EndGameAction());
}

void CivApp::PostLoadScenarioGameAction(MBCHAR const * filename)
{
	g_c3ui->AddAction(new LoadScenarioGameAction(filename));
}

void StartGameAction::Execute(aui_Control *control, uint32 action, uint32 data )
{
	g_civApp->StartGame();
}

void SpriteTestAction::Execute(aui_Control *control, uint32 action, uint32 data )
{
	g_civApp->StartSpriteEditor();
}

void LoadSaveGameAction::Execute(aui_Control *control, uint32 action, uint32 data )
{
	g_civApp->LoadSavedGame(m_filename);
}

#if 0   // never used
void LoadSaveGameMapAction::Execute(aui_Control *control, uint32 action, uint32 data )
{
	g_civApp->LoadSavedGameMap(m_filename);
}
#endif
void RestartGameAction::Execute(aui_Control *control, uint32 action, uint32 data )
{
	g_civApp->RestartGame();
}
void RestartGameSameMapAction::Execute(aui_Control *control, uint32 action, uint32 data )
{
	g_civApp->RestartGameSameMap();
}
void QuitToSPShellAction::Execute(aui_Control *control, uint32 action, uint32 data )
{
	g_civApp->QuitToSPShell();
}
void QuitToLobbyAction::Execute(aui_Control *control, uint32 action, uint32 data )
{
	g_civApp->QuitToLobby();
}
void EndGameAction::Execute(aui_Control *control, uint32 action, uint32 data)
{
	g_civApp->EndGame();
}
void LoadScenarioGameAction::Execute(aui_Control *control, uint32 action, uint32 data)
{
	g_civApp->LoadScenarioGame(m_filename);
}




void InitializeGreatLibrary()
{
    delete g_GreatLibPF;
    g_GreatLibPF = new ProjectFile();
    AddSearchDirectories(g_GreatLibPF, C3DIR_GL, "gl.zfs");
}

void InitializeSoundPF()
{
    delete g_SoundPF;
    g_SoundPF = new ProjectFile();
    AddSearchDirectories(g_SoundPF, C3DIR_SOUNDS, "sound.zfs");
}

void InitializeImageMaps()
{
    delete g_ImageMapPF;
    g_ImageMapPF = new ProjectFile();

    if (g_c3ui->PixelFormat() == AUI_SURFACE_PIXELFORMAT_555)
    {
        AddSearchPacks(g_ImageMapPF, C3DIR_PATTERNS, "pat555.zfs");
        AddSearchPacks(g_ImageMapPF, C3DIR_PICTURES, "pic555.zfs");
    }
    else
    {
        AddSearchPacks(g_ImageMapPF, C3DIR_PATTERNS, "pat565.zfs");
        AddSearchPacks(g_ImageMapPF, C3DIR_PICTURES, "pic565.zfs");
    }
}
