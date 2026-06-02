//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Barbarian placement and generation
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
// - Alter the algorithm used to place barbarians in single player games,
//   making it the same as that used in multiplayer games - JJB 2004/12/13
// - Add a NoBarbarian flag which makes the unit not appear as barbarian
// - Replaced old risk database by new one. (Aug 29th 2005 Martin G�hmann)
// - Added Pirate generation. (April 14th 2006 E)
// - Game does not try to generate barbarian units of invalid type anymore
//   if there is no valid unit type available. (April 29th 2006 Martin G�hmann)
// - Added but not implemented AddInsurgent Code it maynot be necessary
// - Added but outcommented Barbarian Special Forces difficulty code
// - added outcomment for disaster/random event code
// - Standardizes in Babarian period computation. (25-Jan-2008 Martin G�hmann)
// - Standardized visibility check. (22-Feb-2008 Martin G�hmann)
// - Barbarians do not show up inside the borders of a protected civ. (22-Feb-2008 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/gameobj/Barbarians.h"
#include "gs/world/MapPoint.h"
#include "gs/utility/RandGen.h"
#include "gs/gameobj/Player.h"
#include "gs/utility/Globals.h"
#include "gs/gameobj/Unit.h"
#include "gs/outcom/AICause.h"
#include "gs/gameobj/Advances.h"
#include "gs/gameobj/XY_Coordinates.h"
#include "gs/world/World.h"
#include "RiskRecord.h"
#include "gs/database/profileDB.h"
#include "net/general/network.h"
#include "gs/utility/UnitDynArr.h"
#include "gs/world/Cell.h"
#include "gs/gameobj/GameSettings.h"
#include "UnitRecord.h"
#include "gs/gameobj/Exclusions.h"
#include "gs/gameobj/wonderutil.h"



struct BestUnit
{
	sint32 index;
	sint32 attack;
};

bool SomeoneCanHave(const UnitRecord *rec)
{
	for(sint32 p = 0; p < k_MAX_PLAYERS; p++)
	{
		if(player_Get(p) && player_Get(p)->m_advances->HasAdvance(rec->GetEnableAdvanceIndex()))
			return true;
	}
	return false;
}

/*
//Disaster code?  use feat but create a feat flag that IsDisaster
//Golden Age and good events too?
//How do I get it so feat only affects one player?

		sint32 feat;
		if(!g_network.IsNetworkLaunch()
		&& player_Get(selitem_Get()->GetCurPlayer())
		&& advRec->GetTriggerFeatIndex(feat)
		){
			if(!g_network.IsActive() || g_network.ReadyToStart()) {
				feattracker_Get()->AddFeat(feat, m_owner);
			}
		}
	}
*/

sint32 Barbarians::ChooseUnitType()
{
	const RiskRecord *risk = g_theRiskDB->Get(gamesettings_Get()->GetRisk());
	sint32 num_best_units = risk->GetBarbarianUnitRankMin();
	BestUnit *best = new BestUnit[num_best_units];
	sint32 i, j, k;
	sint32 count = 0;

	for(i = 0; i < num_best_units; i++) {
		best[i].index = -1;
		best[i].attack = -1;
	}

	for(i = 0; i < g_theUnitDB->NumRecords(); i++) {
		const UnitRecord *rec = g_theUnitDB->Get(i);
		if(rec->GetCantBuild())
			continue;

		if(!rec->GetMovementTypeLand())
			continue;

		if(rec->GetAttack() < 1)
			continue;

		if(exclusions_Get() && exclusions_Get()->IsUnitExcluded(i))
			continue;
		if(rec->GetNoBarbarian())
			continue;

		if(SomeoneCanHave(rec)) {
			for(j = 0; j < num_best_units; j++) {
				if(rec->GetAttack() > best[j].attack) {
					for(k = num_best_units - 1; k >= j+1; k--) {
						best[k] = best[k-1];
					}
					best[j].index = i;
					best[j].attack = (sint32)rec->GetAttack();
					count++;
					break;
				}
			}
		}
	}

	sint32 ret = -1;
	if(count > 0){
		if(count > num_best_units)
			count = num_best_units;

		sint32 rankMax;
		if(risk->GetBarbarianUnitRankMax() >= count) {
			rankMax = count - 1;
		} else {
			rankMax = risk->GetBarbarianUnitRankMax();
		}
		sint32 range = count - rankMax;
		if (range <= 0) range = 1;
		sint32 whichbest = civrand().Next(range) + rankMax;
		if(whichbest >= count)
			whichbest = count - 1;

		ret = best[whichbest].index;
	}
	delete [] best;
	return ret;
}

