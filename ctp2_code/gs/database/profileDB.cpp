//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Handling of user preferences.
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
// - _DEBUG
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - You need two changes here for a profile option
//
// - Option added to enable viewing info on actions that are too expensive.
// - Option added to close a messagebox automatically on eyepoint clicking.
// - Option added to choose a color set.
// - Option added to select which order buttons are displayed for an army.
// - Option added to select message adding style (top or bottom).
// - Option added to include multiple data directories.
// - Replaced old civilisation database by new one. (Aug 20th 2005 Martin G�hmann)
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
// - Option added to select whether an army is selected or a city is selected,
//   if both options are available. (Oct 8th 2005 Martin G�hmann)
// - DebugSlic and GoodAnim are now part of the advance options. (Oct 16th 2005 Martin G�hmann)
// - Added option to avoid an end turn if there are cities with empty build
//   queues. (Oct. 22nd 2005 Martin G�hmann)
// - Added option to allow end turn if the game runs in the background,
//   useful for automatic AI testing. (Oct. 22nd 2005 Martin G�hmann)
// - Options CityClick, EndTurnWithEmptyBuildQueues and RunInBackground
//   removed from advance options since they do not work. (May 21st 2006 Martin G�hmann)
// - Made automatic treaty ending an option.
// - Option added to select between square and smooth borders. (Feb 4th 2007 Martin G�hmann)
// - Added additional options, most to be implemented later
// - Implemented NRG - option to ccalculate energy ratio affecting production and demand
// - Added DebugAI option
// - Made the upgrade option show up in the debug version. (19-May-2007 Martin G�hmann)
// - Added debug pathing option for the city astar. (17-Jan-2008 Martin G�hmann)
// - Added a new combat option (28-Feb-2009 Maq)
// - Added a no goody huts option (20-Mar-2009 Maq)
// - Added random map settings option. (5-Apr-2009 Maq)
// - Cleaned up advanced options window by removing options already present
//	 in other windows. (10-Apr-2009 Maq)
// - Added start and end age options. (11-Apr-2009 Maq)
// - Added show city production under name option. (15-Apr-2009 Maq)
// - Added show political map button options. (6-Jul-2009 EPW)
// - Removed the AI specific rules from profile, since they already exist in
//   difficultyDB. (25-Jul-2009 Maq)
// -Added display capital stull (5-Jan-10 EPW)
// -Added display relations options (7-Jan-10 EPW)
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/database/profileDB.h"

#include "gs/gameobj/AgreementData.h"      // k_EXPIRATION_NEVER
#include "ctp/ctp2_utils/c3errors.h"
#include "gs/gameobj/Civilisation.h"
#include "gs/fileio/CivPaths.h"           // civpaths_Get()
#include "gs/gameobj/Diffcly.h"
#include "DifficultyRecord.h"   // g_theDifficultyDB
#include "gs/gameobj/Diplomacy_Log.h"
#include "gs/gameobj/GameSettings.h"       // gamesettings_Get()
#include "gs/utility/Globals.h"
#include "gs/gameobj/Player.h"             // player_Get()
#include "gs/core/audio_types.h"
#include "gs/core/audio_observer.h"
#include "gs/database/StrDB.h"              // g_theStringDB
#include "gs/fileio/Token.h"

extern Diplomacy_Log *      g_theDiplomacyLog;

namespace
{
sint32 const                AUDIO_VOLUME_DEFAULT        = 8;
sint32 const                PLAYER_COUNT_DEFAULT        = 3;
sint32 const                PLAYER_COUNT_MAX_DEFAULT    = 16;
sint32 const                SLIDER_MIDDLE               = 5;

sint32 const                USE_UNKNOWN                 = 0;
}

