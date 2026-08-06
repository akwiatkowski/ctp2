//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : UI-free command/query dispatch for the test/automation API.
//
//----------------------------------------------------------------------------
//
// See game_controller.h for the design rationale.  Every handler here operates
// purely on game-state objects and is safe to call in both the interactive and
// headless builds.  Responses are built with nlohmann::json and dumped as a
// single line (no embedded newlines), so they ride the existing
// newline-delimited socket protocol without truncation.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ctp/game_controller.h"

#include <nlohmann/json.hpp>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "ctp/civapp.h"                       // civapp_Get()->IsGameLoaded()
#include "ctp/ctp2_utils/civlog.h"            // civlog::Get
#include "gs/utility/Globals.h"               // k_MAX_PLAYERS, k_GAME_OBJ_TYPE_*
#include "gs/utility/MoveFlags.h"             // k_MOVEMENT_TYPE_* render fixture terrain env
#include "gs/utility/safety.h"                // safe_shift_left_u64
#include "gs/gameobj/player.h"                // player_Get, Player
#include "gs/gameobj/Army.h"                  // Army
#include "gs/gameobj/ArmyData.h"              // ArmyData::Settle / CanSettle
#include "gs/gameobj/Unit.h"                  // Unit
#include "gs/gameobj/UnitData.h"              // Unit::GetData / GetCityData
#include "gs/gameobj/citydata.h"              // CityData
#include "gs/gameobj/BldQue.h"                // BuildQueue / BuildNode
#include "gs/gameobj/Vision.h"                // Vision::IsVisible / IsExplored
#include "gs/world/World.h"                   // world_Get(), GetCell
#include "gs/world/Cell.h"                    // Cell terrain / city / units
#include "gs/world/TileInfo.h"                // debug_set_terrain fixture cleanup
#include "gs/utility/UnitDynArr.h"            // UnitDynamicArray
#include "gs/fileio/gamefile.h"               // GameFile::SaveGame / RestoreGame
#include "gs/fileio/action_log.h"             // action_log::Get / Count / Clear
#include "gs/events/GameEventManager.h"       // gevmanager_Get()->Process()
#include "gs/core/game_observer.h"            // gameobservers_Get()
#include "gs/core/player_view.h"              // player_view::SetCurrentPlayer
#include "gs/core/tiledmap_observer.h"        // render-fixture tile postprocess
#include "gs/gameobj/MovePath.h"              // army_QueueMovePath
#include "gs/gameobj/Events.h"                // GEV_ExploreOrder / AI events
#include "gs/gameobj/Score.h"                 // Score::GetTotalScore
#include "gs/gameobj/Strengths.h"             // Strengths::GetStrength (rank inputs)
#include "gs/gameobj/Civilisation.h"          // Civilisation::Get*CivName
#include "gs/utility/TurnCnt.h"               // turn_Get()->GetRound/GetYear
#include "ConstRecord.h"                      // g_theConstDB (end-of-game year)
#include "UnitRecord.h"                       // g_theUnitDB, UnitRecord
#include "TerrainRecord.h"                    // g_theTerrainDB, TerrainRecord
#include "TerrainImprovementRecord.h"         // debug terrain overlay
#include "BuildingRecord.h"                   // g_theBuildingDB, BuildingRecord
#include "WonderRecord.h"                     // g_theWonderDB, WonderRecord
#include "GovernmentRecord.h"                 // g_theGovernmentDB, GovernmentRecord
#include "gs/gameobj/PlayHap.h"              // PlayerHappiness rate-slider levels
#include "gs/gameobj/UnitTypes.h"            // POP_TYPE (specialists)
#include "BuildListSequenceRecord.h"         // g_theBuildListSequenceDB (governor profiles)
#include "OrderRecord.h"                      // g_theOrderDB, OrderRecord (unit orders)
#include "robot/pathing/Path.h"              // Path (PerformOrderHere target)
#include "gs/utility/TradeDynArr.h"          // TradeDynamicArray (cancel_trade_route)
#include "ResourceRecord.h"                   // g_theResourceDB (trade goods)
#include "gs/gameobj/TradeRoute.h"            // TradeRoute, ROUTE_TYPE
#include "ai/diplomacy/Diplomat.h"            // Diplomat::DeclareWar / ExecuteNewProposal
#include "ai/diplomacy/AgreementMatrix.h"     // AgreementMatrix::HasAgreement (peace treaty)
#include "gs/gameobj/Gold.h"                  // Player gold level (buy_production)
#include "gs/gameobj/ArmyPool.h"              // armypool_Get
#include "AdvanceRecord.h"                    // g_theAdvanceDB, AdvanceRecord
#include "gs/gameobj/Advances.h"              // Advances::CanResearch/GetCost
#include "gs/gameobj/terrainutil.h"           // terrainutil_CanPlayerBuildAt/cost/time
#include "gs/gameobj/unitutil.h"              // unitutil_GetSeaCity / city type
#include "gs/gameobj/TerrImprove.h"           // TerrainImprovement
#include "gs/gameobj/TerrImprovePool.h"       // terrimprovepool_Get
#include "gs/database/profileDB.h"            // profiledb_Get()->IsAIOn()
#include "ai/ctpai.h"                         // CtpAi::BeginDiplomacy
#include "ui/aui_sdl/aui_sdl.h"               // GPU world diagnostics
#include "gfx/tilesys/tiledmap.h"             // debug terrain-overlay fallback
#include "gfx/tilesys/tileset.h"              // debug_tileset_stats (GPU raster probe)
#include "gfx/tilesys/BaseTile.h"             // debug_tileset_stats (GPU raster probe)
#include "gfx/spritesys/director.h"            // debug combat flash
#include "ui/interface/scenarioeditor.h"       // debug scenario start flags

using json = nlohmann::json;

namespace game_controller {

// Human-readable labels for the coarse victory state stored on Score.
// These mirror eScoreVictory in gs/gameobj/Score.h.
const char * VictoryTypeLabel(sint32 type)
{
    switch (type) {
        case kScoreGameInProgress: return "in_progress";
        case kScoreDefeat:         return "defeat";
        case kScoreSoloVictory:    return "solo";
        case kScoreAlliedVictory:  return "allied";
        case kScoreWonderVictory:  return "wonder";
        default:                   return "unknown";
    }
}

// The "visible player" whose viewpoint queries report — the (single) human.
// We deliberately do NOT use selitem_Get() here: it is a UI singleton that may
// be absent/empty headless.  The human player's own vision is the correct,
// build-independent source for "what the player can see".
Player * HumanPlayer()
{
    for (sint32 p = 0; p < k_MAX_PLAYERS; ++p) {
        if (player_Get(p) && player_Get(p)->IsHuman())
            return player_Get(p);
    }
    return nullptr;
}

void RunRound(sint32 round, SetCurrentPlayerFn set_current_player)
{
    auto set_current = [set_current_player](sint32 player) {
        if (set_current_player) {
            set_current_player(player);
        } else {
            player_view::SetCurrentPlayer(player);
        }
    };

    if (turn_Get()) turn_Get()->SkipToRound(round);

    for (sint32 p = 0; p < k_MAX_PLAYERS; ++p) {
        if (!player_Get(p) || player_Get(p)->IsDead()) continue;

        set_current(p);

        if (profiledb_Get()->IsAIOn()) {
            gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_AiBeginMapAnalysis,
                                   GEA_Player, p, GEA_End);
        }
        CtpAi::BeginDiplomacy(p, round);
        if (profiledb_Get()->IsAIOn()) {
            gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_AiBeginTurn,
                                   GEA_Player, p, GEA_End);
        }

        // BeginTurn() already calls NotifyTurnStart internally; only
        // NotifyTurnEnd needs an explicit call because EndTurn() does not
        // notify observers.
        player_Get(p)->BeginTurn();

        // Automation bypasses the UI director path that queues the scheduler,
        // so add it directly and drain AI events before advancing players.
        if (gevmanager_Get()) {
            gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_BeginScheduler,
                                   GEA_Player, p, GEA_End);
            gevmanager_Get()->Process();
        }

        // Resume queued multi-turn orders. Cargo never self-executes: the UI
        // cannot select an army riding a transport, so interactive play never
        // fires BeginTurnExecute for it either.
        if (gevmanager_Get() && player_Get(p)->m_all_armies) {
            for (sint32 a = 0; a < player_Get(p)->m_all_armies->Num(); ++a) {
                Army army = player_Get(p)->m_all_armies->Access(a);
                if (!army.IsValid() || army.NumOrders() == 0) continue;
                if (army.Num() > 0 && army.Access(0).IsBeingTransported())
                    continue;
                gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
                                           GEV_BeginTurnExecute,
                                           GEA_Army, army, GEA_End);
            }
            gevmanager_Get()->Process();
        }

        player_Get(p)->EndTurn();
        if (gameobservers_Get()) gameobservers_Get()->NotifyTurnEnd(p);
    }

    if (turn_Get()) turn_Get()->SkipToRound(round + 1);

    if (Player * human = HumanPlayer()) {
        set_current(human->GetOwner());
    }
}

}  // namespace game_controller

// Owned by tiledmap.cpp; the graphics options screen is the only other writer.
extern sint32 g_isGridOn;

extern int s_goodCellsSeen, s_goodNoActor, s_goodDeclined, s_goodEmitted;   // PROBE
extern char const * s_goodReason;   // PROBE

namespace {

using game_controller::HumanPlayer;

auto gc_log = civlog::Get("gamectl");

// ---- response builders --------------------------------------------------

// Game strings reach us in TWO encodings: fresh-game strings come from the
// StringDB in Latin-1 (a Mali city named with an 0xE9 'é' broke
// query_player_cities live — nlohmann::json::dump() throws type_error.316
// on invalid UTF-8), while strings that round-tripped through a JSON save
// come back as valid UTF-8 (json_save transcodes on write but not back on
// load — see plan: the deep fix is load-side UTF-8→Latin-1 so the UI font
// path stays consistent). So: pass valid UTF-8 through untouched, and
// transcode anything else as Latin-1.
bool IsValidUtf8(const unsigned char * p)
{
    while (*p) {
        if (*p < 0x80) { ++p; continue; }
        int extra = (*p >= 0xF0) ? 3 : (*p >= 0xE0) ? 2 : (*p >= 0xC2) ? 1 : -1;
        if (extra < 0) return false;
        ++p;
        for (int i = 0; i < extra; ++i, ++p)
            if ((*p & 0xC0) != 0x80) return false;
    }
    return true;
}

std::string ToUtf8(const char * s)
{
    std::string out;
    if (!s) return out;
    if (IsValidUtf8((const unsigned char *)s)) return s;
    for (const unsigned char * p = (const unsigned char *)s; *p; ++p) {
        if (*p < 0x80) {
            out += (char)*p;
        } else {
            out += (char)(0xC0 | (*p >> 6));
            out += (char)(0x80 | (*p & 0x3F));
        }
    }
    return out;
}

sint32 ResolveUnitType(const char * name)
{
    if (!g_theUnitDB || !name || !*name)
        return -1;

    sint32 type = -1;
    if (sscanf(name, "%d", &type) == 1)
        return (type >= 0 && type < g_theUnitDB->NumRecords()) ? type : -1;

    for (sint32 i = 0; i < g_theUnitDB->NumRecords(); ++i) {
        const UnitRecord * rec = g_theUnitDB->Get(i);
        if (rec && rec->GetIDText() && strcmp(rec->GetIDText(), name) == 0)
            return i;
    }
    return -1;
}

#if defined(RENDER_TOOL_BUILD)
// Only the render-fixture terrain path needs this; the gameplay path gets its
// movement mask from World::SmartSetTerrain.
uint32 MovementMaskFromTerrain(const TerrainRecord * rec)
{
    uint32 movement = 0;
    if (!rec)
        return movement;
    if (rec->GetMovementTypeLand())         movement |= k_MOVEMENT_TYPE_LAND;
    if (rec->GetMovementTypeSea())          movement |= k_MOVEMENT_TYPE_WATER;
    if (rec->GetMovementTypeAir())          movement |= k_MOVEMENT_TYPE_AIR;
    if (rec->GetMovementTypeMountain())     movement |= k_MOVEMENT_TYPE_MOUNTAIN;
    if (rec->GetMovementTypeTrade())        movement |= k_MOVEMENT_TYPE_TRADE;
    if (rec->GetMovementTypeShallowWater()) movement |= k_MOVEMENT_TYPE_SHALLOW_WATER;
    if (rec->GetMovementTypeSpace())        movement |= k_MOVEMENT_TYPE_SPACE;
    return movement;
}
#endif

// {"status":"ok","cmd":"<verb>","result":{...}}  (result omitted if null)
std::string Ok(const char * verb, const json & result = json())
{
    json r;
    r["status"] = "ok";
    r["cmd"]    = verb;
    if (!result.is_null())
        r["result"] = result;
    return r.dump();
}

// {"status":"error","cmd":"<verb>","detail":"<code>"}
std::string Err(const char * verb, const char * code)
{
    json r;
    r["status"] = "error";
    r["cmd"]    = verb;
    r["detail"] = code;
    return r.dump();
}

// ---- commands -----------------------------------------------------------

// Found a city with the human player's first settler-capable army.  Settle()
// only queues a GEV_Settle event, so we pump the event manager to make the
// command synchronous: the city exists by the time we respond.
std::string CmdBuildCity()
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("build_city", "game_not_loaded");

    Player * human = HumanPlayer();
    if (!human)
        return Err("build_city", "no_human_player");

    DynamicArray<Army> * armies = human->GetAllArmiesList();
    for (sint32 i = 0; i < armies->Num(); ++i) {
        Army army = armies->Access(i);
        ArmyData * ad = army.AccessData();
        if (army.IsValid() && army.CanSettle() && ad) {
            // CanSettle() only checks the UNIT can settle, not the tile.
            // Settling on a tile that already has a city silently REPLACES
            // it (pop and improvements lost) — refuse instead. Found by the
            // first MCP playtest: a settler stuck on Rome's tile "founded"
            // a fresh pop-1 Rome over the pop-2 original.
            MapPoint at = ad->RetPos();
            // Settling is a "special action": it needs movement points
            // (UnitData::CanPerformSpecialAction). A 0-move settler has
            // its settle vetoed downstream, which used to surface as a
            // (bogus) distance rejection — every "distance" rejection in
            // the 2026-06-11 playtest was actually this.
            if (!ad->CanPerformSpecialAction())
                return Err("build_city",
                           "no_moves_left (settling is a special action — "
                           "end_turn so the settler has movement points, "
                           "then retry)");
            Cell * cell = world_Get() ? world_Get()->GetCell(at) : nullptr;
            if (cell && cell->GetCity().IsValid())
                return Err("build_city", "tile_occupied");
            // Pre-check the engine's own CreateCity veto (IsNextToCity uses
            // ISO-grid adjacency, not x/y distance). Vital: the settle event
            // chain queues GEV_KillUnit BEFORE GEV_CreateCity, so a vetoed
            // settle DESTROYS the settler — campaign 7 lost a 740-shield
            // settler learning this. Refuse here, while the unit still lives.
            if (world_Get() &&
                (world_Get()->IsCity(at) || world_Get()->IsNextToCity(at)))
                return Err("build_city",
                           "too_close_to_city (tile is iso-adjacent to an "
                           "existing city; move at least one more tile away)");
            if (cell && cell->GetCityOwner().IsValid() &&
                cell->GetCityOwner().GetOwner() != human->GetOwner())
                return Err("build_city",
                           "inside_foreign_territory (another city's borders "
                           "block settling here)");
            gc_log->info("build_city: settling with army {} of player {}",
                         i, (int)human->GetOwner());
            ad->Settle();
            // Drain the queued GEV_Settle (and any cascade) so the city is
            // actually founded before we return — keeps the driver synchronous.
            if (gevmanager_Get())
                gevmanager_Get()->Process();
            // The settle event can be VETOED downstream (city minimum
            // distance, terrain rules) with no error surfaced — verify the
            // city actually exists instead of reporting blind success.
            cell = world_Get() ? world_Get()->GetCell(at) : nullptr;
            if (!cell || !cell->GetCity().IsValid()) {
                // Name the most common veto cause (minimum city distance)
                // instead of leaving the driver to guess — campaign 7
                // burned turns probing rejected sites blind.
                sint32 nearest = -1;
                for (sint32 p = 0; p < k_MAX_PLAYERS; ++p) {
                    Player * pl = player_Get(p);
                    if (!pl || !pl->m_all_cities) continue;
                    for (sint32 c = 0; c < pl->m_all_cities->Num(); ++c) {
                        Unit city = pl->m_all_cities->Access(c);
                        if (!city.IsValid()) continue;
                        MapPoint cpos;
                        city.GetPos(cpos);
                        sint32 d = std::max(std::abs((int)cpos.x - (int)at.x),
                                            std::abs((int)cpos.y - (int)at.y));
                        if (nearest < 0 || d < nearest) nearest = d;
                    }
                }
                char detail[80];
                snprintf(detail, sizeof(detail),
                         "settle_rejected nearest_city_distance=%d (minimum ~3)",
                         (int)nearest);
                return Err("build_city", detail);
            }
            json result;
            result["pos"] = { {"x", at.x}, {"y", at.y} };
            return Ok("build_city", result);
        }
    }
    return Err("build_city", "no_settler_found");
}

std::string CmdEndTurn(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("end_turn", "game_not_loaded");

    int turns = 1;
    if (args && *args && sscanf(args, "%d", &turns) != 1)
        return Err("end_turn", "bad_args");
    if (turns < 1 || turns > 20)
        return Err("end_turn", "out_of_range");

    sint32 const startRound = turn_Get() ? turn_Get()->GetRound() : 0;
    for (int i = 0; i < turns; ++i)
        game_controller::RunRound(startRound + i, nullptr);

    json result;
    result["round"] = turn_Get() ? turn_Get()->GetRound() : startRound + turns;
    return Ok("end_turn", result);
}

std::string CmdSetShowCityNames(const char * args)
{
    int on = 0;
    if (sscanf(args, "%d", &on) != 1)
        return Err("set_show_city_names", "bad_args");

    profiledb_Get()->SetShowCityNames(on != 0);
    if (tiledmap_Get())
        tiledmap_Get()->BuildTerrainQuads();
    json result;
    result["show_city_names"] = profiledb_Get()->GetShowCityNames() != FALSE;
    return Ok("set_show_city_names", result);
}

std::string CmdDebugTerrainOverlay(const char * args)
{
    sint32 x = 0, y = 0;
    if (sscanf(args, "%d %d", &x, &y) != 2)
        return Err("debug_terrain_overlay", "bad_args");
    if (!tiledmap_Get())
        return Err("debug_terrain_overlay", "no_tiledmap");
    MapPoint pos(x, y);
    const TerrainImprovementRecord *rec = nullptr;
    for (sint32 i = 0; g_theTerrainImprovementDB && i < g_theTerrainImprovementDB->NumRecords(); ++i) {
        const TerrainImprovementRecord *candidate = g_theTerrainImprovementDB->Get(i);
        const TerrainImprovementRecord::Effect *effect = candidate
            ? ((candidate->GetClassTerraform() || candidate->GetClassOceanform())
                ? candidate->GetTerrainEffect(0)
                : terrainutil_GetTerrainEffect(candidate, pos))
            : nullptr;
        if (effect && effect->GetTilesetIndex() > 0) {
            rec = candidate;
            break;
        }
    }
    if (!rec)
        return Err("debug_terrain_overlay", "no_overlay_record");

    tiledmap_Get()->SetTerrainOverlay(const_cast<TerrainImprovementRecord *>(rec), pos, 0xffff);
    tiledmap_Get()->BuildTerrainQuads();
    return Ok("debug_terrain_overlay");
}

std::string CmdDebugSetTerrain(const char * args)
{
	sint32 x = 0, y = 0, terrain = 0;
	if (sscanf(args, "%d %d %d", &x, &y, &terrain) != 3)
		return Err("debug_set_terrain", "bad_args");
	World *w = world_Get();
	if (!w)
		return Err("debug_set_terrain", "no_world");
	if (!g_theTerrainDB || terrain < 0 || terrain >= g_theTerrainDB->NumRecords())
		return Err("debug_set_terrain", "bad_terrain");
	if (x < 0 || y < 0 || x >= w->GetXWidth() || y >= w->GetYHeight())
		return Err("debug_set_terrain", "out_of_bounds");

	MapPoint pos(x, y);
#if defined(RENDER_TOOL_BUILD)
	// Render fixtures must not invoke scenario-editor terrain logic: it can rewrite
	// neighbours and let gameplay consequences leak into visual comparison runs.
	w->SetTerrain(x, y, terrain);
	w->SetMovementType(x, y, MovementMaskFromTerrain(g_theTerrainDB->Get(terrain)));
	for (sint32 dy = -1; dy <= 1; ++dy) {
		for (sint32 dx = -1; dx <= 1; ++dx) {
			sint32 const px = x + dx;
			sint32 const py = y + dy;
			if (px < 0 || py < 0 || px >= w->GetXWidth() || py >= w->GetYHeight())
				continue;
			MapPoint p(px, py);
			tiledmap_observer::PostProcessTile(p, w->GetTileInfo(p));
			tiledmap_observer::RedrawTile(p);
		}
	}
#else
	w->SmartSetTerrain(pos, terrain, 0);
#endif
	if (tiledmap_Get()) {
		if (TileInfo *tileInfo = tiledmap_Get()->GetTileInfo(pos))
			tileInfo->SetRiverPiece(-1);
		tiledmap_Get()->BuildTerrainQuads();
	}

	json result;
	result["pos"] = { {"x", x}, {"y", y} };
	result["terrain"] = terrain;
	return Ok("debug_set_terrain", result);
}

