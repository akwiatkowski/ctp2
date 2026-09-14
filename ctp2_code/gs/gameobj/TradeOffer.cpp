#include "ctp/c3.h"

#include "gs/gameobj/TradeOffer.h"
#include "gs/gameobj/TradeOfferPool.h"
#include "gs/gameobj/player.h"
#include "gs/utility/safety.h"          // safe_player
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
	if(Player * owner = safe_player(GetOwner()))
		owner->RemoveTradeOffer(*this);

	if(network_Get().IsHost()) {
		network_Get().Block(GetOwner());
		network_Get().Enqueue(new NetInfo(NET_INFO_CODE_KILL_TRADE_OFFER,
									  (uint32)(*this)));
		network_Get().Unblock(GetOwner());
	} else if(network_Get().IsClient()) {
		network_Get().AddDeadUnit(m_id);
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

	if (gameobservers_Get()) gameobservers_Get()->NotifyTradeChanged();
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
