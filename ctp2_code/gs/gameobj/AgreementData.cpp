//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Agreement data
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
// _DEBUG
// - Generate debug version
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
// - Replaced old const database by new one. (5-Aug-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/utility/safety.h"
#include "ctp/ctp2_utils/c3errors.h"

#include "gs/utility/Globals.h"
#include "gs/gameobj/Player.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/Gold.h"
#include "gs/gameobj/Advances.h"
#include "gs/gameobj/AgreementData.h"
#include "gs/gameobj/Civilisation.h"
#include "gs/gameobj/DiplomaticRequestData.h"
#include "AdvanceRecord.h"
#include "gs/gameobj/UnitPool.h"
#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicObject.h"
#include "gs/gameobj/UnitData.h"
#include "gs/gameobj/XY_Coordinates.h"
#include "gs/world/World.h"
#include "gs/world/Cell.h"
#include "ConstRecord.h"

#include "gs/gameobj/Agreement.h"

#include "net/general/network.h"

#include "gs/outcom/AICause.h"
#include "gs/gameobj/AgreementPool.h"

#include "gs/gameobj/AgreementPool.h"   // agreementpool_Get()

#include "gs/gameobj/Diplomacy_Log.h"
extern Diplomacy_Log *g_theDiplomacyLog;








AgreementData::AgreementData(const ID id) : GameObj(id.m_id)
{
    Init();

	m_agreement = AGREEMENT_TYPE_NULL ;

	m_round = 0 ;
	m_expires = 0 ;
	m_owner = PLAYER_INDEX_INVALID ;
	m_recipient = PLAYER_INDEX_INVALID ;
	m_isBroken = FALSE;
	m_ownerPollution = 0;
	m_recipientPollution = 0;
}









AgreementData::AgreementData(const ID id, const PLAYER_INDEX owner,
   const PLAYER_INDEX recipient, const AGREEMENT_TYPE agreement, sint32 currentRound) : GameObj(id.m_id)
{
    Init();

    m_id = id;
	m_agreement = agreement ;

	m_round = currentRound ;
	m_expires = k_EXPIRATION_NEVER ;
	m_owner = owner ;
	m_recipient = recipient ;
	m_isBroken = FALSE;

	if(safe_player(owner)) {
		m_ownerPollution = player_Get(owner)->GetCurrentPollution();
	} else {
		m_ownerPollution = 0;
	}

	if(safe_player(recipient)) {
		m_recipientPollution = safe_player(m_recipient)->GetCurrentPollution();
	} else {
		m_recipientPollution = 0;
	}

	ENQUEUE();
}

void AgreementData::Init()
{
    m_owner = -1;
	m_recipient = -1;
    m_thirdParty = -1;
	m_agreement = AGREEMENT_TYPE_NULL;

	m_round = -1;
    m_expires = k_EXPIRATION_NEVER;
	m_isBroken = FALSE;
    m_targetCity = Unit();
}












void AgreementData::MakeAgreement(PLAYER_INDEX owner, PLAYER_INDEX recipient, AGREEMENT_TYPE agreement, sint32 currentRound)
	{
	m_owner = owner ;
	m_recipient = recipient ;
	m_agreement = agreement ;

	m_round = currentRound ;
	m_expires = k_EXPIRATION_NEVER ;


	m_thirdParty = PLAYER_INDEX_INVALID ;

#ifdef _DEBUG
        if (g_theDiplomacyLog) {
            g_theDiplomacyLog->LogMakeAgreement(m_owner, m_recipient, m_thirdParty,
                m_agreement);
        }
#endif // _DEBUG

	ENQUEUE();
}























































































































void AgreementData::SetTarget(const Unit &city)
	{
	m_targetCity = city ;
	}



























