ProfileDB::ProfileDB()
:
    m_nPlayers                          (PLAYER_COUNT_DEFAULT),
    m_ai_on                             (FALSE),
    m_use_nice_start                    (FALSE),
    m_use_map_plugin                    (FALSE),
    m_use_ipx                           (FALSE),
    m_setupRadius                       (0),
    m_powerPoints                       (0),
    m_difficulty                        (0),
    m_risklevel                         (0),
    m_genocide                          (FALSE),
    m_trade                             (FALSE),
    m_simplecombat                      (FALSE),
    m_pollution                         (FALSE),
    m_lineofsight                       (FALSE),
    m_unitAnim                          (FALSE),
    m_goodAnim                          (TRUE),
    m_tradeAnim                         (FALSE),
    m_waterAnim                         (FALSE),
    m_libraryAnim                       (FALSE),
    m_wonderMovies                      (FALSE),
    m_bounceMessage                     (FALSE),
    m_messageAdvice                     (FALSE),
    m_tutorialAdvice                    (FALSE),
    m_enemyMoves                        (TRUE),
    m_revoltWarning                     (FALSE),
    m_enemyIntrude                      (FALSE),
    m_unitLostWarning                   (FALSE),
    m_tradeLostWarning                  (FALSE),
    m_cityLostWarning                   (FALSE),
    m_autocenter                        (TRUE),
    m_fullScreenMovies                  (FALSE),
    m_showCityInfluence                 (FALSE),
    m_invulnerableTrade                 (FALSE),
    m_fogOfWar                          (FALSE),
    m_startType                         (FALSE),
    m_autoSave                          (FALSE),
    m_playerNumber                      (1),
    m_civIndex                          (PLAYER_COUNT_MAX_DEFAULT),
    m_gender                            (GENDER_MALE),
    m_isSaved                           (FALSE),
    m_isScenario                        (FALSE),
    m_noHumansOnHost                    (FALSE),
    m_logPlayerStats                    (FALSE),
    m_sfxVolume                         (AUDIO_VOLUME_DEFAULT),
    m_voiceVolume                       (AUDIO_VOLUME_DEFAULT),
    m_musicVolume                       (AUDIO_VOLUME_DEFAULT),
    m_xWrap                             (TRUE),
    m_yWrap                             (FALSE),
    m_autoGroup                         (FALSE),
    m_autoDeselect                      (FALSE),
    m_autoSelectNext                    (FALSE),
    m_autoSelectFirstUnit               (FALSE),
    m_autoTurnCycle                     (FALSE),
    m_combatLog                         (FALSE),
    m_useLeftClick                      (FALSE),
    m_showZoomedCombat                  (FALSE),
    m_useFingerprinting                 (FALSE),
    m_useRedbookAudio                   (FALSE),
    m_requireCD                         (FALSE),
    m_protected                         (FALSE),
    m_tryWindowsResolution              (TRUE),
    m_useDirectXBlitter                 (TRUE),
    m_screenResWidth                    (640),
    m_screenResHeight                   (480),
    m_zoomedCombatAlways                (FALSE),
    m_attackEveryone                    (FALSE),
    m_nonRandomCivs                     (FALSE),
    m_autoEndMultiple                   (FALSE),
    m_wetdry                            (SLIDER_MIDDLE),
    m_warmcold                          (SLIDER_MIDDLE),
    m_oceanland                         (SLIDER_MIDDLE),
    m_islandcontinent                   (SLIDER_MIDDLE),
    m_homodiverse                       (SLIDER_MIDDLE),
    m_goodcount                         (SLIDER_MIDDLE),
    m_throneRoom                        (FALSE),
    m_max_players                       (PLAYER_COUNT_MAX_DEFAULT),
    m_mapSize                           (MAPSIZE_MEDIUM),
    m_alienEndGame                      (TRUE),
    m_allow_ai_settle_move_cheat        (FALSE),
    m_unitCompleteMessages              (FALSE),
    m_nonContinuousUnitCompleteMessages (FALSE),
    m_debugSlic                         (FALSE),
    m_debugSlicEvents                   (FALSE),
    m_dontKillMessages                  (FALSE),
    m_aiPopCheat                        (TRUE),
    m_showCityNames                     (TRUE),
    m_showArmyNames                     (FALSE),
    m_showTradeRoutes                   (TRUE),
    m_unitSpeed                         (1),
    m_mouseSpeed                        (SLIDER_MIDDLE),
    m_leftHandedMouse                   (FALSE),
    m_cityBuiltMessage                  (TRUE),
    m_useAttackMessages                 (FALSE),
    m_useOldRegisterClick               (FALSE),
    m_useCTP2Mode                       (TRUE),
    m_is_diplomacy_log_on               (FALSE),
    m_cheat_age                         (0),
    m_autoSwitchTabs                    (TRUE),
    m_showPoliticalBorders              (TRUE),
    m_moveHoldTime                      (500),
    m_battleSpeed                       (SLIDER_MIDDLE),
    m_showEnemyHealth                   (TRUE),
    m_scrollDelay                       (0),
    m_autoRenameCities                  (FALSE),
    m_autoOpenCityWindow                (TRUE),
    m_endTurnSound                      (TRUE),
    m_enableLogs                        (TRUE),
    m_displayUnits                      (TRUE),
    m_displayCities                     (TRUE),
    m_displayBorders                    (TRUE),
    m_displayFilter                     (TRUE),
    m_displayTrade                      (TRUE),
    m_displayTerrain                    (TRUE),
	m_displayPolitical                  (FALSE),
	m_displayCapitols                   (FALSE),
	m_displayRelations                  (FALSE),
    m_forest                            (USE_UNKNOWN),
    m_grass                             (USE_UNKNOWN),
    m_plains                            (USE_UNKNOWN),
    m_desert                            (USE_UNKNOWN),
    m_whitePercent                      (USE_UNKNOWN),
    m_brownPercent                      (USE_UNKNOWN),
    m_temperatureRangeAdjust            (USE_UNKNOWN),
    m_land                              (USE_UNKNOWN),
    m_continent                         (USE_UNKNOWN),
    m_homogenous                        (USE_UNKNOWN),
    m_richness                          (USE_UNKNOWN),
    m_closeEyepoint                     (FALSE),
    m_colorSet                          (0),
    m_showExpensive                     (FALSE),
    m_showOrderUnion                    (FALSE),
    m_recentAtTop                       (FALSE),
    m_cityClick                         (FALSE),
    m_dontSave                          (FALSE),
    m_endTurnWithEmptyBuildQueues       (FALSE),
    m_runInBackground                   (FALSE),
    m_autoExpireTreatyTurn              (k_EXPIRATION_NEVER),
    m_cityCaptureOptions                (FALSE),
    m_upgrade                           (FALSE),
    m_smoothBorders                     (FALSE),
    m_CivFlags                          (TRUE),
    m_AICityDefenderBonus               (FALSE),
    m_BarbarianCities                   (FALSE),
    m_SectarianHappiness                (FALSE),
    m_RevoltCasualties                  (FALSE),
    m_RevoltInsurgents                  (FALSE),
    m_BarbarianCamps                    (FALSE),
    m_BarbarianSpawnsBarbarian          (FALSE),
    m_AINoSinking                       (FALSE),
    m_GoldPerUnitSupport                (FALSE),
    m_GoldPerCity                       (FALSE),
    m_AIMilitiaUnit                     (FALSE),
    m_OneCityChallenge                  (FALSE),
    m_NRG                               (FALSE),
    m_debugai                           (FALSE),
    m_ruin                              (FALSE),
    m_NoCityLimit                       (FALSE),
    m_DebugCityAstar                    (FALSE),
    m_newcombat                         (TRUE),
    m_noGoodyHuts                       (FALSE),
    m_randomCustomMap                   (FALSE),
    m_spStartingAge                     (0),
    m_spEndingAge                       (-1),
    m_showCityProduction                (TRUE),
    // Add above this line new profile options
    m_vars                              (new PointerList<ProfileVar>),
    m_loadedFromTutorial                (FALSE)
{
	for (auto & player : m_ai_personality)
	{
		player[0] = 0;
	}

	m_gameName[0]           = 0;
	m_leaderName[0]         = 0;
	m_civName[0]            = 0;
	m_saveNote[0]           = 0;
	m_ruleSets[0]           = 0;
	m_gameWatchDirectory[0] = 0;

	for (size_t map_pass = 0; map_pass < k_NUM_MAP_PASSES; ++map_pass)
	{
		m_map_plugin_name[map_pass][0]  = 0;
		m_map_settings[map_pass]        = nullptr;
	};

	Var("NumPlayers"                 , PV_NUM   , &m_nPlayers                   , nullptr, false);
	Var("AiOn"                       , PV_BOOL  , &m_ai_on                      , nullptr, false);
	Var("UseNiceStart"               , PV_BOOL  , &m_use_nice_start             , nullptr, false);
	Var("UseMapPlugin"               , PV_BOOL  , &m_use_map_plugin             , nullptr, false);

	Var("Difficulty"                 , PV_NUM   , &m_difficulty                 , nullptr, false);
	Var("RiskLevel"                  , PV_NUM   , &m_risklevel                  , nullptr, false);

	Var("Pollution"                  , PV_BOOL  , &m_pollution                  , nullptr, false);
	Var("UnitAnim"                   , PV_BOOL  , &m_unitAnim                   , nullptr, false);

	Var("GoodAnim"                   , PV_BOOL  , &m_goodAnim                   , nullptr, false);
	Var("TradeAnim"                  , PV_BOOL  , &m_tradeAnim                  , nullptr, false);
	Var("WaterAnim"                  , PV_BOOL  , &m_waterAnim                  , nullptr, false);
	Var("LibraryAnim"                , PV_BOOL  , &m_libraryAnim                , nullptr, false);
	Var("WonderMovies"               , PV_BOOL  , &m_wonderMovies               , nullptr, false);
	Var("BounceMessage"              , PV_BOOL  , &m_bounceMessage              , nullptr, false);
	Var("MessageAdvice"              , PV_BOOL  , &m_messageAdvice              , nullptr, false);
	Var("TutorialAdvice"             , PV_BOOL  , &m_tutorialAdvice             , nullptr, false);
	Var("EnemyMoves"                 , PV_BOOL  , &m_enemyMoves                 , nullptr, false);
	Var("RevoltWarning"              , PV_BOOL  , &m_revoltWarning              , nullptr, false);
	Var("EnemyIntrude"               , PV_BOOL  , &m_enemyIntrude               , nullptr, false);
	Var("UnitLostWarning"            , PV_BOOL  , &m_unitLostWarning            , nullptr, false);
	Var("TradeLostWarning"           , PV_BOOL  , &m_tradeLostWarning           , nullptr, false);
	Var("CityLostWarning"            , PV_BOOL  , &m_cityLostWarning            , nullptr, false);
	Var("AutoCenter"                 , PV_BOOL  , &m_autocenter                 , nullptr, false);
	Var("FullScreenMovies"           , PV_BOOL  , &m_fullScreenMovies           , nullptr, false);
	Var("AutoSave"                   , PV_BOOL  , &m_autoSave					, nullptr, false);
	Var("PlayerNumber"               , PV_NUM   , (sint32 *)&m_playerNumber     , nullptr, false);

	Var("CivIndex"                   , PV_NUM   , (sint32 *)&m_civIndex         , nullptr, false);

	Var("GameName"                   , PV_STRING, nullptr, (char *)m_gameName            , false);
	Var("LeaderName"                 , PV_STRING, nullptr, (char*)m_leaderName           , false);
	Var("CivName"                    , PV_STRING, nullptr, (char*)m_civName              , false);
	Var("SaveNote"                   , PV_STRING, nullptr, (char*)m_saveNote             , false);
	Var("Gender"                     , PV_NUM   , (sint32 *)&m_gender           , nullptr, false);

	Var("NoHumansOnHost"             , PV_BOOL  , &m_noHumansOnHost             , nullptr, false);
	Var("LogPlayerStats"             , PV_BOOL  , &m_logPlayerStats             , nullptr, false);

	Var("SfxVolume"                  , PV_NUM   , &m_sfxVolume                  , nullptr, false);
	Var("VoiceVolume"                , PV_NUM   , &m_voiceVolume                , nullptr, false);
	Var("MusicVolume"                , PV_NUM   , &m_musicVolume                , nullptr, false);

	Var("XWrap"                      , PV_BOOL  , &m_xWrap                      , nullptr, false);
	Var("YWrap"                      , PV_BOOL  , &m_yWrap                      , nullptr, false);
	Var("AutoGroup"                  , PV_BOOL  , &m_autoGroup                  , nullptr, false);
	Var("AutoDeselect"               , PV_BOOL  , &m_autoDeselect               , nullptr);
	Var("AutoSelectNext"             , PV_BOOL  , &m_autoSelectNext             , nullptr);
	Var("AutoSelectFirstUnit"        , PV_BOOL  , &m_autoSelectFirstUnit        , nullptr);
	Var("AutoTurnCycle"              , PV_BOOL  , &m_autoTurnCycle              , nullptr, false);
	Var("CombatLog"                  , PV_BOOL  , &m_combatLog                  , nullptr, false);

	Var("UseLeftClick"               , PV_BOOL  , &m_useLeftClick               , nullptr, false);
	Var("ShowZoomedCombat"           , PV_BOOL  , &m_showZoomedCombat           , nullptr, false);
	Var("UseFingerPrinting"          , PV_BOOL  , &m_useFingerprinting          , nullptr, false);
	Var("UseRedbookAudio"            , PV_BOOL  , &m_useRedbookAudio            , nullptr, false);
	Var("RequireCD"                  , PV_BOOL  , &m_requireCD                  , nullptr, false);
	Var("Prophylaxis"                , PV_BOOL  , &m_protected                  , nullptr, false);
	Var("TryWindowsResolution"       , PV_BOOL  , &m_tryWindowsResolution       , nullptr, false);
	Var("UseDirectXBlitter"          , PV_BOOL  , &m_useDirectXBlitter          , nullptr, false);
	Var("ScreenResWidth"             , PV_NUM   , &m_screenResWidth             , nullptr, false);
	Var("ScreenResHeight"            , PV_NUM   , &m_screenResHeight            , nullptr, false);

	Var("ZoomedCombatAlways"         , PV_BOOL  , &m_zoomedCombatAlways         , nullptr, false);
	Var("AttackEveryone"             , PV_BOOL  , &m_attackEveryone             , nullptr, false);
	Var("NonRandomCivs"              , PV_BOOL  , &m_nonRandomCivs              , nullptr, false);
	Var("GameWatchDirectory"         , PV_STRING, nullptr, (char*)m_gameWatchDirectory   , false);
	Var("AutoEndMultiple"            , PV_BOOL  , &m_autoEndMultiple            , nullptr);

	Var("WetDry"                     , PV_NUM   , &m_wetdry                     , nullptr, false);
	Var("WarmCold"                   , PV_NUM   , &m_warmcold                   , nullptr, false);
	Var("OceanLand"                  , PV_NUM   , &m_oceanland                  , nullptr, false);
	Var("IslandContinent"            , PV_NUM   , &m_islandcontinent            , nullptr, false);
	Var("HomoDiverse"                , PV_NUM   , &m_homodiverse                , nullptr, false);
	Var("GoodCount"                  , PV_NUM   , &m_goodcount                  , nullptr, false);

	Var("ThroneRoom"                 , PV_BOOL  , &m_throneRoom                 , nullptr, false);
	Var("MaxPlayers"                 , PV_NUM   , &m_max_players                , nullptr, false);
	Var("MapSize"                    , PV_NUM   , (sint32 *)&m_mapSize          , nullptr, false);

	Var("AlienEndGame"               , PV_BOOL  , &m_alienEndGame               , nullptr, false);
	Var("UnitCompleteMessages"       , PV_BOOL  , &m_unitCompleteMessages       , nullptr);
	Var("NonContinuousUnitCompleteMessages", PV_BOOL  , &m_nonContinuousUnitCompleteMessages, nullptr);
	Var("DebugSlic"                  , PV_BOOL  , &m_debugSlic                  , nullptr);
	Var("DebugSlicEvents"            , PV_BOOL  , &m_debugSlicEvents            , nullptr);
	Var("DiplomacyLog"               , PV_BOOL  , &m_is_diplomacy_log_on        , nullptr, false);
	Var("CheatAge"                   , PV_NUM   , &m_cheat_age                  , nullptr, false);
	Var("DontKillMessages"           , PV_BOOL  , &m_dontKillMessages           , nullptr, false);
	Var("AIPopCheat"                 , PV_BOOL  , &m_aiPopCheat                 , nullptr, false);
	Var("ShowCityNames"              , PV_BOOL  , &m_showCityNames              , nullptr, false);
	Var("ShowArmyNames"              , PV_BOOL  , &m_showArmyNames              , nullptr, false);
	Var("ShowTradeRoutes"            , PV_BOOL  , &m_showTradeRoutes            , nullptr, false);

	Var("UnitSpeed"                  , PV_NUM   , &m_unitSpeed                  , nullptr, false);
	Var("MouseSpeed"                 , PV_NUM   , &m_mouseSpeed                 , nullptr, false);
	Var("LeftHandedMouse"            , PV_BOOL  , &m_leftHandedMouse            , nullptr, false);

	Var("CityBuiltMessage"           , PV_BOOL  , &m_cityBuiltMessage           , nullptr, false);
	Var("UseAttackMessages"          , PV_BOOL  , &m_useAttackMessages          , nullptr, false);

	Var("MapPlugin0"                 , PV_STRING, nullptr, (char *)m_map_plugin_name[0]  , false);
	Var("MapPlugin1"                 , PV_STRING, nullptr, (char *)m_map_plugin_name[1]  , false);
	Var("MapPlugin2"                 , PV_STRING, nullptr, (char *)m_map_plugin_name[2]  , false);
	Var("MapPlugin3"                 , PV_STRING, nullptr, (char *)m_map_plugin_name[3]  , false);

	Var("OldRegisterClick"           , PV_BOOL  , &m_useOldRegisterClick        , nullptr, false);
	Var("CTP2Mode"                   , PV_BOOL  , &m_useCTP2Mode                , nullptr, false);
	Var("MoveHoldTime"               , PV_NUM   , &m_moveHoldTime               , nullptr, false);

	Var("BattleSpeed"                , PV_NUM   , &m_battleSpeed                , nullptr);

	Var("ScrollDelay"                , PV_NUM   , &m_scrollDelay                , nullptr);

	Var("AutoSwitchTabs"             , PV_BOOL  , &m_autoSwitchTabs             , nullptr);
	Var("AutoRenameCities"           , PV_BOOL  , &m_autoRenameCities           , nullptr, false);
	Var("AutoOpenCityWindow"         , PV_BOOL  , &m_autoOpenCityWindow         , nullptr);

	Var("ShowEnemyHealth"            , PV_BOOL  , &m_showEnemyHealth            , nullptr, false);

	Var("ShowCityInfluence"          , PV_BOOL  , &m_showCityInfluence          , nullptr, false);
	Var("ShowPoliticalBorders"       , PV_BOOL  , &m_showPoliticalBorders       , nullptr, false);

	Var("GoodRichness"               , PV_NUM   , &m_richness                   , nullptr, false);
	Var("EndTurnSound"               , PV_BOOL  , &m_endTurnSound               , nullptr);
	Var("EnableLogs"                 , PV_BOOL  , &m_enableLogs                 , nullptr, false);
	Var("DisplayUnits"               , PV_BOOL  , &m_displayUnits               , nullptr, false);
	Var("DisplayCities"              , PV_BOOL  , &m_displayCities              , nullptr, false);
	Var("DisplayBorders"             , PV_BOOL  , &m_displayBorders             , nullptr, false);
	Var("DisplayFilter"              , PV_BOOL  , &m_displayFilter              , nullptr, false);
	Var("DisplayTrade"               , PV_BOOL  , &m_displayTrade               , nullptr, false);
	Var("DisplayTerrain"             , PV_BOOL  , &m_displayTerrain             , nullptr, false);
	Var("DisplayPolitical"           , PV_BOOL  , &m_displayPolitical           , nullptr, false);
	Var("DisplayCapitols"            , PV_BOOL  , &m_displayCapitols            , nullptr, false);
	Var("DisplayRelations"           , PV_BOOL  , &m_displayRelations           , nullptr, false);
	Var("CloseOnEyepoint"            , PV_BOOL  , &m_closeEyepoint              , nullptr);
	Var("ShowExpensive"              , PV_BOOL  , &m_showExpensive              , nullptr);
	Var("ColorSet"                   , PV_NUM   , &m_colorSet                   , nullptr, false);
	Var("ShowOrderUnion"             , PV_BOOL  , &m_showOrderUnion             , nullptr);
	Var("RecentAtTop"                , PV_BOOL  , &m_recentAtTop                , nullptr);
	Var("RuleSets"                   , PV_STRING, nullptr, m_ruleSets                    , false);
	Var("CityClick"                  , PV_BOOL  , &m_cityClick                  , nullptr, false);
	Var("EndTurnWithEmptyBuildQueues", PV_BOOL  , &m_endTurnWithEmptyBuildQueues, nullptr, false);
	Var("RunInBackground"            , PV_BOOL  , &m_runInBackground            , nullptr, false);
	Var("AutoExpireTreatyBase"       , PV_NUM   , &m_autoExpireTreatyTurn       , nullptr, false);
	Var("CityCaptureOptions"         , PV_BOOL  , &m_cityCaptureOptions         , nullptr, false);
#if defined(_DEBUG)
	/// @todo Move this to the scenario editor
	Var("Upgrade"                    , PV_BOOL  , &m_upgrade                    , nullptr);
#else
	Var("Upgrade"                    , PV_BOOL  , &m_upgrade                    , NULL, false);
#endif
	Var("SmoothBorders"              , PV_BOOL  , &m_smoothBorders              , nullptr, false);
	// emod new profile flags // Please make sure that only those show up which are used.
	Var("CivFlags"                   , PV_BOOL  , &m_CivFlags                   , nullptr, false);
	Var("AICityDefenderBonus"        , PV_BOOL  , &m_AICityDefenderBonus        , nullptr, false);
	Var("BarbarianCities"            , PV_BOOL  , &m_BarbarianCities            , nullptr, false);
	Var("SectarianHappiness"         , PV_BOOL  , &m_SectarianHappiness         , nullptr, false);
	Var("RevoltCasualties"           , PV_BOOL  , &m_RevoltCasualties           , nullptr, false);
	Var("RevoltInsurgents"           , PV_BOOL  , &m_RevoltInsurgents           , nullptr, false);
	Var("BarbarianCamps"             , PV_BOOL  , &m_BarbarianCamps	            , nullptr, false);
	Var("BarbarianSpawnsBarbarian"   , PV_BOOL  , &m_BarbarianSpawnsBarbarian   , nullptr, false);
	Var("AINoSinking"                , PV_BOOL  , &m_AINoSinking                , nullptr, false);
	Var("GoldPerUnitSupport"         , PV_BOOL  , &m_GoldPerUnitSupport         , nullptr, false);
	Var("GoldPerCity"                , PV_BOOL  , &m_GoldPerCity                , nullptr, false);
	Var("AIMilitiaUnit"              , PV_BOOL  , &m_AIMilitiaUnit              , nullptr, false);
	Var("OneCityChallenge"           , PV_BOOL  , &m_OneCityChallenge           , nullptr, false);
	Var("EnergySupply&DemandRatio"   , PV_BOOL  , &m_NRG                        , nullptr, false);
	Var("ShowDebugAI"                , PV_BOOL  , &m_debugai                    , nullptr, false);
	Var("CitiesLeaveRuins"           , PV_BOOL  , &m_ruin                       , nullptr, false);
	Var("NoCityLimit"                , PV_BOOL  , &m_NoCityLimit                , nullptr, false);
	Var("DebugCityAstar"             , PV_BOOL  , &m_DebugCityAstar             , nullptr);
	Var("NewCombat"                  , PV_BOOL  , &m_newcombat                  , nullptr, false);
	Var("NoGoodyHuts"                , PV_BOOL  , &m_noGoodyHuts                , nullptr, false);
	Var("RandomCustomMap"            , PV_BOOL  , &m_randomCustomMap            , nullptr, false);
	Var("SPStartingAge"              , PV_NUM   , &m_spStartingAge              , nullptr, false);
	Var("SPEndingAge"				 , PV_NUM   , &m_spEndingAge                , nullptr, false);
	Var("ShowCityProduction"         , PV_BOOL  , &m_showCityProduction         , nullptr, false);
}

