//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Army event handlers
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
// - Do not generate an Assert popup when slaves revolt and take over a city.
// - Do not generate an Assert popup when an army is destroyed in an attack.
// - Added Elite and Leader Chance 6-4-2007
// - Replaced old const database by new one. (5-Aug-2007 Martin G�hmann)
// - The FinishMoveEvent event now fills a transporter up to the transport
//   capacy limit even if the army to be transported has more units than the
//   transporter space, the units that do not fit on board stay at land. (25-Jan-2008 Martin G�hmann)
// - Separated the Settle event drom the Settle in City event. (19-Feb-2008 Martin G�hmann)
// - Merged finish move. (13-Aug-2008 Martin G�hmann)
// - Added an upgrade order event. (13-Sep-2008 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/core/audio_observer.h"
#include "gs/gameobj/ArmyEvent.h"

#include "gs/gameobj/Events.h"
#include "gs/events/GameEventUser.h"
#include "gs/gameobj/Army.h"
#include "gs/gameobj/Order.h"
#include "gs/gameobj/ArmyData.h"
#include "gs/world/MapPoint.h"
#include "gs/world/World.h"
#include "gs/world/Cell.h"
#include "gs/gameobj/UnitData.h"
#include "gs/gameobj/ArmyData.h"
#include "WonderRecord.h"
#include "gs/gameobj/Player.h"
#include "gs/outcom/AICause.h"
#include "ConstRecord.h"
#include "gs/utility/RandGen.h"
#include "gs/gameobj/UnitPool.h"
#include "gs/core/render_observer.h"
#include "gs/utility/MoveFlags.h"
#include "gs/utility/directions.h"
#include "net/general/network.h"
#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicObject.h"
#include "gs/core/player_view.h"
#include "UnitRecord.h"
#include "gs/gameobj/wonderutil.h"
#include "gs/gameobj/ArmyPool.h"
#include "gs/gameobj/CTP2Combat.h"
#include "net/general/net_action.h"
#include "gs/core/audio_types.h"
#include "ai/diplomacy/Diplomat.h"
#include "gs/gameobj/CriticalMessagesPrefs.h"
#include "gs/gameobj/Barbarians.h"			// EMOD
#include "RiskRecord.h"			// Add for barb code
#include "gs/gameobj/GameSettings.h"		// EMOD
#include "GovernmentRecord.h"   // EMOD to access government data
#include "DifficultyRecord.h"   // EMOD
#include "gs/gameobj/CivilisationPool.h"
#include "gs/gameobj/Happy.h"

// Clean architecture: game event observer registry
#include "gs/core/game_observer.h"

#include "gs/gameobj/ArmyPool.h"  // armypool_Get()




STDEHANDLER(ArmyMoveOrderEvent)
{
	Army a;
	Path *p;
	MapPoint curPos;
	sint32 extra;

	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPath(0, p)) return GEV_HD_Continue;
	if(!args->GetPos(0, curPos)) return GEV_HD_Continue;
	if(!args->GetInt(0, extra)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_MOVE, p, curPos, extra);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyMoveToOrderEvent)
{
	Army a;
	WORLD_DIRECTION d;

	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetDirection(0, d)) return GEV_HD_Continue;

	MapPoint oldPos, newPos;
	a.GetPos(oldPos);
	if(oldPos.GetNeighborPosition(d, newPos)) {
		a->AddOrders(UNIT_ORDER_MOVE_TO, newPos);
	}
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyVictoryMoveOrderEvent)
{
	Army a;

//	director_Get()->DecrementPendingGameActions();

	MapPoint newPos;

	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, newPos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_VICTORY_MOVE, newPos);

	return GEV_HD_Continue;
}

STDEHANDLER(ArmyMovePathOrderEvent)
{
	Army army; // Changed from a to army for consistency. // Why consitency? Half of them are a and the other half of them are army in this file.
	MapPoint p;

	if(!args->GetArmy(0, army)) return GEV_HD_Continue;
	if(!args->GetPos(0, p)) return GEV_HD_Continue;
	/// @todo Finish code or remove it.
/*	// EMOD add rebase here? just set pos as p?

	bool rebase = false;
	for(i = 0; i < army.Num(); i++) {
		if(army.AccessData()->m_array[i].GetDBRec()->GetCanRebase()) {
			rebase = true;
		}
	}

	if (rebase) {  ///fix
			if(army.AccessData()->IsOccupiedByForeigner(p)) {
			if (world_Get()->HasCity(p) || terrainutil_HasAirfield(p)) {  //add unit later?

					gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent,
							   GEV_FinishMove,
							   GEA_Army, army,
							   GEA_Direction, dir,
							   GEA_MapPoint, newPos,
							   GEA_Int, order,
							   GEA_End);


		return GEV_HD_Continue;

	end EMOD	*/

	player_view::EnterMovePath(army.GetOwner(), army, army->RetPos(), p);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyUnloadOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	UNIT_ORDER_TYPE ord = UNIT_ORDER_UNLOAD;
	if (a.GetOwner() == player_view::VisiblePlayer())
	{
		CellUnitList cargoToUnload;
		if (player_view::GetSelectedCargo(cargoToUnload) && !player_Get(a.GetOwner())->IsRobot())
		{
		    ord = UNIT_ORDER_UNLOAD_SELECTED_STACK;
		}
	}

	a->ClearOrders();
	a->AddOrders(ord, pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmySleepOrderEvent)
{
	Army a;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_SLEEP);
	return GEV_HD_Continue;
}