bool Barbarians::AddBarbarians(const MapPoint &point, PLAYER_INDEX meat,
                               bool fromGoodyHut, sint32 currentRound)
{
	//add spontaneous barb bool?
	//add bools for barbarian spawn?
	//and sint for # of barbs if all bools are false?
	if(g_network.IsClient() && !g_network.IsLocalPlayer(meat))
		return false;

	if(!InBarbarianPeriod(currentRound))
		return false;

	sint32 unitIndex = Barbarians::ChooseUnitType();

	if(unitIndex < 0) return false;

	sint32 d;
	MapPoint neighbor;
	bool tried[NOWHERE];
	sint32 triedCount = 0;

	sint32 maxBarbarians;
	if(fromGoodyHut)
	{
		sint32 maxHut = g_theRiskDB->Get(gamesettings_Get()->GetRisk())->GetHutMaxBarbarians();
		if (maxHut < 1) maxHut = 1;
		maxBarbarians = civrand().Next(maxHut - 1) + 1;
	}
	else
	{
		sint32 maxSpont = g_theRiskDB->Get(gamesettings_Get()->GetRisk())->GetMaxSpontaniousBarbarians();
		if (maxSpont < 1) maxSpont = 1;
		maxBarbarians = civrand().Next(maxSpont - 1) + 1;
	}

	sint32 count = 0;
	for(d = sint32(NORTH); d < sint32(NOWHERE); d++)
	{
		tried[d] = false;
	}

	for(count = 0; count < maxBarbarians && triedCount < 8;)
	{
		sint32 use = civrand().Next(NOWHERE);
		while(tried[use])
		{
			use++;
			if(use >= NOWHERE)
				use = NORTH;
		}

		tried[use] = true;
		triedCount++;
		if(point.GetNeighborPosition((WORLD_DIRECTION)use, neighbor))
		{
			if
			  (
			        world_Get()->IsLand(neighbor)
			    && !world_Get()->IsCity(neighbor)
			  )
			{
				count++;
				DPRINTF(k_DBG_GAMESTATE, ("Barbarians: Add unit to map, from goody hut: %d\n", fromGoodyHut));
				Unit u = player_Get(PLAYER_INDEX_VANDALS)->CreateUnit(unitIndex,
														   neighbor,
														   Unit(),
														   false,
														   CAUSE_NEW_ARMY_INITIAL);
				if(u.m_id == 0)
					count--;
			}
		}
	}

	return count != 0;
}

// The new barb placement method (adapted from the above,
// using only the algorithm previously used in MP):

#define k_MAX_BARBARIAN_TRIES 400

