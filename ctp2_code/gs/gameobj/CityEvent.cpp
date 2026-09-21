//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : City Game Events
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
// - Readded possibility to gain an advance from a just captured
//   city, by Martin G�hmann. However with or without the change
//   the CaptureCityEvent leaks, maybe a problem of SlicObject.
// - Prevented crash when reporting completion of the Solaris project.
// - Corrected memory leaks for city captures.
// - Corrected memory leaks and invalid arguments for Gaia Controller messages.
// - Corrected message recipients for the Gaia Controller messages.
// - added check to make sure city pop is greater than 1 before city capture options
// - added city leaves ruins options
// - Replaced old const database by new one. (5-Aug-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/utility/TurnCnt.h"

#include <memory>

#include "gs/gameobj/Events.h"
#include "gs/gameobj/CityEvent.h"
#include "gs/events/GameEventUser.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/player.h"
#include "gs/core/render_observer.h"
#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicSegment.h"
#include "gs/slic/SlicObject.h"
#include "gs/slic/QuickSlic.h"
#include "gs/outcom/AICause.h"
#include "gs/core/player_view.h"
#include "gs/slic/SlicObject.h"
#include "gs/utility/RandGen.h"
#include "ConstRecord.h"
#include "gs/core/audio_types.h"
#include "gs/gameobj/UnitData.h"
#include "gs/gameobj/citydata.h"
#include "gs/utility/UnitDynArr.h"
#include "gs/gameobj/citydata.h"
#include "gs/gameobj/BldQue.h"
#include "BuildingRecord.h"
#include "WonderRecord.h"
#include "gs/database/profileDB.h"
#include "net/general/network.h"
#include "net/general/net_info.h"

#include "gs/gameobj/buildingutil.h"
#include "gs/gameobj/wonderutil.h"
#include "UnitRecord.h"
#include "gs/gameobj/UnitPool.h"
#include "gs/world/World.h"
#include "gs/world/Cell.h"
#include "gs/core/tiledmap_observer.h"
#include "gs/gameobj/unitutil.h"
#include "gs/gameobj/ArmyData.h"
#include "gs/gameobj/gaiacontroller.h"

#include "AdvanceRecord.h"
#include "gs/gameobj/Happy.h" //EMOD
#include "TerrainImprovementRecord.h"
#include "TerrainRecord.h"
#include "gs/gameobj/terrainutil.h"
#include "gs/gameobj/TerrImprovePool.h"

extern void player_ActivateSpaceButton(sint32 pl);