void ProfileDB::DefaultSettings()
{
	StringId    leaderNameId = g_theCivilisationDB->Get(m_civIndex)->GetLeaderNameMale();
	StringId    civNameId = g_theCivilisationDB->Get(m_civIndex)->GetPluralCivName();

	strlcpy(m_leaderName, stringdb_Get()->GetNameStr(leaderNameId), sizeof(m_leaderName));
	strlcpy(m_civName, stringdb_Get()->GetNameStr(civNameId), sizeof(m_civName));
}

ProfileDB::~ProfileDB()
{
	Save();

	if (m_vars)
	{
		m_vars->DeleteAll();
		delete m_vars;
	}
}

BOOL ProfileDB::Init(BOOL forTutorial)
{
	MBCHAR profileName[_MAX_PATH];
	MBCHAR *profileTxtFile;

	if (forTutorial)
	{
		m_loadedFromTutorial = TRUE;
		profileTxtFile = civpaths_Get()->FindFile(C3DIR_GAMEDATA,
		                                      "tut_profile.txt", profileName);
	}
	else
	{
		profileTxtFile = civpaths_Get()->FindFile(C3DIR_DIRECT, "userprofile.txt",
		                                      profileName);
		if (!profileTxtFile || !c3files_PathIsValid(profileTxtFile))
		{
			profileTxtFile = civpaths_Get()->FindFile(C3DIR_GAMEDATA,
			                                      "profile.txt", profileName);
		}
	}

	if (profileTxtFile)
	{
		FILE * pro_file = c3files_fopen(C3DIR_DIRECT, profileTxtFile, "r");

		if (pro_file)
		{
			sint32 const    saved_width     = m_screenResWidth;
			sint32 const    saved_height    = m_screenResHeight;
			BOOL const      res             = Parse(pro_file);
			fclose(pro_file);

			if (res)
			{
				Save();
			}

			if (forTutorial)
			{
				m_screenResWidth = saved_width;
				m_screenResHeight = saved_height;
			}

			return res;
		}
	}
	else
	{
		m_nPlayers  = PLAYER_COUNT_DEFAULT;
		m_ai_on     = FALSE;
	}

	return FALSE;
}

