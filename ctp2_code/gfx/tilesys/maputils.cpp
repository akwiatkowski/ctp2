//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Map utilities
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
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Handling corrected for non-X-wrapping worlds.
// - Ambiguous ceil call resolved.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/aui_common/aui.h"
#include "gfx/tilesys/tiledmap.h"
#include "gfx/tilesys/maputils.h"
#include "gs/database/profileDB.h"
#include "gs/gameobj/XY_Coordinates.h"
#include "gs/world/World.h"

extern sint32		g_scaled_pixel_width[9];
extern sint32		g_scaled_pixel_height[9];




void maputils_WrapPoint(
	sint32 x,
	sint32 y,
	sint32 *wrapX,
	sint32 *wrapY
	)
{
	sint32 mapWidth;
	sint32 mapHeight;
	tiledmap_Get()->GetMapMetrics(&mapWidth,&mapHeight);

	*wrapX = x;
	*wrapY = y;

	if (world_Get()->IsXwrap())
	{
		while(*wrapX < 0)
			*wrapX = *wrapX + mapWidth;
		while(*wrapX >= mapWidth)
			*wrapX = *wrapX - mapWidth;
	}
	else
	{
		if (*wrapX < 0)
		{
			*wrapX = 0;
		}
		else if (*wrapX >= mapWidth)
		{
			*wrapX = mapWidth - 1;
		}
		// else: no action: *wrapX OK
	}

	if (world_Get()->IsYwrap()) {

		while(*wrapY<0)
			*wrapY = *wrapY + mapHeight;

		while(*wrapY >= mapHeight)
			*wrapY = *wrapY - mapHeight;
	} else {
		if (*wrapY < 0) *wrapY = 0;
		if (*wrapY >= mapHeight) *wrapY = mapHeight-1;
	}
}

BOOL maputils_TilePointInTileRect(sint32 x, sint32 y, RECT *tileRect)
{
	sint32 mapWidth;
	sint32 mapHeight;
	tiledmap_Get()->GetMapMetrics(&mapWidth,&mapHeight);

	RECT wrappedRect = *tileRect;

	if (wrappedRect.left < 0) wrappedRect.left += mapWidth;
	if (wrappedRect.right >= mapWidth) wrappedRect.right -= mapWidth;
	if (wrappedRect.top < 0) wrappedRect.top += mapHeight;
	if (wrappedRect.bottom >= mapHeight) wrappedRect.bottom -= mapHeight;

	if (wrappedRect.right < wrappedRect.left) {

		if (x < wrappedRect.left && x >= wrappedRect.right) return FALSE;
	} else {

		if (x < wrappedRect.left || x >= wrappedRect.right) return FALSE;
	}

	if (wrappedRect.bottom < wrappedRect.top) {

		if (y < wrappedRect.top && y >= wrappedRect.bottom) return FALSE;
	} else {
		if (y < wrappedRect.top || y >= wrappedRect.bottom) return FALSE;
	}

 	return TRUE;
}


sint32 maputils_TileX2MapX(
	sint32 tileX,
	sint32 tileY
	)
{

    sint32 mapWidth = tiledmap_Get()->GetMapWidth();
    tileX -= (tileY>>1);







	while (tileX<0)
		tileX += mapWidth;





	while (tileX>=mapWidth)
		tileX -= mapWidth;

	return tileX;
}





void maputils_TileX2MapXAbs(
	sint32 tileX,
	sint32 tileY,
	sint32 *mapX
	)
{
	sint32 mapWidth;
	sint32 mapHeight;

	tiledmap_Get()->GetMapMetrics(&mapWidth,&mapHeight);

	*mapX = (tileX - (tileY >> 1)) % mapWidth;

	while (*mapX<0)
		*mapX+=mapWidth;

}

void maputils_MapX2TileX(
	sint32 mapX,
	sint32 mapY,
	sint32 *tileX
	)
{
	sint32 mapWidth;
	sint32 mapHeight;
	tiledmap_Get()->GetMapMetrics(&mapWidth,&mapHeight);
	*tileX = (mapX + mapY/2) % mapWidth;
}

