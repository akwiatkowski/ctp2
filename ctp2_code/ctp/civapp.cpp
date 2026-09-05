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
#include "gs/core/game_observer.h"     // gameobservers_Get() init in InitializeEngine
#include "gs/core/game_observer_registration.h"  // RegisterUIGameObserver + RegisterUIPlayerView
#include "gs/gameobj/MovePath.h"                // army_QueueMovePath (move_unit console cmd)
#include "robot/utility/RoboInit.h"             // roboinit_Initalize
#include "ai/ctpai.h"                           // CtpAi::Initialize
#include "ui/aui_ctp2/ui_events.h"     // ui_events_Initialize / _Cleanup

#ifdef __AUI_USE_SDL__
#include "ui/aui_sdl/aui_sdlcompat.h"
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
#include "gs/gameobj/MessagePool.h"                // m_game->GetMessagesPtr()
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
#include "gs/database/profileDB.h"                  // profiledb_Get()
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
#include "sound/soundmanager.h"               // soundmgr_Get()
#include "SoundRecord.h"
#include "ui/interface/soundscreen.h"
#include "SpecialAttackInfoRecord.h"
#include "SpecialEffectRecord.h"
#include "ui/interface/scenarioeditor.h"
#include "ui/interface/splash.h"						// g_splash_old
#include "ui/interface/spnewgametribescreen.h"
#include "ui/interface/spnewgamewindow.h"
#include "test/smoketest_server.h"
#include "ctp/game_controller.h"          // game_controller::Dispatch
#include "ui/interface/spriteeditor.h"
#include "SpriteRecord.h"
#include "ui/interface/statswindow.h"
#include "ui/aui_ctp2/statuswindow.h"
#include "StrategyRecord.h"
#include "gs/database/StrDB.h"
#include <string>                       // std::string
#include <vector>                       // std::vector (render_map_player labels)
#include "gs/database/thronedb.h"                   // g_theThroneDB
#include "TerrainImprovementRecord.h"
#include "TerrainRecord.h"
#include "gs/gameobj/terrainutil.h"
#include "gfx/tilesys/tiledmap.h"
#include "ui/interface/trademanager.h"
#include "gs/utility/TurnCnt.h"                    // turn_Get()
#include "ui/interface/tutorialwin.h"
#include "gs/fileio/gamefile.h"
#ifdef USE_SDL
#include "ui/aui_sdl/aui_sdl.h"          // aui_SDL::Renderer/ScreenTexture (presented-frame readback)
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

#include <thread>
#include <chrono>

extern OzoneDatabase            *g_theUVDB;
extern MovieDB                  *g_theVictoryMovieDB;
extern FilenameDB               *g_theMessageIconFileDB;
extern PlayListDB               *g_thePlayListDB;
extern StatsWindow          *g_statsWindow;
extern SpriteEditWindow     *g_spriteEditWindow;
extern aui_Surface          *g_sharedSurface;
extern sint32               g_modalWindow;

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

ProjectFile *g_GreatLibPF = nullptr;
ProjectFile *g_ImageMapPF = nullptr;
ProjectFile *g_SoundPF = nullptr;

sint32 g_logCrashes = 1;

sint32 g_oldRandSeed = FALSE;

ProgressWindow *g_theProgressWindow = nullptr;

static bool g_headlessMode = false;

bool is_headless()      { return g_headlessMode; }
void set_headless(bool v)   { g_headlessMode = v; }

