#include "ctp/c3.h"
#include <memory>
#include "gs/gameobj/TaxRate.h"
#include "gs/slic/SlicEngine.h"
#include "gs/database/DB.h"
#include "GovernmentRecord.h"
#include "gs/gameobj/player.h"
#include "gs/utility/safety.h"          // safe_player
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

	if(Player * p = safe_player(owner))
	{
		double const maxRate = g_theGovernmentDB->Get(p->m_government_type)->GetMaxScienceRate();
		if(s > maxRate)
			s = maxRate;
	}

	double oldscience = m_science;
	m_science = s;
	if(oldscience != m_science) {
		slicengine_Get()->RunScienceRateTriggers(owner);
	}

	if(network_Get().IsClient() && network_Get().IsLocalPlayer(owner)) {
		network_Get().SendAction(std::make_unique<NetAction>(NET_ACTION_TAX_RATES,
										   (sint32)(s * 100000.),
										   0,
										   0).release());
	} else if(network_Get().IsHost()) {
		network_Get().Block(owner);
		network_Get().Enqueue(std::make_unique<NetInfo>(NET_INFO_CODE_TAX_RATE,
									  owner,
									  (sint32)(s * 100000.),
									  0, 0).release());
		network_Get().Unblock(owner);
	}
}

void TaxRate::InitTaxRates(double s, sint32 owner)
{
	m_science = s;
}