std::string CmdDebugClearRivers(const char * args)
{
	sint32 cx = 0, cy = 0, radius = 0;
	if (sscanf(args, "%d %d %d", &cx, &cy, &radius) != 3 || radius < 0)
		return Err("debug_clear_rivers", "bad_args");
	World *w = world_Get();
	TiledMap *map = tiledmap_Get();
	if (!w || !map)
		return Err("debug_clear_rivers", "no_world");

	for (sint32 dy = -radius; dy <= radius; ++dy) {
		for (sint32 dx = -radius; dx <= radius; ++dx) {
			sint32 const x = cx + dx;
			sint32 const y = cy + dy;
			if (x < 0 || y < 0 || x >= w->GetXWidth() || y >= w->GetYHeight())
				continue;
			MapPoint pos(x, y);
			if (TileInfo *tileInfo = map->GetTileInfo(pos))
				tileInfo->SetRiverPiece(-1);
		}
	}
	map->BuildTerrainQuads();
	return Ok("debug_clear_rivers");
}

std::string CmdDebugClearTerrainLayers(const char * args)
{
	sint32 cx = 0, cy = 0, radius = 0;
	if (sscanf(args, "%d %d %d", &cx, &cy, &radius) != 3 || radius < 0)
		return Err("debug_clear_terrain_layers", "bad_args");
	World *w = world_Get();
	TiledMap *map = tiledmap_Get();
	if (!w || !map)
		return Err("debug_clear_terrain_layers", "no_world");

	uint32 const envMask = k_MASK_ENV_INSTALLATION
	                   | k_MASK_ENV_MINE
	                   | k_MASK_ENV_IRRIGATION
	                   | k_MASK_ENV_ROAD
	                   | k_MASK_ENV_CANAL_TUNNEL;
	for (sint32 dy = -radius; dy <= radius; ++dy) {
		for (sint32 dx = -radius; dx <= radius; ++dx) {
			sint32 const x = cx + dx;
			sint32 const y = cy + dy;
			if (x < 0 || y < 0 || x >= w->GetXWidth() || y >= w->GetYHeight())
				continue;
			MapPoint pos(x, y);
			if (TileInfo *tileInfo = map->GetTileInfo(pos))
				tileInfo->SetRiverPiece(-1);
			if (Cell *cell = w->GetCell(pos)) {
				cell->SetEnv(cell->GetEnv() & ~envMask);
				while (cell->GetNumImprovements() > 0)
					cell->RemoveImprovement(cell->AccessImprovement(0));
				while (cell->GetNumDBImprovements() > 0)
					cell->RemoveDBImprovement(cell->GetDBImprovement(0));
				cell->DeleteGoodyHut();
			}
		}
	}
	map->BuildTerrainQuads();
	return Ok("debug_clear_terrain_layers");
}

// Toggle the tile grid. It is a global the graphics options screen owns, with
// no way in from a test; the grid is the one per-cell overlay that affects
// EVERY cell at once, which makes it the decisive check that the whole-map
// path composites overlays at all (P13 step 3).
std::string CmdDebugSetGrid(const char * args)
{
	int on = -1;
	if (!args || sscanf(args, "%d", &on) != 1 || (on != 0 && on != 1))
		return Err("debug_set_grid", "bad_args");

	::g_isGridOn = on;
	// Every cached cell image is now stale: the grid is part of the whole-map
	// tile picture, not a separate pass.
	if (tiledmap_Get())
	{
		tiledmap_Get()->InvalidateWorldmap();
		tiledmap_Get()->BuildTerrainQuads();
	}
	json result;
	result["grid"] = on;
	return Ok("debug_set_grid", result);
}

// Political border display: on/off and which STYLE. Both live in the graphics
// options screen with no way in from a test, and the style matters: smooth
// borders stamp a corner icon while the line style draws a colored edge through
// completely different code. The whole-map path composited only the icon style
// for a while (#14260), which no test could have caught without this.
std::string CmdDebugSetBorders(const char * args)
{
	int on = -1, smooth = -1;
	if (!args || sscanf(args, "%d %d", &on, &smooth) != 2
	 || (on != 0 && on != 1) || (smooth != 0 && smooth != 1))
		return Err("debug_set_borders", "bad_args");
	if (!profiledb_Get())
		return Err("debug_set_borders", "no_profile");

	// Report what they WERE. These settings persist to userprofile.txt on exit,
	// so a test that changes them silently changes the user's game (and the
	// next test run's baseline). Returning the previous values lets a caller
	// put them back.
	json result;
	result["was"] = { {"borders", profiledb_Get()->GetShowPoliticalBorders() ? 1 : 0},
	                  {"smooth",  profiledb_Get()->IsSmoothBorders() ? 1 : 0} };

	profiledb_Get()->SetShowPoliticalBorders(on);
	profiledb_Get()->SetShowSmooth(smooth);
	// Borders are part of the whole-map tile picture, so every cached cell
	// image is stale — same reasoning as the grid above.
	if (tiledmap_Get())
	{
		tiledmap_Get()->InvalidateWorldmap();
		tiledmap_Get()->BuildTerrainQuads();
	}
	result["borders"] = on;
	result["smooth"]  = smooth;
	return Ok("debug_set_borders", result);
}

// Put a patch into FOG: explored, but not currently visible.
//
// There was no way to reach that state from a test, and it is the state most
// of an explored map is in for most of a game. debug_reveal_patch only ever
// produces lit terrain, so the whole-map path's handling of fog (#12839) could
// not be developed or tested at all.
//
// Note Vision::AddExplored is NOT "explore without seeing" — it is the same
// FillCircle(CIRCLE_OP_ADD) call as AddVisible. Visibility is a REFERENCE
// COUNT in the low bits with the explored flag as the top bit, so adding sets
// both and there is no add-explored-only primitive. Fog is made by dropping
// the reference again: the count returns to zero (unless a unit really can see
// the cell, which is correct) while the explored bit stays set.
std::string CmdDebugExplorePatch(const char * args)
{
	sint32 x = 0, y = 0, radius = 0;
	if (!args || sscanf(args, "%d %d %d", &x, &y, &radius) != 3 || radius < 0)
		return Err("debug_explore_patch", "bad_args");
	World *w = world_Get();
	Player *human = HumanPlayer();
	if (!w || !human || !human->m_vision)
		return Err("debug_explore_patch", "no_world");
	if (x < 0 || y < 0 || x >= w->GetXWidth() || y >= w->GetYHeight())
		return Err("debug_explore_patch", "bad_position");

	MapPoint const pos(x, y);
	double const r = static_cast<double>(radius);
	human->m_vision->AddVisible(pos, r);
	human->m_vision->RemoveVisible(pos, r);
	if (tiledmap_Get()) {
		tiledmap_Get()->CopyVision();
		tiledmap_Get()->BuildTerrainQuads();
	}
	json result;
	result["pos"] = { {"x", x}, {"y", y} };
	result["radius"] = radius;
	return Ok("debug_explore_patch", result);
}

// Draw explored terrain at full brightness, ignoring current vision.
//
// The deterministic way to take fog OUT of a comparison. Revealing a radius
// does not do that: visibility decays, so two processes capturing the same
// scene drift apart during the seconds a capture takes to settle, and once the
// whole-map path started drawing fog (#12839) that drift became the largest
// difference in a transition-parity run. This flag is honoured by every render
// path and does not decay.
std::string CmdDebugRenderExploredAsVisible(const char * args)
{
	int on = -1;
	if (!args || sscanf(args, "%d", &on) != 1 || (on != 0 && on != 1))
		return Err("debug_render_explored_as_visible", "bad_args");
	if (!tiledmap_Get())
		return Err("debug_render_explored_as_visible", "no_tiledmap");

	tiledmap_Get()->SetRenderExploredAsVisible(on != 0);
	// Fog is baked into the whole-map tile pictures, so every cached cell image
	// is now stale — same reasoning as the grid and the border settings.
	tiledmap_Get()->InvalidateWorldmap();
	tiledmap_Get()->BuildTerrainQuads();

	json result;
	result["render_explored_as_visible"] = on;
	return Ok("debug_render_explored_as_visible", result);
}

// Explored / visible cell counts over a square patch, read from BOTH the
// human player's Vision and the one the tile map is actually rendering
// through. Fog is "explored and not visible", and nothing could observe that
// state: a test could set it up and screenshot the result, but if the frame
// did not change there was no way to tell whether fog failed to draw or the
// visibility never moved in the first place. Reporting both sides also catches
// the two drifting apart, which is a real possibility -- CopyVision aliases
// m_localVision to whichever player is being viewed, not necessarily the human.
std::string CmdDebugVisionStats(const char * args)
{
	sint32 x = 0, y = 0, radius = 0;
	if (!args || sscanf(args, "%d %d %d", &x, &y, &radius) != 3 || radius < 0)
		return Err("debug_vision_stats", "bad_args");
	World *w = world_Get();
	Player *human = HumanPlayer();
	if (!w || !human || !human->m_vision)
		return Err("debug_vision_stats", "no_world");

	Vision const * local = tiledmap_Get() ? tiledmap_Get()->GetLocalVision() : nullptr;

	sint32 cells = 0;
	sint32 humanExplored = 0, humanVisible = 0;
	sint32 localExplored = 0, localVisible = 0;
	for (sint32 dy = -radius; dy <= radius; ++dy)
	{
		for (sint32 dx = -radius; dx <= radius; ++dx)
		{
			sint32 const cx = x + dx, cy = y + dy;
			if (cx < 0 || cy < 0 || cx >= w->GetXWidth() || cy >= w->GetYHeight())
				continue;
			MapPoint const pos(cx, cy);
			++cells;
			if (human->m_vision->IsExplored(pos)) ++humanExplored;
			if (human->m_vision->IsVisible(pos))  ++humanVisible;
			if (local)
			{
				if (local->IsExplored(pos)) ++localExplored;
				if (local->IsVisible(pos))  ++localVisible;
			}
		}
	}

	json result;
	result["cells"]  = cells;
	result["human"]  = { {"explored", humanExplored}, {"visible", humanVisible} };
	result["local"]  = { {"explored", localExplored}, {"visible", localVisible},
	                     {"present", local != nullptr} };
	// The cells that should render fogged.
	result["fogged"] = localExplored - localVisible;
	return Ok("debug_vision_stats", result);
}

// Nearest map good to a position. Goods are placed at generation and there is
// no query for them, which makes "does a good render" awkward to test.
std::string CmdDebugWorldmapSprites(const char * args)
{
	int on = -1;
	if (!args || sscanf(args, "%d", &on) != 1 || (on != 0 && on != 1))
		return Err("debug_worldmap_sprites", "bad_args");
	aui_SDL::SetWorldmapSprites(on != 0);
	json result; result["sprites"] = on;
	return Ok("debug_worldmap_sprites", result);
}

std::string CmdDebugFindGood(const char * args)
{
	sint32 fx = 0, fy = 0;
	if (!args || sscanf(args, "%d %d", &fx, &fy) != 2)
		return Err("debug_find_good", "bad_args");
	World * w = world_Get();
	if (!w) return Err("debug_find_good", "no_world");

	sint32 bestD = 0x7fffffff, bx = -1, by = -1;
	for (sint32 y = 0; y < w->GetYHeight(); ++y)
		for (sint32 x = 0; x < w->GetXWidth(); ++x)
		{
			MapPoint p(x, y);
			if (!w->IsGood(p)) continue;
			sint32 const d = (x - fx) * (x - fx) + (y - fy) * (y - fy);
			if (d < bestD) { bestD = d; bx = x; by = y; }
		}
	if (bx < 0) return Err("debug_find_good", "no_good_on_map");

	json result;
	result["pos"] = { {"x", bx}, {"y", by} };
	return Ok("debug_find_good", result);
}

// Opaque pixel count from the last icon the GPU decoder built. Border icons
// render as nothing on the whole-map path and this says whether the decode
// produced anything to draw.
std::string CmdDebugIconAlpha(const char * args)
{
	if (!tiledmap_Get() || !tiledmap_Get()->GetTileSet())
		return Err("debug_icon_alpha", "no_tileset");
	TileSet * ts = tiledmap_Get()->GetTileSet();

	json result;
	json icons = json::array();
	MAPICON const probe[] = { MAPICON_POLBORDERNW, MAPICON_POLBORDERSW,
	                          MAPICON_POLBORDERNE, MAPICON_POLBORDERSE };
	char const * names[] = { "NW", "SW", "NE", "SE" };
	for (int i = 0; i < 4; ++i)
	{
		Pixel16 * data = ts->GetMapIconData(probe[i]);
		POINT dim = ts->GetMapIconDimensions(probe[i]);
		json e;
		e["edge"] = names[i];
		e["has_data"] = (data != nullptr);
		e["w"] = dim.x; e["h"] = dim.y;
		if (data && dim.x > 0 && dim.y > 0)
		{
			aui_SDL::EnsureMapIconTexture(data, dim.x, dim.y, 0x7c00);
			e["opaque_pixels"] = aui_SDL::LastIconOpaquePixels();
		}
		icons.push_back(e);
	}
	result["icons"] = icons;
	return Ok("debug_icon_alpha", result);
}

std::string CmdDebugRevealPatch(const char * args)
{
	sint32 x = 0, y = 0, radius = 0;
	if (sscanf(args, "%d %d %d", &x, &y, &radius) != 3 || radius < 0)
		return Err("debug_reveal_patch", "bad_args");
	World *w = world_Get();
	Player *human = HumanPlayer();
	if (!w || !human || !human->m_vision)
		return Err("debug_reveal_patch", "no_world");
	if (x < 0 || y < 0 || x >= w->GetXWidth() || y >= w->GetYHeight())
		return Err("debug_reveal_patch", "bad_position");

	MapPoint pos(x, y);
	double const revealRadius = static_cast<double>(radius);
	human->m_vision->AddExplored(pos, revealRadius);
	human->m_vision->AddVisible(pos, revealRadius);
	if (tiledmap_Get()) {
		tiledmap_Get()->CopyVision();
		tiledmap_Get()->BuildTerrainQuads();
	}
	json result;
	result["pos"] = { {"x", x}, {"y", y} };
	result["radius"] = radius;
	return Ok("debug_reveal_patch", result);
}

#if defined(RENDER_TOOL_BUILD) && defined(USE_SDL)
// P13 step 1 — build (or incrementally update) the whole-map GPU target and
// report how many cells were redrawn. The count IS the contract: the first call
// draws the explored map, and a call that follows a pan or zoom must draw zero.
std::string CmdDebugWorldmapBuild(const char * args)
{
	if (!aui_SDL::GpuWorldmapEnabled())
		return Err("debug_worldmap_build", "worldmap_disabled");
	if (!tiledmap_Get())
		return Err("debug_worldmap_build", "no_tiledmap");

	// "rebuild" forces a full recomposite; otherwise the dirty path applies.
	if (args && strstr(args, "rebuild") != nullptr)
		tiledmap_Get()->InvalidateWorldmap();
	// Drive it through Refresh, the only context where the tile composite works.
	tiledmap_Get()->RetargetTileSurface(nullptr);
	tiledmap_Get()->Refresh();
	int const redrawn = tiledmap_Get()->LastWorldmapRedrawCount();
	json result;
	result["cells_redrawn"] = redrawn;
	// Atlas occupancy. Evictions during a build are the interesting number: the
	// batch emits quads that reference atlas slots and draws them afterwards,
	// so a slot recycled mid-build makes an already-emitted quad sample pixels
	// that belong to a different cell.
	// P14: how many cells the GPU raster path composited this build (0 when
	// CTP2_GPU_RASTER is off or every cell carried overlays).
	result["raster_cells"] = tiledmap_Get()->LastWorldmapRasterCount();
	result["atlas_slots_used"] = tiledmap_Get()->GpuTileCacheSize();
	result["atlas_capacity"]   = tiledmap_Get()->GpuTileCacheCapacity();
	result["atlas_evictions"]  = (int64_t) tiledmap_Get()->GpuTileCacheEvictions();
	result["texture_w"] = aui_SDL::WorldmapW();
	result["texture_h"] = aui_SDL::WorldmapH();
	result["has_texture"] = aui_SDL::WorldmapTexture() != nullptr;
	// Sampled in the same call, with the target still bound: a count read from a
	// later command cannot tell a drawing bug from a discarded render target.
	result["dx_range"] = { tiledmap_Get()->m_worldmapMinX, tiledmap_Get()->m_worldmapMaxX };
	result["dy_range"] = { tiledmap_Get()->m_worldmapMinY, tiledmap_Get()->m_worldmapMaxY };
	result["tile_wh"] = { tiledmap_Get()->m_worldmapTileW, tiledmap_Get()->m_worldmapTileH };
	result["zoom_level"] = (int)tiledmap_Get()->GetZoomLevel();
	result["zoom_largest"] = (int)k_ZOOM_LARGEST;
	result["zoom_tile_wh"] = { (int)tiledmap_Get()->GetZoomTilePixelWidth(),
	                           (int)tiledmap_Get()->GetZoomTilePixelHeight() };
	result["atlas_misses"] = tiledmap_Get()->m_worldmapMisses;
	result["atlas_uploads"] = tiledmap_Get()->m_worldmapUploads;
	result["coverage_hits"] = aui_SDL::SampleWorldmapCoverage(30);
	result["coverage_samples"] = 900;
	return Ok("debug_worldmap_build", result);
}

// Read one ARGB pixel back out of the whole-map target, in absolute map-pixel
// coordinates. This is the oracle for "did terrain actually land where the map
// says it should", independent of any camera or present.
std::string CmdDebugWorldmapPixel(const char * args)
{
	int x = 0, y = 0;
	if (!args || sscanf(args, "%d %d", &x, &y) != 2)
		return Err("debug_worldmap_pixel", "bad_args");
	SDL_Renderer * renderer = aui_SDL::Renderer();
	SDL_Texture *  target   = aui_SDL::WorldmapTexture();
	if (!renderer || !target)
		return Err("debug_worldmap_pixel", "no_worldmap_texture");
	if (x < 0 || y < 0 || x >= aui_SDL::WorldmapW() || y >= aui_SDL::WorldmapH())
		return Err("debug_worldmap_pixel", "out_of_bounds");

	SDL_Texture * const prev = SDL_GetRenderTarget(renderer);
	uint32 pixel = 0;
	bool ok = false;
	if (CTP2_SDL_SetRenderTarget(renderer, target))
		ok = CTP2_SDL_RenderReadPixelARGB(renderer, x, y, &pixel);
	CTP2_SDL_SetRenderTarget(renderer, prev);
	if (!ok)
		return Err("debug_worldmap_pixel", "readback_failed");

	json result;
	result["x"] = x;
	result["y"] = y;
	result["argb"] = pixel;
	result["rgb"] = pixel & 0x00FFFFFFu;
	return Ok("debug_worldmap_pixel", result);
}

// P13 step 1 probe. ADR-003 puts the WHOLE map in one GPU render target instead
// of the screen+margin window mirror, so the first question to settle is whether
// a texture that size can be created, rendered into, and read back at all — the
// Gigantic map is 6,580 x 5,040px (~133MB at 32bpp) against a 16,384^2 Metal
// limit. Reports the measured answer rather than the arithmetic.
std::string CmdDebugGpuWorldmapProbe(const char * args)
{
	World * w = world_Get();
	if (!w)
		return Err("debug_gpu_worldmap_probe", "no_world");
	SDL_Renderer * renderer = aui_SDL::Renderer();
	if (!renderer)
		return Err("debug_gpu_worldmap_probe", "no_renderer");

	// Whole-map extent at native zoom: one tile grid per column, half a grid per
	// row (isometric rows interleave), matching the tileset constants the world
	// mirror is pinned to.
	int const mapW = static_cast<int>(w->GetXWidth());
	int const mapH = static_cast<int>(w->GetYHeight());
	// Optional "<tilesX> <tilesY>" override, so the worst case (Gigantic, 70x140)
	// can be probed without generating a Gigantic game.
	int overrideW = 0, overrideH = 0;
	bool const overridden = args && sscanf(args, "%d %d", &overrideW, &overrideH) == 2
	                        && overrideW > 0 && overrideH > 0;
	int const tilesX = overridden ? overrideW : mapW;
	int const tilesY = overridden ? overrideH : mapH;
	int const texW = tilesX * k_TILE_GRID_WIDTH;
	int const texH = tilesY * (k_TILE_GRID_HEIGHT / 2);

	json result;
	result["map_tiles_x"] = tilesX;
	result["map_tiles_y"] = tilesY;
	result["overridden"] = overridden;
	result["texture_w"] = texW;
	result["texture_h"] = texH;
	result["bytes"] = static_cast<double>(texW) * texH * 4.0;

	SDL_Texture * target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_TARGET, texW, texH);
	if (!target)
	{
		result["created"] = false;
		result["sdl_error"] = SDL_GetError();
		return Ok("debug_gpu_worldmap_probe", result);
	}
	result["created"] = true;

	// Clear to a known colour and read it back from the FAR corner: allocation
	// alone proves nothing if the driver cannot actually target a texture this
	// large, and the far corner is what a silently-clamped size would miss.
	bool readback_ok = false;
	uint32 pixel = 0;
	if (CTP2_SDL_SetRenderTarget(renderer, target))
	{
		SDL_SetRenderDrawColor(renderer, 0x12, 0x34, 0x56, 255);
		SDL_RenderClear(renderer);
		readback_ok = CTP2_SDL_RenderReadPixelARGB(renderer, texW - 1, texH - 1, &pixel);
		CTP2_SDL_SetRenderTarget(renderer, nullptr);
	}
	result["readback"] = readback_ok;
	if (readback_ok)
	{
		result["pixel"] = pixel & 0x00FFFFFFu;
		result["pixel_matches"] = ((pixel & 0x00FFFFFFu) == 0x00123456u);
	}
	else
	{
		result["sdl_error"] = SDL_GetError();
	}
	SDL_DestroyTexture(target);
	return Ok("debug_gpu_worldmap_probe", result);
}