void maputils_MapXY2PixelXY(
	sint32 mapX,
	sint32 mapY,
	sint32 *pixelX,
	sint32 *pixelY
	)
{
	if (!tiledmap_Get()) return;

	sint32		 mapWidth;
	sint32		 mapHeight;
	sint32		nudge;
	RECT		 splitViewRectL;
	RECT		 splitViewRectR;
	RECT		 splitViewRectT;
	RECT		 splitViewRectB;
	POINT		tempPos;

	RECT		*mapViewRect = tiledmap_Get()->GetMapViewRect();

	tiledmap_Get()->GetMapMetrics(&mapWidth,&mapHeight);

	splitViewRectB = splitViewRectT = splitViewRectR = splitViewRectL = *mapViewRect;

	if (mapY & 0x01)

		nudge = tiledmap_Get()->GetZoomTilePixelWidth()>>1;
	else
		nudge = 0;

	sint32 realX;
	maputils_MapX2TileX(mapX,mapY,&realX);
	tempPos.x = realX;
	tempPos.y = mapY;












	if(mapViewRect->right >= mapWidth && mapViewRect->left < mapWidth) {
		splitViewRectR.left = mapViewRect->left;
		splitViewRectR.right = mapWidth;
		splitViewRectR.top = 0;
		splitViewRectR.bottom = mapHeight;
		splitViewRectL.left = 0 - (splitViewRectR.right - splitViewRectR.left);
		splitViewRectL.right = mapViewRect->right - mapWidth;
		splitViewRectL.top = 0;
		splitViewRectL.bottom = mapHeight;
	}
	else if(mapViewRect->left < 0 && mapViewRect->right >= 0) {
		splitViewRectR.left = mapViewRect->left + mapWidth;
		splitViewRectR.right = mapWidth;
		splitViewRectR.top = 0;
		splitViewRectR.bottom = mapHeight;
		splitViewRectL.left = 0 - (splitViewRectR.right - splitViewRectR.left);
		splitViewRectL.right = mapViewRect->right;
		splitViewRectL.top = 0;
		splitViewRectL.bottom = mapHeight;
	}

	if(mapViewRect->bottom >= mapHeight && mapViewRect->top < mapHeight) {
		splitViewRectB.top = mapViewRect->top;
		splitViewRectB.bottom = mapHeight;
		splitViewRectB.left = 0;
		splitViewRectB.right = mapWidth;
		splitViewRectT.top = 0 - (splitViewRectB.bottom - splitViewRectB.top);
		splitViewRectT.bottom = mapViewRect->bottom - mapHeight;
		splitViewRectT.left = 0;
		splitViewRectT.right = mapWidth;
	}
	else if((mapViewRect->top < 0 && world_Get()->IsYwrap()) && mapViewRect->bottom >= 0) {
		splitViewRectB.top = mapViewRect->top + mapHeight;
		splitViewRectB.bottom = mapHeight;
		splitViewRectB.left = 0;
		splitViewRectB.right = mapWidth;
		splitViewRectT.top = 0 - (splitViewRectB.bottom - splitViewRectB.top);
		splitViewRectT.bottom = mapViewRect->bottom;
		splitViewRectT.left = 0;
		splitViewRectT.right = mapWidth;
	}

	if(PtInRect(&splitViewRectL, tempPos))
	{
		*pixelX = ((sint32)((realX * (tiledmap_Get()->GetZoomTilePixelWidth()) + nudge) - (splitViewRectL.left * (tiledmap_Get()->GetZoomTilePixelWidth()))));
	}
	else if(PtInRect(&splitViewRectR, tempPos))
	{
		*pixelX = (sint32)((realX * (tiledmap_Get()->GetZoomTilePixelWidth()) + nudge) - (splitViewRectR.left * (tiledmap_Get()->GetZoomTilePixelWidth())));
	}
	else
	{
		*pixelX = (sint32)((realX * (tiledmap_Get()->GetZoomTilePixelWidth()) + nudge) - (mapViewRect->left * (tiledmap_Get()->GetZoomTilePixelWidth())));
	}

	if(PtInRect(&splitViewRectT, tempPos))
	{
	 	*pixelY = (sint32)((mapY * ((tiledmap_Get()->GetZoomTilePixelHeight())/2)) - (splitViewRectT.top * ((tiledmap_Get()->GetZoomTilePixelHeight())/2)));

	}
	else if(PtInRect(&splitViewRectB, tempPos))
	{
	 	*pixelY = (sint32)((mapY * ((tiledmap_Get()->GetZoomTilePixelHeight())/2)) - (splitViewRectB.top * ((tiledmap_Get()->GetZoomTilePixelHeight())/2)));

	}
	else
	{
	  	*pixelY = (sint32)((mapY * ((tiledmap_Get()->GetZoomTilePixelHeight())/2)) - (mapViewRect->top * ((tiledmap_Get()->GetZoomTilePixelHeight())/2)));

	}

	sint32 xoff;
	sint32 yoff;

	tiledmap_Get()->GetSmoothScrollOffsets(xoff,yoff);

	*pixelX -= xoff;
	*pixelY -= yoff;
}

void maputils_MapXY2PixelXY
(
    sint32  mapX,
    sint32  mapY,
    POINT & pixel
)
{
    sint32  pixelX;
    sint32  pixelY;
    maputils_MapXY2PixelXY(mapX, mapY, &pixelX, &pixelY);
    pixel.x = pixelX;
    pixel.y = pixelY;
}

