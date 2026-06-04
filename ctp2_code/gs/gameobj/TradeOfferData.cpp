//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Trade Offer data
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

#include "ctp/c3.h"
#include "gs/gameobj/TradeOfferData.h"
#include "gs/gameobj/Player.h"
#include "gs/gameobj/Gold.h"
#include "net/general/network.h"
#include "net/general/net_action.h"
#include "net/general/net_info.h"
#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicObject.h"

BOOL TradeOfferData::Accept(PLAYER_INDEX player,
                            const Unit &sourceCity,
                            Unit const & destCity)
{
#ifndef RECIPROCAL_ROUTES
	Assert(m_askingType == ROUTE_TYPE_GOLD);
	if(m_askingType != ROUTE_TYPE_GOLD)
		return FALSE;
#endif

	SlicObject *so = new SlicObject("363TradeOfferAccepted");
	so->AddRecipient(destCity.GetOwner());
	so->AddCivilisation(m_fromCity.GetOwner());
	so->AddCity(m_fromCity);
	so->AddCity(destCity);
	so->AddGood(m_offerResource);
	slicengine_Get()->Execute(so);

	if(g_network.IsHost()) {
		g_network.Enqueue(new NetInfo(NET_INFO_CODE_SEND_OFFER_ACCEPT_MESSAGE,
									  m_fromCity.m_id, destCity.m_id, m_offerResource));
	}

	if(g_network.IsClient()) {
		g_network.SendAction(new NetAction(NET_ACTION_ACCEPT_TRADE_OFFER,
										   m_id,
										   player,
										   (uint32)sourceCity,
										   (uint32)destCity));
		return FALSE;
	}

	if(m_offerType == ROUTE_TYPE_SLAVE) {
		if(m_fromCity.SendSlaveTo(destCity)) {
			Gold giveAmount;
			giveAmount.SetLevel(m_askingResource);
			player_Get(player)->GiveGold(m_fromCity.GetOwner(), giveAmount);
			return TRUE;
		}
		return FALSE;
	}

	if(m_offerType == ROUTE_TYPE_RESOURCE &&
	   !m_fromCity.HasResource(m_offerResource)) {

		return TRUE;
	}

	TradeRoute fromRoute;
	TradeRoute toRoute;
	fromRoute = player_Get(m_fromCity.GetOwner())->CreateTradeRoute(
		m_fromCity, m_offerType, m_offerResource, destCity, destCity.GetOwner(), m_askingResource);

	if (fromRoute.IsValid())
    {
#ifdef RECIPROCAL_ROUTES
	    toRoute = player_Get(player)->CreateTradeRoute(sourceCity,
												     m_askingType,
												     m_askingResource,
												     m_toCity,
												     m_fromCity.GetOwner());
	    if (toRoute.IsValid())
        {
    	    toRoute.SetRecip(fromRoute);
	        fromRoute.SetRecip(toRoute);
        }
        else
        {
		    fromRoute.Kill();
	    }
#endif
    }

	return fromRoute.IsValid();
}
