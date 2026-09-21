#include "ctp/c3.h"
#include <memory>
#include "gs/utility/safety.h"

#include "gs/gameobj/player.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/DiplomaticRequestData.h"
#include "gs/gameobj/DiplomaticRequest.h"
#include "gs/gameobj/DiplomaticRequestPool.h"
#include "net/general/network.h"
#include "net/general/net_info.h"
#include "gs/outcom/AICause.h"


#include "gs/gameobj/DiplomaticRequestPool.h"   // diplomaticrequestpool_Get()









void DiplomaticRequest::KillRequest()
	{
	DiplomaticRequest	tmp(*this) ;
	tmp.RemoveAllReferences() ;
	}









void DiplomaticRequest::RemoveAllReferences()
	{
	if(network_Get().IsHost())
		{
		network_Get().Enqueue(std::make_unique<NetInfo>(NET_INFO_CODE_KILL_DIP_REQUEST,
									  (uint32)m_id).release());
		}
    sint32 r = GetRecipient();

	if(Player* rp = safe_player(r)) {
		rp->RemoveDiplomaticReferences(*this) ;
	}

    sint32 o = GetOwner();

	if(Player* op = safe_player(o)) {
		op->RemoveDiplomaticReferences(*this) ;
	}

	diplomaticrequestpool_Get()->Del(*this) ;
	}









const DiplomaticRequestData* DiplomaticRequest::GetData() const
	{
	return (diplomaticrequestpool_Get()->GetDiplomaticRequest(*this)) ;
	}









DiplomaticRequestData* DiplomaticRequest::AccessData()
	{
	return (diplomaticrequestpool_Get()->AccessDiplomaticRequest(*this)) ;
	}

void DiplomaticRequest::Complete()
{
	AccessData()->Complete();
}

sint32 DiplomaticRequest::GetTone() const
{
	return GetData()->GetTone();
}

sint32 DiplomaticRequest::GetRound() const
{
	return GetData()->GetRound();
}
