//----------------------------------------------------------------------------
// P11 Stage 3 G0 — unit tests for the terrain tile-cache signature + LRU
// atlas-slot allocator. Pure logic, no graphics API, so these run in the fast
// suite. See GpuTileCache.h for the design.
//----------------------------------------------------------------------------
#include "doctest.h"

#include "gfx/tilesys/GpuTileCache.h"

#include <set>

TEST_CASE("terrain cell signature packs each field into its own byte range")
{
	// All-zero inputs => zero key.
	CHECK(TerrainCellSignature(0, 0, 0, 0, 0, 0) == 0);

	// Each field occupies a distinct bit range, so setting one field to 1
	// lands exactly one bit in the expected position and nothing else moves.
	CHECK(TerrainCellSignature(1, 0, 0, 0, 0, 0) == (uint64_t)1);           // tileNum: bits 0..15
	CHECK(TerrainCellSignature(0, 1, 0, 0, 0, 0) == ((uint64_t)1 << 16));   // tilesetIndex: bit 16
	CHECK(TerrainCellSignature(0, 0, 1, 0, 0, 0) == ((uint64_t)1 << 24));   // t0 (SW): bit 24
	CHECK(TerrainCellSignature(0, 0, 0, 1, 0, 0) == ((uint64_t)1 << 32));   // t1 (NW): bit 32
	CHECK(TerrainCellSignature(0, 0, 0, 0, 1, 0) == ((uint64_t)1 << 40));   // t2 (NE): bit 40
	CHECK(TerrainCellSignature(0, 0, 0, 0, 0, 1) == ((uint64_t)1 << 48));   // t3 (SE): bit 48

	// Max per-field values do not overflow into a neighbouring field.
	CHECK(TerrainCellSignature(0xFFFF, 0, 0, 0, 0, 0) == (uint64_t)0xFFFF);
	CHECK(TerrainCellSignature(0, 0xFF, 0, 0, 0, 0) == ((uint64_t)0xFF << 16));
}

TEST_CASE("terrain cell signature is a bijection over the used fields")
{
	// Distinct inputs must yield distinct keys (no collisions); identical
	// inputs must yield identical keys (stable). Sweep a small grid.
	std::set<uint64_t> keys;
	int count = 0;
	for (uint16_t tile = 0; tile < 3; ++tile)
		for (uint8_t tset = 0; tset < 3; ++tset)
			for (uint8_t a = 0; a < 3; ++a)
				for (uint8_t b = 0; b < 3; ++b)
				{
					uint64_t k1 = TerrainCellSignature(tile, tset, a, b, a, b);
					uint64_t k2 = TerrainCellSignature(tile, tset, a, b, a, b);
					CHECK(k1 == k2);          // stable
					keys.insert(k1);
					++count;
				}
	CHECK((int)keys.size() == count); // all distinct => bijective on this grid
}

TEST_CASE("cache: first sight is a MISS, repeat is a HIT with a stable slot")
{
	GpuTileCache cache(4, 4, 94, 72); // 16 slots, tile 94x72

	uint64_t const sig = TerrainCellSignature(10, 2, 1, 1, 1, 1);

	GpuTileSlot first;
	CHECK(cache.Get(sig, first) == GpuTileCache::MISS);
	CHECK(cache.Size() == 1);

	GpuTileSlot again;
	CHECK(cache.Get(sig, again) == GpuTileCache::HIT);
	CHECK(cache.Size() == 1);
	CHECK(again.index == first.index);   // same physical slot
	CHECK(again.atlasX == first.atlasX);
	CHECK(again.atlasY == first.atlasY);

	CHECK(cache.Hits() == 1);
	CHECK(cache.Misses() == 1);
	CHECK(cache.Evictions() == 0);
}

