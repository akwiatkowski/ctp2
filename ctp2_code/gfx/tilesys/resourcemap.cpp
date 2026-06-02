//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Handling of resource map (city window subscreen)
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
// - Wrapping corrected.
// - Memory leak repaired.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gfx/tilesys/resourcemap.h"

#include "gfx/gfx_utils/colorset.h"                   // colorset_Get()
#include "gfx/spritesys/Actor.h"
#include "gfx/spritesys/GoodActor.h"
#include "gfx/spritesys/UnitActor.h"
#include "gfx/spritesys/director.h"                   // director_Get()
#include "gfx/tilesys/BaseTile.h"
#include "gs/world/TileInfo.h"
#include "gfx/tilesys/maputils.h"
#include "gfx/tilesys/tiledmap.h"                   // tiledmap_Get
#include "gfx/tilesys/tileset.h"
#include "gs/database/StrDB.h"                      // g_theStringDB
#include "gs/database/profileDB.h"                  // profiledb_Get()
#include "gs/events/GameEventManager.h"
#include "gs/gameobj/CityInfluenceIterator.h"
#include "gs/gameobj/CityRadius.h"
#include "gs/gameobj/Player.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/UnitData.h"
#include "gs/gameobj/UnitPool.h"
#include "gs/gameobj/citydata.h"
#include "WonderRecord.h"
#include "gs/slic/SlicEngine.h"
#include "gs/utility/Globals.h"
#include "gs/world/Cell.h"
#include "gs/world/MapPoint.h"
#include "gs/world/World.h"                      // world_Get()
#include "robot/aibackdoor/dynarr.h"
#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_Factory.h"
#include "ui/aui_common/aui_blitter.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_stringtable.h"
#include "ui/aui_common/aui_surface.h"
#include "ui/aui_common/aui_window.h"
#include "ui/aui_ctp2/SelItem.h"                    // selitem_Get()
#include "ui/aui_ctp2/c3_updateaction.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_utils/primitives.h"
#include "ui/aui_utils/textutils.h"
#include "ui/ldl/ldl_user.h"

#define k_NUDGE		48

#define k_CITYNAME_PTSIZE		12
#define k_POP_PTSIZE			10
#define k_POP_BOX_XOFFSET		3
#define k_POP_BOX_YOFFSET		5
#define k_POP_BOX_SIZE			10
#define k_POP_BOX_SIZE_MINIMUM	4
#define k_FONT					0

#define k_RES_PTSIZE			8

#define k_OFFSET_WIDTH			62




#define k_MAX_CITY_RADIUS       3






ResourceMap::ResourceMap(AUI_ERRCODE *retval,
							sint32 id,
							MBCHAR *ldlBlock,
							ControlActionCallback *ActionFunc,
							void *cookie)
	:
		aui_ImageBase(ldlBlock),
		aui_TextBase(ldlBlock),
		aui_Control(retval, id, ldlBlock, ActionFunc, cookie),
		PatternBase(ldlBlock, NULL)
{
	InitCommonLdl(ldlBlock);
}

ResourceMap::ResourceMap(AUI_ERRCODE *retval,
							uint32 id,
							sint32 x,
							sint32 y,
							sint32 width,
							sint32 height,
							MBCHAR *pattern,
							ControlActionCallback *ActionFunc,
							void *cookie)
	:
		aui_ImageBase((sint32)0),
		aui_TextBase((MBCHAR *)NULL),
		aui_Control(retval, id, x, y, width, height, ActionFunc, cookie),
		PatternBase(pattern)
{
	InitCommon( k_RESOURCEMAP_DEFAULT_SCALE );
}

ResourceMap::~ResourceMap()
{
	delete m_surface;
	delete m_string;
	delete m_updateAction;
}

void ResourceMap::InitCommonLdl(MBCHAR *ldlBlock)
{
    ldl_datablock * block = aui_Ldl::FindDataBlock(ldlBlock);
	Assert( block != NULL );
	if ( !block ) return;

	sint32 scale = k_RESOURCEMAP_DEFAULT_SCALE;

	if (block->GetAttributeType( k_RESOURCEMAP_LDL_SCALE ) == ATTRIBUTE_TYPE_INT)
    {
		scale = block->GetInt( k_RESOURCEMAP_LDL_SCALE );
	}

	InitCommon( scale );
}