// GPU-rasterisation feasibility probe. DrawTransitionTile's inner loop reads a
// raw Pixel16 stream over the tile diamond where values 0..3 are inline
// MARKERS: each consumes the next pixel from transition strip 0..3. Whether
// that composite can move to the GPU as "one base quad + up to four strip
// quads" hinges on the marker LAYOUT: if the (marker, position) map is shared
// across base tiles, a transition strip can be pre-splatted into diamond
// positions once per (from, to, which) — an additive space. If every base tile
// has its own layout, the splat multiplies by base-tile count. This measures
// which world we live in, instead of guessing.
std::string CmdDebugTilesetStats(const char * /*args*/)
{
	TiledMap * map = tiledmap_Get();
	TileSet * ts = map ? map->GetTileSet() : nullptr;
	if (!ts)
		return Err("debug_tileset_stats", "no_tileset");

	auto startPixel = [](int y) {
		return (y < k_TILE_PIXEL_HEADROOM)
		       ? 2 * ((k_TILE_PIXEL_HEADROOM - 1) - y)
		       : 2 * (y - k_TILE_PIXEL_HEADROOM);
	};

	int tiles = 0;
	int tilesWithMarkers = 0;
	std::map<uint64_t, int> layoutCounts;   // layout hash -> #tiles
	int minCount[4] = {1 << 30, 1 << 30, 1 << 30, 1 << 30};
	int maxCount[4] = {0, 0, 0, 0};

	for (uint16 i = 0; i < k_MAX_BASE_TILES; ++i)
	{
		BaseTile * bt = ts->GetBaseTile(i);
		if (!bt) continue;
		Pixel16 * data = bt->GetTileData();
		if (!data) continue;
		++tiles;

		// Walk the diamond exactly as DrawTransitionTile does and hash the
		// sequence of (marker, y, x) positions. FNV-1a over the triples: two
		// tiles share a hash iff (collisions aside) they share a layout.
		uint64_t h = 1469598103934665603ULL;
		int counts[4] = {0, 0, 0, 0};
		Pixel16 const * p = data;
		for (int y = 0; y < k_TILE_PIXEL_HEIGHT; ++y)
		{
			int const sx = startPixel(y);
			int const ex = k_TILE_PIXEL_WIDTH - sx;
			for (int x = sx; x < ex; ++x)
			{
				Pixel16 const v = *p++;
				if (v < 4)
				{
					++counts[v];
					uint64_t const trip = ((uint64_t) v << 32)
					                    | ((uint64_t) (uint16) y << 16)
					                    | (uint64_t) (uint16) x;
					h ^= trip;
					h *= 1099511628211ULL;
				}
			}
		}
		if (counts[0] + counts[1] + counts[2] + counts[3] > 0)
		{
			++tilesWithMarkers;
			++layoutCounts[h];
			for (int k = 0; k < 4; ++k)
			{
				if (counts[k] < minCount[k]) minCount[k] = counts[k];
				if (counts[k] > maxCount[k]) maxCount[k] = counts[k];
			}
		}
	}

	json result;
	result["base_tiles"] = tiles;
	result["tiles_with_markers"] = tilesWithMarkers;
	result["distinct_layouts"] = (int) layoutCounts.size();
	json counts = json::array();
	for (int k = 0; k < 4; ++k)
		counts.push_back({ {"min", tilesWithMarkers ? minCount[k] : 0},
		                   {"max", maxCount[k]} });
	result["marker_counts"] = counts;
	// How many tiles share the most common layout — if this equals
	// tiles_with_markers, the layout is universal.
	int biggest = 0;
	for (auto const & kv : layoutCounts)
		if (kv.second > biggest) biggest = kv.second;
	result["largest_layout_tiles"] = biggest;
	return Ok("debug_tileset_stats", result);
}
#endif


std::string CmdDebugDeselect()
{
	if (!selitem_Get())
		return Err("debug_deselect", "no_selitem");
	selitem_Get()->Deselect(selitem_Get()->GetVisiblePlayer());
	if (tiledmap_Get())
		tiledmap_Get()->BuildTerrainQuads();
	return Ok("debug_deselect");
}

std::string CmdDebugCombatFlash(const char * args)
{
    sint32 x = 0, y = 0;
    if (sscanf(args, "%d %d", &x, &y) != 2)
        return Err("debug_combat_flash", "bad_args");
    if (!director_Get())
        return Err("debug_combat_flash", "no_director");

    MapPoint pos(x, y);
    director_Get()->AddCombatFlash(pos);
    director_Get()->HandleNextAction();

    json result;
    result["pos"] = { {"x", x}, {"y", y} };
    return Ok("debug_combat_flash", result);
}

std::string CmdDebugScenarioStartFlags(const char * args)
{
    int on = 0;
    if (sscanf(args, "%d", &on) != 1)
        return Err("debug_scenario_start_flags", "bad_args");

    ScenarioEditor::DebugSetStartFlags(on ? SCEN_START_LOC_MODE_PLAYER : SCEN_START_LOC_MODE_NONE);
    if (tiledmap_Get())
        tiledmap_Get()->BuildTerrainQuads();

    json result;
    result["show_start_flags"] = on != 0;
    return Ok("debug_scenario_start_flags", result);
}

std::string CmdDebugCloakArmy(const char * args)
{
    int idx = -1;
    if (sscanf(args, "%d", &idx) != 1)
        return Err("debug_cloak_army", "bad_args");
    Player * human = HumanPlayer();
    if (!human || !human->GetAllArmiesList())
        return Err("debug_cloak_army", "no_human_player");
    if (idx < 0 || idx >= human->GetAllArmiesList()->Num())
        return Err("debug_cloak_army", "bad_army_index");

    Army army = human->GetAllArmiesList()->Access(idx);
    if (!army.IsValid() || !army.AccessData() || army.Num() < 1)
        return Err("debug_cloak_army", "invalid_army");
    Unit unit = army.AccessData()->Access(0);
    if (!unit.IsValid())
        return Err("debug_cloak_army", "invalid_unit");
    unit.Cloak();
    return Ok("debug_cloak_army");
}

std::string CmdDebugCityDefense(const char * args)
{
    struct SavedImprovements { sint32 cityId; uint64 improvements; };
    static std::vector<SavedImprovements> s_saved;

    int cityIdx = -1;
    char kind[32] = {0};
    if (sscanf(args, "%d %31s", &cityIdx, kind) != 2)
        return Err("debug_city_defense", "bad_args");
    Player * human = HumanPlayer();
    if (!human || !human->GetAllCitiesList())
        return Err("debug_city_defense", "no_human_player");
    if (cityIdx < 0 || cityIdx >= human->GetAllCitiesList()->Num())
        return Err("debug_city_defense", "bad_city_index");

    Unit city = human->GetAllCitiesList()->Access(cityIdx);
    CityData * cd = city.IsValid() && city.GetData() ? city.GetData()->GetCityData() : nullptr;
    if (!cd)
        return Err("debug_city_defense", "invalid_city");

    if (strcmp(kind, "clear") == 0) {
        for (size_t i = 0; i < s_saved.size(); ++i) {
            if (s_saved[i].cityId != city.m_id) continue;
            cd->SetImprovements(s_saved[i].improvements);
            s_saved.erase(s_saved.begin() + i);
            if (tiledmap_Get())
                tiledmap_Get()->BuildTerrainQuads();
            return Ok("debug_city_defense");
        }
        if (tiledmap_Get())
            tiledmap_Get()->BuildTerrainQuads();
        return Ok("debug_city_defense");
    }

    sint32 building = -1;
    for (sint32 i = 0; g_theBuildingDB && i < g_theBuildingDB->NumRecords(); ++i) {
        const BuildingRecord *rec = g_theBuildingDB->Get(i);
        if (!rec) continue;
        if ((strcmp(kind, "walls") == 0 && rec->GetCityWalls())
            || (strcmp(kind, "forcefield") == 0 && rec->GetForceField())) {
            building = i;
            break;
        }
    }
    if (building < 0)
        return Err("debug_city_defense", "no_matching_building");

    bool saved = false;
    for (SavedImprovements const &entry : s_saved)
        saved = saved || entry.cityId == city.m_id;
    if (!saved)
        s_saved.push_back({city.m_id, cd->GetImprovements()});

    cd->SetImprovements(cd->GetImprovements() | safe_shift_left_u64(building));
    if (tiledmap_Get())
        tiledmap_Get()->BuildTerrainQuads();

    json result;
    result["city"] = cityIdx;
    result["building"] = building;
    result["kind"] = kind;
    return Ok("debug_city_defense", result);
}

std::string CmdDebugGalleryCase(const char * args)
{
    char kind[64] = {0};
    char arg[128] = {0};
    sint32 x = 0, y = 0;
    int parsed = sscanf(args, "%63s %d %d %127s", kind, &x, &y, arg);
    if (parsed < 3)
        return Err("debug_gallery_case", "bad_args");
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("debug_gallery_case", "game_not_loaded");
    Player * human = HumanPlayer();
    World * w = world_Get();
    if (!human || !w)
        return Err("debug_gallery_case", "no_world");
    if (x < 0 || y < 0 || x >= w->GetXWidth() || y >= w->GetYHeight())
        return Err("debug_gallery_case", "bad_position");

    MapPoint pos(x, y);
    CmdDebugClearTerrainLayers((std::to_string(x) + " " + std::to_string(y) + " 60").c_str());

    if (strcmp(kind, "underwater_city") == 0) {
        w->SmartSetTerrain(pos, TERRAIN_WATER_SHELF, 0);
        w->SetMovementType(x, y, k_MOVEMENT_TYPE_WATER | k_MOVEMENT_TYPE_SHALLOW_WATER);
        if (tiledmap_Get())
            tiledmap_Get()->BuildTerrainQuads();
        Unit city = human->CreateCity(unitutil_GetSeaCity(), pos, CAUSE_NEW_CITY_INITIAL, nullptr, CITY_STYLE_EDITOR);
        if (!city.IsValid())
            return Err("debug_gallery_case", "create_city_failed");
    } else if (strcmp(kind, "city") == 0 || strcmp(kind, "city_walls") == 0 || strcmp(kind, "city_forcefield") == 0) {
        Unit city = human->CreateCity(unitutil_GetCityTypeFor(pos), pos, CAUSE_NEW_CITY_INITIAL, nullptr, CITY_STYLE_EDITOR);
        if (!city.IsValid())
            return Err("debug_gallery_case", "create_city_failed");
        CityData * cd = city.GetData() ? city.GetData()->GetCityData() : nullptr;
        if (cd && strcmp(kind, "city") != 0) {
            char defenseArgs[64];
            sint32 cityIdx = human->GetAllCitiesList()->Num() - 1;
            snprintf(defenseArgs, sizeof(defenseArgs), "%d %s", (int)cityIdx,
                     strcmp(kind, "city_walls") == 0 ? "walls" : "forcefield");
            CmdDebugCityDefense(defenseArgs);
        }
    } else if (strcmp(kind, "unit") == 0) {
        sint32 const type = ResolveUnitType(parsed >= 4 ? arg : "UNIT_MARINE");
        if (type < 0)
            return Err("debug_gallery_case", "bad_unit_type");
        Unit u = human->CreateUnit(type, pos, Unit(), false, CAUSE_NEW_ARMY_INITIAL);
        if (!u.IsValid())
            return Err("debug_gallery_case", "create_unit_failed");
    } else if (strcmp(kind, "combat_flash") == 0) {
        director_Get()->AddCombatFlash(pos);
        director_Get()->HandleNextAction();
    } else if (strcmp(kind, "terrain_overlay") == 0) {
        char overlayArgs[64];
        snprintf(overlayArgs, sizeof(overlayArgs), "%d %d", (int)x, (int)y);
        return CmdDebugTerrainOverlay(overlayArgs);
    } else {
        return Err("debug_gallery_case", "unknown_kind");
    }

    if (tiledmap_Get())
        tiledmap_Get()->BuildTerrainQuads();
    json result;
    result["kind"] = kind;
    result["pos"] = { {"x", x}, {"y", y} };
    if (parsed >= 4)
        result["arg"] = arg;
    return Ok("debug_gallery_case", result);
}

std::string CmdSetZoomLevel(const char * args)
{
    int level = 0;
    if (sscanf(args, "%d", &level) != 1 || level < k_ZOOM_SMALLEST || level > k_ZOOM_LARGEST)
        return Err("set_zoom_level", "bad_args");
    if (!tiledmap_Get())
        return Err("set_zoom_level", "no_tiledmap");

    while (tiledmap_Get()->GetZoomLevel() < level && tiledmap_Get()->ZoomIn()) {}
    while (tiledmap_Get()->GetZoomLevel() > level && tiledmap_Get()->ZoomOut()) {}
    tiledmap_Get()->BuildTerrainQuads();

    json result;
    result["zoom_level"] = tiledmap_Get()->GetZoomLevel();
    return Ok("set_zoom_level", result);
}

// set_production <city_idx> <what>
// what: a numeric unit type, "cheapest_military", "settler",
//       "building <building_id>" (city improvements: granaries etc. —
//       the growth lever units can't provide), or "clear" (empty the build
//       queue: stop producing entirely; the 173-round rematch showed
//       perpetual unit spam actively drains score).
std::string CmdSetProduction(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("set_production", "game_not_loaded");

    int  city_idx = 0;
    char keyword[64];
    if (sscanf(args, "%d %63s", &city_idx, keyword) != 2)
        return Err("set_production", "bad_args");

    Player * human = HumanPlayer();
    if (!human)
        return Err("set_production", "no_human_player");
    if (city_idx < 0 || city_idx >= human->GetAllCitiesList()->Num())
        return Err("set_production", "bad_city_index");

    Unit city = human->GetAllCitiesList()->Access(city_idx);
    if (!city.IsValid() || !city.GetData()->GetCityData())
        return Err("set_production", "invalid_city");

    CityData * cd       = city.GetData()->GetCityData();
    sint32     gov_type = human->GetGovernmentType();
    sint32     unit_type = -1;

    if (strcmp(keyword, "clear") == 0) {
        if (cd->GetBuildQueue())
            cd->GetBuildQueue()->Clear();
        json result;
        result["city"] = city_idx;
        result["building"] = nullptr;
        return Ok("set_production", result);
    }

    if (strcmp(keyword, "building") == 0) {
        int b = -1;
        if (sscanf(args, "%*d %*s %d", &b) != 1)
            return Err("set_production", "bad_args");
        if (!g_theBuildingDB || b < 0 || b >= g_theBuildingDB->NumRecords())
            return Err("set_production", "bad_building");
        if (!cd->CanBuildBuilding(b))
            return Err("set_production", "cannot_build_building");
        // BuildImprovement APPENDS — without a clear the new order queues
        // behind whatever is already there (campaign 7: a granary stuck
        // behind a 740-shield settler for 20 rounds). set_production
        // means "build this next": replace, don't append.
        if (cd->GetBuildQueue())
            cd->GetBuildQueue()->Clear();
        cd->BuildImprovement(b);
        const BuildingRecord * rec = g_theBuildingDB->Get(b, gov_type);
        json result;
        result["city"]     = city_idx;
        result["category"] = k_GAME_OBJ_TYPE_IMPROVEMENT;
        result["type"]     = b;
        result["name"]     = rec ? ToUtf8(rec->GetNameText()) : "";
        gc_log->info("set_production: city {} -> building {}", city_idx, b);
        return Ok("set_production", result);
    }

    if (strcmp(keyword, "wonder") == 0) {
        // Wonders are a SEPARATE database (g_theWonderDB) from ordinary
        // improvements — set_production "building <id>" cannot reach them.
        // They are the biggest single score lever (and carry empire effects
        // like Great Library's free advances), so they get their own keyword.
        int w = -1;
        if (sscanf(args, "%*d %*s %d", &w) != 1)
            return Err("set_production", "bad_args");
        if (!g_theWonderDB || w < 0 || w >= g_theWonderDB->NumRecords())
            return Err("set_production", "bad_wonder");
        if (!cd->CanBuildWonder(w))
            return Err("set_production", "cannot_build_wonder");
        if (cd->GetBuildQueue())
            cd->GetBuildQueue()->Clear();
        cd->BuildWonder(w);
        const WonderRecord * rec = g_theWonderDB->Get(w);
        json result;
        result["city"]     = city_idx;
        result["category"] = k_GAME_OBJ_TYPE_WONDER;
        result["type"]     = w;
        result["name"]     = rec ? ToUtf8(rec->GetNameText()) : "";
        gc_log->info("set_production: city {} -> wonder {}", city_idx, w);
        return Ok("set_production", result);
    }

    // Cheapest buildable unit satisfying the keyword's predicate, or -1.
    auto cheapest_buildable = [&](auto && pred) {
        sint32 best = 0x7fffffff, found = -1;
        for (sint32 i = 0; i < g_theUnitDB->NumRecords(); ++i) {
            const UnitRecord * rec = g_theUnitDB->Get(i, gov_type);
            if (!rec || rec->GetCantBuild() || !pred(rec)) continue;
            if (!cd->CanBuildUnit(i)) continue;
            sint32 cost = rec->GetShieldCost();
            if (cost > 0 && cost < best) { best = cost; found = i; }
        }
        return found;
    };

    if (strcmp(keyword, "cheapest_military") == 0) {
        unit_type = cheapest_buildable(
            [](const UnitRecord * r) { return r->GetAttack() > 0.0; });
    } else if (strcmp(keyword, "settler") == 0) {
        unit_type = cheapest_buildable(
            [](const UnitRecord * r) { return r->GetSettle() || r->GetNumCanSettleOn() > 0; });
    } else {
        unit_type = atoi(keyword);
    }

    if (unit_type < 0 || unit_type >= g_theUnitDB->NumRecords())
        return Err("set_production", "unit_not_found");
    if (!cd->CanBuildUnit(unit_type))
        return Err("set_production", "cannot_build_unit");

    gc_log->info("set_production: city {} -> unit {}", city_idx, (int)unit_type);
    // Same replace-not-append contract as the building branch.
    if (cd->GetBuildQueue())
        cd->GetBuildQueue()->Clear();
    cd->BuildUnit(unit_type);

    json result;
    result["city"]     = city_idx;
    result["category"] = k_GAME_OBJ_TYPE_UNIT;
    result["type"]     = unit_type;
    return Ok("set_production", result);
}

// save_game <path>  — binary or JSON depending on extension (GameFile decides).
std::string CmdSaveGame(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("save_game", "game_not_loaded");
    if (!args[0])
        return Err("save_game", "bad_args");
    gc_log->info("save_game: {}", args);
    GameFile::SaveGame(args, nullptr);
    return Ok("save_game");
}

// load_game <path>  — actor recreation happens inside LoadJson, shared with
// the UI and headless batch load paths.
std::string CmdLoadGame(const char * args)
{
    if (!args[0])
        return Err("load_game", "bad_args");
    gc_log->info("load_game: {}", args);
    if (!GameFile::RestoreGame(args))
        return Err("load_game", "load_failed");
    return Ok("load_game");
}

// log_get — the Action Log: every meaningful action/event the engine fired
// (orders, world-changing outcomes, real diplomacy) regardless of source —
// human UI, gateway command, AI turn, slic.  Each entry is {turn, player,
// event, args}.  The ledger rides in the JSON save, so it spans save/load.
// Read-only; the engine owns population (no log_append).
std::string CmdLogGet()
{
    json result;
    result["action_log"] = action_log::Get();
    result["count"]      = (sint32) action_log::Count();
    return Ok("log_get", result);
}

// log_clear — reset the ledger.  The engine repopulates as events fire; this
// is a testing/debugging lever (e.g. isolate one turn's actions).
std::string CmdLogClear()
{
    action_log::Clear();
    return Ok("log_clear");
}

