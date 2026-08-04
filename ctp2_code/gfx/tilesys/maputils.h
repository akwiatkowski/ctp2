#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __MAPUTILS_H__
#define __MAPUTILS_H__

void	maputils_WrapPoint(sint32 x,sint32 y,sint32 *wrapX,sint32 *wrapY);
BOOL	maputils_TilePointInTileRect(sint32 x, sint32 y, RECT *tileRect);

sint32	maputils_TileX2MapX(sint32 tileX,sint32 tileY);
void	maputils_MapX2TileX(sint32 mapX,sint32 mapY,sint32 *tileX);
void    maputils_MapXY2PixelXY(sint32 mapX,	sint32 mapY, sint32 *pixelX, sint32 *pixelY);
void    maputils_MapXY2PixelXY(sint32 mapX, sint32 mapY, POINT & pixel);
void	maputils_MapXY2PixelXY(sint32 mapX, sint32 mapY, sint32 * pixelX, sint32 * pixelY, RECT * mapViewRect);

void	maputils_TileX2MapXAbs(sint32 tileX,sint32 tileY,sint32 *mapX);

// Absolute map -> whole-map-texture pixel (P13 / ADR-003).
//
// Unlike maputils_MapXY2PixelXY this consults NO view rect: the whole-map
// target holds the entire map, so a tile's place in it depends only on its map
// coordinates. That is also why it needs none of that function's wrap-splitting
// -- there is no view edge for the map to wrap around.
//
// Plain isometric arithmetic: a column is one tile wide, odd rows are nudged
// half a tile, and rows advance by HALF THE DIAMOND (k_TILE_PIXEL_HEIGHT / 2)
// because they interleave. Not half the grid cell: the 72px grid height
// includes 24px of elevation headroom above the diamond, and stepping by that
// spaces rows 1.5x too far apart (fixed in fda82c49).
//
// Returns the tile SLOT's top-left. DrawTransitionTile places the diamond
// k_TILE_PIXEL_HEADROOM down inside the slot, so callers aligning to the
// visible diamond -- rather than to terrain quads, which use the slot -- must
// add that offset themselves.
void	maputils_MapXY2WorldmapPixelXY(sint32 mapX, sint32 mapY, sint32 *pixelX, sint32 *pixelY);

#endif