void ResourceMap::InitCommon( sint32 scale)
{
	AUI_ERRCODE			errcode;

	m_scale = scale;
	m_drawHilite = FALSE;

	m_unit = Unit();

	m_updateAction = NULL;

	m_surface = aui_Factory::new_Surface(errcode,
		tiledmap_Get()->GetZoomTilePixelWidth() * ((k_MAX_CITY_RADIUS * 2) + 1),
		tiledmap_Get()->GetZoomTilePixelHeight() * ((k_MAX_CITY_RADIUS * 2) + 2));

	Assert(m_surface);
	if ( !m_surface ) return;

	m_string = new aui_StringTable( &errcode, "ResourceMapStrings" );
	Assert( m_string );
}

AUI_ERRCODE ResourceMap::DrawThis( aui_Surface *surface, sint32 x, sint32 y )
{




	if ( IsHidden() ) return AUI_ERRCODE_OK;

	if ( !surface ) surface = m_window->TheSurface();

	RECT rect = { 0, 0, m_width, m_height };
	OffsetRect( &rect, m_x + x, m_y + y );
	ToWindow( &rect );






	UpdateFromSurface(surface, &rect);

	if (surface == m_window->TheSurface()) {
		m_window->AddDirtyRect(&rect);
	}

	return AUI_ERRCODE_OK;
}

void ResourceMap::MouseLGrabInside( aui_MouseEvent *mouseData )
{
	if (IsDisabled()) return;

	if (GetWhichSeesMouse() && GetWhichSeesMouse() != this) return;

	SetWhichSeesMouse(this);

	mouseData->position.x -= X();
	mouseData->position.y -= Y();

	Click(mouseData);
	DrawSurface();
}

void ResourceMap::MouseMoveInside(aui_MouseEvent *data)

{
	if (IsDisabled()) return;

	if (GetWhichSeesMouse() && GetWhichSeesMouse() != this) return;
	SetWhichSeesMouse(this);

    Assert(data);

	POINT temp = data->position;
	temp.x -= X();
	temp.y -= Y();

    MapPoint tmp;
    if (MousePointToTilePos(temp, tmp)){
        m_current_mouse_tile = tmp;
    }

	SetHiliteMouseTile(m_current_mouse_tile);

	m_drawHilite = TRUE;
	tiledmap_Get()->DrawHilite( FALSE );
}

void ResourceMap::MouseMoveAway(aui_MouseEvent *data)

{
	if (IsDisabled()) return;

	if (GetWhichSeesMouse() && GetWhichSeesMouse() != this) return;
	SetWhichSeesMouse(this);

    Assert(data);

	m_drawHilite = FALSE;
}





void ResourceMap::NotifyPopChanged()
{
	DrawSurface();
}







sint32 ResourceMap::DrawSurface(void)
{
	sint32 width = m_surface->Width();
	sint32 height = m_surface->Height();

	RECT rect = {0,0,width,height};
	primitives_PaintRect16(m_surface,&rect,0x0000);

	if (!m_unit.IsValid()) return -1;

	MapPoint pos;
	m_unit.GetData()->GetPos(pos);

	double scale = tiledmap_Get()->GetScale();
	sint32 zoomLevel = tiledmap_Get()->GetZoomLevel();

	if ( m_scale ) {
		tiledmap_Get()->SetZoomLevel(k_ZOOM_SMALLEST );
	}
	else {
		tiledmap_Get()->SetZoomLevel(k_ZOOM_NORMAL );
	}

	tiledmap_Get()->RecalculateViewRect(m_normalizedViewRect);

	MapPoint	myPos1 = pos;
	sint32	leftEdge;
	sint32	topEdge;
	maputils_MapXY2PixelXY(myPos1.x, myPos1.y, &leftEdge, &topEdge, &m_normalizedViewRect);
	leftEdge -= tiledmap_Get()->GetZoomTilePixelWidth() * k_MAX_CITY_RADIUS;
	topEdge -= tiledmap_Get()->GetZoomTilePixelHeight() * k_MAX_CITY_RADIUS;

	m_leftEdge = leftEdge;
	m_topEdge = topEdge;

	tiledmap_Get()->LockThisSurface(m_surface);

	m_usedRect.left = m_usedRect.top = m_usedRect.right = m_usedRect.bottom = -1;

	DrawResourceMapThing(m_surface, DrawATile);

	tiledmap_Get()->UnlockSurface();

	tiledmap_Get()->SetZoomLevel(zoomLevel);
	tiledmap_Get()->SetScale( scale );

	tiledmap_Get()->GetMapBounds( m_mapBounds );

	m_mapViewRect.left = 3;
	m_mapViewRect.right = 7;
	m_mapViewRect.top = 2;
	m_mapViewRect.bottom = 9;

	DrawSprites(m_surface, NULL);

	return 0;
}

