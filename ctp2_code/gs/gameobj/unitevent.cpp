//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Unit events
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
// - AddUnitToArmyEvent does not crash in the debug version if the unit to
//   be is transported and has therefore no army. This makes slic code save. (7-Nov-2007 Martin G�hmann)
// - Added an upgrade unit event. (13-Sep-2008 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/gameobj/UnitEvent.h"

#include "gs/outcom/AICause.h"
#include "gs/gameobj/Army.h"
#include "gs/gameobj/ArmyData.h"
#include "gs/world/Cell.h"
#include "gs/world/cellunitlist.h"
#include "gs/core/game_observer.h"
#include "gs/core/render_observer.h"
#include "gs/gameobj/Events.h"
#include "gs/events/GameEventUser.h"
#include "net/general/net_info.h"
#include "net/general/network.h"                    // g_network
#include "gs/gameobj/Order.h"
#include "gs/gameobj/Player.h"
#include "gs/slic/SlicEngine.h"                 // slicengine_Get()
#include "gs/slic/SlicObject.h"
#include "gs/core/tiledmap_observer.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/UnitData.h"
#include "UnitRecord.h"
#include "gs/world/World.h"                      // g_theWorld

STDEHANDLER(KillUnitEvent)
{
	Unit u;
	CAUSE_REMOVE_ARMY cause;
	sint32 killer;

	if(gameEventType == GEV_KillUnit) {
		if(!args->GetUnit(0, u))
			return GEV_HD_Continue;
	} else if(gameEventType == GEV_KillCity) {
		if(!args->GetCity(0, u))
			return GEV_HD_Continue;
	}

	if(!args->GetInt(0, (sint32&)cause))
		return GEV_HD_Continue;

	if(!args->GetPlayer(0, killer))
		killer = -1;

	if(u->GetCargoList() && u->GetCargoList()->Num() > 0) {


		gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_KillUnit,
							   GEA_Unit, u,
							   GEA_Int, cause,
							   GEA_Player, killer,
							   GEA_End);
		sint32 c;
		for(c = 0; c < u->GetCargoList()->Num(); c++) {
			gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_KillUnit,
								   GEA_Unit, u->GetCargoList()->Access(c).m_id,
								   GEA_Int, CAUSE_REMOVE_ARMY_TRANSPORT_DIED,
								   GEA_Player, killer,
								   GEA_End);
		}

		return GEV_HD_Stop;
	}

	u.Kill(cause, killer);
	return GEV_HD_Continue;
}

STDEHANDLER(UnitBeginTurnVisionEvent)
{
	Unit u;
	sint32 player;

	if(!args->GetUnit(0, u))
		return GEV_HD_Continue;

	if(!args->GetPlayer(0, player))
		return GEV_HD_Continue;

	u.BeginTurnVision(player);
	return GEV_HD_Continue;
}

STDEHANDLER(BeginTurnUnitEvent)
{
	Unit u;
	if(!args->GetUnit(0, u))
		return GEV_HD_Continue;

	u.BeginTurn();

	// Auto-explore tick: if the unit is on auto-explore and idle (no pending
	// orders, or arrived at the previous target), re-pick the nearest
	// unexplored tile and re-issue a MOVE_TO.  Clear the flag if the map is
	// fully explored from here.
	UnitData * ud = u.AccessData();
	if (ud && ud->IsExploring()) {
		Army army = u.GetArmy();
		bool needRetarget = !army.IsValid()
		                 || army.AccessData() == NULL
		                 || army.AccessData()->NumOrders() == 0;

		MapPoint cur;
		u.GetPos(cur);
		if (cur == ud->ExploreTarget()) needRetarget = true;

		if (needRetarget) {
			Player * owner = player_Get(u.GetOwner());
			MapPoint target;
			if (owner && owner->FindNearestUnexplored(cur, target)) {
				ud->SetExploreTarget(target);
				if (army.IsValid()) {
					army.AddOrders(UNIT_ORDER_MOVE_TO, target);
				}
			} else {
				// Nothing left to explore from here.
				ud->SetExploring(false);
			}
		}
	}

	return GEV_HD_Continue;
}

STDEHANDLER(AddUnitToArmyEvent)
{
	Unit u;
	Army a;
	CAUSE_NEW_ARMY cause;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetInt(0, (sint32 &)cause)) return GEV_HD_Continue;

	// If transported: Unit has no army
	if(u.Flag(k_UDF_IS_IN_TRANSPORT))
	{
		return GEV_HD_Continue;
	}

	u->ChangeArmy(a, cause);

	Assert(u->GetArmy()->m_id == a->m_id);

	return GEV_HD_Continue;
}