// Auto-explore order: pick the nearest unexplored tile (BFS over the owner's
// known map), flag every unit in the army as exploring, and seed a MOVE_TO.
// BeginTurnUnitEvent re-issues each round from the new position; this
// handler only sets the initial state.  Clears the flag if no reachable
// unexplored tile remains.
STDEHANDLER(ArmyExploreOrderEvent)
{
	Army a;
	if (!args->GetArmy(0, a))    return GEV_HD_Continue;
	if (!a.IsValid())             return GEV_HD_Continue;

	ArmyData * ad = a.AccessData();
	if (!ad) return GEV_HD_Continue;

	Player * owner = player_Get(ad->GetOwner());
	if (!owner) return GEV_HD_Continue;

	MapPoint const start = ad->RetPos();
	MapPoint target;
	if (!owner->FindNearestUnexplored(start, target)) {
		// Map fully explored from here — flag stays cleared.
		for (sint32 i = 0; i < ad->Num(); ++i) {
			Unit u = ad->Access(i);
			if (UnitData * ud = u.AccessData()) ud->SetExploring(false);
		}
		return GEV_HD_Continue;
	}

	for (sint32 i = 0; i < ad->Num(); ++i) {
		Unit u = ad->Access(i);
		if (UnitData * ud = u.AccessData()) {
			ud->SetExploring(true);
			ud->SetExploreTarget(target);
		}
	}

	a->AddOrders(UNIT_ORDER_MOVE_TO, target);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyMoveUnloadOrderEvent)
{
	Army a;
	MapPoint pos;
	Path *p;
	sint32 extra;

	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPath(0, p)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;
	if(!args->GetInt(0, extra)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_MOVE_THEN_UNLOAD, p, pos, extra);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyEntrenchOrderEvent)
{
	Army a;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_ENTRENCH );
	return GEV_HD_Continue;

}
STDEHANDLER(ArmyDetrenchOrderEvent)
{
	Army a;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_DETRENCH );
	return GEV_HD_Continue;

}
STDEHANDLER(ArmyDisbandArmyOrderEvent)
{
	Army a;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_DISBAND);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyGroupOrderEvent)
{
	Army a;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_GROUP );
	return GEV_HD_Continue;

}

STDEHANDLER(ArmyGroupUnitOrderEvent)
{
	Army a;
	Unit u;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;

	MapPoint pos;

	a->AddOrders(UNIT_ORDER_GROUP_UNIT, nullptr, pos, u.m_id);

	return GEV_HD_Continue;

}
STDEHANDLER(ArmyUngroupOrderEvent)
{
	Army a;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_UNGROUP );
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyInvestigateCityOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_INVESTIGATE_CITY , pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyNullifyWallsOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_NULLIFY_WALLS , pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyStealTechnologyOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_STEAL_TECHNOLOGY , pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyInciteRevolutionOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_INCITE_REVOLUTION , pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyAssassinateRulerOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_ASSASSINATE , pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyInvestigateReadinessOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_INVESTIGATE_READINESS , pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyBombardOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_BOMBARD, pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyFranchiseOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_FRANCHISE , pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmySueOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_SUE , pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmySueFranchiseOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_SUE_FRANCHISE);

	return GEV_HD_Continue;
}

STDEHANDLER(ArmyExpelOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_EXPEL , pos);
	audio_observer::AddGameSound((sint32)GAMESOUNDS_ALERT);

	return GEV_HD_Continue;
}

STDEHANDLER(ArmyEstablishEmbassyOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_ESTABLISH_EMBASSY , pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyThrowPartyOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_THROW_PARTY , pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyAdvertiseOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_ADVERTISE , pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyPlantNukeOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_PLANT_NUKE , pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmySlaveRaidOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_SLAVE_RAID , pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyEnslaveSettlerOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_ENSLAVE_SETTLER , pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyUndergroundRailwayOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_UNDERGROUND_RAILWAY , pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyInciteUprisingOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_INCITE_UPRISING , pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyBioInfectOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_BIO_INFECT, pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyPlagueOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_PLAGUE, pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyNanoInfectOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_NANO_INFECT , pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyConvertCityOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_CONVERT , pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyReformCityOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_REFORM);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmySellIndulgencesOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_INDULGENCE , pos);

	Unit c;
	c.m_id = world_Get()->GetCity(pos).m_id;

	return GEV_HD_Continue;
}

STDEHANDLER(ArmySoothsayOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_SOOTHSAY , pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyCreateParkOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_CREATE_PARK , pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyPillageOrderEvent)
{
	Army a;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_PILLAGE);
	return GEV_HD_Continue;

}
STDEHANDLER(ArmyInjoinOrderEvent)
{
	Army a;
	MapPoint pos;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_INJOIN , pos);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyPirateOrderEvent)
{
	Army a;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_INTERCEPT_TRADE);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyGetExpelledOrderEvent)
{
	Army a;
	MapPoint pos;
	sint32 player;
	sint32 victim;

	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;
	if(!args->GetPlayer(0, player)) return GEV_HD_Continue;

	victim = a->GetOwner();

	a->AutoAddOrdersWrongTurn(UNIT_ORDER_EXPEL_TO, nullptr, pos, 0);

	SlicObject *so = new SlicObject("42UnitExpelled");
	so->AddCivilisation(player);
	so->AddRecipient(victim);
	slicengine_Get()->Execute(so);

	return GEV_HD_Continue;
}

STDEHANDLER(ArmySettleOrderEvent)
{
	Army a;

	if(!args->GetArmy(0, a)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_SETTLE);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmySettleInCityOrderEvent)
{
	Army a;

	if(!args->GetArmy(0, a)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_SETTLE_IN_CITY);
	return GEV_HD_Continue;
}

STDEHANDLER(BoardTransportOrderEvent)
{
	Army a;

	if(!args->GetArmy(0, a)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_BOARD_TRANSPORT);
	return GEV_HD_Continue;
}

STDEHANDLER(LaunchOrderEvent)
{
	Army a;
	MapPoint reentryPos;

	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, reentryPos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_LAUNCH, reentryPos);
	return GEV_HD_Continue;
}

STDEHANDLER(TargetOrderEvent)
{
	Army a;
	MapPoint targetPos;

	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, targetPos)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_TARGET, targetPos);
	return GEV_HD_Continue;
}

