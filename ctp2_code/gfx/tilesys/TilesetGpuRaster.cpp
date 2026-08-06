// P14 — GPU terrain rasterisation. See the header for the design; this file is
// the decode. The one rule everything here follows: reproduce what
// DrawTransitionTile's inner loop would have written, pixel for pixel, using
// the same pixelutils_16to8888 conversion — the parity oracle holds the two
// paths bit-equal, so any "improvement" on the legacy decode belongs behind a
// different flag, not here.

#include "os/include/ctp2_config.h"
#include "ctp/c3.h"

#include "gfx/tilesys/TilesetGpuRaster.h"

#include "gfx/gfx_utils/pixelutils.h"
#include "gfx/tilesys/tileset.h"
#include "gfx/tilesys/BaseTile.h"
#include "gs/world/TileInfo.h"        // k_NUM_TRANSITIONS

namespace
{
// The diamond walk DrawTransitionTile does, shared by every decode here.
// k_TILE_PIXEL_HEADROOM rows of headroom sit ABOVE the diamond in the slot;
// the stream covers only the k_TILE_PIXEL_HEIGHT diamond rows.
int StartPixel(int y)
{
	return (y < k_TILE_PIXEL_HEADROOM)
	       ? 2 * ((k_TILE_PIXEL_HEADROOM - 1) - y)
	       : 2 * (y - k_TILE_PIXEL_HEADROOM);
}

// The atlas the entries pack into. 2048x2048 ARGB8888 = 16MB. The additive
// space (bases + splats) measured a few hundred entries on the shipped
// tileset; if a modded tileset ever exhausts this, entries report !ok and
// those cells stay on the CPU path — degraded performance, correct pixels.
int const k_TS_ATLAS_W = 2048;
int const k_TS_ATLAS_H = 2048;

uint64_t StripKey(int layoutId, int edge, uint16_t from, uint16_t to)
{
	return ((uint64_t) (uint16_t) layoutId << 48)
	     | ((uint64_t) (uint16_t) edge << 32)
	     | ((uint64_t) from << 16) | (uint64_t) to;
}

uint64_t DefaultKey(int layoutId, int edge, uint16_t from)
{
	return ((uint64_t) (uint16_t) layoutId << 48)
	     | ((uint64_t) (uint16_t) edge << 32)
	     | ((uint64_t) from << 16) | 0xFFFFULL;
}
}

void TilesetGpuRaster::Reset()
{
	m_layouts.clear();
	m_layoutByHash.clear();
	m_layoutOfTile.clear();
	m_base.clear();
	m_strips.clear();
	m_defaults.clear();
	m_shelfX = m_shelfY = m_shelfH = 0;
	m_atlasBroken = false;
	m_generatedFrom = nullptr;
}

bool TilesetGpuRaster::Pack(int w, int h, int &ax, int &ay)
{
	// Plain shelf packer with a 1px gutter. Entries are never freed — the
	// space is additive and bounded — so there is nothing cleverer to do.
	if (m_atlasBroken) return false;
	if (!aui_SDL::EnsureTilesetAtlas(k_TS_ATLAS_W, k_TS_ATLAS_H))
	{
		m_atlasBroken = true;
		return false;
	}
	int const gw = w + 1, gh = h + 1;
	if (m_shelfX + gw > k_TS_ATLAS_W)
	{
		m_shelfY += m_shelfH;
		m_shelfX = 0;
		m_shelfH = 0;
	}
	if (m_shelfY + gh > k_TS_ATLAS_H || gw > k_TS_ATLAS_W)
		return false;   // full — not broken; later smaller entries may still fit
	ax = m_shelfX;
	ay = m_shelfY;
	m_shelfX += gw;
	if (gh > m_shelfH) m_shelfH = gh;
	return true;
}

