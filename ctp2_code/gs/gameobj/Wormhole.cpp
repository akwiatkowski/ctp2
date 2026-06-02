//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Wormhole handling.
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
// - The good sprite index is now taken from the resource database instead of
//   the good's sprite state database. However this file isn't used as the
//   wormhole has been removed from the game, but maybe there is someone
//   who whishes to put it back into the game. (Aug 29th 2005 Martin G�hmann)
// - Replaced old const database by new one. (5-Aug-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/gameobj/Wormhole.h"
#include "gs/gameobj/Player.h"
#include "robot/aibackdoor/civarchive.h"
#include "gs/gameobj/XY_Coordinates.h"
#include "gs/world/World.h"
#include "ConstRecord.h"
#include "ctp/ctp2_utils/pointerlist.h"
#include "gs/utility/RandGen.h"
#include "gs/newdb/UnitRec.h"
#include "gs/core/goodactor_factory.h"
#include "gs/core/render_observer.h"
#include "gs/gameobj/UnitPool.h"
#include "gs/outcom/AICause.h"
#include "net/general/network.h"
#include "net/general/net_endgame.h"
#include "gs/utility/directions.h"
#include "gs/fileio/gamefile.h"
#include "ResourceRecord.h"

#define k_WORMHOLE_GOOD_ID_STR		"WORMHOLE"

// Wormhole's definition + lifecycle moved to gs/utility/gameinit.cpp
// (matches the rest of the gs/ singletons). Accessor pair in Wormhole.h.

Wormhole::Wormhole(sint32 discoverer, sint32 currentRound)
{
	m_discoverer = discoverer;

	sint16 centerY = static_cast<sint16>(world_Get()->GetYHeight() / 2);
	sint32 orbitHeight = (world_Get()->GetYHeight() * g_theConstDB->Get(0)->GetWormholeOrbitHeightPercentage()) / 100;
	m_topY = centerY - orbitHeight / 2;
	m_bottomY = centerY + orbitHeight / 2;
	m_pos.y = centerY;
	m_pos.x = sint16(civrand().Next(world_Get()->GetXWidth()));
	m_curDir = NORTHEAST;

	m_entries = new PointerList<EntryRecord>;
	m_discoveredAt = currentRound;

	sint32 id = g_theResourceDB->Get(g_theResourceDB->FindRecordNameIndex(k_WORMHOLE_GOOD_ID_STR))->GetSpriteID();
	m_actor = goodactor_factory_create(id, m_pos);
}

Wormhole::Wormhole(sint32 discoverer, MapPoint &startPos, sint32 currentRound)
{
	m_discoverer = discoverer;
	sint16 centerY =  static_cast<sint16>(world_Get()->GetYHeight() / 2);
	sint32 orbitHeight = (world_Get()->GetYHeight() * g_theConstDB->Get(0)->GetWormholeOrbitHeightPercentage()) / 100;
	m_topY = centerY - orbitHeight / 2;
	m_bottomY = centerY + orbitHeight / 2;
	m_pos.y = startPos.y;
	m_pos.x = startPos.x;
	m_curDir = NORTHEAST;

	m_entries = new PointerList<EntryRecord>;
	m_discoveredAt = currentRound;

	sint32 id = g_theResourceDB->Get(g_theResourceDB->FindRecordNameIndex(k_WORMHOLE_GOOD_ID_STR))->GetSpriteID();
	m_actor = goodactor_factory_create(id, m_pos);
}

Wormhole::Wormhole(CivArchive &archive)
{
	m_entries = new PointerList<EntryRecord>;
	Serialize(archive);

	sint32 id = g_theResourceDB->Get(g_theResourceDB->FindRecordNameIndex(k_WORMHOLE_GOOD_ID_STR))->GetSpriteID();
	m_actor = goodactor_factory_create(id, m_pos);
}

Wormhole::~Wormhole()
{
	if(m_entries) {
		m_entries->DeleteAll();
		delete m_entries;
	}

	goodactor_factory_destroy(m_actor);
}