TEST_CASE("cache: slots tile the atlas row-major and stay in bounds")
{
	GpuTileCache cache(3, 2, 94, 72); // 6 slots, 3 cols x 2 rows
	CHECK(cache.AtlasW() == 3 * 94);
	CHECK(cache.AtlasH() == 2 * 72);

	std::set<int> usedIndices;
	for (int i = 0; i < cache.Capacity(); ++i)
	{
		GpuTileSlot slot;
		CHECK(cache.Get(TerrainCellSignature((uint16_t)(100 + i), 0, 0, 0, 0, 0), slot)
		      == GpuTileCache::MISS);
		usedIndices.insert(slot.index);

		// Row-major geometry.
		CHECK(slot.atlasX == (slot.index % cache.Cols()) * cache.TileW());
		CHECK(slot.atlasY == (slot.index / cache.Cols()) * cache.TileH());
		// Fully inside the atlas.
		CHECK(slot.atlasX >= 0);
		CHECK(slot.atlasY >= 0);
		CHECK(slot.atlasX + cache.TileW() <= cache.AtlasW());
		CHECK(slot.atlasY + cache.TileH() <= cache.AtlasH());
	}
	CHECK((int)usedIndices.size() == cache.Capacity()); // every slot distinct
}

TEST_CASE("cache: LRU eviction reuses the least-recently-used slot")
{
	GpuTileCache cache(2, 1, 94, 72); // capacity 2 — easy to overflow

	GpuTileSlot sa, sb, sc;
	uint64_t const A = TerrainCellSignature(1, 0, 0, 0, 0, 0);
	uint64_t const B = TerrainCellSignature(2, 0, 0, 0, 0, 0);
	uint64_t const C = TerrainCellSignature(3, 0, 0, 0, 0, 0);

	CHECK(cache.Get(A, sa) == GpuTileCache::MISS); // [A]
	CHECK(cache.Get(B, sb) == GpuTileCache::MISS); // [B, A]  (A now LRU)
	CHECK(cache.Size() == 2);

	// Atlas full; inserting C must evict A (the LRU) and reuse its slot.
	CHECK(cache.Get(C, sc) == GpuTileCache::MISS);
	CHECK(cache.Evictions() == 1);
	CHECK(cache.Size() == 2);
	CHECK(sc.index == sa.index); // C took A's freed slot

	// A is gone => a fresh MISS; B survived => HIT.
	GpuTileSlot tmp;
	CHECK(cache.Get(B, tmp) == GpuTileCache::HIT);
	CHECK(cache.Get(A, tmp) == GpuTileCache::MISS);
}

TEST_CASE("cache: touching an entry protects it from eviction")
{
	GpuTileCache cache(2, 1, 94, 72); // capacity 2

	uint64_t const A = TerrainCellSignature(1, 0, 0, 0, 0, 0);
	uint64_t const B = TerrainCellSignature(2, 0, 0, 0, 0, 0);
	uint64_t const C = TerrainCellSignature(3, 0, 0, 0, 0, 0);

	GpuTileSlot s;
	cache.Get(A, s);           // [A]
	cache.Get(B, s);           // [B, A]
	cache.Get(A, s);           // HIT touches A -> [A, B]  (B now LRU)

	// Inserting C must now evict B, not A.
	cache.Get(C, s);           // [C, A]
	CHECK(cache.Get(A, s) == GpuTileCache::HIT);   // A protected
	CHECK(cache.Get(B, s) == GpuTileCache::MISS);  // B was evicted
}

TEST_CASE("cache: Clear drops entries but keeps lifetime telemetry")
{
	GpuTileCache cache(2, 2, 94, 72);

	GpuTileSlot s;
	cache.Get(TerrainCellSignature(1, 0, 0, 0, 0, 0), s);
	cache.Get(TerrainCellSignature(2, 0, 0, 0, 0, 0), s);
	CHECK(cache.Size() == 2);
	uint64_t const missesBefore = cache.Misses();

	cache.Clear();
	CHECK(cache.Size() == 0);
	CHECK(cache.Misses() == missesBefore); // counters are cumulative

	// After Clear the atlas is fresh: the same sig is a MISS again and lands in
	// the first slot.
	CHECK(cache.Get(TerrainCellSignature(1, 0, 0, 0, 0, 0), s) == GpuTileCache::MISS);
	CHECK(s.index == 0);
}
