#include "ctp/c3.h"

#include "gs/gameobj/TradeOffer.h"
#include "robot/aibackdoor/civarchive.h"
#include "gs/gameobj/TradeOfferPool.h"
#include "gs/gameobj/Player.h"
#include "net/general/net_info.h"
#include "net/general/network.h"

#include "gs/gameobj/message.h"
#include "gs/gameobj/MessageData.h"
#include "gs/core/game_observer.h"

void TradeOffer::KillOffer()
{
	TradeOffer tmp(*this);
	tmp.RemoveAllReferences();
}

void TradeOffer::RemoveAllReferences()
{
	player_Get(GetOwner())->RemoveTradeOffer(*this);

	if(g_network.IsHost()) {
		g_network.Block(GetOwner());
		g_network.Enqueue(new NetInfo(NET_INFO_CODE_KILL_TRADE_OFFER,
									  (uint32)(*this)));
		g_network.Unblock(GetOwner());
	} else if(g_network.IsClient()) {
		g_network.AddDeadUnit(m_id);
	}


	tradeofferpool_Get()->Remove(*this);

	sint32 p;
	for(p = 0; p < k_MAX_PLAYERS; p++) {
		if(player_Get(p)) {
			sint32 i;
			for(i = player_Get(p)->m_messages->Num() - 1; i >= 0; i--) {
			}
		}
	}

	if (g_gameObservers) g_gameObservers->NotifyTradeChanged();
}

const TradeOfferData* TradeOffer::GetData() const
{
	return tradeofferpool_Get()->GetTradeOffer(*this);
}

TradeOfferData* TradeOffer::AccessData()
{
	return tradeofferpool_Get()->AccessTradeOffer(*this);
}

BOOL TradeOffer::Accept(PLAYER_INDEX player,
			const Unit &sourceCity,
			Unit const &destCity)
{
	return AccessData()->Accept(player, sourceCity, destCity);
}
