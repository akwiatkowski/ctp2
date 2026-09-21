//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : New game (SP, MP, scenario) initialisation
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
// USE_TEST_MP_AS_SP
// - When defined, will convert saved MP games to SP games for testing.
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Added player selection for new single player game and scenarios of
//   all types, by Martin G�hmann.
// - Fixed scenarios that allow players other than player 1 to be played,
//   by Martin G�hmann
// - Added multiplayer to single player game conversion for testing.
// - Prevent assigning the same civilisation index twice, while keeping the
//   human player selection.
// - TradePool is fixed on reload if the number of goods in the
//   savegame differs from the number of goods in the database.
//   - June 4th 2005 Martin G�hmann
// - Allowed for nPlayers to be 2 or 3 - JJB 2005/06/28
// - Removed auto-tutorial on low difficulty - JJB 2005/06/28
// - Removed refferences to the civilisation database. (Aug 20th 2005 Martin G�hmann)
// - Removed unused SpriteStateDB refferences. (Aug 28th 2005 Martin G�hmann)
// - Reused obsolate concept icon database slot for new map icon database. (3-Mar-2007 Martin G�hmann)
// - Removed old concept database. (31-Mar-2007 Martin G�hmann)
// - Replaced old const database by new one. (5-Aug-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/utility/gameinit.h"
#include "ctp/ctp2_utils/civlog.h"

// File-static logger.  Anonymous namespace = internal linkage.  Name
// "log" suffix avoids std::log (cmath) name collisions in this TU.
namespace {
auto gameinit_log = civlog::Get("gameinit");
}  // namespace

#include "robot/pathing/A_Star_Heuristic_Cost.h"
#include "gs/gameobj/AchievementTracker.h"
#include "AdvanceRecord.h"
#include "gs/gameobj/Advances.h"
#include "AgeRecord.h"
#include "gs/gameobj/AgreementPool.h"
#include "gs/outcom/AICause.h"
#include "gs/gameobj/ArmyPool.h"
#include "BuildingRecord.h"
#include "ctp/ctp2_utils/c3debug.h"
#include "ctp/ctp2_utils/c3errors.h"
#include "gs/world/Cell.h"
#include "ctp/civ3_main.h"
#include "gs/gameobj/CivilisationPool.h"
#include "ConstRecord.h"
#include "gs/gameobj/Diffcly.h"   // diffutil_GetYearFromTurn
#include "gs/core/game_observer.h"     // gameobservers_Get()
#include "gs/core/game.h"              // Ctp2::Game (trampoline target for pollution_Get/Set)
#include "ctp/civapp.h"                // civapp_Get → CivApp::GetGame
#include "gs/gameobj/CriticalMessagesPrefs.h"
#include "ai/ctpai.h"
#include "gs/database/DB.h"
#include "ctp/debugtools/debugmemory.h"
#include "DifficultyRecord.h"
#include "gs/gameobj/Diplomacy_Log.h"
#include "gs/gameobj/DiplomaticRequestPool.h"
#include "gs/core/render_observer.h"
#include "gs/gameobj/EventTracker.h"
#include "gs/fileio/action_log.h"          // action_log::Clear on fresh game
#include "gs/gameobj/Exclusions.h"
#include "gs/gameobj/FeatTracker.h"
#include "gs/database/filenamedb.h"
#include "gs/fileio/gamefile.h"
#include "gs/gameobj/GameSettings.h"
#include "gs/utility/Globals.h"                    // allocated::...
#include "IconRecord.h"
#include "gs/gameobj/installationpool.h"
#include "gs/gameobj/installationtree.h"
#include <ios>
#include <iostream>
#include <memory>
#include <vector>
#include "gs/gameobj/MaterialPool.h"
#include "gs/gameobj/MessagePool.h"
#include "gs/utility/MoveFlags.h"
#include "gs/database/moviedb.h"
#include "net/general/network.h"
#include "gs/utility/newturncount.h"
#include "gs/gameobj/Order.h"
#include "gs/gameobj/player.h"
#include "gs/database/PlayListDB.h"
#include "ctp/ctp2_utils/pointerlist.h"
#include "gs/gameobj/pollution.h"
#include "robot/aibackdoor/pool.h"
#include "gs/database/profileDB.h"
#include "gs/utility/QuadTree.h"
#include "gs/utility/RandGen.h"
#include "ResourceRecord.h"
#include "robot/utility/RoboInit.h"
#include "gs/core/splash_progress.h"   // SPLASH_STRING macro
#include "gs/core/player_view.h"       // player_view::Init / Cleanup / etc.
#include "gs/slic/SlicEngine.h"
#include "SoundRecord.h"
#include "SpriteRecord.h"
#include "gs/database/StrDB.h"
#include "TerrainRecord.h"
#include "gs/gameobj/TerrImprovePool.h"
#include "gs/database/thronedb.h"
#include "gs/core/tiledmap_observer.h"
#include "gs/gameobj/Vision.h"
#include "gs/gameobj/TopTen.h"
#include "gs/gameobj/TradeBids.h"
#include "gs/gameobj/TradeOfferPool.h"
#include "gs/gameobj/TradePool.h"
#include "gs/gameobj/tradeutil.h"           // constutil_GetMapSizeMapPoint
#include "gs/utility/TurnCnt.h"
#include "gs/gameobj/UnitData.h"
#include "gs/utility/UnitDynArr.h"
#include "gs/gameobj/UnitPool.h"
#include "UnitRecord.h"
#include "gs/world/UnseenCell.h"
#include "gs/database/UVDB.h"
#include "WonderRecord.h"
#include "gs/gameobj/WonderTracker.h"
#include "gs/gameobj/wonderutil.h"
#include "gs/world/World.h"
#include "gs/gameobj/Wormhole.h"
#include "gs/gameobj/XY_Coordinates.h"

extern void Astar_Init();
extern void Astar_Cleanup();


extern MBCHAR g_slic_filename[_MAX_PATH];
extern MBCHAR g_tutorial_filename[_MAX_PATH];
extern HWND               gHwnd;
extern void               verifyYwrap();
extern BOOL               g_aPlayerIsDead;
extern sint32             g_numGoods; // To fix games with altered ressource database
extern sint32             *g_newGoods;
extern sint32 g_abort_parse;
extern sint32 g_oldRandSeed;
extern sint32 g_cheat_age;

// Most session-state subsystems live in Ctp2::Game; the legacy
// foo_Get/foo_Set accessors trampoline through civapp_Get()->GetGame()
// so callers that haven't migrated to game.GetX() still work.  When no
// live Game is reachable (pre-CivApp / post-dtor), Set drops the
// allocation on the floor to match the pre-trampoline behaviour where
// the static pointer was simply overwritten.
#define GAME_TRAMPOLINE(GETTER, SETTER, GAME_METHOD, TYPE)              \
    TYPE * GETTER(void) {                                               \
        CivApp * app = civapp_Get();                                    \
        Ctp2::Game * game = app ? app->GetGame() : nullptr;             \
        return game ? game->Get##GAME_METHOD##Ptr() : nullptr;          \
    }                                                                   \
    void SETTER(TYPE *p) {                                              \
        CivApp * app = civapp_Get();                                    \
        Ctp2::Game * game = app ? app->GetGame() : nullptr;             \
        if (game) game->Set##GAME_METHOD##Ptr(p); else { std::unique_ptr<TYPE> guard(p); } \
    }

GAME_TRAMPOLINE(gamesettings_Get, gamesettings_Set, Settings, GameSettings)

static Wormhole             *g_wormhole=nullptr;
Wormhole * wormhole_Get()    { return g_wormhole; }
void       wormhole_Set(Wormhole *w) { g_wormhole = w; }

// File-static: outside callers must use stringdb_Get / stringdb_Set.
// Was 'extern StringDB *g_theStringDB' declared in StrDB.h.
static StringDB             *g_theStringDB = nullptr;

StringDB * stringdb_Get()        { return g_theStringDB; }
void       stringdb_Set(StringDB *p) { g_theStringDB = p; }
OzoneDatabase               *g_theUVDB=nullptr;
static ThroneDB             *g_theThroneDB = nullptr;

ThroneDB * thronedb_Get()        { return g_theThroneDB; }
void       thronedb_Set(ThroneDB *p) { g_theThroneDB = p; }
PlayListDB                  *g_thePlayListDB = nullptr;
GAME_TRAMPOLINE(world_Get, world_Set, World, World)
GAME_TRAMPOLINE(unitpool_Get, unitpool_Set, Units, UnitPool)

ArmyPool * armypool_Get() {
    CivApp * app = civapp_Get();
    Ctp2::Game * game = app ? app->GetGame() : nullptr;
    return game ? game->GetArmiesPtr() : nullptr;
}
ArmyPool * armypool_Set(ArmyPool *p) {
    // Non-standard legacy signature returns the previous pointer; no
    // live caller uses the return value, but preserve the shape.
    CivApp * app = civapp_Get();
    Ctp2::Game * game = app ? app->GetGame() : nullptr;
    if (!game) { std::unique_ptr<ArmyPool> guard(p); return nullptr; }
    ArmyPool * prev = game->GetArmiesPtr();
    game->SetArmiesPtr(p);
    return prev;
}
// Raw alias — Game owns the array after NewGame adopts it; the alias
// stays valid for the session so player_Get keeps working.  Ownership
// before adoption sits with the creating function (leak on early exit
// matches pre-refactor semantics).
static Player               **g_player=nullptr;

