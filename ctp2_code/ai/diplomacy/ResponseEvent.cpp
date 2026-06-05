//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Diplomacy response events
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
// - Added HotSeat and PBEM human-human diplomacy support. (17-Oct-2007 Martin G�hmann)
// - Seperated the NewProposal event from the Response event so that the
//   NewProposal event can be called from slic witout any problems. (17-Oct-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "ai/diplomacy/ResponseEvent.h"

#include "gs/gameobj/Events.h"
#include "gs/events/GameEventUser.h"
#include "gs/gameobj/Unit.h"

#include "gs/database/StrDB.h"
#include "gs/events/GameEventManager.h"

#include "gs/outcom/AICause.h"
#include "gs/gameobj/Player.h"

#include "ai/diplomacy/Diplomat.h"
#include "ai/mapanalysis/mapanalysis.h"

#include "gs/core/player_view.h"

#include "net/general/network.h"
#include "gs/utility/TurnCnt.h"

STDEHANDLER(NewProposalEvent)
{
	PLAYER_INDEX sender;
	PLAYER_INDEX receiver;

	if (!args->GetPlayer(0, sender))
		return GEV_HD_Continue;

	if (!args->GetPlayer(1, receiver))
		return GEV_HD_Continue;

	Diplomat::GetDiplomat(sender).ExecuteEventNewProposal(receiver);

	return GEV_HD_Continue;
}

STDEHANDLER(ResponseEvent)
{
	PLAYER_INDEX sender;
	PLAYER_INDEX receiver;

	if (!args->GetPlayer(0, sender))
		return GEV_HD_Continue;

	if (!args->GetPlayer(1, receiver))
		return GEV_HD_Continue;

	if (!player_Get(sender) || !player_Get(receiver))
		return GEV_HD_Continue;

	Diplomat & receiver_diplomat = Diplomat::GetDiplomat(receiver);
	Diplomat & sender_diplomat = Diplomat::GetDiplomat(sender);

	const Response & receiver_response_pending = receiver_diplomat.GetResponsePending(sender);
	const Response & sender_response_pending = sender_diplomat.GetResponsePending(receiver);

	bool show_response;
	if (sender_diplomat.GetReceiverHasInitiative(receiver))
	{
		show_response =
		        receiver == player_view::VisiblePlayer()
		    &&!(sender_response_pending == Diplomat::s_badResponse);
	}
	else
	{
		show_response =
		        sender == player_view::VisiblePlayer()
			&&!(receiver_response_pending == Diplomat::s_badResponse);
	}

	if (show_response)
	{
		gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_ResponseReady,
							   GEA_Player, sender,
							   GEA_Player, receiver,
							   GEA_End);
	}

	// If AI or network are involved
	if(
	   (
	    (           player_Get(sender)->IsRobot()
	       && (    !network_Get().IsActive()
	            ||  network_Get().IsLocalPlayer(sender)
	          )
	    )
	||  (           network_Get().IsActive()
	       &&       network_Get().IsLocalPlayer(sender)
	       &&       sender != player_view::VisiblePlayer()
	    )
	   )
	&&             !sender_diplomat.GetReceiverHasInitiative(receiver)
	  )
	{
		sender_diplomat.ExecuteResponse(sender, receiver);
		return GEV_HD_Continue;
	}

	// If AI is involved
	if(
	   (
	    (           player_Get(receiver)->IsRobot()
	       && (    !network_Get().IsActive()
	            ||  network_Get().IsLocalPlayer(receiver)
	          )
	    )
	||  (           network_Get().IsActive()
	       &&       network_Get().IsLocalPlayer(receiver)
	       &&       receiver != player_view::VisiblePlayer()
	    )
	   )
	&&              sender_diplomat.GetReceiverHasInitiative(receiver)
	  )
	{
		receiver_diplomat.ExecuteResponse(sender, receiver);
		return GEV_HD_Continue;
	}

	return GEV_HD_Continue;
}

void ResponseEventCallbacks::AddCallbacks()
{

	gevmanager_Get()->AddCallback(GEV_NewProposal,
							  GEV_PRI_Primary,
							  &s_NewProposalEvent);

	gevmanager_Get()->AddCallback(GEV_ProposalResponse,
							  GEV_PRI_Primary,
							  &s_ResponseEvent);

	gevmanager_Get()->AddCallback(GEV_Reject,
							  GEV_PRI_Primary,
							  &s_ResponseEvent);

	gevmanager_Get()->AddCallback(GEV_Counter,
							  GEV_PRI_Primary,
							  &s_ResponseEvent);

	gevmanager_Get()->AddCallback(GEV_Threaten,
							  GEV_PRI_Primary,
							  &s_ResponseEvent);
}