STDEHANDLER(SleepUnitEvent)
{
    Unit u;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;

	u.Sleep();
	return GEV_HD_Continue;
}

STDEHANDLER(WakeUnitEvent)
{
    Unit u;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;

	u.WakeUp();
	return GEV_HD_Continue;
}

STDEHANDLER(EntrenchUnitEvent)
{
    Unit u;
	if(!args->GetUnit(0, u))

		return GEV_HD_Stop;

	u->Entrench();
	return GEV_HD_Continue;
}

STDEHANDLER(DetrenchUnitEvent)
{
	Unit u;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;

	u->Detrench();
	return GEV_HD_Continue;
}

STDEHANDLER(InvestigationEvent)
{
	Unit c;

	if(!args->GetCity(0, c)) return GEV_HD_Continue;

	if (g_gameObservers) g_gameObservers->NotifyCityEspionageDisplay(c);
	return GEV_HD_Continue;
}

STDEHANDLER(InciteRevolutionUnitEvent)
{
	Unit u, c;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;
	if(!args->GetCity(0, c)) return GEV_HD_Continue;

	u.InciteRevolution(c);
	return GEV_HD_Continue;
}

STDEHANDLER(AssassinateRulerUnitEvent)
{
	Unit u, c;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;
	if(!args->GetCity(0, c)) return GEV_HD_Continue;

	u.AssassinateRuler(c);
	return GEV_HD_Continue;
}

STDEHANDLER(PlantNukeUnitEvent)
{

	return GEV_HD_Continue;
}

STDEHANDLER(UndergroundRailwayUnitEvent)
{
	Unit u, c;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;
	if(!args->GetCity(0, c)) return GEV_HD_Continue;

	c.RemoveOneSlave(u.GetOwner());

	Unit hc;
	double distance;
	sint32 r = player_Get(u.GetOwner())->GetNearestCity(u.RetPos(),
													  hc,
													  distance);
	Assert(r);

	if(!r)
		return GEV_HD_Continue;

	if(hc.m_id == 0) {
		Assert(hc.m_id != 0);
		return GEV_HD_Continue;
	}
	u->GetArmy()->AddSpecialActionUsed(u);

	MapPoint cpos;
	hc.GetPos(cpos);
	gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_MakePop,
						   GEA_City, hc.m_id,
						   GEA_End);

	u->GetArmy()->ActionSuccessful(SPECATTACK_FREESLAVES, u, c);

	SlicObject *so;
	so = new SlicObject("163FreeslaveCompleteVictim") ;
	so->AddRecipient(c.GetOwner()) ;
	so->AddCity(c) ;
	slicengine_Get()->Execute(so) ;

	so = new SlicObject("165FreeslaveCompleteAgressor") ;
	so->AddRecipient(u.GetOwner()) ;
	so->AddCity(c);
	so->AddCity(hc) ;
	slicengine_Get()->Execute(so) ;

	return GEV_HD_Continue;
}

STDEHANDLER(InciteUprisingUnitEvent)
{
	Unit u, c;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;
	if(!args->GetCity(0, c)) return GEV_HD_Continue;

	c.DoUprising(UPRISING_CAUSE_INCITED);
	return GEV_HD_Continue;
}

STDEHANDLER(EstablishEmbassyUnitEvent)
{
	Unit u, c;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;
	if(!args->GetCity(0, c)) return GEV_HD_Continue;

	ORDER_RESULT res = u.EstablishEmbassy(c);

	if(res == ORDER_RESULT_SUCCEEDED)
    {
        SlicObject *so = new SlicObject("143EmbassyVictim") ;
        so->AddRecipient(c.GetOwner()) ;
        so->AddCivilisation(u.GetOwner());
		so->AddUnitRecord(u.GetType());
		so->AddCity(c) ;
        slicengine_Get()->Execute(so) ;

        so = new SlicObject("144EmbassyAttacker");
        so->AddRecipient(u.GetOwner()) ;
        so->AddCivilisation(c.GetOwner());
		so->AddUnitRecord(u.GetType());
		so->AddCity(c) ;
        slicengine_Get()->Execute(so) ;
    }
	return GEV_HD_Continue;
}

