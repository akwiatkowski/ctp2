//----------------------------------------------------------------------------
// P14 — GPU terrain rasterisation from a decoded-tileset atlas.
//
// Why this exists
// ---------------
// Every terrain cell CtP2 has ever shown was rasterised by DrawTransitionTile:
// a CPU loop over the tile diamond where stream values 0..3 are inline MARKERS,
// each substituting the next pixel from one of four transition strips (or a
// positional default buffer). The P11-P13 work moved compositing and the camera
// to the GPU, but the pixels inside every atlas tile still came from that 1999
// loop, re-run for every distinct cell appearance.
//
// Measured structure of the actual tileset (debug_tileset_stats, 2026-08-06):
// 381 base tiles, 97 with markers, and only 13 DISTINCT marker layouts. So the
// decode splits additively instead of per-combination:
//
//   base layer   : one per base tile — the diamond with marker holes left
//                  transparent. 381 max.
//   strip splat  : one per (layout, edge, from, to) — the transition strip's
//                  stream laid into that layout's marker positions. Lazily
//                  generated; a few hundred in practice.
//   default splat: one per (layout, edge, from) — the positional default
//                  buffer (tile from*100+99) sampled at the layout's marker
//                  positions. The default pointer advances per DIAMOND PIXEL in
//                  the CPU loop, not per marker, which is why it is positional.
//
// A cell is then 1-5 GPU quads from one static atlas that is never evicted —
// the additive space cannot outgrow it the way per-combination caching can.
// Every pixel is converted with the same pixelutils_16to8888 the CPU scratch
// path uses and alpha is binary (colorkey), so the GPU composite is bit-equal
// to the CPU one by construction; the parity test holds it to that.
//
// This class owns the CPU-side brain: layouts, entries, shelf packing, decode.
// The texture lives in aui_SDL (EnsureTilesetAtlas / UploadTilesetAtlasRect),
// matching how the quad atlas is split.
//----------------------------------------------------------------------------
#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __TILESETGPURASTER_H__
#define __TILESETGPURASTER_H__

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "ui/aui_sdl/aui_sdl.h"

class TileSet;

class TilesetGpuRaster
{
public:
	// Emit the GPU quads that reproduce DrawTransitionTile(tileNum,
	// transitions) at cell-slot position (destX, destY) — destY is the SLOT
	// top; the diamond lands k_TILE_PIXEL_HEADROOM below it, exactly as the
	// CPU composite places it. Returns false (appending nothing) when this
	// cell cannot be GPU-rasterised — missing tileset data, atlas exhausted —
	// and the caller keeps the CPU path for that cell.
	bool ComposeCell(TileSet *ts, uint16_t tileNum, uint16_t fromIndex,
	                 uint8_t const transitions[4],
	                 int destX, int destY,
	                 std::vector<aui_SDL::GpuQuad> &out);

	// Drop everything (tileset reload). Entries are pure functions of the
	// tileset file, so nothing else ever invalidates them.
	void Reset();

	// Diagnostics for the probe commands.
	int Entries() const { return (int) (m_base.size() + m_strips.size() + m_defaults.size()); }
	int Layouts() const { return (int) m_layouts.size(); }
	bool AtlasBroken() const { return m_atlasBroken; }

private:
	struct Entry
	{
		bool ok = false;   // decode failed / atlas full — remembered so a bad
		                   // entry is not retried every frame
		int ax = 0, ay = 0;      // atlas rect
		int w = 0, h = 0;
		int ox = 0, oy = 0;      // offset of the rect within the tile slot
	};
	struct Layout
	{
		// Marker positions per edge, in STREAM ORDER (the diamond walk), which
		// is what makes a strip splat a fixed permutation of the strip.
		std::vector<std::pair<int16_t, int16_t>> pos[4];
	};

	int LayoutOf(TileSet *ts, uint16_t tileNum);
	Entry const &BaseEntry(TileSet *ts, uint16_t tileNum);
	Entry const &StripEntry(TileSet *ts, int layoutId, int edge,
	                        uint16_t from, uint16_t to);
	Entry const &DefaultEntry(TileSet *ts, int layoutId, int edge,
	                          uint16_t from);
	bool Pack(int w, int h, int &ax, int &ay);
	bool UploadSplat(Entry &e,
	                 std::vector<std::pair<int16_t, int16_t>> const &pos,
	                 std::vector<uint32_t> const &pixels);

	TileSet *m_generatedFrom = nullptr;   // identity check: reload => Reset

	std::vector<Layout> m_layouts;
	std::unordered_map<uint64_t, int> m_layoutByHash;
	std::unordered_map<uint16_t, int> m_layoutOfTile;   // tileNum -> id, -1 none

	std::unordered_map<uint32_t, Entry> m_base;       // tileNum
	std::unordered_map<uint64_t, Entry> m_strips;     // layout|edge|from|to
	std::unordered_map<uint64_t, Entry> m_defaults;   // layout|edge|from

	// Shelf packer over the static atlas texture.
	int m_shelfX = 0, m_shelfY = 0, m_shelfH = 0;
	bool m_atlasBroken = false;
};

#endif