void AgreementData::Dump(const sint32 i)
	{
	MBCHAR	s[_MAX_PATH] ;

	int len = snprintf(s, sizeof(s), "%d (%d) - P%d & P%d agree to ", i, m_expires, m_owner, m_recipient) ;
	if (len < 0 || len >= (int)sizeof(s)) return;

	switch (m_agreement)
		{
		case AGREEMENT_TYPE_DEMAND_STOP_TRADE :
			snprintf(s + len, sizeof(s) - len, " stop trade with P%d", m_thirdParty) ;
			break ;

		case AGREEMENT_TYPE_DEMAND_LEAVE_OUR_LANDS :
			snprintf(s + len, sizeof(s) - len, " leave the lands") ;
			break ;

		case AGREEMENT_TYPE_REDUCE_POLLUTION :
			snprintf(s + len, sizeof(s) - len, " reduce pollution") ;
			break ;

		case AGREEMENT_TYPE_CEASE_FIRE :
			snprintf(s + len, sizeof(s) - len, " cease fire") ;
			break ;

		case AGREEMENT_TYPE_PACT_CAPTURE_CITY :
			snprintf(s + len, sizeof(s) - len, " capture city %d", m_targetCity.m_id) ;
			break ;

		case AGREEMENT_TYPE_PACT_END_POLLUTION :
			snprintf(s + len, sizeof(s) - len, " end pollution") ;
			break ;

		default :
			snprintf(s + len, sizeof(s) - len, " \"Unknown diplomatic agreement type\"") ;
			break ;

		}

	DPRINTF(k_DBG_INFO, ("%s\n", s)) ;
	}



















































void AgreementData::ExtractPlayer(sint32 indexId, sint32 memberId, MBCHAR *sExpanded)
	{
	Civilisation	*civ = nullptr;

	if (indexId >= 2)
		{
		if (m_agreement != AGREEMENT_TYPE_DEMAND_STOP_TRADE)

			{
			c3errors_ErrorDialogFromDB("AGREEMENT_ERROR", "AGREEMENT_ERROR_NOT_DEMAND_STOP_TRADE") ;
			return;
			}

		}

	switch (indexId)
		{
		case 0 :
			civ = safe_player(m_owner)->GetCivilisation() ;
			break ;

		case 1 :
			civ = safe_player(m_recipient)->GetCivilisation() ;
			break ;

		case 2 :
			civ = safe_player(m_thirdParty)->GetCivilisation() ;
			break ;

		default :
			c3errors_ErrorDialogFromDB("AGREEMENT_ERROR", "AGREEMENT_ERROR_INDEX_OUT_OF_BOUNDS") ;
			return ;

		}

	switch (memberId)
		{
		case 0 :
			strlcpy(sExpanded, civ->GetLeaderName(), sizeof(sExpanded)) ;
			break ;

		case 1 :
			civ->GetSingularCivName(sExpanded) ;
			break ;

		case 2 :
			civ->GetPluralCivName(sExpanded) ;
			break ;

		case 3 :
			civ->GetCountryName(sExpanded) ;
			break ;

		case 4 :
			snprintf(sExpanded, sizeof(sExpanded), "%ld", player_Get(civ->GetOwner())->GetGold()) ;
			break ;

		default :
			c3errors_ErrorDialogFromDB("AGREEMENT_ERROR", "AGREEMENT_ERROR_UNKNOWN_PLAYER_MEMBER") ;
			return;

		}

	}



































































































































































































































sint32 AgreementData::DecrementTurns()
	{
	if (m_expires == k_EXPIRATION_NEVER)
		return (k_EXPIRATION_NEVER) ;

	if (m_expires>0)
		m_expires-- ;

	ENQUEUE();
	return (m_expires) ;
	}











void AgreementData::EndTurn()
	{
	DecrementTurns() ;
	ENQUEUE();
	}