BOOL ProfileDB::Parse(FILE *file)
{
	char line[k_MAX_NAME_LEN];
	sint32 linenum = 0;
	while(!feof(file)) {
		if(fgets(line, k_MAX_NAME_LEN, file) == nullptr)
			return TRUE;
		linenum++;
		sint32 len = strlen(line);

		while(len > 0 && isspace(line[len - 1])) {
			line[len - 1] = 0;
			len--;
		}

		char *  name  = line;
		while(isspace(*name) && *name != 0) {
			name++;
		}
		if(*name == 0 || *name == '#') {
			continue;
		}
		if(!isalpha(*name)) {
			c3errors_ErrorDialog("Profile", "Line %d: name must start with a letter", linenum);
			return FALSE;
		}

		char *  value = name + 1;
		while(*value != '=' && *value != 0) {
			if(isspace(*value))
				*value = 0;
			value++;
		}
		if(*value != '=') {
			c3errors_ErrorDialog("Profile", "Line %d: no = found", linenum);
			return FALSE;
		}
		*value = 0;
		value++;
		while(isspace(*value) && *value != 0) {
			value++;
		}





		PointerList<ProfileVar>::Walker walk(m_vars);
		bool found = false;
		while(walk.IsValid() && !found) {
			ProfileVar *var = walk.GetObj();
			if(stricmp(var->m_name, name) == 0) {
				found = true;
				switch(var->m_type) {
					case PV_BOOL:
						if(*value == 0) {
							c3errors_ErrorDialog("Profile", "Line %d: no value found", linenum);
							return FALSE;
						}
						if(stricmp(value, "yes") == 0 ||
						   stricmp(value, "true") == 0 ||
						   stricmp(value, "1") == 0) {
							*var->m_numValue = 1;
						} else if(stricmp(value, "no") == 0 ||
								  stricmp(value, "false") == 0 ||
								  stricmp(value, "0") == 0) {
							*var->m_numValue = 0;
						} else {
							c3errors_ErrorDialog("Profile", "Line %d: %s is an illegal value for %s", linenum, value, var->m_name);
							return FALSE;
						}
						break;
					case PV_NUM:
						if(*value == 0) {
							c3errors_ErrorDialog("Profile", "Line %d: no value found", linenum);
							return FALSE;
						}
						*var->m_numValue = atoi(value);
						break;
					case PV_STRING:
						if(strlen(value) > k_MAX_NAME_LEN - 1) {
							c3errors_ErrorDialog("Profile", "Line %d: string too long", linenum);
							return FALSE;
						}
						// TODO(strlcpy): unknown dst size
						strcpy(var->m_stringValue, value);
						break;
					default:
						Assert(FALSE);
				}
			}
			walk.Next();
		}
	}

	return TRUE;
}