// query_armies — the human's armies with what a player needs to command
// them: index (for move_army/auto_explore), position, movement points left
// this turn, settle capability, and the member units.
std::string QueryArmies()
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_armies", "game_not_loaded");

    Player * human = HumanPlayer();
    if (!human)
        return Err("query_armies", "no_human_player");

    json list = json::array();
    DynamicArray<Army> * armies = human->GetAllArmiesList();
    for (sint32 i = 0; armies && i < armies->Num(); ++i) {
        Army army = armies->Access(i);
        ArmyData * ad = army.AccessData();
        if (!army.IsValid() || !ad) continue;

        MapPoint pos = ad->RetPos();
        double moves = 0.0;
        ad->CurMinMovementPoints(moves);

        json units = json::array();
        json cargo = json::array();
        sint32 capacity = 0;
        for (sint32 u = 0; u < ad->Num(); ++u) {
            Unit unit = ad->Access(u);
            if (!unit.IsValid()) continue;
            json j;
            j["type"] = unit.GetType();
            j["name"] = ToUtf8(unit.GetName());
            j["hp"]   = unit.GetHP();
            units.push_back(j);
            if (UnitData * ud = unit.AccessData()) {
                capacity += ud->GetMaxCargoCapacity();
                if (UnitDynamicArray * cl = ud->GetCargoList()) {
                    for (sint32 ci = 0; ci < cl->Num(); ++ci) {
                        Unit cu = cl->Access(ci);
                        if (cu.IsValid()) cargo.push_back(ToUtf8(cu.GetName()));
                    }
                }
            }
        }

        json a;
        a["index"]      = i;
        a["pos"]        = { {"x", pos.x}, {"y", pos.y} };
        a["moves_left"] = moves;
        a["can_settle"] = army.CanSettle();
        a["units"]      = units;
        a["cargo"]          = cargo;     // units riding in this army's transports
        a["cargo_capacity"] = capacity;  // total transport slots
        list.push_back(a);
    }

    json result;
    result["armies"] = list;
    return Ok("query_armies", result);
}

// move_army <army_idx> <x> <y> — pathfind and queue a move order, then pump
// events so movement starts immediately. The army walks as far as this
// turn's movement points allow; the rest of the path continues on later
// turns. The result reports where the army actually stands afterwards.
std::string CmdMoveArmy(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("move_army", "game_not_loaded");

    int idx = -1, x = -1, y = -1;
    if (sscanf(args, "%d %d %d", &idx, &x, &y) != 3)
        return Err("move_army", "bad_args");

    Player * human = HumanPlayer();
    if (!human)
        return Err("move_army", "no_human_player");

    DynamicArray<Army> * armies = human->GetAllArmiesList();
    if (!armies || idx < 0 || idx >= armies->Num())
        return Err("move_army", "bad_army_index");

    World * w = world_Get();
    if (!w || x < 0 || x >= w->GetXWidth() || y < 0 || y >= w->GetYHeight())
        return Err("move_army", "bad_destination");

    Army army = armies->Access(idx);
    ArmyData * ad = army.AccessData();
    if (!army.IsValid() || !ad)
        return Err("move_army", "invalid_army");

    MapPoint src = ad->RetPos();
    MapPoint dest((sint16)x, (sint16)y);
    if (!army_QueueMovePath(human->GetOwner(), army, src, dest)) {
        // Pathfinding refuses unexplored destinations — but stepping into
        // ADJACENT fog is a basic player ability (how anyone marches into
        // the unknown). Mirror the explore fallback: a point MOVE_TO order
        // needs no path. Validate enterability first — a land army ordered
        // into ocean fog would otherwise "succeed" and silently never move.
        if (!src.IsNextTo(dest))
            return Err("move_army", "no_path");
        if (!w->CanEnter(dest, ad->GetMovementType())) {
            // One legal exception: BOARDING. A land army may step onto a
            // water tile that holds an own transport with enough free
            // cargo space — MoveIntoCell handles the actual embarkation.
            Cell * dcell = w->GetCell(dest);
            sint32 capacity = 0;
            for (sint32 u = 0; dcell && u < dcell->GetNumUnits(); ++u) {
                Unit t = dcell->AccessUnit(u);
                if (t.IsValid() && t.GetOwner() == human->GetOwner())
                    capacity += t.GetData() ? t.GetData()->GetMaxCargoCapacity() : 0;
            }
            if (capacity < ad->Num())
                return Err("move_army", "impassable");
        }
        army.ClearOrders();
        army.AddOrders(UNIT_ORDER_MOVE_TO, dest);
    }

    // A manual order overrides auto-explore — otherwise the explore tick
    // would re-route the army somewhere else next turn.
    for (sint32 u = 0; u < ad->Num(); ++u) {
        Unit unit = ad->Access(u);
        if (UnitData * ud = unit.AccessData()) ud->SetExploring(false);
    }

    // Drain the queued GEV_MoveOrder so the army starts walking now.
    if (gevmanager_Get())
        gevmanager_Get()->Process();

    MapPoint now = ad->RetPos();
    gc_log->info("move_army: army {} ({},{}) -> ({},{}), now at ({},{})",
                 idx, (int)src.x, (int)src.y, x, y, (int)now.x, (int)now.y);

    json result;
    result["army"] = idx;
    result["from"] = { {"x", src.x}, {"y", src.y} };
    result["dest"] = { {"x", x}, {"y", y} };
    result["pos"]  = { {"x", now.x}, {"y", now.y} };
    result["arrived"] = (now.x == x && now.y == y);
    return Ok("move_army", result);
}

// auto_explore <army_idx> — hand the army to the explore order; the per-turn
// hook keeps re-picking new targets, revealing the map without driving every
// step by hand.
std::string CmdAutoExplore(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("auto_explore", "game_not_loaded");

    int idx = -1;
    if (sscanf(args, "%d", &idx) != 1)
        return Err("auto_explore", "bad_args");

    Player * human = HumanPlayer();
    if (!human)
        return Err("auto_explore", "no_human_player");

    DynamicArray<Army> * armies = human->GetAllArmiesList();
    if (!armies || idx < 0 || idx >= armies->Num())
        return Err("auto_explore", "bad_army_index");

    Army army = armies->Access(idx);
    if (!army.IsValid())
        return Err("auto_explore", "invalid_army");

    if (!gevmanager_Get())
        return Err("auto_explore", "no_event_manager");
    gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_ExploreOrder,
                               GEA_Army, army, GEA_End);
    gevmanager_Get()->Process();

    gc_log->info("auto_explore: army {}", idx);
    return Ok("auto_explore");
}

// ---- queries ------------------------------------------------------------

// Coarse class label for a tile improvement — shared by query_terraform (the
// buildable list) and query_map (what is actually on the ground) so both speak
// the same vocabulary: farm/mine/road/structure/wonder/terraform/other.
const char * TerrainImpClass(const TerrainImprovementRecord * r)
{
    if (!r) return "other";
    if (r->GetClassTerraform() || r->GetClassOceanform()) return "terraform";
    if (r->GetClassFarm()      || r->GetClassOceanFarm())  return "farm";
    if (r->GetClassMine()      || r->GetClassOceanMine())  return "mine";
    if (r->GetClassRoad()      || r->GetClassOceanRoad())  return "road";
    if (r->GetClassStructure1()|| r->GetClassStructure2()) return "structure";
    if (r->GetClassWonder())                               return "wonder";
    return "other";
}

// Describe one city for the human's viewpoint.
json CityJson(sint32 owner, sint32 city_idx, const Unit & u)
{
    json c;
    c["owner"] = owner;
    c["index"] = city_idx;
    MapPoint pos;
    u.GetPos(pos);
    c["pos"] = { {"x", pos.x}, {"y", pos.y} };
    c["name"] = ToUtf8(u.GetName());

    CityData * cd = u.GetData() ? u.GetData()->GetCityData() : nullptr;
    if (cd) {
        c["population"] = cd->PopCount();
        // Net per-turn yields — what the city panel shows the player.
        json y;
        y["food"]       = cd->GetNetCityFood();
        y["production"] = cd->GetNetCityProduction();
        y["gold"]       = cd->GetNetCityGold();
        y["science"]    = cd->GetScience();
        y["happiness"]  = cd->GetHappiness();
        c["yields"] = y;
        // A rioting city produces nothing — the defining problem of a
        // freshly captured city (foreign pop + unhappiness). Surfacing it
        // is what lets a driver react (garrison, buy a happiness building).
        c["rioting"] = cd->GetIsRioting();
        // Build/growth progress — without these a driver cannot tell a
        // slow build from a deadlocked one (e.g. a settler in a pop-1
        // city is held forever by BuildFrontUnit's RemovesAPop guard).
        c["shields_stored"] = cd->GetStoredCityProduction();

        // Food breakdown — the *why* behind growth_rate.  net = gross
        // produced minus what the population eats; a stagnant city with a
        // healthy gross is being eaten by its own size (consumed) or by
        // unit support, not by poor terrain.  required is the food the
        // current population needs to not starve.
        sint32 const popCount   = cd->PopCount();
        sint32 const maxPop     = cd->GetMaxPop();
        c["food"] = { {"gross",            cd->GetGrossCityFood()},
                      {"net",              cd->GetNetCityFood()},
                      {"consumed",         cd->GetConsumedFood()},
                      {"required",         cd->GetFoodRequired()},
                      {"max_from_terrain", cd->GetMaxFoodFromTerrain()} };

        // partial_population accumulates growth_rate per turn; the city
        // gains a pop at k_PEOPLE_PER_POPULATION (10000). growth_rate <= 0
        // means the city will never grow.  But a *positive* food surplus
        // can still yield growth_rate 0 when population has hit max_pop —
        // the size cap raised by Aqueduct/Sewer-class buildings.  at_pop_cap
        // disambiguates "starving" (net food <= 0) from "capped" (need a
        // bigger-city building), the two failure modes a driver confuses.
        c["growth"] = { {"food_stored",        cd->GetStoredCityFood()},
                        {"partial_population", cd->GetPartialPopulation()},
                        {"pop_threshold",      k_PEOPLE_PER_POPULATION},
                        {"growth_rate",        cd->GetGrowthRate()},
                        {"max_pop",            maxPop},
                        {"size_index",         cd->GetSizeIndex()},
                        {"at_pop_cap",         popCount >= maxPop},
                        {"starvation_turns",   cd->GetStarvationTurns()} };

        // Gold upkeep this city pays each turn: wages to its citizens plus
        // building maintenance (CalcWages + GetSupportBuildingsCost).  NOTE:
        // this is NOT military unit upkeep — that is a player-level gold cost,
        // not attributed per-city.  Grows with city size and building count.
        c["gold_upkeep"] = cd->GetSupport();

        // Buildings already built, by name — lets a consumer see at a glance
        // which growth/economy/happiness levers a city already has (and which
        // it is missing) instead of inferring it from yields alone.
        json built = json::array();
        for (sint32 b = 0; g_theBuildingDB && b < g_theBuildingDB->NumRecords(); ++b) {
            if (!cd->HasBuilding(b)) continue;
            const BuildingRecord * brec = g_theBuildingDB->Get(b);
            built.push_back({ {"type", b},
                              {"name", brec ? ToUtf8(brec->GetNameText()) : ""} });
        }
        c["buildings_built"] = built;
        BuildQueue * queue = cd->GetBuildQueue();
        BuildNode * head = queue ? queue->GetHead() : nullptr;
        if (head) {
            // Derived, not read from m_settler_pending: that flag is reset
            // in BuildQueue::EndTurn, so it is never visible to a query
            // that runs between rounds. Mirrors BuildFrontUnit's guards.
            json blocked = nullptr;
            if (head->m_category == k_GAME_OBJ_TYPE_UNIT &&
                head->m_cost <= cd->GetStoredCityProduction()) {
                const UnitRecord * rec = g_theUnitDB->Get(head->m_type);
                sint32 unitpop = 0;
                if (rec && rec->GetBuildingRemovesAPop() && cd->PopCount() < 2)
                    blocked = "settler_needs_pop_2";
                else if (rec && rec->GetPopCostsToBuild(unitpop) &&
                         cd->PopCount() <= unitpop)
                    blocked = "unit_pop_cost_exceeds_city_pop";
            }
            c["building"] = { {"category", head->m_category},
                              {"type",     head->m_type},
                              {"cost",     head->m_cost},
                              {"blocked",  blocked} };
        } else {
            c["building"] = nullptr;
        }
    }
    return c;
}

// query_cities — every city visible to the human (fog-of-war filtered).
std::string QueryCities()
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_cities", "game_not_loaded");

    Player * human = HumanPlayer();
    if (!human)
        return Err("query_cities", "no_human_player");

    json cities = json::array();
    for (sint32 p = 0; p < k_MAX_PLAYERS; ++p) {
        if (!player_Get(p)) continue;
        UnitDynamicArray * list = player_Get(p)->GetAllCitiesList();
        if (!list) continue;
        for (sint32 i = 0; i < list->Num(); ++i) {
            Unit u = list->Access(i);
            if (!u.IsValid()) continue;
            MapPoint pos;
            u.GetPos(pos);
            // Only what the human can actually see.  A player always sees its
            // own cities; enemy cities only when their tile is visible.
            bool visible = (p == human->GetOwner()) ||
                           (human->m_vision && human->m_vision->IsVisible(pos));
            if (!visible) continue;
            cities.push_back(CityJson(p, i, u));
        }
    }

    json result;
    result["visible_player"] = human->GetOwner();
    result["cities"]         = cities;
    return Ok("query_cities", result);
}

// query_city <city_idx>  — detail for one of the human's cities, including the
// buildable-unit affordance list ("what can I do here").
std::string QueryCity(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_city", "game_not_loaded");

    int city_idx = 0;
    if (sscanf(args, "%d", &city_idx) != 1)
        return Err("query_city", "bad_args");

    Player * human = HumanPlayer();
    if (!human)
        return Err("query_city", "no_human_player");
    if (city_idx < 0 || city_idx >= human->GetAllCitiesList()->Num())
        return Err("query_city", "bad_city_index");

    Unit u = human->GetAllCitiesList()->Access(city_idx);
    if (!u.IsValid() || !u.GetData() || !u.GetData()->GetCityData())
        return Err("query_city", "invalid_city");

    json result   = CityJson(human->GetOwner(), city_idx, u);
    CityData * cd = u.GetData()->GetCityData();
    sint32 gov    = human->GetGovernmentType();

    // Buildable units: the affordance set a player would see in the build menu.
    json buildable = json::array();
    for (sint32 i = 0; i < g_theUnitDB->NumRecords(); ++i) {
        if (!cd->CanBuildUnit(i)) continue;
        const UnitRecord * rec = g_theUnitDB->Get(i, gov);
        json item;
        item["category"] = k_GAME_OBJ_TYPE_UNIT;
        item["type"]     = i;
        item["name"]     = rec ? ToUtf8(rec->GetNameText()) : "";
        item["cost"]     = rec ? rec->GetShieldCost() : 0;
        item["attack"]   = rec ? rec->GetAttack() : 0.0;
        item["defense"]  = rec ? rec->GetDefense() : 0.0;
        buildable.push_back(item);
    }
    result["buildable"] = buildable;

    // Buildable city improvements (granary-class growth levers) — with
    // names: the model must be able to find "Granary" without a DB dump.
    json buildings = json::array();
    for (sint32 i = 0; g_theBuildingDB && i < g_theBuildingDB->NumRecords(); ++i) {
        if (!cd->CanBuildBuilding(i)) continue;
        const BuildingRecord * rec = g_theBuildingDB->Get(i, gov);
        if (!rec) continue;
        json item;
        item["category"] = k_GAME_OBJ_TYPE_IMPROVEMENT;
        item["type"]     = i;
        item["name"]     = ToUtf8(rec->GetNameText());
        item["cost"]     = rec->GetProductionCost();
        buildings.push_back(item);
    }
    result["buildable_buildings"] = buildings;

    // Buildable wonders — a separate DB, and the single biggest score lever
    // (plus empire-wide effects). Without this list a driver never knows a
    // wonder is available; set_production "wonder <type>" builds the chosen one.
    json wonders = json::array();
    for (sint32 i = 0; g_theWonderDB && i < g_theWonderDB->NumRecords(); ++i) {
        if (!cd->CanBuildWonder(i)) continue;
        const WonderRecord * rec = g_theWonderDB->Get(i);
        if (!rec) continue;
        json item;
        item["category"] = k_GAME_OBJ_TYPE_WONDER;
        item["type"]     = i;
        item["name"]     = ToUtf8(rec->GetNameText());
        item["cost"]     = rec->GetProductionCost();
        wonders.push_back(item);
    }
    result["buildable_wonders"] = wonders;

    // Specialists: citizens reassigned off tile-work into scientist/entertainer/
    // farmer/laborer/merchant roles (fixed per-head output regardless of terrain
    // — the lever for a city whose worked tiles are poor). Feed set_specialist.
    result["specialists"] = {
        {"workers",      cd->WorkerCount()},
        {"scientists",   cd->SpecialistCount(POP_SCIENTIST)},
        {"entertainers", cd->SpecialistCount(POP_ENTERTAINER)},
        {"farmers",      cd->SpecialistCount(POP_FARMER)},
        {"laborers",     cd->SpecialistCount(POP_LABORER)},
        {"merchants",    cd->SpecialistCount(POP_MERCHANT)} };

    // City governor (mayor): when enabled, the engine auto-manages this city's
    // build queue per a named optimization profile (production/growth/science/
    // gold/...). Feed set_governor; profile list via query_governor_profiles.
    sint32 const seq = cd->GetBuildListSequenceIndex();
    const char * seqName = "";
    if (g_theBuildListSequenceDB && seq >= 0 && seq < g_theBuildListSequenceDB->NumRecords()) {
        const BuildListSequenceRecord * sr = g_theBuildListSequenceDB->Get(seq);
        seqName = sr ? sr->GetNameText() : "";
    }
    result["governor"] = { {"enabled",             cd->GetUseGovernor()},
                           {"build_list_sequence", seq},
                           {"build_list_name",     seqName} };
    return Ok("query_city", result);
}

// query_units — every unit the human can see (fog-of-war filtered via the unit
// visibility bitmask, same mechanism the renderer uses). Includes the human's
// own units. Reports type, position, hp and whether it is a city.
std::string QueryUnits()
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_units", "game_not_loaded");

    Player * human = HumanPlayer();
    if (!human)
        return Err("query_units", "no_human_player");
    sint32 vis = human->GetOwner();

    json units = json::array();
    for (sint32 p = 0; p < k_MAX_PLAYERS; ++p) {
        if (!player_Get(p) || !player_Get(p)->m_all_units) continue;
        for (sint32 i = 0; i < player_Get(p)->m_all_units->Num(); ++i) {
            Unit u = player_Get(p)->m_all_units->Access(i);
            if (!u.IsValid()) continue;
            if (!(u.GetVisibility() & (1 << vis))) continue;
            MapPoint pos;
            u.GetPos(pos);
            json j;
            j["owner"]   = p;
            j["type"]    = u.GetType();
            j["name"]    = ToUtf8(u.GetName());
            j["pos"]     = { {"x", pos.x}, {"y", pos.y} };
            j["hp"]      = u.GetHP();
            j["is_city"] = u.IsCity();
            units.push_back(j);
        }
    }

    json result;
    result["visible_player"] = vis;
    result["units"]          = units;
    return Ok("query_units", result);
}

// query_research — the science affordance set: what's being researched, what
// could be, and what each option costs. Research is the main score engine
// (advances unlock units/buildings/terraform and feed the score formula).
std::string QueryResearch()
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_research", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human || !human->m_advances)
        return Err("query_research", "no_human_player");
    if (!g_theAdvanceDB)
        return Err("query_research", "no_advance_db");

    Advances * adv = human->m_advances;
    sint32 researching = adv->GetResearching();

    json result;
    json cur;
    cur["id"] = researching;
    if (researching >= 0 && researching < g_theAdvanceDB->NumRecords()) {
        const AdvanceRecord * r = g_theAdvanceDB->Get(researching);
        cur["name"] = r ? ToUtf8(r->GetNameText()) : "";
        cur["cost"] = adv->GetCost(researching);
    }
    result["researching"] = cur;

    sint32 known = 0;
    json avail = json::array();
    for (sint32 i = 0; i < g_theAdvanceDB->NumRecords(); ++i) {
        if (adv->HasAdvance(i)) { ++known; continue; }
        if (!adv->CanResearch(i)) continue;
        const AdvanceRecord * r = g_theAdvanceDB->Get(i);
        if (!r) continue;
        json j;
        j["id"]   = i;
        j["name"] = ToUtf8(r->GetNameText());
        j["cost"] = adv->GetCost(i);
        avail.push_back(j);
    }
    result["known_count"] = known;
    result["available"]   = avail;
    return Ok("query_research", result);
}

// set_research <advance_id>
std::string CmdSetResearch(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("set_research", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human || !human->m_advances)
        return Err("set_research", "no_human_player");

    int id = -1;
    if (sscanf(args, "%d", &id) != 1)
        return Err("set_research", "bad_args");
    if (!g_theAdvanceDB || id < 0 || id >= g_theAdvanceDB->NumRecords())
        return Err("set_research", "bad_advance");
    if (human->m_advances->HasAdvance(id))
        return Err("set_research", "already_known");
    if (!human->m_advances->CanResearch(id))
        return Err("set_research", "prerequisites_missing");

    human->StartResearching(id);
    const AdvanceRecord * r = g_theAdvanceDB->Get(id);
    json result;
    result["id"]   = id;
    result["name"] = r ? ToUtf8(r->GetNameText()) : "";
    gc_log->info("set_research: {} ({})", id, r ? r->GetNameText() : "?");
    return Ok("set_research", result);
}

