//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Director events
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
// - Special attack centers only if the auto center option on units and
//   cities is set. (23-Feb-2008 Martin G�hmann)
// - Stopped centering map on stealth units that you can't see. (12-Apr-2009 Maq)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "gfx/spritesys/UnitActor.h"
#include "gfx/spritesys/director.h"
#include "gfx/spritesys/directorevent.h"
#include "gfx/tilesys/tiledmap.h"
#include "gs/database/profileDB.h"
#include "gs/events/GameEventUser.h"
#include "gs/events/GameEventManager.h"
#include "gs/gameobj/Army.h"
#include "gs/gameobj/ArmyData.h"
#include "gs/gameobj/Events.h"
#include "gs/gameobj/Player.h"
#include "gs/gameobj/UnitData.h"
#include "gs/gameobj/UnitPool.h"
#include "gs/gameobj/unitutil.h"
#include "SoundRecord.h"
#include "SpecialAttackInfoRecord.h"
#include "SpecialEffectRecord.h"
#include "net/general/network.h"
#include "sound/soundmanager.h"
#include "ui/aui_ctp2/SelItem.h"

STDEHANDLER(DirectorMoveUnitsEvent)
{
	Army a;
	MapPoint from;
	MapPoint to;

	if(!args->GetArmy(0, a)) return GEV_HD_Continue;
	if(!args->GetPos(0, from)) return GEV_HD_Continue;
	if(!args->GetPos(1, to)) return GEV_HD_Continue;

	if(a.Num() <= 0) return GEV_HD_Continue;

	if(a->IsStealth()
		&& !a->IsVisible(selitem_Get()->GetPlayerOnScreen())) return GEV_HD_Continue;

//	BOOL theTileIsVisible = tiledmap_Get()->TileIsCompletelyVisible(to.x, to.y);

	if (selitem_Get()->GetPlayerOnScreen() != -1 &&
		selitem_Get()->GetPlayerOnScreen() != selitem_Get()->GetVisiblePlayer() &&
		!network_Get().IsActive())

			return GEV_HD_Continue;

	Unit top_src = a->GetTopVisibleUnit(selitem_Get()->GetVisiblePlayer());
	if (top_src.m_id == 0)
		top_src = a[0];

	sint32 numRevealed = 0;

	const sint32 numRest = a->Num() - 1;

  Director::UnitActorVec restOfStack;

	if (numRest > 0) {
		a->GetActors(top_src, restOfStack);
	}

  Director::UnitActorVec revealedActors;

	if (numRevealed > 0) {

		// Something missing here.

	}

	MapPoint newPos = to;

	if(selitem_Get()->IsAutoCenterOn()
	&& !director_Get()->TileWillBeCompletelyVisible(newPos.x, newPos.y)
	&& (top_src.GetVisibility() & (1 << selitem_Get()->GetVisiblePlayer()))
	&& (   profiledb_Get()->IsEnemyMoves()
	    || top_src.GetOwner() == selitem_Get()->GetVisiblePlayer()
	   )
	){
		director_Get()->AddCenterMap(newPos);
	}

	if (!to.IsNextTo(from)) {
		director_Get()->AddTeleport(top_src, from, newPos, revealedActors, restOfStack);
	} else {
		director_Get()->AddMove(
      top_src,
      from,
      newPos,
      revealedActors,
		  restOfStack,
      false,
      top_src.GetMoveSoundID());

	}

	if (top_src.GetData()->HasLeftMap())
		director_Get()->AddHide(top_src);

	return GEV_HD_Continue;
}