STDEHANDLER(UpgradeOrderEvent)
{
	Army a;
	MapPoint targetPos;

	if(!args->GetArmy(0, a)) return GEV_HD_Continue;

	a->AddOrders(UNIT_ORDER_UPGRADE);
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyMoveEvent)
{
	Army army;
	if(!args->GetArmy(0, army)) return GEV_HD_Continue;

	WORLD_DIRECTION dir;
	if(!args->GetDirection(0, dir))	return GEV_HD_Continue;

	UNIT_ORDER_TYPE order;
	if(!args->GetInt(0, (sint32 &)order)) return GEV_HD_Continue;

	sint32 extra;
	if(!args->GetInt(1, extra)) return GEV_HD_Continue;

	MapPoint extraOrderPos;
	if(!args->GetPos(0, extraOrderPos))	return GEV_HD_Continue;

	MapPoint    oldPos;
	army.GetPos(oldPos);

	MapPoint    newPos;
	if (!oldPos.GetNeighborPosition(dir, newPos))
		return GEV_HD_Continue;

	ArmyData *  armyData    = army.AccessData();
	if (armyData->CheckSpecialUnitMove(newPos)) {
		return GEV_HD_Continue;
	}

	armyData->CheckLoadSleepingCargoFromCity(nullptr);

	if (armyData->IsMovePointsEnough(newPos))
	{
		/// @todo finish this code or remove it
/*EMOD for rebasing? this just moves it to newpos maybe do pathing order? or move to? and then set newpos as endpos?
		if (order == UNIT_ORDER_MOVE_TO

// EMOD - Rebasing of units, especially aircraft - code removed trying to create a code that automatically moves a unit from a
//city to another city anywhere in the world and costing that unit 1 move.

		UnitDynamicArray revealedUnits;
		for (sint32 i = m_nElements - 1; i>= 0; i--) {   //for(i = 0; i < m_nElements; i++) {
			if(!m_array[i].GetDBRec()->GetCanRebase()){
				if (!IsOccupiedByForeigner(order->m_point)){
					if (world_Get()->HasCity(order->m_point) || terrainutil_HasAirfield(order->m_point)) {  //add unit later?
						m_array[i].SetPosition(order->m_point, revealedUnits);
						return true;
					}
				}
				return false;
			}
		}

*/		//end EMOD
		if (armyData->IsOccupiedByForeigner(newPos))
		{
			CellUnitList * defender = world_Get()->GetCell(newPos)->UnitArmy();

			for (sint32 d = 0; d < defender->Num(); ++d)
			{
				if (defender->Access(d).Flag(k_UDF_CANT_BE_ATTACKED))
					return GEV_HD_Continue;
			}

			PLAYER_INDEX owner = army.GetOwner();
			if ((owner == PLAYER_INDEX_VANDALS) &&
				wonderutil_GetProtectFromBarbarians(player_Get(defender->GetOwner())->m_builtWonders))
			{
				return GEV_HD_Continue;
			}

			if(!armyData->CheckWasEnemyVisible(newPos))
			{
				gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent,
									   GEV_ContactMade,
									   GEA_Player, owner,
									   GEA_Player, defender->GetOwner(),
									   GEA_End);

				if(order == UNIT_ORDER_MOVE ||
				   order == UNIT_ORDER_MOVE_TO)
				{
					DPRINTF(k_DBG_GAMESTATE, ("Army 0x%lx clear orders, was not visible\n", army.m_id));
					gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent,
										   GEV_ClearOrders,
										   GEA_Army, army,
										   GEA_End);

					if(!extra)
						return GEV_HD_Continue;

					if (newPos != extraOrderPos)
						return GEV_HD_Continue;
				}
			}

			if (order == UNIT_ORDER_MOVE_THEN_UNLOAD)
				return GEV_HD_Continue;

			for (sint32 i = 0; i < army.Num(); i++)
			{
				if ((*armyData)[i].Flag(k_UDF_FOUGHT_THIS_TURN) ||
				    (*armyData)[i].Flag(k_UDF_USED_SPECIAL_ACTION_THIS_TURN)
				   )
				{
					return GEV_HD_Continue;
				}
			}

			gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent,
								   GEV_FinishAttack,
								   GEA_Army, army,
								   GEA_MapPoint, newPos,
								   GEA_End);

			gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent,
								   GEV_ClearOrders,
								   GEA_Army, army,
								   GEA_End);
		}
		else
		{
			gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent,
							       GEV_FinishMove,
							       GEA_Army, army,
							       GEA_Direction, dir,
							       GEA_MapPoint, newPos,
							       GEA_Int, order,
							       GEA_End);
		}
	}
	else
	{
		gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent,
							   GEV_CantMoveYet,
							   GEA_Army, army,
							   GEA_Direction, dir,
							   GEA_MapPoint, newPos,
							   GEA_End);
	}

	return GEV_HD_Continue;
}

STDEHANDLER(ClearOrdersEvent)
{
	Army army;
	if (args->GetArmy(0, army))
	{
		army.ClearOrders();
	}

	return GEV_HD_Continue;
}

STDEHANDLER(FinishAttackEvent)
{
	Army        a;
	MapPoint    pos;
	if (    args->GetArmy(0, a)     // May return false when the whole army
	                                // has been destroyed during the attack.
	     && args->GetPos(0, pos)
	   )
	{
		a.AddOrders(UNIT_ORDER_FINISH_ATTACK, nullptr, pos, 0);
	}

	return GEV_HD_Continue;
}

STDEHANDLER(FinishMoveEvent)
{
	Army army;
	WORLD_DIRECTION dir;
	MapPoint pos;
	sint32 order;

	if(!args->GetArmy(0, army))
		return GEV_HD_Continue;

	if(!args->GetDirection(0, dir))
		return GEV_HD_Continue;

	if(!args->GetPos(0, pos))
		return GEV_HD_Continue;

	if(!args->GetInt(0, order))
		return GEV_HD_Continue;

	army->FinishMove(dir, pos, (UNIT_ORDER_TYPE)order);

	return GEV_HD_Continue;
}