void maputils_MapXY2PixelXY(
	sint32 mapX,
	sint32 mapY,
	sint32 *pixelX,
	sint32 *pixelY,
	RECT *mapViewRect
	)
{
	sint32		 mapWidth;
	sint32		 mapHeight;
	sint32		nudge;
	RECT		 splitViewRectL;
	RECT		 splitViewRectR;
	RECT		 splitViewRectT;
	RECT		 splitViewRectB;
	POINT		tempPos;

	sint32		tileWidth = k_TILE_PIXEL_WIDTH + 2;
	double		scale = tiledmap_Get()->GetScale();




	tiledmap_Get()->GetMapMetrics(&mapWidth,&mapHeight);

	splitViewRectB = splitViewRectT = splitViewRectR = splitViewRectL = *mapViewRect;

	if (mapY & 0x01) {

		nudge = (sint32)((tileWidth*scale)/2);
		nudge = tiledmap_Get()->GetZoomTilePixelWidth() / 2;
	} else
		nudge = 0;

	sint32 realX;
	maputils_MapX2TileX(mapX,mapY,&realX);
	tempPos.x = realX;
	tempPos.y = mapY;












	if(mapViewRect->right >= mapWidth && mapViewRect->left < mapWidth) {
		splitViewRectR.left = mapViewRect->left;
		splitViewRectR.right = mapWidth;
		splitViewRectR.top = 0;
		splitViewRectR.bottom = mapHeight;
		splitViewRectL.left = 0 - (splitViewRectR.right - splitViewRectR.left);
		splitViewRectL.right = mapViewRect->right - mapWidth;
		splitViewRectL.top = 0;
		splitViewRectL.bottom = mapHeight;
	}
	else if(mapViewRect->left < 0 && mapViewRect->right >= 0) {
		splitViewRectR.left = mapViewRect->left + mapWidth;
		splitViewRectR.right = mapWidth;
		splitViewRectR.top = 0;
		splitViewRectR.bottom = mapHeight;
		splitViewRectL.left = 0 - (splitViewRectR.right - splitViewRectR.left);
		splitViewRectL.right = mapViewRect->right;
		splitViewRectL.top = 0;
		splitViewRectL.bottom = mapHeight;
	}

	if(mapViewRect->bottom >= mapHeight && mapViewRect->top < mapHeight) {
		splitViewRectB.top = mapViewRect->top;
		splitViewRectB.bottom = mapHeight;
		splitViewRectB.left = 0;
		splitViewRectB.right = mapWidth;
		splitViewRectT.top = 0 - (splitViewRectB.bottom - splitViewRectB.top);
		splitViewRectT.bottom = mapViewRect->bottom - mapHeight;
		splitViewRectT.left = 0;
		splitViewRectT.right = mapWidth;
	}
	else if(mapViewRect->top < 0 && mapViewRect->bottom >= 0) {
		splitViewRectB.top = mapViewRect->top + mapHeight;
		splitViewRectB.bottom = mapHeight;
		splitViewRectB.left = 0;
		splitViewRectB.right = mapWidth;
		splitViewRectT.top = 0 - (splitViewRectB.bottom - splitViewRectB.top);
		splitViewRectT.bottom = mapViewRect->bottom;
		splitViewRectT.left = 0;
		splitViewRectT.right = mapWidth;
	}

	if(PtInRect(&splitViewRectL, tempPos))
	{
		*pixelX = ((sint32)((realX * (tiledmap_Get()->GetZoomTilePixelWidth()) + nudge) - (splitViewRectL.left * (tiledmap_Get()->GetZoomTilePixelWidth()))));
	}
	else if(PtInRect(&splitViewRectR, tempPos))
	{
		*pixelX = (sint32)((realX * (tiledmap_Get()->GetZoomTilePixelWidth()) + nudge) - (splitViewRectR.left * (tiledmap_Get()->GetZoomTilePixelWidth())));
	}
	else
	{
		*pixelX = (sint32)((realX * (tiledmap_Get()->GetZoomTilePixelWidth()) + nudge) - (mapViewRect->left * (tiledmap_Get()->GetZoomTilePixelWidth())));
	}

	if(PtInRect(&splitViewRectT, tempPos))
	{
		*pixelY = (sint32)((mapY * ((tiledmap_Get()->GetZoomTilePixelHeight())/2)) - (splitViewRectT.top * ((tiledmap_Get()->GetZoomTilePixelHeight())/2)));
	}
	else if(PtInRect(&splitViewRectB, tempPos))
	{
		*pixelY = (sint32)((mapY * ((tiledmap_Get()->GetZoomTilePixelHeight())/2)) - (splitViewRectB.top * ((tiledmap_Get()->GetZoomTilePixelHeight())/2)));
	}
	else
	{
		*pixelY = (sint32)((mapY * ((tiledmap_Get()->GetZoomTilePixelHeight())/2)) - (mapViewRect->top * ((tiledmap_Get()->GetZoomTilePixelHeight())/2)));
	}
}