sint32 ResourceMap::DrawSpaceImprovements( aui_Surface *pSurface, sint32 xOff, sint32 yOff )
{
	MapPoint pos;

	if (m_unit.m_id)
		m_unit.GetData()->GetPos(pos);
	else
		return 0;

	double scale = tiledmap_Get()->GetScale();
	sint32 zoomLevel = tiledmap_Get()->GetZoomLevel();

	if ( m_scale ) {
		tiledmap_Get()->SetZoomLevel(k_ZOOM_SMALLEST );
	}
	else {
		tiledmap_Get()->SetZoomLevel(k_ZOOM_NORMAL );
	}

	MapPoint newpos;
	for (sint32 j =0;j < 3;j++) {
		pos.GetNeighborPosition(NORTHWEST, newpos);
		pos = newpos;
	}

	tiledmap_Get()->GetMapBounds( m_mapBounds );
	sint32 tileX;
	maputils_MapX2TileX(pos.x, pos.y, &tileX);
	m_mapViewRect.left = 3;
	m_mapViewRect.right = 7;
	m_mapViewRect.top = 2;
	m_mapViewRect.bottom = 9;

	sint32 nudge = 0;
	sint32 index = 0;

	tiledmap_Get()->LockThisSurface(pSurface);

	for (sint32 y = 0;y < 7;y++) {
		if (y & 0x01) {
			if ( !m_scale ) {
				nudge = k_NUDGE;
			}
			else {
				nudge = k_NUDGE / 2;
			}
			pos.GetNeighborPosition(SOUTHWEST, newpos);
		}
		else {
			pos.GetNeighborPosition(SOUTHEAST, newpos);
			nudge = 0;
		}

        sint32 i;
		maputils_MapX2TileX(pos.x,pos.y,&i);

		sint32 x;
		for (x = 0; x < 3; ++x) {
			if (x==0 && (y==0 || y==6)) continue;
			if ( !m_scale )
				DrawImprovements(pSurface, pos.y, i+x, x*96+nudge+xOff,y*24+yOff);
			else
				DrawImprovements(pSurface, pos.y, i+x, x*48+nudge+xOff,y*12+yOff);

#if 0   // Useless local variable updates
            sint32 mapX = maputils_TileX2MapX(i+x,pos.y);
			MapPoint tempPos( mapX, pos.y);
			Cell *cell = world_Get()->GetCell(tempPos);
#endif
			index++;
		}
		if (y==2 || y==4) {
			if ( !m_scale )
				DrawImprovements(pSurface, pos.y, i+x, x*96+nudge+xOff,y*24+yOff);
			else
				DrawImprovements(pSurface, pos.y, i+x, x*48+nudge+xOff,y*12+yOff);

#if 0   // Useless local variable updates
			sint32 mapX = maputils_TileX2MapX(i+x,pos.y);
			MapPoint tempPos (mapX, pos.y);
			Cell *cell = world_Get()->GetCell(tempPos);
#endif
			index++;
		}
		pos = newpos;
	}

	tiledmap_Get()->UnlockSurface();

	tiledmap_Get()->SetZoomLevel(zoomLevel);
	tiledmap_Get()->SetScale( scale );

	return TRUE;
}