STDEHANDLER(ThrowPartyUnitEvent)
{
	Unit u, c;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;
	if(!args->GetCity(0, c)) return GEV_HD_Continue;

	ORDER_RESULT res = u->ThrowParty(c,0);

	if(res == ORDER_RESULT_SUCCEEDED)
    {
        SlicObject *so = new SlicObject("149PartyCompleteVictim") ;
        so->AddRecipient(c.GetOwner());
        so->AddPlayer(u.GetOwner());
		so->AddUnitRecord(u.GetType());
		so->AddCity(c) ;
        slicengine_Get()->Execute(so) ;

        so = new SlicObject("150PartyCompleteAttacker");
        so->AddRecipient(u.GetOwner()) ;
        so->AddPlayer(c.GetOwner());
		so->AddUnitRecord(u.GetType());
		so->AddCity(c) ;
        slicengine_Get()->Execute(so) ;
    }
	return GEV_HD_Continue;
}

STDEHANDLER(BioInfectCityUnitEvent)
{
	Unit u, c;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;
	if(!args->GetCity(0, c)) return GEV_HD_Continue;

	SlicObject *so;

	so = new SlicObject("33CrisisCityInfected") ;
	so->AddRecipient(c.GetOwner()) ;
	so->AddCity(c) ;
	slicengine_Get()->Execute(so) ;

	DPRINTF(k_DBG_GAMESTATE, ("Bio infection succeeded\n"));

	so = new SlicObject("10iBioInfectComplete") ;
	so->AddRecipient(c.GetOwner()) ;
	so->AddCivilisation(u.GetOwner()) ;
	so->AddCity(c) ;
	slicengine_Get()->Execute(so) ;

	so = new SlicObject("11iBioInfectComplete") ;
	so->AddRecipient(u.GetOwner()) ;
	so->AddCivilisation(c.GetOwner()) ;
	so->AddCity(c) ;
	slicengine_Get()->Execute(so) ;

	gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_BioInfectCity,
						   GEA_City, c.m_id,
						   GEA_Player, u.GetOwner(),
						   GEA_End);

	u.GetArmy()->ActionSuccessful(SPECATTACK_BIOTERROR, u, c);
	return GEV_HD_Continue;
}

STDEHANDLER(PlagueCityUnitEvent)
{
	Unit u, c;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;
	if(!args->GetCity(0, c)) return GEV_HD_Continue;

	SlicObject *so;

	so = new SlicObject("33CrisisCityInfected") ;
	so->AddRecipient(c.GetOwner()) ;
	so->AddCity(c) ;
	slicengine_Get()->Execute(so) ;

	DPRINTF(k_DBG_GAMESTATE, ("Bio infection succeeded\n"));

	so = new SlicObject("10jPlagueComplete") ;
	so->AddRecipient(c.GetOwner()) ;
	so->AddCivilisation(c.GetOwner()) ;
	so->AddCity(c) ;
	slicengine_Get()->Execute(so) ;

	so = new SlicObject("11jPlagueComplete") ;
	so->AddRecipient(u.GetOwner()) ;
	so->AddCivilisation(u.GetOwner()) ;
	so->AddCity(c) ;
	slicengine_Get()->Execute(so) ;

	gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_PlagueCity,
						   GEA_City, c.m_id,
						   GEA_Player, u.GetOwner(),
						   GEA_End);

	u.GetArmy()->ActionSuccessful(SPECATTACK_BIOTERROR, u, c);
	return GEV_HD_Continue;
}

STDEHANDLER(NanoInfectCityUnitEvent)
{
	Unit u, c;

	if(!args->GetUnit(0, u)) return GEV_HD_Continue;
	if(!args->GetCity(0, c)) return GEV_HD_Continue;

	DPRINTF(k_DBG_GAMESTATE, ("Nano infection succeeded"));

	SlicObject *so;
	so = new SlicObject("911CrisisCityIsNanoInfected") ;
	so->AddRecipient(c.GetOwner()) ;
	so->AddCivilisation(u.GetOwner()) ;
	so->AddCity(c) ;
	slicengine_Get()->Execute(so) ;

	gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_NanoInfectCity,
						   GEA_City, c.m_id,
						   GEA_Player, u.GetOwner(),
						   GEA_End);

	u.GetArmy()->ActionSuccessful(SPECATTACK_NANOTERROR, u, c);
	return GEV_HD_Continue;
}

