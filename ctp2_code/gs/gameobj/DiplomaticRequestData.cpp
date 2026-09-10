//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Diplomatic request poll
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
// - None
//
//----------------------------------------------------------------------------

#include <memory>

#include "ctp/c3.h"
#include "gs/gameobj/DiplomaticRequestData.h"

#include "ctp/ctp2_utils/c3errors.h"

#include "gs/utility/Globals.h"
#include "AdvanceRecord.h"
#include "gs/gameobj/Player.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/Agreement.h"
#include "gs/gameobj/message.h"
#include "gs/gameobj/DiplomaticRequest.h"
#include "gs/gameobj/MessageData.h"
#include "gs/gameobj/MessagePool.h"
#include "gs/database/StrDB.h"
#include "gs/gameobj/UnitData.h"
#include "gs/slic/SlicObject.h"
#include "gs/slic/SlicEngine.h"
#include "gs/outcom/AICause.h"

#include "net/general/network.h"
#include "net/general/net_info.h"
#include "net/general/net_action.h"
#include "gs/core/player_view.h"

	;

#include "gs/gameobj/Diplomacy_Log.h"
extern Diplomacy_Log *g_theDiplomacyLog;

#include "gs/gameobj/TradePool.h"








DiplomaticRequestData::DiplomaticRequestData(const ID id, sint32 currentRound)
:
    GameObj             (id.m_id),
	m_round             (currentRound),
    m_owner             (PLAYER_INDEX_INVALID),
	m_recipient         (PLAYER_INDEX_INVALID),
	m_thirdParty        (PLAYER_INDEX_INVALID),
    m_request           (REQUEST_TYPE_NULL),
    m_response          (REQUEST_RESPONSE_TYPE_NULL),
    m_tone              (k_MESSAGE_TONE_NEUTRAL),
    m_advance           (0),
    m_reciprocalAdvance (0),
    m_targetCity        (),
	m_reciprocalCity    (),
	m_amount            (0)
{ ; }

DiplomaticRequestData::DiplomaticRequestData(const ID id, const PLAYER_INDEX owner, const PLAYER_INDEX recipient, const REQUEST_TYPE request, sint32 currentRound)
:
    GameObj             (id.m_id),
	m_round             (currentRound),
    m_owner             (owner),
	m_recipient         (recipient),
	m_thirdParty        (PLAYER_INDEX_INVALID),
    m_request           (request),
    m_response          (REQUEST_RESPONSE_TYPE_NULL),
    m_tone              (k_MESSAGE_TONE_NEUTRAL),
    m_advance           (9),
    m_reciprocalAdvance (0),
    m_targetCity        (),
	m_reciprocalCity    (),
	m_amount            (0)
{ ; }













void DiplomaticRequestData::SetThirdParty(const PLAYER_INDEX thirdParty)
{
	Assert(thirdParty >=0 && thirdParty<k_MAX_PLAYERS) ;
	m_thirdParty = thirdParty ;
}












void DiplomaticRequestData::SetResponse(const REQUEST_RESPONSE_TYPE response)
{
	m_response = response ;
}













void DiplomaticRequestData::SetTarget(const Unit &city)
{
	m_targetCity = city ;
}












void DiplomaticRequestData::SetGold(const Gold &amount)
{
	m_amount = amount ;
#if 0
	if(network_Get().IsClient())
	{
		network_Get().SendAction(new NetAction(NET_ACTION_SET_REQUEST_GOLD,
										   (uint32)m_id,
										   (uint32)m_amount.GetLevel()));
	}
	else
	{
		network_Get().Enqueue(this);
	}
#endif
}











