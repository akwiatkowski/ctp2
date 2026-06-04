#include "ctp/c3.h"
#include "gs/utility/safety.h"

#include "gs/gameobj/Gold.h"
#include "gs/gameobj/Player.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/AgreementData.h"
#include "gs/gameobj/Agreement.h"
#include "gs/gameobj/AgreementPool.h"
#include "gs/utility/TurnCnt.h"
#include "net/general/network.h"
#include "net/general/net_info.h"
#include "net/general/net_action.h"


	#include "gs/gameobj/AgreementPool.h"   // agreementpool_Get()









void Agreement::KillAgreement()
	{
	Agreement	tmp(*this) ;
	tmp.RemoveAllReferences() ;
	}









void Agreement::RemoveAllReferences()
{
	if(Player* r = safe_player(GetRecipient())) {
		r->RemoveAgreementReferences(*this) ;
	}
	if(Player* o = safe_player(GetOwner())) {
		o->RemoveAgreementReferences(*this) ;
	}

	if(g_network.IsHost()) {
		g_network.Enqueue(new NetInfo(NET_INFO_CODE_KILL_AGREEMENT,
									  m_id));
	} else if(g_network.IsClient()) {
		g_network.SendAction(new NetAction(NET_ACTION_KILL_AGREEMENT,
										   m_id));
	}

	agreementpool_Get()->Del(*this) ;
}


























const AgreementData* Agreement::GetData() const
	{
	return (agreementpool_Get()->GetAgreement(*this)) ;
	}









AgreementData* Agreement::AccessData()
	{
	return (agreementpool_Get()->AccessAgreement(*this)) ;
	}

void Agreement::Break()
{
	AccessData()->Break();
}

BOOL Agreement::IsBroken() const
{
	return GetData()->IsBroken();
}

void Agreement::BeginTurnOwner()
{
	AccessData()->BeginTurnOwner(turn_Get()->GetRound());
}

void Agreement::BeginTurnRecipient()
{
	AccessData()->BeginTurnRecipient(turn_Get()->GetRound());
}