STDEHANDLER(CaptureCityEvent)
{
	Unit city;
	sint32 newOwner;
	sint32 cause;

	if(!args->GetCity(0, city))
		return GEV_HD_Continue;

	if(!args->GetPlayer(0, newOwner))
		return GEV_HD_Continue;

	if(!args->GetInt(0, cause))
		return GEV_HD_Continue;

	sint32 const    originalOwner   = city.GetOwner();
	MapPoint        pos;
    city.GetPos(pos);

	//EMOD capitol stuff is in army event shouldn't it go here?
	if (city.CD()->IsCapitol()) {
		sint32 sep = (player_Get(originalOwner)->m_all_cities->Num()) / 2;
		for (sint32 j = 0; j < sep; ++j) {
			Unit revcity = player_Get(originalOwner)->m_all_cities->Get(j) ;
			CityData	*revcityData = revcity.AccessData()->GetCityData() ;
			revcityData->GetHappy()->ForceRevolt() ;
		}
	}

	city.ResetCityOwner(newOwner, TRUE, (CAUSE_REMOVE_CITY) cause); //this is where capitol is destroyed. unitdata::resetcityowner

	if (city.GetData()->GetCityData()->PopCount() < 1)
    {
		gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_KillCity,
			GEA_City, city,
			GEA_Int, CAUSE_REMOVE_ARMY_ATTACKED,
			GEA_Player, newOwner,
			GEA_End);
	//added cities leaving ruin options requires a tileimp with the flag IsCityRuin to be placed
			if (profiledb_Get()->GetCityLeavesRuins()){
				for (sint32 imp = 0; imp < g_theTerrainImprovementDB->NumRecords(); imp++) {
					const TerrainImprovementRecord *rec = g_theTerrainImprovementDB->Get(imp);
					if (rec->GetIsCityRuin()){
						player_Get(newOwner)->CreateSpecialImprovement(imp, pos, 0);
					}
				}
			}
	}
	else if (city.IsValid())
	{
		// Added by Maq - Reset shield store for captured cities.
		city.CD()->SetShieldstore(0);

		if (city.GetOwner() == player_view::VisiblePlayer())
		{
			render_observer::AddCenterMap(pos);
		}

		if (newOwner == player_view::VisiblePlayer())
		{
			player_view::SetSelectCity(city);
		}

		if (city.AccessData()->CountSlaves() > 0)
		{
			auto so = std::make_unique<SlicObject>("20IAFreeSlaves");
			so->AddRecipient(newOwner);
			so->AddCity(city);
			slicengine_Get()->Execute(std::move(so));

			// The AI frees slaves if it has no slaves or any units that can catch slaves
			if(player_Get(newOwner)->IsRobot())
			{
				bool freeSlaves = true;

				sint32 i;

				for(i = 0; i < player_Get(newOwner)->m_all_cities->Num(); i++)
				{
					Unit newOwnerCity = player_Get(newOwner)->GetCityFromIndex(i);

					if(newOwnerCity != city && newOwnerCity.AccessData()->CountSlaves() > 0)
					{
						freeSlaves = false;
						break;
					}
				}

				if(freeSlaves)
				{
					for(i = 0; i < player_Get(newOwner)->m_all_units->Num(); i++)
					{
						const UnitRecord *rec = player_Get(newOwner)->m_all_units->Access(i).GetDBRec();
						if
						  (
						       rec->HasSlaveRaids()
						    || rec->HasSettlerSlaveRaids()
						  )
						{
							freeSlaves = false;
							break;
						}
					}
				}

				if(freeSlaves)
				{
					city->FreeSlaves();
				}
			}
		}

		if (
			(profiledb_Get()->IsCityCaptureOptions())
			//(profiledb_Get()->GetValueByName("CityCaptureOptions"))
		&& (city.GetData()->GetCityData()->PopCount() > 0)
		){
//EMOD Capture city options
            /// @todo Check impact: this could be considered a human cheat.
            ///       The AI is unable to select raze (when over the city cap).
		    auto so = std::make_unique<SlicObject>("999CITYCAPTUREOPTIONS");
		    so->AddRecipient(newOwner);
		    so->AddCity(city);
		    slicengine_Get()->Execute(std::move(so));
//END EMOD
        }
        else
        {
            auto so = std::make_unique<SlicObject>("911CityNewOwner");
            so->AddRecipient(originalOwner);
            so->AddPlayer(originalOwner);
            so->AddPlayer(newOwner);
            so->AddCity(city);
            slicengine_Get()->Execute(std::move(so));
        }

		if(civrand().Next(100) <
		   g_theConstDB->Get(0)->GetCaptureCityAdvanceChance() * 100) {
			//Added by Martin G�hmann to allow city advance gaining from
			//a captured city.

			//Check if there are any advances to steal:
			sint32 num;
			const std::vector<uint8_t> canSteal =
				player_Get(newOwner)->m_advances->CanAskFor(
				  player_Get(originalOwner)->m_advances.get(), num);
			if(num > 0){
				sint32 i;
				sint32 count = 0;
				sint32 which = civrand().Next(num);

				for(i = 0; i < g_theAdvanceDB->NumRecords(); i++) {
					if(canSteal[i]) {
						if(which == count) {
							player_Get(newOwner)->m_advances->GiveAdvance(i, CAUSE_SCI_COMBAT);
							auto so = std::make_unique<SlicObject>("99AdvanceFromCapturingCity");
							so->AddCivilisation(newOwner);
							so->AddCivilisation(originalOwner);
							so->AddRecipient(newOwner);
							so->AddCity(city);
							so->AddAdvance(i);
							slicengine_Get()->Execute(std::move(so));

							so = std::make_unique<SlicObject>("99aAdvanceFromCapturingCityVictim");
							so->AddCivilisation(originalOwner);
							so->AddCivilisation(newOwner);
							so->AddRecipient(originalOwner);
							so->AddCity(city);
							so->AddAdvance(i);
							slicengine_Get()->Execute(std::move(so));

							break;
						}
						count++;
					}
				}
				Assert(i < g_theAdvanceDB->NumRecords());
			}
		}
		Assert(player_Get(newOwner));
		player_Get(newOwner)->FulfillCaptureCityAgreement(city);
		slicengine_Get()->RunCityCapturedTriggers(newOwner, originalOwner,
		                                      city);

		if(city.GetVisibility() & (1 << player_view::VisiblePlayer()))
		{
			sint32 soundID = gamesounds_GetGameSoundID(GAMESOUNDS_CITYCONQUERED);
			if (soundID != 0)
				render_observer::AddPlaySound(soundID, city.RetPos());
		}
	}
	return GEV_HD_Continue;
}