BOOL ResourceMap::DrawACity(aui_Surface *pSurface, MapPoint const & pos, void *context)
{
	Unit		city = world_Get()->GetCell(pos)->GetCity();
	if (city.m_id == 0) return FALSE;

	UnitActorPtr actor = city.GetActor();
	if (!actor) return FALSE;

	sint32		mapWidth, mapHeight;
	tiledmap_Get()->GetMapMetrics(&mapWidth,&mapHeight);

	ResourceMap		*resourceMap = (ResourceMap *)context;
	sint32		x, y;
	maputils_MapXY2PixelXY(pos.x, pos.y, &x, &y, resourceMap->GetNormalizedViewRect());

	x -= resourceMap->GetLeftEdge();
	y -= resourceMap->GetTopEdge();

	if (x < 0) x += (mapWidth * tiledmap_Get()->GetZoomTilePixelWidth());
	if (y < 0) y += (mapHeight * tiledmap_Get()->GetZoomTilePixelHeight()/2);

	if (resourceMap->GetScale())
	{
		actor->DrawDirect(pSurface, x, y, tiledmap_Get()->GetZoomScale(k_ZOOM_SMALLEST));
	}
	else
	{
		actor->DrawDirect(pSurface, x, y, tiledmap_Get()->GetZoomScale(k_ZOOM_LARGEST));
	}

	return TRUE;
}

BOOL ResourceMap::DrawALandCity(aui_Surface *pSurface, MapPoint const & pos, void *context)
{
	Unit		city = world_Get()->GetCell(pos)->GetCity();
	if (city.m_id == 0) return FALSE;

	UnitActorPtr actor = city.GetActor();
	if (!actor) return FALSE;

	sint32		mapWidth, mapHeight;
	tiledmap_Get()->GetMapMetrics(&mapWidth,&mapHeight);

	ResourceMap		*resourceMap = (ResourceMap *)context;
	sint32		x, y;
	maputils_MapXY2PixelXY(pos.x, pos.y, &x, &y, resourceMap->GetNormalizedViewRect());

	x -= resourceMap->GetLeftEdge();
	y -= resourceMap->GetTopEdge();

	if (x < 0) x += (mapWidth * tiledmap_Get()->GetZoomTilePixelWidth());
	if (y < 0) y += (mapHeight * tiledmap_Get()->GetZoomTilePixelHeight()/2);

	if (resourceMap->GetScale())
	{
		actor->DrawDirect(pSurface, x, y, tiledmap_Get()->GetZoomScale(k_ZOOM_SMALLEST));
	}
	else
	{
		actor->DrawDirect(pSurface, x, y, tiledmap_Get()->GetZoomScale(k_ZOOM_LARGEST));
	}

	return TRUE;
}

BOOL ResourceMap::DrawAGood(aui_Surface *pSurface, MapPoint const & pos, void *context)
{

	TileInfo *curTileInfo = tiledmap_Get()->GetTileInfo(pos);
	Assert(curTileInfo != NULL);
	if(!curTileInfo || !curTileInfo->HasGoodActor()) return FALSE;

	GoodActor	*goodActor = curTileInfo->GetGoodActor();
	if (!goodActor) return FALSE;

	sint32		mapWidth, mapHeight;
	tiledmap_Get()->GetMapMetrics(&mapWidth,&mapHeight);

	ResourceMap		*resourceMap = (ResourceMap *)context;
	sint32		x, y;
	maputils_MapXY2PixelXY(pos.x, pos.y, &x, &y, resourceMap->GetNormalizedViewRect());

	x -= resourceMap->GetLeftEdge();
	y -= resourceMap->GetTopEdge();

	if (x < 0) x += (mapWidth * tiledmap_Get()->GetZoomTilePixelWidth());
	if (y < 0) y += (mapHeight * tiledmap_Get()->GetZoomTilePixelHeight()/2);

	if (resourceMap->GetScale())
	{
		goodActor->DrawDirect(pSurface, x, y, tiledmap_Get()->GetZoomScale(k_ZOOM_SMALLEST));
	}
	else {
		goodActor->DrawDirect(pSurface, x, y, tiledmap_Get()->GetZoomScale(k_ZOOM_LARGEST));
	}

	return TRUE;
}