void AgreementData::RecipientIsViolating(PLAYER_INDEX curPlayer, BOOL force, sint32 currentRound)
{

	char objName[256];
	BOOL sendMessage = FALSE;
	sint32 now = currentRound;
	sint32 rounds = now - m_round;
	BOOL tellAi = FALSE;
	sint32 otherCiv = -1;
	BOOL addCity = FALSE;

	switch(m_agreement) {
		case AGREEMENT_TYPE_DEMAND_STOP_TRADE:
			tellAi = TRUE;
			if(rounds > g_theConstDB->Get(0)->GetStopTradeRounds()) {
				sendMessage = TRUE;
				snprintf(objName, sizeof(objName), "008StopTradeWithBroken");
				otherCiv = m_thirdParty;
			}
			break;
		case AGREEMENT_TYPE_DEMAND_LEAVE_OUR_LANDS:
			tellAi = TRUE;
			if(rounds > g_theConstDB->Get(0)->GetLeaveOurLandsRounds() || force) {
				sendMessage = TRUE;
				snprintf(objName, sizeof(objName), "260LeaveOurLandsBroken");
			}
			break;
		case AGREEMENT_TYPE_REDUCE_POLLUTION:
			tellAi = TRUE;
			if(rounds > g_theConstDB->Get(0)->GetReducePollutionRounds()) {
				sendMessage = TRUE;
				snprintf(objName, sizeof(objName), "261ReducePollutionBroken");
			}
			break;
#if 0
		case AGREEMENT_TYPE_PACT_CAPTURE_CITY:
			tellAi = TRUE;
			if(rounds > g_theConstDB->Get(0)->GetCaptureCityRounds()) {
				sendMessage = TRUE;
				snprintf(objName, sizeof(objName), "262CaptureCityBroken");
				addCity = TRUE;
			}
			break;
#endif
		case AGREEMENT_TYPE_PACT_END_POLLUTION:
			tellAi = TRUE;
			if(rounds > g_theConstDB->Get(0)->GetEndPollutionRounds()) {
				sendMessage = TRUE;
				snprintf(objName, sizeof(objName), "263EndPollutionBroken");
			}
			break;
		case AGREEMENT_TYPE_DEMAND_ATTACK_ENEMY:
			tellAi = TRUE;
			if(rounds > g_theConstDB->Get(0)->GetAttackEnemyRounds()) {
				sendMessage = TRUE;
				snprintf(objName, sizeof(objName), "264AttackEnemyBroken");
				otherCiv = m_thirdParty;
			}
			break;
		case AGREEMENT_TYPE_NO_PIRACY:
			tellAi = TRUE;
			sendMessage = TRUE;
			snprintf(objName, sizeof(objName), "004NoPiracyBroken");
			break;
		default:
			break;
	}

	if(tellAi) {





	}

	if(sendMessage) {
		SlicObject *so1 = new SlicObject(objName);
		size_t const objNameLen = strlen(objName);
		snprintf(objName + objNameLen, sizeof(objName) - objNameLen, "ByYou");
		SlicObject *so2 = new SlicObject(objName);

		so1->AddCivilisation(m_recipient);
		so1->AddRecipient(m_owner);

		so2->AddCivilisation(m_owner);
		so2->AddRecipient(m_recipient);
		if(otherCiv >= 0) {
			so1->AddCivilisation(otherCiv);
			so2->AddCivilisation(otherCiv);
		}
		if(addCity) {
			so1->AddCity(m_targetCity);
			so2->AddCity(m_targetCity);
		}
		slicengine_Get()->Execute(so1);
		if(slicengine_Get()->GetSegment(objName)) {
			slicengine_Get()->Execute(so2);
		} else {
			delete so2;
		}


		Agreement me(m_id);
		if(agreementpool_Get()->IsValid(me)) {
			me.Kill();
		}
	}
}

void AgreementData::OwnerIsViolating(PLAYER_INDEX curPlayer, sint32 currentRound)
{

	char objName[256];
	BOOL sendMessage = FALSE;
	sint32 now = currentRound;
	sint32 rounds = now - m_round;
	BOOL tellAi = FALSE;
	sint32 otherCiv = -1;
	BOOL addCity = FALSE;

	switch(m_agreement) {
		case AGREEMENT_TYPE_PACT_END_POLLUTION:

			tellAi = TRUE;
			if(rounds > g_theConstDB->Get(0)->GetEndPollutionRounds()) {
				sendMessage = TRUE;
				snprintf(objName, sizeof(objName), "263EndPollutionBroken");
			}
			break;
		default:
			break;
	}

	if(tellAi) {





	}

	if(sendMessage) {
		SlicObject *so1 = new SlicObject(objName);
		size_t const objNameLen = strlen(objName);
		snprintf(objName + objNameLen, sizeof(objName) - objNameLen, "ByYou");
		SlicObject *so2 = new SlicObject(objName);

		so1->AddCivilisation(m_owner);
		so1->AddRecipient(m_recipient);

		so2->AddCivilisation(m_recipient);
		so2->AddRecipient(m_owner);
		if(otherCiv >= 0) {
			so1->AddCivilisation(otherCiv);
			so2->AddCivilisation(otherCiv);
		}
		if(addCity) {
			so1->AddCity(m_targetCity);
			so2->AddCity(m_targetCity);
		}
		slicengine_Get()->Execute(so1);
		slicengine_Get()->Execute(so2);


		Agreement me(m_id);
		if(agreementpool_Get()->IsValid(me)) {
			me.Kill();
		}
	}
}