STDEHANDLER(ConvertCityUnitEvent)
{
	Unit u, c;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;
	if(!args->GetCity(0, c)) return GEV_HD_Continue;

	DPRINTF(k_DBG_GAMESTATE, ("Conversion succeeded\n"));
	if(u.GetDBRec()->GetIsTelevangelist()) {
		gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_ConvertCity,
							   GEA_City, c.m_id,
							   GEA_Player, u.GetOwner(),
							   GEA_Int, CONVERTED_BY_TELEVANGELIST,
							   GEA_End);

	} else {
		gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_ConvertCity,
							   GEA_City, c.m_id,
							   GEA_Player, u.GetOwner(),
							   GEA_Int, CONVERTED_BY_CLERIC,
							   GEA_End);

	}

	{
		SlicObject *so = new SlicObject("151ConvertCompleteVictim") ;
		so->AddRecipient(c.GetOwner()) ;
		so->AddCity(c) ;
		so->AddUnitRecord(u.GetType());
		slicengine_Get()->Execute(so) ;
	}

	u.GetArmy()->ActionSuccessful(SPECATTACK_CONVERTCITY, u, c);
	gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_ContactMade,
						   GEA_Player, c.GetOwner(),
						   GEA_Player, u.GetOwner(),
						   GEA_End);

	return GEV_HD_Continue;
}

STDEHANDLER(ReformCityUnitEvent)
{
	Unit u, c;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;
	if(!args->GetCity(0, c)) return GEV_HD_Continue;

	DPRINTF(k_DBG_GAMESTATE, ("Reformation succeeded\n"));

	gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_UnconvertCity,
						   GEA_City, c.m_id,
						   GEA_End);

	u.GetArmy()->ActionSuccessful(SPECATTACK_REFORMCITY, u, c);

	SlicObject *so = new SlicObject("135ReformCity") ;
	so->AddCity(c);
	so->AddRecipient(c.GetOwner()) ;
	slicengine_Get()->Execute(so) ;
	return GEV_HD_Continue;
}

STDEHANDLER(CreateParkUnitEvent)
{
	Unit u, c;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;
	if(!args->GetCity(0, c)) return GEV_HD_Continue;

	gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_CreatePark,
						   GEA_City, c.m_id,
						   GEA_Player, u.GetOwner(),
						   GEA_End);

	return GEV_HD_Continue;
}

STDEHANDLER(InjoinUnitEvent)
{
	Unit u, c;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;
	if(!args->GetCity(0, c)) return GEV_HD_Continue;

	gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_InjoinCity,
						   GEA_City, c.m_id,
						   GEA_Player, u.GetOwner(),
						   GEA_End);

	return GEV_HD_Continue;
}

STDEHANDLER(NukeCityUnitEvent)
{
	Unit u, c;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;
	if(!args->GetCity(0, c)) return GEV_HD_Continue;

	gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_NukeCity,
						   GEA_City, c,
						   GEA_Player, u.GetOwner(),
						   GEA_End);

	gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_KillUnit,
		GEA_Unit, u,
		GEA_Int, CAUSE_REMOVE_ARMY_NUKE,
		GEA_Player, -1,
		GEA_End);

	return GEV_HD_Continue;
}

STDEHANDLER(NukeLocationUnitEvent)
{
	Unit u;
	MapPoint pos;

	if(!args->GetUnit(0, u)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	sint32 nukeOwner = u.GetOwner();

	gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_KillUnit,
						   GEA_Unit, u.m_id,
						   GEA_Int, CAUSE_REMOVE_ARMY_NUKE,
						   GEA_Player, -1,
						   GEA_End);

	CellUnitList tempKillList;

	SquareIterator it(pos, 1);
	for(it.Start(); !it.End(); it.Next()) {
		Cell *cell = g_theWorld->GetCell(it.Pos());
		sint32 i;
		for(i = 0; i < cell->GetNumUnits(); i++) {
			if(cell->AccessUnit(i).m_id != u.m_id) {
				tempKillList.Insert(cell->AccessUnit(i));
			}
		}
	}


	for(sint32 j = 0; j < tempKillList.Num(); j++) {
		gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_KillUnit,
							   GEA_Unit, tempKillList[j].m_id,
							   GEA_Int, CAUSE_REMOVE_ARMY_NUKE,
							   GEA_Player, nukeOwner,
							   GEA_End);
	}

	gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent,
						   GEV_KillTile,
						   GEA_MapPoint, pos,
						   GEA_End);

	tiledmap_observer::InvalidateMix();
	tiledmap_observer::InvalidateMap();
	tiledmap_observer::Refresh();
	return GEV_HD_Continue;
}