STDEHANDLER(MoveIntoTransportEvent)
{
	Army a;
	MapPoint pos;

	if(!args->GetArmy(0, a))
	{
		Assert( armypool_Get()->IsValid(a) );
		return GEV_HD_Continue;
	}
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	CellUnitList transports;
	sint32 canMoveIntoTransport = a.AccessData()->NumUnitsCanMoveIntoTransport(pos, transports);

	if(a.AccessData()->Num() <= canMoveIntoTransport)
	{
		a.AccessData()->MoveIntoTransport(pos, transports);
	}
	// @ToDo merge this with the above when also the settler escord can find the goal without a settler
	else if(canMoveIntoTransport > 0)
	{
		gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent,
							   GEV_MoveIntoTransport,
							   GEA_Army, a,
							   GEA_MapPoint, pos,
							   GEA_End);

		a->RemainNumUnits(canMoveIntoTransport);
	}
	else
	{
		audio_observer::AddGameSound((sint32)GAMESOUNDS_ILLEGAL_MOVE);
	}

	return GEV_HD_Continue;
}

STDEHANDLER(BattleEvent)
{
	Army army;
	MapPoint pos;

	if(!args->GetArmy(0, army))
		return GEV_HD_Continue;

	if(!args->GetPos(0, pos))
		return GEV_HD_Continue;

	CellUnitList defender;
	world_Get()->GetArmy(pos, defender);

	bool const  i_died = !army.AccessData()->Fight(defender);
	if (!i_died)
	{
		for (sint32 k = 0; k < army.Num(); k++) {
			bool out_of_fuel;
			army[k].DeductMoveCost(g_theConstDB->Get(0)->GetSpecialActionMoveCost(),
								   out_of_fuel);
		}
	}

	return GEV_HD_Continue;
}

STDEHANDLER(AftermathEvent)
{
	Army army;
	MapPoint pos;
	Unit ta;
	Unit td;
	sint32 attack_owner, defense_owner;

	sint32 fromARealBattle;
	if(!args->GetInt(0, fromARealBattle)) return GEV_HD_Continue;

	Assert(!fromARealBattle || combat_Get());
	if (combat_Get())
	{
		delete combat_Get();
		combat_Set(nullptr);
	}

	args->GetArmy(0, army);

	if(!args->GetPos(0, pos))
		return GEV_HD_Continue;

	args->GetUnit(0, ta);
	args->GetUnit(1, td);

	if(!args->GetPlayer(0, attack_owner))
		return GEV_HD_Continue;

	if(!args->GetPlayer(1, defense_owner))
		return GEV_HD_Continue;

	Unit            c = world_Get()->GetCell(pos)->GetCity();
	CellUnitList    defender;
	world_Get()->GetArmy(pos, defender);

	if(c.IsValid())
	{
		//add civil war here?  no because its in MoveEvent? Why?  EMOD

		//end EMOD
		if(civrand().Next(100) < g_theConstDB->Get(0)->GetAssaultDestroyBuildingChance() * 100)
		{
			//shouldn't constDB allow players to set how many buildings be destroyed?
			c.DestroyRandomBuilding();
		}
		//this code actually isn't used...
		if(defender.Num() > 0)
		{
			if(c.PopCount() > 1 && (civrand().Next(100) < g_theConstDB->Get(0)->GetAssaultKillPopChance() * 100))
			{
				//emod allows for differnt city casualty rates instead of just one  6.22.2007
				sint32 casualties = 0;
				if (g_theConstDB->Get(0)->GetCapturedCityKillPop() < 0)
				{
					casualties = civrand().Next(c.PopCount()) * -1 ;
				}
				else
				{
					casualties = g_theConstDB->Get(0)->GetCapturedCityKillPop() * -1;
				}

				c.CD()->ChangePopulation(casualties);
			}
		}
	}

	if(ta.IsValid())
	{
		render_observer::AddTerminateFaceoff(ta);
	}

	if(td.IsValid())
	{
		render_observer::AddTerminateFaceoff(td);
	}

	bool attackerWon = false;

	if(army.IsValid() && army.Num() > 0)
	{
		for(sint32 i = 0; i < army.Num(); i++)
		{
			if(army[i].GetHP() < 0.5)
			{
				continue;
			}

			attackerWon = true;
			break;
		}
	}

	if(attackerWon)
	{
		if(defense_owner == player_view::VisiblePlayer() && !c.m_id)
		{
			audio_observer::AddGameSound((sint32)GAMESOUNDS_VICTORY_FANFARE);
		}

		if(attack_owner == player_view::VisiblePlayer() && !c.m_id)
		{
			audio_observer::AddGameSound((sint32)GAMESOUNDS_VICTORY_FANFARE);
		}

		if(army.IsValid())
		{
			army.AccessData()->DoVictoryEnslavement(defense_owner);
		}
	}
	else
	{
		if(attack_owner == player_view::VisiblePlayer() && !c.m_id)
		{
			audio_observer::AddGameSound((sint32)GAMESOUNDS_LOSE_PLAYER_BATTLE);
		}

		if(defense_owner == player_view::VisiblePlayer() && !c.m_id)
		{
			audio_observer::AddGameSound((sint32)GAMESOUNDS_LOSE_PLAYER_BATTLE);
		}

		defender.DoVictoryEnslavement(attack_owner);
	}

	if(attackerWon && army.IsValid() && army.Num() > 0)
	{
		for(sint32 i = 0; i < army.Num(); i++)
		{
			if(army[i].GetHP() < 0.5) {
				continue;
			}
			//TODO find out why its not running the leader & elite code
			// apparently a '#' outcomented the value in constDB.txt
			if( (army[i].GetAttack() > 0)
			&&  (civrand().Next(100) < sint32(g_theConstDB->Get(0)->GetCombatLeaderChance() * 100.0))
			&&  (army[i].IsElite()) //IsElite
			){
				player_Get(attack_owner)->CreateLeader(); //Great Leader Code - Emod 6-5-2007
			}

			if( (army[i].GetAttack() > 0)
			&&  (civrand().Next(100) < sint32(g_theConstDB->Get(0)->GetCombatEliteChance() * 100.0))
			&&  (army[i].IsVeteran())
			){
				army[i].SetElite(); //elite code - Emod 6-5-2007
			}

			if( (army[i].GetAttack() > 0)
			&&  (civrand().Next(100) < sint32(g_theConstDB->Get(0)->GetCombatVeteranChance() * 100.0))
			&&  (!army[i].IsVeteran())
			){
				army[i].SetVeteran();
			}

			//end emod
			army[i].WakeUp();
			army[i].ClearFlag(k_UDF_WAS_TOP_UNIT_BEFORE_BATTLE);
			army[i].SetFlag(k_UDF_FIRST_MOVE);
		}
//		director_Get()->IncrementPendingGameActions();
		gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_VictoryMoveOrder,
							   GEA_Army, army,
							   GEA_MapPoint, pos,
							   GEA_End);
	}

	for(sint32 i = 0; i < defender.Num() ; i++)
	{
		if(defender[i].GetAttack() > 0 &&
		   civrand().Next(100) < sint32(g_theConstDB->Get(0)->GetCombatVeteranChance() * 100.0))
		{
			defender[i].SetVeteran();
		}
		if( (defender[i].GetAttack() > 0)
		&&  (civrand().Next(100) < sint32(g_theConstDB->Get(0)->GetCombatEliteChance() * 100.0))
		&&  (defender[i].IsVeteran())
		){
			defender[i].SetElite();
		}
		//Great Leader Code - Emod 6-5-2007
		if( (defender[i].GetAttack() > 0)
		&&  (civrand().Next(100) < sint32(g_theConstDB->Get(0)->GetCombatLeaderChance() * 100.0))
		&&  (defender[i].IsElite())
		){
			player_Get(defense_owner)->CreateLeader();
		}

		//copy and make for elite units
		//add great leader chance but only if veteran and/or elite
		defender[i].WakeUp();
		defender[i].ClearFlag(k_UDF_WAS_TOP_UNIT_BEFORE_BATTLE);
	}

	return GEV_HD_Continue;
}