void AgreementData::BeginTurnOwner(sint32 currentRound)
{

	if(!player_Get(m_recipient) || safe_player(m_recipient)->m_isDead)
		return;

	switch(m_agreement) {
		case AGREEMENT_TYPE_DEMAND_STOP_TRADE:
		{
			if(player_Get(m_recipient)) {

				sint32 trade = safe_player(m_recipient)->GetTradeWith(m_thirdParty);
				if(trade > 0) {
					RecipientIsViolating(m_owner, FALSE, currentRound);
				}
			}
			break;
		}
		case AGREEMENT_TYPE_DEMAND_LEAVE_OUR_LANDS:
		{
			if(player_Get(m_recipient)) {

				DynamicArray<Army> *armies = safe_player(m_recipient)->m_all_armies;
				sint32 i;
				sint32 n = armies->Num();
				for(i = 0; i < n; i++) {
					MapPoint pos;
					sint32 j;
					BOOL hasAttackUnits = FALSE;
					for(j = 0; j < armies->Access(i).Num(); j++) {
						if(armies->Access(i).Access(j).GetAttack() > 0.00001) {
							hasAttackUnits = TRUE;
							break;
						}
					}
					if(!hasAttackUnits)
						continue;

					armies->Access(i).GetPos(pos);
					if(world_Get()->GetCell(pos)->GetOwner() == m_owner) {
						RecipientIsViolating(m_owner, FALSE, currentRound);
						return;
					}
				}
			}
			break;
		}
		case AGREEMENT_TYPE_REDUCE_POLLUTION:
		{
			if(player_Get(m_recipient)) {
				if(safe_player(m_recipient)->GetCurrentPollution() >= m_recipientPollution) {
					RecipientIsViolating(m_owner, FALSE, currentRound);
				}
			}
			break;
		}
#if 0
		case AGREEMENT_TYPE_PACT_CAPTURE_CITY:
		{
			if(player_Get(m_recipient)) {
				if(unitpool_Get()->IsValid(m_targetCity)) {
					if(m_targetCity.GetOwner() == m_owner ||
					   m_targetCity.GetOwner() == m_recipient) {

					}
				}
			}
			break;
		}
#endif
		case AGREEMENT_TYPE_PACT_END_POLLUTION:
		{
			if(player_Get(m_owner)) {
				sint32 now = currentRound;
				sint32 rounds = now - m_round;
				Agreement me(m_id);
				if(safe_player(m_owner)->GetCurrentPollution() > m_ownerPollution &&
				   safe_player(m_owner)->GetCurrentPollution() > uint32(g_theConstDB->Get(0)->GetMinEcoPactViolationLevel())) {
					OwnerIsViolating(m_owner, currentRound);
				}


				if(agreementpool_Get()->IsValid(me)) {
					if(rounds > g_theConstDB->Get(0)->GetEndPollutionRounds()) {
						m_ownerPollution = safe_player(m_owner)->GetCurrentPollution();
						if(player_Get(m_recipient)) {
							m_recipientPollution = safe_player(m_recipient)->GetCurrentPollution();
						}

						m_round = now;
						if(network_Get().IsHost())
							network_Get().Enqueue(this);
					}
				}
			}
			break;
		}
		case AGREEMENT_TYPE_DEMAND_ATTACK_ENEMY:
		{
			if(player_Get(m_recipient)) {
				if(safe_player(m_recipient)->GetLastAttacked(m_thirdParty) < m_round) {
					RecipientIsViolating(m_owner, FALSE, currentRound);
				}
			}
			break;
		}
		default:

			return;
	}
}

void AgreementData::Break()