// query_terraform <x> <y> — terraform options for one tile: which transform
// improvements the player can build there, what terrain they yield, and the
// Public Works price (with the player's PW balance for context).
std::string QueryTerraform(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_terraform", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("query_terraform", "no_human_player");

    int x = -1, y = -1;
    if (sscanf(args, "%d %d", &x, &y) != 2)
        return Err("query_terraform", "bad_args");
    World * w = world_Get();
    if (!w || x < 0 || x >= w->GetXWidth() || y < 0 || y >= w->GetYHeight())
        return Err("query_terraform", "bad_position");

    MapPoint pos((sint16)x, (sint16)y);
    sint32 const terrain = w->GetTerrainType(pos);
    sint32 const materials = human->GetMaterialsStored();
    Cell * cell = w->GetCell(pos);

    // Categorise an improvement so a driver/UI can filter "what can I build on
    // this tile" — terraform AND ordinary tile infrastructure (farms add food,
    // mines add production, roads add trade/movement).  Previously only
    // terraform-class showed, so farms/roads/mines were invisible even though
    // the `terraform` verb (CanCreateImprovement) can build them.
    json options = json::array();
    for (sint32 i = 0; g_theTerrainImprovementDB && i < g_theTerrainImprovementDB->NumRecords(); ++i) {
        const TerrainImprovementRecord * rec = g_theTerrainImprovementDB->Get(i);
        if (!rec) continue;
        bool const isTerraform = rec->GetClassTerraform() || rec->GetClassOceanform();
        sint32 to = -1;
        if (isTerraform) {
            if (!rec->GetTerraformTerrainIndex(to)) continue;
            if (to == terrain) continue;  // no-op transform
        }
        // Buildability gate — advances, terrain rules, excludes, already-built,
        // borders.  check_materials=FALSE on purpose: this is a QUERY, so we list
        // everything buildable in principle and report current affordability
        // separately in the `affordable` field below.  (Gating on materials here
        // would hide every option whenever Public Works is low — e.g. right after
        // researching the tech that unlocks terraforming, before PW accumulates.)
        ERR_BUILD_INST err;
        if (!human->CanCreateImprovement(i, pos, 0, false, err)) continue;
        sint32 const cost = terrainutil_GetProductionCost(i, pos, 0);
        json o;
        o["improvement_id"]  = i;
        o["name"]            = ToUtf8(rec->GetNameText());
        o["class"]           = TerrainImpClass(rec);
        o["is_terraform"]    = isTerraform;
        if (isTerraform) {
            o["to_terrain"]      = to;
            const TerrainRecord * tr = g_theTerrainDB ? g_theTerrainDB->Get(to) : nullptr;
            o["to_terrain_name"] = tr ? ToUtf8(tr->GetNameText()) : "";
        }
        o["cost"]            = cost;
        o["turns"]           = terrainutil_GetProductionTime(i, pos, 0);
        o["affordable"]      = cost <= materials;
        options.push_back(o);
    }

    json result;
    result["pos"]          = { {"x", x}, {"y", y} };
    result["terrain"]      = terrain;
    // Terraforming only works INSIDE your borders — surface the owner so a
    // driver understands an empty options list.
    result["tile_owner"]   = cell ? cell->GetOwner() : -1;
    result["materials"]    = materials;
    result["material_tax"] = human->m_materialsTax;
    result["options"]      = options;
    return Ok("query_terraform", result);
}