Player *  player_Get(sint32 i)                { return g_player ? g_player[i] : nullptr; }
Player ** player_arr_Get()                { return g_player; }
void      player_arr_Set(Player **p)          { g_player = p; }
PointerList<Player>         *g_deadPlayer = nullptr;
GAME_TRAMPOLINE(rand_ptr, rand_ptr_Set, Rand, RandomGenerator)
GAME_TRAMPOLINE(tradepool_Get,      tradepool_Set,      Trades,      TradePool)
GAME_TRAMPOLINE(tradeofferpool_Get, tradeofferpool_Set, TradeOffers, TradeOfferPool)
static std::unique_ptr<QuadTree<Unit>> g_theUnitTree;

QuadTree<Unit> * unit_tree_Get()              { return g_theUnitTree.get(); }
// Non-owning assign: net_gamesettings deletes the old tree itself before
// calling Set(nullptr), so release the old slot instead of deleting it.
void             unit_tree_Set(QuadTree<Unit> *p) { g_theUnitTree.release(); g_theUnitTree.reset(p); }
GAME_TRAMPOLINE(pollution_Get, pollution_Set, Pollution, Pollution)
GAME_TRAMPOLINE(diplomaticrequestpool_Get, diplomaticrequestpool_Set, DiplomaticRequests,  DiplomaticRequestPool)
GAME_TRAMPOLINE(messagepool_Get,           messagepool_Set,           Messages,            MessagePool)
GAME_TRAMPOLINE(civilisationpool_Get,      civilisationpool_Set,      Civilisations,       CivilisationPool)
GAME_TRAMPOLINE(agreementpool_Get,         agreementpool_Set,         Agreements,          AgreementPool)
GAME_TRAMPOLINE(terrimprovepool_Get,       terrimprovepool_Set,       TerrainImprovements, TerrainImprovementPool)
GAME_TRAMPOLINE(installationpool_Get,      installationpool_Set,      Installations,       InstallationPool)
static std::unique_ptr<InstallationQuadTree> g_theInstallationTree;

InstallationQuadTree * installation_tree_Get()              { return g_theInstallationTree.get(); }
// Non-owning assign — same rationale as unit_tree_Set.
void                   installation_tree_Set(InstallationQuadTree *p) { g_theInstallationTree.release(); g_theInstallationTree.reset(p); }
GAME_TRAMPOLINE(topten_Get, topten_Set, TopTen, TopTen)

GAME_TRAMPOLINE(turn_Get, turn_Set, Turn, TurnCount)

static ProfileDB            *g_theProfileDB = nullptr;

ProfileDB * profiledb_Get()                 { return g_theProfileDB; }
void        profiledb_Set(ProfileDB *p)         { g_theProfileDB = p; }

MovieDB                     *g_theVictoryMovieDB = nullptr;
FilenameDB                  *g_theMessageIconFileDB = nullptr;
Pool<Order>                 *g_theOrderPond = nullptr;
Pool<UnseenCell>            *g_theUnseenPond = nullptr;
Diplomacy_Log               *g_theDiplomacyLog=nullptr;
GAME_TRAMPOLINE(wonder_tracker_Get, wonder_tracker_Set, Wonders,      WonderTracker)
GAME_TRAMPOLINE(eventtracker_Get,   eventtracker_Set,   EventTracker, EventTracker)
GAME_TRAMPOLINE(feattracker_Get, feattracker_Set, Feats, FeatTracker)
GAME_TRAMPOLINE(tradebids_Get,          tradebids_Set,          TradeBids,    TradeBids)
GAME_TRAMPOLINE(achievementtracker_Get, achievementtracker_Set, Achievements, AchievementTracker)
static std::unique_ptr<CriticalMessagesPrefs> g_theCriticalMessagesPrefs;

CriticalMessagesPrefs * critical_messages_prefs_Get() { return g_theCriticalMessagesPrefs.get(); }
// Non-owning assign — same rationale as unit_tree_Set.
void critical_messages_prefs_Set(CriticalMessagesPrefs *p) { g_theCriticalMessagesPrefs.release(); g_theCriticalMessagesPrefs.reset(p); }

MapPoint g_player_start_list[k_MAX_PLAYERS];
sint32 g_player_start_score[k_MAX_PLAYERS];

sint32 g_abort_parse  = FALSE;


MBCHAR g_improve_filename[_MAX_PATH];
MBCHAR g_pollution_filename[_MAX_PATH];
MBCHAR g_global_warming_filename[_MAX_PATH];
MBCHAR g_ozone_filename[_MAX_PATH];
MBCHAR g_terrain_filename[_MAX_PATH];
MBCHAR g_installation_filename[_MAX_PATH];
MBCHAR g_government_filename[_MAX_PATH];
MBCHAR g_governmenticondb_filename[_MAX_PATH];
MBCHAR g_wonder_filename[_MAX_PATH];
MBCHAR g_constdb_filename[_MAX_PATH];
MBCHAR g_pop_filename[_MAX_PATH];
MBCHAR g_civilisation_filename[_MAX_PATH];
MBCHAR g_agedb_filename[_MAX_PATH];
MBCHAR g_thronedb_filename[_MAX_PATH];
MBCHAR g_conceptdb_filename[_MAX_PATH];
MBCHAR g_terrainicondb_filename[_MAX_PATH];
MBCHAR g_advanceicondb_filename[_MAX_PATH];
MBCHAR g_advancedb_filename[_MAX_PATH];
MBCHAR g_mapicondb_filename[_MAX_PATH];
MBCHAR g_tileimprovementdb_filename[_MAX_PATH];
MBCHAR g_tileimprovementicondb_filename[_MAX_PATH];
MBCHAR g_spritestatedb_filename[_MAX_PATH]; // Free slot
MBCHAR g_specialeffectdb_filename[_MAX_PATH];
MBCHAR g_specialattackinfodb_filename[_MAX_PATH];
MBCHAR g_city_style_db_filename[_MAX_PATH];
MBCHAR g_age_city_style_db_filename[_MAX_PATH];





MBCHAR g_goodsspritestatedb_filename[_MAX_PATH]; // Future free slot
MBCHAR g_cityspritestatedb_filename[_MAX_PATH]; // Free slot
MBCHAR g_uniticondb_filename[_MAX_PATH];
MBCHAR g_wondericondb_filename[_MAX_PATH];
MBCHAR g_improveicondb_filename[_MAX_PATH];
MBCHAR g_difficultydb_filename[_MAX_PATH];
MBCHAR g_stringdb_filename[_MAX_PATH];
MBCHAR g_unitdb_filename[_MAX_PATH];
MBCHAR g_sounddb_filename[_MAX_PATH];
MBCHAR g_goods_filename[_MAX_PATH];
MBCHAR g_risk_filename[_MAX_PATH];
MBCHAR g_wondermoviedb_filename[_MAX_PATH];
MBCHAR g_victorymoviedb_filename[_MAX_PATH];
MBCHAR g_endgame_filename[_MAX_PATH];
MBCHAR g_messageiconfdb_filename[_MAX_PATH];
MBCHAR g_goodsicondb_filename[_MAX_PATH];
MBCHAR g_orderdb_filename[_MAX_PATH];
MBCHAR g_mapdb_filename[_MAX_PATH];
MBCHAR g_playlistdb_filename[_MAX_PATH];
MBCHAR g_branchdb_filename[_MAX_PATH];
MBCHAR g_endgameicondb_filename[_MAX_PATH];
MBCHAR g_citysize_filename[_MAX_PATH];
MBCHAR g_featdb_filename[_MAX_PATH];
MBCHAR g_endgameobject_filename[_MAX_PATH];

MBCHAR g_goal_db_filename[_MAX_PATH];
MBCHAR g_personality_db_filename[_MAX_PATH];
MBCHAR g_squad_class_db_filename[_MAX_PATH];
MBCHAR g_unit_buildlist_db_filename[_MAX_PATH];
MBCHAR g_building_buildlist_db_filename[_MAX_PATH];
MBCHAR g_wonder_buildlist_db_filename[_MAX_PATH];
MBCHAR g_buildlist_sequence_db_filename[_MAX_PATH];
MBCHAR g_improvement_list_db_filename[_MAX_PATH];
MBCHAR g_strategy_db_filename[_MAX_PATH];
MBCHAR g_diplomacy_db_filename[_MAX_PATH];
MBCHAR g_advance_list_db_filename[_MAX_PATH];
MBCHAR g_diplomacy_proposal_filename[_MAX_PATH];
MBCHAR g_diplomacy_threat_filename[_MAX_PATH];

sint32 gameinit_GetCivForSlot(sint32 slot);





sint32 g_scenarioUsePlayerNumber = 0;

BOOL                      g_setDifficultyUponLaunch = FALSE;
sint32                    g_difficultyToSetUponLaunch = 0;
BOOL                      g_setBarbarianRiskUponLaunch = FALSE;
sint32                    g_barbarianRiskUponLaunch = 0;

// selitem_Get() definition moved to ui/aui_ctp2/SelItem.cpp — the global
// belongs next to its class, not in game-state init.

static BOOL g_startEmailGame   = FALSE;
static BOOL g_startHotseatGame = FALSE;

BOOL gameinit_IsEmailGame()         { return g_startEmailGame; }
void gameinit_SetEmailGame(BOOL v)      { g_startEmailGame = v; }
BOOL gameinit_IsHotseatGame()       { return g_startHotseatGame; }
void gameinit_SetHotseatGame(BOOL v)    { g_startHotseatGame = v; }
static HotseatPlayerSetup g_hsPlayerSetup[k_MAX_PLAYERS];