// Null-safe wrapper around ProgressTo.  In headless
// mode the global stays NULL (ProgressWindow::BeginProgress short-circuits),
// so the unconditional `ProgressTo(...)` callsites
// peppered through InitializeAppDB would UB-fault on member-call entry.  Use
// this helper at every loading-progress call site.
static inline void ProgressTo(sint32 val, MBCHAR const * msg = nullptr)
{
	if (g_theProgressWindow) g_theProgressWindow->StartCountingTo(val, msg);
}

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

    for (int i = 0; civpaths_Get()->FindPath(a_Type, i++, path); )
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

    for (int i = 0; civpaths_Get()->FindPath(a_Type, i++, path); )
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
// Globals    : profiledb_Get():   user preferences (read)
//              civpaths_Get():       (updated)
//
// Returns    : -
//
// Remark(s)  : The top directories (ctp2_data-style) are read from a
//              semicolon-separated string 'Rulesets' in userprofile.txt.
//
//----------------------------------------------------------------------------
void InitDataIncludePath()
{
	MBCHAR                  ruleSets[MAX_PATH];
	strlcpy(ruleSets, profiledb_Get()->GetRuleSets(), sizeof(ruleSets));

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
		civpaths_Get()->InsertExtraDataPath(*p);
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
// Globals    : profiledb_Get()  : user preferences (read)
//
// Returns    : -
//
// Remark(s)  : - When the user preference is invalid, color set 0 is used.
//              - The existence of the file is not checked.
//
//----------------------------------------------------------------------------
void SelectColorSet()
{
	Assert(profiledb_Get());
	ColorSet::Initialize(profiledb_Get()->GetValueByName("ColorSet"));
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
	uint32 GetTicks()
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
    m_game                  (std::make_unique<Ctp2::Game>())
{
    // Game container is constructed eagerly so accessor trampolines
    // (e.g. pollution_Get → m_game->GetPollutionPtr()) work for the
    // entire CivApp lifetime, including the gameinit_Initialize window
    // that runs before NewGame.  The Game starts empty (all subsystems
    // null); NewGame populates them.
}

// Out-of-line dtor — Ctp2::Game is forward-declared in civapp.h, so
// the unique_ptr's deleter needs to see the full type here.
CivApp::~CivApp() = default;

void CivApp::InitializeAppUI()
{
	civapp_log->info("InitializeAppUI: called");
#if CTP2_ENABLE_NETWORKING
	// Set CTP2 specific data for the Anet library (multiplayer only)
	NETFunc::GameType	= GAMEID;				// CTP2 game id for Anet
	NETFunc::DllPath	= "dll" FILE_SEP "net";	// Anet DLLs are in dll\net (relative to executable)
#endif

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
		if (soundmgr_Get())
		{
			soundmgr_Get()->EnableMusic();
			soundmgr_Get()->PickNextTrack();
			soundmgr_Get()->StartMusic();
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
	StringId str1;
	StringId str2;

	if(strings1->GetDetailsEven(str1)) {
		if(rec2->GetDetailsEx0(strings2)) {
			if(strings2->GetDetailsEven(str2)) {
				fprintf(dipFile, "%s %s (%s/%s)\n\n",
						stringdb_Get()->GetNameStr(str1), stringdb_Get()->GetNameStr(str2),
						stringdb_Get()->GetIdStr(str1), stringdb_Get()->GetIdStr(str2));
			}
		}
		if(rec2->GetDetailsEx1(strings2)) {
			if(strings2->GetDetailsEven(str2)) {
				fprintf(dipFile, "%s %s (%s/%s)\n\n",
						stringdb_Get()->GetNameStr(str1), stringdb_Get()->GetNameStr(str2),
						stringdb_Get()->GetIdStr(str1), stringdb_Get()->GetIdStr(str2));
			}
		}
		if(rec2->GetDetailsEx2(strings2)) {
			if(strings2->GetDetailsEven(str2)) {
				fprintf(dipFile, "%s %s (%s/%s)\n\n",
						stringdb_Get()->GetNameStr(str1), stringdb_Get()->GetNameStr(str2),
						stringdb_Get()->GetIdStr(str1), stringdb_Get()->GetIdStr(str2));
			}
		}
		if(rec2->GetDetailsEx3(strings2)) {
			if(strings2->GetDetailsEven(str2)) {
				fprintf(dipFile, "%s %s (%s/%s)\n\n",
						stringdb_Get()->GetNameStr(str1), stringdb_Get()->GetNameStr(str2),
						stringdb_Get()->GetIdStr(str1), stringdb_Get()->GetIdStr(str2));
			}
		}
		if(rec2->GetDetailsEx4(strings2)) {
			if(strings2->GetDetailsEven(str2)) {
				fprintf(dipFile, "%s %s (%s/%s)\n\n",
						stringdb_Get()->GetNameStr(str1), stringdb_Get()->GetNameStr(str2),
						stringdb_Get()->GetIdStr(str1), stringdb_Get()->GetIdStr(str2));
			}
		}
	}
}
#endif

/// Initialize databases
bool CivApp::InitializeAppDB()
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
	stringdb_Set(new StringDB());
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
    if (!stringdb_Get()->Parse(g_stringdb_filename))
    {
        return false;
    }

	// Has to be done after the initialization of the string database
	ProgressTo(10, stringdb_Get()->GetNameStr("LOADING"));

    // Fill the databases from file

	civapp_log->info("InitializeAppDB: Parsing SoundDB");
	if (!g_theSoundDB->Parse(C3DIR_GAMEDATA, g_sounddb_filename))
    {
		return false;
	}

	ProgressTo( 20 );

	if (strcmp(g_mapicondb_filename, "") != 0)   // May not exist for mods
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
	if(profiledb_Get()->GetCivIndex() >= g_theCivilisationDB->NumRecords())
		profiledb_Get()->SetCivIndex(1); // Set to first non-Barbarian civ

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

		snprintf(lastdot, _MAX_PATH - (lastdot - g_citysize_filename), ".txt");
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
		sint32 i;
		sint32 j;

		for(i = 1; i < g_theDiplomacyProposalDB->NumRecords(); i++) {
			const DiplomacyProposalRecord *rec1 = g_theDiplomacyProposalDB->Get(i);
			for(j = 1; j < g_theDiplomacyProposalDB->NumRecords(); j++) {
				if(j == i) continue;

				const DiplomacyProposalRecord *rec2 = g_theDiplomacyProposalDB->Get(j);
				if(rec1->GetExcludes() & rec2->GetClass()) continue;

				StringId str1;
				StringId str2;
				const DiplomacyProposalRecord::StrengthStrings *strings1;
				const DiplomacyProposalRecord::StrengthStrings *strings2;

				if(rec1->GetDetails0(strings1)) {
					if(strings1->GetDetailsEven(str1)) {
						if(rec2->GetDetailsEx0(strings2)) {
							if(strings2->GetDetailsEven(str2)) {
								fprintf(dipFile, "%s %s (%s/%s)\n\n",
										stringdb_Get()->GetNameStr(str1), stringdb_Get()->GetNameStr(str2),
										stringdb_Get()->GetIdStr(str1), stringdb_Get()->GetIdStr(str2));
							}
						}
					}
				}
				if(rec1->GetDetails1(strings1)) {
					if(strings1->GetDetailsEven(str1)) {
						if(rec2->GetDetailsEx1(strings2)) {
							if(strings2->GetDetailsEven(str2)) {
								fprintf(dipFile, "%s %s (%s/%s)\n\n",
										stringdb_Get()->GetNameStr(str1), stringdb_Get()->GetNameStr(str2),
										stringdb_Get()->GetIdStr(str1), stringdb_Get()->GetIdStr(str2));
							}
						}
					}
				}
				if(rec1->GetDetails2(strings1)) {
					if(strings1->GetDetailsEven(str1)) {
						if(rec2->GetDetailsEx2(strings2)) {
							if(strings2->GetDetailsEven(str2)) {
								fprintf(dipFile, "%s %s (%s/%s)\n\n",
										stringdb_Get()->GetNameStr(str1), stringdb_Get()->GetNameStr(str2),
										stringdb_Get()->GetIdStr(str1), stringdb_Get()->GetIdStr(str2));
							}
						}
					}
				}
				if(rec1->GetDetails3(strings1)) {
					if(strings1->GetDetailsEven(str1)) {
						if(rec2->GetDetailsEx3(strings2)) {
							if(strings2->GetDetailsEven(str2)) {
								fprintf(dipFile, "%s %s (%s/%s)\n\n",
										stringdb_Get()->GetNameStr(str1), stringdb_Get()->GetNameStr(str2),
										stringdb_Get()->GetIdStr(str1), stringdb_Get()->GetIdStr(str2));
							}
						}
					}
				}
				if(rec1->GetDetails4(strings1)) {
					if(strings1->GetDetailsEven(str1)) {
						if(rec2->GetDetailsEx4(strings2)) {
							if(strings2->GetDetailsEven(str2)) {
								fprintf(dipFile, "%s %s (%s/%s)\n\n",
										stringdb_Get()->GetNameStr(str1), stringdb_Get()->GetNameStr(str2),
										stringdb_Get()->GetIdStr(str1), stringdb_Get()->GetIdStr(str2));
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








bool CivApp::IsScenarioEditorShown() const
{
	return ScenarioEditor::IsShown();
}

bool CivApp::IsScenarioEditorGivingAdvances() const
{
	return ScenarioEditor::IsGivingAdvances();
}

sint32 CivApp::InitializeEngine()
{
	civapp_log->info("InitializeEngine: started");

	// Wire the game-observer registry global before anything else that might
	// fire Notify*().  See game_observer.cpp for why this is deferred from
	// static-init time.
	gameobservers_Set(&GameObserverRegistry::Instance());

	Splash::Initialize();

	CivPaths_InitCivPaths();

	profiledb_Set(new ProfileDB);
	if (!profiledb_Get()->Init(FALSE)) {
		c3errors_FatalDialog("CivApp", "Unable to init the ProfileDB.");
		return -1;
	}

	g_logCrashes = profiledb_Get()->GetEnableLogs();

	InitDataIncludePath();
	civpaths_Get()->InitCDPath();
	GreatLibrary::Initialize_Great_Library_Data();

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

    SPLASH_STRING(profiledb_Get()->IsAIOn() ? "AI is ON" : "AI is OFF");

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

		sint32 const userCivIndex = profiledb_Get()->GetCivIndex();

		if (static_cast<int>(userCivIndex) < g_theCivilisationDB->NumRecords())
		{
			MBCHAR const * const    dbCivName =
			    stringdb_Get()->GetNameStr
			        (g_theCivilisationDB->Get(userCivIndex)->GetPluralCivName());

			if (0 == strcmp(dbCivName, profiledb_Get()->GetCivName()))
			{
				// No action: keep the leader name of the user.
			}
			else
			{
				// Restore civilisation default country and leader names.
				profiledb_Get()->DefaultSettings();
			}
		}
		else
		{
			// Possible after using a mod with less civilisations
			profiledb_Get()->SetCivIndex(1);
			profiledb_Get()->DefaultSettings();
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

	if (profiledb_Get()->IsUseFingerprinting())
		if (!ctpfinger_Check()) {

			c3errors_FatalDialog(appstrings_GetString(APPSTR_INITIALIZE),
									appstrings_GetString(APPSTR_CANTFINDFILE));
		}

	ProgressTo( 620 );

	if (c3ui_Get()->TheMouse())
    {
		double const sensitivity = 0.25 * (1 + profiledb_Get()->GetMouseSpeed());
		c3ui_Get()->TheMouse()->Sensitivity() = sensitivity;
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

void CivApp::CleanupAppUI()
{
#if CTP2_ENABLE_NETWORKING
	NetShell::Leave( k_NS_FLAGS_DESTROY );
#endif

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

    { C3UI * ui = c3ui_Get(); allocated::clear(ui); c3ui_Set(ui); }

#if defined(_DEBUG) && !defined(__AUI_USE_SDL__)
	sint32 const cleanBaseRefCount = aui_Base::GetBaseRefCount();
	Assert(0 == cleanBaseRefCount);
#endif

	allocated::clear(g_GreatLibPF);
	allocated::clear(g_ImageMapPF);
	allocated::clear(g_SoundPF);
}






void CivApp::CleanupAppDB()
{
    allocated::clear(g_theMapDB);
    { Exclusions *p = exclusions_Get(); delete p; exclusions_Set(nullptr); }
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
    { ThroneDB *p = thronedb_Get(); delete p; thronedb_Set(nullptr); };
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
    { auto * p = stringdb_Get(); allocated::clear(p); stringdb_Set(p); }
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




void CivApp::CleanupApp()
{
	if (m_appLoaded)
	{
		network_Get().Cleanup();
		GreatLibrary::Shutdown_Great_Library_Data();
		Splash::Cleanup();
		messagewin_Cleanup();

		// X_Set(nullptr) routes into m_x.reset(nullptr) which deletes
		// via unique_ptr's deleter — no manual delete first.  The
		// pre-trampoline pattern `delete X_Get(); X_Set(NULL);` would
		// double-free here.
		slicengine_Set(nullptr);
		m_game->SetMessagesPtr(nullptr);

		CivScenarios::Cleanup();
		SoundManager::Cleanup();

		{ ProfileDB * db = profiledb_Get(); allocated::clear(db); profiledb_Set(db); }

		gameinit_Cleanup();
		ui_events_Cleanup();
		events_Cleanup();
		gameEventManager_Cleanup();
		network_Get().Cleanup();
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







sint32 CivApp::InitializeGameUI()
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

	ProgressTo(10, stringdb_Get()->GetNameStr("LOADING"));

	SPLASH_STRING("Creating Main Windows...");
#if defined(_DEBUG)
	splash_MarkOld();
#endif

	SPLASH_STRING("Creating Status Window...");
	sint32 errcode = c3windows_MakeStatusWindow(TRUE);
	Assert(errcode == 0);
	if (errcode != 0) return 7;
	statuswindow_Get()->Hide(); // Maybe should be removed entirely

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

    AUI_ERRCODE auiErr = c3ui_Get()->AddWindow( background_Get() );
	Assert(auiErr == AUI_ERRCODE_OK);
	if ( auiErr != AUI_ERRCODE_OK ) return 11;

	ProgressTo( 100 );

	auiErr = c3ui_Get()->AddWindow( statuswindow_Get() );
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






sint32 CivApp::InitializeGame()
{
	// Headless: c3ui_Get() is null and every helper below (c3windows_*,
	// ChatBox, GrabItem, MainControlPanel, director_Get()->*, scenario UI
	// reload, etc.) crashes or no-ops on UI singletons.  Reroute to
	// the headless path which does only the game-state restore +
	// minimal subsystem init (AI, gevManager).  Both --new-game and
	// --load-game share this entry point now.
	if (!c3ui_Get()) return InitializeGameHeadless();

	ProgressWindow::BeginProgress(
		g_theProgressWindow,
		"InitProgressWindow",
		770 );

	ProgressTo
	    (10, stringdb_Get()->GetNameStr("LOADING"));

	init_keymap();

	ProgressTo( 20 );

	SPLASH_STRING("Initializing Sprite Engine...");

	sprite_Initialize();

	ProgressTo( 540 );

    if (m_dbLoaded && profiledb_Get()->IsScenario())
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

	if (m_dbLoaded && profiledb_Get()->IsScenario()) {
		if(controlpanel_Get())
			controlpanel_Get()->CreateTileImpBanks();
	}

	ProgressTo( 590 );

	gameEventManager_Initialize();

	// Prevent the event handler corrupting the (diplomacy) data in the
	// middle of a file restore operation.
	m_game->GetEventsPtr()->Pause();

	events_Initialize();
	ui_events_Initialize();

	ProgressTo( 600 );

	g_fog_toggle = FALSE;
	g_god = FALSE;

	if (!gameinit_Initialize(-1, -1)) {
		m_game->GetEventsPtr()->Resume();
		return FALSE;
	}

	ProgressTo( 610 );

	ProgressTo( 620 );

	if(is_scenario_Get() && !g_oldRandSeed) {






		civrand().Initialize(static_cast<sint32>(time(nullptr)));
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
	m_game->GetEventsPtr()->Dump();
#endif

	ProgressTo( 650 );

	GraphicsOptions::Initialize();

	SPLASH_STRING("Initializing Tile Engine...");
	tile_Initialize(false);

	ProgressTo( 660 );

	if (is_scenario_Get()) {
		tiledmap_Get()->PostProcessMap();
	}

	ProgressTo( 670 );

	radar_Initialize();

	ProgressTo( 680 );

	Splash::Cleanup();

	ProgressTo( 690 );

	// m_game was constructed eagerly in CivApp's ctor; NewGame just
	// populates its subsystems for this session.  After CleanupGame()
	// runs we keep the empty container alive so the next session can
	// reuse it.
	m_game->NewGame(
		profiledb_Get()->GetNPlayers(),
		diffutil_GetYearFromTurn(gamesettings_Get()->GetDifficulty(), 0)
	);

	m_gameLoaded = TRUE;
	//m_game->GetEventsPtr()->Resume();
	//m_game->GetEventsPtr()->Process();

	ProgressTo( 700 );

	director_Get()->CatchUp();

  m_game->GetEventsPtr()->Resume();
  m_game->GetEventsPtr()->Process();

	ProgressTo( 710 );

	if(!network_Get().IsActive() && !network_Get().IsNetworkLaunch())
	{
		m_game->GetEventsPtr()->AddEvent(GEV_INSERT_Tail,
			GEV_BeginTurn,
			GEA_Player, selitem_Get()->GetCurPlayer(),
			GEA_Int, player_Get(selitem_Get()->GetCurPlayer())->m_current_round,
			GEA_End);
	}

	ProgressTo( 720 );

	if(!network_Get().IsActive()) {
		if (director_Get())
			director_Get()->AddCopyVision();
	}

	ProgressTo( 730 );

	director_Get()->ReloadAllSprites();

	ProgressTo( 740 );

	ProgressTo( 750 );

	if (!turn_Get()->IsHotSeat())
	{
		MainControlPanel::UpdateCityList();
	}

	g_scenarioUsePlayerNumber = 0;

	ProgressTo( 760 );

	if (selitem_Get())
    {
        selitem_Get()->Refresh();

		if (director_Get())
			director_Get()->AddCenterMap(selitem_Get()->GetCurSelectPos());
	}

	ProgressTo( 770 );

	g_oldRandSeed = FALSE;

	if (!turn_Get()->IsHotSeat())
	{
		MainControlPanel::UpdatePlayer(selitem_Get()->GetCurPlayer());
	}

    if (g_launchIntoCheatMode)
    {
        ScenarioEditor::Display();
    }

	if (    turn_Get()->IsEmail()
	     && player_Get(selitem_Get()->GetCurPlayer())->IsTurnOver()
	){
		m_game->GetEventsPtr()->AddEvent(GEV_INSERT_Tail,
		                       GEV_BeginTurn,
		                       GEA_Player, selitem_Get()->GetCurPlayer(),
		                       GEA_Int,    player_Get(selitem_Get()->GetCurPlayer())->GetCurRound() + 1,
		                       GEA_End);
	}

	ProgressWindow::EndProgress( g_theProgressWindow );

	return 0;
}





sint32 InitializeSpriteEditorUI()
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

	ProgressTo(10, stringdb_Get()->GetNameStr("LOADING"));

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

	AUI_ERRCODE	auiErr = c3ui_Get()->AddWindow(background_Get());
	Assert(auiErr == AUI_ERRCODE_OK);
	if ( auiErr != AUI_ERRCODE_OK ) return 11;

	SPLASH_STRING("Creating Info Bar...");
	InfoBar::Initialize();

	ProgressTo( 100 );

	g_modalWindow = 0;
	AttractWindow::Initialize();

	ProgressTo( 110 );

	HideControlPanel();
	statuswindow_Get()->Hide();

	ProgressTo( 120 );

	errcode = SpriteEditWindow_Initialize();
	auiErr = c3ui_Get()->AddWindow( g_spriteEditWindow );
	Assert(auiErr == AUI_ERRCODE_OK);
	if ( auiErr != AUI_ERRCODE_OK ) return 11;

	ProgressWindow::EndProgress( g_theProgressWindow );

	return 0;
}





sint32 CivApp::InitializeSpriteEditor()
{
	ProgressWindow::BeginProgress(
		g_theProgressWindow,
		"InitProgressWindow",
		860 );

	ProgressTo(10, stringdb_Get()->GetNameStr("LOADING"));

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

	if (m_dbLoaded && profiledb_Get()->IsScenario())
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

	m_game->GetSlicPtr()->RunTrigger(TRIGGER_LIST_GAME_LOADED, ST_END);

	ProgressTo( 730 );

#ifdef _DEBUG
	m_game->GetEventsPtr()->Dump();
#endif

	ProgressTo( 740 );

	SPLASH_STRING("Initializing AI...");
	roboinit_Initalize();
	CtpAi::Initialize();

	ProgressTo( 750 );

	SPLASH_STRING("Initializing Tile Engine...");
	tile_Initialize(false);

	ProgressTo( 760 );

	radar_Initialize();

	ProgressTo( 770 );

	Splash::Cleanup();

	ProgressTo( 780 );

	// See comment at the matching site in InitializeGameHeadless.
	m_game->NewGame(
		profiledb_Get()->GetNPlayers(),
		diffutil_GetYearFromTurn(gamesettings_Get()->GetDifficulty(), 0)
	);

	m_gameLoaded = TRUE;

	director_Get()->CatchUp();

	ProgressTo( 790 );

	turn_Get()->BeginNewTurn(FALSE);

	if(!network_Get().IsActive()) {
		if(g_scenarioUsePlayerNumber == 0 && !turn_Get()->IsHotSeat() &&
			!turn_Get()->IsEmail()) {
			selitem_Get()->SetPlayerOnScreen(1);
		}
		if (director_Get())
			director_Get()->AddCopyVision();
	}

	ProgressTo( 800 );

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


void CivApp::CleanupGameUI()
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
	// Clear the loaded flag FIRST: teardown below destroys the world
	// (m_game->Cleanup), but CleanupGame also pumps ProcessUI to drain the
	// director/UI. ProcessUI's scroll path is gated on m_gameLoaded, and
	// scrolling a half-destroyed game dereferences the freed world. Clearing
	// it up front keeps ProcessUI out of the game-render path during cleanup.
	m_gameLoaded = false;

	// Clear per-session subsystems before legacy gameinit_Cleanup() runs.
	// The Game container itself is owned by CivApp for its full lifetime
	// (constructed eagerly in CivApp's ctor) — Cleanup() just empties it
	// so the next NewGame can repopulate.
	if (m_game) {
		m_game->Cleanup();
	}

	gameinit_CleanupMessages();
	uint32 target_milliseconds = 100000;
	uint32 used_milliseconds;
	ProcessUI(target_milliseconds, used_milliseconds);

	network_Get().Cleanup();

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

	if (soundmgr_Get())
		soundmgr_Get()->DumpAllSounds();








	if(!keepScenInfo) {

		profiledb_Get()->SetIsScenario(FALSE);
		civpaths_Get()->ClearCurScenarioPath();
		civpaths_Get()->ClearCurScenarioPackPath();
		memset(scenario_name_buf(), '\0', k_SCENARIO_NAME_MAX);
		CleanupAppDB();
		InitializeAppDB();
	}

#if defined(_DEBUG) && defined(_MEMORYLOGGING) && defined(_DEBUG_MEMORY)
	DebugMemory_LeaksShow(8675209);
#endif

	m_gameLoaded            = false;

	g_launchIntoCheatMode   = FALSE;
	g_god                   = FALSE;
	g_isCheatModeOn         = FALSE;

	c3ui_Get()->BlackScreen();
}

void CivApp::StartMessageSystem()
{
	if (!m_dbLoaded)
    {
		InitializeAppDB();
    }

    // SetMessagesPtr(new) deletes the previous via reset().
    m_game->SetMessagesPtr(new MessagePool());
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








void CivApp::ProcessGraphicsCallback()
{
    static bool s_inCallback = false;

	if (s_inCallback)   return;

	if (!tiledmap_Get())    return;
	if (!background_Get())  return;
	if (!director_Get())    return;
	if (!background_Get())  return;
	if (!c3ui_Get())        return;

	s_inCallback = true;

	tiledmap_Get()->RestoreMixFromMap(background_Get()->TheSurface());
	background_Get()->Draw();
	c3ui_Get()->Process();

	if (!network_Get().IsActive() || network_Get().ReadyToStart())
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

	if (c3ui_Get()->TheMouse()) {
		if (c3ui_Get()->TheMouse()->IsSuspended() )
		{
			used_milliseconds = Os::GetTicks() - start_time_ms;

			if(g_runInBackground
			|| profiledb_Get()->GetValueByName("RunInBackground")
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

		bool netGameLoading = (network_Get().IsNetworkLaunch() || network_Get().IsActive())
			                  && !network_Get().ReadyToStart();

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

					tiledmap_Get()->CopyMixDirtyRects(background_Get()->GetDirtyList());

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

					c3ui_Get()->Process();

					uint32 target_milliseconds=30;
					uint32 used_milliseconds;

					ProcessNet(target_milliseconds, used_milliseconds);
				}

				tiledmap_Get()->RetargetTileSurface(nullptr);
				tiledmap_Get()->Refresh();
				tiledmap_Get()->InvalidateMap();
				tiledmap_Get()->ValidateMix();
			}
            else
            {
				if(tiledmap_Get()) {
					tiledmap_Get()->RestoreMixFromMap(background_Get()->TheSurface());
				}
				if(background_Get())
					background_Get()->Draw();

			}

			lastTicks = curTicks;
		}
	}

	if (m_appLoaded) {
		c3ui_Get()->Process();
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

			// New shared dispatch: UI-free command/query handlers that behave
			// identically in headless and UI builds. Falls through to the
			// legacy chain below for verbs not yet migrated.  DispatchSafe
			// (not Dispatch) because the poll left the smoke mutex LOCKED;
			// it is released only by a send_* call, so an escaping exception
			// would wedge the server forever.
			bool gc_handled = false;
			std::string gc_resp = game_controller::DispatchSafe(cmd, gc_handled);
			if (gc_handled) {
				smoketest_send_json(gc_resp.c_str());
			}
			else if (strcmp(cmd, "new_game") == 0) {
				if (m_appLoaded && !m_gameLoaded) {
					initialplayscreen_newgamePress(nullptr, AUI_BUTTON_ACTION_EXECUTE, 0, nullptr);
					smoketest_send_response("ok", cmd, nullptr);
				} else {
					smoketest_send_response("error", cmd, "not_on_main_menu");
				}
			}
			else if (strcmp(cmd, "start_game") == 0) {
				if (m_appLoaded && !m_gameLoaded) {
					spnewgamescreen_startPress(nullptr, AUI_BUTTON_ACTION_EXECUTE, 0, nullptr);
					smoketest_send_response("ok", cmd, nullptr);
				} else {
					smoketest_send_response("error", cmd, "not_on_new_game_screen");
				}
			}
			else if (strcmp(cmd, "end_turn") == 0) {
				if (m_gameLoaded) {
					director_Get()->AddEndTurn();
					smoketest_send_response("ok", cmd, nullptr);
				} else {
					smoketest_send_response("error", cmd, "game_not_loaded");
				}
			}
			else if (strncmp(cmd, "advance_round", 13) == 0) {
				if (m_gameLoaded) {
					int n = 1;
					if (cmd[13] != '\0' && sscanf(cmd + 13, "%d", &n) != 1) n = -1;
					if (n < 1 || n > 1000) {
						smoketest_send_response("error", cmd, "bad_args");
					} else {
						for (int i = 0; i < n; ++i) {
							game_controller::RunRound(
								turn_Get() ? turn_Get()->GetSessionRound() : 0,
								nullptr);
						}
						char detail[48];
						snprintf(detail, sizeof(detail), "round=%d",
						         (int)(turn_Get() ? turn_Get()->GetSessionRound() : 0));
						smoketest_send_response("ok", "advance_round", detail);
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
							Player *human = nullptr;
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
									smoketest_send_response("ok", cmd, nullptr);
								}
							}
						}
					}
				} else {
					smoketest_send_response("error", cmd, "game_not_loaded");
				}
			}
			else if (strcmp(cmd, "turn_counter") == 0) {
				if (m_gameLoaded && turn_Get()) {
					char detail[64];
					snprintf(detail, sizeof(detail), "round=%d", turn_Get()->GetRound());
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
						Player *human = nullptr;
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
							if (!stringdb_Get()->GetStringID(adv_name, str_id)) {
								smoketest_send_response("error", cmd, "advance_name_not_found");
							} else {
								sint32 adv_idx = -1;
								if (!g_theAdvanceDB->GetNamedItem(str_id, adv_idx)) {
									smoke_log->info("Setting research to advance {} ({})",
										(int)adv_idx, adv_name);
									human->SetResearching(adv_idx);
									smoketest_send_response("ok", cmd, nullptr);
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
						Player *human = nullptr;
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
			else if (strncmp(cmd, "screenshot ", 11) == 0) {
				const char *path = cmd + 11;
				if (!path[0]) {
					smoketest_send_response("error", cmd, "bad_args");
				} else {
#ifdef USE_SDL
					aui_SDLSurface *sdlSurf = static_cast<aui_SDLSurface*>(c3ui_Get()->Primary());
					if (sdlSurf && sdlSurf->DDS()) {
						if (CTP2_SDL_SaveBMP(sdlSurf->DDS(), path)) {
							smoke_log->info("Screenshot saved to {}", path);
							smoketest_send_response("ok", cmd, nullptr);
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
			else if (strncmp(cmd, "screenshot_presented ", 21) == 0) {
				// screenshot_presented <presented.bmp> [primary.bmp]
				// Presented-frame readback: re-composite the GPU present
				// stage (Copy from the persistent screen texture into a
				// target texture, NO present — the backbuffer after
				// RenderPresent is undefined, re-compositing is
				// deterministic) and read the pixels back. The optional
				// second path also saves the software primary surface IN
				// THE SAME DISPATCH, so the pair is atomic — no animation
				// frame can land between the two captures. Together they
				// validate the texture upload + RenderCopy path that every
				// visible frame goes through.
				char presPath[1024] = {0};
				char primPath[1024] = {0};
				sscanf(cmd + 21, "%1023s %1023s", presPath, primPath);
				const char *path = presPath;
				if (!path[0]) {
					smoketest_send_response("error", cmd, "bad_args");
				} else {
#ifdef USE_SDL
					SDL_Renderer *renderer = aui_SDL::Renderer();
					SDL_Texture  *texture  = aui_SDL::ScreenTexture();
					if (!renderer || !texture) {
						smoketest_send_response("error", cmd, "no_renderer");
					} else {
						// Copy the screen texture into a same-size TARGET
						// texture and read that back. Target state resets
						// viewport/scale to 1:1 texture size, so the read
						// is exact regardless of HiDPI backbuffer scale or
						// SDL_RenderSetLogicalSize on the window target.
						// P11 Stage 2 D: mirror the exact present composite. With
						// per-layer GPU compositing the visible frame is the world
						// texture with the UI texture alpha-blended over it, so the
						// readback must reproduce both layers (re-composing only the
						// screen texture would read a stale/never-uploaded texture).
						bool const layered = aui_SDL::GpuLayersEnabled()
						                   && aui_SDL::WorldTexture()
						                   && aui_SDL::UiTexture();
						// Target is SCREEN-sized (the presented frame), from the
						// screen texture — NOT the world texture, which is oversized
						// by the pan margin (P11 2a). The world layer is windowed back
						// to the screen below.
						int texW = 0, texH = 0;
						CTP2_SDL_GetTextureSize(texture, &texW, &texH);
						SDL_Texture *target = SDL_CreateTexture(renderer,
						        SDL_PIXELFORMAT_ARGB8888,
						        SDL_TEXTUREACCESS_TARGET, texW, texH);
						bool ok = false;
						if (target && CTP2_SDL_SetRenderTarget(renderer, target)) {
							SDL_RenderClear(renderer);
							if (layered) {
								if (aui_SDL::GpuQuadsEnabled() && aui_SDL::QuadAtlasTexture()
								    && aui_SDL::QuadFrameComplete()) {
									CTP2_SDL_SetRenderTarget(renderer, aui_SDL::WorldTexture());
									SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
									SDL_RenderClear(renderer);
									SDL_Texture * const atlas = aui_SDL::QuadAtlasTexture();
									for (aui_SDL::GpuQuad const & q : aui_SDL::QuadDrawList()) {
										CTP2_SDL_RenderTextureSrcDst(renderer, atlas,
											q.sx, q.sy, q.sw, q.sh,
											(float)q.dx, (float)q.dy, (float)q.dw, (float)q.dh);
									}
									for (aui_SDL::GpuSpriteQuad const & q : aui_SDL::SpriteDrawList()) {
										if (q.screen_space)
											continue;
										SDL_SetTextureBlendMode(q.texture, q.additive ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
										SDL_SetTextureColorMod(q.texture, q.red, q.green, q.blue);
										SDL_SetTextureAlphaMod(q.texture, q.alpha);
										CTP2_SDL_RenderTextureSrcDstFlip(renderer, q.texture,
											q.sx, q.sy, q.sw, q.sh,
											(float)q.dx, (float)q.dy, (float)q.dw, (float)q.dh,
											q.mirror);
									}
									CTP2_SDL_SetRenderTarget(renderer, target);
								}
								// P11 2a: mirror Flip's windowed present exactly — the
								// world+fog layers are windowed from the (oversized)
								// texture to the screen viewport, slid by CameraOff and
								// scaled by CameraZoom (identity by default). UI stays
								// full-screen. Must match aui_SDLSurface::Flip or the
								// oracle diverges from the real frame.
								bool  const cam  = aui_SDL::GpuCameraEnabled();
								float const W    = (float)texW, H = (float)texH;
								float const z    = cam ? aui_SDL::CameraZoom() : 1.0f;
								float const offX = cam ? aui_SDL::CameraOffX() : 0.0f;
								float const offY = cam ? aui_SDL::CameraOffY() : 0.0f;
								auto windowed = [&](SDL_Texture *tex, float baseX, float baseY) {
									float const srcW = W / z, srcH = H / z;
									float const srcX = baseX + (W - srcW) * 0.5f - offX;
									float const srcY = baseY + (H - srcH) * 0.5f - offY;
									CTP2_SDL_RenderTextureWindow(renderer, tex,
										srcX, srcY, srcW, srcH, 0.0f, 0.0f, W, H);
								};
								// P12: quad and mirrored paths share the same oversized
								// world-space origin; only the producer differs.
								//
								// P13 step 2.1 (ADR-003): Flip presents the whole-map
								// target when it is ready and the window mirror
								// otherwise. The readback MUST branch the same way.
								// While it did not, this oracle re-composited the
								// window mirror no matter what CTP2_GPU_WORLDMAP was
								// set to -- so every screenshot test was blind to the
								// whole-map path, and the tile-gap and transition bugs
								// in it could only be found by looking at the game.
								if (aui_SDL::GpuWorldmapEnabled() && aui_SDL::WorldmapTexture()) {
									// Filtering, the seam split and the source rect are
									// all inside PresentWorldmapWindow, which Flip
									// calls too -- sharing the function is what keeps
									// this oracle honest, rather than a comment
									// asking the next edit to mirror it by hand.
									aui_SDL::PresentWorldmapWindow(renderer, W, H, z, offX, offY);
									// Mirror Flip: sprites are drawn over the
									// windowed terrain on this path, not into
									// the texture.
									aui_SDL::RenderWorldmapSpriteQuads(renderer, W, H, z, offX, offY);
								} else {
									windowed(aui_SDL::WorldTexture(),
										(float)aui_SDL::WorldContentOffX(),
										(float)aui_SDL::WorldContentOffY());
								}
								// P11 C: fog mask darkens the world between the world
								// and UI copies (mirrors Flip's present).
								if (aui_SDL::GpuFogEnabled() && aui_SDL::FogTexture())
									windowed(aui_SDL::FogTexture(), 0.0f, 0.0f);
								for (aui_SDL::GpuSpriteQuad const & q : aui_SDL::SpriteDrawList()) {
									if (!q.screen_space)
										continue;
									SDL_SetTextureBlendMode(q.texture, q.additive ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
									SDL_SetTextureColorMod(q.texture, q.red, q.green, q.blue);
									SDL_SetTextureAlphaMod(q.texture, q.alpha);
									CTP2_SDL_RenderTextureSrcDstFlip(renderer, q.texture,
										q.sx, q.sy, q.sw, q.sh,
										(float)q.dx, (float)q.dy, (float)q.dw, (float)q.dh,
										q.mirror);
								}
								CTP2_SDL_RenderTexture(renderer, aui_SDL::UiTexture());
							} else {
								CTP2_SDL_RenderTexture(renderer, texture);
							}
							ok = CTP2_SDL_SaveRendererPixels(renderer, path, texW, texH);
							CTP2_SDL_SetRenderTarget(renderer, nullptr);
						}
						if (target) SDL_DestroyTexture(target);
						// Atomic pair: capture the software primary in the
						// same dispatch (no Draw can run in between).
						if (ok && primPath[0]) {
							aui_SDLSurface *prim = static_cast<aui_SDLSurface*>(c3ui_Get()->Primary());
							ok = prim && prim->DDS()
							  && CTP2_SDL_SaveBMP(prim->DDS(), primPath);
						}
						if (ok) {
							smoke_log->info("Presented-frame readback saved to {}", path);
							smoketest_send_response("ok", cmd, nullptr);
						} else {
							smoketest_send_response("error", cmd, "readback_failed");
						}
					}
#else
					smoketest_send_response("error", cmd, "not_sdl");
#endif
				}
			}
#ifdef RENDER_TOOL_BUILD
			else if (strncmp(cmd, "screenshot_map_only ", 20) == 0) {
				// screenshot_map_only <path>
				// Render only the map layer: terrain, fog-visible map sprites, trade/map
				// effects. No UI windows, modal overlays, mouse hilite, legal moves, or
				// turn/gameplay processing. This is the map-to-texture seam for visual
				// review and future offscreen map rendering.
				char path[1024] = {0};
				sscanf(cmd + 20, "%1023s", path);
				bool ready = false;
				if (!path[0]) {
					smoketest_send_response("error", cmd, "bad_args");
				} else if (!background_Get()) {
					smoketest_send_response("error", cmd, "no_background");
				} else if (!tiledmap_Get()) {
					smoketest_send_response("error", cmd, "no_tiledmap");
				} else {
					tiledmap_Get()->BuildTerrainQuads();
					if (background_render_map_only(background_Get()) != AUI_ERRCODE_OK) {
						smoketest_send_response("error", cmd, "render_failed");
					} else {
						ready = true;
					}
				}
				if (ready) {
#ifdef USE_SDL
					bool ok = false;
					SDL_Renderer *renderer = aui_SDL::Renderer();
					SDL_Texture  *screenTexture = aui_SDL::ScreenTexture();
					bool const drawGpuWorld = renderer && screenTexture
						&& aui_SDL::GpuLayersEnabled()
						&& aui_SDL::GpuQuadsEnabled()
						&& aui_SDL::WorldTexture()
						&& aui_SDL::QuadAtlasTexture()
						&& aui_SDL::QuadFrameComplete();

					if (drawGpuWorld) {
						int texW = 0, texH = 0;
						CTP2_SDL_GetTextureSize(screenTexture, &texW, &texH);
						SDL_Texture *target = SDL_CreateTexture(renderer,
							SDL_PIXELFORMAT_ARGB8888,
							SDL_TEXTUREACCESS_TARGET, texW, texH);
						if (target && CTP2_SDL_SetRenderTarget(renderer, target)) {
							CTP2_SDL_SetRenderTarget(renderer, aui_SDL::WorldTexture());
							SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
							SDL_RenderClear(renderer);
							SDL_Texture * const atlas = aui_SDL::QuadAtlasTexture();
							for (aui_SDL::GpuQuad const & q : aui_SDL::QuadDrawList()) {
								CTP2_SDL_RenderTextureSrcDst(renderer, atlas,
									q.sx, q.sy, q.sw, q.sh,
									(float)q.dx, (float)q.dy, (float)q.dw, (float)q.dh);
							}
							for (aui_SDL::GpuSpriteQuad const & q : aui_SDL::SpriteDrawList()) {
								if (q.screen_space)
									continue;
								SDL_SetTextureBlendMode(q.texture, q.additive ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
								SDL_SetTextureColorMod(q.texture, q.red, q.green, q.blue);
								SDL_SetTextureAlphaMod(q.texture, q.alpha);
								CTP2_SDL_RenderTextureSrcDstFlip(renderer, q.texture,
									q.sx, q.sy, q.sw, q.sh,
									(float)q.dx, (float)q.dy, (float)q.dw, (float)q.dh,
									q.mirror);
							}

							CTP2_SDL_SetRenderTarget(renderer, target);
							SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
							SDL_RenderClear(renderer);
							bool  const cam  = aui_SDL::GpuCameraEnabled();
							float const W    = (float)texW, H = (float)texH;
							float const z    = cam ? aui_SDL::CameraZoom() : 1.0f;
							float const offX = cam ? aui_SDL::CameraOffX() : 0.0f;
							float const offY = cam ? aui_SDL::CameraOffY() : 0.0f;
							float const srcW = W / z, srcH = H / z;
							float const srcX = (float)aui_SDL::WorldContentOffX() + (W - srcW) * 0.5f - offX;
							float const srcY = (float)aui_SDL::WorldContentOffY() + (H - srcH) * 0.5f - offY;
							CTP2_SDL_RenderTextureWindow(renderer, aui_SDL::WorldTexture(),
								srcX, srcY, srcW, srcH, 0.0f, 0.0f, W, H);
							if (aui_SDL::GpuFogEnabled() && aui_SDL::FogTexture()) {
								float const fogSrcX = (W - srcW) * 0.5f - offX;
								float const fogSrcY = (H - srcH) * 0.5f - offY;
								CTP2_SDL_RenderTextureWindow(renderer, aui_SDL::FogTexture(),
									fogSrcX, fogSrcY, srcW, srcH, 0.0f, 0.0f, W, H);
							}
							ok = CTP2_SDL_SaveRendererPixels(renderer, path, texW, texH);
							CTP2_SDL_SetRenderTarget(renderer, nullptr);
						}
						if (target) SDL_DestroyTexture(target);
					} else {
						aui_SDLSurface *bs = static_cast<aui_SDLSurface *>(background_Get()->TheSurface());
						aui_Surface *primary = c3ui_Get() ? c3ui_Get()->Primary() : nullptr;
						int const viewW = primary ? primary->Width() : 0;
						int const viewH = primary ? primary->Height() : 0;
						SDL_Surface *src = bs ? bs->DDS() : nullptr;
						SDL_Surface *shot = (src && viewW > 0 && viewH > 0) ? CTP2_SDL_CreateARGB8888Surface(viewW, viewH) : nullptr;
						if (shot) {
							SDL_Rect srect = { aui_SDL::WorldContentOffX(), aui_SDL::WorldContentOffY(), viewW, viewH };
							SDL_Rect drect = { 0, 0, viewW, viewH };
#if defined(CTP2_USE_SDL3)
							bool const blitOk = SDL_BlitSurface(src, &srect, shot, &drect);
#else
							bool const blitOk = SDL_BlitSurface(src, &srect, shot, &drect) == 0;
#endif
							ok = blitOk && CTP2_SDL_SaveBMP(shot, path);
							CTP2_SDL_DestroySurface(shot);
						}
					}
					if (ok) {
						smoketest_send_response("ok", cmd, drawGpuWorld ? "gpu_world" : "cpu_world");
					} else {
						smoketest_send_response("error", cmd, "save_failed");
					}
#else
					smoketest_send_response("error", cmd, "not_sdl");
#endif
				}
			}
#endif
			else if (strncmp(cmd, "camera_debug_set ", 17) == 0) {
				// camera_debug_set <offX> <offY>
				// TEMPORARY (P11 pixel-proof debug): force the GPU camera pan offset
				// to exact pixel values, bypassing velocity/target integration, so
				// a harness can verify that the presented frame actually shifts.
				float offX = 0.0f, offY = 0.0f;
				sscanf(cmd + 17, "%f %f", &offX, &offY);
#ifdef USE_SDL
				aui_SDL::SetCameraOffset(offX, offY);
				char detail[64];
				snprintf(detail, sizeof(detail), "off=%.1f,%.1f", offX, offY);
				smoketest_send_response("ok", cmd, detail);
#else
				smoketest_send_response("error", cmd, "not_sdl");
#endif
			}
			else if (strncmp(cmd, "camera_debug_zoom ", 18) == 0) {
				// camera_debug_zoom <zoom>
				// Force the camera zoom directly, bypassing pinch integration and
				// the detent, so a harness can check that sprites stay glued to
				// their tiles at a zoom other than 1. Without this the harness
				// could only ever test the one zoom where most errors vanish.
				float zoom = 1.0f;
				sscanf(cmd + 18, "%f", &zoom);
#ifdef USE_SDL
				aui_SDL::SetCamera(aui_SDL::CameraOffX(), aui_SDL::CameraOffY(), zoom);
				char detail[64];
				snprintf(detail, sizeof(detail), "zoom=%.3f", aui_SDL::CameraZoom());
				smoketest_send_response("ok", cmd, detail);
#else
				smoketest_send_response("error", cmd, "not_sdl");
#endif
			}
			else if (strncmp(cmd, "camera_debug_pan ", 17) == 0) {
				// camera_debug_pan <dx> <dy>
				// TEMPORARY (P11 pixel-proof debug): add to the buttery-pan
				// TARGET (screen px) exactly as trackpad input does, so a
				// harness can exercise the real ease/recenter path and verify
				// the glide passes through sub-tile positions.
				float dx = 0.0f, dy = 0.0f;
				sscanf(cmd + 17, "%f %f", &dx, &dy);
#ifdef USE_SDL
				if (aui_SDL::GpuCameraEnabled()) {
					aui_SDL::AddPanTarget(dx, dy);
					smoketest_send_response("ok", cmd, nullptr);
				} else {
					smoketest_send_response("error", cmd, "camera_off");
				}
#else
				smoketest_send_response("error", cmd, "not_sdl");
#endif
			}
			else if (strncmp(cmd, "pick_tile ", 10) == 0) {
				// pick_tile <screenX> <screenY>
				// Test seam: run the real screen->tile inversion the mouse uses
				// and report the tile it lands on. Picking has no other
				// observable output, so without this a harness can only check
				// it by clicking and watching what gets selected -- which
				// conflates the inversion with selection rules.
#ifdef USE_SDL
				int sx = 0, sy = 0;
				if (tiledmap_Get() && sscanf(cmd + 10, "%d %d", &sx, &sy) == 2) {
					POINT pt; pt.x = sx; pt.y = sy;
					MapPoint tile;
					BOOL const hit = tiledmap_Get()->MousePointToTilePos(pt, tile);
					char detail[96];
					snprintf(detail, sizeof(detail), "hit=%d tile=%d,%d",
					         hit ? 1 : 0, tile.x, tile.y);
					smoketest_send_response("ok", cmd, detail);
				} else {
					smoketest_send_response("error", cmd, "not_ready");
				}
#else
				smoketest_send_response("error", cmd, "not_sdl");
#endif
			}
			else if (strcmp(cmd, "debug_close_build_manager") == 0) {
				EditQueue::Hide();
				smoketest_send_response("ok", cmd, nullptr);
			}
			else if (strncmp(cmd, "camera_debug_center", 19) == 0) {
				// camera_debug_center [x y]
				// TEMPORARY (P11 pixel-proof debug): synchronously center the
				// map view on (x, y) — or the current selection — and force a
				// full terrain redraw (the dh_centerMap recipe), so a harness
				// gets real terrain pixels in view without depending on
				// director timing.
#ifdef USE_SDL
				if (g_modalWindow > 0) {
					// background_draw_handler skips terrain under a modal
					// (e.g. the Loading progress window) — report it so a
					// harness can retry once the modal clears.
					smoketest_send_response("error", cmd, "modal");
				} else if (selitem_Get() && radar_map_Get() && tiledmap_Get()
				    && background_Get()) {
					MapPoint pos = selitem_Get()->GetCurSelectPos();
					sint32 x = 0, y = 0;
					if (sscanf(cmd + 19, "%d %d", &x, &y) == 2)
						pos = MapPoint(x, y);
					radar_map_Get()->CenterMap(pos);
					tiledmap_Get()->Refresh();
					tiledmap_Get()->InvalidateMap();
					tiledmap_Get()->InvalidateMix();
					background_draw_handler(background_Get());
					RECT const * vr = tiledmap_Get()->GetMapViewRect();
					char detail[96];
					snprintf(detail, sizeof(detail),
					         "pos=%d,%d view=%ld,%ld,%ld,%ld",
					         pos.x, pos.y, (long)vr->left, (long)vr->top,
					         (long)vr->right, (long)vr->bottom);
					smoketest_send_response("ok", cmd, detail);
				} else {
					smoketest_send_response("error", cmd, "not_ready");
				}
#else
				smoketest_send_response("error", cmd, "not_sdl");
#endif
			}
			else if (strncmp(cmd, "camera_debug_layers ", 20) == 0) {
				// camera_debug_layers <world.bmp> <ui.bmp> [bgwin.bmp]
				// TEMPORARY (P11 pixel-proof debug): dump the CPU world and UI
				// composite surfaces (and optionally the background window's
				// own surface, to tell "terrain never drawn" from "terrain
				// drawn but not composited/mirrored").
				char worldPath[1024] = {0};
				char uiPath[1024] = {0};
				char bgPath[1024] = {0};
				sscanf(cmd + 20, "%1023s %1023s %1023s", worldPath, uiPath, bgPath);
#ifdef USE_SDL
				aui_UI *ui = c3ui_Get();
				bool ok = ui && ui->WorldSurface() && ui->UiSurface();
				if (ok) {
					aui_SDLSurface *ws = static_cast<aui_SDLSurface *>(ui->WorldSurface());
					aui_SDLSurface *us = static_cast<aui_SDLSurface *>(ui->UiSurface());
					ok = ws->DDS() && us->DDS()
					  && CTP2_SDL_SaveBMP(ws->DDS(), worldPath)
					  && CTP2_SDL_SaveBMP(us->DDS(), uiPath);
					if (ok && bgPath[0] && background_Get()) {
						aui_SDLSurface *bs = static_cast<aui_SDLSurface *>(
							background_Get()->TheSurface());
						ok = bs && bs->DDS() && CTP2_SDL_SaveBMP(bs->DDS(), bgPath);
					}
					if (ok && bgPath[0] && tiledmap_Get()) {
						// Sibling dump: the tile renderer's own surface, to
						// tell "tiles never painted" from "mix copy broken".
						char mixPath[1040];
						snprintf(mixPath, sizeof(mixPath), "%s.mix.bmp", bgPath);
						aui_SDLSurface *ms = static_cast<aui_SDLSurface *>(
							tiledmap_Get()->GetSurface());
						if (ms && ms->DDS())
							CTP2_SDL_SaveBMP(ms->DDS(), mixPath);
					}
				}
				if (ok) {
					// Diagnose world-layer routing: BltToSecondary routes a
					// blit to the world layer only when its source surface
					// IS the world-window key. Report whether the key
					// resolves to the background window's live surface.
					aui_Surface *key = ui->WorldSurfaceKey();
					aui_Surface *live = background_Get()
					                  ? background_Get()->TheSurface() : nullptr;
					char detail[64];
					snprintf(detail, sizeof(detail), "key_null=%d eq=%d",
					         key == nullptr ? 1 : 0, key == live ? 1 : 0);
					smoketest_send_response("ok", cmd, detail);
				} else {
					smoketest_send_response("error", cmd, "save_failed");
				}
#else
				smoketest_send_response("error", cmd, "not_sdl");
#endif
			}
			else if (strncmp(cmd, "render_map ", 11) == 0) {
				// render_map <path> [zoom]
				// Unfogged, whole-map isometric render to a BMP (real game art),
				// for the empire-timelapse tooling. Distinct from `screenshot`,
				// which only grabs the current fogged viewport.
				if (!m_gameLoaded) {
					smoketest_send_response("error", cmd, "game_not_loaded");
				} else {
#ifdef USE_SDL
					char pathBuf[1024] = {0};
					int  zoom = 1;   // small tiles -> manageable surface; tune per call
					sscanf(cmd + 11, "%1023s %d", pathBuf, &zoom);
					TiledMap *tm = tiledmap_Get();
					if (!pathBuf[0]) {
						smoketest_send_response("error", cmd, "bad_args");
					} else if (!tm) {
						smoketest_send_response("error", cmd, "no_map");
					} else {
						sint32 SW = 0, SH = 0;
						tm->FullMapPixelSize(zoom, &SW, &SH);
						AUI_ERRCODE err = AUI_ERRCODE_OK;
						aui_SDLSurface *off =
							new aui_SDLSurface(&err, SW, SH, 16, nullptr, FALSE);
						if (off && err == AUI_ERRCODE_OK && off->DDS()) {
							tm->RenderFullMap(off, zoom);
							if (CTP2_SDL_SaveBMP(off->DDS(), pathBuf)) {
								smoke_log->info("Full map rendered to {} ({}x{} zoom {})",
								                pathBuf, SW, SH, zoom);
								smoketest_send_response("ok", cmd, nullptr);
							} else {
								smoketest_send_response("error", cmd, "sdl_save_failed");
							}
						} else {
							smoketest_send_response("error", cmd, "surface_alloc_failed");
						}
						delete off;
					}
#else
					smoketest_send_response("error", cmd, "not_sdl");
#endif
				}
			}
			else if (strncmp(cmd, "render_map_player ", 18) == 0) {
				// render_map_player <player> <path> [zoom]
				// Fogged, cropped-to-explored isometric render from ONE empire's
				// perspective, with infrastructure + unit/city sprites. Responds
				// "ok" with detail "crop=x,y,w,h" (the explored pixel rect).
				if (!m_gameLoaded) {
					smoketest_send_response("error", cmd, "game_not_loaded");
				} else {
#ifdef USE_SDL
					int  player = -1, zoom = 1;
					char pathBuf[1024] = {0};
					sscanf(cmd + 18, "%d %1023s %d", &player, pathBuf, &zoom);
					TiledMap *tm = tiledmap_Get();
					if (player < 0 || !pathBuf[0]) {
						smoketest_send_response("error", cmd, "bad_args");
					} else if (!tm) {
						smoketest_send_response("error", cmd, "no_map");
					} else {
						sint32 SW = 0, SH = 0;
						tm->FullMapPixelSize(zoom, &SW, &SH);
						AUI_ERRCODE err = AUI_ERRCODE_OK;
						aui_SDLSurface *off =
							new aui_SDLSurface(&err, SW, SH, 16, nullptr, FALSE);
						if (off && err == AUI_ERRCODE_OK && off->DDS()) {
							RECT crop = {0, 0, 0, 0};
							std::vector<TiledMap::CityLabel> labels;
							tm->RenderPlayerView(off, zoom, player, &crop, &labels);
							if (CTP2_SDL_SaveBMP(off->DDS(), pathBuf)) {
								// detail: "crop=x,y,w,h cities=px~py~owner~pop~name;..."
								std::string detail = "crop=" +
									std::to_string((int) crop.left) + "," +
									std::to_string((int) crop.top) + "," +
									std::to_string((int) (crop.right - crop.left)) + "," +
									std::to_string((int) (crop.bottom - crop.top));
								if (!labels.empty()) {
									detail += " cities=";
									for (size_t k = 0; k < labels.size(); ++k) {
										if (k) detail += ";";
										detail += std::to_string(labels[k].px) + "~" +
											std::to_string(labels[k].py) + "~" +
											std::to_string(labels[k].owner) + "~" +
											std::to_string(labels[k].pop) + "~" +
											labels[k].name;
									}
								}
								smoketest_send_response("ok", cmd, detail.c_str());
							} else {
								smoketest_send_response("error", cmd, "sdl_save_failed");
							}
						} else {
							smoketest_send_response("error", cmd, "surface_alloc_failed");
						}
						delete off;
					}
#else
					smoketest_send_response("error", cmd, "not_sdl");
#endif
				}
			}
			else if (strncmp(cmd, "move_unit ", 10) == 0) {
				if (m_gameLoaded) {
					int city_idx = 0;
					int dx = 0;
					int dy = 0;
					if (sscanf(cmd + 10, "%d %d %d", &city_idx, &dx, &dy) != 3) {
						smoketest_send_response("error", cmd, "bad_args");
					} else {
						Player *human = nullptr;
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
													if (army_QueueMovePath(human->GetOwner(), army, city_pos, dest)) {
														smoke_log->info("Moving unit from ({},{}) to ({},{})",
														(int)city_pos.x, (int)city_pos.y, (int)dest.x, (int)dest.y);
														moved = true;
													} else {
														smoke_log->warn("move_unit: no path from ({},{}) to ({},{})",
														(int)city_pos.x, (int)city_pos.y, (int)dest.x, (int)dest.y);
													}
													break;
												}
										}
									}
								}
								if (moved) {
									smoketest_send_response("ok", cmd, nullptr);
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
						Player *human = nullptr;
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
											m_game->GetEventsPtr()->AddEvent(GEV_INSERT_Tail,
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
							                        issued ? nullptr : "no_movable_unit");
						}
					}
				} else {
					smoketest_send_response("error", cmd, "game_not_loaded");
				}
			}
			else if (strcmp(cmd, "list_visible_units") == 0) {
				if (m_gameLoaded) {
					Player *human = nullptr;
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
										strlcat(detail, entry, sizeof(detail));
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
				smoketest_send_response("ok", cmd, nullptr);
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
	if((network_Get().IsActive() || network_Get().IsNetworkLaunch()) &&
		!network_Get().ReadyToStart())
		return 0;

	if(victorywin_IsOnScreen())
		return 0;

	if (c3ui_Get()->TheMouse()) {
		if( c3ui_Get()->TheMouse()->IsSuspended()
		&& !g_runInBackground
		&& !profiledb_Get()->GetValueByName("RunInBackground")
		){
			return 0;
		}
	}
	return 1;
}

sint32 CivApp::ProcessRobot(const uint32 target_milliseconds, uint32 &used_milliseconds)
{
	if((network_Get().IsActive() || network_Get().IsNetworkLaunch()) &&
		!network_Get().ReadyToStart())
		return 0;

	if(victorywin_IsOnScreen())
		return 0;

	uint32 const    start_time_ms = Os::GetTicks();

	if (c3ui_Get()->TheMouse())
	{
		if (c3ui_Get()->TheMouse()->IsSuspended() && !g_runInBackground && !profiledb_Get()->GetValueByName("RunInBackground"))
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
		network_Get().Process();
	}

	used_milliseconds   = Os::GetTicks() - start_time;
	return 0;
}

sint32 CivApp::ProcessSLIC()
{
	if (!m_game->GetSlicPtr())
		return 0;

	m_game->GetSlicPtr()->ProcessUITriggers();

	static time_t   lastRanSlicTimers   = 0;
    time_t          now                 = time(nullptr);
	if (now > lastRanSlicTimers + m_game->GetSlicPtr()->GetTimerGranularity())
    {
		m_game->GetSlicPtr()->RunTimerTriggers();
        /// @todo Check lastRanSlicTimers = now;
	}

	if (m_game->GetSlicPtr()->WaitingForLoad())
    {
		main_RestoreGame(m_game->GetSlicPtr()->GetLoadName());
	}

	return 0;
}

sint32 CivApp::ProcessProfile()
{
    uint32 target_milliseconds = (g_no_timeslice) ? 10000000 : 30;
	uint32 used_milliseconds;

	ProcessNet(target_milliseconds, used_milliseconds);

	ProcessUI(target_milliseconds, used_milliseconds);


	ProcessSLIC();

	ProcessRobot(target_milliseconds, used_milliseconds);

	if (soundmgr_Get())
		soundmgr_Get()->Process(target_milliseconds, used_milliseconds);

    return 0;
}

sint32 CivApp::Process()
{
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

	if (soundmgr_Get())
		soundmgr_Get()->Process(target_milliseconds, used_milliseconds);

	if(m_game->GetEventsPtr())
		m_game->GetEventsPtr()->Process();


	if (m_gameLoaded && g_savedGameRequest && selitem_Get())
    {
        Player *    p = player_Get(selitem_Get()->GetCurPlayer());
		if((p && !p->IsRobot())
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

sint32 CivApp::ProcessRenderTool()
{
	uint32 used_milliseconds = 0;
	return ProcessUI(0, used_milliseconds);
}

sint32 CivApp::StartGame()
{
	return InitializeGame();
}

sint32 CivApp::InitializeGameHeadless()
{
	civapp_log->info("InitializeGameHeadless: started");

	civapp_log->debug("calling sprite_Initialize()");
	sprite_Initialize();

	civapp_log->debug("calling gameEventManager_Initialize()");
	gameEventManager_Initialize();

	civapp_log->debug("calling m_game->GetEventsPtr()->Pause()");
	m_game->GetEventsPtr()->Pause();

	civapp_log->debug("calling events_Initialize()");
	events_Initialize();

	civapp_log->debug("calling gameinit_Initialize");
	if (!gameinit_Initialize(-1, -1)) {
		m_game->GetEventsPtr()->Resume();
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
	m_game->NewGame(
		profiledb_Get()->GetNPlayers(),
		diffutil_GetYearFromTurn(gamesettings_Get()->GetDifficulty(), 0)
	);

	m_gameLoaded = TRUE;

	civapp_log->debug("calling m_game->GetEventsPtr()->Resume + Process");
	m_game->GetEventsPtr()->Resume();
	m_game->GetEventsPtr()->Process();

	// Initialize AI subsystems (pathfinder, governors, scheduler, diplomat).
	// The interactive game does this in InitializeGame() via roboinit_Initalize
	// and CtpAi::Initialize(); the headless path must do the same or the AI
	// never makes decisions (settlers never settle, score stays flat).
	civapp_log->debug("calling roboinit_Initalize / CtpAi::Initialize");
	roboinit_Initalize();
	CtpAi::Initialize();

	civapp_log->info("InitializeGameHeadless: done (game loaded)");
	return 0;
}

sint32 CivApp::StartSpriteEditor()
{
	return InitializeSpriteEditor();
}







sint32 CivApp::EndGame()
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
	civapp_log->info("LoadSavedGame: starting load from '{}'", name);

	ProgressWindow::BeginProgress(
		g_theProgressWindow,
		"InitProgressWindow",
		1300 );

	ProgressTo( 10, stringdb_Get()->GetNameStr("LOADING") );

	FILE * fin = fopen(name, "r");
	if (fin == nullptr) {
		civapp_log->error("LoadSavedGame: could not open '{}'", name);
		ProgressWindow::EndProgress(g_theProgressWindow);
		c3errors_ErrorDialog("Load save game", "Could not open %s", name);
		return 1;
	}
	fclose(fin);
	civapp_log->info("LoadSavedGame: file '{}' exists and is readable", name);

	ProgressTo( 20 );

	if (m_gameLoaded) {
		civapp_log->info("LoadSavedGame: cleaning up current game before load");
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

	// Actor recreation for JSON-loaded units happens inside LoadJson
	// (json_save.cpp), shared with the headless and test-API load paths.
	if (!GameFile::RestoreGame(name)) {
		civapp_log->error("LoadSavedGame: restoration failed for '{}'", name);
		ProgressWindow::EndProgress(g_theProgressWindow);
		// Restore may have partially populated the replacement game. Dispose
		// of it before returning to the menu; it must never become playable.
		EndGame();
		c3errors_ErrorDialog("Load save game", "Could not restore %s", name);
		return 1;
	}

	ProgressTo( 1290 );

	tiledmap_Get()->InvalidateMap();

	ProgressTo( 1300 );

	if (!turn_Get()->IsHotSeat())
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

sint32 CivApp::RestartGame()
{
	if (m_gameLoaded) {
		CleanupGame(true);
		StartMessageSystem();
	}

	return StartGame();
}

sint32 CivApp::RestartGameSameMap()
{
	Assert(rand_ptr());
	g_oldRandSeed = rand_ptr() ? civrand().GetSeed() : 0;

	if (m_gameLoaded)
	{
		CleanupGame(true);
		StartMessageSystem();
	}

	if (profiledb_Get()->IsScenario())
	{
		spnewgamescreen_scenarioExitCallback(nullptr, AUI_BUTTON_ACTION_EXECUTE, 0, nullptr);
		return 0;
	}
	else
	{
		return StartGame();
	}
}

sint32 CivApp::QuitToSPShell()
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

sint32 CivApp::QuitToLobby()
{
#if !CTP2_ENABLE_NETWORKING
	return QuitToSPShell();
#else
	if (m_gameLoaded) {
		CleanupGame(false);
		StartMessageSystem();
	}

	return NetShell::Enter( k_NS_FLAGS_RETURN );
#endif
}

void CivApp::QuitGame()
{
	if (m_gameLoaded)
		CleanupGame(true);

	CleanupApp();
}


void CivApp::AutoSave(sint32 player, bool isQuickSave)
{
	if ((network_Get().IsActive() && !network_Get().IsHost()) || network_Get().IsNetworkLaunch())
		return;

	MBCHAR const *  autosaveItem    = (isQuickSave) ? "QUICKSAVE_NAME" : "AUTOSAVE_NAME";
	MBCHAR const *  autosaveName    = stringdb_Get()->GetNameStr(autosaveItem);

	MBCHAR			leaderName[k_MAX_NAME_LEN];
	// TODO(phase-2): strncpy → strlcpy — dst is char* or non-standard length, requires manual review
	strncpy(leaderName, profiledb_Get()->GetLeaderName(), SAVE_LEADER_NAME_SIZE);
	leaderName[SAVE_LEADER_NAME_SIZE] = '\0';
	c3files_StripSpaces(leaderName);

	MBCHAR			filename[_MAX_PATH];
	snprintf(filename, sizeof(filename), "%s-%s", autosaveName, leaderName);

	C3SAVEDIR       dir = (network_Get().IsActive()) ? C3SAVEDIR_MP : C3SAVEDIR_GAME;

	MBCHAR			path[_MAX_PATH];
	civpaths_Get()->GetSavePath(dir, path);

	MBCHAR			fullpath[_MAX_PATH];
	snprintf(fullpath, sizeof(fullpath), "%s%s%s", path, FILE_SEP, leaderName);

	if (c3files_PathIsValid(fullpath) || c3files_CreateDirectory(fullpath))
	{
		snprintf(fullpath, sizeof(fullpath), "%s%s%s%s%s", path, FILE_SEP, leaderName, FILE_SEP, filename);

		is_scenario_Set(FALSE);
		// SaveGame always writes JSON (Phase 0.C-1).
		GameFile::SaveGame(fullpath, nullptr);
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

	c3ui_Get()->AddAction(new LoadSaveGameAction(filename));
}


void CivApp::PostStartGameAction()
{
	c3ui_Get()->AddAction(new StartGameAction());
}

void CivApp::PostSpriteTestAction()
{
	c3ui_Get()->AddAction(new SpriteTestAction());
}

void CivApp::PostLoadSaveGameAction(MBCHAR const * name)
{
	c3ui_Get()->AddAction(new LoadSaveGameAction(name));
}

void CivApp::PostLoadQuickSaveAction(sint32 player)
{
	/// @todo Check network_Get().IsHost? See AutoSave. Otherwise, the dir assignment
	///       becomes constant.
	if (network_Get().IsActive())
	{
		return;
	}

	MBCHAR			leaderName[SAVE_LEADER_NAME_SIZE + 1];
	strlcpy(leaderName, profiledb_Get()->GetLeaderName(), sizeof(leaderName));
	c3files_StripSpaces(leaderName);

	MBCHAR			filename[_MAX_PATH];
	snprintf(filename, sizeof(filename), "%s-%s", stringdb_Get()->GetNameStr("QUICKSAVE_NAME"), leaderName);

	C3SAVEDIR       dir = (network_Get().IsActive()) ? C3SAVEDIR_MP : C3SAVEDIR_GAME;

	MBCHAR			path[_MAX_PATH];
	civpaths_Get()->GetSavePath(dir, path);

	MBCHAR			fullpath[_MAX_PATH];
	snprintf(fullpath, sizeof(fullpath), "%s%s%s", path, FILE_SEP, leaderName);

	if (c3files_PathIsValid(fullpath) || c3files_CreateDirectory(fullpath))
	{
		snprintf(fullpath, sizeof(fullpath), "%s%s%s%s%s", path, FILE_SEP, leaderName, FILE_SEP, filename);

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
	c3ui_Get()->AddAction(new LoadSaveGameMapAction(name));
}
#endif

void CivApp::PostRestartGameAction()
{
	c3ui_Get()->AddAction(new RestartGameAction());
}

void CivApp::PostRestartGameSameMapAction()
{
	Player * p = player_Get(selitem_Get()->GetVisiblePlayer());

	if (p && profiledb_Get())
	{
		profiledb_Get()->SetLeaderName(p->GetLeaderName());
		profiledb_Get()->SetCivIndex(p->GetCivilisation()->GetCivilisation());
	}

	c3ui_Get()->AddAction(new RestartGameSameMapAction());
}

void CivApp::PostQuitToSPShellAction()
{
	c3ui_Get()->AddAction(new QuitToSPShellAction());
}

void CivApp::PostQuitToLobbyAction()
{
	c3ui_Get()->AddAction(new QuitToLobbyAction());
}

void CivApp::PostEndGameAction()
{
	c3ui_Get()->AddAction(new EndGameAction());
}

void CivApp::PostLoadScenarioGameAction(MBCHAR const * filename)
{
	c3ui_Get()->AddAction(new LoadScenarioGameAction(filename));
}

void StartGameAction::Execute(aui_Control *control, uint32 action, uint32 data )
{
	civapp_Get()->StartGame();
}

void SpriteTestAction::Execute(aui_Control *control, uint32 action, uint32 data )
{
	civapp_Get()->StartSpriteEditor();
}

void LoadSaveGameAction::Execute(aui_Control *control, uint32 action, uint32 data )
{
	civapp_Get()->LoadSavedGame(m_filename);
}

#if 0   // never used
void LoadSaveGameMapAction::Execute(aui_Control *control, uint32 action, uint32 data )
{
	civapp_Get()->LoadSavedGameMap(m_filename);
}
#endif
void RestartGameAction::Execute(aui_Control *control, uint32 action, uint32 data )
{
	civapp_Get()->RestartGame();
}
void RestartGameSameMapAction::Execute(aui_Control *control, uint32 action, uint32 data )
{
	civapp_Get()->RestartGameSameMap();
}
void QuitToSPShellAction::Execute(aui_Control *control, uint32 action, uint32 data )
{
	civapp_Get()->QuitToSPShell();
}
void QuitToLobbyAction::Execute(aui_Control *control, uint32 action, uint32 data )
{
	civapp_Get()->QuitToLobby();
}
void EndGameAction::Execute(aui_Control *control, uint32 action, uint32 data)
{
	civapp_Get()->EndGame();
}
void LoadScenarioGameAction::Execute(aui_Control *control, uint32 action, uint32 data)
{
	civapp_Get()->LoadScenarioGame(m_filename);
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

    if (c3ui_Get()->PixelFormat() == AUI_SURFACE_PIXELFORMAT_555)
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