STDEHANDLER(CityTurnPreProductionEvent)
{
	Unit city;
	if(!args->GetCity(0, city))
		return GEV_HD_Continue;

	city.CalcHappiness(player_Get(city.GetOwner())->m_virtualGoldSpent, TRUE);
	city.CheckRiot();
	return GEV_HD_Continue;
}

STDEHANDLER(CityBeginTurnEvent)
{
	Unit city;
	if(!args->GetCity(0, city))
		return GEV_HD_Continue;

    UnitDynamicArray dead;
	city.BeginTurnCity(dead);
	dead.KillList(CAUSE_REMOVE_ARMY_UNKNOWN, -1);
	return GEV_HD_Continue;
}

STDEHANDLER(CityBeginTurnVisionEvent)
{
	Unit city;
	sint32 player;

	if(!args->GetCity(0, city))
		return GEV_HD_Continue;

	if(!args->GetPlayer(0, player))
		return GEV_HD_Continue;

	city.BeginTurnVision(player);
	return GEV_HD_Continue;
}

STDEHANDLER(CityBuildFrontEvent)
{
	Unit city;
	if(!args->GetCity(0, city))
		return GEV_HD_Continue;

	city.CD()->BuildFront();

	// EMOD for popcoststo build attempt to fix 6-01-2006 works but you must have that pop number to build then disband
	if (city.CD()->GetBuildQueue()->m_popcoststobuild_pending) {
		auto so = std::make_unique<SlicObject>("111BuildingSettlerCityOfOne");
		so->AddCity(city);
		so->AddUnitRecord(city.CD()->GetBuildQueue()->GetHead()->m_type);
		so->AddRecipient(city.GetOwner());
		slicengine_Get()->Execute(std::move(so));
	}
	// End EMOD

	if (city.CD()->GetBuildQueue()->m_settler_pending) {
		if (city.CD()->PopCount() == 1) {  //Isn't this already reflected in bldque.cpp(407)?
			auto so = std::make_unique<SlicObject>("111BuildingSettlerCityOfOne");
			so->AddCity(city);
			so->AddUnitRecord(city.CD()->GetBuildQueue()->GetHead()->m_type);
			so->AddRecipient(city.GetOwner());
			slicengine_Get()->Execute(std::move(so));
		}
	}

	return GEV_HD_Continue;
}

STDEHANDLER(CityCreateUnitEvent)
{
	Unit homeCity;
	if(!args->GetCity(0, homeCity)) {

		return GEV_HD_Continue;
	}

	Unit unit;
	if(!args->GetUnit(0, unit))
		return GEV_HD_Continue;

	homeCity.CD()->GetBuildQueue()->FinishCreatingUnit(unit);

	return GEV_HD_Continue;
}

