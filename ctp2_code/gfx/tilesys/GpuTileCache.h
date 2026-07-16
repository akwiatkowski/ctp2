//----------------------------------------------------------------------------
// P11 Stage 3 G0 — GPU terrain tile-cache (pure allocator + LRU brain).
//
// Why this exists
// ---------------
// CtP2 composites every terrain cell on the CPU each frame (base tile + up to
// four pre-baked directional transition strips, marker-substituted together by
// TiledMap::DrawTransitionTile). That per-cell composite is deterministic: a
// cell's final pixels are *fully* determined by its base tile number, its
// terrain's tileset index, and the four diagonal neighbour transitions. So the
// same visual combination recurs across thousands of cells (map interiors are
// identical; only edges vary) — a modest set of DISTINCT combinations.
//
// This class is the "brain" of a dynamic GPU tile atlas: it maps a cell's
// appearance signature to a fixed slot in a tile atlas, evicting the
// least-recently-used slot when the atlas is full. It holds NO pixels and
// touches NO graphics API — that keeps it pure and unit-testable. The G1
// terrain-quad pass owns the SDL side: on a MISS it composites the cell once
// (reusing DrawTransitionTile into a scratch surface) and uploads those pixels
// into the returned slot's atlas rect; on a HIT it draws a textured quad from
// the already-cached rect. So the expensive CPU composite happens only the
// first time each distinct combination is seen, not every frame.
//----------------------------------------------------------------------------
#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __GPUTILECACHE_H__
#define __GPUTILECACHE_H__

#include <cstdint>
#include <list>
#include <unordered_map>

// Pack a terrain cell's appearance-determining inputs into a single 64-bit key.
// These are exactly the inputs DrawTransitionTile reads (besides the static
// tileset tables, which are constant for a session):
//   tileNum       - base tile index (TileInfo::GetTileNum(), uint16)
//   tilesetIndex  - the cell terrain's tileset index (0..TERRAIN_MAX-1)
//   t0..t3        - the four diagonal transition neighbours
//                   (TileInfo::GetTransition(0..3), each a neighbour tileset
//                    index, ordered SW, NW, NE, SE)
// Each field lives in its own byte range, so the packing is collision-free:
// distinct inputs => distinct keys, identical inputs => identical key. Fog is
// NOT part of the key — it is a separate GPU pass (Stage 2 C), so a cell's
// cached terrain image is fog-independent.
inline uint64_t TerrainCellSignature(uint16_t tileNum, uint8_t tilesetIndex,
                                     uint8_t t0, uint8_t t1,
                                     uint8_t t2, uint8_t t3)
{
	return  (uint64_t)tileNum
	     | ((uint64_t)tilesetIndex << 16)
	     | ((uint64_t)t0           << 24)
	     | ((uint64_t)t1           << 32)
	     | ((uint64_t)t2           << 40)
	     | ((uint64_t)t3           << 48);
}

// The atlas rect a signature is mapped to. (atlasX, atlasY) is the top-left
// pixel of this slot inside the atlas image; the slot is TileW() x TileH().
struct GpuTileSlot
{
	int index  = -1;   // slot ordinal (0..Capacity()-1); -1 == unset
	int atlasX = 0;    // pixel column of the slot in the atlas
	int atlasY = 0;    // pixel row of the slot in the atlas
};

class GpuTileCache
{
public:
	// A cols x rows grid of slots, each tileW x tileH pixels. Capacity is
	// cols*rows and must be >= 1. Slots are laid out row-major in the atlas.
	GpuTileCache(int cols, int rows, int tileW, int tileH);

	enum Result { HIT, MISS };

	// Resolve a signature to its atlas slot.
	//   HIT  - sig is already cached; slot is filled and marked
	//          most-recently-used. The caller draws from it directly.
	//   MISS - sig was not cached; a slot has been allocated for it (evicting
	//          the least-recently-used entry if the atlas was full) and filled.
	//          The caller MUST composite the cell and upload its pixels into
	//          slot's atlas rect before drawing.
	// Never fails (capacity is always >= 1).
	Result Get(uint64_t sig, GpuTileSlot & slot);

	// Drop every entry (e.g. tileset reload / zoom change that invalidates the
	// composited pixels). Atlas geometry is unchanged.
	void Clear();

	int Capacity() const { return m_capacity; }
	int Cols()     const { return m_cols; }
	int Rows()     const { return m_rows; }
	int TileW()    const { return m_tileW; }
	int TileH()    const { return m_tileH; }
	int AtlasW()   const { return m_cols * m_tileW; }
	int AtlasH()   const { return m_rows * m_tileH; }

	// Live diagnostics — used by the unit tests and by runtime telemetry.
	int      Size()      const { return (int)m_index.size(); }
	uint64_t Hits()      const { return m_hits; }
	uint64_t Misses()    const { return m_misses; }
	uint64_t Evictions() const { return m_evictions; }

private:
	// Recency list: front == most-recently-used, back == least-recently-used.
	// Each node carries the signature and the fixed slot ordinal it occupies.
	struct Entry { uint64_t sig; int slotIndex; };

	int const m_cols;
	int const m_rows;
	int const m_tileW;
	int const m_tileH;
	int const m_capacity;

	std::list<Entry>                                       m_lru;
	std::unordered_map<uint64_t, std::list<Entry>::iterator> m_index;
	int      m_nextFreeSlot; // next never-used slot ordinal (before any eviction)
	uint64_t m_hits;
	uint64_t m_misses;
	uint64_t m_evictions;

	GpuTileSlot SlotFromIndex(int slotIndex) const;
};

#endif // __GPUTILECACHE_H__