void ProfileDB::SetTutorialAdvice( BOOL val )
{
	m_tutorialAdvice = val;
}

void ProfileDB::SetDiplmacyLog(BOOL b)
{
	if (b == m_is_diplomacy_log_on)
	{
		// No action: keep current log status
	}
	else
	{
		delete g_theDiplomacyLog;
		g_theDiplomacyLog       = b ? new Diplomacy_Log : nullptr;
		m_is_diplomacy_log_on   = b;
	}
}

void ProfileDB::SetPollutionRule( BOOL rule )
{
	m_pollution = rule;

	if (GameSettings *gs = gamesettings_Get()) {
		gs->SetPollution( m_pollution );
	}
}

void ProfileDB::SetSFXVolume(sint32 vol)
{
	m_sfxVolume = vol;
	audio_observer::SetVolume((sint32)SOUNDTYPE_SFX, vol);
}

void ProfileDB::SetVoiceVolume(sint32 vol)
{
	m_voiceVolume = vol;
	audio_observer::SetVolume((sint32)SOUNDTYPE_VOICE, vol);
}

void ProfileDB::SetMusicVolume(sint32 vol)
{
	m_musicVolume = vol;
	audio_observer::SetVolume((sint32)SOUNDTYPE_MUSIC, vol);
}

void ProfileDB::SetDifficulty(uint32 x)
{
	Assert((x>=0) && (x<7));
	if(x >= 0 && x < 7)
	{
		m_difficulty = x;
		if (player_arr_Get())
		{
			for (sint32 p = 0; p < k_MAX_PLAYERS; p++)
			{
				if (player_Get(p))
				{
					delete player_Get(p)->m_difficulty;
					player_Get(p)->m_difficulty =
					    new Difficulty(x,
					                   p,
					                   !player_Get(p)->IsRobot()
					                  );
				}
			}
		}
	}
}