void Wormhole::Serialize(CivArchive &archive)
{
	sint32 i, c;
	if(archive.IsStoring()) {
	} else {
		if(save_file_version_Get() < 55) {
			archive.LoadChunk((uint8*)&m_pos, (uint8*)((uint8*)&m_discoveredAt + sizeof(m_discoveredAt)));
			archive >> c;
			for(i = 0; i < c; i++) {
				EntryRecord *rec = new EntryRecord();
				rec->m_unit.Serialize(archive);
				archive >> rec->m_round;
				m_entries->AddTail(rec);
			}
		}
	}
}

BOOL Wormhole::CheckEnter(const Unit &unit, sint32 currentRound)
{
	MapPoint upos;
	if(!unit.GetDBRec()->GetWormholeProbe())
		return FALSE;

	unit.GetPos(upos);
	if(m_pos != upos) {
		return FALSE;
	}
	m_entries->AddTail(new EntryRecord(unit, currentRound));
	return TRUE;
}

void Wormhole::BeginTurn(sint32 player)
{
	Move();

	PointerList<EntryRecord>::Walker walk(m_entries);
	while(walk.IsValid()) {
		if(!unitpool_Get()->IsValid(walk.GetObj()->m_unit)) {
			delete walk.Remove();
			continue;
		}
		if(walk.GetObj()->m_unit.GetOwner() != player) {
			walk.Next();
			continue;
		}
		if(unitpool_Get()->IsValid(walk.GetObj()->m_unit)) {
			walk.GetObj()->m_unit.Kill(CAUSE_REMOVE_ARMY_PROBE_RECOVERED, -1);
		}

		delete walk.Remove();

#if 0

		if(walk.GetObj()->m_round + g_theConstDB->WormholeReturnTime() <= 0 /* g_turn->GetRound() — dead branch, see #if 0 */) {
			EntryRecord *erec = walk.GetObj();
			walk.Remove();
			ere1c->m_unit.ExitWormhole(m_pos);

			render_observer::AddShow(erec->m_unit);

			delete erec;

		} else {
			walk.Next();
		}
#endif
	}
	if(g_network.IsHost()) {
		g_network.QueuePacketToAll(new NetWormhole());
	}
}

#define k_HORIZONTAL_MOVES 5

void Wormhole::Move()
{
	sint32 i;
	sint32 speed = g_theConstDB->Get(0)->GetWormholeSpeed();
	for(i = 0; i < speed; i++) {
		if((m_curDir == EAST || m_curDir == NORTHEAST) &&
		   !world_Get()->IsXwrap()) {
			if(!m_pos.GetNeighborPosition(m_curDir, m_pos)) {
				m_pos.x = 0;
			}
		}
		m_pos.GetNeighborPosition(m_curDir, m_pos);
		switch(m_curDir) {
			case NORTHEAST:
				if(m_pos.y <= m_topY) {
					m_curDir = EAST;
					m_horizontalMoves = k_HORIZONTAL_MOVES;
				}
				break;
			case EAST:
				m_horizontalMoves--;
				if(m_horizontalMoves <= 0) {
					if(m_pos.y == m_topY) {
						m_curDir = SOUTHEAST;
					} else {
						m_curDir = NORTHEAST;
					}
				}
				break;
			case SOUTHEAST:
				if(m_pos.y >= m_bottomY) {
					m_curDir = EAST;
					m_horizontalMoves = k_HORIZONTAL_MOVES;
				}
				break;
		}
	}
	DPRINTF(k_DBG_INFO, ("Wormhole now at %d,%d\n", m_pos.x, m_pos.y));
}

BOOL Wormhole::IsVisible(sint32 player, sint32 currentRound) const
{
	if(m_discoveredAt + g_theConstDB->Get(0)->GetWormholeVisibleToAllTurns() <= currentRound) {
		return TRUE;
	} else if(player == m_discoverer) {
		return TRUE;
	}
	return FALSE;
}