// terraform <x> <y> <improvement_id> — spend Public Works to start a terrain
// transform; it completes after the option's `turns`.
std::string CmdTerraform(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("terraform", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("terraform", "no_human_player");

    int x = -1, y = -1, id = -1;
    if (sscanf(args, "%d %d %d", &x, &y, &id) != 3)
        return Err("terraform", "bad_args");
    World * w = world_Get();
    if (!w || x < 0 || x >= w->GetXWidth() || y < 0 || y >= w->GetYHeight())
        return Err("terraform", "bad_position");
    if (!g_theTerrainImprovementDB || id < 0 || id >= g_theTerrainImprovementDB->NumRecords())
        return Err("terraform", "bad_improvement");

    MapPoint pos((sint16)x, (sint16)y);
    ERR_BUILD_INST err;
    if (!human->CanCreateImprovement(id, pos, 0, true, err))
        return Err("terraform", "cannot_build_here");

    TerrainImprovement imp = human->CreateImprovement(id, pos, 0);
    if (!terrimprovepool_Get() || !terrimprovepool_Get()->IsValid(imp.m_id))
        return Err("terraform", "create_failed");
    if (gevmanager_Get())
        gevmanager_Get()->Process();

    json result;
    result["pos"]   = { {"x", x}, {"y", y} };
    result["turns"] = terrainutil_GetProductionTime(id, pos, 0);
    gc_log->info("terraform: improvement {} at ({},{})", id, x, y);
    return Ok("terraform", result);
}

// set_material_tax <percent 0..100> — divert city production into the Public
// Works pool that pays for terraforming. Without this the human's PW stays 0.
std::string CmdSetMaterialTax(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("set_material_tax", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("set_material_tax", "no_human_player");

    int pct = -1;
    if (sscanf(args, "%d", &pct) != 1 || pct < 0 || pct > 100)
        return Err("set_material_tax", "bad_args");

    human->SetMaterialsTax(pct / 100.0);
    json result;
    result["material_tax"] = human->m_materialsTax;
    return Ok("set_material_tax", result);
}

// board <army_idx> — embark a land army into transports standing ON ITS OWN
// TILE (port boarding: city tile holds both troops and docked boats).
// Boarding an ADJACENT transport is move_army onto its tile.
std::string CmdBoard(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("board", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("board", "no_human_player");

    int idx = -1;
    if (sscanf(args, "%d", &idx) != 1)
        return Err("board", "bad_args");
    DynamicArray<Army> * armies = human->GetAllArmiesList();
    if (!armies || idx < 0 || idx >= armies->Num())
        return Err("board", "bad_army_index");
    Army army = armies->Access(idx);
    ArmyData * ad = army.AccessData();
    if (!army.IsValid() || !ad)
        return Err("board", "invalid_army");

    // Capture the member units BEFORE processing: when every unit boards,
    // the army dissolves into the transport's cargo and the handle goes
    // invalid — Army::Num() on it after Process() was a SIGSEGV (caught
    // by the crash reporter while building the cargo fixture).
    std::vector<Unit> members;
    for (sint32 u = 0; u < army.Num(); ++u)
        members.push_back(army.Access(u));

    army.ClearOrders();
    army.AddOrders(UNIT_ORDER_BOARD_TRANSPORT);
    if (gevmanager_Get()) gevmanager_Get()->Process();

    // The board order silently no-ops when no transport with free
    // capacity is on or next to the army's tile — verify the units are
    // actually aboard instead of reporting blind success. (Found by
    // campaign 7: an "ok" board left the settler ashore and the coracle
    // sailed empty for 30 rounds.)
    sint32 aboard = 0;
    for (auto & unit : members) {
        if (unit.IsValid() && unit.IsBeingTransported())
            ++aboard;
    }
    if (aboard == 0)
        return Err("board", "no_transport_in_range");

    gc_log->info("board: army {} ({} unit(s) aboard)", idx, (int)aboard);
    json result;
    result["units_aboard"] = aboard;
    return Ok("board", result);
}

// fortify <army_idx> — entrench the army in place (defensive bonus).
// Garrisons that merely stand around take full damage; campaign 4's
// annihilation taught the difference.
std::string CmdFortify(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("fortify", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("fortify", "no_human_player");

    int idx = -1;
    if (sscanf(args, "%d", &idx) != 1)
        return Err("fortify", "bad_args");
    DynamicArray<Army> * armies = human->GetAllArmiesList();
    if (!armies || idx < 0 || idx >= armies->Num())
        return Err("fortify", "bad_army_index");
    Army army = armies->Access(idx);
    ArmyData * ad = army.AccessData();
    if (!army.IsValid() || !ad)
        return Err("fortify", "invalid_army");

    army.ClearOrders();
    army.AddOrders(UNIT_ORDER_ENTRENCH);
    if (gevmanager_Get()) gevmanager_Get()->Process();

    if (!ad->IsEntrenched() && !ad->IsEntrenching())
        return Err("fortify", "entrench_rejected");
    json result;
    result["entrenched"]  = ad->IsEntrenched();
    result["entrenching"] = ad->IsEntrenching();
    gc_log->info("fortify: army {}", idx);
    return Ok("fortify", result);
}

// unload <army_idx> <x> <y> — order a transport army to disembark its cargo
// onto an adjacent tile (the amphibious landing). The game validates
// passability/capacity; we report the cargo count afterwards.
std::string CmdUnload(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("unload", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("unload", "no_human_player");

    int idx = -1, x = -1, y = -1;
    if (sscanf(args, "%d %d %d", &idx, &x, &y) != 3)
        return Err("unload", "bad_args");
    DynamicArray<Army> * armies = human->GetAllArmiesList();
    if (!armies || idx < 0 || idx >= armies->Num())
        return Err("unload", "bad_army_index");
    World * w = world_Get();
    if (!w || x < 0 || x >= w->GetXWidth() || y < 0 || y >= w->GetYHeight())
        return Err("unload", "bad_position");

    Army army = armies->Access(idx);
    ArmyData * ad = army.AccessData();
    if (!army.IsValid() || !ad)
        return Err("unload", "invalid_army");

    MapPoint dest((sint16)x, (sint16)y);
    if (!ad->RetPos().IsNextTo(dest) && !(ad->RetPos() == dest))
        return Err("unload", "not_adjacent");

    if (!gevmanager_Get())
        return Err("unload", "no_event_manager");
    gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_UnloadOrder,
                               GEA_Army, army,
                               GEA_MapPoint, dest, GEA_End);
    gevmanager_Get()->Process();

    json result;
    result["army"] = idx;
    result["pos"]  = { {"x", x}, {"y", y} };
    gc_log->info("unload: army {} at ({},{})", idx, x, y);
    return Ok("unload", result);
}

// group_army <army_idx> — merge EVERY unit standing on the army's tile into
// it (the UI's "group all"). Stacks up to 12 units fight as ONE army —
// campaign 4 was lost by sending single-unit armies into a stack.
std::string CmdGroupArmy(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("group_army", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("group_army", "no_human_player");

    int idx = -1;
    if (sscanf(args, "%d", &idx) != 1)
        return Err("group_army", "bad_args");
    DynamicArray<Army> * armies = human->GetAllArmiesList();
    if (!armies || idx < 0 || idx >= armies->Num())
        return Err("group_army", "bad_army_index");
    Army army = armies->Access(idx);
    ArmyData * ad = army.AccessData();
    if (!army.IsValid() || !ad)
        return Err("group_army", "invalid_army");

    ad->GroupAllUnits();
    if (gevmanager_Get()) gevmanager_Get()->Process();

    json result;
    result["army"]  = idx;
    result["units"] = ad->Num();
    gc_log->info("group_army: army {} now {} units", idx, (int)ad->Num());
    return Ok("group_army", result);
}

// ungroup_army <army_idx> — split the stack back into single-unit armies.
std::string CmdUngroupArmy(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("ungroup_army", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("ungroup_army", "no_human_player");

    int idx = -1;
    if (sscanf(args, "%d", &idx) != 1)
        return Err("ungroup_army", "bad_args");
    DynamicArray<Army> * armies = human->GetAllArmiesList();
    if (!armies || idx < 0 || idx >= armies->Num())
        return Err("ungroup_army", "bad_army_index");
    Army army = armies->Access(idx);
    if (!army.IsValid() || !army.AccessData())
        return Err("ungroup_army", "invalid_army");

    if (gevmanager_Get()) {
        gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_UngroupOrder,
                                   GEA_Army, army, GEA_End);
        gevmanager_Get()->Process();
    }
    return Ok("ungroup_army");
}

// declare_war <player_id> — formal war declaration via the diplomacy layer
// (sets the DECLARE_WAR agreement both engines honor). Requires CONTACT:
// you cannot declare war on a civilization you have never met.
std::string CmdDeclareWar(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("declare_war", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("declare_war", "no_human_player");

    int id = -1;
    if (sscanf(args, "%d", &id) != 1)
        return Err("declare_war", "bad_args");
    if (id < 0 || id >= k_MAX_PLAYERS || !player_Get(id))
        return Err("declare_war", "bad_player");
    if (id == human->GetOwner())
        return Err("declare_war", "thats_you");
    if (human->HasWarWith(id))
        return Err("declare_war", "already_at_war");
    if (!human->HasContactWith(id) || !player_Get(id)->HasContactWith(human->GetOwner()))
        return Err("declare_war", "no_contact");

    Diplomat::GetDiplomat(human->GetOwner()).DeclareWar(id);
    if (gevmanager_Get()) gevmanager_Get()->Process();

    json result;
    result["target"] = id;
    result["at_war"] = human->HasWarWith(id);
    gc_log->info("declare_war: {} -> {}", (int)human->GetOwner(), id);
    return Ok("declare_war", result);
}

// attack <army_idx> <x> <y> — order an army onto an ADJACENT enemy-occupied
// tile; the move resolves combat (and captures the city if the defenders
// die and a city stands there). Requires being at war with the defender.
std::string CmdAttack(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("attack", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("attack", "no_human_player");

    int idx = -1, x = -1, y = -1;
    if (sscanf(args, "%d %d %d", &idx, &x, &y) != 3)
        return Err("attack", "bad_args");
    DynamicArray<Army> * armies = human->GetAllArmiesList();
    if (!armies || idx < 0 || idx >= armies->Num())
        return Err("attack", "bad_army_index");
    World * w = world_Get();
    if (!w || x < 0 || x >= w->GetXWidth() || y < 0 || y >= w->GetYHeight())
        return Err("attack", "bad_position");

    Army army = armies->Access(idx);
    ArmyData * ad = army.AccessData();
    if (!army.IsValid() || !ad)
        return Err("attack", "invalid_army");

    MapPoint src = ad->RetPos();
    MapPoint dest((sint16)x, (sint16)y);
    if (!src.IsNextTo(dest))
        return Err("attack", "not_adjacent");

    Cell * cell = w->GetCell(dest);
    sint32 defender = -1;
    if (cell && cell->GetCity().IsValid())
        defender = cell->GetCity().GetOwner();
    else if (cell && cell->GetNumUnits() > 0)
        defender = cell->AccessUnit(0).GetOwner();
    if (defender < 0)
        return Err("attack", "nothing_to_attack");
    if (defender == human->GetOwner())
        return Err("attack", "own_forces");
    if (!human->HasWarWith(defender))
        return Err("attack", "not_at_war");

    // A manual order overrides auto-explore.
    for (sint32 u = 0; u < ad->Num(); ++u) {
        Unit unit = ad->Access(u);
        if (UnitData * ud = unit.AccessData()) ud->SetExploring(false);
    }
    army.ClearOrders();
    army.AddOrders(UNIT_ORDER_MOVE_TO, dest);
    if (gevmanager_Get()) gevmanager_Get()->Process();

    // Report what the battlefield looks like afterwards.
    MapPoint now = ad->RetPos();
    json result;
    result["army_survived"]  = armypool_Get() && army.IsValid();
    result["pos"]            = { {"x", now.x}, {"y", now.y} };
    result["captured_tile"]  = (now.x == x && now.y == y);
    cell = w->GetCell(dest);
    result["defenders_left"] = cell ? cell->GetNumUnits() : 0;
    gc_log->info("attack: army {} ({},{}) -> ({},{})", idx, (int)src.x, (int)src.y, x, y);
    return Ok("attack", result);
}

// bombard <army_idx> <x> <y> — ranged strike on an ADJACENT enemy-occupied
// tile. Unlike attack, the bombarding army stays put and takes no damage;
// use it to soften a stack (or a city's defenders) before the assault.
// Requires war, a unit with bombard capability, and remaining special-action
// points this turn.
std::string CmdBombard(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("bombard", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("bombard", "no_human_player");

    int idx = -1, x = -1, y = -1;
    if (sscanf(args, "%d %d %d", &idx, &x, &y) != 3)
        return Err("bombard", "bad_args");
    DynamicArray<Army> * armies = human->GetAllArmiesList();
    if (!armies || idx < 0 || idx >= armies->Num())
        return Err("bombard", "bad_army_index");
    World * w = world_Get();
    if (!w || x < 0 || x >= w->GetXWidth() || y < 0 || y >= w->GetYHeight())
        return Err("bombard", "bad_position");

    Army army = armies->Access(idx);
    ArmyData * ad = army.AccessData();
    if (!army.IsValid() || !ad)
        return Err("bombard", "invalid_army");

    MapPoint dest((sint16)x, (sint16)y);
    if (!ad->RetPos().IsNextTo(dest))
        return Err("bombard", "not_adjacent");

    CellUnitList defenders;
    w->GetArmy(dest, defenders);
    if (defenders.Num() == 0)
        return Err("bombard", "nothing_to_bombard");
    sint32 defOwner = defenders.GetOwner();
    if (defOwner == human->GetOwner())
        return Err("bombard", "own_forces");
    if (!human->HasWarWith(defOwner))
        return Err("bombard", "not_at_war");
    if (!ad->CanBombard(dest))
        return Err("bombard", "cannot_bombard");

    // Total defender HP before/after is the honest damage report — the
    // bombard event itself succeeds silently even when every shot misses.
    double hpBefore = 0.0;
    for (sint32 i = 0; i < defenders.Num(); ++i)
        hpBefore += defenders[i].GetHP();

    if (!gevmanager_Get())
        return Err("bombard", "no_event_manager");
    gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_BombardOrder,
                               GEA_Army, army,
                               GEA_MapPoint, dest, GEA_End);
    gevmanager_Get()->Process();

    CellUnitList after;
    w->GetArmy(dest, after);
    double hpAfter = 0.0;
    for (sint32 i = 0; i < after.Num(); ++i)
        hpAfter += after[i].GetHP();

    json result;
    result["target"]         = { {"x", x}, {"y", y} };
    result["defenders_left"] = after.Num();
    result["damage_dealt"]   = hpBefore - hpAfter;
    gc_log->info("bombard: army {} -> ({},{}), damage {}", idx, x, y, hpBefore - hpAfter);
    return Ok("bombard", result);
}

// buy_production <city_idx> — rush-buy the city's current build item with
// gold (the "overtime" buy of the city panel). The lever for getting a
// freshly captured city productive (a riot-calming building NOW, not in 30
// rounds) and for emergency military. Reports the price actually paid.
std::string CmdBuyProduction(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("buy_production", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("buy_production", "no_human_player");

    int city_idx = -1;
    if (sscanf(args, "%d", &city_idx) != 1)
        return Err("buy_production", "bad_args");
    if (city_idx < 0 || city_idx >= human->GetAllCitiesList()->Num())
        return Err("buy_production", "bad_city_index");
    Unit u = human->GetAllCitiesList()->Access(city_idx);
    CityData * cd = (u.IsValid() && u.GetData()) ? u.GetData()->GetCityData() : nullptr;
    if (!cd)
        return Err("buy_production", "invalid_city");
    if (!cd->GetBuildQueue() || !cd->GetBuildQueue()->GetHead())
        return Err("buy_production", "nothing_being_built");
    if (cd->AlreadyBoughtFront())
        return Err("buy_production", "already_bought");

    sint32 cost = cd->GetOvertimeCost();
    sint32 gold = human->m_gold ? human->m_gold->GetLevel() : 0;
    if (cost > gold) {
        json result;
        result["cost"] = cost;
        result["gold"] = gold;
        return Err("buy_production", "not_enough_gold");
    }

    if (!cd->BuyFront())
        return Err("buy_production", "buy_rejected");
    if (gevmanager_Get()) gevmanager_Get()->Process();

    json result;
    result["cost"]       = cost;
    result["gold_after"] = human->m_gold ? human->m_gold->GetLevel() : 0;
    gc_log->info("buy_production: city {} for {} gold", city_idx, (int)cost);
    return Ok("buy_production", result);
}

// propose_peace <player_id> — send a formal PEACE TREATY proposal through
// the diplomacy layer. The AI considers it with its real evaluation (war
// regard, relative strength) and may REJECT — the result reports whether
// the war actually ended. Peace is the AI's choice, not a cheat switch.
std::string CmdProposePeace(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("propose_peace", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("propose_peace", "no_human_player");

    int id = -1;
    if (sscanf(args, "%d", &id) != 1)
        return Err("propose_peace", "bad_args");
    if (id < 0 || id >= k_MAX_PLAYERS || !player_Get(id))
        return Err("propose_peace", "bad_player");
    if (id == human->GetOwner())
        return Err("propose_peace", "thats_you");
    if (!human->HasContactWith(id))
        return Err("propose_peace", "no_contact");
    if (!human->HasWarWith(id))
        return Err("propose_peace", "not_at_war");

    NewProposal proposal;
    proposal.senderId          = human->GetOwner();
    proposal.receiverId        = id;
    proposal.priority          = 1;
    proposal.detail.first_type = PROPOSAL_TREATY_PEACE;
    proposal.detail.tone       = DIPLOMATIC_TONE_EQUAL;
    Diplomat::GetDiplomat(human->GetOwner()).ExecuteNewProposal(proposal);
    if (gevmanager_Get()) gevmanager_Get()->Process();

    // The AI's verdict: an accepted treaty lands in the agreement matrix
    // and ends the war state immediately; a rejection leaves both intact.
    bool treaty = AgreementMatrix::s_agreements.HasAgreement(
                      human->GetOwner(), id, PROPOSAL_TREATY_PEACE);
    json result;
    result["target"]       = id;
    result["at_war"]       = human->HasWarWith(id);
    result["peace_treaty"] = treaty;
    result["accepted"]     = treaty && !human->HasWarWith(id);
    gc_log->info("propose_peace: {} -> {} (treaty {}, at_war {})",
                 (int)human->GetOwner(), id, treaty, human->HasWarWith(id));
    return Ok("propose_peace", result);
}

// grant_advance <advance_id> — DEBUG/TEST cheat: hand the human an advance
// outright (prerequisites included via the game's own SetHasAdvance). Exists
// so integration tests can reach late-game mechanics (e.g. swamp terraform
// needs Industrial Revolution) without playing 300 rounds. Not exposed as an
// MCP tool; reachable via raw_cmd when explicitly requested.
std::string CmdGrantAdvance(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("grant_advance", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human || !human->m_advances)
        return Err("grant_advance", "no_human_player");

    int id = -1;
    if (sscanf(args, "%d", &id) != 1)
    {
        // Also accept the internal ID text (e.g. ADVANCE_NANO_ASSEMBLY) so
        // tests don't have to hardcode database indices.
        char name[128] = {0};
        if (g_theAdvanceDB && sscanf(args, "%127s", name) == 1)
        {
            for (sint32 i = 0; i < g_theAdvanceDB->NumRecords(); ++i)
            {
                const AdvanceRecord * rec = g_theAdvanceDB->Get(i);
                if (rec && rec->GetIDText() && strcmp(rec->GetIDText(), name) == 0)
                {
                    id = i;
                    break;
                }
            }
        }
        if (id < 0)
            return Err("grant_advance", "bad_args");
    }
    if (!g_theAdvanceDB || id < 0 || id >= g_theAdvanceDB->NumRecords())
        return Err("grant_advance", "bad_advance");

    // Player::SetHasAdvance is the notification/ceremony layer and does NOT
    // store the bit — the Advances object does.
    human->m_advances->SetHasAdvance(id);
    const AdvanceRecord * r = g_theAdvanceDB->Get(id);
    gc_log->info("grant_advance (DEBUG): {} ({})", id, r ? r->GetNameText() : "?");
    json result;
    result["id"]   = id;
    result["name"] = r ? ToUtf8(r->GetNameText()) : "";
    return Ok("grant_advance", result);
}

// create_unit <UNIT_ID|index> <x> <y> — DEBUG/TEST cheat: spawn a unit for
// the human at a position, bypassing production. Companion to grant_advance:
// together they let integration tests reach late-game content (undersea
// cities, space layer) that is organically hundreds of rounds away. The
// type accepts the internal ID text (UNIT_SEA_ENGINEER) or a DB index.
// Not exposed as an MCP tool; reachable via raw_cmd when explicitly asked.
std::string CmdCreateUnit(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("create_unit", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("create_unit", "no_human_player");
    if (!g_theUnitDB)
        return Err("create_unit", "no_unit_db");

    char name[128] = {0};
    int x = -1, y = -1;
    if (sscanf(args, "%127s %d %d", name, &x, &y) != 3)
        return Err("create_unit", "bad_args");

    sint32 type = ResolveUnitType(name);
    if (type < 0 || type >= g_theUnitDB->NumRecords())
        return Err("create_unit", "bad_unit_type");

    World * w = world_Get();
    if (!w || x < 0 || y < 0 || x >= w->GetXWidth() || y >= w->GetYHeight())
        return Err("create_unit", "bad_position");

    MapPoint pos(x, y);
    Unit u = human->CreateUnit(type, pos, Unit(), false,
                               CAUSE_NEW_ARMY_INITIAL);
    if (!u.IsValid())
        return Err("create_unit", "create_failed");

    gc_log->info("create_unit (DEBUG): type {} at ({},{})", type, x, y);
    json result;
    result["type"] = type;
    result["pos"]  = { {"x", x}, {"y", y} };
    return Ok("create_unit", result);
}

// disband_unit <army_index> — disband one of the human's armies, freeing the
// shields/turn it costs in support. The dominant efficiency lever once a war
// is unwinnable/unreachable: a stack of obsolete units bleeds production every
// turn for no benefit. Uses ArmyData::Disband (which refuses your LAST army).
std::string CmdDisbandUnit(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("disband_unit", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("disband_unit", "no_human_player");

    int idx = -1;
    if (sscanf(args, "%d", &idx) != 1)
        return Err("disband_unit", "bad_args");
    DynamicArray<Army> * armies = human->GetAllArmiesList();
    if (!armies || idx < 0 || idx >= armies->Num())
        return Err("disband_unit", "bad_army_index");
    Army army = armies->Access(idx);
    ArmyData * ad = army.AccessData();
    if (!army.IsValid() || !ad)
        return Err("disband_unit", "invalid_army");
    if (armies->Num() < 2 && human->m_all_cities->Num() < 1)
        return Err("disband_unit", "last_army");  // Disband() would no-op

    sint32 const n_units = ad->Num();
    ad->Disband();
    if (gevmanager_Get()) gevmanager_Get()->Process();

    json result;
    result["disbanded_army"] = idx;
    result["units_removed"]  = n_units;
    result["armies_left"]    = human->GetAllArmiesList()->Num();
    gc_log->info("disband_unit: army {} ({} units)", idx, n_units);
    return Ok("disband_unit", result);
}

// set_government <government_type> — switch government. The master multiplier
// on science/gold/happiness/production and max city size. SetGovernmentType
// self-gates on the enabling advance and handles the anarchy transition; it
// returns false if the type is invalid, unchanged, or the advance is missing.
std::string CmdSetGovernment(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("set_government", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("set_government", "no_human_player");

    int type = -1;
    if (sscanf(args, "%d", &type) != 1)
        return Err("set_government", "bad_args");
    if (!g_theGovernmentDB || type < 0 || type >= g_theGovernmentDB->NumRecords())
        return Err("set_government", "bad_government");
    const GovernmentRecord * grec = g_theGovernmentDB->Get(type);
    if (type == human->GetGovernmentType())
        return Err("set_government", "already_that_government");
    if (grec && !human->HasAdvance(grec->GetEnableAdvanceIndex()))
        return Err("set_government", "advance_missing");

    bool const ok = human->SetGovernmentType(type);
    if (!ok)
        return Err("set_government", "rejected");
    if (gevmanager_Get()) gevmanager_Get()->Process();

    json result;
    result["requested_government"] = type;
    result["name"]                 = grec ? ToUtf8(grec->GetNameText()) : "";
    // A switch FROM an established government routes through anarchy first, so
    // the active type may still be the old one (or 0/anarchy) this turn.
    result["active_government"]    = human->GetGovernmentType();
    gc_log->info("set_government: -> {} (active {})", type, human->GetGovernmentType());
    return Ok("set_government", result);
}

// establish_trade_route <src_city_index> <dest_city_index> [good_index]
// — create a resource trade route from one of YOUR cities to another visible
// city, generating trade value. If good_index is omitted, the first good the
// source city can collect is used. Gold-economy lever the play loop ignored.
std::string CmdEstablishTradeRoute(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("establish_trade_route", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("establish_trade_route", "no_human_player");

    int src_idx = -1, dst_idx = -1, good = -1;
    int const n = sscanf(args, "%d %d %d", &src_idx, &dst_idx, &good);
    if (n < 2)
        return Err("establish_trade_route", "bad_args");

    UnitDynamicArray * mine = human->GetAllCitiesList();
    if (!mine || src_idx < 0 || src_idx >= mine->Num())
        return Err("establish_trade_route", "bad_source_city");
    Unit srcCity = mine->Access(src_idx);
    CityData * scd = srcCity.IsValid() && srcCity.GetData() ? srcCity.GetData()->GetCityData() : nullptr;
    if (!scd)
        return Err("establish_trade_route", "invalid_source_city");

    // Destination can be any city the human can see (own or foreign).
    Unit destCity;
    for (sint32 p = 0; p < k_MAX_PLAYERS && !destCity.IsValid(); ++p) {
        if (!player_Get(p)) continue;
        UnitDynamicArray * list = player_Get(p)->GetAllCitiesList();
        if (!list) continue;
        for (sint32 i = 0; i < list->Num(); ++i) {
            Unit u = list->Access(i);
            if (p == human->GetOwner() && i == dst_idx) { destCity = u; break; }
        }
    }
    if (!destCity.IsValid())
        return Err("establish_trade_route", "bad_dest_city");
    if (destCity.m_id == srcCity.m_id)
        return Err("establish_trade_route", "same_city");

    // Auto-pick a good the source city can actually collect, if unspecified.
    if (good < 0 && g_theResourceDB) {
        for (sint32 r = 0; r < g_theResourceDB->NumRecords(); ++r) {
            if (scd->GetGoodCountInRadius(r) > 0) { good = r; break; }
        }
    }
    if (good < 0 || !g_theResourceDB || good >= g_theResourceDB->NumRecords())
        return Err("establish_trade_route", "no_good_available");
    // CRITICAL: CreateTradeRoute null-derefs on a good the source city cannot
    // actually export.  Validate collectability for an EXPLICIT good too, not
    // just the auto-picked one — otherwise a bad good_index crashes the engine.
    if (scd->GetGoodCountInRadius(good) <= 0)
        return Err("establish_trade_route", "good_not_in_source_radius");

    TradeRoute route = human->CreateTradeRoute(srcCity, ROUTE_TYPE_RESOURCE, good,
                                               destCity, human->GetOwner(), 0);
    if (gevmanager_Get()) gevmanager_Get()->Process();
    if (!route.IsValid())
        return Err("establish_trade_route", "route_rejected");

    json result;
    result["source_city"] = src_idx;
    result["dest_city"]   = dst_idx;
    result["good"]        = good;
    const ResourceRecord * rr = g_theResourceDB->Get(good);
    result["good_name"]   = rr ? ToUtf8(rr->GetNameText()) : "";
    gc_log->info("establish_trade_route: city {} -> {} good {}", src_idx, dst_idx, good);
    return Ok("establish_trade_route", result);
}

// set_science_rate <percent 0..100> — set the science share of commerce (the
// rest becomes gold). The engine clamps to the government's max science rate.
// A direct score dial: more science = faster advances; the UI slider the play
// loop never touched. Read back economy.science_rate in query_player.
std::string CmdSetScienceRate(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("set_science_rate", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("set_science_rate", "no_human_player");

    int pct = -1;
    if (sscanf(args, "%d", &pct) != 1)
        return Err("set_science_rate", "bad_args");
    if (pct < 0 || pct > 100)
        return Err("set_science_rate", "out_of_range");

    human->SetTaxes(static_cast<double>(pct) / 100.0);
    if (gevmanager_Get()) gevmanager_Get()->Process();

    double applied = 0.0;
    human->GetScienceTaxRate(applied);
    json result;
    result["requested_science_rate"] = static_cast<double>(pct) / 100.0;
    result["science_rate"]           = applied;   // may be capped by government
    result["gold_rate"]              = 1.0 - applied;
    gc_log->info("set_science_rate: requested {}%, applied {}", pct, applied);
    return Ok("set_science_rate", result);
}

// set_rates <workday> <wages> <rations> — the three social sliders (pass -1 to
// leave one unchanged). Each trades raw output for happiness around the
// government's expectation level: lower RATIONS frees food for growth (the
// non-terraform growth lever), higher workday adds production, etc. Levels are
// integer steps; query_player.economy reports current level + expectation.
std::string CmdSetRates(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("set_rates", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("set_rates", "no_human_player");

    int workday = -1, wages = -1, rations = -1;
    if (sscanf(args, "%d %d %d", &workday, &wages, &rations) < 1)
        return Err("set_rates", "bad_args");

    if (workday >= 0) human->SetWorkdayLevel(workday);
    if (wages   >= 0) human->SetWagesLevel(wages);
    if (rations >= 0) human->SetRationsLevel(rations);
    if (gevmanager_Get()) gevmanager_Get()->Process();

    PlayerHappiness * h = human->m_global_happiness;
    json result;
    result["workday"] = { {"level", h ? h->GetUnitlessWorkday() : 0}, {"expectation", human->GetWorkdayExpectation()} };
    result["wages"]   = { {"level", h ? h->GetUnitlessWages()   : 0}, {"expectation", human->GetWagesExpectation()} };
    result["rations"] = { {"level", h ? h->GetUnitlessRations() : 0}, {"expectation", human->GetRationsExpectation()} };
    gc_log->info("set_rates: workday {} wages {} rations {}", workday, wages, rations);
    return Ok("set_rates", result);
}

// set_readiness <peace|alert|war|0|1|2> — the military footing the UI exposes
// but the API ignored. Higher readiness raises every unit's ready-HP (combat
// effectiveness) at a per-turn gold cost (economy.readiness.cost); switching
// applies immediately. A war-prep lever distinct from declaring war.
std::string CmdSetReadiness(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("set_readiness", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("set_readiness", "no_human_player");

    // Accept a word or a 0/1/2 index.
    READINESS_LEVEL level;
    if      (!strncmp(args, "peace", 5) || args[0] == '0') level = READINESS_LEVEL_PEACE;
    else if (!strncmp(args, "alert", 5) || args[0] == '1') level = READINESS_LEVEL_ALERT;
    else if (!strncmp(args, "war",   3) || args[0] == '2') level = READINESS_LEVEL_WAR;
    else return Err("set_readiness", "bad_args");

    human->SetReadinessLevel(level, true);   // immediate
    if (gevmanager_Get()) gevmanager_Get()->Process();

    sint32 const applied = human->GetReadinessLevel();
    char const * label = (applied == 0) ? "peace" : (applied == 1) ? "alert" : (applied == 2) ? "war" : "unknown";
    json result;
    result["readiness"] = { {"level", applied}, {"label", label}, {"cost", human->GetReadinessCost()} };
    gc_log->info("set_readiness: level {} ({})", applied, label);
    return Ok("set_readiness", result);
}

// set_specialist <city_index> <pop_type> <delta> — convert citizens between
// tile-working and a specialist role. pop_type: 1=scientist 2=entertainer
// 3=farmer 4=laborer 5=merchant. delta>0 turns workers INTO specialists
// (needs that many free workers); delta<0 turns them back. Specialists give a
// fixed per-head yield independent of terrain — the lever for a city on poor
// tiles (e.g. add scientists for flat science, farmers for flat food).
std::string CmdSetSpecialist(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("set_specialist", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("set_specialist", "no_human_player");

    int city_idx = -1, ptype = -1, delta = 0;
    if (sscanf(args, "%d %d %d", &city_idx, &ptype, &delta) != 3)
        return Err("set_specialist", "bad_args");
    UnitDynamicArray * mine = human->GetAllCitiesList();
    if (!mine || city_idx < 0 || city_idx >= mine->Num())
        return Err("set_specialist", "bad_city_index");
    CityData * cd = mine->Access(city_idx).GetData() ? mine->Access(city_idx).GetData()->GetCityData() : nullptr;
    if (!cd)
        return Err("set_specialist", "invalid_city");
    // Only the assignable specialist roles (not worker/slave/max).
    if (ptype < POP_SCIENTIST || ptype > POP_MERCHANT)
        return Err("set_specialist", "bad_pop_type");
    if (delta == 0)
        return Err("set_specialist", "zero_delta");
    POP_TYPE type = static_cast<POP_TYPE>(ptype);
    // Pre-validate exactly as ChangeSpecialists does, so we report instead of
    // silently no-opping.
    if (delta > 0 && cd->WorkerCount() < delta)
        return Err("set_specialist", "not_enough_workers");
    if (delta < 0 && cd->SpecialistCount(type) + delta < 0)
        return Err("set_specialist", "not_enough_specialists");

    cd->ChangeSpecialists(type, delta);
    if (gevmanager_Get()) gevmanager_Get()->Process();

    json result;
    result["city"]       = city_idx;
    result["pop_type"]   = ptype;
    result["delta"]      = delta;
    result["now"]        = cd->SpecialistCount(type);
    result["workers"]    = cd->WorkerCount();
    gc_log->info("set_specialist: city {} type {} delta {}", city_idx, ptype, delta);
    return Ok("set_specialist", result);
}

// set_governor <city_index> <0|1> [build_list_sequence] — toggle the city
// mayor. When on, the engine auto-manages the build queue per a named profile
// (query_governor_profiles lists them); the optional index switches profile.
std::string CmdSetGovernor(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("set_governor", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("set_governor", "no_human_player");

    int city_idx = -1, on = -1, seq = -1;
    int const n = sscanf(args, "%d %d %d", &city_idx, &on, &seq);
    if (n < 2)
        return Err("set_governor", "bad_args");
    UnitDynamicArray * mine = human->GetAllCitiesList();
    if (!mine || city_idx < 0 || city_idx >= mine->Num())
        return Err("set_governor", "bad_city_index");
    CityData * cd = mine->Access(city_idx).GetData() ? mine->Access(city_idx).GetData()->GetCityData() : nullptr;
    if (!cd)
        return Err("set_governor", "invalid_city");
    if (n >= 3) {
        if (!g_theBuildListSequenceDB || seq < 0 || seq >= g_theBuildListSequenceDB->NumRecords())
            return Err("set_governor", "bad_build_list_sequence");
        cd->SetBuildListSequenceIndex(seq);
    }
    cd->SetUseGovernor(on != 0);
    if (gevmanager_Get()) gevmanager_Get()->Process();

    sint32 const s = cd->GetBuildListSequenceIndex();
    const char * nm = "";
    if (g_theBuildListSequenceDB && s >= 0 && s < g_theBuildListSequenceDB->NumRecords()) {
        const BuildListSequenceRecord * sr = g_theBuildListSequenceDB->Get(s);
        nm = sr ? sr->GetNameText() : "";
    }
    json result;
    result["city"]                = city_idx;
    result["enabled"]             = cd->GetUseGovernor();
    result["build_list_sequence"] = s;
    result["build_list_name"]     = nm;
    gc_log->info("set_governor: city {} enabled {} seq {}", city_idx, on != 0, s);
    return Ok("set_governor", result);
}

// query_governor_profiles — the named build-list sequences a city governor can
// follow (index + name), e.g. PRODUCTION / GROWTH / SCIENCE / GOLD / OFFENSE.
// Feed the index to set_governor's optional profile argument.
std::string QueryGovernorProfiles()
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_governor_profiles", "game_not_loaded");
    json profiles = json::array();
    for (sint32 i = 0; g_theBuildListSequenceDB && i < g_theBuildListSequenceDB->NumRecords(); ++i) {
        const BuildListSequenceRecord * sr = g_theBuildListSequenceDB->Get(i);
        if (!sr) continue;
        profiles.push_back({ {"index", i}, {"name", sr->GetNameText()} });
    }
    json result;
    result["profiles"] = profiles;
    return Ok("query_governor_profiles", result);
}

// Human-readable list of the target types an order needs — so a driver knows
// where to point do_unit_order without decoding the pretest bitmask.
static json OrderTargetTypes(const OrderRecord * o)
{
    json t = json::array();
    if (o->GetTargetPretestEnemyCity())          t.push_back("enemy_city");
    if (o->GetTargetPretestOwnCity())            t.push_back("own_city");
    if (o->GetTargetPretestEnemyArmy())          t.push_back("enemy_army");
    if (o->GetTargetPretestEnemySpecialUnit())   t.push_back("enemy_special_unit");
    if (o->GetTargetPretestEnemySettler())       t.push_back("enemy_settler");
    if (o->GetTargetPretestEnemyTradeUnit())     t.push_back("enemy_trade_unit");
    if (o->GetTargetPretestTradeRoute())         t.push_back("trade_route");
    if (o->GetTargetPretestTerrainImprovement()) t.push_back("terrain_improvement");
    if (o->GetTargetPretestNone())               t.push_back("none");
    return t;
}

// query_unit_orders <army_index> — the special orders THIS army is capable of
// (the right-click menu the UI builds): index, name, the target type(s) each
// needs, and gold cost. Covers spy/diplomat ops (investigate/steal/incite/
// embassy), pillage, expel, infect, convert, etc. Feed an index to do_unit_order.
std::string QueryUnitOrders(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_unit_orders", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("query_unit_orders", "no_human_player");
    int idx = -1;
    if (sscanf(args, "%d", &idx) != 1)
        return Err("query_unit_orders", "bad_args");
    DynamicArray<Army> * armies = human->GetAllArmiesList();
    if (!armies || idx < 0 || idx >= armies->Num())
        return Err("query_unit_orders", "bad_army_index");
    ArmyData * ad = armies->Access(idx).AccessData();
    if (!ad)
        return Err("query_unit_orders", "invalid_army");

    json orders = json::array();
    for (sint32 i = 0; g_theOrderDB && i < g_theOrderDB->NumRecords(); ++i) {
        const OrderRecord * o = g_theOrderDB->Get(i);
        if (!o) continue;
        // ORDER_TEST_ILLEGAL means no unit in the army can ever do it; anything
        // else (OK / NEEDS_TARGET / LACKS_GOLD / NO_MOVEMENT) means it's a
        // capability of this army worth surfacing.
        if (ad->TestOrder(o) == ORDER_TEST_ILLEGAL) continue;
        orders.push_back({ {"order_index", i},
                           {"name",        o->GetNameText()},
                           {"target",      OrderTargetTypes(o)},
                           {"gold_cost",   o->GetGold()} });
    }
    json result;
    result["army"]   = idx;
    result["orders"] = orders;
    return Ok("query_unit_orders", result);
}

// do_unit_order <army_index> <order_index> [x y] — execute one order from
// query_unit_orders. With x,y the order acts on that tile (the army must be ON
// it or ADJACENT — move there first); without, it acts in place. The engine's
// own TestOrderHere is the gate, so an illegal order is REPORTED, never crashes.
std::string CmdDoUnitOrder(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("do_unit_order", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("do_unit_order", "no_human_player");

    int idx = -1, oidx = -1, x = -1, y = -1;
    int const n = sscanf(args, "%d %d %d %d", &idx, &oidx, &x, &y);
    if (n < 2)
        return Err("do_unit_order", "bad_args");
    DynamicArray<Army> * armies = human->GetAllArmiesList();
    if (!armies || idx < 0 || idx >= armies->Num())
        return Err("do_unit_order", "bad_army_index");
    Army army = armies->Access(idx);
    ArmyData * ad = army.AccessData();
    if (!ad)
        return Err("do_unit_order", "invalid_army");
    if (!g_theOrderDB || oidx < 0 || oidx >= g_theOrderDB->NumRecords())
        return Err("do_unit_order", "bad_order_index");
    const OrderRecord * o = g_theOrderDB->Get(oidx);
    if (!o)
        return Err("do_unit_order", "bad_order_index");

    MapPoint here = ad->RetPos();
    bool const haveTarget = (n >= 4);
    MapPoint target = haveTarget ? MapPoint((sint16)x, (sint16)y) : here;
    if (haveTarget) {
        World * w = world_Get();
        if (!w || x < 0 || x >= w->GetXWidth() || y < 0 || y >= w->GetYHeight())
            return Err("do_unit_order", "bad_position");
        // Like bombard: the army must already be on or next to the target.
        if (target != here && !here.IsNextTo(target))
            return Err("do_unit_order", "not_adjacent");
    }

    // Engine pretest — translate the verdict instead of executing on failure.
    ORDER_TEST t = ad->TestOrderHere(o, target);
    if (t != ORDER_TEST_OK) {
        const char * why = "order_rejected";
        switch (t) {
            case ORDER_TEST_ILLEGAL:        why = "illegal_for_this_unit"; break;
            case ORDER_TEST_LACKS_GOLD:     why = "lacks_gold";            break;
            case ORDER_TEST_NEEDS_TARGET:   why = "needs_target";          break;
            case ORDER_TEST_INVALID_TARGET: why = "invalid_target";        break;
            case ORDER_TEST_NO_MOVEMENT:    why = "no_moves_left";         break;
            default: break;
        }
        return Err("do_unit_order", why);
    }

    if (haveTarget && target != here) {
        Path p;
        p.SetStart(here);
        p.AddDir(here.GetNeighborDirection(target));
        ad->PerformOrderHere(o, &p);
    } else {
        ad->PerformOrder(o);  // acts on the army's own tile
    }
    if (gevmanager_Get()) gevmanager_Get()->Process();

    json result;
    result["army"]        = idx;
    result["order_index"] = oidx;
    result["order_name"]  = o->GetNameText();
    if (haveTarget) result["target"] = { {"x", x}, {"y", y} };
    gc_log->info("do_unit_order: army {} order {} ({})", idx, oidx, o->GetNameText());
    return Ok("do_unit_order", result);
}

// upgrade_unit <army_index> — modernise every upgradable unit in the army to
// its current-tech equivalent (spends gold). Reports how many were upgraded.
std::string CmdUpgradeUnit(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("upgrade_unit", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("upgrade_unit", "no_human_player");
    int idx = -1;
    if (sscanf(args, "%d", &idx) != 1)
        return Err("upgrade_unit", "bad_args");
    DynamicArray<Army> * armies = human->GetAllArmiesList();
    if (!armies || idx < 0 || idx >= armies->Num())
        return Err("upgrade_unit", "bad_army_index");
    ArmyData * ad = armies->Access(idx).AccessData();
    if (!ad)
        return Err("upgrade_unit", "invalid_army");

    // Count upgradable units up front so we can report (Upgrade() returns only
    // whether anything happened).
    sint32 upgradable = 0, type = 0, costs = 0;
    for (sint32 i = 0; i < ad->Num(); ++i) {
        Unit u = ad->Access(i);
        if (u.GetData() && u.GetData()->CanUpgrade(type, costs)) ++upgradable;
    }
    if (upgradable == 0)
        return Err("upgrade_unit", "nothing_to_upgrade");

    bool const ok = ad->Upgrade();
    if (gevmanager_Get()) gevmanager_Get()->Process();

    json result;
    result["army"]              = idx;
    result["units_upgradable"]  = upgradable;
    result["upgraded"]          = ok;
    gc_log->info("upgrade_unit: army {} upgradable {} ok {}", idx, upgradable, ok);
    return Ok("upgrade_unit", result);
}

// propose <target_player> <proposal_type> [arg] — generalised diplomacy. Sends
// any single-clause proposal the engine supports and reports the AI verdict.
// proposal_type is a PROPOSAL_TYPE id (e.g. 17 OFFER_GIVE_ADVANCE, 19 OFFER_
// GIVE_GOLD, 32 TREATY_CEASEFIRE, 33 TREATY_PEACE, 38 TREATY_ALLIANCE). [arg]
// is the advance id for GIVE/REQUEST_ADVANCE, or gold amount for GIVE/REQUEST_GOLD.
std::string CmdPropose(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("propose", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("propose", "no_human_player");

    int target = -1, ptype = -1, arg = -1;
    int const n = sscanf(args, "%d %d %d", &target, &ptype, &arg);
    if (n < 2)
        return Err("propose", "bad_args");
    if (target < 0 || target >= k_MAX_PLAYERS || !player_Get(target))
        return Err("propose", "bad_player");
    if (target == human->GetOwner())
        return Err("propose", "thats_you");
    if (!human->HasContactWith(target))
        return Err("propose", "no_contact");
    if (ptype <= PROPOSAL_NONE || ptype >= PROPOSAL_MAX)
        return Err("propose", "bad_proposal_type");

    PROPOSAL_TYPE pt = static_cast<PROPOSAL_TYPE>(ptype);
    NewProposal proposal;
    proposal.senderId          = human->GetOwner();
    proposal.receiverId        = target;
    proposal.priority          = 1;
    proposal.detail.first_type = pt;
    proposal.detail.tone       = DIPLOMATIC_TONE_EQUAL;
    // Wire the clause argument to the right slot.
    if (pt == PROPOSAL_OFFER_GIVE_ADVANCE || pt == PROPOSAL_REQUEST_GIVE_ADVANCE) {
        if (n < 3 || !g_theAdvanceDB || arg < 0 || arg >= g_theAdvanceDB->NumRecords())
            return Err("propose", "bad_advance_arg");
        proposal.detail.first_arg.advanceType = arg;
    } else if (pt == PROPOSAL_OFFER_GIVE_GOLD || pt == PROPOSAL_REQUEST_GIVE_GOLD) {
        if (n < 3 || arg < 0)
            return Err("propose", "bad_gold_arg");
        proposal.detail.first_arg.gold = arg;
    }
    Diplomat::GetDiplomat(human->GetOwner()).ExecuteNewProposal(proposal);
    if (gevmanager_Get()) gevmanager_Get()->Process();

    json result;
    result["target"]        = target;
    result["proposal_type"] = ptype;
    result["at_war"]        = human->HasWarWith(target);
    // For treaty clauses, an accepted deal shows up in the agreement matrix.
    if (pt == PROPOSAL_TREATY_PEACE || pt == PROPOSAL_TREATY_CEASEFIRE ||
        pt == PROPOSAL_TREATY_ALLIANCE) {
        result["agreement"] = AgreementMatrix::s_agreements.HasAgreement(
                                  human->GetOwner(), target, pt);
    }
    gc_log->info("propose: {} -> {} type {} arg {}", (int)human->GetOwner(), target, ptype, arg);
    return Ok("propose", result);
}

// sell_building <city_index> <building_type> — sell a built improvement for
// gold (once per city per turn). building_type from query_city.buildings_built.
std::string CmdSellBuilding(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("sell_building", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("sell_building", "no_human_player");
    int city_idx = -1, btype = -1;
    if (sscanf(args, "%d %d", &city_idx, &btype) != 2)
        return Err("sell_building", "bad_args");
    UnitDynamicArray * mine = human->GetAllCitiesList();
    if (!mine || city_idx < 0 || city_idx >= mine->Num())
        return Err("sell_building", "bad_city_index");
    CityData * cd = mine->Access(city_idx).GetData() ? mine->Access(city_idx).GetData()->GetCityData() : nullptr;
    if (!cd)
        return Err("sell_building", "invalid_city");
    if (!g_theBuildingDB || btype < 0 || btype >= g_theBuildingDB->NumRecords())
        return Err("sell_building", "bad_building");
    if (!cd->HasBuilding(btype))
        return Err("sell_building", "building_not_present");
    if (cd->SellingBuilding() >= 0)
        return Err("sell_building", "already_sold_this_turn");

    cd->SellBuilding(btype);
    if (gevmanager_Get()) gevmanager_Get()->Process();
    const BuildingRecord * rec = g_theBuildingDB->Get(btype);
    json result;
    result["city"]      = city_idx;
    result["building"]  = btype;
    result["name"]      = rec ? ToUtf8(rec->GetNameText()) : "";
    result["gold"]      = human->GetGold();
    gc_log->info("sell_building: city {} building {}", city_idx, btype);
    return Ok("sell_building", result);
}

// query_trade_routes — your outgoing trade routes, per source city, with an
// index usable by cancel_trade_route. (Routes originate at a source city's
// trade-source list.)
std::string QueryTradeRoutes()
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_trade_routes", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("query_trade_routes", "no_human_player");
    UnitDynamicArray * mine = human->GetAllCitiesList();
    json routes = json::array();
    for (sint32 c = 0; mine && c < mine->Num(); ++c) {
        CityData * cd = mine->Access(c).GetData() ? mine->Access(c).GetData()->GetCityData() : nullptr;
        if (!cd) continue;
        TradeDynamicArray * src = cd->GetTradeSourceList();
        for (sint32 r = 0; src && r < src->Num(); ++r) {
            TradeRoute route = src->Access(r);
            if (!route.IsValid()) continue;
            Unit dest = route.GetDestination();
            routes.push_back({ {"source_city",  c},
                               {"route_index",  r},
                               {"dest_city",    dest.IsValid() ? ToUtf8(dest.GetName()) : ""} });
        }
    }
    json result;
    result["routes"] = routes;
    return Ok("query_trade_routes", result);
}

// cancel_trade_route <city_index> <route_index> — cancel one of your outgoing
// routes (indices from query_trade_routes).
std::string CmdCancelTradeRoute(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("cancel_trade_route", "game_not_loaded");
    Player * human = HumanPlayer();
    if (!human)
        return Err("cancel_trade_route", "no_human_player");
    int city_idx = -1, route_idx = -1;
    if (sscanf(args, "%d %d", &city_idx, &route_idx) != 2)
        return Err("cancel_trade_route", "bad_args");
    UnitDynamicArray * mine = human->GetAllCitiesList();
    if (!mine || city_idx < 0 || city_idx >= mine->Num())
        return Err("cancel_trade_route", "bad_city_index");
    CityData * cd = mine->Access(city_idx).GetData() ? mine->Access(city_idx).GetData()->GetCityData() : nullptr;
    if (!cd)
        return Err("cancel_trade_route", "invalid_city");
    TradeDynamicArray * src = cd->GetTradeSourceList();
    if (!src || route_idx < 0 || route_idx >= src->Num())
        return Err("cancel_trade_route", "bad_route_index");
    TradeRoute route = src->Access(route_idx);
    if (!route.IsValid())
        return Err("cancel_trade_route", "invalid_route");

    human->RemoveTradeRoute(route, CAUSE_KILL_TRADE_ROUTE_RESET);
    if (gevmanager_Get()) gevmanager_Get()->Process();
    json result;
    result["source_city"] = city_idx;
    result["route_index"] = route_idx;
    gc_log->info("cancel_trade_route: city {} route {}", city_idx, route_idx);
    return Ok("cancel_trade_route", result);
}

// query_map — terrain, fog state and city markers for every tile the human has
// explored. Only explored tiles are listed (unexplored tiles are omitted
// entirely); the "visible" flag distinguishes currently-seen tiles from
// remembered ones. Units belong to query_units; this stays terrain+cities.
std::string QueryMap()
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_map", "game_not_loaded");

    Player * human = HumanPlayer();
    if (!human)
        return Err("query_map", "no_human_player");
    World * w = world_Get();
    if (!w)
        return Err("query_map", "no_world");
    Vision * vis = human->m_vision;

    const sint32 W = w->GetXWidth();
    const sint32 H = w->GetYHeight();
    json tiles = json::array();
    sint32 n_explored = 0, n_visible = 0;

    for (sint32 y = 0; y < H; ++y) {
        for (sint32 x = 0; x < W; ++x) {
            MapPoint pos(x, y);
            if (!(vis && vis->IsExplored(pos))) continue;
            ++n_explored;
            bool visible = vis->IsVisible(pos);
            if (visible) ++n_visible;

            Cell * c = w->GetCell(pos);
            json t;
            t["x"]       = x;
            t["y"]       = y;
            t["terrain"] = c ? c->GetTerrain() : -1;
            t["visible"] = visible;
            if (c) {
                Unit city = c->GetCity();
                if (city.IsValid())
                    t["city"] = (sint32)city.GetOwner();
                // Tile improvements actually on the ground (farms/mines/roads/
                // structures) — so the map can SHOW infrastructure, not just
                // terrain.  Emitted only when present to keep the payload lean.
                sint32 const nimp = c->GetNumDBImprovements();
                if (nimp > 0 && g_theTerrainImprovementDB) {
                    json imps = json::array();
                    for (sint32 k = 0; k < nimp; ++k) {
                        sint32 const dbi = c->GetDBImprovement(k);
                        const TerrainImprovementRecord * irec =
                            (dbi >= 0 && dbi < g_theTerrainImprovementDB->NumRecords())
                                ? g_theTerrainImprovementDB->Get(dbi) : nullptr;
                        imps.push_back({ {"type",  dbi},
                                         {"class", TerrainImpClass(irec)},
                                         {"name",  irec ? ToUtf8(irec->GetNameText()) : ""} });
                    }
                    t["improvements"] = imps;
                }
            }
            tiles.push_back(t);
        }
    }

    json result;
    result["visible_player"] = human->GetOwner();
    result["width"]    = W;
    result["height"]   = H;
    result["explored"] = n_explored;
    result["visible"]  = n_visible;
    result["tiles"]    = tiles;
    return Ok("query_map", result);
}

// query_world — UNFOGGED full-map snapshot for visualization / timelapse.
// Unlike query_map (clipped to the human's fog-of-war), this sees the whole
// board, so an AI player's empire can be rendered exactly as it stands. The
// terrain is a flat row-major array of terrain ids (length width*height) to
// keep the per-turn payload compact; cities and per-player score/city-count
// follow. Pairs with a per-turn recorder to build an empire timelapse.
std::string QueryWorld()
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_world", "game_not_loaded");
    World * w = world_Get();
    if (!w)
        return Err("query_world", "no_world");

    const sint32 W = w->GetXWidth();
    const sint32 H = w->GetYHeight();

    json terrain = json::array();
    for (sint32 y = 0; y < H; ++y) {
        for (sint32 x = 0; x < W; ++x) {
            Cell * c = w->GetCell(MapPoint(x, y));
            terrain.push_back(c ? c->GetTerrain() : -1);
        }
    }

    json cities  = json::array();
    json players = json::array();
    for (sint32 p = 0; p < k_MAX_PLAYERS; ++p) {
        Player * pl = player_Get(p);
        if (!pl) continue;

        sint32 ncity = 0;
        UnitDynamicArray * list = pl->GetAllCitiesList();
        for (sint32 i = 0; list && i < list->Num(); ++i) {
            Unit u = list->Access(i);
            if (!u.IsValid()) continue;
            ++ncity;
            MapPoint pos;
            u.GetPos(pos);
            CityData * cd = u.GetData() ? u.GetData()->GetCityData() : nullptr;
            cities.push_back({ {"owner", p}, {"x", pos.x}, {"y", pos.y},
                               {"pop", cd ? cd->PopCount() : 0} });
        }

        std::string leader = ToUtf8(pl->GetLeaderName());
        MBCHAR civname[k_MAX_NAME_LEN] = {0};
        Civilisation * civ = pl->GetCivilisation();
        if (civ && civ->AccessData())
            civ->GetSingularCivName(civname);

        players.push_back({ {"id", p},
                            {"cities", ncity},
                            {"dead", pl->IsDead() ? true : false},
                            {"name", leader},
                            {"civ", ToUtf8(civname)},
                            {"score", pl->m_score ? pl->m_score->GetTotalScore() : 0} });
    }

    json result;
    result["width"]   = W;
    result["height"]  = H;
    result["terrain"] = terrain;
    result["cities"]  = cities;
    result["players"] = players;
    return Ok("query_world", result);
}

// ---- admin queries --------------------------------------------------------
//
// Unlike the player-view queries above, these are OMNISCIENT: they report the
// whole game state with no fog-of-war filtering. They exist for the gateway's
// admin panel and debugging — a driver that wants the player's perspective
// must use the query_* family instead.

// One player slot, shared by query_players and query_player so the two can
// never drift apart.
json PlayerJson(sint32 p, Player * pl)
{
    std::string name = ToUtf8(pl->GetLeaderName());
    // Civilisation names come from the game's StringDB via the player's
    // Civilisation object — never synthesized here.
    MBCHAR civ[k_MAX_NAME_LEN]     = {0};
    MBCHAR country[k_MAX_NAME_LEN] = {0};
    Civilisation * c = pl->GetCivilisation();
    if (c && c->AccessData()) {
        c->GetSingularCivName(civ);
        c->GetCountryName(country);
    }
    json j;
    j["id"]         = p;
    j["name"]       = name;
    j["civ"]        = ToUtf8(civ);       // adjective/singular, e.g. "Roman"
    j["country"]    = ToUtf8(country);   // nation, e.g. "Rome"
    j["human"]      = pl->IsHuman();
    j["dead"]       = pl->IsDead();
    j["gold"]       = pl->GetGold();
    // Victory state is needed by long-running autoplay loops so they can stop
    // when the game is decided without polling every city every turn.
    // Coarse victory state lets long-running autoplay loops stop when the game
    // is decided without polling every city every turn.
    sint32 vtype = pl->m_score ? pl->m_score->GetPartialScore(SCORE_CAT_TYPE_OF_VICTORY) : kScoreGameInProgress;
    j["has_won_the_game"] = pl->m_hasWonTheGame != FALSE;
    j["victory_type"]     = vtype;
    j["victory_label"]    = game_controller::VictoryTypeLabel(vtype);
    // Material tax is only meaningful/changeable for the human, but exposing it
    // on every row keeps PlayerJson uniform and lets the autoplay loop verify
    // the economy invariant with a single query_players call.
    j["material_tax"]     = pl->m_materialsTax;
    j["num_cities"] = pl->GetNumCities();
    j["num_units"]  = pl->m_all_units ? pl->m_all_units->Num() : 0;
    j["num_armies"] = pl->GetAllArmiesList() ? pl->GetAllArmiesList()->Num() : 0;
    j["government"] = pl->GetGovernmentType();
    j["score"]      = pl->m_score ? pl->m_score->GetTotalScore() : 0;
    // Economic rate dials the UI exposes but the API long ignored: the
    // science<->gold commerce split and the workday/wages/rations social
    // sliders. `expectation` is the government's neutral level for each slider
    // (deviating trades output for happiness); max_science_rate is the
    // government's science-split cap. Feed set_science_rate / set_rates.
    {
        double sci = 0.0;
        pl->GetScienceTaxRate(sci);
        json econ;
        econ["science_rate"]     = sci;            // 0..1 fraction of commerce -> science
        econ["gold_rate"]        = 1.0 - sci;
        const GovernmentRecord * grec = g_theGovernmentDB ?
            g_theGovernmentDB->Get(pl->GetGovernmentType()) : nullptr;
        econ["max_science_rate"] = grec ? grec->GetMaxScienceRate() : 1.0;
        // Current slider levels live in the happiness object (Player::Get*Level
        // is unimplemented for workday/wages); expectations come from the gov.
        PlayerHappiness * h = pl->m_global_happiness;
        econ["workday"]  = { {"level", h ? h->GetUnitlessWorkday() : 0}, {"expectation", pl->GetWorkdayExpectation()} };
        econ["wages"]    = { {"level", h ? h->GetUnitlessWages()   : 0}, {"expectation", pl->GetWagesExpectation()} };
        econ["rations"]  = { {"level", h ? h->GetUnitlessRations() : 0}, {"expectation", pl->GetRationsExpectation()} };
        // Military readiness footing (peace/alert/war): raises unit ready-HP at a
        // per-turn gold cost. Feed set_readiness.
        {
            sint32 const rl = pl->GetReadinessLevel();
            char const * label = (rl == 0) ? "peace" : (rl == 1) ? "alert" : (rl == 2) ? "war" : "unknown";
            econ["readiness"] = { {"level", rl}, {"label", label}, {"cost", pl->GetReadinessCost()} };
        }
        j["economy"] = econ;
    }
    // Diplomacy relative to the HUMAN player (null for the human's own row).
    if (Player * human = HumanPlayer(); human && human->GetOwner() != p) {
        j["contact"] = human->HasContactWith(p);
        j["at_war"]  = human->HasWarWith(p);
    }
    if (pl->m_score) {
        // The components the score screen shows — RANK is RELATIVE, so a
        // player's score can fall while every absolute number improves.
        json sc;
        sc["feats"]      = pl->m_score->GetPartialScore(SCORE_CAT_FEATS);
        sc["advances"]   = pl->m_score->GetPartialScore(SCORE_CAT_ADVANCES);
        sc["wonders"]    = pl->m_score->GetPartialScore(SCORE_CAT_WONDERS);
        sc["population"] = pl->m_score->GetPartialScore(SCORE_CAT_POPULATION);
        sc["cities"]     = pl->m_score->GetPartialScore(SCORE_CAT_CITIES0TO30)
                         + pl->m_score->GetPartialScore(SCORE_CAT_CITIES30TO100)
                         + pl->m_score->GetPartialScore(SCORE_CAT_CITIES100TO500)
                         + pl->m_score->GetPartialScore(SCORE_CAT_CITIES500PLUS);
        sc["conquest"]   = pl->m_score->GetPartialScore(SCORE_CAT_OPPONENTS_CONQUERED)
                         + pl->m_score->GetPartialScore(SCORE_CAT_CITIES_RECAPTURED);
        sc["rank"]       = pl->m_score->GetPartialScore(SCORE_CAT_RANK);
        j["score_breakdown"] = sc;

        // Rank demystified: SCORE_CAT_RANK is 100 * yours / everyone's
        // EMPIRE STRENGTH (Score::GetPlayerStrength), where strength =
        // units + gold + buildings + wonders + production. Equal-pop
        // players can differ 2x on rank (campaign 7: 640 vs 1340) --
        // these components say WHY. Telemetry only; strategy decides
        // what (if anything) to do about it.
        if (pl->m_strengths) {
            json st;
            st["units"]      = pl->m_strengths->GetStrength(STRENGTH_CAT_UNITS);
            st["gold"]       = pl->m_strengths->GetStrength(STRENGTH_CAT_GOLD);
            st["buildings"]  = pl->m_strengths->GetStrength(STRENGTH_CAT_BUILDINGS);
            st["wonders"]    = pl->m_strengths->GetStrength(STRENGTH_CAT_WONDERS);
            st["production"] = pl->m_strengths->GetStrength(STRENGTH_CAT_PRODUCTION);
            st["military"]   = pl->m_strengths->GetStrength(STRENGTH_CAT_MILITARY);
            st["knowledge"]  = pl->m_strengths->GetStrength(STRENGTH_CAT_KNOWLEDGE);
            j["strength"] = st;
        }
    }
    return j;
}

// query_players — every live player slot with headline stats.
std::string QueryPlayers()
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_players", "game_not_loaded");

    json players = json::array();
    for (sint32 p = 0; p < k_MAX_PLAYERS; ++p) {
        Player * pl = player_Get(p);
        if (!pl) continue;
        players.push_back(PlayerJson(p, pl));
    }

    json result;
    result["players"] = players;
    return Ok("query_players", result);
}

// query_player <id> — one player slot (same shape as a query_players entry).
std::string QueryPlayer(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_player", "game_not_loaded");

    int id = -1;
    if (sscanf(args, "%d", &id) != 1)
        return Err("query_player", "bad_args");
    if (id < 0 || id >= k_MAX_PLAYERS || !player_Get(id))
        return Err("query_player", "bad_player");

    return Ok("query_player", PlayerJson(id, player_Get(id)));
}

// query_turn — where the clock stands. Session-level accessors on purpose:
// the legacy GetRound/GetYear route through the currently-viewing PLAYER's
// recorded round, which lags the global clock between rounds — exactly the
// idle window in which this query runs.
std::string QueryTurn()
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_turn", "game_not_loaded");

    json result;
    // GetYear() == NewTurnCount::GetCurrentYear(), derived from the LIVE round.
    // GetSessionYear() returns TurnCount::m_year, which is only seeded to
    // 4000 BC at game start and refreshed solely on the serve/load SetRound
    // path — so a plain end_turn loop (autoplay, headless) leaves it frozen at
    // 4000 BC even at round 300. Same bug class as the UI fix in a83a851f.
    sint32 const year = turn_Get() ? turn_Get()->GetYear() : 0;
    result["round"] = turn_Get() ? turn_Get()->GetSessionRound() : 0;
    result["year"]  = year;
    // The game's scored deadline: at end_of_game_year (default 2300 AD) the
    // "out of time" end-game fires and the highest score wins.  Headless does
    // NOT stop at the deadline — it keeps running in UNSCORED overtime, which
    // is why no victory flag appears past it.  Surface the deadline so a driver
    // knows whether the game is still live or already decided on score.
    const ConstRecord * cr = g_theConstDB ? g_theConstDB->Get(0) : nullptr;
    if (cr) {
        sint32 const end_year  = cr->GetEndOfGameYear();
        sint32 const warn_year = cr->GetEndOfGameYearEarlyWarning();
        result["end_of_game_year"]         = end_year;
        result["end_of_game_warning_year"] = warn_year;
        result["past_deadline"]            = year >= end_year;  // true => unscored overtime
    }
    return Ok("query_turn", result);
}

// query_player_cities <player_id> — ALL cities of one player (no fog filter).
std::string QueryPlayerCities(const char * args)
{
    if (!civapp_Get() || !civapp_Get()->IsGameLoaded())
        return Err("query_player_cities", "game_not_loaded");

    int id = -1;
    if (sscanf(args, "%d", &id) != 1)
        return Err("query_player_cities", "bad_args");
    if (id < 0 || id >= k_MAX_PLAYERS || !player_Get(id))
        return Err("query_player_cities", "bad_player");

    json cities = json::array();
    UnitDynamicArray * list = player_Get(id)->GetAllCitiesList();
    for (sint32 i = 0; list && i < list->Num(); ++i) {
        Unit u = list->Access(i);
        if (!u.IsValid()) continue;
        cities.push_back(CityJson(id, i, u));
    }

    json result;
    result["owner"]  = id;
    result["cities"] = cities;
    return Ok("query_player_cities", result);
}

// query_terrains — static dictionary mapping terrain ids (as reported by
// query_map) to names, passability and base tile yields. Values come from
// the game's TerrainRecord DB; fetch once and cache client-side.
std::string QueryTerrains()
{
    if (!g_theTerrainDB)
        return Err("query_terrains", "no_terrain_db");

    json list = json::array();
    for (sint32 i = 0; i < g_theTerrainDB->NumRecords(); ++i) {
        const TerrainRecord * t = g_theTerrainDB->Get(i);
        if (!t) continue;
        const TerrainRecord::Modifiers * m = t->GetEnvBase();
        sint32 movement = 0;
        if (m) m->GetMovement(movement);
        json j;
        j["id"]       = i;
        j["name"]     = ToUtf8(t->GetNameText());
        j["internal"] = ToUtf8(t->GetIDText());
        j["land"]     = t->GetMovementTypeLand();
        j["water"]    = t->GetMovementTypeSea() || t->GetMovementTypeShallowWater();
        j["mountain"] = t->GetMovementTypeMountain();
        j["food"]     = m ? m->GetFood()   : 0;
        j["shields"]  = m ? m->GetShield() : 0;
        j["gold"]     = m ? m->GetGold()   : 0;
        j["movement"] = movement;  // cost in 1/100 MP; 0 = record default
        list.push_back(j);
    }

    json result;
    result["terrains"] = list;
    return Ok("query_terrains", result);
}

// query_names — compact static/dynamic dictionaries for offline tooling.  The
// timelapse recorder stores this once in the meta row so Chronicle captions can
// resolve action-log ids without querying the running game during rendering.
std::string QueryNames()
{
    json result;

    json players = json::object();
    for (sint32 p = 0; p < k_MAX_PLAYERS; ++p) {
        Player * pl = player_Get(p);
        if (!pl) continue;
        MBCHAR civ[k_MAX_NAME_LEN]     = {0};
        MBCHAR country[k_MAX_NAME_LEN] = {0};
        Civilisation * c = pl->GetCivilisation();
        if (c && c->AccessData()) {
            c->GetSingularCivName(civ);
            c->GetCountryName(country);
        }
        players[std::to_string(p)] = {
            {"leader", ToUtf8(pl->GetLeaderName())},
            {"civ", ToUtf8(civ)},
            {"country", ToUtf8(country)},
        };
    }
    result["players"] = players;

    auto named_records = [](auto * db) {
        json names = json::object();
        if (!db) return names;
        for (sint32 i = 0; i < db->NumRecords(); ++i) {
            auto const * rec = db->Get(i);
            if (rec)
                names[std::to_string(i)] = ToUtf8(rec->GetNameText());
        }
        return names;
    };

    result["advances"]             = named_records(g_theAdvanceDB);
    result["buildings"]            = named_records(g_theBuildingDB);
    result["wonders"]              = named_records(g_theWonderDB);
    result["units"]                = named_records(g_theUnitDB);
    result["terrain_improvements"] = named_records(g_theTerrainImprovementDB);

    return Ok("query_names", result);
}

std::string QueryGpuWorld()
{
    json result;
    result["enabled"] = aui_SDL::GpuQuadsEnabled();
    // Reported separately from "enabled": the whole-map target is opt-in and
    // implies quads, so a parity test that only checked "enabled" could not
    // tell the P13 path from the ADR-002 one and would silently compare a
    // path against itself.
    result["worldmap"] = aui_SDL::GpuWorldmapEnabled();
    // The flag alone does NOT mean the frame came from the whole-map target:
    // the present falls back to the ADR-002 window mirror whenever the texture
    // is absent (aui_sdlsurface.cpp, "Falls back ... if the target is not
    // ready"). A parity test that checks only the flag passes that fallback as
    // if it had measured the P13 path.
    result["worldmap_texture"] = aui_SDL::WorldmapTexture() != nullptr;
    result["complete"] = aui_SDL::QuadFrameComplete();
    char const *reason = aui_SDL::QuadFrameIncompleteReason();
    result["fallback_reason"] = reason ? reason : "";
    { result["goods"] = { {"cells", s_goodCellsSeen}, {"no_actor", s_goodNoActor},
                          {"declined", s_goodDeclined}, {"emitted", s_goodEmitted},
                          {"reason", s_goodReason} }; }   // PROBE
    result["terrain_quads"] = aui_SDL::QuadDrawList().size();
    result["sprite_quads"] = aui_SDL::SpriteDrawList().size();
    // The exact inputs to the present's source rect. Reported so a parity run
    // can compare the two paths' geometry directly instead of inferring it by
    // correlating presented pixels — tile art is periodic, so a correlation
    // peak can sit a whole tile off and still look convincing.
    result["worldmap_origin"] = { aui_SDL::WorldmapOriginX(), aui_SDL::WorldmapOriginY() };
    // Terrain, sprites and picking must all agree on where the view's top-left
    // sits in the whole-map texture. Picking inverts through the origin; sprites
    // are placed through the sprite base. Any difference between these two is
    // exactly the "click the unit, select its neighbour" error, in texture
    // pixels, so report it rather than leaving it to be inferred from pixels.
    result["worldmap_sprite_base"] = { aui_SDL::WorldmapSpriteBaseX(), aui_SDL::WorldmapSpriteBaseY() };
    result["worldmap_size"] = { aui_SDL::WorldmapW(), aui_SDL::WorldmapH() };
    result["world_content_off"] = { aui_SDL::WorldContentOffX(), aui_SDL::WorldContentOffY() };
    result["camera"] = { {"zoom", aui_SDL::CameraZoom()},
                         {"off_x", aui_SDL::CameraOffX()},
                         {"off_y", aui_SDL::CameraOffY()} };
    return Ok("query_gpu_world", result);
}

}  // namespace

namespace game_controller {

std::string Dispatch(const std::string & line, bool & handled)
{
    handled = true;

    if (line == "build_city")                                  return CmdBuildCity();
    if (line.rfind("end_turn", 0) == 0)                         return CmdEndTurn(line.c_str() + 8);
    if (line.rfind("set_show_city_names ", 0) == 0)             return CmdSetShowCityNames(line.c_str() + 20);
    if (line.rfind("debug_terrain_overlay ", 0) == 0)           return CmdDebugTerrainOverlay(line.c_str() + 22);
    if (line.rfind("debug_set_terrain ", 0) == 0)               return CmdDebugSetTerrain(line.c_str() + 18);
    if (line.rfind("debug_clear_rivers ", 0) == 0)              return CmdDebugClearRivers(line.c_str() + 19);
    if (line.rfind("debug_clear_terrain_layers ", 0) == 0)      return CmdDebugClearTerrainLayers(line.c_str() + 27);
    if (line.rfind("debug_reveal_patch ", 0) == 0)              return CmdDebugRevealPatch(line.c_str() + 19);
    if (line.rfind("debug_explore_patch ", 0) == 0)             return CmdDebugExplorePatch(line.c_str() + 20);
    if (line.rfind("debug_vision_stats ", 0) == 0)              return CmdDebugVisionStats(line.c_str() + 19);
    if (line.rfind("debug_render_explored_as_visible ", 0) == 0) return CmdDebugRenderExploredAsVisible(line.c_str() + 33);
    if (line.rfind("debug_find_good ", 0) == 0)                 return CmdDebugFindGood(line.c_str() + 16);
    if (line.rfind("debug_worldmap_sprites ", 0) == 0)          return CmdDebugWorldmapSprites(line.c_str() + 23);
    if (line == "debug_icon_alpha")                             return CmdDebugIconAlpha(nullptr);
    if (line.rfind("debug_set_grid ", 0) == 0)                  return CmdDebugSetGrid(line.c_str() + 15);
    if (line.rfind("debug_set_borders ", 0) == 0)               return CmdDebugSetBorders(line.c_str() + 18);
#if defined(RENDER_TOOL_BUILD) && defined(USE_SDL)
    if (line == "debug_worldmap_build")                         return CmdDebugWorldmapBuild("");
    if (line.rfind("debug_worldmap_build ", 0) == 0)            return CmdDebugWorldmapBuild(line.c_str() + 21);
    if (line.rfind("debug_worldmap_pixel ", 0) == 0)            return CmdDebugWorldmapPixel(line.c_str() + 21);
    if (line == "debug_gpu_worldmap_probe")                     return CmdDebugGpuWorldmapProbe(nullptr);
    if (line.rfind("debug_gpu_worldmap_probe ", 0) == 0)        return CmdDebugGpuWorldmapProbe(line.c_str() + 25);
    if (line == "debug_tileset_stats")                          return CmdDebugTilesetStats(nullptr);
#endif
    if (line == "debug_deselect")                               return CmdDebugDeselect();
    if (line.rfind("debug_combat_flash ", 0) == 0)              return CmdDebugCombatFlash(line.c_str() + 19);
    if (line.rfind("debug_scenario_start_flags ", 0) == 0)      return CmdDebugScenarioStartFlags(line.c_str() + 27);
    if (line.rfind("debug_cloak_army ", 0) == 0)                return CmdDebugCloakArmy(line.c_str() + 17);
    if (line.rfind("debug_city_defense ", 0) == 0)              return CmdDebugCityDefense(line.c_str() + 19);
    if (line.rfind("debug_gallery_case ", 0) == 0)              return CmdDebugGalleryCase(line.c_str() + 19);
    if (line.rfind("set_zoom_level ", 0) == 0)                  return CmdSetZoomLevel(line.c_str() + 15);
    if (line.rfind("set_production ", 0) == 0)                  return CmdSetProduction(line.c_str() + 15);
    if (line.rfind("save_game ", 0) == 0)                       return CmdSaveGame(line.c_str() + 10);
    if (line.rfind("load_game ", 0) == 0)                       return CmdLoadGame(line.c_str() + 10);
    if (line == "log_get")                                      return CmdLogGet();
    if (line == "log_clear")                                    return CmdLogClear();
    if (line == "query_cities")                                 return QueryCities();
    if (line.rfind("query_city ", 0) == 0)                      return QueryCity(line.c_str() + 11);
    if (line == "query_city")                                   return QueryCity("");
    if (line == "query_units")                                  return QueryUnits();
    if (line == "query_armies")                                 return QueryArmies();
    if (line.rfind("move_army ", 0) == 0)                       return CmdMoveArmy(line.c_str() + 10);
    if (line.rfind("auto_explore ", 0) == 0)                    return CmdAutoExplore(line.c_str() + 13);
    if (line == "query_map")                                    return QueryMap();
    if (line == "query_world")                                  return QueryWorld();
    if (line == "query_players")                                return QueryPlayers();
    if (line.rfind("query_player_cities ", 0) == 0)             return QueryPlayerCities(line.c_str() + 20);
    if (line.rfind("query_player ", 0) == 0)                    return QueryPlayer(line.c_str() + 13);
    if (line == "query_turn")                                   return QueryTurn();
    if (line == "query_terrains")                               return QueryTerrains();
    if (line == "query_names")                                  return QueryNames();
    if (line == "query_gpu_world")                              return QueryGpuWorld();
    if (line == "query_research")                               return QueryResearch();
    if (line.rfind("set_research ", 0) == 0)                    return CmdSetResearch(line.c_str() + 13);
    if (line.rfind("query_terraform ", 0) == 0)                 return QueryTerraform(line.c_str() + 16);
    if (line.rfind("terraform ", 0) == 0)                       return CmdTerraform(line.c_str() + 10);
    if (line.rfind("set_material_tax ", 0) == 0)                return CmdSetMaterialTax(line.c_str() + 17);
    if (line.rfind("grant_advance ", 0) == 0)                   return CmdGrantAdvance(line.c_str() + 14);
    if (line.rfind("create_unit ", 0) == 0)                     return CmdCreateUnit(line.c_str() + 12);
    if (line.rfind("declare_war ", 0) == 0)                     return CmdDeclareWar(line.c_str() + 12);
    if (line.rfind("attack ", 0) == 0)                          return CmdAttack(line.c_str() + 7);
    if (line.rfind("bombard ", 0) == 0)                         return CmdBombard(line.c_str() + 8);
    if (line.rfind("buy_production ", 0) == 0)                  return CmdBuyProduction(line.c_str() + 15);
    if (line.rfind("propose_peace ", 0) == 0)                   return CmdProposePeace(line.c_str() + 14);
    if (line.rfind("group_army ", 0) == 0)                      return CmdGroupArmy(line.c_str() + 11);
    if (line.rfind("unload ", 0) == 0)                          return CmdUnload(line.c_str() + 7);
    if (line.rfind("board ", 0) == 0)                           return CmdBoard(line.c_str() + 6);
    if (line.rfind("fortify ", 0) == 0)                        return CmdFortify(line.c_str() + 8);
    if (line == "debug_crash") {
        // DEBUG: deliberate SIGSEGV to exercise the crash reporter
        // (backtrace + event ring in /tmp/ctp2-crash.log). Not an MCP tool.
        volatile int * boom = nullptr;
        *boom = 42;
    }
    if (line.rfind("ungroup_army ", 0) == 0)                    return CmdUngroupArmy(line.c_str() + 13);
    if (line.rfind("disband_unit ", 0) == 0)                     return CmdDisbandUnit(line.c_str() + 13);
    if (line.rfind("set_government ", 0) == 0)                   return CmdSetGovernment(line.c_str() + 15);
    if (line.rfind("establish_trade_route ", 0) == 0)            return CmdEstablishTradeRoute(line.c_str() + 22);
    if (line.rfind("set_science_rate ", 0) == 0)                 return CmdSetScienceRate(line.c_str() + 17);
    if (line.rfind("set_rates ", 0) == 0)                        return CmdSetRates(line.c_str() + 10);
    if (line.rfind("set_readiness ", 0) == 0)                    return CmdSetReadiness(line.c_str() + 14);
    if (line.rfind("set_specialist ", 0) == 0)                   return CmdSetSpecialist(line.c_str() + 15);
    if (line.rfind("set_governor ", 0) == 0)                     return CmdSetGovernor(line.c_str() + 13);
    if (line == "query_governor_profiles")                       return QueryGovernorProfiles();
    if (line.rfind("query_unit_orders ", 0) == 0)                return QueryUnitOrders(line.c_str() + 18);
    if (line.rfind("do_unit_order ", 0) == 0)                     return CmdDoUnitOrder(line.c_str() + 14);
    if (line.rfind("upgrade_unit ", 0) == 0)                      return CmdUpgradeUnit(line.c_str() + 13);
    if (line.rfind("propose ", 0) == 0)                           return CmdPropose(line.c_str() + 8);
    if (line.rfind("sell_building ", 0) == 0)                     return CmdSellBuilding(line.c_str() + 14);
    if (line == "query_trade_routes")                            return QueryTradeRoutes();
    if (line.rfind("cancel_trade_route ", 0) == 0)               return CmdCancelTradeRoute(line.c_str() + 19);

    handled = false;
    return std::string();
}

std::string DispatchSafe(const std::string & line, bool & handled)
{
    try {
        return Dispatch(line, handled);
    } catch (const std::exception & e) {
        gc_log->error("Dispatch threw on '{}': {}", line, e.what());
    } catch (...) {
        gc_log->error("Dispatch threw a non-std exception on '{}'", line);
    }
    handled = true;
    std::string verb = line.substr(0, line.find(' '));
    return Err(verb.c_str(), "exception");
}

}  // namespace game_controller