STDEHANDLER(CityBuildUnitEvent)
{
	Unit city;
	sint32 type;

	if(!args->GetCity(0, city)) return GEV_HD_Continue;
	if(!args->GetInt(0, type)) return GEV_HD_Continue;

	city.BuildUnit(type);
	return GEV_HD_Continue;
}

STDEHANDLER(CityBuildBuildingEvent)
{
	Unit city;
	sint32 type;
	if(!args->GetCity(0, city)) return GEV_HD_Continue;
	if(!args->GetInt(0, type)) return GEV_HD_Continue;

	city.BuildImprovement(type);
	return GEV_HD_Continue;
}

STDEHANDLER(CityBuildWonderEvent)
{
	Unit city;
	sint32 type;
	if(!args->GetCity(0, city)) return GEV_HD_Continue;
	if(!args->GetInt(0, type)) return GEV_HD_Continue;

	city.BuildWonder(type);
	return GEV_HD_Continue;
}

STDEHANDLER(ZeroProductionEvent)
{
	Unit city;
	if(!args->GetCity(0, city)) return GEV_HD_Continue;

	city.CD()->SetShieldstore(0);
	return GEV_HD_Continue;
}

STDEHANDLER(RollOverProductionEvent)
{
	Unit city;
	sint32 shields;
	if(!args->GetCity(0, city)) return GEV_HD_Continue;
	if(!args->GetInt(0, shields)) return GEV_HD_Continue;

	city.CD()->SetShieldstore(shields);
	return GEV_HD_Continue;
}

STDEHANDLER(MakePopEvent)
{
	Unit city;
	sint32 origPlayer;

	if(!args->GetCity(0, city)) return GEV_HD_Continue;
	if(!args->GetPlayer(0, origPlayer))
		origPlayer = -1;

// EMOD to ADD City population caps
//	if(g_theDifficultyDB->Get(gamesettings_Get()->GetDifficulty())->GetCityPopCap() {
//	sint32 PopCap = g_theDifficultyDB->Get(gamesettings_Get()->GetDifficulty())->GetCityPopCap()
//	sint32 PopCapIncrease = buildingutil_GetIncreasesPopCap(city.CD->GetEffectiveBuildings());
//			PopCap += PopCapIncrease;
//		if cd.Popcount() < PopCap {
//			city.CD()->ChangePopulation(1);
//		}
//	} else {

	city.CD()->ChangePopulation(1);
	if (origPlayer >= 0) {
		city.CD()->AddSlaveBit(origPlayer);
		city.CD()->ChangeSpecialists(POP_SLAVE, 1);
	}

	return GEV_HD_Continue;
}

STDEHANDLER(KillPopEvent)
{
	Unit city;

	if(!args->GetCity(0, city)) return GEV_HD_Continue;

	city.CD()->ChangePopulation(-1);

	return GEV_HD_Continue;
}

STDEHANDLER(FinishUprisingEvent)
{
	Army sa;
	Unit city;
	sint32 cause;

	if(!args->GetCity(0, city)) return GEV_HD_Continue;
	if(!args->GetArmy(0, sa)) return GEV_HD_Continue;
	if(!args->GetInt(0, cause)) return GEV_HD_Continue;

	city.CD()->FinishUprising(sa, UPRISING_CAUSE(cause));
	return GEV_HD_Continue;
}

STDEHANDLER(CleanupUprisingEvent)
{
	Army sa;
	Unit city;

	args->GetArmy(0, sa);
	if(!args->GetCity(0, city)) {
		if(sa.IsValid()) {
			sa->DecrementDontKillCount();
		}
		return GEV_HD_Continue;
	}

	city.CD()->CleanupUprising(sa);
	if(sa.IsValid()) {
		sa->DecrementDontKillCount();
	}
	return GEV_HD_Continue;
}

