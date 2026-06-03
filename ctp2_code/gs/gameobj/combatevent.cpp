//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C/C++ header
// Description  : Combat events
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
// - Fixed a memory leak concerning combat_Get(). (7-Nov-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/gameobj/combatevent.h"
#include "gs/gameobj/Events.h"
#include "gs/gameobj/CTP2Combat.h"
#include "gs/core/battle_observer.h"
#include "gs/events/GameEventUser.h"
#include "gs/events/GameEventManager.h"
#include "gs/gameobj/Army.h"
#include "gs/world/MapPoint.h"
#include "gs/world/World.h"
#include "gs/world/Cell.h"
#include "gs/world/cellunitlist.h"
#include "gs/gameobj/ArmyData.h"
#include "gs/utility/Globals.h"

STDEHANDLER(RunCombatEvent)
{
	Army army;
	MapPoint pos;
	sint32 attacker;
	sint32 defender;

	if(!args->GetArmy(0, army))
		return(GEV_HD_Continue);
	if(!args->GetPos(0, pos))
		return(GEV_HD_Continue);
	if(!args->GetPlayer(0, attacker))
		return(GEV_HD_Continue);
	if(!args->GetPlayer(1, defender))
		return(GEV_HD_Continue);
	if(!combat_Get())
		return(GEV_HD_Continue);

	bool const  isDoneAtStart   = combat_Get()->IsDone();
	bool const  playAnimations  =
	    isDoneAtStart ? false : combat_Get()->ResolveOneRound();

	if (isDoneAtStart)
	{
		// No action: probably a place holder for the final user click.
	}
	else if (combat_Get()->IsDone())
	{
		gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent,
		                       GEV_BattleAftermath,
		                       GEA_Army,     army,
		                       GEA_MapPoint, pos,
		                       GEA_Unit,     army[0],
		                       GEA_Unit,     world_Get()->GetCell(pos)->AccessUnit(0),
		                       GEA_Player,   attacker,
		                       GEA_Player,   defender,
		                       GEA_Int,      1,
		                       GEA_End
		                      );
		combat_Get()->KillUnits(GEV_INSERT_AfterCurrent);
	}
	else
	{
		gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent,
		                       GEV_RunCombat,
		                       GEA_Army,     army,
		                       GEA_MapPoint, pos,
		                       GEA_Player,   attacker,
		                       GEA_Player,   defender,
		                       GEA_End
		                      );
	}

	return (playAnimations && combat_Get()->IsBattleActive())
	       ? GEV_HD_NeedUserInput
	       : GEV_HD_Continue;
}

STDEHANDLER(StartCombatEvent)
{
	Army a;
	MapPoint p;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, p)) return GEV_HD_Continue;

	if (combat_Get())
	{
		// Close previous screen - if still open
		battle_observer::CloseBattleView();
		combat_Get()->DeactivateBattle();
		delete combat_Get();
		combat_Set(nullptr);
	}

	CellUnitList defender;
	world_Get()->GetCell(p)->GetArmy(defender);

	Assert(a.Num() > 0);
	Assert(defender.Num() > 0);
	if (a.Num() > 0 && defender.Num() > 0 &&
	    a.GetOwner() != defender.GetOwner()
	   )
	{
		combat_Set(new CTP2Combat(k_COMBAT_WIDTH, k_COMBAT_HEIGHT, *a.AccessData(), defender));
	}

	return GEV_HD_Continue;
}

void combatevent_Initialize()
{
	gevmanager_Get()->AddCallback(GEV_RunCombat, GEV_PRI_Primary, &s_RunCombatEvent);
	gevmanager_Get()->AddCallback(GEV_StartCombat, GEV_PRI_Primary, &s_StartCombatEvent);
}

void combatevent_Cleanup()
{
	delete combat_Get();
	combat_Set(nullptr);
}