int TilesetGpuRaster::LayoutOf(TileSet *ts, uint16_t tileNum)
{
	auto const found = m_layoutOfTile.find(tileNum);
	if (found != m_layoutOfTile.end()) return found->second;

	BaseTile *bt = ts->GetBaseTile(tileNum);
	Pixel16 const *p = bt ? bt->GetTileData() : nullptr;
	if (!p)
	{
		m_layoutOfTile[tileNum] = -1;
		return -1;
	}

	Layout layout;
	uint64_t h = 1469598103934665603ULL;
	bool any = false;
	for (int y = 0; y < k_TILE_PIXEL_HEIGHT; ++y)
	{
		int const sx = StartPixel(y);
		int const ex = k_TILE_PIXEL_WIDTH - sx;
		for (int x = sx; x < ex; ++x)
		{
			Pixel16 const v = *p++;
			if (v < k_NUM_TRANSITIONS)
			{
				any = true;
				layout.pos[v].push_back({ (int16_t) x, (int16_t) y });
				uint64_t const trip = ((uint64_t) v << 32)
				                    | ((uint64_t) (uint16_t) y << 16)
				                    | (uint64_t) (uint16_t) x;
				h ^= trip;
				h *= 1099511628211ULL;
			}
		}
	}
	if (!any)
	{
		m_layoutOfTile[tileNum] = -1;
		return -1;
	}
	auto const known = m_layoutByHash.find(h);
	int id;
	if (known != m_layoutByHash.end())
		id = known->second;
	else
	{
		id = (int) m_layouts.size();
		m_layouts.push_back(std::move(layout));
		m_layoutByHash[h] = id;
	}
	m_layoutOfTile[tileNum] = id;
	return id;
}

TilesetGpuRaster::Entry const &TilesetGpuRaster::BaseEntry(TileSet *ts, uint16_t tileNum)
{
	auto const found = m_base.find(tileNum);
	if (found != m_base.end()) return found->second;

	Entry &e = m_base[tileNum];
	BaseTile *bt = ts->GetBaseTile(tileNum);
	Pixel16 const *p = bt ? bt->GetTileData() : nullptr;
	if (!p) return e;

	// The full diamond, marker holes transparent. ARGB 0 outside the diamond
	// and in the holes; everything else exactly what pixelutils_StorePixel
	// would have written on the 32bpp scratch.
	std::vector<uint32_t> pixels((size_t) k_TILE_PIXEL_WIDTH * k_TILE_PIXEL_HEIGHT, 0u);
	for (int y = 0; y < k_TILE_PIXEL_HEIGHT; ++y)
	{
		int const sx = StartPixel(y);
		int const ex = k_TILE_PIXEL_WIDTH - sx;
		for (int x = sx; x < ex; ++x)
		{
			Pixel16 const v = *p++;
			if (v >= k_NUM_TRANSITIONS)
				pixels[(size_t) y * k_TILE_PIXEL_WIDTH + x] = pixelutils_16to8888(v);
		}
	}
	int ax = 0, ay = 0;
	if (!Pack(k_TILE_PIXEL_WIDTH, k_TILE_PIXEL_HEIGHT, ax, ay)) return e;
	aui_SDL::UploadTilesetAtlasRect(ax, ay, k_TILE_PIXEL_WIDTH, k_TILE_PIXEL_HEIGHT,
	                                pixels.data(), k_TILE_PIXEL_WIDTH * 4);
	e.ok = true;
	e.ax = ax; e.ay = ay;
	e.w = k_TILE_PIXEL_WIDTH; e.h = k_TILE_PIXEL_HEIGHT;
	e.ox = 0; e.oy = k_TILE_PIXEL_HEADROOM;
	return e;
}

bool TilesetGpuRaster::UploadSplat(Entry &e,
                                   std::vector<std::pair<int16_t, int16_t>> const &pos,
                                   std::vector<uint32_t> const &vals)
{
	// Tight bounding box: strips are edge runs, a fraction of the diamond.
	int minX = 1 << 30, minY = 1 << 30, maxX = -1, maxY = -1;
	for (auto const &xy : pos)
	{
		if (xy.first < minX) minX = xy.first;
		if (xy.first > maxX) maxX = xy.first;
		if (xy.second < minY) minY = xy.second;
		if (xy.second > maxY) maxY = xy.second;
	}
	if (maxX < minX) return false;
	int const w = maxX - minX + 1;
	int const h = maxY - minY + 1;
	std::vector<uint32_t> pixels((size_t) w * h, 0u);
	for (size_t n = 0; n < pos.size(); ++n)
		pixels[(size_t) (pos[n].second - minY) * w + (pos[n].first - minX)] = vals[n];

	int ax = 0, ay = 0;
	if (!Pack(w, h, ax, ay)) return false;
	aui_SDL::UploadTilesetAtlasRect(ax, ay, w, h, pixels.data(), w * 4);
	e.ok = true;
	e.ax = ax; e.ay = ay;
	e.w = w; e.h = h;
	e.ox = minX; e.oy = k_TILE_PIXEL_HEADROOM + minY;
	return true;
}

