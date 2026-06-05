#include "ctp/c3.h"
#include "gs/gameobj/TaxRate.h"
#include "gs/slic/SlicEngine.h"
#include "gs/database/DB.h"
#include "GovernmentRecord.h"
#include "gs/gameobj/Player.h"
#include "net/general/network.h"
#include "net/general/net_info.h"
#include "net/general/net_action.h"
uint32 TaxRate_TaxRate_GetVersion()
	{
	return (k_TAXRATE_VERSION_MAJOR<<16 | k_TAXRATE_VERSION_MINOR);
	}

void TaxRate::SetTaxRates(double s, sint32 owner)
{
	Assert(0.0 <= s);
    Assert(s <= 1.0);

	if(s > g_theGovernmentDB->Get(player_Get(owner)->m_government_type)->GetMaxScienceRate())
		s = g_theGovernmentDB->Get(player_Get(owner)->m_government_type)->GetMaxScienceRate();

	double oldscience = m_science;
	m_science = s;
	if(oldscience != m_science) {
		slicengine_Get()->RunScienceRateTriggers(owner);
	}

	if(network_Get().IsClient() && network_Get().IsLocalPlayer(owner)) {
		network_Get().SendAction(new NetAction(NET_ACTION_TAX_RATES,
										   (sint32)(s * 100000.),
										   0,
										   0));
	} else if(network_Get().IsHost()) {
		network_Get().Block(owner);
		network_Get().Enqueue(new NetInfo(NET_INFO_CODE_TAX_RATE,
									  owner,
									  (sint32)(s * 100000.),
									  0, 0));
		network_Get().Unblock(owner);
	}
}

void TaxRate::InitTaxRates(double s, sint32 owner)
{
	m_science = s;
}