sint32 Barbarians::ChooseSeaUnitType()
{
	const RiskRecord *risk = g_theRiskDB->Get(gamesettings_Get()->GetRisk());
	sint32 num_best_units = risk->GetBarbarianUnitRankMin();
	BestUnit *best = new BestUnit[num_best_units];
	sint32 i, j, k;
	sint32 count = 0;

	for(i = 0; i < num_best_units; i++)
	{
		best[i].index = -1;
		best[i].attack = -1;
	}

	for(i = 0; i < g_theUnitDB->NumRecords(); i++)
	{
		const UnitRecord *rec = g_theUnitDB->Get(i);
		if(rec->GetCantBuild())
			continue;

		if(!rec->GetMovementTypeSea())
			continue;

		if(rec->GetAttack() < 1)
			continue;

		if(exclusions_Get() && exclusions_Get()->IsUnitExcluded(i))
			continue;
		if(rec->GetNoBarbarian())
			continue;

		if(SomeoneCanHave(rec))
		{
			for(j = 0; j < num_best_units; j++)
			{
				if(rec->GetAttack() > best[j].attack)
				{
					for(k = num_best_units - 1; k >= j+1; k--)
					{
						best[k] = best[k-1];
					}
					best[j].index = i;
					best[j].attack = (sint32)rec->GetAttack();
					count++;
					break;
				}
			}
		}
	}

	sint32 ret = -1;
	if(count > 0)
	{
		if(count > num_best_units)
			count = num_best_units;

		sint32 rankMax;
		if(risk->GetBarbarianUnitRankMax() >= count)
		{
			rankMax = count - 1;
		}
		else
		{
			rankMax = risk->GetBarbarianUnitRankMax();
		}

		sint32 whichbest = civrand().Next(count - rankMax) + rankMax;
		if(whichbest >= count)
			whichbest = count - 1;

		ret = best[whichbest].index;
	}

	delete [] best;
	return ret;
}

//EMOD to add sea barbarians
bool Barbarians::AddPirates(const MapPoint &point, PLAYER_INDEX meat,
                            bool fromGoodyHut, sint32 currentRound)
{
	if(g_network.IsClient() && !g_network.IsLocalPlayer(meat))
		return false;

	if(!InBarbarianPeriod(currentRound))
		return false;

	sint32 unitIndex = Barbarians::ChooseSeaUnitType();

	if(unitIndex < 0) return false;

	sint32 d;
	MapPoint neighbor;
	bool tried[NOWHERE];
	sint32 triedCount = 0;

	sint32 maxBarbarians;
	if(fromGoodyHut)
	{
		maxBarbarians = civrand().Next(g_theRiskDB->Get(gamesettings_Get()->GetRisk())->GetHutMaxBarbarians() - 1) + 1;
	}
	else
	{
		maxBarbarians = civrand().Next(g_theRiskDB->Get(gamesettings_Get()->GetRisk())->GetMaxSpontaniousBarbarians() - 1) + 1;
	}

	sint32 count = 0;
	for(d = sint32(NORTH); d < sint32(NOWHERE); d++)
	{
		tried[d] = false;
	}

	for(count = 0; count < maxBarbarians && triedCount < 8;)
	{
		sint32 use = civrand().Next(NOWHERE);
		while(tried[use])
		{
			use++;
			if(use >= NOWHERE)
				use = NORTH;
		}

		tried[use] = true;
		triedCount++;
		if(point.GetNeighborPosition((WORLD_DIRECTION)use, neighbor))
		{
			if
			  (
			        world_Get()->IsWater(neighbor)
			    && !world_Get()->IsCity(neighbor)
			  )
			{
				count++;
				DPRINTF(k_DBG_GAMESTATE, ("Barbarians: Add unit to map, from goody hut: %d\n", fromGoodyHut));
				Unit u = player_Get(PLAYER_INDEX_VANDALS)->CreateUnit(unitIndex,
														   neighbor,
														   Unit(),
														   false,
														   CAUSE_NEW_ARMY_INITIAL);
				if(u.m_id == 0)
					count--;
			}
		}
	}

	return count != 0;
}

//end EMOD

