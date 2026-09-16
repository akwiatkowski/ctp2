//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Action Log carrier + event-bus tap.  See action_log.h.
//
//----------------------------------------------------------------------------
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/fileio/action_log.h"

#include <array>

#include "gs/events/GameEventTypes.h"     // GAME_EVENT, GEA_*, GEAC_*, GEV_PRI_Post
#include "gs/events/GameEventManager.h"   // gevmanager_Get(), GetArgString, ArgCharToIndex
#include "gs/events/GameEventArgList.h"   // typed arg getters
#include "gs/events/GameEventHook.h"      // GameEventHookCallback, GEV_HD_Continue
#include "gs/gameobj/Events.h"            // STDEHANDLER
#include "gs/utility/TurnCnt.h"           // turn_Get()->GetSessionRound()
#include "gs/gameobj/Unit.h"              // Unit (cities are units), GetOwner
#include "gs/gameobj/Army.h"              // Army
#include "gs/world/MapPoint.h"            // MapPoint
#include "gs/gameobj/TerrImprove.h"       // TerrainImprovement
#include "gs/gameobj/TradeRoute.h"        // TradeRoute

//----------------------------------------------------------------------------
// Carrier — function-local-static store (no init-order issues, empty default).
//----------------------------------------------------------------------------

namespace
{
	nlohmann::json & store()
	{
		static nlohmann::json s_store = nlohmann::json::array();
		return s_store;
	}
}

namespace action_log
{
	void Append(const nlohmann::json & entry)
	{
		store().push_back(entry);
	}

	void Set(const nlohmann::json & array)
	{
		store() = array.is_array() ? array : nlohmann::json::array();
	}

	void Clear()
	{
		store() = nlohmann::json::array();
	}

	const nlohmann::json & Get()
	{
		return store();
	}

	std::size_t Count()
	{
		return store().size();
	}

	namespace
	{
		bool s_recording = true;
	}

	void SetRecording(bool on)
	{
		s_recording = on;
	}

	bool IsRecording()
	{
		return s_recording;
	}
}

//----------------------------------------------------------------------------
// The allowlist — Buckets 1 (orders) + 2 (world-changing outcomes) +
// 3-keep (real diplomacy), ~104 of 186 events.  Per-turn plumbing, UI clicks,
// AI deliberation, and low-level bookkeeping are deliberately excluded: they
// would produce hundreds of entries/turn with no decision signal.  Widen by
// adding a line here.  (See plans/ctp2-action-log.md for the full rationale.)
//----------------------------------------------------------------------------

