//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Multiplayer endgame packet handling. This file looks like
//                unused or not updated accordingly.
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
// - Removed old endgame database include file. (Aug 20th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "net/general/net_endgame.h"
#include "net/general/network.h"
#include "gs/gameobj/Wormhole.h"
#include "gs/utility/TurnCnt.h"

extern TurnCount *g_turn;
#include "gs/gameobj/EndGame.h" // Not part of the project
#include "net/io/net_util.h"
#include "gs/gameobj/Player.h"

#include "ctp/ctp2_utils/pointerlist.h"

extern Player **g_player;

//----------------------------------------------------------------------------
//
// Name       : NetEndGame::NetEndGame
//
// Description: Constructor
//
// Parameters : sint32 owner: The owner of the according endgame.
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
NetEndGame::NetEndGame(sint32 owner)
{
	m_owner = (uint8)owner;
}

//----------------------------------------------------------------------------
//
// Name       : NetEndGame::Packetize
//
// Description: Generate an application data packet to transmit.
//
// Parameters : buf         : buffer to store the message
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
void NetEndGame::Packetize(uint8 *buf, uint16 &size)
{
	size = 0;
	PUSHID(k_PACKET_ENDGAME_ID);
	PUSHBYTE(m_owner);
}

//----------------------------------------------------------------------------
//
// Name       : NetEndGame::Unpacketize
//
// Description: Retrieve the data from a received application data packet.
//
// Parameters : id          : Sender identification?
//              buf         : Buffer with received message
//              size        : Length of received message (in bytes)
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
void NetEndGame::Unpacketize(uint16 id, uint8 *buf, uint16 size)
{
	uint16 pos = 0;
	uint16 packid;
	PULLID(packid);
	Assert(packid == k_PACKET_ENDGAME_ID);

	PULLBYTE(m_owner);
	if(!g_player[m_owner])
		return;
















}

//----------------------------------------------------------------------------
//
// Name       : NetWormhole::Packetize
//
// Description: Generate an application data packet to transmit.
//
// Parameters : buf         : buffer to store the message
//
// Globals    : -
//
// Returns    : size        : number of bytes stored in buf
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
void NetWormhole::Packetize(uint8 *buf, uint16 &size)
{
	size = 0;
	PUSHID(k_PACKET_WORMHOLE_ID);

	Wormhole *wh = wormhole_Get();
	if(!wh) {
		PUSHBYTE(0);
		return;
	} else {
		PUSHBYTE(1);
	}

	PUSHLONG(wh->m_discoverer);
	sint32 packpos = g_network.PackedPos(wh->m_pos);
	PUSHLONG(packpos);

	PUSHBYTE(wh->m_curDir);
	PUSHLONG(wh->m_horizontalMoves);
	PUSHLONG(wh->m_topY);
	PUSHLONG(wh->m_bottomY);
	PUSHLONG(wh->m_discoveredAt);

	sint32 numEntries = wh->m_entries->GetCount();
	PUSHLONG(numEntries);
	PointerList<EntryRecord>::Walker walk(wh->m_entries);
	while(walk.IsValid()) {
		PUSHLONG(walk.GetObj()->m_unit.m_id);
		PUSHLONG(walk.GetObj()->m_round);
		walk.Next();
	}
}

//----------------------------------------------------------------------------
//
// Name       : NetWormhole::Unpacketize
//
// Description: Retrieve the data from a received application data packet.
//
// Parameters : id          : Sender identification?
//              buf         : Buffer with received message
//              size        : Length of received message (in bytes)
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
void NetWormhole::Unpacketize(uint16 id, uint8 *buf, uint16 size)
{
	uint16 pos = 0;
	uint16 packid;
	PULLID(packid);
	Assert(packid == k_PACKET_WORMHOLE_ID);

	uint8 haveWormhole;
	PULLBYTE(haveWormhole);
	if(!haveWormhole) {
		if(Wormhole *wh = wormhole_Get()) {
			delete wh;
			wormhole_Set(NULL);
		}
		return;
	}

	sint32 discoverer;
	MapPoint wpos;

	PULLLONG(discoverer);
	sint32 packpos;
	PULLLONG(packpos);
	g_network.UnpackedPos(packpos, wpos);

	Wormhole *wh = wormhole_Get();
	if(!wh) {
		wh = new Wormhole(discoverer, wpos, g_turn->GetRound());
		wormhole_Set(wh);
	}

	wh->m_pos = wpos;
	wh->m_discoverer = discoverer;

	PULLBYTETYPE(wh->m_curDir, WORLD_DIRECTION);
	PULLLONG(wh->m_horizontalMoves);
	PULLLONG(wh->m_topY);
	PULLLONG(wh->m_bottomY);
	PULLLONG(wh->m_discoveredAt);

	sint32 numEntries;
	PULLLONG(numEntries);
	Unit unit;
	sint32 round;
	sint32 i;
	for(i = 0; i < numEntries; i++) {
		PULLLONG(unit.m_id);
		PULLLONG(round);
		wh->m_entries->AddTail(new EntryRecord(unit, round));
	}
}