STDEHANDLER(NukeCityEvent)
{
	Unit c;
	sint32 nuker;

	if(!args->GetCity(0, c)) return GEV_HD_Continue;
	if(!args->GetPlayer(0, nuker)) return GEV_HD_Continue;

	if(player_Get(c.GetOwner())) {
		sint32 i;

		for(i = 0; i < player_Get(c.GetOwner())->m_all_units->Num(); i++) {

			Unit u = player_Get(c.GetOwner())->m_all_units->Access(i);

			if(!u.GetDBRec()->HasNuclearAttack())
				continue;

			if(!unitpool_Get()->IsValid(u->GetTargetCity()))
				continue;

			if(u->GetTargetCity().GetOwner() != nuker)
				continue;

			unitutil_ExecuteMadLaunch(u);
		}
	}

	if(network_Get().IsHost() && nuker == player_view::CurPlayer()) {

		network_Get().Block(nuker);
	}

	UnitDynamicArray killList;
	c.GetNuked(killList);

	if(network_Get().IsHost() && nuker == player_view::CurPlayer()) {
		network_Get().Unblock(nuker);
	}

	sint32 j;
	for(j = 0; j < killList.Num(); j++) {
		if(killList[j].DeathEffectsHappy()) {
			player_Get(killList[j].GetOwner())->RegisterLostUnits(1, c.RetPos(), DEATH_EFFECT_CALC);
		}
	}

	for(j = 0; j < killList.Num(); j++) {
		gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent, GEV_KillUnit,
		                       GEA_Unit, killList[j],
		                       GEA_Int, CAUSE_REMOVE_ARMY_NUKE,
		                       GEA_Player, nuker,
		                       GEA_End);
	}

	return GEV_HD_Continue;
}

STDEHANDLER(MakeFranchiseEvent)
{
	Unit c;
	sint32 p;

	if(!args->GetCity(0, c)) return GEV_HD_Continue;
	if(!args->GetPlayer(0, p)) return GEV_HD_Continue;

	c->MakeFranchise(p);
	return GEV_HD_Continue;
}

STDEHANDLER(SlaveRaidCityEvent)
{

	Unit city;
	if(!args->GetCity(0, city)) return GEV_HD_Continue;

	city.CD()->ChangePopulation(-1);

	return GEV_HD_Continue;
}

STDEHANDLER(BioInfectCityEvent)
{
	Unit c;
	sint32 pl;

	if(!args->GetCity(0, c)) return GEV_HD_Continue;
	if(!args->GetPlayer(0, pl)) return GEV_HD_Continue;

	c.BioInfect(pl);
	return GEV_HD_Continue;
}

STDEHANDLER(PlagueCityEvent)
{
	Unit c;
	sint32 pl;

	if(!args->GetCity(0, c)) return GEV_HD_Continue;
	if(!args->GetPlayer(0, pl)) return GEV_HD_Continue;

	c.CD()->Plague(pl);
	return GEV_HD_Continue;
}

STDEHANDLER(NanoInfectCityEvent)
{
	Unit c;
	sint32 pl;

	if(!args->GetCity(0, c)) return GEV_HD_Continue;
	if(!args->GetPlayer(0, pl)) return GEV_HD_Continue;

	c.NanoInfect(pl);
	return GEV_HD_Continue;
}

STDEHANDLER(ConvertCityEvent)
{
	Unit c;
	sint32 pl;
	sint32 by;
	if(!args->GetCity(0, c)) return GEV_HD_Continue;
	if(!args->GetPlayer(0, pl)) return GEV_HD_Continue;
	if(!args->GetInt(0, by)) return GEV_HD_Continue;

	c.ConvertTo(pl, (CONVERTED_BY)by);
	return GEV_HD_Continue;
}

STDEHANDLER(UnconvertCityEvent)
{
	Unit c;
	if(!args->GetCity(0, c)) return GEV_HD_Continue;

	c.Unconvert();
	return GEV_HD_Continue;
}

STDEHANDLER(AddHappyTimerEvent)
{
	Unit c;
	sint32 turns;
	sint32 amount;
	sint32 reason;

	if(!args->GetCity(0, c)) return GEV_HD_Continue;
	if(!args->GetInt(0, turns)) return GEV_HD_Continue;
	if(!args->GetInt(1, amount)) return GEV_HD_Continue;
	if(!args->GetInt(2, reason)) return GEV_HD_Continue;

	c.AddHappyTimer(turns, amount, (HAPPY_REASON)reason);
	return GEV_HD_Continue;
}