namespace
{
	std::array const s_allowlist =
	{
		// --- Bucket 1: orders / intent (all *Order events) -----------------
		GEV_MoveOrder, GEV_MoveToOrder, GEV_MovePathOrder, GEV_SleepOrder,
		GEV_ExploreOrder, GEV_UnloadOrder, GEV_MoveUnloadOrder, GEV_EntrenchOrder,
		GEV_DetrenchOrder, GEV_DisbandArmyOrder, GEV_GroupOrder, GEV_GroupUnitOrder,
		GEV_UngroupOrder, GEV_ParadropOrder, GEV_InvestigateCityOrder,
		GEV_NullifyWallsOrder, GEV_StealTechnologyOrder, GEV_InciteRevolutionOrder,
		GEV_AssassinateRulerOrder, GEV_InvestigateReadinessOrder, GEV_BombardOrder,
		GEV_FranchiseOrder, GEV_SueOrder, GEV_SueFranchiseOrder, GEV_ExpelOrder,
		GEV_EstablishEmbassyOrder, GEV_AdvertiseOrder, GEV_PlantNukeOrder,
		GEV_SlaveRaidOrder, GEV_EnslaveSettlerOrder, GEV_UndergroundRailwayOrder,
		GEV_InciteUprisingOrder, GEV_BioInfectOrder, GEV_PlagueOrder,
		GEV_NanoInfectOrder, GEV_ConvertCityOrder, GEV_ReformCityOrder,
		GEV_SellIndulgencesOrder, GEV_SoothsayOrder, GEV_CloakOrder, GEV_UncloakOrder,
		GEV_RustleOrder, GEV_CreateParkOrder, GEV_CreateRiftOrder, GEV_PillageOrder,
		GEV_InjoinOrder, GEV_UseSpaceLadderOrder, GEV_AirliftOrder, GEV_DescendOrder,
		GEV_ThrowPartyOrder, GEV_PirateOrder, GEV_GetExpelledOrder, GEV_SettleOrder,
		GEV_BoardTransportOrder, GEV_LaunchOrder, GEV_TargetOrder, GEV_ClearTargetOrder,
		GEV_SettleInCityOrder, GEV_UpgradeOrder,

		// --- Bucket 2: world-changing outcomes (story beats) ---------------
		GEV_CreateCity, GEV_CaptureCity, GEV_KillCity, GEV_Settle, GEV_SettleInCity,
		GEV_GrantAdvance, GEV_BuildUnit, GEV_CreateUnit, GEV_MakePop, GEV_KillPop,
		GEV_Battle, GEV_BattleAftermath, GEV_KillUnit, GEV_CreateWonder,
		GEV_WonderRemoved, GEV_BuildWonder, GEV_AccomplishFeat, GEV_EnterAge,
		GEV_NukeCity, GEV_NukeCityUnit, GEV_NukeLocationUnit, GEV_KillPlayer,
		GEV_DisbandUnit, GEV_DisbandCity, GEV_UpgradeUnit, GEV_ConvertCity,
		GEV_UnconvertCity, GEV_GiveCity, GEV_EstablishEmbassy, GEV_GlobalWarming,
		GEV_OzoneDepletion, GEV_ImprovementComplete, GEV_CreateBuilding,
		GEV_BuildingRemoved, GEV_SellBuilding, GEV_BuyFront,
		GEV_CityRiot, GEV_EnslaveSettler,

		// --- Bucket 3-keep: real diplomacy (excludes AI deliberation) ------
		GEV_ContactMade, GEV_Accept, GEV_Reject, GEV_Counter, GEV_Threaten,
		GEV_NewProposal, GEV_GiveMap, GEV_BorderIncursion, GEV_NextDiplomaticState,
		GEV_ProposalResponse,
	};
}

//----------------------------------------------------------------------------
// Generic argument extraction.  Walks the event's signature string (e.g.
// CreateCity "%P%l%i%i&c") character by character.  Each slot is prefixed by
// '%' (input), '&' or '$' (output, filled during processing — visible because
// we tap at GEV_PRI_Post).  The kind char maps to a GEA_* enum via
// ArgCharToIndex; the per-kind typed getter is indexed by how many of that kind
// we have seen so far.  Defensive throughout: a missing/unknown arg yields a
// kind-only placeholder, never a throw.
//----------------------------------------------------------------------------