BOOL ResourceMap::DrawATile(aui_Surface *pSurface, MapPoint const & pos, void *context)
{
	sint32		mapWidth, mapHeight;
	tiledmap_Get()->GetMapMetrics(&mapWidth,&mapHeight);

	ResourceMap		*resourceMap = (ResourceMap *)context;
	sint32		x, y;
	maputils_MapXY2PixelXY(pos.x, pos.y, &x, &y, resourceMap->GetNormalizedViewRect());

	x -= resourceMap->GetLeftEdge();
	y -= resourceMap->GetTopEdge();

	if (x < 0) x += (mapWidth * tiledmap_Get()->GetZoomTilePixelWidth());
	if (y < 0) y += (mapHeight * tiledmap_Get()->GetZoomTilePixelHeight()/2);

	if(x >= mapWidth * tiledmap_Get()->GetZoomTilePixelWidth()) {


		x -= mapWidth * tiledmap_Get()->GetZoomTilePixelWidth();
	}
	if (y >= mapHeight * tiledmap_Get()->GetZoomTilePixelHeight() / 2)
	{
		y -= mapHeight * tiledmap_Get()->GetZoomTilePixelHeight() / 2;
	}

	sint32 tileX;
	maputils_MapX2TileX(pos.x, pos.y, &tileX);

	resourceMap->CalculateWrap(pSurface, pos.y, tileX, x, y);

	if(resourceMap->m_usedRect.left < 0 || x < resourceMap->m_usedRect.left) {
		resourceMap->m_usedRect.left = x;
	}

	if(resourceMap->m_usedRect.right < 0 || x + tiledmap_Get()->GetZoomTilePixelWidth() > resourceMap->m_usedRect.right) {
		resourceMap->m_usedRect.right = x + tiledmap_Get()->GetZoomTilePixelWidth();
	}

	if(resourceMap->m_usedRect.top < 0 || y < resourceMap->m_usedRect.top) {
		resourceMap->m_usedRect.top = y;
	}

	if(resourceMap->m_usedRect.bottom < 0 ||  y + tiledmap_Get()->GetZoomTilePixelHeight() > resourceMap->m_usedRect.bottom) {
		resourceMap->m_usedRect.bottom = y + tiledmap_Get()->GetZoomTilePixelHeight();
	}

	Cell	*cell = world_Get()->GetCell(pos);

	if (cell) {
		Unit	c;

		resourceMap->GetOwningCity(c);

		if (cell->GetOwner() != c.GetOwner()) {
			if ( !resourceMap->GetScale() ) {
				tiledmap_Get()->DrawTileBorder(pSurface, x, y, colorset_Get()->GetPlayerColor(cell->GetOwner()));
			}
			else {
				tiledmap_Get()->DrawTileBorderScaled(pSurface, pos, x, y,
						tiledmap_Get()->GetZoomTilePixelWidth(),
						tiledmap_Get()->GetZoomTilePixelHeight(),
						colorset_Get()->GetPlayerColor(cell->GetOwner()));
			}
		}

	}

	return TRUE;
}

BOOL ResourceMap::DrawResourceMapThing(aui_Surface *pSurface, ResourceMapDrawFunc *func)
{
	MapPoint	pos = m_unit.RetPos();

	sint32 zoomLevel = tiledmap_Get()->GetZoomLevel();

	if ( m_scale ) tiledmap_Get()->SetZoomLevel(k_ZOOM_SMALLEST );
	else tiledmap_Get()->SetZoomLevel(k_ZOOM_NORMAL );

	CityInfluenceIterator it(pos, m_unit.CD()->GetSizeIndex());
	for(it.Start(); !it.End(); it.Next()) {
		func(pSurface, it.Pos(), (void *)this);
	}


	tiledmap_Get()->SetZoomLevel(zoomLevel);

	return TRUE;
}