void DiplomaticRequestData::Dump(const sint32 i)
{
	switch (m_request)
	{
		case REQUEST_TYPE_GREETING :
			DPRINTF(k_DBG_INFO, ("%d - From P%d to P%d : Greetings\n", i, m_owner, m_recipient)) ;
			break ;

		case REQUEST_TYPE_DEMAND_ADVANCE :
			DPRINTF(k_DBG_INFO, ("%d - From P%d to P%d : Demand Advance #%d\n", i, m_owner, m_recipient, m_advance)) ;
			break ;

		case REQUEST_TYPE_DEMAND_CITY :
			DPRINTF(k_DBG_INFO, ("%d - From P%d to P%d : demand City #%d\n", i, m_owner, m_recipient, m_targetCity)) ;
			break ;

		case REQUEST_TYPE_DEMAND_MAP :
			DPRINTF(k_DBG_INFO, ("%d - From P%d to P%d : demand map\n", i, m_owner, m_recipient)) ;
			break ;

		case REQUEST_TYPE_DEMAND_GOLD :
			DPRINTF(k_DBG_INFO, ("%d - From P%d to P%d : demand %d Gold\n", i, m_owner, m_recipient, m_amount)) ;
			break ;

		case REQUEST_TYPE_DEMAND_STOP_TRADE :
			DPRINTF(k_DBG_INFO, ("%d - From P%d to P%d : demand stop trading with P%d\n", i, m_owner, m_recipient, m_thirdParty)) ;
			break ;

		case REQUEST_TYPE_DEMAND_ATTACK_ENEMY :
			DPRINTF(k_DBG_INFO, ("%d - From P%d to P%d : demand attack enemy P%d\n", i, m_owner, m_recipient, m_thirdParty)) ;
			break ;

		case REQUEST_TYPE_DEMAND_LEAVE_OUR_LANDS :
			DPRINTF(k_DBG_INFO, ("%d - From P%d to P%d : vacate lands\n", i, m_owner, m_recipient)) ;
			break ;

		case REQUEST_TYPE_DEMAND_REDUCE_POLLUTION :
			DPRINTF(k_DBG_INFO, ("%d - From P%d to P%d : reduce pollution\n", i, m_owner, m_recipient)) ;
			break ;

		case REQUEST_TYPE_OFFER_ADVANCE :
			DPRINTF(k_DBG_INFO, ("%d - From P%d to P%d : offer of Advance #%d\n", i, m_owner, m_recipient, m_advance)) ;
			break ;

		case REQUEST_TYPE_OFFER_CITY :
			DPRINTF(k_DBG_INFO, ("%d - From P%d to P%d : offer of City #%d\n", i, m_owner, m_recipient, m_targetCity)) ;
			break ;

		case REQUEST_TYPE_OFFER_MAP :
			DPRINTF(k_DBG_INFO, ("%d - From P%d to P%d : offer of map\n", i, m_owner, m_recipient)) ;
			break ;

		case REQUEST_TYPE_OFFER_GOLD :
			DPRINTF(k_DBG_INFO, ("%d - From P%d to P%d : offer of %d Gold\n", i, m_owner, m_recipient, m_amount)) ;
			break ;

		case REQUEST_TYPE_OFFER_CEASE_FIRE :
			DPRINTF(k_DBG_INFO, ("%d - From P%d to P%d : offers cease fire\n", i, m_owner, m_recipient)) ;
			break ;

		case REQUEST_TYPE_OFFER_PERMANENT_ALLIANCE :
			DPRINTF(k_DBG_INFO, ("%d - From P%d to P%d : offer permanent alliance\n", i, m_owner, m_recipient)) ;
			break ;

		case REQUEST_TYPE_OFFER_PACT_CAPTURE_CITY :
			DPRINTF(k_DBG_INFO, ("%d - From P%d to P%d : offer pact to capture City #%d\n", i, m_owner, m_recipient, m_targetCity)) ;
			break ;

		case REQUEST_TYPE_OFFER_PACT_END_POLLUTION :
			DPRINTF(k_DBG_INFO, ("%d - From P%d to P%d : offer pact to end pollution\n", i, m_owner, m_recipient)) ;
			break ;

		case REQUEST_TYPE_EXCHANGE_ADVANCE :
			DPRINTF(k_DBG_INFO, ("%d - From P%d to P%d : exchange Advance #%d for Advance #%d\n", i, m_owner, m_recipient, m_advance, m_reciprocalAdvance)) ;
			break ;

		case REQUEST_TYPE_EXCHANGE_CITY :
			DPRINTF(k_DBG_INFO, ("%d - From P%d to P%d : exchange City #%d for City #%d\n", i, m_owner, m_recipient, m_reciprocalCity, m_targetCity)) ;
			break ;

		case REQUEST_TYPE_EXCHANGE_MAP :
			DPRINTF(k_DBG_INFO, ("%d - From P%d to P%d : exchange maps\n", i, m_owner, m_recipient)) ;
			break ;

		default :
			Assert(0) ;

			break ;

	}

}