/*sint32 Barbarians::ChooseInsurgentUnitType()
{
	const RiskRecord *risk = g_theRiskDB->Get(gamesettings_Get()->GetRisk());
	sint32 num_best_units = risk->GetBarbarianUnitRankMin();
	BestUnit *best = new BestUnit[num_best_units];
	sint32 i, j, k;
	sint32 count = 0;

	for(i = 0; i < num_best_units; i++) {
		best[i].index = -1;
		best[i].attack = -1;
	}

	for(i = 0; i < g_theUnitDB->NumRecords(); i++) {
		const UnitRecord *rec = g_theUnitDB->Get(i);
		if(rec->GetCantBuild())  //removed because guerrillas may be obsolete for major powers
			continue;

		if(!rec->GetIsGuerrilla())
			continue;

		if(!rec->GetMovementTypeLand())
			continue;

		if(rec->GetAttack() < 1)
			continue;

		if(exclusions_Get() && exclusions_Get()->IsUnitExcluded(i))
			continue;
		if(rec->GetNoBarbarian())
			continue;




		if(SomeoneCanHave(rec)) {  //Someone can have is an enable advance check
			for(j = 0; j < num_best_units; j++) {
				if(rec->GetAttack() > best[j].attack) {
					for(k = num_best_units - 1; k >= j+1; k--) {
						best[k] = best[k-1];
					}
					best[j].index = i;
					best[j].attack = (sint32)rec->GetAttack();
					count++;
					break;
				}
			}
		}
	}

	sint32 ret = -1;
	if(count > 0){
		if(count > num_best_units)
			count = num_best_units;

		sint32 rankMax;
		if(risk->GetBarbarianUnitRankMax() >= count) {
			rankMax = count - 1;
		} else {
			rankMax = risk->GetBarbarianUnitRankMax();
		}
		sint32 whichbest = civrand().Next(count - rankMax) + rankMax;
		if(whichbest >= count)
			whichbest = count - 1;

		ret = best[whichbest].index;
	}
	delete [] best;
	return ret;
}

//EMOD to add special guerrilla barbarians; because guerrillas may go obsolete for normal players.  Eventually add this to insurgent code in citydata. but is it needed?
bool Barbarians::AddInsurgents(const MapPoint &point, PLAYER_INDEX meat,
							   BOOL fromGoodyHut, sint32 currentRound)
{
	if(g_network.IsClient() && !g_network.IsLocalPlayer(meat))
		return FALSE;

	if(currentRound < g_theRiskDB->Get(gamesettings_Get()->GetRisk())->GetBarbarianFirstTurn() ||
	   currentRound >= g_theRiskDB->Get(gamesettings_Get()->GetRisk())->GetBarbarianLastTurn()) {
		return FALSE;
	}

	sint32 unitIndex = Barbarians::ChooseUnitType();

	if(unitIndex < 0) return FALSE;

	sint32 d;
	MapPoint neighbor;
	BOOL tried[NOWHERE];
	sint32 triedCount = 0;

	sint32 maxBarbarians;
	if(fromGoodyHut) {
		maxBarbarians = civrand().Next(g_theRiskDB->Get(gamesettings_Get()->GetRisk())->GetHutMaxBarbarians() - 1) + 1;
	} else {
		maxBarbarians = civrand().Next(g_theRiskDB->Get(gamesettings_Get()->GetRisk())->GetMaxSpontaniousBarbarians() - 1) + 1;
	}

	sint32 count = 0;
	for(d = sint32(NORTH); d < sint32(NOWHERE); d++) {
		tried[d] = FALSE;
	}

	for(count = 0; count < maxBarbarians && triedCount < 8;) {
		sint32 use = civrand().Next(NOWHERE);
		while(tried[use]) {
			use++;
			if(use >= NOWHERE)
				use = NORTH;
		}

		tried[use] = TRUE;
		triedCount++;
		if(point.GetNeighborPosition((WORLD_DIRECTION)use, neighbor)) {
			if(world_Get()->IsLand(neighbor) &&
			   !world_Get()->IsCity(neighbor)) {
				count++;
				Unit u = player_Get(PLAYER_INDEX_VANDALS)->CreateUnit(unitIndex,
														   neighbor,
														   Unit(),
														   FALSE,
														   CAUSE_NEW_ARMY_INITIAL);
				if(u.m_id == 0)
					count--;
			}
		}
	}
	return count != 0;
}

bool Barbarians::AddFreeTraders(const MapPoint &point, PLAYER_INDEX meat,
							   BOOL fromGoodyHut)   //code for a random free trade unit that acts as a corporation?


//end EMOD*/