HotseatPlayerSetup * hs_player_setup_buf() { return g_hsPlayerSetup; }
void hs_player_setup_Clear() {
	for (sint32 i = 0; i < k_MAX_PLAYERS; i++) {
		g_hsPlayerSetup[i].civ = 0;
		g_hsPlayerSetup[i].isHuman = 0;
		g_hsPlayerSetup[i].name.clear();
		g_hsPlayerSetup[i].email.clear();
	}
}

//----------------------------------------------------------------------------

static sint32 s_networkSettlers[k_MAX_PLAYERS];

namespace
{

//----------------------------------------------------------------------------
//
// Name       : CreateBarbarians
//
// Description: Create a barbarian player.
//
// Parameters : diff                : difficulty level of the game
//
// Globals    : g_theProfileDB      : AI available?
//              g_player            : updated
//              s_networkSettlers   : updated
//
// Returns    : -
//
// Remark(s)  : Will create a human player when the AI is unavailble.
//
//----------------------------------------------------------------------------
void CreateBarbarians(sint32 const diff)
{
	if (g_theProfileDB->IsAIOn())
	{
		g_player[PLAYER_INDEX_VANDALS]      =
		    std::make_unique<Player>(PLAYER_INDEX_VANDALS, diff, PLAYER_TYPE_ROBOT,
		               CIV_INDEX_VANDALS, GENDER_MALE
		              ).release();
	}
	else
	{
		g_player[PLAYER_INDEX_VANDALS]      =
		    std::make_unique<Player>(PLAYER_INDEX_VANDALS, diff, PLAYER_TYPE_HUMAN,
		               CIV_INDEX_RANDOM, GENDER_RANDOM
		              ).release();
	}

	s_networkSettlers[PLAYER_INDEX_VANDALS] = 0;
}

//----------------------------------------------------------------------------
//
// Name       : CreateInitialHuman
//
// Description: Create the main human player.
//
// Parameters : diff      		: difficulty level of the game
//              index           : index to use in player list
//              requestedCiv    : selected civilisation
//
//
// Globals    : g_theProfileDB  : player settings
//              g_player        : updated
//              selitem_Get() : updated
//              NewTurnCount    : updated
//
// Returns    : -
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
void CreateInitialHuman
(
	sint32 const	diff,
	sint32 const	index,
	sint32 const	requestedCiv

)
{
	sint32 const civ = (static_cast<sint32>(CIV_INDEX_RANDOM) == requestedCiv)
	                      ? g_theProfileDB->GetCivIndex()
	                      : requestedCiv;

	g_player[index]     = std::make_unique<Player>(index, diff, PLAYER_TYPE_HUMAN, civ,
	                                 g_theProfileDB->GetGender()
	                                ).release();

	// Set the selected player so that the game starts with the first turn
	// and the correct player.
	player_view::SetCurrentPlayer(index);

	// Make sure that the current player is kept by turning it into the stop
	// player.
	NewTurnCount::SetStopPlayer(index);
}

}; // namespace

//----------------------------------------------------------------------------

sint32 gameinit_PlaceInitalUnits(sint32 nPlayers, MapPoint player_start_list[k_MAX_PLAYERS])
{
	sint32 i;
	sint32 j;
	sint32 settler = -1;

	Assert(nPlayers <= k_MAX_PLAYERS);
	if (nPlayers > k_MAX_PLAYERS)
		nPlayers = k_MAX_PLAYERS;

	sint32 n = g_theUnitDB->NumRecords();
	for (i=0; i<n; i++) {
		if (g_theUnitDB->Get(i)->GetSettleLand()) {
			settler = i;
			break;
		}
	}

	if (settler == -1) {
		settler = 0;
	}




	const DifficultyRecord *drec = g_theDifficultyDB->Get(g_theProfileDB->GetDifficulty());
	sint32 humanStart = drec->GetHumanStartLocation();
	if(humanStart > nPlayers - 1)
		humanStart = nPlayers - 1;
	while(player_start_list[humanStart].x < 0 && humanStart >= 0)
		humanStart--;
	Assert(humanStart >= 0);
	if(humanStart < 0)
		return 0;

	Unit id;
	for (i=1; i<nPlayers; i++)
	{
		if (g_player[i]==nullptr)
			continue;

		sint32 which = i - 1;
		if(which == humanStart)
		{
			if(i != 1)
			{
				which = 1;
			}
		}
		else
			if(i == 1)
			{
				which = humanStart;
			}

		if(player_start_list[which].x < 0)
			break;

		sint32 nUnits = 1;
		if(network_Get().IsLaunchHost())
		{
			nUnits = s_networkSettlers[i];

			if (nUnits < 1 && i != PLAYER_INDEX_VANDALS)
				nUnits = 1;
		}
	else
	{
			if((gameinit_IsHotseatGame() || gameinit_IsEmailGame()) &&
			   g_hsPlayerSetup[i].isHuman)
			{
				nUnits = 1;
			}
			else
				if (g_player[i]->IsRobot())
				{
					nUnits = drec->GetAIStartUnits();
				}

				if (g_player_start_score[which] < sint32(drec->GetExtraSettlerChance()))
//add additional free start units here
				{
					nUnits++;
				}
		}

		if (nUnits < 1)
			nUnits = 1;

		for(j = 0; j < nUnits; j++)
		{
			id =  g_player[i]->CreateUnit(settler, player_start_list[which], Unit(),
										  FALSE, CAUSE_NEW_ARMY_INITIAL);
		}

#ifdef _DEBUG
		sint32 age;
		g_theProfileDB->SetCheatAge(g_cheat_age);
		if (g_theProfileDB->GetCheatAge(age)) {
		for(; j <9; j++) {
			id =  g_player[i]->CreateUnit(settler, player_start_list[which], Unit(),
			                              FALSE, CAUSE_NEW_ARMY_INITIAL);
		}
		}
#endif

	}
	return i - 1;
}

void gameinit_SpewUnits(sint32 player, MapPoint &pos)
{
	FILE *uFile = fopen("logs" FILE_SEP "unitlist.txt", "r");
	sint32 n = g_theUnitDB->NumRecords();
	sint32 i;
	if(!uFile) {
		for (i=0; i<n; i++) {
			if (!g_theUnitDB->Get(i)->GetHasPopAndCanBuild() &&
				!g_theUnitDB->Get(i)->GetIsTrader()
				) {

				do {
					pos.y++;
					if (world_Get()->GetYHeight()<= pos.y) {
						pos.x++;
						pos.y = 2;
					}
					if (world_Get()->GetXWidth()<=pos.x) {
						return;
					}
				}  while(!world_Get()->CanEnter(pos, g_theUnitDB->Get(i)->GetMovementType()));

				Unit id1 = g_player[player]->CreateUnit(i, pos, Unit(),
				                                        FALSE, CAUSE_NEW_ARMY_INITIAL);
				id1.SetIsProfessional(TRUE);
			}
		}
		pos.x++;
	}
	else
	{
		fscanf(uFile, "%d\n", &n);

		std::vector<sint32> uids(n);

		for (i=0; i<n; i++) {
			fscanf(uFile, "%d\n", &uids[i]);
		}
		fclose(uFile);

		for (i=0; i<n; i++) {
			sint32 uid = uids[i];
			if (!g_theUnitDB->Get(uid)->GetHasPopAndCanBuild() &&
				!g_theUnitDB->Get(uid)->GetIsTrader()
				) {

				do {
					pos.y++;
					if (world_Get()->GetYHeight()<= pos.y) {
						pos.x++;
						pos.y = 2;
					}
					if (world_Get()->GetXWidth()<=pos.x) {
						return;
					}
				}  while(!world_Get()->CanEnter(pos, g_theUnitDB->Get(uid)->GetMovementType()));

				Unit id1 = g_player[player]->CreateUnit(uid, pos, Unit(),
				                                        FALSE, CAUSE_NEW_ARMY_INITIAL);
				id1.SetIsProfessional(TRUE);

			}
		}

	}
}

void gameinit_PlaceInitalUnits()
{
	sint32 j;
	MapPoint pos;

	for (j=0; j<k_MAX_PLAYERS; j++) {
		if(!g_player[j]) continue;
		pos.x = static_cast<sint16>(j * 2);
		pos.y = 2;
		gameinit_SpewUnits(j, pos);
	}
}

