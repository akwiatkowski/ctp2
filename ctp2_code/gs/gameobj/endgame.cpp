#include "ctp/c3.h"
#include "gs/gameobj/EndGame.h"
#include "gs/database/EndGameDB.h"

#include "gs/gameobj/Player.h"

#include "gs/slic/SlicObject.h"
#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicSegment.h"
#include "gs/utility/RandGen.h"
#include "gs/utility/UnitDynArr.h"
#include "gs/gameobj/UnitData.h"
#include "gs/gameobj/citydata.h"
#include "gs/core/player_view.h"

#include "gs/core/game_observer.h"
#include "gs/gameobj/Advances.h"
#include "AdvanceRecord.h"
#include "net/general/network.h"
#include "net/general/net_endgame.h"
#include "gs/database/StrDB.h"
#include "net/general/net_info.h"
#include "gs/gameobj/Score.h"
#include "gs/gameobj/GameSettings.h"
#include "gs/gameobj/advanceutil.h"
#include "gs/utility/gstypes.h"



EndGame::EndGame(PLAYER_INDEX owner)
{
	m_owner = owner;
	Init();
}

EndGame::~EndGame()
{
	
		delete [] m_numBuilt;
	
		delete [] m_savedNumBuilt;
}

void EndGame::Init()
{
	m_currentStage = -1;
	m_currentStageBegan = -1;

	m_numBuilt = new sint32[endgamedb_Get()->m_nRec];
	m_savedNumBuilt = new sint32[endgamedb_Get()->m_nRec];

    sint32 i;
    for (i=0; i < endgamedb_Get()->m_nRec; i++) {
        m_numBuilt[i] = 0;
		m_savedNumBuilt[i] = 0;
    }
}

void EndGame::AddObject(sint32 type)
{
	const EndGameRecord *egrec = endgamedb_Get()->Get(type);
	if(egrec->NotifyLabBuilt()) {
		SlicObject *so = new SlicObject("302EndGameOtherCivBuiltLab");
		so->AddAllRecipientsBut(m_owner);
		so->AddCivilisation(m_owner);
		slicengine_Get()->Execute(so);

		if(g_network.IsHost()) {
			g_network.Block(m_owner);
			g_network.Enqueue(new NetInfo(NET_INFO_CODE_OTHER_CIV_LAB_MSG,
										  m_owner));
			g_network.Unblock(m_owner);
		}

		so = new SlicObject("308EndGameFinishedXLab");
		so->AddRecipient(m_owner);
		slicengine_Get()->Execute(so);
	}

	m_numBuilt[type]++;

	if(g_network.IsHost()) {
		g_network.Block(m_owner);
		g_network.QueuePacketToAll(new NetEndGame(m_owner));
		g_network.Unblock(m_owner);
	}

	if(m_owner == player_view::VisiblePlayer()) {
		gameobservers_Get()->NotifyRequestEndGameShow(this);
	}
}

void EndGame::ClearAll()
{
	sint32 i;

	m_currentStage = -1;
	m_currentStageBegan = -1;

	for(i = 0; i < endgamedb_Get()->m_nRec;  i++) {
		m_numBuilt[i] = 0;
		m_savedNumBuilt[i] = 0;
	}
}

BOOL EndGame::BeginSequence(sint32 currentRound)
{
	sint32 i;
	for(i = 0; i < endgamedb_Get()->m_nRec; i++) {

		if(endgamedb_Get()->Get(i)->ExactlyOneRequired() &&
		   m_numBuilt[i] < 1)
			return FALSE;
	}

	SlicObject *so = new SlicObject("303EndGameOtherCivStartedSequence");
	so->AddAllRecipientsBut(m_owner);
	so->AddCivilisation(m_owner);
	slicengine_Get()->Execute(so);

	if(g_network.IsHost()) {
		g_network.Enqueue(new NetInfo(NET_INFO_CODE_OTHER_CIV_SEQUENCE_MSG,
									  m_owner));
	}

	m_currentStage = 0;
	m_currentStageBegan = currentRound;

	if(m_owner == player_view::VisiblePlayer()) {
		gameobservers_Get()->NotifyRequestEndGameShow(this);
	}

	return TRUE;
}