BOOL ResourceMap::DrawSprites(aui_Surface *pSurface, RECT *destRect)
{
	Assert(pSurface);
	if (!pSurface) return FALSE;

	if (!m_unit) return FALSE;

	if(!unitpool_Get()->IsValid(m_unit)) return FALSE;

	MapPoint pos;
	m_unit.GetData()->GetPos(pos);

	DrawResourceMapThing(pSurface, DrawAGood);
	DrawResourceMapThing(pSurface, DrawACity);

	m_totalFood = m_totalProd = m_totalGold = 0;

	Cell *cell = world_Get()->GetCell( pos );
	m_totalFood += cell->GetFoodProduced();
	m_totalProd += cell->GetShieldsProduced();

	return TRUE;
}







sint32 ResourceMap::CalculateWrap(
			aui_Surface *surface,
			sint32 i,
			sint32 j,
			sint32 x,
			sint32 y
			)
{
	maputils_WrapPoint(j,i,&j,&i);

	sint32 k = maputils_TileX2MapX(j,i);

	MapPoint tempPos (k, i);
	MapPoint	pos = tempPos;

	TileInfo *tileInfo = tiledmap_Get()->GetTileInfo(pos);
	if (tileInfo == NULL) return -1;

	BaseTile *baseTile = tiledmap_Get()->GetTileSet()->GetBaseTile(tileInfo->GetTileNum());
	if (baseTile == NULL) return -1;

	sint16	river = tileInfo->GetRiverPiece();

	if ( !m_scale ) {

		tiledmap_Get()->DrawTransitionTile(NULL, pos, x, y);

		tiledmap_Get()->DrawOverlay(NULL, baseTile->GetHatData(), x, y);

		if (river != -1)
			tiledmap_Get()->DrawOverlay(NULL, tiledmap_Get()->GetTileSet()->GetRiverData(river), x, y);

		tiledmap_Get()->DrawImprovementsLayer(NULL, pos, x, y);

	}
	else {

		tiledmap_Get()->DrawTransitionTileScaled(NULL, pos, x, y, tiledmap_Get()->GetZoomTilePixelWidth(), tiledmap_Get()->GetZoomTilePixelHeight() );

		tiledmap_Get()->DrawScaledOverlay(NULL, baseTile->GetHatData(), x, y,
										tiledmap_Get()->GetZoomTilePixelWidth(),
										tiledmap_Get()->GetZoomTileGridHeight());

		if ( river != -1 )
			tiledmap_Get()->DrawScaledOverlay(NULL, tiledmap_Get()->GetTileSet()->GetRiverData(river), x, y,
											tiledmap_Get()->GetZoomTilePixelWidth(),
											tiledmap_Get()->GetZoomTileGridHeight());

		tiledmap_Get()->DrawImprovementsLayer(NULL, pos, x, y);
	}

	return 0;
}

sint32 ResourceMap::DrawImprovements(
			aui_Surface *surface,
			sint32 i,
			sint32 j,
			sint32 x,
			sint32 y
			)
{
	maputils_WrapPoint(j,i,&j,&i);

	sint32 k = maputils_TileX2MapX(j,i);

	MapPoint pos (k, i);

	tiledmap_Get()->DrawImprovementsLayer(NULL, pos, x, y);

	return 0;
}

