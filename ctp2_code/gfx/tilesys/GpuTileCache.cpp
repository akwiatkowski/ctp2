//----------------------------------------------------------------------------
// P11 Stage 3 G0 — GPU terrain tile-cache implementation. See GpuTileCache.h
// for the design rationale. This translation unit is pure C++ (no graphics
// API) so it links into both the game and the unit-test binary.
//----------------------------------------------------------------------------
#include "gfx/tilesys/GpuTileCache.h"

#include <cassert>

GpuTileCache::GpuTileCache(int cols, int rows, int tileW, int tileH)
	: m_cols(cols)
	, m_rows(rows)
	, m_tileW(tileW)
	, m_tileH(tileH)
	, m_capacity(cols * rows)
	, m_nextFreeSlot(0)
	, m_hits(0)
	, m_misses(0)
	, m_evictions(0)
{
	assert(cols > 0 && rows > 0 && tileW > 0 && tileH > 0);
	// Reserve so steady-state lookups don't rehash.
	m_index.reserve((size_t)m_capacity * 2);
}

GpuTileSlot GpuTileCache::SlotFromIndex(int slotIndex) const
{
	// Row-major slot layout in the atlas image.
	GpuTileSlot slot;
	slot.index  = slotIndex;
	slot.atlasX = (slotIndex % m_cols) * m_tileW;
	slot.atlasY = (slotIndex / m_cols) * m_tileH;
	return slot;
}

GpuTileCache::Result GpuTileCache::Get(uint64_t sig, GpuTileSlot & slot)
{
	auto found = m_index.find(sig);
	if (found != m_index.end())
	{
		// HIT: move this entry to the front (most-recently-used). splice keeps
		// the iterator stored in m_index valid, so no map update is needed.
		m_lru.splice(m_lru.begin(), m_lru, found->second);
		slot = SlotFromIndex(found->second->slotIndex);
		++m_hits;
		return HIT;
	}

	// MISS: pick the slot to fill.
	int slotIndex = -1;
	if (m_nextFreeSlot < m_capacity)
	{
		// Atlas still has a never-used slot.
		slotIndex = m_nextFreeSlot++;
	}
	else
	{
		// Atlas is full: evict the least-recently-used entry (list back) and
		// reuse its atlas slot for the newcomer.
		Entry const & victim = m_lru.back();
		slotIndex = victim.slotIndex;
		m_index.erase(victim.sig);
		m_lru.pop_back();
		++m_evictions;
	}

	m_lru.push_front(Entry{ sig, slotIndex });
	m_index.emplace(sig, m_lru.begin());
	slot = SlotFromIndex(slotIndex);
	++m_misses;
	return MISS;
}

void GpuTileCache::Clear()
{
	m_lru.clear();
	m_index.clear();
	m_nextFreeSlot = 0;
	// Keep cumulative hit/miss/eviction counters — they are lifetime telemetry.
}