void ProfileDB::Var(char *name, PROF_VAR_TYPE type, sint32 *numValue,
                    char *stringValue, bool visible)
{
	m_vars->AddTail(new ProfileVar(name, type, numValue, stringValue, visible));
}

void ProfileDB::Save()
{
	if(m_loadedFromTutorial || m_dontSave) {
		return;
	}

	FILE *file = c3files_fopen(C3DIR_DIRECT, "userprofile.txt", "w");
	if(file) {
		PointerList<ProfileVar>::Walker walk(m_vars);
		while(walk.IsValid()) {
			ProfileVar *var = walk.GetObj();
			fprintf(file, "%s=", var->m_name);

			switch(var->m_type) {
				case PV_BOOL:
					if(*var->m_numValue) {
						fprintf(file, "Yes\n");
					} else {
						fprintf(file, "No\n");
					}
					break;
				case PV_NUM:
					fprintf(file, "%d\n", *var->m_numValue);
					break;
				case PV_STRING:
					fprintf(file, "%s\n", var->m_stringValue);
					break;
			}
			walk.Next();
		}
		fclose(file);
	}
}

sint32 ProfileDB::GetValueByName(const char * name) const
{
	for
	(
	    PointerList<ProfileVar>::Walker walk(m_vars);
	    walk.IsValid();
	    walk.Next()
	)
	{
		ProfileVar *    var = walk.GetObj();
		if (stricmp(var->m_name, name) == 0)
		{
			if (var->m_type == PV_BOOL || var->m_type == PV_NUM)
			{
				return *var->m_numValue;
			}
			else
			{
				// This function only works for boolean or integer values
				return 0;
			}
		}
	}

	return 0;
}

void ProfileDB::SetValueByName(const char *name, sint32 value)
{
	PointerList<ProfileVar>::Walker walk(m_vars);
	while(walk.IsValid()) {
		ProfileVar *var = walk.GetObj();
		if(stricmp(var->m_name, name) == 0) {
			if(var->m_type == PV_BOOL || var->m_type == PV_NUM) {
				*var->m_numValue = value;
				return;
			} else {
				return;
			}
		}
		walk.Next();
	}
}