void ResourceMap::DrawCityName(aui_Surface *surface, sint32 x, sint32 y, const Unit &unit)
{
	Assert(unit.IsCity());
	if (!unit.IsCity()) return;

	sint32 yoffset = y - k_TILE_PIXEL_HEADROOM;

	CityData *cityData = unit.GetData()->GetCityData() ;
	MBCHAR *name = cityData->GetName() ;

	if (x >= 0 && yoffset >= 0 && x < surface->Width() && yoffset < surface->Height())
    {
	    RECT rect;
		rect.left = x;
		rect.top = yoffset;
		rect.right = x+textutils_GetWidth(surface,name);
		rect.bottom = y+textutils_GetHeight(surface,name);

		textutils_ColoredDropString(
			surface,
			name,
			x,
			yoffset,
			k_CITYNAME_PTSIZE,
			(COLOR)(COLOR_PLAYER1+unit.GetOwner()),
			(COLOR)(COLOR_BLACK),
			k_FONT
			);
	}

	sint32 pop          = cityData->PopCount();
	MBCHAR str[80];
	snprintf(str, sizeof(str),"%i",pop);

    sint32  popEdgeSize = std::max(k_POP_BOX_SIZE, k_POP_BOX_SIZE_MINIMUM);
	sint32  nudge       = 0;
	if (pop > 9)
		nudge = 4;
	if (pop > 99)
		nudge = 2;

	RECT popRect = {x,
					 y,
					 x + (popEdgeSize*2),
					 y + (popEdgeSize*2)};
	primitives_PaintRect16(surface,&popRect,colorset_Get()->GetPlayerColor(unit.GetOwner()));
	primitives_FrameRect16(surface,&popRect,0x0000);

	textutils_CenteredColoredDropString(
		surface,
		str,
		&popRect,
		k_POP_PTSIZE,
		COLOR_WHITE,
		COLOR_BLACK,
		k_FONT
		);
}

sint32 ResourceMap::UpdateFromSurface(aui_Surface *destSurface, RECT *destRect)
{
	if(m_usedRect.right - m_usedRect.left < m_usedRect.bottom - m_usedRect.top) {

		sint32 diff = (m_usedRect.bottom - m_usedRect.top) - (m_usedRect.right - m_usedRect.left);
		if(diff > 1) {
			m_usedRect.left -= diff / 2;
			m_usedRect.right += diff / 2;
		} else {
			m_usedRect.left -= 1;
		}
	} else if(m_usedRect.right - m_usedRect.left > m_usedRect.bottom - m_usedRect.top) {

		sint32 diff = (m_usedRect.right - m_usedRect.left) - (m_usedRect.bottom - m_usedRect.top);
		if(diff > 1) {
			m_usedRect.top -= diff / 2;
			m_usedRect.bottom += diff / 2;
		} else {
			m_usedRect.top -= 1;
		}
	}

	RECT rect = {m_usedRect.left,m_usedRect.top,m_usedRect.right,m_usedRect.bottom};
	c3ui_Get()->TheBlitter()->StretchBlt(destSurface, destRect, m_surface, &rect, k_AUI_BLITTER_FLAG_COPY);

	return 0;
}

void ResourceMap::Click(aui_MouseEvent *data)
{
	MapPoint		pos;
	POINT			point = data->position;

	if (MousePointToTilePos(point, pos))
	{
		if (data->lbutton && !data->rbutton)
		{
			HandlePop(pos);
		}
	}
}

BOOL ResourceMap::PointInMask(POINT hitPt)
{
	double const scale = (m_scale) ? 0.5 : 1.0;
	sint32 const x = (sint32)((double)hitPt.x / scale);
	sint32 const y = (sint32)(((double)hitPt.y / scale) + k_TILE_PIXEL_HEADROOM);

	TILEHITMASK *thm = tiledmap_Get()->GetTileHitMask();

	return (x >= thm[y].start) && (x <= thm[y].end);
}