void EndGame::BeginTurn(sint32 currentRound)
{
	SlicObject *so;

	if(m_currentStage < 0 || m_currentStage >= endgamedb_Get()->GetNumStages())
		return;

	if(GetCataclysmChance() > 0) {
		double cataclysm_chance = GetCataclysmChance();
		cataclysm_chance /= GetTurnsForNextStage();
		cataclysm_chance *= 10;
		if(civrand().Next(1000) < sint32(cataclysm_chance)) {
			Cataclysm();
			return;
		}
	}

	sint32 turnsForNextStage = GetTurnsForNextStage();
    if ((turnsForNextStage >= 0) &&
        (currentRound > m_currentStageBegan + turnsForNextStage)) {
        if (MetRequirementsForNextStage()) {
            AdvanceStage(currentRound);
        } else {
			if (!HaveEnoughECDs() &&
				(slicengine_Get()->GetSegment("061NeedEcd")->TestLastShown(m_owner, 5, currentRound))) {
				so = new SlicObject("061NeedEcd");
				so->AddRecipient(m_owner);
				slicengine_Get()->Execute(so);
			}

			if(!HaveEnoughFields() &&
			   (slicengine_Get()->GetSegment("062NeedField")->TestLastShown(m_owner, 5, currentRound))) {
				so = new SlicObject("062NeedField");
				so->AddRecipient(m_owner);
				slicengine_Get()->Execute(so);
			}

			if(!HaveMaxSplicers() &&
			   (slicengine_Get()->GetSegment("063ShouldBuildSplicer")->TestLastShown(m_owner, 5, currentRound))) {
				so = new SlicObject("063ShouldBuildSplicer");
				so->AddRecipient(m_owner);
				slicengine_Get()->Execute(so);
			}
		}
	} else if(m_currentStage == 2 &&
			  turnsForNextStage >= 0 &&
			  (((m_currentStageBegan + turnsForNextStage) -
				currentRound) < 5)) {
		if(slicengine_Get()->GetSegment("053AlienAlmostDone")->TestLastShown(m_owner, 5, currentRound)) {
			so = new SlicObject("053AlienAlmostDone");
			so->AddRecipient(m_owner);
			slicengine_Get()->Execute(so);

			so = new SlicObject("054AlienAlmostDoneOthers");
			so->AddAllRecipientsBut(m_owner);
			so->AddCivilisation(m_owner);
			slicengine_Get()->Execute(so);

			if(g_network.IsHost()) {
				g_network.Enqueue(new NetInfo(NET_INFO_CODE_ALIEN_ALMOST_DONE_OTHERS_MSG,
											  m_owner));
			}
		}
	}

	if(g_network.IsHost()) {
		g_network.Block(m_owner);
		g_network.QueuePacketToAll(new NetEndGame(m_owner));
		g_network.Unblock(m_owner);
	}
}

