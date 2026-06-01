#include "ctp/c3.h"
#include "net/general/net_keys.h"
#include "net/io/net_util.h"

#include "gs/gameobj/UnitPool.h"
#include "gs/gameobj/TradePool.h"
#include "gs/gameobj/TradeOfferPool.h"
#include "gs/gameobj/TerrImprovePool.h"
#include "gs/gameobj/installationpool.h"
#include "gs/gameobj/CivilisationPool.h"
#include "gs/gameobj/DiplomaticRequestPool.h"
#include "gs/gameobj/MessagePool.h"
#include "gs/outcom/AICause.h"
#include "gs/gameobj/ArmyPool.h"
#include "gs/gameobj/AgreementPool.h"

void NetKeys::Packetize(uint8 *buf, uint16 &size)
{
	size = 0;
	PUSHID(k_PACKET_KEYS_ID);
	PUSHLONG(unitpool_Get()->HackGetKey());
	PUSHLONG(tradepool_Get()->HackGetKey());
	PUSHLONG(tradeofferpool_Get()->HackGetKey());
	PUSHLONG(terrimprovepool_Get()->HackGetKey());
	PUSHLONG(installationpool_Get()->HackGetKey());
	PUSHLONG(civilisationpool_Get()->HackGetKey());
	PUSHLONG(diplomaticrequestpool_Get()->HackGetKey());
	PUSHLONG(messagepool_Get()->HackGetKey());
	PUSHLONG(armypool_Get()->HackGetKey());
	PUSHLONG(agreementpool_Get()->HackGetKey());
}

void NetKeys::Unpacketize(uint16 id, uint8 *buf, uint16 size)
{
	sint32 pos = 0;
	uint16 packid;
	PULLID(packid);
	Assert(packid == k_PACKET_KEYS_ID);

	uint32 key;
	PULLLONG(key); unitpool_Get()->HackSetKey(key);
	PULLLONG(key); tradepool_Get()->HackSetKey(key);
	PULLLONG(key); tradeofferpool_Get()->HackSetKey(key);
	PULLLONG(key); terrimprovepool_Get()->HackSetKey(key);
	PULLLONG(key); installationpool_Get()->HackSetKey(key);
	PULLLONG(key); civilisationpool_Get()->HackSetKey(key);
	PULLLONG(key); diplomaticrequestpool_Get()->HackSetKey(key);
	PULLLONG(key); messagepool_Get()->HackSetKey(key);
	PULLLONG(key); armypool_Get()->HackSetKey(key);
	PULLLONG(key); agreementpool_Get()->HackSetKey(key);

	Assert(pos == size);
}