STDEHANDLER(DirectorActionSuccessful)
{
	Unit unit;
	Unit c;
	if(!args->GetUnit(0, unit)) return GEV_HD_Continue;

	MapPoint pos = unit.RetPos();
	static MapPoint attackPos;

	if(!args->GetCity(0,c)) {
		if(!args->GetUnit(1, c)) {
			if(!args->GetPos(0, attackPos)) {
				attackPos = pos;
			}
		} else {
			attackPos = c.RetPos();
		}
	} else {
		attackPos = c.RetPos();
	}

	SPECATTACK attack;
	switch(gameEventType)
	{
		case GEV_InciteRevolutionUnit: attack = SPECATTACK_INCITEREVOLUTION; break;
		case GEV_AssassinateRulerUnit: attack = SPECATTACK_BOMBCABINET; break;
		case GEV_MakeFranchise: attack = SPECATTACK_CREATEFRANCHISE; break;
		case GEV_PlantNukeUnit: attack = SPECATTACK_PLANTNUKE; break;
		case GEV_SlaveRaidCity: attack = SPECATTACK_SLAVERAID; break;
		case GEV_NukeCityUnit: attack = SPECATTACK_NUKE; break;
		case GEV_NukeLocationUnit: attack = SPECATTACK_NUKE; break;
		case GEV_EnslaveSettler: attack = SPECATTACK_ENSLAVESETTLER; break;
		case GEV_InciteUprisingUnit: attack = SPECATTACK_SLAVEUPRISING; break;
		case GEV_EstablishEmbassyUnit: attack = SPECATTACK_ESTABLISHEMBASSY; break;
		case GEV_BioInfectCityUnit: attack = SPECATTACK_BIOTERROR; break;
		case GEV_NanoInfectCityUnit: attack = SPECATTACK_NANOTERROR; break;
		case GEV_ConvertCityUnit: attack = SPECATTACK_CONVERTCITY; break;
		case GEV_ReformCityUnit: attack = SPECATTACK_REFORMCITY; break;
		case GEV_IndulgenceSaleMade: attack = SPECATTACK_SELLINDULGENCE; break;
		case GEV_CreateParkUnit: attack = SPECATTACK_CREATEPARK; break;
		case GEV_InjoinUnit: attack = SPECATTACK_INJOIN; break;


		case GEV_Lawsuit: attack = SPECATTACK_NONE; break;
		case GEV_RemoveFranchise: attack = SPECATTACK_NONE; break;
		case GEV_ExpelUnits: attack = SPECATTACK_NONE; break;
		case GEV_UndergroundRailwayUnit: attack = SPECATTACK_NONE; break;
		case GEV_PillageUnit: attack = SPECATTACK_NONE; break;

		default:
			attack = SPECATTACK_NONE;
	}

	sint32 soundID;
	sint32 spriteID;
	if(attack != SPECATTACK_NONE)
	{
		const SpecialAttackInfoRecord *rec;
		rec = unitutil_GetSpecialAttack(attack);
		soundID = rec->GetSoundIDIndex();
		spriteID = rec->GetSpriteID()->GetValue();
		if (spriteID != -1 && soundID != -1)
		{
			if(selitem_Get()->IsAutoCenterOn())
			{
				if((((unit.GetOwner() == selitem_Get()->GetVisiblePlayer()) ||
					 (unit.GetVisibility() & (1 << selitem_Get()->GetVisiblePlayer()))) ||
					unitpool_Get()->IsValid(c) &&
					((c.GetOwner() == selitem_Get()->GetVisiblePlayer()) ||
					 (c.GetVisibility() & (1 << selitem_Get()->GetVisiblePlayer()))))) {

					director_Get()->AddCenterMap(attackPos);
				}
			}

			if(c.IsValid())
				director_Get()->AddSpecialAttack(unit, c, attack);
			else
				director_Get()->AddSpecialEffect(attackPos, spriteID, soundID);

		} else {
			if (soundID != -1) {
				sint32 visiblePlayer = selitem_Get()->GetVisiblePlayer();
				if ((visiblePlayer == unit.GetOwner()) ||
					(unit.GetVisibility() & (1 << visiblePlayer))) {

					soundmgr_Get()->AddSound(SOUNDTYPE_SFX, (uint32)0, 	soundID, attackPos.x, attackPos.y);
				}
			}
		}
	}
	else
	{
		sint32 visiblePlayer = selitem_Get()->GetVisiblePlayer();
		if(visiblePlayer < 0 || visiblePlayer >= k_MAX_PLAYERS)
			return GEV_HD_Continue;

		Player *vp = player_Get(visiblePlayer);
		if(vp && vp->m_vision && vp->m_vision->IsVisible(attackPos))
		{
			SpecialEffectRecord const *effect = g_theSpecialEffectDB->Get(g_theSpecialEffectDB->FindTypeIndex("SPECEFFECT_GENERAL_SUCCESS"));
			if(!effect)
				return GEV_HD_Continue;

			spriteID = effect->GetValue();
			soundID  = g_theSoundDB->FindTypeIndex("SOUND_ID_GENERALSUCCEED");
			if(selitem_Get()->IsAutoCenterOn())
			{
				director_Get()->AddCenterMap(attackPos);
			}

			director_Get()->AddSpecialEffect(attackPos, spriteID, soundID);
		}
	}
	return GEV_HD_Continue;
}