STDEHANDLER(MADLaunchEvent)
{
	Unit u;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;

	gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_MovePathOrder,
						   GEA_Army, u->GetArmy(),
						   GEA_MapPoint, u->GetTargetCity().RetPos(),
						   GEA_End);
	return GEV_HD_Continue;
}

STDEHANDLER(DisbandUnitEvent)
{
	Unit u;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;

	u.Kill(CAUSE_REMOVE_ARMY_DISBANDED, -1);
	return GEV_HD_Continue;
}

STDEHANDLER(LaunchUnitEvent)
{
	Unit u;
	MapPoint pos;

	if(!args->GetUnit(0, u)) return GEV_HD_Continue;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	if(!player_Get(u.GetOwner())) return GEV_HD_Continue;

	sint32 spaceTurns;
	if(!u.GetDBRec()->GetSpaceLaunch(spaceTurns))
		return GEV_HD_Continue;

	if(spaceTurns > 0) {
		g_theWorld->RemoveUnitReference(u.RetPos(), u);
		u->RemoveUnitVision();
		u.SetFlag(k_UDF_HAS_LEFT_MAP);
		u.SetFlag(k_UDF_IN_SPACE);

		u.GetArmy()->SetReentry(spaceTurns, pos);
		render_observer::AddHide(u);
	} else {
		gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_Reentry,
							   GEA_Army, u.GetArmy().m_id,
							   GEA_End);
	}

	return GEV_HD_Continue;
}

STDEHANDLER(SetTargetEvent)
{
	Unit u;
	Unit city;

	if(!args->GetUnit(0, u)) return GEV_HD_Continue;
	if(!args->GetCity(0, city)) return GEV_HD_Continue;

	u->SetTargetCity(city);
	return GEV_HD_Continue;
}

STDEHANDLER(ClearTargetEvent)
{
	Unit u;

	if(!args->GetUnit(0, u)) return GEV_HD_Continue;

	u->SetTargetCity(Unit());
	return GEV_HD_Continue;
}

STDEHANDLER(ActivateAllUnitsEvent)
{
	MapPoint pos;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	sint32 i;
	Cell *cell = g_theWorld->GetCell(pos);
	for(i = 0; i < cell->GetNumUnits(); i++) {
		if(cell->AccessUnit(i).IsEntrenched()) {
			gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent,
								   GEV_DetrenchUnit,
								   GEA_Unit, cell->AccessUnit(i),
								   GEA_End);
		} else if(cell->AccessUnit(i).IsAsleep()) {
			gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent,
								   GEV_WakeUnit,
								   GEA_Unit, cell->AccessUnit(i),
								   GEA_End);
		}
	}
	return GEV_HD_Continue;
}

STDEHANDLER(SetUnloadMovementUnitEvent)
{
	Unit u;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;

	u.SetMovementPoints(0.0);
	if(g_network.IsHost()) {
		g_network.Block(u.GetOwner());
		g_network.Enqueue(new NetInfo(NET_INFO_CODE_SET_MOVEMENT_TO_ZERO, u.m_id));
		g_network.Unblock(u.GetOwner());
	}
	return GEV_HD_Continue;
}

STDEHANDLER(UpgradeUnit)
{
	Unit u;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;
	sint32 type  = 0;
	sint32 costs = 0;

	if(u->CanUpgrade(type, costs))
	{
		u->Upgrade(type, costs);
	}

	return GEV_HD_Continue;
}

