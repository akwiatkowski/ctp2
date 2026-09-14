//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Tile help window draw handling
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
// - Tile help window now dims tiles under the fog of war.
//   - Dec. 23rd 2004 - Martin Gühmann
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_window.h"
#include "ui/aui_utils/primitives.h"
#include "gfx/tilesys/tiledmap.h"
#include "gfx/tilesys/BaseTile.h"
#include "gs/world/TileInfo.h"
#include "gfx/tilesys/maputils.h"
#include "ui/aui_utils/primitives.h"
#include "ui/aui_common/aui_control.h"
#include "ui/aui_ctp2/tilecontrol.h"
#include "gfx/spritesys/GoodActor.h"


// Added by Martin Gühmann
extern sint32			g_isFastCpu; // Actual permernent set to 1




TileControl::TileControl(AUI_ERRCODE *retval, uint32 id, MBCHAR *ldlBlock )
:
	aui_ImageBase( ldlBlock ),
	aui_TextBase( ldlBlock, (MBCHAR *)nullptr ),
	aui_Control(retval,id,ldlBlock)
{

}

TileControl::TileControl(
	AUI_ERRCODE *retval,
	uint32 id,
	sint32 x,
	sint32 y,
	sint32 width,
	sint32 height,
	ControlActionCallback *ActionFunc,
	void *cookie )
:
	aui_ImageBase( (sint32)0 ),
	aui_TextBase(nullptr),
	aui_Control( retval, id, x, y, width, height, ActionFunc, cookie )
{

}


AUI_ERRCODE TileControl::DrawThis( aui_Surface *surface, sint32 x, sint32 y )
{

	if ( IsHidden() ) return AUI_ERRCODE_OK;
	if ( !surface ) surface = m_window->TheSurface();

	RECT rect = { 0, 0, m_width, m_height };
	OffsetRect( &rect, m_x + x, m_y + y );
	ToWindow( &rect );

	primitives_PaintRect16( surface, &rect, 0x0000 );

	sint32 i;
	maputils_MapX2TileX(m_currentTile.x, m_currentTile.y, &i);






	if (DrawTile(surface, m_currentTile.y,i,
				m_x + (m_width/2) - (k_TILE_PIXEL_WIDTH/2),
				m_y - k_TILE_PIXEL_HEADROOM + (m_height/2) - (k_TILE_PIXEL_HEIGHT/2)) == -1) return AUI_ERRCODE(-1);

	if (surface == m_window->TheSurface()) m_window->AddDirtyRect(&rect);




	return AUI_ERRCODE_OK;
}

sint32 TileControl::DrawTile(
			aui_Surface *surface,
			sint32 i,
			sint32 j,
			sint32 x,
			sint32 y
			)
{

	MapPoint	pos;
	sint16		river = -1;

	maputils_WrapPoint(j,i,&j,&i);

	sint32 k = maputils_TileX2MapX(j,i);

	MapPoint tempPos (k, i);

	pos = tempPos;

	TileInfo *tileInfo = tiledmap_Get()->GetTileInfo(pos);
	if (tileInfo == nullptr) return -1;

	river = tileInfo->GetRiverPiece();

	BaseTile *baseTile = tiledmap_Get()->GetTileSet()->GetBaseTile(tileInfo->GetTileNum());
	if (baseTile == nullptr) return -1;




	tiledmap_Get()->LockThisSurface(surface);

// Added by Martin Gühmann
	bool fog =((   tiledmap_Get()->GetLocalVision()
	            && tiledmap_Get()->GetLocalVision()->IsExplored(pos)
	            &&!tiledmap_Get()->GetLocalVision()->IsVisible(pos)));

	if (!fog) {
		tiledmap_Get()->DrawTransitionTile(nullptr, pos, x, y);
		tiledmap_Get()->DrawOverlay(nullptr, baseTile->GetHatData(), x, y);
		if (river != -1)
			tiledmap_Get()->DrawOverlay(nullptr, tiledmap_Get()->GetTileSet()->GetRiverData(river), x, y);
	}
	else {
		if (g_isFastCpu) {
			tiledmap_Get()->DrawBlendedTile(nullptr, pos,x,y,k_FOW_COLOR,k_FOW_BLEND_VALUE);
			tiledmap_Get()->DrawBlendedOverlay(nullptr, baseTile->GetHatData(),x,y,k_FOW_COLOR,k_FOW_BLEND_VALUE);
			if (river != -1)
				tiledmap_Get()->DrawBlendedOverlay(nullptr, tiledmap_Get()->GetTileSet()->GetRiverData(river),x,y,k_FOW_COLOR,k_FOW_BLEND_VALUE);
		}
		else {
			tiledmap_Get()->DrawDitheredTile(nullptr, x,y,k_FOW_COLOR);
			tiledmap_Get()->DrawDitheredOverlay(nullptr, baseTile->GetHatData(),x,y,k_FOW_COLOR);
			if (river != -1)
				tiledmap_Get()->DrawDitheredOverlay(nullptr, tiledmap_Get()->GetTileSet()->GetRiverData(river),x,y,k_FOW_COLOR);
		}
	}

	tiledmap_Get()->DrawImprovementsLayer(nullptr, pos, x, y);

	tiledmap_Get()->UnlockSurface();

	GoodActor *good = tileInfo->GetGoodActor();
	if (good) {
		good->DrawDirect(surface, x, y, 1.0);
	}

	return 0;
}
