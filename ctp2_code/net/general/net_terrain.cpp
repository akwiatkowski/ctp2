//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Multiplayer terrain improvement packet handling.
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
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "net/general/net_terrain.h"

#include "gs/utility/gstypes.h"            // TERRAIN_TYPES
#include "net/general/network.h"
#include "net/io/net_util.h"
#include "gs/gameobj/TerrImprove.h"
#include "gs/gameobj/TerrImprovePool.h"
#include "gs/world/World.h"              // world_Get()
#include "gs/gameobj/Player.h"
#include "gfx/tilesys/tiledmap.h"           // tiledmap_Get()

//----------------------------------------------------------------------------
//
// Name       : NetTerrainImprovement::NetTerrainImprovement
//
// Description: Constructor
//
// Parameters : TerrainImprovementData *data: The terrain improvement data to
//                                            send/retrieve.
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
NetTerrainImprovement::NetTerrainImprovement(TerrainImprovementData * data)
:   m_data  (data)
{ ; }

//----------------------------------------------------------------------------
//
// Name       : NetTerrainImprovement::Packetize
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
void NetTerrainImprovement::Packetize(uint8 *buf, uint16 &size)
{
	buf[0] = k_PACKET_TERRAIN_ID >> 8;
	buf[1] = k_PACKET_TERRAIN_ID & 0xff;

	size = 2;
	PUSHLONG(m_data->m_id);
	PUSHBYTE((uint8)m_data->m_owner);
	PUSHBYTE((uint8)m_data->m_type);
	PUSHSHORT((uint16)m_data->m_point.x);
	PUSHSHORT((uint16)m_data->m_point.y);
	PUSHSHORT((uint16)m_data->m_turnsToComplete);
	PUSHBYTE((unsigned char) m_data->m_transformType);
	PUSHLONG(m_data->m_materialCost);
}

//----------------------------------------------------------------------------
//
// Name       : NetFeatTracker::Unpacketize
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
void NetTerrainImprovement::Unpacketize(uint16 id, uint8 *buf, uint16 size)
{
	Assert(MAKE_CIV3_ID(buf[0], buf[1]) == k_PACKET_TERRAIN_ID);
	sint32 pos = 2;
	MapPoint oldpoint;
	PLAYER_INDEX oldOwner = -1;

	TerrainImprovement imp;
	PULLLONGTYPE(imp, TerrainImprovement);

	network_Get().CheckReceivedObject((uint32)imp);

	if(!terrimprovepool_Get()->IsValid(imp)) {
		m_data = new TerrainImprovementData(imp);
	} else {
		m_data = terrimprovepool_Get()->AccessTerrainImprovement(imp);
		oldpoint = m_data->m_point;
		oldOwner = m_data->m_owner;
	}

	pos = 6;
	PULLBYTE(m_data->m_owner);
	PULLBYTETYPE(m_data->m_type, TERRAIN_IMPROVEMENT);
	PULLSHORT(m_data->m_point.x);
	PULLSHORT(m_data->m_point.y);
	PULLSHORT(m_data->m_turnsToComplete);
	PULLBYTETYPE(m_data->m_transformType,TERRAIN_TYPES);
	PULLLONG(m_data->m_materialCost);

	if(!terrimprovepool_Get()->IsValid(imp)) {
		terrimprovepool_Get()->HackSetKey(((uint32)imp & k_ID_KEY_MASK) + 1);
		terrimprovepool_Get()->Insert(m_data);
		world_Get()->InsertImprovement(imp, m_data->m_point);
		tiledmap_Get()->RedrawTile(&m_data->m_point);
		player_Get(m_data->m_owner)->AddImprovement(imp);
	} else {
		if(oldpoint != m_data->m_point) {

			world_Get()->RemoveImprovement(imp, oldpoint);
		world_Get()->InsertImprovement(imp, m_data->m_point);

			tiledmap_Get()->RedrawTile(&oldpoint);
			tiledmap_Get()->RedrawTile(&m_data->m_point);
		}

		if(oldOwner != m_data->m_owner) {

			player_Get(oldOwner)->RemoveImprovementReferences(imp);
		player_Get(m_data->m_owner)->AddImprovement(imp);
		}
	}
}