STDEHANDLER(CreateParkEvent)
{
	Unit c;
	sint32 pl;
	if(!args->GetCity(0, c)) return GEV_HD_Continue;
	if(!args->GetPlayer(0, pl)) return GEV_HD_Continue;

	c.CityToPark(pl);

	return GEV_HD_Continue;
}

STDEHANDLER(InjoinCityEvent)
{
	Unit c;
	sint32 pl;
	if(!args->GetCity(0, c)) return GEV_HD_Continue;
	if(!args->GetPlayer(0, pl)) return GEV_HD_Continue;

	c.Injoin(pl);

	return GEV_HD_Continue;
}

STDEHANDLER(CreateBuildingEvent)
{
	Unit c;
	sint32		building;
	sint32		player;
	std::unique_ptr<SlicObject> so;
	SlicSegment *seg;

	if(!args->GetCity(0, c)) return GEV_HD_Continue;
	if(!args->GetInt(0, building)) return GEV_HD_Continue;

	c.CD()->AddImprovement(building);

	Unit u;
	c.CD()->GetBuildQueue()->FinishBuildFront(u);

	player = c.GetOwner();
	if(!player_Get(player)) {
		return GEV_HD_Continue;
	}
	if(player_Get(player)->GetGaiaController()->HasMaxSatsBuilt()) {
		seg = slicengine_Get()->GetSegment("GCMaxSatsReached");
		if(seg && !seg->TestLastShown(player, 10000, turn_Get()->GetRound())) {
			so = std::make_unique<SlicObject>("GCMaxSatsReached");
			so->AddRecipient(player);
			so->AddPlayer(player);
			slicengine_Get()->Execute(std::move(so));
		}
	}

	if(player_Get(player)->GetGaiaController()->HasMinSatsBuilt()) {
		seg = slicengine_Get()->GetSegment("GCMinSatsReachedUs");
		if (seg && !seg->TestLastShown(player, 10000, turn_Get()->GetRound()))
		{
			so = std::make_unique<SlicObject>("GCMinSatsReachedUs");
			so->AddPlayer(player);
			so->AddRecipient(player);
			slicengine_Get()->Execute(std::move(so));

			so	= std::make_unique<SlicObject>("GCMinSatsReachedThem");
			so->AddPlayer(player);
			so->AddAllRecipientsBut(player);
			slicengine_Get()->Execute(std::move(so));
		}
	}

	if(player_Get(player)->GetGaiaController()->HasMinCoresBuilt()) {
		seg = slicengine_Get()->GetSegment("GCMinCoresReachedUs");
		if (seg && !seg->TestLastShown(player, 10000, turn_Get()->GetRound()))
		{
			so = std::make_unique<SlicObject>("GCMinCoresReachedUs");
			so->AddRecipient(player);
			so->AddPlayer(player);
			slicengine_Get()->Execute(std::move(so));

			so = std::make_unique<SlicObject>("GCMinCoresReachedThem");
			so->AddPlayer(player);
			so->AddAllRecipientsBut(player);
			slicengine_Get()->Execute(std::move(so));
		}
	}

	return GEV_HD_Continue;
}