STDEHANDLER(CheckOrdersEvent)
{
	Army a;
	if(!args->GetArmy(0, a))
		return GEV_HD_Continue;

	if(!g_network.IsActive() || g_network.IsLocalPlayer(a.GetOwner()))
	{
		a->CheckAddEventOrder();

		a.AccessData()->ExecuteOrders();
	}
	return GEV_HD_Continue;
}

STDEHANDLER(BeginTurnArmyEvent)
{
	Army a;
	if(!args->GetArmy(0, a))
		return GEV_HD_Continue;

	a.BeginTurn();
	return GEV_HD_Continue;
}

STDEHANDLER(MoveUnitsEvent)
{
	Army a;
	MapPoint from, to;

	if(!args->GetArmy(0, a)) return GEV_HD_Stop;
	if(!args->GetPos(0, from)) return GEV_HD_Continue;
	if(!args->GetPos(1, to)) return GEV_HD_Continue;

	for(sint32 i = 0; i < a.Num(); i++)
	{
		if(a[i].GetDBRec()->GetCantMove())
		{
			return GEV_HD_Stop;
		}
	}

	gevmanager_Get()->AddEvent
	                      (
	                       GEV_INSERT_AfterCurrent,
	                       GEV_CheckOrders,
	                       GEA_Army,        a,
	                       GEA_MapPoint,    from,
	                       GEA_MapPoint,    to,
	                       GEA_End
	                      );

	a->MoveUnits(to);

//	sint32 old_cell_owner = world_Get()->GetCell(from)->GetOwner();
	sint32 new_cell_owner = world_Get()->GetCell(to)->GetOwner();
	sint32 army_owner = a->GetOwner();

	/* Guard the player array access: new_cell_owner can be -1 (unowned / barbarian
	   territory) or an out-of-range value from corrupt map data.  The array is
	   allocated with k_MAX_PLAYERS (32) entries. */
	Player *player_ptr = (new_cell_owner >= 0 && new_cell_owner < k_MAX_PLAYERS)
		? player_Get(new_cell_owner)
		: nullptr;
	if ( new_cell_owner != -1 &&
		 new_cell_owner != army_owner &&
		 player_ptr &&
		 player_ptr->IsVisible(to)) {

		const Diplomat & new_cell_diplomat = Diplomat::GetDiplomat(new_cell_owner);
		uint32 incursion_permission = new_cell_diplomat.GetIncursionPermission();
		if (army_owner >= 0 && army_owner < 32 &&
			!(incursion_permission & (0x1 << army_owner)) &&
			!new_cell_diplomat.GetBorderIncursionBy(army_owner))
		{
			bool is_threat = (a->HasCargo() || !a->IsCivilian()) &&
			                 a->PlayerCanSee(new_cell_owner);

			if (is_threat)
			{
				gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_BorderIncursion,
					GEA_Player, new_cell_owner,
					GEA_Player, army_owner,
					GEA_End);
			}
		}
	}

	if(world_Get()->HasCity(to))
	{
		Unit c = world_Get()->GetCity(to);
		PLAYER_INDEX city_owner = c.GetOwner();
		if(city_owner != a->GetOwner())
		{
			if(!a->IsEnemy(city_owner))
			{
				SlicObject *so;
				if(g_network.IsActive()
				&& g_network.TeamsEnabled()
				&& player_Get(a->GetOwner())->m_networkGroup == player_Get(city_owner)->m_networkGroup
				){
					so = new SlicObject("110aCantAttackTeammates");
				}
				else
				{
					so = new SlicObject("110CantAttackAllies");
				}
				so->AddRecipient(a->GetOwner());
				so->AddCivilisation(city_owner);
				so->AddUnit(a[0]);
				so->AddLocation(to);
				slicengine_Get()->Execute(so);
			}
			else
			{
				for(sint32 i = 0; i < a->Num(); i++)
				{
					if(!a[i].IsCantCaptureCity())
					{
						PLAYER_INDEX originalOwner = c.GetOwner();

						if(civrand().Next(100) < g_theConstDB->Get(0)->GetCaptureKillPopChance() * 100)
						{
							gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_KillPop,
												   GEA_City, c.m_id,
												   GEA_End);
#if 0
							if(g_network.IsHost())
							{
								g_network.Block(a.GetOwner());
							}
							c.CD()->ChangePopulation(-1);
							if(g_network.IsHost())
							{
								g_network.Unblock(a.GetOwner());
							}
#endif
						}

						if (c.IsCapitol())
						{
							//civil war code didnt work here either  EMOD

							SlicObject *so = new SlicObject("127CapitalCityCapturedVictim");
							so->AddRecipient(originalOwner);
							so->AddCity(c);
							slicengine_Get()->Execute(so);

							so = new SlicObject("126CapitalCityCapturedAttacker");
							so->AddRecipient(a->GetOwner());
							so->AddCivilisation(originalOwner);
							so->AddCity(c);
							slicengine_Get()->Execute(so);
						}
						else if (c.GetData()->GetCityData()->PopCount() >= 1)
						{
						//	Maybe we find another use for this message
						//	SlicObject *so = new SlicObject("123CitiesCapturedVictim");
						//	so->AddRecipient(originalOwner);
						//	so->AddCity(c);
						//	slicengine_Get()->Execute(so);
						}

						if(c.GetOwner() == player_view::VisiblePlayer())
							render_observer::AddCenterMap(to);

						for(sint32 k = 0; k < a->Num(); k++)
						{
							a[k].SetMovementPoints(0.0);
						}

						gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent,
											   GEV_CaptureCity,
											   GEA_City, c,
											   GEA_Player, a->GetOwner(),
											   GEA_Int, (sint32)CAUSE_REMOVE_CITY_ATTACK,
											   GEA_End);
						break;
					}
				}
			}
		}
	}

	a->CheckActiveDefenders(to, false);

	if(!a.IsValid())
	{
		return GEV_HD_Stop;
	}

	a->CheckTerrainEvents();

	a->IncrementOrderPath();

	return GEV_HD_Continue;
}