namespace
{
	nlohmann::json ExtractArgs(GAME_EVENT type, GameEventArgList * args)
	{
		nlohmann::json arr = nlohmann::json::array();
		if (!args)
			return arr;

		char const * sig = GameEventManager::GetArgString(type);
		if (!sig)
			return arr;

		// Running per-kind index (the typed getters address the Nth arg of
		// their own kind, not the Nth overall arg).
		std::array<sint32, GEA_End> kindSeen = { 0 };

		for (char const * p = sig; *p; ++p)
		{
			char const prefix = *p;
			if (prefix != '%' && prefix != '&' && prefix != '$')
				break;                       // malformed signature — stop
			++p;
			if (!*p)
				break;

			GAME_EVENT_ARGUMENT const kind = GameEventManager::ArgCharToIndex(*p);
			if (kind <= GEA_Null || kind >= GEA_End)
				continue;

			sint32 const idx = kindSeen[kind]++;
			bool   const out = (prefix == '&' || prefix == '$');

			nlohmann::json a;
			switch (kind)
			{
			case GEA_Player:
			{
				sint32 v = 0;
				if (args->GetPlayer(idx, v)) a = {{"kind", "player"}, {"value", v}};
				break;
			}
			case GEA_Int:
			{
				sint32 v = 0;
				if (args->GetInt(idx, v)) a = {{"kind", "int"}, {"value", v}};
				break;
			}
			case GEA_City:
			{
				Unit c;
				if (args->GetCity(idx, c)) a = {{"kind", "city"}, {"id", (uint32) c.m_id}};
				break;
			}
			case GEA_Unit:
			{
				Unit u;
				if (args->GetUnit(idx, u)) a = {{"kind", "unit"}, {"id", (uint32) u.m_id}};
				break;
			}
			case GEA_Army:
			{
				Army army;
				if (args->GetArmy(idx, army)) a = {{"kind", "army"}, {"id", (uint32) army.m_id}};
				break;
			}
			case GEA_MapPoint:
			{
				MapPoint pos;
				if (args->GetPos(idx, pos))
					a = {{"kind", "location"}, {"x", pos.x}, {"y", pos.y}, {"z", pos.z}};
				break;
			}
			case GEA_Direction:
			{
				WORLD_DIRECTION d{};
				if (args->GetDirection(idx, d)) a = {{"kind", "direction"}, {"value", (sint32) d}};
				break;
			}
			case GEA_Advance:
			{
				sint32 v = 0;
				if (args->GetAdvance(idx, v)) a = {{"kind", "advance"}, {"value", v}};
				break;
			}
			case GEA_Wonder:
			{
				sint32 v = 0;
				if (args->GetWonder(idx, v)) a = {{"kind", "wonder"}, {"value", v}};
				break;
			}
			case GEA_Improvement:
			{
				TerrainImprovement imp;
				if (args->GetImprovement(idx, imp))
					a = {{"kind", "improvement"}, {"id", (uint32) imp.m_id}};
				break;
			}
			case GEA_TradeRoute:
			{
				TradeRoute route;
				if (args->GetTradeRoute(idx, route))
					a = {{"kind", "trade_route"}, {"id", (uint32) route.m_id}};
				break;
			}
			case GEA_Path:
				a = {{"kind", "path"}};                 // verbose; identity only
				break;
			case GEA_Gold:
				a = {{"kind", "gold"}};                 // rare in allowlist; placeholder
				break;
			case GEA_Pop:
				a = {{"kind", "pop"}};                  // event firing is the signal
				break;
			default:
				break;
			}

			if (a.is_null())
				a = {{"kind", "unknown"}};

			if (out)
				a["out"] = true;

			arr.push_back(std::move(a));
		}

		return arr;
	}

	// Best-effort owning player for the top-level `player` field: the first
	// player arg, else the first city's owner, else the first unit's owner,
	// else -1 (unknown).
	sint32 DerivePlayer(GameEventArgList * args)
	{
		if (!args)
			return -1;

		sint32 p = 0;
		if (args->GetPlayer(0, p))
			return p;

		Unit c;
		if (args->GetCity(0, c) && c.IsValid())
			return c.GetOwner();

		Unit u;
		if (args->GetUnit(0, u) && u.IsValid())
			return u.GetOwner();

		return -1;
	}
}

//----------------------------------------------------------------------------
// The tap.  One generic handler registered at GEV_PRI_Post across the whole
// allowlist (registration *is* the gate — only allowlisted events reach here).
// Read-only: stamps an entry and returns GEV_HD_Continue, never perturbing
// state or RNG (the N-turn determinism test is the guard).
//----------------------------------------------------------------------------

STDEHANDLER(ActionLogTap)
{
	nlohmann::json entry;
	entry["turn"]   = turn_Get() ? turn_Get()->GetSessionRound() : -1;
	entry["player"] = DerivePlayer(args);
	entry["event"]  = GameEventManager::GetEventName(gameEventType);
	entry["args"]   = ExtractArgs(gameEventType, args);

	action_log::Append(entry);

	return GEV_HD_Continue;
}

void actionlog_tap_Initialize()
{
	if (!action_log::IsRecording())
		return;

	GameEventManager * gev = gevmanager_Get();
	if (!gev)
		return;

	for (GAME_EVENT ev : s_allowlist)
		gev->AddCallback(ev, GEV_PRI_Post, &s_ActionLogTap);
}

void actionlog_tap_Cleanup()
{
	GameEventManager * gev = gevmanager_Get();
	if (!gev)
		return;

	for (GAME_EVENT ev : s_allowlist)
		gev->RemoveCallback(ev, &s_ActionLogTap);
}