{
    m_isBroken = TRUE;

    switch(m_agreement) {
		case AGREEMENT_TYPE_DEMAND_LEAVE_OUR_LANDS:


            break;
        default:
            break;
    }

#ifdef _DEBUG
        if (g_theDiplomacyLog) {
            g_theDiplomacyLog->LogBrokenAgreement(m_owner, m_recipient, m_thirdParty,
                m_agreement);
        }
#endif // _DEBUG

}

void AgreementData::BeginTurnRecipient(sint32 currentRound)
{

	
#if 0   // Unreachable
    if(!player_Get(m_owner) || safe_player(m_owner)->m_isDead)
		return;

	switch(m_agreement) {
		case AGREEMENT_TYPE_DEMAND_STOP_TRADE:
		{
			if(player_Get(m_recipient)) {

				sint32 trade = safe_player(m_recipient)->GetTradeWith(m_thirdParty);
				if(trade > 0) {
					RecipientIsViolating(m_recipient, FALSE, currentRound);
				}
			}
			break;
		}
		case AGREEMENT_TYPE_DEMAND_LEAVE_OUR_LANDS:
		{
			if(player_Get(m_recipient)) {

				DynamicArray<Army> *armies = safe_player(m_recipient)->m_all_armies;
				sint32 i, n = armies->Num();
				for(i = 0; i < n; i++) {
					MapPoint pos;
					sint32 j;
					BOOL hasAttackUnits = FALSE;
					for(j = 0; j < armies->Access(i).Num(); j++) {
						if(armies->Access(i).Access(j).GetAttack() > 0.00001) {
							hasAttackUnits = TRUE;
							break;
						}
					}
					if(!hasAttackUnits)
						continue;

					armies->Access(i).GetPos(pos);
					if(world_Get()->GetCell(pos)->GetOwner() == m_owner) {
						RecipientIsViolating(m_recipient, FALSE, currentRound);
						return;
					}
				}
			}
			break;
		}
		case AGREEMENT_TYPE_REDUCE_POLLUTION:
		{
			if(player_Get(m_recipient)) {
				if(safe_player(m_recipient)->GetCurrentPollution() >= m_recipientPollution) {
					RecipientIsViolating(m_recipient, FALSE, currentRound);
				}
			}
			break;
		}

		case AGREEMENT_TYPE_PACT_CAPTURE_CITY:
		{
			if(player_Get(m_recipient)) {
				if(unitpool_Get()->IsValid(m_targetCity)) {
					if(m_targetCity.GetOwner() == m_owner ||
					   m_targetCity.GetOwner() == m_recipient) {

					}
				}
			}
			break;
		}

        case AGREEMENT_TYPE_PACT_END_POLLUTION:
		{
			if(player_Get(m_recipient)) {
				sint32 now = currentRound;
				sint32 rounds = now - m_round;
				Agreement me(m_id);

				if(safe_player(m_recipient)->GetCurrentPollution() > m_recipientPollution &&
				   safe_player(m_recipient)->GetCurrentPollution() > uint32(g_theConstDB->MinEcoPactViolationLevel())) {
					RecipientIsViolating(m_recipient, FALSE, currentRound);
				}


				if(agreementpool_Get()->IsValid(me)) {
					if(rounds > g_theConstDB->EndPollutionRounds()) {
						m_ownerPollution = safe_player(m_owner)->GetCurrentPollution();
						if(player_Get(m_recipient)) {
							m_recipientPollution = safe_player(m_recipient)->GetCurrentPollution();
						}

						m_round = now;
						if(network_Get().IsHost())
							network_Get().Enqueue(this);
					}
				}
			}
			break;
		}
		case AGREEMENT_TYPE_DEMAND_ATTACK_ENEMY:
		{
			if(player_Get(m_recipient)) {
				if(safe_player(m_recipient)->GetLastAttacked(m_thirdParty) < m_round) {
					RecipientIsViolating(m_recipient, FALSE, currentRound);
				}
			}
			break;
		}
		default:




			return;
	}
#endif
}

void AgreementData::SetExpires(sint32 turns)
{
	Assert((turns == k_EXPIRATION_NEVER) || (turns > 0)) ;
	m_expires = turns ;
	ENQUEUE();
}
