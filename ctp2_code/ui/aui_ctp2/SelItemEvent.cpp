//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : The events of the selected items.
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
// - A just founded city is only seleceted if the player is visible and not a robot. (23-Feb-2008 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/events/GameEventUser.h"
#include "ui/aui_ctp2/SelItemEvent.h"
#include "gs/gameobj/Events.h"
#include "ui/aui_ctp2/SelItem.h"
#include "ui/interface/controlpanelwindow.h"
#include "ui/aui_ctp2/ctp2_Static.h"

STDEHANDLER(SelItemArmyMovedEvent)
{
	Army a;
	if(!args->GetArmy(0, a))
		return GEV_HD_Continue;

	selitem_Get()->ArmyMovedCallback(a);

	return GEV_HD_Continue;
}

STDEHANDLER(SelItemSettleEvent)
{
	sint32 res;
	sint32 pl;
	if(!args->GetPlayer(0, pl)) return GEV_HD_Continue;
	if(pl != selitem_Get()->GetVisiblePlayer())
		return GEV_HD_Continue;

	if(!args->GetInt(0, res)) return GEV_HD_Continue;

	if(res) {
		selitem_Get()->Deselect(selitem_Get()->GetVisiblePlayer());
	}
	return GEV_HD_Continue;
}

STDEHANDLER(SelItemCreateCityEvent)
{
	sint32 pl;
	Unit city;
	if(!args->GetPlayer(0, pl)) return GEV_HD_Continue;
	if(!args->GetCity(0, city)) return GEV_HD_Continue;

	if( pl == selitem_Get()->GetVisiblePlayer()
	&& !player_Get(pl)->IsRobot()
	){
		selitem_Get()->SetSelectUnit(city);
	}

	return GEV_HD_Continue;
}

STDEHANDLER(SelItemEntrenchUnitEvent)
{
	Unit u;
	if(!args->GetUnit(0, u)) return GEV_HD_Continue;

	Army selArmy;
	if(selitem_Get()->GetSelectedArmy(selArmy)) {
		if(selArmy.m_id == u.GetArmy().m_id && !CanAutoSelect(selArmy)) {
			selitem_Get()->NextItem();
		}
	}
	return GEV_HD_Continue;
}

STDEHANDLER(SelItemCantMoveYetEvent)
{
	Army a;
	if(!args->GetArmy(0, a)) return GEV_HD_Continue;

	Army selArmy;
	if(selitem_Get()->GetSelectedArmy(selArmy)) {
		if(selArmy.m_id == a.m_id && !CanAutoSelect(selArmy)) {
			selitem_Get()->NextItem();
		}
	}
	return GEV_HD_Continue;
}

STDEHANDLER(SelItemArmySelectedEvent)
{
	Army a;
	if(!args->GetArmy(0, a)) return GEV_HD_Stop;

	return GEV_HD_Continue;
}

void selecteditemevent_Initialize()
{
	gevmanager_Get()->AddCallback(GEV_MoveUnits, GEV_PRI_Post, &s_SelItemArmyMovedEvent);
	gevmanager_Get()->AddCallback(GEV_Settle, GEV_PRI_Post, &s_SelItemSettleEvent);
	gevmanager_Get()->AddCallback(GEV_CreateCity, GEV_PRI_Post, &s_SelItemCreateCityEvent);
	gevmanager_Get()->AddCallback(GEV_EntrenchUnit, GEV_PRI_Post, &s_SelItemEntrenchUnitEvent);

	gevmanager_Get()->AddCallback(GEV_CantMoveYet, GEV_PRI_Post, &s_SelItemCantMoveYetEvent);
	gevmanager_Get()->AddCallback(GEV_ArmySelected, GEV_PRI_Primary, &s_SelItemArmySelectedEvent);
}

void selecteditemevent_Cleanup()
{
}