sint32 gameinit_InitializeGameFiles()
{
	MBCHAR const fn[] = "InitializeGameFiles";

	g_abort_parse = FALSE;

	FILE * fin = c3files_fopen(C3DIR_GAMEDATA, "gamefile.txt", "r");
	if (!fin)
	{
		c3errors_ErrorDialog(fn, "Missing game file");
		g_abort_parse = TRUE;
		return FALSE;
	}

	MBCHAR str1[_MAX_PATH];
	int r = fscanf(fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog(fn, "Missing strings file");
		g_abort_parse = TRUE;
		return FALSE;
	}

	MBCHAR dir[_MAX_PATH];
	dir[0] = 0;
	snprintf(g_stringdb_filename, sizeof(g_stringdb_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog(fn, "Missing sounds file");
		return FALSE;
	}
	snprintf(g_sounddb_filename, sizeof(g_sounddb_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing constants file");
		return FALSE;
	}

	snprintf(g_constdb_filename, sizeof(g_constdb_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing Age file");
		return FALSE;
	}
	snprintf(g_agedb_filename, sizeof(g_agedb_filename), "%s%s", dir, str1);

	r = fscanf( fin, "%s", str1 );
	if ( r == EOF ) {
		c3errors_ErrorDialog( fn, "Missing Throne file" );
		return FALSE;
	}
	snprintf(g_thronedb_filename, sizeof(g_thronedb_filename), "%s%s", dir, str1 );

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing TerrainIconDB file");
		return FALSE;
	}
	snprintf(g_terrainicondb_filename, sizeof(g_terrainicondb_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing terrain file");
		return FALSE;
	}
	snprintf(g_terrain_filename, sizeof(g_terrain_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing AdvanceIconDB file");
		return FALSE;
	}
	snprintf(g_advanceicondb_filename, sizeof(g_advanceicondb_filename), "%s%s", dir, str1);

	SPLASH_STRING("Loading Advance DB...");
	r=fscanf (fin, "%s", str1);
	if (r == EOF) {
	c3errors_ErrorDialog  (fn, "Missing advances file");
		return FALSE;
	}
	snprintf(g_advancedb_filename, sizeof(g_advancedb_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing concepts file");
		return FALSE;
	}
	snprintf(g_conceptdb_filename, sizeof(g_conceptdb_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing MapIconDB file");
		return FALSE;
	}
	snprintf(g_mapicondb_filename, sizeof(g_mapicondb_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing tile improvements file");
		return FALSE;
	}
	snprintf(g_tileimprovementdb_filename, sizeof(g_tileimprovementdb_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing TileImprovmentIconDB file");
		return FALSE;
	}
	snprintf(g_tileimprovementicondb_filename, sizeof(g_tileimprovementicondb_filename), "%s%s", dir, str1);

	SPLASH_STRING("Loading Sprite DB...");
	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing sprite file");
		return FALSE;
	}
	snprintf(g_spritestatedb_filename, sizeof(g_spritestatedb_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing Special Effect ID file");
		return FALSE;
	}
	snprintf(g_specialeffectdb_filename, sizeof(g_specialeffectdb_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if(r == EOF) {
		c3errors_ErrorDialog(fn, "Missing Special Attack Info DB file");
		return FALSE;
	}
	snprintf(g_specialattackinfodb_filename, sizeof(g_specialattackinfodb_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing GoodsSpriteID file");
		return FALSE;
	}
	snprintf(g_goodsspritestatedb_filename, sizeof(g_goodsspritestatedb_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing GoodsSpriteID file");
		return FALSE;
	}
	snprintf(g_cityspritestatedb_filename, sizeof(g_cityspritestatedb_filename), "%s%s", dir, str1);

	r = fscanf( fin, "%s", str1 );
	if ( r == EOF ) {
		c3errors_ErrorDialog( fn, "Missing BranchID file" );
		return FALSE;
	}
	snprintf(g_branchdb_filename, sizeof(g_branchdb_filename), "%s%s", dir, str1 );

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing UnitIconDB file");
		return FALSE;
	}
	snprintf(g_uniticondb_filename, sizeof(g_uniticondb_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing Units file");
		return FALSE;
	}

	snprintf(g_unitdb_filename, sizeof(g_unitdb_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing wonder file");
		return FALSE;
	}
	snprintf(g_wonder_filename, sizeof(g_wonder_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing WonderIconDB file");
		return FALSE;
	}
	snprintf(g_wondericondb_filename, sizeof(g_wondericondb_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing WonderMovieDB file");
		return FALSE;
	}
	snprintf(g_wondermoviedb_filename, sizeof(g_wondermoviedb_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing VictoryMovieDB file");
		return FALSE;
	}
	snprintf(g_victorymoviedb_filename, sizeof(g_victorymoviedb_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing improvement file");
		return FALSE;
	}
	snprintf(g_improve_filename, sizeof(g_improve_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing UnitIconDB file");
		return FALSE;
	}
	snprintf(g_improveicondb_filename, sizeof(g_improveicondb_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing pollution file");
		return FALSE;
	}
	snprintf(g_pollution_filename, sizeof(g_pollution_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing global warming pollution file");
		return FALSE;
	}
	snprintf(g_global_warming_filename, sizeof(g_global_warming_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing ozone pollution file");
		return FALSE;
	}
	snprintf(g_ozone_filename, sizeof(g_ozone_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing government file");
		return FALSE;
	}
	snprintf(g_government_filename, sizeof(g_government_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing government icondb file");
		return FALSE;
	}
	snprintf(g_governmenticondb_filename, sizeof(g_governmenticondb_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing population file");
		return FALSE;
	}
	snprintf(g_pop_filename, sizeof(g_pop_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing difficulty file");
		return FALSE;
	}
	snprintf(g_difficultydb_filename, sizeof(g_difficultydb_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if(r == EOF) {
		c3errors_ErrorDialog(fn, "Missing installations file");
		return FALSE;
	}
	snprintf(g_installation_filename, sizeof(g_installation_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if(r == EOF) {
		c3errors_ErrorDialog(fn, "Missing civilisations file");
		return FALSE;
	}
	snprintf(g_civilisation_filename, sizeof(g_civilisation_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if(r == EOF) {
		c3errors_ErrorDialog(fn, "Missing slic file");
		return FALSE;
	}
	snprintf(g_slic_filename, sizeof(g_slic_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if(r == EOF) {
		c3errors_ErrorDialog(fn, "Missing tutorial file");
		return FALSE;
	}
	snprintf(g_tutorial_filename, sizeof(g_tutorial_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if(r == EOF) {
		c3errors_ErrorDialog(fn, "Missing goods file");
		return FALSE;
	}
	snprintf(g_goods_filename, sizeof(g_goods_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if(r == EOF) {
		c3errors_ErrorDialog(fn, "Missing risk file");
		return FALSE;
	}
	snprintf(g_risk_filename, sizeof(g_risk_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if(r == EOF) {
		c3errors_ErrorDialog(fn, "Missing end game database");
		return FALSE;
	}
	snprintf(g_endgame_filename, sizeof(g_endgame_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if(r == EOF) {
		c3errors_ErrorDialog(fn, "Missing end game icon database");
		return FALSE;
	}
	snprintf(g_endgameicondb_filename, sizeof(g_endgameicondb_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if (r==EOF) {
		c3errors_ErrorDialog(fn, "Missing Message Icon Filename database");
		return FALSE;
	}
	snprintf(g_messageiconfdb_filename, sizeof(g_messageiconfdb_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if (r==EOF) {
		c3errors_ErrorDialog(fn, "Missing Goods Icon Filename database");
		return FALSE;
	}
	snprintf(g_goodsicondb_filename, sizeof(g_goodsicondb_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if(r == EOF) {
		c3errors_ErrorDialog(fn, "Missing Order DB filename");
		return FALSE;
	}
	snprintf(g_orderdb_filename, sizeof(g_orderdb_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if(r == EOF) {
		c3errors_ErrorDialog(fn, "Missing map database filename");
		return FALSE;
	}
	snprintf(g_mapdb_filename, sizeof(g_mapdb_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if(r == EOF) {
		c3errors_ErrorDialog(fn, "Missing playlist database filename");
		return FALSE;
	}
	snprintf(g_playlistdb_filename, sizeof(g_playlistdb_filename), "%s%s", dir, str1);





	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing squad classes DB filename");
		return FALSE;
	}
	snprintf(g_squad_class_db_filename, sizeof(g_squad_class_db_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing goals DB filename");
		return FALSE;
	}
	snprintf(g_goal_db_filename, sizeof(g_goal_db_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing wonder build lists DB filename");
		return FALSE;
	}
	snprintf(g_wonder_buildlist_db_filename, sizeof(g_wonder_buildlist_db_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing building build lists DB filename");
		return FALSE;
	}
	snprintf(g_building_buildlist_db_filename, sizeof(g_building_buildlist_db_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing unit build lists DB filename");
		return FALSE;
	}
	snprintf(g_unit_buildlist_db_filename, sizeof(g_unit_buildlist_db_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing improvement lists DB filename");
		return FALSE;
	}
	snprintf(g_improvement_list_db_filename, sizeof(g_improvement_list_db_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing strategies DB filename");
		return FALSE;
	}
	snprintf(g_strategy_db_filename, sizeof(g_strategy_db_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing build list sequence DB filename");
		return FALSE;
	}
	snprintf(g_buildlist_sequence_db_filename, sizeof(g_buildlist_sequence_db_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing diplomacy DB filename");
		return FALSE;
	}
	snprintf(g_diplomacy_db_filename, sizeof(g_diplomacy_db_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if(r == EOF) {
		c3errors_ErrorDialog(fn, "Missing city size DB filename");
		return FALSE;
	}
	snprintf(g_citysize_filename, sizeof(g_citysize_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if(r == EOF) {
		c3errors_ErrorDialog(fn, "Missing diplomacy proposal DB filename");
		return FALSE;
	}
	snprintf(g_diplomacy_proposal_filename, sizeof(g_diplomacy_proposal_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if(r == EOF) {
		c3errors_ErrorDialog(fn, "Missing diplomacy threat DB filename");
		return FALSE;
	}
	snprintf(g_diplomacy_threat_filename, sizeof(g_diplomacy_threat_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if(r == EOF) {
		c3errors_ErrorDialog(fn, "Missing advance list DB filename");
		return FALSE;
	}
	snprintf(g_advance_list_db_filename, sizeof(g_advance_list_db_filename), "%s%s", dir, str1);

	r = fscanf (fin, "%s", str1);
	if (r == EOF) {
		c3errors_ErrorDialog  (fn, "Missing personality DB filename");
		return FALSE;
	}
	snprintf(g_personality_db_filename, sizeof(g_personality_db_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if(r == EOF) {
		c3errors_ErrorDialog(fn, "Missing feat DB filename");
		return FALSE;
	}
	snprintf(g_featdb_filename, sizeof(g_featdb_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if(r == EOF) {
		c3errors_ErrorDialog(fn, "Missing end game object DB filename");
		return FALSE;
	}
	snprintf(g_endgameobject_filename, sizeof(g_endgameobject_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if(r == EOF) {
		c3errors_ErrorDialog(fn, "Missing city style DB filename");
		return FALSE;
	}
	snprintf(g_city_style_db_filename, sizeof(g_city_style_db_filename), "%s%s", dir, str1);

	r = fscanf(fin, "%s", str1);
	if(r == EOF) {
		c3errors_ErrorDialog(fn, "Missing age city style DB filename");
		return FALSE;
	}
	snprintf(g_age_city_style_db_filename, sizeof(g_age_city_style_db_filename), "%s%s", dir, str1);

	c3files_fclose(fin);

	return TRUE;
}




sint32 spriteEditor_Initialize(sint32 mWidth, sint32 mHeight)
{
	// (Legacy g_debugWindow->SetDebugMask(k_DBG_AI) dropped — modern code
	// uses DPRINTF(k_DBG_AI, ...) directly with no UI filtering.)

	g_theProfileDB->SetNPlayers(3); // What's this?

	sint32 nPlayers = g_theProfileDB->GetNPlayers();

	g_theOrderPond	= std::make_unique<Pool<Order>>(INITIAL_CHUNK_LIST_SIZE).release();
	g_theUnseenPond	= std::make_unique<Pool<UnseenCell>>(INITIAL_CHUNK_LIST_SIZE).release();

	g_theProfileDB->SetTutorialAdvice(FALSE);







	uint32 seed = g_oldRandSeed ? g_oldRandSeed : GetTickCount();
	srand(seed);
	rand_ptr_Set(std::make_unique<RandomGenerator>(seed).release());






	SPLASH_STRING("Initializing Pathing...");
	Astar_Init();





	gamesettings_Set(std::make_unique<GameSettings>().release());

	SPLASH_STRING("Initializing the Map...");

	// (custommapscreen_setValues call dropped — it read ProfileDB values and
	// wrote them straight back, so the net game-state effect was zero.  Its
	// only real action was refreshing UI slider widgets, which now happens
	// on the UI side when the custom-map screen opens.)

	MapPoint	mapSize;
	constutil_GetMapSizeMapPoint(g_theProfileDB->GetMapSize(), mapSize);

	world_Set(std::make_unique<World>(mapSize,
	                       g_theProfileDB->IsXWrap(),
	                       g_theProfileDB->IsYWrap()).release());

	world_Get()->CreateTheWorld(g_player_start_list,
	                           g_player_start_score);

	sint32 i;
	for(i = 0; i < nPlayers - 1; i++)
		if(g_player_start_list[i].x < 0)
		{
			nPlayers = i + 1;
			break;
		}

	g_theProfileDB->SetNPlayers(nPlayers);

	Assert(world_Get());

	turn_Set(std::make_unique<TurnCount>(g_theProfileDB->GetNPlayers(),
		diffutil_GetYearFromTurn(gamesettings_Get()->GetDifficulty(), 0)).release());

	player_view::Init(nPlayers);








	SPLASH_STRING("Allocating Object Pools...");

	g_theUnitTree = std::make_unique<QuadTree<Unit>>(sint16(world_Get()->GetXWidth()),
	                                   sint16(world_Get()->GetYHeight()),
	                                   world_Get()->IsYwrap());

	g_theInstallationTree = std::make_unique<InstallationQuadTree>(sint16(world_Get()->GetXWidth()),
	                                                 sint16(world_Get()->GetYHeight()),
	                                                 world_Get()->IsYwrap());

	// Trade pool before unit pool — same re-init ordering rationale as
	// gameinit_Initialize (route kills must run while old units live).
	tradepool_Set(std::make_unique<TradePool>().release());

	unitpool_Set(std::make_unique<UnitPool>().release());
	Assert(unitpool_Get());

	armypool_Set(std::make_unique<ArmyPool>().release());
	Assert(armypool_Get());

	tradeofferpool_Set(std::make_unique<TradeOfferPool>().release());
	Assert(tradeofferpool_Get());

	pollution_Set(std::make_unique<Pollution>().release());
	Assert(pollution_Get());

	topten_Set(std::make_unique<TopTen>().release());
	Assert(topten_Get());




	SPLASH_STRING("Initializing SLIC Engine...");

    if (!SlicEngine::Reload(g_slic_filename))
    {
		return FALSE;
    }

	g_theProfileDB->SetTutorialAdvice(FALSE);
	g_theProfileDB->SetThroneRoom(FALSE);
	g_theProfileDB->SetDifficulty(0);
	g_theProfileDB->SetRiskLevel(0);
	g_theProfileDB->SetNonRandomCivs(TRUE);






	SPLASH_STRING("Initializing Object Pools...");

	terrimprovepool_Set(std::make_unique<TerrainImprovementPool>().release());
	Assert(terrimprovepool_Get()) ;

	diplomaticrequestpool_Set(std::make_unique<DiplomaticRequestPool>().release()) ;
	Assert(diplomaticrequestpool_Get()) ;

	civilisationpool_Set(std::make_unique<CivilisationPool>().release()) ;
	Assert(civilisationpool_Get()) ;

	agreementpool_Set(std::make_unique<AgreementPool>().release()) ;
	Assert(agreementpool_Get()) ;

	messagepool_Set(std::make_unique<MessagePool>().release()) ;
	Assert(messagepool_Get()) ;

	g_theCriticalMessagesPrefs = std::make_unique<CriticalMessagesPrefs>();
	Assert(g_theCriticalMessagesPrefs) ;

	installationpool_Set(std::make_unique<InstallationPool>().release());
	Assert(installationpool_Get()) ;

	installationpool_Get()->RebuildQuadTree();

	g_wormhole = nullptr;

	wonder_tracker_Set(std::make_unique<WonderTracker>().release());

	achievementtracker_Set(std::make_unique<AchievementTracker>().release());

	tradebids_Set(std::make_unique<TradeBids>().release());





	SPLASH_STRING("Setting Up Players...");

	g_player = std::make_unique<Player *[]>(k_MAX_PLAYERS).release();
	Assert(g_player);

	g_deadPlayer = std::make_unique<PointerList<Player>>().release();

	sint32 diff = g_theProfileDB->GetDifficulty();

	CreateBarbarians(diff);

	sint32 civ = g_theProfileDB->GetCivIndex();

	// TODO: check if the fixed index 1 is correct here,
	//       and whether the start/stop player have to be set.
	g_player[1] = std::make_unique<Player>(PLAYER_INDEX(1),
							 diff,
							 PLAYER_TYPE_HUMAN,
							 civ,
							 g_theProfileDB->GetGender()).release();

	s_networkSettlers[1] = 1;
	if ( strlen(g_theProfileDB->GetLeaderName()) > 0)
		g_player[1]->m_civilisation->AccessData()->SetLeaderName(g_theProfileDB->GetLeaderName());

	for (i=2; i<nPlayers; i++)
	{
		if (g_theProfileDB->IsAIOn() &&
			((!network_Get().IsNetworkLaunch() ||
			i > network_Get().GetNumHumanPlayers()) ||
			(network_Get().IsNetworkLaunch() &&
			g_theProfileDB->NoHumanPlayersOnHost() &&
			i==network_Get().GetNumHumanPlayers())))
		{


			g_player[i] = std::make_unique<Player>(PLAYER_INDEX(i),
									 diff,
									 PLAYER_TYPE_ROBOT,
									 CIV_INDEX_RANDOM,
									 GENDER_RANDOM).release();
		}
	else
	{
			g_player[i] = std::make_unique<Player>(PLAYER_INDEX(i), diff, PLAYER_TYPE_HUMAN, CIV_INDEX_RANDOM, GENDER_RANDOM).release();
		}
		s_networkSettlers[i] = 1;
	}

#ifdef _DEBUG
	if (g_theProfileDB->IsDiplomacyLogOn())
    {
		g_theDiplomacyLog = std::make_unique<Diplomacy_Log>().release();
	}

    verifyYwrap();
#endif






	SPLASH_STRING("Creating AI Interface's...");

	if (g_theProfileDB->IsAIOn() || network_Get().IsNetworkLaunch())
	{
		PLAYER_INDEX ai_players[k_MAX_PLAYERS];
		sint32 next = 0;

		for (i=0; i< k_MAX_PLAYERS; i++)
			if(g_player[i] && g_player[i]->IsRobot())
				ai_players[next++] = PLAYER_INDEX(i);

		if(!g_theProfileDB->IsAIOn() && network_Get().IsNetworkLaunch())
			g_theProfileDB->SetAI(TRUE);
	}
	else
	{
		for(i = 0; i < k_MAX_PLAYERS; i++)
			if(g_player[i])
				g_player[i]->m_playerType = PLAYER_TYPE_HUMAN;
	}

	tradeofferpool_Get()->ReRegisterOffers();










	{
		sint32 visible = player_view::VisiblePlayer();
		if (visible >= 0 && g_player[visible]) {
			player_view::Refresh();
		}
	}

	world_Get()->A_star_heuristic->Update();

	world_Get()->RecalculateZOC();




	g_player[1]->m_gold->SetLevel(1000000);
	g_player[1]->m_materialPool->AddMaterials(1000000);

	g_scenarioUsePlayerNumber = 0;








	turn_Get()->SetHotSeat(FALSE);
	turn_Get()->SetEmail(FALSE);

	gameinit_SetHotseatGame(FALSE);
	gameinit_SetEmailGame(FALSE);

	{
		sint32 p;
		sint32 c;
		sint32 u;
		for(p = 0; p < k_MAX_PLAYERS; p++) {
			if(g_player[p]) {
				for(c = 0; c < g_player[p]->m_all_cities->Num(); c++) {
					Unit city = g_player[p]->m_all_cities->Access(c);
					render_observer::HackSetSpriteID(city, city.CD()->GetDesiredSpriteIndex());
				}
				for(u = 0; u < g_player[p]->m_all_units->Num(); u++) {
					Unit unit = g_player[p]->m_all_units->Access(u);
					sint32 spriteIndex =  unit.GetDBRec()->GetDefaultSprite()->GetValue();
					unit.GetSpriteState()->SetIndex(spriteIndex);
					render_observer::ChangeUnitImage(unit.GetSpriteState(), unit.GetType(), unit);
				}
			}
		}
	}

	if (g_setDifficultyUponLaunch)
	{
		g_theProfileDB->SetDifficulty(g_difficultyToSetUponLaunch);
		g_setDifficultyUponLaunch = FALSE;
	}

	if (g_setBarbarianRiskUponLaunch)
	{
		g_theProfileDB->SetRiskLevel(g_barbarianRiskUponLaunch);
		g_setBarbarianRiskUponLaunch = FALSE;
	}




	if (gameobservers_Get()) gameobservers_Get()->NotifySetGraphMinRound(0);

	return 1;
}

sint32 gameinit_GetCivForSlot(sint32 slot)
{

	if(network_Get().IsLaunchHost()) {

		if(slot > network_Get().GetNumHumanPlayers()) {
			sint32 firstRobot = network_Get().GetNumHumanPlayers() + 1;
			NSAIPlayerInfo *nsaipi = network_Get().GetNSAIPlayerInfo(slot - firstRobot);
			if(nsaipi) {
				return nsaipi->m_civ;
			}
		}
	else
	{

			NSPlayerInfo *nspi = network_Get().GetNSPlayerInfo(slot - 1);
			if(nspi)
				return nspi->m_civ;
		}
	}


	switch(start_info_type_Get()) {
		case STARTINFOTYPE_NONE:
		case STARTINFOTYPE_NOLOCS:
		case STARTINFOTYPE_POSITIONSFIXED:
			return CIV_INDEX_RANDOM;
		case STARTINFOTYPE_CIVSFIXED:

			if (slot-1 >= world_Get()->GetNumStartingPositions()) {
				return CIV_INDEX_RANDOM;
			}
	else
	{
				return world_Get()->GetStartingPointCiv(slot - 1);
			}
		default:
			return CIV_INDEX_RANDOM;
	}
}


sint32 gameinit_Initialize(sint32 mWidth, sint32 mHeight)
{
	gameinit_log->info("gameinit_Initialize: started (w={}, h={})",
	                   mWidth, mHeight);

	// (Legacy g_debugWindow->SetDebugMask(k_DBG_AI) dropped — modern code
	// uses DPRINTF(k_DBG_AI, ...) directly with no UI filtering.)

	uint32 seed;

	// Reduced min players from 4 to 2 - JJB
	if(!network_Get().IsNetworkLaunch() && g_theProfileDB->GetNPlayers() < 2) {
		g_theProfileDB->SetNPlayers(2);
	}
	sint32 nPlayers = g_theProfileDB->GetNPlayers();
	Assert(2 <= nPlayers);

	g_theOrderPond = std::make_unique<Pool<Order>>(INITIAL_CHUNK_LIST_SIZE).release();
	g_theUnseenPond = std::make_unique<Pool<UnseenCell>>(INITIAL_CHUNK_LIST_SIZE).release();

	if(network_Get().IsActive()
	|| network_Get().IsNetworkLaunch()
	|| gameinit_IsHotseatGame()
	|| gameinit_IsEmailGame()
	){
		g_theProfileDB->SetTutorialAdvice(FALSE);
	}
	// Removed the auto-tutorial on low difficulty, since it causes
	// more problems than it solves - JJB

#ifdef _DEBUG
	FILE * fin = fopen ("dbgseed.txt", "r");

	if (fin) {
		fscanf (fin, "%d", &seed);
	}
	else
	{

		seed = g_oldRandSeed ? g_oldRandSeed : GetTickCount();

		fin = fopen("logs/oldseed.txt", "w");
		if (fin) {
			fprintf (fin, "%d\n", seed);
		}
	}

	DPRINTF(k_DBG_FIX, ("** RANDOM SEED %d\n", seed));
	if (fin) {
		fclose (fin);
	}
#else

	seed = g_oldRandSeed ? g_oldRandSeed : GetTickCount();
#endif

	srand(seed);

	rand_ptr_Set(std::make_unique<RandomGenerator>(seed).release());





	SPLASH_STRING("Initializing Pathing...");
	Astar_Init();





		gamesettings_Set(std::make_unique<GameSettings>().release());

	SPLASH_STRING("Initializing the Map...");

	bool loadEverything =
        !is_scenario_Get() || (start_info_type_Get() == STARTINFOTYPE_NOLOCS);
	gameinit_log->debug("loadEverything={}, is_scenario_Get()={}, g_startInfoType={}",
	           loadEverything, (bool)is_scenario_Get(), (int)start_info_type_Get());

		// (custommapscreen_setValues call dropped — same rationale as above.)

		MapPoint	mapSize;
		constutil_GetMapSizeMapPoint(g_theProfileDB->GetMapSize(), mapSize);

		world_Set(std::make_unique<World>(mapSize,
		                       g_theProfileDB->IsXWrap(),
		                       g_theProfileDB->IsYWrap()).release());

		world_Get()->CreateTheWorld(g_player_start_list,
		                           g_player_start_score);

#ifdef _DEBUG
		if (!network_Get().IsActive()) {
			Assert(g_player_start_list[1].x >= 0);
		}
#endif

		nPlayers = g_theProfileDB->GetNPlayers();
		for(sint32 i = 0; i < nPlayers - 1; i++) {
			if(g_player_start_list[i].x < 0) {
				nPlayers = i + 1;
				break;
			}
		}

		Assert(nPlayers <= k_MAX_PLAYERS);
		if (k_MAX_PLAYERS < nPlayers) {
			exit(0);
		}
		g_theProfileDB->SetNPlayers(nPlayers);


	Assert(world_Get());

	gameinit_log->debug("step: post-World, before TurnCount");
		turn_Set(std::make_unique<TurnCount>(g_theProfileDB->GetNPlayers(),
			diffutil_GetYearFromTurn(gamesettings_Get()->GetDifficulty(), 0)).release());
		if(network_Get().IsActive() || network_Get().IsNetworkLaunch()) {
			sint32 startAge = network_Get().GetStartingAge();
			if(startAge != 0) {
				turn_Get()->SkipToRound(0 );
			}
		}

	gameinit_log->debug("step: player_view::Init ({})", nPlayers);
		player_view::Init(nPlayers);








	SPLASH_STRING("Allocating Object Pools...");

	g_theUnitTree = std::make_unique<QuadTree<Unit>>(sint16(world_Get()->GetXWidth()),
	                                   sint16(world_Get()->GetYHeight()),
	                                   world_Get()->IsYwrap());

	g_theInstallationTree = std::make_unique<InstallationQuadTree>(sint16(world_Get()->GetXWidth()),
	                                                 sint16(world_Get()->GetYHeight()),
	                                                 world_Get()->IsYwrap());

	// Order matters on RE-init (in-process load_game): replace the
	// TradePool BEFORE the UnitPool so the old UnitPool's CityData dtors
	// see the fresh (empty) trade pool and skip their route-kill cascade
	// (guarded via TradeRoute::IsValid in ~CityData). The old order ran
	// that cascade against half-destroyed pools — "No such object" abort
	// on any save with live trade routes (deep-state save/load soak).
	// Note: Game::Set*Ptr uses unique_ptr::reset, so the NEW pool is
	// already current while the OLD pool's dtor runs.
	tradepool_Set(std::make_unique<TradePool>().release());

		unitpool_Set(std::make_unique<UnitPool>().release());
	Assert(unitpool_Get());

		armypool_Set(std::make_unique<ArmyPool>().release());
	Assert(armypool_Get());

    // 55 is probably the last save game version for CTP1
		tradeofferpool_Set(std::make_unique<TradeOfferPool>().release());
	Assert(tradeofferpool_Get());

		pollution_Set(std::make_unique<Pollution>().release());
	Assert(pollution_Get());

		topten_Set(std::make_unique<TopTen>().release());
	Assert(topten_Get());

	SPLASH_STRING("Initializing SLIC Engine...");

        if (!SlicEngine::Reload(g_slic_filename))
        {
			return FALSE;
		}

		if (!network_Get().IsActive() && !network_Get().IsNetworkLaunch())
        {
			slicengine_Get()->SetTutorialActive(g_theProfileDB->IsTutorialAdvice());
			slicengine_Get()->SetTutorialPlayer(g_theProfileDB->GetPlayerIndex());

			if (g_theProfileDB->IsTutorialAdvice())
            {
				if (slicengine_Get()->Load(g_tutorial_filename, k_TUTORIAL_FILE))
                {
					slicengine_Get()->Link();
				}

				g_theProfileDB->SetThroneRoom(FALSE);
				g_theProfileDB->SetDifficulty(0);
				g_theProfileDB->SetRiskLevel(0);
				g_theProfileDB->SetNonRandomCivs(TRUE);
			}
		}
	else
	{
			g_theProfileDB->SetTutorialAdvice(FALSE);
		}

	SPLASH_STRING("Initializing Object Pools...");

		terrimprovepool_Set(std::make_unique<TerrainImprovementPool>().release());
	Assert(terrimprovepool_Get()) ;

		diplomaticrequestpool_Set(std::make_unique<DiplomaticRequestPool>().release()) ;
	Assert(diplomaticrequestpool_Get()) ;

		civilisationpool_Set(std::make_unique<CivilisationPool>().release()) ;
	Assert(civilisationpool_Get()) ;

		agreementpool_Set(std::make_unique<AgreementPool>().release()) ;
	Assert(agreementpool_Get()) ;

		messagepool_Set(std::make_unique<MessagePool>().release()) ;
	Assert(messagepool_Get()) ;

	g_theCriticalMessagesPrefs = std::make_unique<CriticalMessagesPrefs>() ;
	Assert(g_theCriticalMessagesPrefs) ;

		installationpool_Set(std::make_unique<InstallationPool>().release());
	Assert(installationpool_Get()) ;

	installationpool_Get()->RebuildQuadTree();

		g_wormhole = nullptr;

		wonder_tracker_Set(std::make_unique<WonderTracker>().release());

		achievementtracker_Set(std::make_unique<AchievementTracker>().release());

	    // Exclusions not used

		feattracker_Set(std::make_unique<FeatTracker>().release());

		tradebids_Set(std::make_unique<TradeBids>().release());

		eventtracker_Set(std::make_unique<EventTracker>().release());

		// Fresh game starts with an empty Action Log — otherwise a new game
		// within one process would inherit the prior game's ledger (the tap's
		// carrier is a process-lifetime static, not Game-singleton-scoped).
		action_log::Clear();

	SPLASH_STRING("Setting Up Players...");

	g_player = std::make_unique<Player *[]>(k_MAX_PLAYERS).release();
	g_deadPlayer = std::make_unique<PointerList<Player>>().release();

	sint32 diff = gamesettings_Get()->GetDifficulty();


	sint32 i;
		//
		//	Normal game code
		CreateBarbarians(diff);

		sint32 netIndex = 0;
		NSPlayerInfo *nspi = nullptr;
		sint32 civ = g_theProfileDB->GetCivIndex();
		if(network_Get().IsLaunchHost()) {
			nspi = network_Get().GetNSPlayerInfo(netIndex++);
			if(nspi) {
				civ = nspi->m_civ;
			}
		}

		sint32 const		humanIndex	=
			(nspi) ? 1 : g_theProfileDB->GetPlayerIndex();
		CreateInitialHuman(diff, humanIndex, civ);

		if (nspi)
		{
			g_player[humanIndex]->m_gold->SetLevel(nspi->m_civpoints);
			s_networkSettlers[humanIndex] = nspi->m_settlers;
			if (!nspi->m_name.empty())
			{
				g_player[humanIndex]->m_civilisation->AccessData()->
					SetLeaderName(nspi->m_name.c_str());
			}
		}
	else
	{
			s_networkSettlers[humanIndex] = 1;
			if ((strlen(g_theProfileDB->GetLeaderName()) > 0) &&
				!g_theProfileDB->IsTutorialAdvice()
			   )
			{
				g_player[humanIndex]->m_civilisation->AccessData()->
					SetLeaderName(g_theProfileDB->GetLeaderName());
			}
		}

		sint32 firstRobot = -1;

		for (i = 1; i < nPlayers; ++i)
		{
		  if (i != humanIndex)
		  {
			if (g_theProfileDB->IsAIOn() &&
				((!network_Get().IsNetworkLaunch() || i > network_Get().GetNumHumanPlayers()) ||
				 (network_Get().IsNetworkLaunch() && g_theProfileDB->NoHumanPlayersOnHost() && i==network_Get().GetNumHumanPlayers()))) {


				NSAIPlayerInfo *nsaipi = nullptr;
				if(network_Get().IsLaunchHost()) {
					if(firstRobot < 0)
						firstRobot = i;

					nsaipi = network_Get().GetNSAIPlayerInfo(i - firstRobot);
				}
				if(nsaipi) {
					g_player[i] = std::make_unique<Player>(PLAYER_INDEX(i),
											 diff,
											 PLAYER_TYPE_ROBOT,
											 nsaipi->m_civ,
											 GENDER_RANDOM).release();
					g_player[i]->m_networkGroup = nsaipi->m_group;
					g_player[i]->m_gold->SetLevel(nsaipi->m_civpoints);
					s_networkSettlers[i] = nsaipi->m_settlers;
				}
	else
	{
					g_player[i] = std::make_unique<Player>(PLAYER_INDEX(i),
											 diff,
											 PLAYER_TYPE_ROBOT,
											 CIV_INDEX_RANDOM,
											 GENDER_RANDOM).release();
					s_networkSettlers[i] = 1;
				}
			}
	else
	{
				if(!network_Get().IsLaunchHost()) {
					g_player[i] = std::make_unique<Player>(PLAYER_INDEX(i), diff, PLAYER_TYPE_HUMAN, CIV_INDEX_RANDOM, GENDER_RANDOM).release();
					s_networkSettlers[i] = 1;
				}
	else
	{
					NSPlayerInfo *nspi = network_Get().GetNSPlayerInfo(netIndex++);

					civ = nspi ? nspi->m_civ : CIV_INDEX_RANDOM;


					g_player[i] = std::make_unique<Player>(PLAYER_INDEX(i),
											 diff,
											 PLAYER_TYPE_HUMAN,
											 civ,
											 GENDER_RANDOM).release();
					if(nspi) {
						g_player[i]->m_networkId = nspi->m_id;
						g_player[i]->m_networkGroup = nspi->m_group;
						g_player[i]->m_gold->SetLevel(nspi->m_civpoints);
						s_networkSettlers[i] = nspi->m_settlers;
						if(!nspi->m_name.empty()) {
							g_player[i]->m_civilisation->AccessData()->
								SetLeaderName(nspi->m_name.c_str());
						}
					}
	else
	{
						s_networkSettlers[i] = 1;
					}

				}
			}
		  } // if (i != humanIndex)
		} // for

		if(network_Get().IsLaunchHost() && network_Get().TeamsEnabled()) {
			for(i = 1; i < k_MAX_PLAYERS; i++) {
				if(g_player[i]) {
					sint32 j;
					for(j = 1; j < k_MAX_PLAYERS; j++) {
						if(j == i)
							continue;
						if(!g_player[j])
							continue;
						if(g_player[i]->m_networkGroup == g_player[j]->m_networkGroup) {
							g_player[i]->mask_alliance |= (1 << j);
							g_player[i]->m_contactedPlayers |= (1 << j);
							g_player[i]->m_embassies |= (1 << j);
							g_player[i]->m_diplomatic_state[j] = DIPLOMATIC_STATE_ALLIED;
						}
					}
				}
			}
		}

#ifdef _DEBUG
	if (g_theProfileDB->IsDiplomacyLogOn())
	{
		g_theDiplomacyLog = std::make_unique<Diplomacy_Log>().release();
	}

	verifyYwrap();
#endif

	bool createRobotInterface = true;

	SPLASH_STRING("Initializing A-star Pathing...");
	roboinit_Initalize();
	CtpAi::Cleanup();

		SPLASH_STRING("Initialize AI data elements...");
		CtpAi::Initialize();

	SPLASH_STRING("Load AI data elements done...");

	if(createRobotInterface)
	{
		SPLASH_STRING("Create Robot Interface...");

		if(g_theProfileDB->IsAIOn() || network_Get().IsNetworkLaunch() )
		{
			PLAYER_INDEX ai_players[k_MAX_PLAYERS];

			sint32 next = 0;

			for (i=0; i< k_MAX_PLAYERS; i++)
			{
				if(g_player[i] && g_player[i]->IsRobot())
				{
					ai_players[next++] = PLAYER_INDEX(i);
				}
			}

			if(!g_theProfileDB->IsAIOn() && network_Get().IsNetworkLaunch())
			{
				g_theProfileDB->SetAI(TRUE);
			}
		}
	else
	{
			for(i = 0; i < k_MAX_PLAYERS; i++)
			{
				if(g_player[i])
					g_player[i]->m_playerType = PLAYER_TYPE_HUMAN;
			}
		}
	}

	tradeofferpool_Get()->ReRegisterOffers();

	{
		sint32 numPlaced = 0;
#ifdef _DEBUG
		FILE *mouseFile = fopen( "__debuginit__", "r" );
		if ( mouseFile )
		{
			fclose( mouseFile );

			gameinit_PlaceInitalUnits();
		}
	else
	{
			numPlaced = gameinit_PlaceInitalUnits(g_theProfileDB->GetNPlayers(), g_player_start_list);
		}
#else
		numPlaced = gameinit_PlaceInitalUnits(g_theProfileDB->GetNPlayers(), g_player_start_list);
#endif
		if(numPlaced < g_theProfileDB->GetNPlayers() - 1) {
			for (sint32 n = numPlaced; n < k_MAX_PLAYERS; n++)
            {
				std::unique_ptr<Player>{g_player[n]};
				g_player[n] = nullptr;
			}
		}
	}


	{
		sint32 visible = player_view::VisiblePlayer();
		if (visible >= 0 && g_player[visible] && !turn_Get()->IsHotSeat()) {
			player_view::Refresh();
		}
	}

	SPLASH_STRING("Update World stats...");

	world_Get()->A_star_heuristic->Update();
	world_Get()->RecalculateZOC();

#ifdef _DEBUG

	{
		for (sint32 x = 0; x < world_Get()->GetXWidth(); x++) {
			for (sint32 y = 0; y < world_Get()->GetYHeight(); y++)
            {
				Cell *cell = world_Get()->GetCell(x, y);
				for (sint32 u = 0; u < cell->GetNumUnits(); u++)
                {
					Assert(cell->AccessUnit(u).IsValid());
				}
			}
		}
	}
#endif

	SPLASH_STRING("Calculate good distances...");

	for(i=0; i<k_MAX_PLAYERS; i++)
	{
		if (g_player[i])
		{
			for (int j=0; j<g_player[i]->m_all_cities->Num(); j++)
			{
				g_player[i]->m_all_cities->Access(j)->GetCityData()->FindGoodDistances();
			}
		}
	}

	SPLASH_STRING("Reset vision...");
	render_observer::AddCopyVision();

	if(!network_Get().IsActive() && !network_Get().IsNetworkLaunch())
	{
		SPLASH_STRING("Check for Lemur...");

		if(strcmp(g_theProfileDB->GetLeaderName(), "Leemur") == 0)
		{
			g_player[1]->m_gold->SetLevel(1000000);

			g_player[1]->m_materialPool->AddMaterials(1000000);
		}
	}

	if(is_scenario_Get() && loadEverything && g_scenarioUsePlayerNumber > 0) {
		for(i = 0; i < k_MAX_PLAYERS; i++) {
			if(!g_player[i])
				continue;

			g_player[i]->m_playerType = PLAYER_TYPE_ROBOT;
		}

		if(g_scenarioUsePlayerNumber < k_MAX_PLAYERS && g_player[g_scenarioUsePlayerNumber])
			g_player[g_scenarioUsePlayerNumber]->m_playerType = PLAYER_TYPE_HUMAN;

		//Set current player the selected player so that the
		//game starts with the first turn and the correct player.
		player_view::SetCurrentPlayer(g_scenarioUsePlayerNumber);
		//Make sure that the current player is kept by turning it
		//into the stop player.
		NewTurnCount::SetStopPlayer(g_scenarioUsePlayerNumber);

	}
	else
	{
		g_scenarioUsePlayerNumber = 0;
	}

	{
		if (gameinit_IsHotseatGame() || gameinit_IsEmailGame())
		{
			bool foundFirstHuman = false;

			for(i = 1; i < g_theProfileDB->GetNPlayers(); i++)
			{
				Assert(g_player[i]);
				if(!g_player[i])
					continue;

				if (g_hsPlayerSetup[i].isHuman && !foundFirstHuman)
				{
					NewTurnCount::SetStopPlayer(player_view::CurPlayer());
					player_view::SetVisiblePlayer(i);
					foundFirstHuman = true;
				}

				if(g_hsPlayerSetup[i].isHuman && g_player[i]->IsRobot())
					g_player[i]->m_playerType = PLAYER_TYPE_HUMAN;
				else if(!g_hsPlayerSetup[i].isHuman && !g_player[i]->IsRobot())
					g_player[i]->m_playerType = PLAYER_TYPE_ROBOT;

				g_player[i]->m_civilisation->ResetCiv(g_hsPlayerSetup[i].civ, g_player[i]->m_civilisation->GetGender());
				if(g_player[i]->IsHuman())
				{
				if(!g_hsPlayerSetup[i].name.empty())
					g_player[i]->m_civilisation->AccessData()->SetLeaderName(g_hsPlayerSetup[i].name.c_str());

				if (i == 1)
				{
					g_theProfileDB->SetLeaderName(g_hsPlayerSetup[i].name.c_str());
				}
				g_player[i]->m_email = g_hsPlayerSetup[i].email;
				}
			}
		}

        turn_Get()->SetHotSeat(gameinit_IsHotseatGame());
        turn_Get()->SetEmail(gameinit_IsEmailGame());
	}

	gameinit_SetHotseatGame(FALSE);
	gameinit_SetEmailGame(FALSE);

	if (g_setDifficultyUponLaunch) {
		g_theProfileDB->SetDifficulty(g_difficultyToSetUponLaunch);
		g_setDifficultyUponLaunch = FALSE;
	}

	if (g_setBarbarianRiskUponLaunch) {
		g_theProfileDB->SetRiskLevel(g_barbarianRiskUponLaunch);
		g_setBarbarianRiskUponLaunch = FALSE;
	}

    if (gameobservers_Get()) {
        gameobservers_Get()->NotifySetGraphMinRound(is_scenario_Get() ? turn_Get()->GetRound() : 0);
    }

	// Clean good old -> new good table
	// Created in CityData if the good database was changed in size.
	std::unique_ptr<sint32[]>{g_newGoods};
	g_newGoods = nullptr;

	return 1;
}


void gameinit_CleanupMessages()
{
	if (g_player)
    {
    	for(size_t i = 0; i < k_MAX_PLAYERS; i++)
        {
		    if (g_player[i])
            {
			    g_player[i]->m_messages->KillList();
            }
		}
	}
}

void gameinit_Cleanup()
{
	// This must come before g_theArmyPool, since this is needed
	CtpAi::Cleanup();

	// Startup may fail before NewGame adopts the player array. Give the
	// session owner that storage before destroying any dependent pools.
	if (civapp_Get() && civapp_Get()->GetGame()) {
		auto *game = civapp_Get()->GetGame();
		if (g_player) game->AdoptPlayers(g_player);
		game->Cleanup();
	}
	allocated::clear(g_theCriticalMessagesPrefs);

	allocated::clear(g_theInstallationTree);
	allocated::clear(g_theUnitTree);
	player_view::Cleanup();




	allocated::clear(g_wormhole);

	allocated::clear(g_theOrderPond);
	allocated::clear(g_theUnseenPond);



#ifdef _DEBUG
	allocated::clear(g_theDiplomacyLog);
#endif
	Astar_Cleanup();

	roboinit_Cleanup();
}

sint32 gameinit_ResetForNetwork()
{
	installationpool_Set(std::make_unique<InstallationPool>().release());

	agreementpool_Set(std::make_unique<AgreementPool>().release());

	civilisationpool_Set(std::make_unique<CivilisationPool>().release());

	diplomaticrequestpool_Set(std::make_unique<DiplomaticRequestPool>().release());

	terrimprovepool_Set(std::make_unique<TerrainImprovementPool>().release());

	tradepool_Set(std::make_unique<TradePool>().release());

	unitpool_Set(std::make_unique<UnitPool>().release());  // Set() deletes the previous instance

	tradepool_Set(std::make_unique<TradePool>().release());

	tradeofferpool_Set(std::make_unique<TradeOfferPool>().release());

	armypool_Set(std::make_unique<ArmyPool>().release());

    std::unique_ptr<Pool<Order>>{g_theOrderPond};
	g_theOrderPond = std::make_unique<Pool<Order>>(INITIAL_CHUNK_LIST_SIZE).release();

    std::unique_ptr<Pool<UnseenCell>>{g_theUnseenPond};
	g_theUnseenPond = std::make_unique<Pool<UnseenCell>>(INITIAL_CHUNK_LIST_SIZE).release();

	return 0;
}

void gameinit_ResetMapSize()
{
	// Engine-side reset: rebuild tiledmap_Get(), reset each player's vision,
	// recompute continents.  UI build re-renders tileset, radar window,
	// and background via the OnMapResized observer hook fired at the end.
	MapPoint mapsize(world_Get()->GetXWidth(), world_Get()->GetYHeight());
	tiledmap_factory_recreate(mapsize.x, mapsize.y);

	for (int i = 0; i < k_MAX_PLAYERS; i++)
    {
		if (g_player[i])
        {
			g_player[i]->m_vision = std::make_unique<Vision>(i);
		}
	}

	world_Get()->NumberContinents();

    g_theUnitTree =
        std::make_unique<QuadTree<Unit>>(mapsize.x, mapsize.y, world_Get()->IsYwrap());

    g_theInstallationTree =
        std::make_unique<InstallationQuadTree>(mapsize.x, mapsize.y, world_Get()->IsYwrap());

    installationpool_Set(std::make_unique<InstallationPool>().release());

    agreementpool_Set(std::make_unique<AgreementPool>().release());

    diplomaticrequestpool_Set(std::make_unique<DiplomaticRequestPool>().release());

    terrimprovepool_Set(std::make_unique<TerrainImprovementPool>().release());

    tradepool_Set(std::make_unique<TradePool>().release());

    unitpool_Set(std::make_unique<UnitPool>().release());

    tradepool_Set(std::make_unique<TradePool>().release());

    armypool_Set(std::make_unique<ArmyPool>().release());

    std::unique_ptr<Pool<Order>>{g_theOrderPond};
    g_theOrderPond = std::make_unique<Pool<Order>>(INITIAL_CHUNK_LIST_SIZE).release();

    std::unique_ptr<Pool<UnseenCell>>{g_theUnseenPond};
    g_theUnseenPond = std::make_unique<Pool<UnseenCell>>(INITIAL_CHUNK_LIST_SIZE).release();

    CtpAi::Initialize();

    // Let the UI (if any) re-render tileset, radar window, background, etc.
    if (gameobservers_Get()) gameobservers_Get()->NotifyMapResized();
}