STDEHANDLER(CreateWonderEvent)
{
	Unit c;
	sint32 wonder;
	if(!args->GetCity(0, c)) return GEV_HD_Continue;
	if(!args->GetInt(0, wonder)) return GEV_HD_Continue;

	c.CD()->AddWonder(wonder);
	wonderutil_AddBuilt(wonder);
	player_Get(c->GetOwner())->AddWonder(wonder, c);

	if (c->GetOwner() == player_view::VisiblePlayer() &&
		!Player::IsThisPlayerARobot(c->GetOwner())) {

		if ( profiledb_Get()->IsWonderMovies() ) {
			render_observer::AddPlayWonderMovie(c.CD()->GetBuildQueue()->GetHead()->m_type);
		}

	}
	if(network_Get().IsHost()) {
		network_Get().Block(c.GetOwner());
		network_Get().Enqueue(std::make_unique<NetInfo>(NET_INFO_CODE_WONDER_BUILT,
									  c.CD()->GetBuildQueue()->GetHead()->m_type, (uint32)c.m_id).release());
		network_Get().Unblock(c.GetOwner());
	}

	Unit u;
	c.CD()->GetBuildQueue()->FinishBuildFront(u);
	std::unique_ptr<SlicObject> so;

	if(wonder == wonderutil_GetFobCityIndex()) {
		so = std::make_unique<SlicObject>("911ForbiddenCityPeace");
		so->AddRecipient(c.GetOwner());
		so->AddCity(c);
		slicengine_Get()->Execute(std::move(so));
	}

	if(wonder == wonderutil_GetGaiaIndex()) {
		// Notify the other players that they have to hurry to win.
		// Starting at 1: the Barbarians do not have to be notified.
		for (sint32 i = 1; i < k_MAX_PLAYERS; ++i)
		{
			if (player_Get(i) && !player_Get(i)->IsDead() && (i != c.GetOwner()))
			{
				auto so = std::make_unique<SlicObject>("GCMustDiscoverGaiaController");
				so->AddRecipient(i);
				so->AddPlayer(i);
				so->AddPlayer(c.GetOwner());
				slicengine_Get()->Execute(std::move(so));	// will delete so after handling
			}
		}
	}

	return GEV_HD_Continue;
}

STDEHANDLER(RushBuyEvent)
{
	Unit c;
	if(!args->GetCity(0, c)) return GEV_HD_Continue;

	c.CD()->BuyFront();
	return GEV_HD_Continue;
}

STDEHANDLER(DisbandCityEvent)
{
	Unit c;
	if(!args->GetCity(0, c)) return GEV_HD_Continue;

	c.DisbandCity();
	return GEV_HD_Continue;
}

STDEHANDLER(SellBuildingEvent)
{
	Unit c;
	sint32 b;

	if(!args->GetCity(0, c)) return GEV_HD_Continue;
	if(!args->GetInt(0, b)) return GEV_HD_Continue;

	c.CD()->SellBuilding(b, TRUE);
	return GEV_HD_Continue;
}

STDEHANDLER(KillTileEvent)
{
	MapPoint pos;
	if(!args->GetPos(0, pos)) return GEV_HD_Continue;

	Cell *cell = world_Get()->GetCell(pos);
	if(cell->GetCanDie()) {
		cell->Kill();

		world_Get()->CutImprovements(pos);

		cell->CalcTerrainMoveCost();
		tiledmap_observer::PostProcessTile(pos, world_Get()->GetTileInfo(pos));
		tiledmap_observer::TileChanged(pos);
		MapPoint npos;
		for(WORLD_DIRECTION d = NORTH; d < NOWHERE;
			d = (WORLD_DIRECTION)((sint32)d + 1)) {
			if(pos.GetNeighborPosition(d, npos)) {
				tiledmap_observer::PostProcessTile(
					npos,
					world_Get()->GetTileInfo(npos));
				tiledmap_observer::TileChanged(npos);
			}
		}
		tiledmap_observer::RedrawTile(pos);
	}
	return GEV_HD_Continue;
}