void EndGame::AdvanceStage(sint32 currentRound)
{
	SlicObject *so;
/*
	sint32 cataclysm_chance = GetCataclysmChance();
	if(cataclysm_chance > 0) {
		if(civrand().Next(100) < cataclysm_chance) {
			Cataclysm();
			return;
		}
	}
*/
	bool openScreen = true;

	m_currentStage++;
	if(m_currentStage >= endgamedb_Get()->GetNumStages()) {

		SlicObject *so = new SlicObject("309EndGameWon");
		so->AddAllRecipientsBut(m_owner);
		so->AddCivilisation(m_owner);
		slicengine_Get()->Execute(so);
		if(g_network.IsHost()) {
			g_network.Block(m_owner);
			g_network.Enqueue(new NetInfo(NET_INFO_CODE_WON_END_GAME,
										  m_owner));
			g_network.Unblock(m_owner);
		}

		player_Get(m_owner)->m_score->SetWonByWonder();
		player_Get(m_owner)->GameOver(GAME_OVER_WON_WORMHOLE, -1);
		m_currentStage = endgamedb_Get()->GetNumStages();
		openScreen = false;
		gamesettings_Get()->SetAlienEndGameWon(m_owner);

		sint32 i;
		for(i = 0; i < k_MAX_PLAYERS; i++) {
			if(player_Get(i) && i != m_owner) {
				player_Get(i)->GameOver(GAME_OVER_LOST_OVERRUN_BY_SMURFS, m_owner);
			}
		}
	}
	m_currentStageBegan = currentRound;

	switch (m_currentStage) {
	  case 1: {
		  so = new SlicObject("059EmbryoStage2");
		  so->AddRecipient(m_owner);
		  slicengine_Get()->Execute(so);




		  so = new SlicObject("058AlienScrappedOwner");
		  so->AddAllRecipientsBut(m_owner);
		  so->AddCivilisation(m_owner);
		  slicengine_Get()->Execute(so);

		  if(g_network.IsHost()) {
			  g_network.Enqueue(new NetInfo(NET_INFO_CODE_ALIEN_SCRAPPED_OWNER,
											m_owner));
		  }
		  break;
	  }
	  case 2: {
		  so = new SlicObject("060EmbryoStage3");
		  so->AddRecipient(m_owner);
		  slicengine_Get()->Execute(so);
		  break;
	  }
	}

	if(openScreen && m_owner == player_view::VisiblePlayer()) {
		gameobservers_Get()->NotifyRequestEndGameShow(this);
	}
}

sint32 EndGame::GetCataclysmChance()
{


	sint32 i;
	sint32 maxChance = 0;

	for(i = 0; i < endgamedb_Get()->m_nRec; i++) {
		const EndGameRecord *egrec = endgamedb_Get()->Get(i);
		if(egrec->GetCataclysmNum() > 0) {
			sint32 chance = (egrec->GetCataclysmNum() - m_numBuilt[i]) * egrec->GetCataclysmPercent();
			if(chance > maxChance)
				maxChance = chance;
		}
	}
	return maxChance;
}

sint32 EndGame::GetTurnsForNextStage()
{

	sint32 i;
	sint32 min = -1;
	for(i = 0; i < endgamedb_Get()->m_nRec; i++) {
		const EndGameRecord *egrec = endgamedb_Get()->Get(i);
		if(egrec->ControlsSpeed()) {
			if(m_numBuilt[i] < 0) {

				continue;
			}
			if(min < 0 || egrec->GetTurnsPerStage(m_numBuilt[i]) < min) {
				min = egrec->GetTurnsPerStage(m_numBuilt[i]);
			}
		}
	}
	return min;
}