void unitevent_Initialize()
{
	gevmanager_Get()->AddCallback(GEV_KillUnit, GEV_PRI_Primary, &s_KillUnitEvent);
	gevmanager_Get()->AddCallback(GEV_KillCity, GEV_PRI_Primary, &s_KillUnitEvent);

	gevmanager_Get()->AddCallback(GEV_UnitBeginTurnVision, GEV_PRI_Primary, &s_UnitBeginTurnVisionEvent);
	gevmanager_Get()->AddCallback(GEV_BeginTurnUnit, GEV_PRI_Primary, &s_BeginTurnUnitEvent);

	gevmanager_Get()->AddCallback(GEV_AddUnitToArmy, GEV_PRI_Primary, &s_AddUnitToArmyEvent);

	gevmanager_Get()->AddCallback(GEV_SleepUnit, GEV_PRI_Primary, &s_SleepUnitEvent);
	gevmanager_Get()->AddCallback(GEV_WakeUnit, GEV_PRI_Primary, &s_WakeUnitEvent);
	gevmanager_Get()->AddCallback(GEV_EntrenchUnit, GEV_PRI_Primary, &s_EntrenchUnitEvent);
	gevmanager_Get()->AddCallback(GEV_DetrenchUnit, GEV_PRI_Primary, &s_DetrenchUnitEvent);
	gevmanager_Get()->AddCallback(GEV_DisplayInvestigationWindow, GEV_PRI_Primary, &s_InvestigationEvent);
	gevmanager_Get()->AddCallback(GEV_InciteRevolutionUnit, GEV_PRI_Primary, &s_InciteRevolutionUnitEvent);
	gevmanager_Get()->AddCallback(GEV_AssassinateRulerUnit, GEV_PRI_Primary, &s_AssassinateRulerUnitEvent);

	gevmanager_Get()->AddCallback(GEV_PlantNukeUnit, GEV_PRI_Primary, &s_PlantNukeUnitEvent);
	gevmanager_Get()->AddCallback(GEV_UndergroundRailwayUnit, GEV_PRI_Primary, &s_UndergroundRailwayUnitEvent);
	gevmanager_Get()->AddCallback(GEV_InciteUprisingUnit, GEV_PRI_Primary, &s_InciteUprisingUnitEvent);
	gevmanager_Get()->AddCallback(GEV_EstablishEmbassyUnit, GEV_PRI_Primary, &s_EstablishEmbassyUnitEvent);
	gevmanager_Get()->AddCallback(GEV_ThrowPartyUnit, GEV_PRI_Primary, &s_ThrowPartyUnitEvent);
	gevmanager_Get()->AddCallback(GEV_BioInfectCityUnit, GEV_PRI_Primary, &s_BioInfectCityUnitEvent);
	gevmanager_Get()->AddCallback(GEV_PlagueCityUnit, GEV_PRI_Primary, &s_PlagueCityUnitEvent);
	gevmanager_Get()->AddCallback(GEV_NanoInfectCityUnit, GEV_PRI_Primary, &s_NanoInfectCityUnitEvent);
	gevmanager_Get()->AddCallback(GEV_ConvertCityUnit, GEV_PRI_Primary, &s_ConvertCityUnitEvent);
	gevmanager_Get()->AddCallback(GEV_ReformCityUnit, GEV_PRI_Primary, &s_ReformCityUnitEvent);
	gevmanager_Get()->AddCallback(GEV_CreateParkUnit, GEV_PRI_Primary, &s_CreateParkUnitEvent);
	gevmanager_Get()->AddCallback(GEV_InjoinUnit, GEV_PRI_Primary, &s_InjoinUnitEvent);

	gevmanager_Get()->AddCallback(GEV_NukeCityUnit, GEV_PRI_Primary, &s_NukeCityUnitEvent);
	gevmanager_Get()->AddCallback(GEV_NukeLocationUnit, GEV_PRI_Primary, &s_NukeLocationUnitEvent);
	gevmanager_Get()->AddCallback(GEV_DisbandUnit, GEV_PRI_Primary, &s_DisbandUnitEvent);
	gevmanager_Get()->AddCallback(GEV_LaunchUnit, GEV_PRI_Primary, &s_LaunchUnitEvent);
	gevmanager_Get()->AddCallback(GEV_SetTarget, GEV_PRI_Primary, &s_SetTargetEvent);
	gevmanager_Get()->AddCallback(GEV_ClearTarget, GEV_PRI_Primary, &s_ClearTargetEvent);
	gevmanager_Get()->AddCallback(GEV_MADLaunch, GEV_PRI_Primary, &s_MADLaunchEvent);

	gevmanager_Get()->AddCallback(GEV_ActivateAllUnits, GEV_PRI_Primary, &s_ActivateAllUnitsEvent);

	gevmanager_Get()->AddCallback(GEV_SetUnloadMovementUnit, GEV_PRI_Primary, &s_SetUnloadMovementUnitEvent);
	gevmanager_Get()->AddCallback(GEV_UpgradeUnit, GEV_PRI_Primary, &s_UpgradeUnit);
}

void unitevent_Cleanup()
{
}