STDEHANDLER(DirectorReallyBeginScheduler)
{
	if (!director_Get()->m_holdSchedulerSequence.expired()) {
		director_Get()->ActionFinished(director_Get()->m_holdSchedulerSequence);
		director_Get()->m_holdSchedulerSequence.reset();
	}
	return GEV_HD_Continue;
}

void directorevent_Initialize()
{
	gevmanager_Get()->AddCallback(GEV_MoveUnits, GEV_PRI_Post, &s_DirectorMoveUnitsEvent);

	gevmanager_Get()->AddCallback(GEV_InciteRevolutionUnit, GEV_PRI_Post, &s_DirectorActionSuccessful);
	gevmanager_Get()->AddCallback(GEV_AssassinateRulerUnit, GEV_PRI_Post, &s_DirectorActionSuccessful);
	gevmanager_Get()->AddCallback(GEV_MakeFranchise, GEV_PRI_Post, &s_DirectorActionSuccessful);
	gevmanager_Get()->AddCallback(GEV_PlantNukeUnit, GEV_PRI_Post, &s_DirectorActionSuccessful);
	gevmanager_Get()->AddCallback(GEV_SlaveRaidCity, GEV_PRI_Post, &s_DirectorActionSuccessful);
	gevmanager_Get()->AddCallback(GEV_NukeCity, GEV_PRI_Post, &s_DirectorActionSuccessful);
	gevmanager_Get()->AddCallback(GEV_EnslaveSettler, GEV_PRI_Post, &s_DirectorActionSuccessful);
	gevmanager_Get()->AddCallback(GEV_InciteUprisingUnit, GEV_PRI_Post, &s_DirectorActionSuccessful);
	gevmanager_Get()->AddCallback(GEV_EstablishEmbassyUnit, GEV_PRI_Post, &s_DirectorActionSuccessful);
	gevmanager_Get()->AddCallback(GEV_BioInfectCityUnit, GEV_PRI_Post, &s_DirectorActionSuccessful);
	gevmanager_Get()->AddCallback(GEV_NanoInfectCityUnit, GEV_PRI_Post, &s_DirectorActionSuccessful);
	gevmanager_Get()->AddCallback(GEV_ConvertCityUnit, GEV_PRI_Post, &s_DirectorActionSuccessful);
	gevmanager_Get()->AddCallback(GEV_ReformCityUnit, GEV_PRI_Post, &s_DirectorActionSuccessful);
	gevmanager_Get()->AddCallback(GEV_IndulgenceSaleMade, GEV_PRI_Post, &s_DirectorActionSuccessful);
	gevmanager_Get()->AddCallback(GEV_CreateParkUnit, GEV_PRI_Post, &s_DirectorActionSuccessful);
	gevmanager_Get()->AddCallback(GEV_PillageUnit, GEV_PRI_Post, &s_DirectorActionSuccessful);
	gevmanager_Get()->AddCallback(GEV_InjoinUnit, GEV_PRI_Post, &s_DirectorActionSuccessful);
	gevmanager_Get()->AddCallback(GEV_Lawsuit, GEV_PRI_Post, &s_DirectorActionSuccessful);
	gevmanager_Get()->AddCallback(GEV_RemoveFranchise, GEV_PRI_Post, &s_DirectorActionSuccessful);
	gevmanager_Get()->AddCallback(GEV_ExpelUnits, GEV_PRI_Post, &s_DirectorActionSuccessful);
	gevmanager_Get()->AddCallback(GEV_UndergroundRailwayUnit, GEV_PRI_Post, &s_DirectorActionSuccessful);
	gevmanager_Get()->AddCallback(GEV_NukeCityUnit, GEV_PRI_Pre, &s_DirectorActionSuccessful);
	gevmanager_Get()->AddCallback(GEV_NukeLocationUnit, GEV_PRI_Pre, &s_DirectorActionSuccessful);

	gevmanager_Get()->AddCallback(GEV_BeginScheduler, GEV_PRI_Post, &s_DirectorReallyBeginScheduler);
}

void directorevent_Cleanup()
{
}