STDEHANDLER(ArmyFinishUnloadEvent)
{
	Army transportArmy;
	Army unloadingArmy;
	MapPoint to_pt;

	if(!args->GetArmy(0, transportArmy)) return GEV_HD_Continue;
	if(!args->GetArmy(1, unloadingArmy)) return GEV_HD_Continue;
	if(!args->GetPos(0, to_pt)) return GEV_HD_Continue;

	transportArmy->FinishUnloadOrder(unloadingArmy, to_pt);

	return GEV_HD_Continue;
}

STDEHANDLER(LawsuitEvent)
{
	Army a;
	MapPoint point;
	Unit lawyer;

	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, point)) return GEV_HD_Continue;
	if(!args->GetUnit(0, lawyer)) return GEV_HD_Continue;

	Cell *cell = world_Get()->GetCell(point);

	a->InformAI(UNIT_ORDER_SUE, point);

	if(cell->GetNumUnits() <= 0)
		return GEV_HD_Continue;

	sint32 victim = cell->AccessUnit(0)->GetOwner();
	sint32 i, n = cell->GetNumUnits();
	sint32 utype = -1;
	for(i = n-1; i >= 0; i--) {
		Unit *u = &(cell->AccessUnit(i));
		if(u->GetDBRec()->GetCanBeSued()) {
			if(utype < 0)
				utype = u->GetType();

			victim = u->GetOwner();
			gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_KillUnit,
								   GEA_Unit, u->m_id,
								   GEA_Int, CAUSE_REMOVE_ARMY_SUE,
								   GEA_Player, a->GetOwner(),
								   GEA_End);

		}
	}

	a->AddSpecialActionUsed(lawyer);

	if(utype >= 0) {
		SlicObject *so = new SlicObject("161SueCompleteVictim");
		so->AddRecipient(victim);
		so->AddLocation(a->RetPos());
		so->AddUnitRecord(utype);
		slicengine_Get()->Execute(so);
	}

	return GEV_HD_Continue;
}

STDEHANDLER(RemoveFranchiseEvent)
{
	Army a;
	Unit lawyer;
	Unit city;

	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetUnit(0, lawyer)) return GEV_HD_Continue;
	if(!args->GetUnit(0, city)) return GEV_HD_Continue;

	city.SetFranchiseTurnsRemaining(g_theConstDB->Get(0)->GetTurnsToSueFranchise());
	return GEV_HD_Continue;
}

STDEHANDLER(ExpelUnitsEvent)
{
	return GEV_HD_Continue;
}

STDEHANDLER(EnslaveSettlerEvent)
{
	Army    a;
	Unit    slaver;
	Unit    settler;

	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetUnit(0, slaver)) return GEV_HD_Continue;
	if(!args->GetUnit(1, settler)) return GEV_HD_Continue;

	PLAYER_INDEX    settlerOwner    = settler.GetOwner();
	PLAYER_INDEX    slaverOwner     = slaver.GetOwner();

	gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_KillUnit,
						   GEA_Unit,    settler,
						   GEA_Int,     CAUSE_REMOVE_ARMY_ENSLAVED,
						   GEA_Player,  slaverOwner,
						   GEA_End);

	Unit home_city;
	if (player_Get(slaverOwner)->GetSlaveCity(slaver.RetPos(), home_city))
	{
		gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_MakePop,
		                       GEA_City,    home_city.m_id,
		                       GEA_Player,  settlerOwner,
		                       GEA_End);
	}
	// else No action: the slaver does not have any cities

	return GEV_HD_Continue;
}