BOOL EndGame::HaveEnoughECDs()
{
    sint32 i;
    for(i = 0; i < endgamedb_Get()->m_nRec; i++) {
        const EndGameRecord *egrec = endgamedb_Get()->Get(i);
		if(strcmp(stringdb_Get()->GetIdStr(egrec->m_name), "ET_COMMUNICATION_DEVICE") != 0)
			continue;

        if(egrec->RequiredToAdvanceFromStage(m_currentStage) > m_numBuilt[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL EndGame::HaveEnoughFields()
{
    sint32 i;
    for(i = 0; i < endgamedb_Get()->m_nRec; i++) {
        const EndGameRecord *egrec = endgamedb_Get()->Get(i);
		if(strcmp(stringdb_Get()->GetIdStr(egrec->m_name), "CONTAINMENT_FIELD") != 0)
			continue;

        if(egrec->RequiredToAdvanceFromStage(m_currentStage) > m_numBuilt[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL EndGame::HaveMaxSplicers()
{
    sint32 i;
    for(i = 0; i < endgamedb_Get()->m_nRec; i++) {
        const EndGameRecord *egrec = endgamedb_Get()->Get(i);
		if(strcmp(stringdb_Get()->GetIdStr(egrec->m_name), "GENE_SEQUENCER") != 0)
			continue;

        if(m_numBuilt[i] < egrec->GetMaxAllowed()) {
            return FALSE;
        } else {
			return TRUE;
		}
    }
    return TRUE;
}

BOOL EndGame::HaveAllPrerequisites()
{
    sint32 i;
    for(i = 0; i < endgamedb_Get()->m_nRec; i++) {
        const EndGameRecord *egrec = endgamedb_Get()->Get(i);

        if(egrec->RequiredToAdvanceFromStage(m_currentStage) > m_numBuilt[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL EndGame::MetRequirementsForNextStage()
{


	if(m_currentStage >= endgamedb_Get()->GetNumStages()) {

		return TRUE;
	}

	if(m_currentStage >= endgamedb_Get()->GetNumStages() - 1) {

		if(!player_Get(m_owner)->m_advances->HasAdvance(advanceutil_GetAlienLifeAdvance())) {
			return FALSE;
		}
	}

	if (!HaveAllPrerequisites()) {
		return FALSE;
	}

	return TRUE;
}

void EndGame::Cataclysm()
{

	SlicObject *so = new SlicObject("300EndGameCataclysm");
	so->AddRecipient(m_owner);
	slicengine_Get()->Execute(so);

	so = new SlicObject("301EndGameCataclysmOtherCiv");
	so->AddAllRecipientsBut(m_owner);
	so->AddCivilisation(m_owner);
	slicengine_Get()->Execute(so);

	if(g_network.IsHost()) {
		g_network.Enqueue(new NetInfo(NET_INFO_CODE_CATACLYSM_OTHER,
									  m_owner));
	}

	Init();

	sint32 i;
	for(i = 0; i < player_Get(m_owner)->m_all_cities->Num(); i++) {
		player_Get(m_owner)->m_all_cities->Access(i).AccessData()->GetCityData()->RemoveEndGameObjects();
	}


	if(m_owner == player_view::VisiblePlayer()) {
		gameobservers_Get()->NotifyRequestEndGameClose();
	}
}

void EndGame::XLabCaptured()
{
	Init();
	sint32 i;
	for(i = 0; i < player_Get(m_owner)->m_all_cities->Num(); i++) {
		player_Get(m_owner)->m_all_cities->Access(i).AccessData()->GetCityData()->RemoveEndGameObjects();
	}


	if(m_owner == player_view::VisiblePlayer()) {
		gameobservers_Get()->NotifyRequestEndGameClose();
	}
}

sint32 EndGame::GetTurnsSinceStageBegan(sint32 currentRound) const
{
	return(currentRound - m_currentStageBegan);
}

sint32 EndGame::GetStage()
{
	return(m_currentStage);
}

sint32 EndGame::GetNumberBuilt(sint32 type)
{
	return(m_numBuilt[type]);
}

sint32 EndGame::GetDisplayedStage()
{
	return(m_savedCurrentStage);
}

sint32 EndGame::GetNumberShown(sint32 type)
{
	return(m_savedNumBuilt[type]);
}




void EndGame::UpdateDisplayState()
{
	sint32 i;

	m_savedCurrentStage = m_currentStage;

	for (i=0; i < endgamedb_Get()->m_nRec; i++) {
		m_savedNumBuilt[i] = m_numBuilt[i];
    }
}

BOOL EndGame::HasLab()
{
	sint32 i;
	for(i = 0; i < endgamedb_Get()->m_nRec; i++) {
		if(endgamedb_Get()->Get(i)->NotifyLabBuilt()) {
			if(m_numBuilt[i] > 0) {
				return TRUE;
			}
		}
	}
	return FALSE;
}