void cityevent_Initialize()
{
	gevmanager_Get()->AddCallback(GEV_CaptureCity, GEV_PRI_Primary, &s_CaptureCityEvent);

	gevmanager_Get()->AddCallback(GEV_CityTurnPreProduction, GEV_PRI_Primary, &s_CityTurnPreProductionEvent);
	gevmanager_Get()->AddCallback(GEV_CityBeginTurn, GEV_PRI_Primary, &s_CityBeginTurnEvent);
	gevmanager_Get()->AddCallback(GEV_CityBeginTurnVision, GEV_PRI_Primary, &s_CityBeginTurnVisionEvent);
	gevmanager_Get()->AddCallback(GEV_BuildFront, GEV_PRI_Primary, &s_CityBuildFrontEvent);
	gevmanager_Get()->AddCallback(GEV_CreateUnit, GEV_PRI_Post, &s_CityCreateUnitEvent);

	gevmanager_Get()->AddCallback(GEV_BuildUnit, GEV_PRI_Primary, &s_CityBuildUnitEvent);
	gevmanager_Get()->AddCallback(GEV_BuildBuilding, GEV_PRI_Primary, &s_CityBuildBuildingEvent);
	gevmanager_Get()->AddCallback(GEV_BuildWonder, GEV_PRI_Primary, &s_CityBuildWonderEvent);
	gevmanager_Get()->AddCallback(GEV_ZeroProduction, GEV_PRI_Primary, &s_ZeroProductionEvent);
	gevmanager_Get()->AddCallback(GEV_RollOverProduction, GEV_PRI_Primary, &s_RollOverProductionEvent);

	gevmanager_Get()->AddCallback(GEV_MakePop, GEV_PRI_Primary, &s_MakePopEvent);
	gevmanager_Get()->AddCallback(GEV_KillPop, GEV_PRI_Primary, &s_KillPopEvent);

	gevmanager_Get()->AddCallback(GEV_FinishUprising, GEV_PRI_Primary, &s_FinishUprisingEvent);
	gevmanager_Get()->AddCallback(GEV_CleanupUprising, GEV_PRI_Primary, &s_CleanupUprisingEvent);

	gevmanager_Get()->AddCallback(GEV_NukeCity, GEV_PRI_Primary, &s_NukeCityEvent);
	gevmanager_Get()->AddCallback(GEV_MakeFranchise, GEV_PRI_Primary, &s_MakeFranchiseEvent);
	gevmanager_Get()->AddCallback(GEV_SlaveRaidCity, GEV_PRI_Primary, &s_SlaveRaidCityEvent);
	gevmanager_Get()->AddCallback(GEV_BioInfectCity, GEV_PRI_Primary, &s_BioInfectCityEvent);
	gevmanager_Get()->AddCallback(GEV_PlagueCity, GEV_PRI_Primary, &s_PlagueCityEvent);
	gevmanager_Get()->AddCallback(GEV_NanoInfectCity, GEV_PRI_Primary, &s_NanoInfectCityEvent);
	gevmanager_Get()->AddCallback(GEV_ConvertCity, GEV_PRI_Primary, &s_ConvertCityEvent);
	gevmanager_Get()->AddCallback(GEV_UnconvertCity, GEV_PRI_Primary, &s_UnconvertCityEvent);
	gevmanager_Get()->AddCallback(GEV_AddHappyTimer, GEV_PRI_Primary, &s_AddHappyTimerEvent);
	gevmanager_Get()->AddCallback(GEV_CreatePark, GEV_PRI_Primary, &s_CreateParkEvent);
	gevmanager_Get()->AddCallback(GEV_InjoinCity, GEV_PRI_Primary, &s_InjoinCityEvent);

	gevmanager_Get()->AddCallback(GEV_CreateBuilding, GEV_PRI_Primary, &s_CreateBuildingEvent);
	gevmanager_Get()->AddCallback(GEV_CreateWonder, GEV_PRI_Primary, &s_CreateWonderEvent);

	gevmanager_Get()->AddCallback(GEV_BuyFront, GEV_PRI_Primary, &s_RushBuyEvent);
	gevmanager_Get()->AddCallback(GEV_DisbandCity, GEV_PRI_Primary, &s_DisbandCityEvent);
	gevmanager_Get()->AddCallback(GEV_SellBuilding, GEV_PRI_Primary, &s_SellBuildingEvent);

	gevmanager_Get()->AddCallback(GEV_KillTile, GEV_PRI_Primary, &s_KillTileEvent);

}

void cityevent_Cleanup()
{
}