STDEHANDLER(TeleportEvent)
{
	Army a;
	MapPoint pos;

	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	a->SetPositionAndFixActors(pos);

	return GEV_HD_Continue;
}

STDEHANDLER(ReentryEvent)
{
	Army a;

	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	a->Reenter();
	return GEV_HD_Continue;
}

STDEHANDLER(SetUnloadMovementEvent)
{
	Army a;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	a->SetUnloadMovementPoints();
	return GEV_HD_Continue;
}

STDEHANDLER(ArmyBeginTurnExecuteEvent)
{
	Army a;
	if (args->GetArmy(0, a))
	{
		a->ExecuteOrders();
	}

	return GEV_HD_Continue;
}

void armyevent_Initialize()
{
	gevmanager_Get()->AddCallback(GEV_MoveOrder, GEV_PRI_Primary, &s_ArmyMoveOrderEvent);
	gevmanager_Get()->AddCallback(GEV_MoveToOrder, GEV_PRI_Primary, &s_ArmyMoveToOrderEvent);
	gevmanager_Get()->AddCallback(GEV_MovePathOrder, GEV_PRI_Primary, &s_ArmyMovePathOrderEvent);
	gevmanager_Get()->AddCallback(GEV_VictoryMoveOrder, GEV_PRI_Primary, &s_ArmyVictoryMoveOrderEvent);

	gevmanager_Get()->AddCallback(GEV_UnloadOrder, GEV_PRI_Primary, &s_ArmyUnloadOrderEvent);
	gevmanager_Get()->AddCallback(GEV_SleepOrder, GEV_PRI_Primary, &s_ArmySleepOrderEvent);
	gevmanager_Get()->AddCallback(GEV_ExploreOrder, GEV_PRI_Primary, &s_ArmyExploreOrderEvent);
	gevmanager_Get()->AddCallback(GEV_MoveUnloadOrder, GEV_PRI_Primary, &s_ArmyMoveUnloadOrderEvent);
	gevmanager_Get()->AddCallback(GEV_EntrenchOrder, GEV_PRI_Primary, &s_ArmyEntrenchOrderEvent);
	gevmanager_Get()->AddCallback(GEV_DetrenchOrder, GEV_PRI_Primary, &s_ArmyDetrenchOrderEvent);
	gevmanager_Get()->AddCallback(GEV_DisbandArmyOrder, GEV_PRI_Primary, &s_ArmyDisbandArmyOrderEvent);
	gevmanager_Get()->AddCallback(GEV_GroupOrder, GEV_PRI_Primary, &s_ArmyGroupOrderEvent);
	gevmanager_Get()->AddCallback(GEV_GroupUnitOrder, GEV_PRI_Primary, &s_ArmyGroupUnitOrderEvent);
	gevmanager_Get()->AddCallback(GEV_UngroupOrder, GEV_PRI_Primary, &s_ArmyUngroupOrderEvent);
	gevmanager_Get()->AddCallback(GEV_InvestigateCityOrder, GEV_PRI_Primary, &s_ArmyInvestigateCityOrderEvent);
	gevmanager_Get()->AddCallback(GEV_NullifyWallsOrder, GEV_PRI_Primary, &s_ArmyNullifyWallsOrderEvent);
	gevmanager_Get()->AddCallback(GEV_StealTechnologyOrder, GEV_PRI_Primary, &s_ArmyStealTechnologyOrderEvent);
	gevmanager_Get()->AddCallback(GEV_InciteRevolutionOrder, GEV_PRI_Primary, &s_ArmyInciteRevolutionOrderEvent);
	gevmanager_Get()->AddCallback(GEV_AssassinateRulerOrder, GEV_PRI_Primary, &s_ArmyAssassinateRulerOrderEvent);
	gevmanager_Get()->AddCallback(GEV_InvestigateReadinessOrder, GEV_PRI_Primary, &s_ArmyInvestigateReadinessOrderEvent);
	gevmanager_Get()->AddCallback(GEV_BombardOrder, GEV_PRI_Primary, &s_ArmyBombardOrderEvent);
	gevmanager_Get()->AddCallback(GEV_FranchiseOrder, GEV_PRI_Primary, &s_ArmyFranchiseOrderEvent);
	gevmanager_Get()->AddCallback(GEV_SueOrder, GEV_PRI_Primary, &s_ArmySueOrderEvent);
	gevmanager_Get()->AddCallback(GEV_SueFranchiseOrder, GEV_PRI_Primary, &s_ArmySueFranchiseOrderEvent);
	gevmanager_Get()->AddCallback(GEV_ExpelOrder, GEV_PRI_Primary, &s_ArmyExpelOrderEvent);
	gevmanager_Get()->AddCallback(GEV_EstablishEmbassyOrder, GEV_PRI_Primary, &s_ArmyEstablishEmbassyOrderEvent);
	gevmanager_Get()->AddCallback(GEV_ThrowPartyOrder, GEV_PRI_Primary, &s_ArmyThrowPartyOrderEvent);
	gevmanager_Get()->AddCallback(GEV_AdvertiseOrder, GEV_PRI_Primary, &s_ArmyAdvertiseOrderEvent);
	gevmanager_Get()->AddCallback(GEV_PlantNukeOrder, GEV_PRI_Primary, &s_ArmyPlantNukeOrderEvent);
	gevmanager_Get()->AddCallback(GEV_SlaveRaidOrder, GEV_PRI_Primary, &s_ArmySlaveRaidOrderEvent);
	gevmanager_Get()->AddCallback(GEV_EnslaveSettlerOrder, GEV_PRI_Primary, &s_ArmyEnslaveSettlerOrderEvent);
	gevmanager_Get()->AddCallback(GEV_UndergroundRailwayOrder, GEV_PRI_Primary, &s_ArmyUndergroundRailwayOrderEvent);
	gevmanager_Get()->AddCallback(GEV_InciteUprisingOrder, GEV_PRI_Primary, &s_ArmyInciteUprisingOrderEvent);
	gevmanager_Get()->AddCallback(GEV_BioInfectOrder, GEV_PRI_Primary, &s_ArmyBioInfectOrderEvent);
	gevmanager_Get()->AddCallback(GEV_PlagueOrder, GEV_PRI_Primary, &s_ArmyPlagueOrderEvent);
	gevmanager_Get()->AddCallback(GEV_NanoInfectOrder, GEV_PRI_Primary, &s_ArmyNanoInfectOrderEvent);
	gevmanager_Get()->AddCallback(GEV_ConvertCityOrder, GEV_PRI_Primary, &s_ArmyConvertCityOrderEvent);
	gevmanager_Get()->AddCallback(GEV_ReformCityOrder, GEV_PRI_Primary, &s_ArmyReformCityOrderEvent);
	gevmanager_Get()->AddCallback(GEV_SellIndulgencesOrder, GEV_PRI_Primary, &s_ArmySellIndulgencesOrderEvent);
	gevmanager_Get()->AddCallback(GEV_SoothsayOrder, GEV_PRI_Primary, &s_ArmySoothsayOrderEvent);
	gevmanager_Get()->AddCallback(GEV_CreateParkOrder, GEV_PRI_Primary, &s_ArmyCreateParkOrderEvent);
	gevmanager_Get()->AddCallback(GEV_PillageOrder, GEV_PRI_Primary, &s_ArmyPillageOrderEvent);
	gevmanager_Get()->AddCallback(GEV_InjoinOrder, GEV_PRI_Primary, &s_ArmyInjoinOrderEvent);
	gevmanager_Get()->AddCallback(GEV_PirateOrder, GEV_PRI_Primary, &s_ArmyPirateOrderEvent);
	gevmanager_Get()->AddCallback(GEV_GetExpelledOrder, GEV_PRI_Primary, &s_ArmyGetExpelledOrderEvent);
	gevmanager_Get()->AddCallback(GEV_SettleOrder, GEV_PRI_Primary, &s_ArmySettleOrderEvent);
	gevmanager_Get()->AddCallback(GEV_SettleInCityOrder, GEV_PRI_Primary, &s_ArmySettleInCityOrderEvent);
	gevmanager_Get()->AddCallback(GEV_BoardTransportOrder, GEV_PRI_Primary, &s_BoardTransportOrderEvent);
	gevmanager_Get()->AddCallback(GEV_LaunchOrder, GEV_PRI_Primary, &s_LaunchOrderEvent);
	gevmanager_Get()->AddCallback(GEV_TargetOrder, GEV_PRI_Primary, &s_TargetOrderEvent);
	gevmanager_Get()->AddCallback(GEV_UpgradeOrder, GEV_PRI_Primary, &s_UpgradeOrderEvent);

	gevmanager_Get()->AddCallback(GEV_MoveArmy, GEV_PRI_Primary, &s_ArmyMoveEvent);
	gevmanager_Get()->AddCallback(GEV_ClearOrders, GEV_PRI_Primary, &s_ClearOrdersEvent);
	gevmanager_Get()->AddCallback(GEV_FinishAttack, GEV_PRI_Primary, &s_FinishAttackEvent);
	gevmanager_Get()->AddCallback(GEV_FinishMove, GEV_PRI_Primary, &s_FinishMoveEvent);

	gevmanager_Get()->AddCallback(GEV_MoveUnits, GEV_PRI_Primary, &s_MoveUnitsEvent);
	gevmanager_Get()->AddCallback(GEV_CheckOrders, GEV_PRI_Primary, &s_CheckOrdersEvent);

	gevmanager_Get()->AddCallback(GEV_MoveIntoTransport, GEV_PRI_Primary, &s_MoveIntoTransportEvent);
	gevmanager_Get()->AddCallback(GEV_Battle, GEV_PRI_Primary, &s_BattleEvent);
	gevmanager_Get()->AddCallback(GEV_BattleAftermath, GEV_PRI_Primary, &s_AftermathEvent);

	gevmanager_Get()->AddCallback(GEV_BeginTurnArmy, GEV_PRI_Primary, &s_BeginTurnArmyEvent);
	gevmanager_Get()->AddCallback(GEV_FinishUnload, GEV_PRI_Primary, &s_ArmyFinishUnloadEvent);

	gevmanager_Get()->AddCallback(GEV_Lawsuit, GEV_PRI_Primary, &s_LawsuitEvent);
	gevmanager_Get()->AddCallback(GEV_RemoveFranchise, GEV_PRI_Primary, &s_RemoveFranchiseEvent);
	gevmanager_Get()->AddCallback(GEV_ExpelUnits, GEV_PRI_Primary, &s_ExpelUnitsEvent);
	gevmanager_Get()->AddCallback(GEV_EnslaveSettler, GEV_PRI_Primary, &s_EnslaveSettlerEvent);

	gevmanager_Get()->AddCallback(GEV_Teleport, GEV_PRI_Primary, &s_TeleportEvent);
	gevmanager_Get()->AddCallback(GEV_Reentry, GEV_PRI_Primary, &s_ReentryEvent);

	gevmanager_Get()->AddCallback(GEV_SetUnloadMovement, GEV_PRI_Primary, &s_SetUnloadMovementEvent);

	gevmanager_Get()->AddCallback(GEV_BeginTurnExecute, GEV_PRI_Primary, &s_ArmyBeginTurnExecuteEvent);
}

void armyevent_Cleanup()
{
}