void DiplomaticRequestData::Enact(BOOL fromCurPlayer)
{
	std::unique_ptr<SlicObject> so;

#ifdef _DEBUG
    if (g_theDiplomacyLog) {
        g_theDiplomacyLog->LogEnact(m_owner, m_recipient, m_request);
    }
#endif // _DEBUG

	if(network_Get().IsClient() && !fromCurPlayer) {
		network_Get().SendAction(new NetAction(NET_ACTION_ENACT_REQUEST,
										   (uint32)m_id));
		DiplomaticRequest me(m_id);
		if(!network_Get().IsLocalPlayer(player_view::CurPlayer()))
			network_Get().AddEnact(me);

		if(!network_Get().IsMyTurn())
			return;
	} else if(network_Get().IsHost()) {
		if(!fromCurPlayer && !network_Get().IsLocalPlayer(player_view::CurPlayer())) {
			network_Get().QueuePacket(network_Get().IndexToId(player_view::CurPlayer()),
								  new NetInfo(NET_INFO_CODE_ENACT_REQUEST_NEED_ACK,
											  (uint32)m_id));
			DiplomaticRequest me(m_id);
			network_Get().AddEnact(me);
			return;
		} else {
			network_Get().Block(player_view::CurPlayer());
			network_Get().Enqueue(new NetInfo(NET_INFO_CODE_ENACT_REQUEST,
										  (uint32)m_id));
			network_Get().Unblock(player_view::CurPlayer());
		}
	}

	if(!player_Get(m_owner) || !player_Get(m_recipient)) {

		DiplomaticRequest me(m_id);
		me.Kill();
		return;
	}

	m_response = REQUEST_RESPONSE_TYPE_ACCEPTED ;
	switch (m_request)
		{
		case REQUEST_TYPE_DEMAND_ADVANCE :
			player_Get(m_owner)->MakeShortCeaseFire(m_recipient, AGREEMENT_TYPE_DEMAND_ADVANCE) ;
			player_Get(m_recipient)->GiveAdvance(m_owner, m_advance, CAUSE_SCI_DIPLOMACY) ;
			slicengine_Get()->RunDiscoveryTradedTriggers(m_recipient, m_owner, m_advance);

			so.reset(new SlicObject("01dipAcceptDemandAdvance"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
			so->AddAdvance(m_advance) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_DEMAND_CITY :
#ifdef _DIPLOMATIC_CITY_EXCHANGE
			player_Get(m_owner)->MakeShortCeaseFire(m_recipient, AGREEMENT_TYPE_DEMAND_CITY) ;
			player_Get(m_recipient)->GiveCity(m_owner, m_targetCity) ;
			so.reset(new SlicObject("01dipAcceptDemandCity"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
			so->AddCity(m_targetCity) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
#endif
			break ;

		case REQUEST_TYPE_DEMAND_MAP :
			player_Get(m_owner)->MakeShortCeaseFire(m_recipient, AGREEMENT_TYPE_DEMAND_MAP) ;
			player_Get(m_recipient)->GiveMap(m_owner) ;
			so.reset(new SlicObject("01dipAcceptDemandMaps"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_DEMAND_GOLD :
			player_Get(m_owner)->MakeShortCeaseFire(m_recipient, AGREEMENT_TYPE_DEMAND_GOLD) ;
			if (player_Get(m_recipient)->GiveGold(m_owner, m_amount))
                so.reset(new SlicObject("01dipAcceptDemandGold"));
			else
                so.reset(new SlicObject("01dipRejectDemandGold"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner);
			so->AddCivilisation(m_recipient) ;
			so->AddGold(m_amount.GetLevel()) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_DEMAND_STOP_TRADE :
			player_Get(m_owner)->MakeShortCeaseFire(m_recipient, AGREEMENT_TYPE_DEMAND_STOP_TRADE, m_thirdParty) ;
			player_Get(m_recipient)->StopTradingWith(m_thirdParty) ;
            tradepool_Get()->BreakOffTrade(m_owner, m_thirdParty);
			so.reset(new SlicObject("01dipAcceptDemandStoptrade"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
			so->AddCivilisation(m_thirdParty) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));





			break ;
		case REQUEST_TYPE_DEMAND_ATTACK_ENEMY:
			player_Get(m_owner)->MakeShortCeaseFire(m_recipient, AGREEMENT_TYPE_DEMAND_ATTACK_ENEMY) ;
			so.reset(new SlicObject("01dipAcceptDemandAttack"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
			so->AddCivilisation(m_thirdParty) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));


            tradepool_Get()->BreakOffTrade(m_owner, m_thirdParty);
            tradepool_Get()->BreakOffTrade(m_recipient, m_thirdParty);
			break ;

		case REQUEST_TYPE_DEMAND_LEAVE_OUR_LANDS :
			player_Get(m_owner)->MakeLeaveOurLands(m_recipient) ;
			so.reset(new SlicObject("01dipAcceptDemandLeave"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_DEMAND_REDUCE_POLLUTION :
			player_Get(m_owner)->MakeReducePollution(m_recipient) ;
			so.reset(new SlicObject("01dipAcceptDemandPollution"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_OFFER_ADVANCE :
			player_Get(m_owner)->MakeShortCeaseFire(m_recipient, AGREEMENT_TYPE_OFFER_ADVANCE) ;
			player_Get(m_owner)->GiveAdvance(m_recipient, m_advance, CAUSE_SCI_DIPLOMACY) ;
			slicengine_Get()->RunDiscoveryTradedTriggers(m_owner, m_recipient, m_advance);

			so.reset(new SlicObject("01dipAcceptOfferAdvance"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
			so->AddAdvance(m_advance) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_OFFER_CITY :
#ifdef _DIPLOMATIC_CITY_EXCHANGE
			player_Get(m_owner)->MakeShortCeaseFire(m_recipient, AGREEMENT_TYPE_OFFER_CITY) ;
			player_Get(m_owner)->GiveCity(m_recipient, m_targetCity) ;
			so.reset(new SlicObject("01dipAcceptOfferCity"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
			so->AddCity(m_targetCity) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
#endif
			break ;

		case REQUEST_TYPE_OFFER_MAP :
			player_Get(m_owner)->MakeShortCeaseFire(m_recipient, AGREEMENT_TYPE_OFFER_MAP) ;
			player_Get(m_owner)->GiveMap(m_recipient) ;
			so.reset(new SlicObject("01dipAcceptOfferMaps"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_OFFER_GOLD :
			player_Get(m_owner)->MakeShortCeaseFire(m_recipient, AGREEMENT_TYPE_OFFER_GOLD) ;
			player_Get(m_owner)->GiveGold(m_recipient, m_amount) ;
			so.reset(new SlicObject("01dipAcceptOfferGold"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
			so->AddGold(m_amount.GetLevel()) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_OFFER_CEASE_FIRE :
			player_Get(m_owner)->MakeCeaseFire(m_recipient) ;
			so.reset(new SlicObject("01dipAcceptTreatyCease"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_OFFER_PERMANENT_ALLIANCE :
			player_Get(m_owner)->FormAlliance(m_recipient) ;
			so.reset(new SlicObject("01dipAcceptTreatyAlliance"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_OFFER_PACT_CAPTURE_CITY :
			player_Get(m_owner)->MakeCaptureCityPact(m_recipient, m_targetCity) ;
			break ;

		case REQUEST_TYPE_OFFER_PACT_END_POLLUTION :
		{
			player_Get(m_owner)->MakeEndPollutionPact(m_recipient) ;
			so.reset(new SlicObject("01dipAcceptTreatyEco"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));

			std::unique_ptr<SlicObject> so2(new SlicObject("01dipAcceptTreatyEco"));


			so2->AddRecipient(m_recipient);
			so2->AddCivilisation(m_recipient) ;
			so2->AddCivilisation(m_owner) ;
			so2->AddAttitude(ATTITUDE_TYPE_NEUTRAL);
			slicengine_Get()->Execute(so2.release()) ;
			break ;
		}

		case REQUEST_TYPE_EXCHANGE_ADVANCE :
			player_Get(m_owner)->MakeShortCeaseFire(m_recipient, AGREEMENT_TYPE_EXCHANGE_ADVANCE) ;
			player_Get(m_owner)->GiveAdvance(m_recipient, m_advance, CAUSE_SCI_DIPLOMACY) ;
			player_Get(m_recipient)->GiveAdvance(m_owner, m_reciprocalAdvance, CAUSE_SCI_DIPLOMACY) ;

			slicengine_Get()->RunDiscoveryTradedTriggers(m_owner, m_recipient, m_advance);
			slicengine_Get()->RunDiscoveryTradedTriggers(m_recipient, m_owner, m_reciprocalAdvance);

			so.reset(new SlicObject("01dipAcceptExchangeAdvance"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
			so->AddAdvance(m_advance) ;
			so->AddAdvance(m_reciprocalAdvance) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_EXCHANGE_CITY :
#ifdef _DIPLOMATIC_CITY_EXCHANGE
			so.reset(new SlicObject("01dipAcceptExchangeCity"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
			so->AddCity(m_targetCity) ;
			so->AddCity(m_reciprocalCity) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));

			player_Get(m_owner)->MakeShortCeaseFire(m_recipient, AGREEMENT_TYPE_EXCHANGE_CITY) ;
			player_Get(m_recipient)->ExchangeCity(m_owner, m_reciprocalCity, m_targetCity) ;
#endif
			break ;

		case REQUEST_TYPE_EXCHANGE_MAP :
			player_Get(m_owner)->MakeShortCeaseFire(m_recipient, AGREEMENT_TYPE_EXCHANGE_MAP) ;
			player_Get(m_recipient)->ExchangeMap(m_owner) ;
			so.reset(new SlicObject("01dipAcceptExchangeMaps"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_DEMAND_NO_PIRACY :
			player_Get(m_owner)->MakeNoPiracyPact(m_recipient) ;
			so.reset(new SlicObject("01dipAcceptDemandPiracy"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		default :

			break ;

		}

	    if(so) {
			so->SetIsDiplomaticResponse();
			slicengine_Get()->Execute(so.release());
		}
		player_Get(m_owner)->RegisterDiplomaticResponse(DiplomaticRequest(m_id));

#ifdef _DEBUG
    if (g_theDiplomacyLog) {
        g_theDiplomacyLog->LogRegard(m_owner, m_recipient);
        g_theDiplomacyLog->LogRegard(m_recipient, m_owner);
    }
#endif // _DEBUG

	DiplomaticRequest me(m_id);
	me.Kill();


}












ATTITUDE_TYPE DiplomaticRequestData::GetAttitude(PLAYER_INDEX p1, PLAYER_INDEX p2)
{
	Assert((p1>=0) && (p1<k_MAX_PLAYERS)) ;
	Assert((player_Get(p1)) && (!player_Get(p1)->IsDead())) ;
	Assert((p2>=0) && (p2<k_MAX_PLAYERS)) ;
	Assert((player_Get(p2)) && (!player_Get(p2)->IsDead())) ;

	return (player_Get(p1)->GetAttitude(p2)) ;
}













void DiplomaticRequestData::Reject(BOOL fromServer)
{
	std::unique_ptr<SlicObject> so;

	if(network_Get().IsClient() && !fromServer) {
		network_Get().SendAction(new NetAction(NET_ACTION_REJECT_REQUEST,
										   (uint32)m_id));
	} else if(network_Get().IsHost()) {
		network_Get().Block(m_recipient);
		network_Get().Enqueue(new NetInfo(NET_INFO_CODE_REJECT_REQUEST,
									  (uint32)m_id));
		network_Get().Unblock(m_recipient);
	}

	if(!player_Get(m_owner) || player_Get(m_owner)->m_isDead ||
	   !player_Get(m_recipient) || player_Get(m_recipient)->m_isDead) {
		return;
	}

#ifdef _DEBUG
    if (g_theDiplomacyLog) {
        g_theDiplomacyLog->LogReject(m_owner, m_recipient, m_request);
    }
#endif

	m_response = REQUEST_RESPONSE_TYPE_REJECTED ;
	switch (m_request)
		{
		case REQUEST_TYPE_GREETING :
			break ;

		case REQUEST_TYPE_DEMAND_ADVANCE :
			so.reset(new SlicObject("01dipRejectDemandAdvance"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
			so->AddAdvance(m_advance) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_DEMAND_CITY :
			so.reset(new SlicObject("01dipRejectDemandCity"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
			so->AddCity(m_targetCity) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_DEMAND_MAP :
			so.reset(new SlicObject("01dipRejectDemandMaps"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_DEMAND_GOLD :
			so.reset(new SlicObject("01dipRejectDemandGold"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
			so->AddGold(m_amount.GetLevel()) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_DEMAND_STOP_TRADE :
			so.reset(new SlicObject("01dipRejectDemandStoptrade"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
			if(!player_Get(m_thirdParty) || player_Get(m_thirdParty)->IsDead()) {
				return;
			}
			so->AddCivilisation(m_thirdParty) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_DEMAND_ATTACK_ENEMY :
			so.reset(new SlicObject("01dipRejectDemandAttack"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
			if(!player_Get(m_thirdParty) || player_Get(m_thirdParty)->IsDead()) {
				return;
			}
			so->AddCivilisation(m_thirdParty) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_DEMAND_LEAVE_OUR_LANDS :
			so.reset(new SlicObject("01dipRejectDemandLeave"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_DEMAND_REDUCE_POLLUTION :
			so.reset(new SlicObject("01dipRejectDemandPollution"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_OFFER_ADVANCE :
			so.reset(new SlicObject("01dipRejectOfferAdvance"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
			so->AddAdvance(m_advance) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_OFFER_CITY :
			so.reset(new SlicObject("01dipRejectOfferCity"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
			so->AddCity(m_targetCity) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_OFFER_MAP :
			so.reset(new SlicObject("01dipRejectOfferMaps"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_OFFER_GOLD :
			so.reset(new SlicObject("01dipRejectOfferGold"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
			so->AddGold(m_amount.GetLevel()) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_OFFER_CEASE_FIRE :
			so.reset(new SlicObject("01dipRejectTreatyCease"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_OFFER_PERMANENT_ALLIANCE :
			so.reset(new SlicObject("01dipRejectTreatyAlliance"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_OFFER_PACT_CAPTURE_CITY :
			break ;

		case REQUEST_TYPE_OFFER_PACT_END_POLLUTION :
			so.reset(new SlicObject("01dipRejectTreatyEco"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_EXCHANGE_CITY :
			so.reset(new SlicObject("01dipRejectExchangeCity"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
			so->AddCity(m_targetCity) ;
			so->AddCity(m_reciprocalCity) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_EXCHANGE_ADVANCE :
			so.reset(new SlicObject("01dipRejectExchangeAdvance"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
			so->AddAdvance(m_advance) ;
			so->AddAdvance(m_reciprocalAdvance) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_EXCHANGE_MAP :
			so.reset(new SlicObject("01dipRejectExchangeMaps"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		case REQUEST_TYPE_DEMAND_NO_PIRACY :
			so.reset(new SlicObject("01dipRejectDemandPiracy"));
			so->AddRecipient(m_owner) ;
			so->AddCivilisation(m_owner) ;
			so->AddCivilisation(m_recipient) ;
            so->AddAttitude(GetAttitude(m_recipient, m_owner));
			break ;

		default :
			c3errors_FatalDialogFromDB("DIPLOMACY_ERROR", "DIPLOMACY_INVALID_REQUEST_FOUND") ;
			break ;

		}

	if(so) {
		so->SetIsDiplomaticResponse();
		slicengine_Get()->Execute(so.release());
	}

	player_Get(m_owner)->RegisterDiplomaticResponse(DiplomaticRequest(m_id));

#ifdef _DEBUG
    if (g_theDiplomacyLog) {
        g_theDiplomacyLog->LogRegard(m_owner, m_recipient);
        g_theDiplomacyLog->LogRegard(m_recipient, m_owner);
    }
#endif // _DEBUG

}

MBCHAR *DiplomaticRequestData::GetRequestString()
{
	switch (m_request) {
		case REQUEST_TYPE_GREETING :
            return("01dipRequestGreetings");
            break;
		case REQUEST_TYPE_DEMAND_ADVANCE :
            return("01dipDemandAdvance");
            break;
		case REQUEST_TYPE_DEMAND_CITY :
            return("01dipDemandCity");
            break;
		case REQUEST_TYPE_DEMAND_MAP :
            return("01dipDemandMaps");
            break;
		case REQUEST_TYPE_DEMAND_GOLD :
            return("01dipDemandGold");
            break;
		case REQUEST_TYPE_DEMAND_STOP_TRADE :
            return("01dipDemandStoptrade");
            break;
		case REQUEST_TYPE_DEMAND_ATTACK_ENEMY :
            return("01dipDemandAttack");
            break;
		case REQUEST_TYPE_DEMAND_LEAVE_OUR_LANDS :
            return("01dipDemandLeave");
            break;
		case REQUEST_TYPE_DEMAND_REDUCE_POLLUTION :
            return("01dipDemandPollution");
            break;
		case REQUEST_TYPE_OFFER_ADVANCE :
            return("01dipOfferAdvance");
            break;
		case REQUEST_TYPE_OFFER_CITY :
            return("01dipOfferCity");
            break;
		case REQUEST_TYPE_OFFER_MAP :
            return("01dipOfferMaps");
            break;
		case REQUEST_TYPE_OFFER_GOLD :
            return("01dipOfferGold");
            break;
		case REQUEST_TYPE_OFFER_CEASE_FIRE :
            return("01dipTreatyCease");
            break;
		case REQUEST_TYPE_OFFER_PERMANENT_ALLIANCE :
            return("01dipTreatyAlliance");
            break;
		case REQUEST_TYPE_OFFER_PACT_CAPTURE_CITY :
            return("01dipTreatyCapture");
            break;
		case REQUEST_TYPE_OFFER_PACT_END_POLLUTION :
            return("01dipTreatyEco");
            break;
		case REQUEST_TYPE_EXCHANGE_CITY :
            return("01dipExchangeCity");
            break;
		case REQUEST_TYPE_EXCHANGE_ADVANCE :
            return("01dipExchangeAdvance");
            break;
		case REQUEST_TYPE_EXCHANGE_MAP :
            return("01dipExchangeMaps");
            break;
		case REQUEST_TYPE_DEMAND_NO_PIRACY :
            return("01dipDemandPiracy");
            break;
		default :
			c3errors_FatalDialogFromDB("DIPLOMACY_ERROR", "DIPLOMACY_INVALID_REQUEST_FOUND") ;
			break ;

    }
    return(nullptr);
}

void DiplomaticRequestData::SetAdvance(const AdvanceType &advance)
{
	m_advance = advance;
}

void DiplomaticRequestData::SetWanted(const AdvanceType &advance)
{
	m_reciprocalAdvance = advance;
}

void DiplomaticRequestData::SetWanted(const Unit &city)
{
	m_reciprocalCity = city;
}

void DiplomaticRequestData::Complete()
{
	switch(GetAttitude(m_owner, m_recipient)) {
		case ATTITUDE_TYPE_STRONG_HOSTILE:
			m_tone = k_MESSAGE_TONE_HOSTILE;
			break;
		case ATTITUDE_TYPE_WEAK_HOSTILE:
			m_tone = k_MESSAGE_TONE_HOSTILE;
			break;
		case ATTITUDE_TYPE_NEUTRAL:
			m_tone = k_MESSAGE_TONE_NEUTRAL;
			break;
		case ATTITUDE_TYPE_WEAK_FRIENDLY:
			m_tone = k_MESSAGE_TONE_FRIENDLY;
			break;
		case ATTITUDE_TYPE_STRONG_FRIENDLY:
			m_tone = k_MESSAGE_TONE_FRIENDLY;
			break;
		default:
			Assert(FALSE);
			break;
	}

#ifdef _DEBUG
    if (g_theDiplomacyLog) {
        g_theDiplomacyLog->LogTone(m_owner, m_recipient, GetAttitude(m_owner, m_recipient));
    }
#endif // _DEBUG

	if(network_Get().IsHost()) {
		network_Get().Enqueue(this);
	} else if(network_Get().IsClient()) {
		network_Get().AddCreatedObject(this);
		network_Get().SendDiplomaticRequest(this);
	}
}

sint32 DiplomaticRequestData::GetTone() const
{
	return m_tone;
}