BOOL ResourceMap::MousePointToTilePos(POINT point, MapPoint &tilePos)
{
	double const scale = (m_scale) ? 0.5 : 1.0;
	sint32	headroom = (sint32)(k_TILE_PIXEL_HEADROOM * scale);
	sint32 width = (sint32)(k_TILE_GRID_WIDTH * scale);
	sint32 height = (sint32)((k_TILE_GRID_HEIGHT-k_TILE_PIXEL_HEADROOM) * scale);
	sint32 x = point.x;
	sint32 y = point.y;

	if (!(m_mapViewRect.top & 1))
		y -= headroom;

	MapPoint		pos;
	pos.x = (sint16) (x / width + m_mapViewRect.left);
	pos.y = (sint16) ((y / height) + m_mapViewRect.top/2);

	POINT			hitPt;
	hitPt.x = x % width;
	hitPt.y = y % height;

	sint32 maxX = m_mapBounds.right;

	if (!PointInMask(hitPt))
	{
		pos.x = (sint16)((x + (width/2)) / width - 1 + m_mapViewRect.left);
		pos.y = (sint16)(((y + (height)/2) / height - 1) + m_mapViewRect.top/2);

		hitPt.x = (x + (width/2)) % width;
		hitPt.y = (y + (height)/2) % height;

		if (PointInMask(hitPt))
		{
			if (pos.x >= pos.y)
			{
				tilePos.x = pos.x - pos.y;
				tilePos.y = pos.y * 2 + 1;
			}
			else
			{
				tilePos.x = static_cast<sint16>(maxX + pos.x - pos.y);
				tilePos.y = pos.y * 2 + 1;
			}
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		if (pos.x >= pos.y)
		{
			tilePos.x = pos.x - pos.y;
			tilePos.y = pos.y * 2;
		}
		else
		{
			tilePos.x = static_cast<sint16>(maxX + pos.x - pos.y);
			tilePos.y = pos.y * 2;
		}
	}

	if (world_Get()->IsYwrap()) {
		if (tilePos.x <0)
		{
			tilePos.x += static_cast<sint16>(world_Get()->GetWidth());
		}
		else if (world_Get()->GetWidth() <= tilePos.x)
		{
			tilePos.x -= static_cast<sint16>(world_Get()->GetWidth());
		}

		sint16 sx, sy;
		if (tilePos.y < 0) {
			sx = (sint16)world_Get()->GetWidth();
			sy = (sint16)world_Get()->GetHeight();
			tilePos.y += sy;
			tilePos.x = (tilePos.x + (sx - (sy/2))) % sx;
		} else if (world_Get()->GetHeight() <= tilePos.y) {
			sx = (sint16)world_Get()->GetWidth();
			sy = (sint16)world_Get()->GetHeight();
			tilePos.y -= sy;
			tilePos.x = (tilePos.x - (sx - (sy/2))) % sx;
		}
	} else {

		if (tilePos.y <0)
		{
			tilePos.y = 0;
			return FALSE;
		}
		else if (world_Get()->GetHeight() <= tilePos.y)
		{
			tilePos.y = static_cast<sint16>(world_Get()->GetHeight() - 1);
			return FALSE;
		}
	}

	if (tilePos.x <0)
	{
		tilePos.x += static_cast<sint16>(world_Get()->GetWidth());
	}
	else if (world_Get()->GetWidth() <= tilePos.x)
	{
		tilePos.x -= static_cast<sint16>(world_Get()->GetWidth());
	}

	return TRUE;
}

void ResourceMap::SetHiliteMouseTile(MapPoint &pos)
{
	m_hiliteMouseTile = pos;
}

void ResourceMap::DrawHiliteMouseTile(aui_Surface *destSurf, RECT *destRect)
{
	sint32 zoomLevel = tiledmap_Get()->GetZoomLevel();
	double scale = tiledmap_Get()->GetScale();

	if ( m_scale ) {
		tiledmap_Get()->SetZoomLevel(k_ZOOM_SMALLEST);
	}
	else {
		tiledmap_Get()->SetZoomLevel(k_ZOOM_NORMAL );
	}

	tiledmap_Get()->DrawHitMask(destSurf, m_hiliteMouseTile, &m_mapViewRect, destRect);

	tiledmap_Get()->SetZoomLevel(zoomLevel);
	tiledmap_Get()->SetScale( scale );
}

void ResourceMap::HandlePop( MapPoint point )
{
	MapPoint mp;
	m_unit.GetData()->GetPos(mp);

	sint32 diffY = 5 - point.y;
	sint32 diffX = 2 - point.x;

	sint32 x = mp.x - diffX;
	sint32 y = mp.y - diffY;

	sint32 xx, yy;
	maputils_WrapPoint(x, y, &xx, &yy);
	point.x = (sint16)xx;
	point.y = (sint16)yy;
}




AUI_ERRCODE ResourceMap::Idle( void )
{
	static uint32 lastDraw = 0;
	if (GetTickCount() - lastDraw > 100)
	{
		lastDraw = GetTickCount();
		DrawThis(NULL, 0, 0);
	}

	return AUI_ERRCODE_OK;
}