void Barbarians::BeginYear(sint32 currentRound)
{
	if(!InBarbarianPeriod(currentRound))
		return;

	const RiskRecord *risk = g_theRiskDB->Get(gamesettings_Get()->GetRisk());

	if(civrand().Next(10000) < risk->GetBarbarianChance() * 10000)
	{
/// @todo Refactor this (extract functions/methods) to make the code cleaner.
///       The combination of continue + multiple break levels makes it very
///       difficult to read. Added the initialisation of p "just in case".
///       It could be OK without the initialisation, but I am not in the mood
///       to work it out at the moment.
///       What is p supposed to represent anyway? p will become k_MAX_PLAYERS
///       (not a valid player) when noone can see some random tile???

		MapPoint point;
		sint32 tries;
		sint32 p    = PLAYER_UNASSIGNED;

		// this is for standard attack units
		for(tries = 0; tries < k_MAX_BARBARIAN_TRIES; tries++) {
			point.x = sint16(civrand().Next(world_Get()->GetXWidth()));
			point.y = sint16(civrand().Next(world_Get()->GetYHeight()));

			sint32 owner = world_Get()->GetCell(point)->GetOwner();
			if(owner > 0
			&& player_Get(owner)
			&& wonderutil_GetProtectFromBarbarians(player_Get(owner)->m_builtWonders)
			){
				continue;
			}

			if(!world_Get()->IsLand(point))
			{
				continue;
			}

			p = IsVisibleToAnyone(point);

			if(p > 0)
				break;
		}
		if(tries < k_MAX_BARBARIAN_TRIES)
		{
			AddBarbarians(point, p, false, currentRound);  //AddBarbarians(point, -1, FALSE);
		}
//EMOD for Pirates

/// @todo Also have to think about maybe reinitialising p here, or you might end
///       up with the value of the previous loop.
		p    = PLAYER_UNASSIGNED;

		sint32 ptries;
		for(ptries = 0; ptries < k_MAX_BARBARIAN_TRIES; ptries++)
		{
			point.x = sint16(civrand().Next(world_Get()->GetXWidth()));
			point.y = sint16(civrand().Next(world_Get()->GetYHeight()));

			sint32 owner = world_Get()->GetCell(point)->GetOwner();
			if(owner > 0
			&& player_Get(owner)
			&& wonderutil_GetProtectFromBarbarians(player_Get(owner)->m_builtWonders)
			){
				continue;
			}

			if (!world_Get()->IsWater(point))
			{
				continue;
			}

			p = IsVisibleToAnyone(point);

			if(p > 0)
				break;
		}

		if(ptries < k_MAX_BARBARIAN_TRIES)
		{
			// is p selecting the the nearest player?
			AddPirates(point, p, false, currentRound); //AddPirates(point, -1, FALSE);
		}
/*  EMOD Barbarian Special Forces code
		sint32 sftries;
	if(g_theDifficultyDB->Get(gamesettings_Get()->GetDifficulty())->GetBarbarianSpecialForces())
	  {
		for(sftries = 0; sftries < k_MAX_BARBARIAN_TRIES; ptries++) {
			point.x = sint16(civrand().Next(world_Get()->GetXWidth()));
			point.y = sint16(civrand().Next(world_Get()->GetYHeight()));

			if (!world_Get()->IsLand(point)) {
				continue;
			}

			for(p = 1; p < k_MAX_PLAYERS; p++) {
				if(player_Get(p) && player_Get(p)->IsVisible(point))
					break;
			}
			if(p >= k_MAX_PLAYERS) {
				break;
			}
		}
		if(ptries < k_MAX_BARBARIAN_TRIES) {
			AddSFBarbarians(point, -1, FALSE);
		}
//end SF-Barbarian
 Add insurgent code here or keep in CityData?

*/
//end EMOD

	}
}

sint32 Barbarians::IsVisibleToAnyone(MapPoint point)
{
	for(sint32 p = 1; p < k_MAX_PLAYERS; p++)
	{
		if(player_Get(p) && player_Get(p)->IsVisible(point))
			return p;
	}

	return PLAYER_UNASSIGNED;
}

bool Barbarians::InBarbarianPeriod(sint32 currentRound)
{
	const RiskRecord *risk = g_theRiskDB->Get(gamesettings_Get()->GetRisk());

	return currentRound >= risk->GetBarbarianFirstTurn() // First turn is included.
	    && currentRound <= risk->GetBarbarianLastTurn(); // Last turn is included, too.
}