TilesetGpuRaster::Entry const &TilesetGpuRaster::StripEntry(
	TileSet *ts, int layoutId, int edge, uint16_t from, uint16_t to)
{
	uint64_t const key = StripKey(layoutId, edge, from, to);
	auto const found = m_strips.find(key);
	if (found != m_strips.end()) return found->second;

	Entry &e = m_strips[key];
	Pixel16 const *stream = ts->GetTransitionData(from, to, (uint16) edge);
	if (!stream) return e;   // caller falls to the default splat
	auto const &pos = m_layouts[layoutId].pos[edge];
	if (pos.empty()) return e;

	// The strip is consumed in stream order as markers of this edge appear —
	// so entry n of the stream lands at position n of the layout's list.
	std::vector<uint32_t> vals(pos.size());
	for (size_t n = 0; n < pos.size(); ++n)
		vals[n] = pixelutils_16to8888(stream[n]);
	UploadSplat(e, pos, vals);
	return e;
}

TilesetGpuRaster::Entry const &TilesetGpuRaster::DefaultEntry(
	TileSet *ts, int layoutId, int edge, uint16_t from)
{
	uint64_t const key = DefaultKey(layoutId, edge, from);
	auto const found = m_defaults.find(key);
	if (found != m_defaults.end()) return found->second;

	Entry &e = m_defaults[key];
	// The default buffer is tile from*100+99, and the CPU loop advances it per
	// DIAMOND PIXEL regardless of markers — positional, not a stream. So the
	// value for a marker at (x,y) is the default tile's own pixel at (x,y).
	BaseTile *bt = ts->GetBaseTile((uint16) (from * 100 + 99));
	Pixel16 const *p = bt ? bt->GetTileData() : nullptr;
	if (!p) return e;
	auto const &pos = m_layouts[layoutId].pos[edge];
	if (pos.empty()) return e;

	// Decode the default tile positionally (its values are DATA here — the CPU
	// path stores them raw, marker range or not), then pick the positions.
	std::vector<uint32_t> full((size_t) k_TILE_PIXEL_WIDTH * k_TILE_PIXEL_HEIGHT, 0u);
	for (int y = 0; y < k_TILE_PIXEL_HEIGHT; ++y)
	{
		int const sx = StartPixel(y);
		int const ex = k_TILE_PIXEL_WIDTH - sx;
		for (int x = sx; x < ex; ++x)
			full[(size_t) y * k_TILE_PIXEL_WIDTH + x] = pixelutils_16to8888(*p++);
	}
	std::vector<uint32_t> vals(pos.size());
	for (size_t n = 0; n < pos.size(); ++n)
		vals[n] = full[(size_t) pos[n].second * k_TILE_PIXEL_WIDTH + pos[n].first];
	UploadSplat(e, pos, vals);
	return e;
}

bool TilesetGpuRaster::ComposeCell(TileSet *ts, uint16_t tileNum, uint16_t fromIndex,
                                   uint8_t const transitions[4],
                                   int destX, int destY,
                                   std::vector<aui_SDL::GpuQuad> &out)
{
	if (!ts) return false;
	if (ts != m_generatedFrom)
	{
		// Tileset (re)loaded: every cached entry described the old pixel data.
		Reset();
		m_generatedFrom = ts;
	}

	Entry const &base = BaseEntry(ts, tileNum);
	if (!base.ok) return false;

	int const layoutId = LayoutOf(ts, tileNum);
	SDL_Texture *tex = aui_SDL::TilesetAtlasTexture();
	if (!tex) return false;

	// Collect first, emit after: a cell must be all-or-nothing, or a missing
	// strip would leave marker holes showing whatever was under the tile.
	aui_SDL::GpuQuad quads[5];
	int count = 0;
	quads[count++] = { base.ax, base.ay, base.w, base.h,
	                   destX + base.ox, destY + base.oy, base.w, base.h, tex };

	if (layoutId >= 0)
	{
		for (int edge = 0; edge < k_NUM_TRANSITIONS; ++edge)
		{
			if (m_layouts[layoutId].pos[edge].empty()) continue;
			Entry const &strip = StripEntry(ts, layoutId, edge,
			                                fromIndex, transitions[edge]);
			Entry const *use = &strip;
			if (!strip.ok)
			{
				// No strip for this (from,to,edge): the CPU loop takes the
				// positional default buffer. No default buffer either would
				// mean the DEFAULT_PIXEL constant path — rare, and not worth
				// a third entry kind; the cell just stays on the CPU.
				Entry const &def = DefaultEntry(ts, layoutId, edge, fromIndex);
				if (!def.ok) return false;
				use = &def;
			}
			quads[count++] = { use->ax, use->ay, use->w, use->h,
			                   destX + use->ox, destY + use->oy,
			                   use->w, use->h, tex };
		}
	}

	for (int i = 0; i < count; ++i)
		out.push_back(quads[i]);
	return true;
}
