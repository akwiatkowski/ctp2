#include "ctp/c3.h"
#include "gfx/tilesys/workmap.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_blitter.h"
#include "ui/aui_common/aui_Factory.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_window.h"
#include "ui/aui_common/aui_stringtable.h"

#include "ui/aui_utils/primitives.h"
#include "gs/utility/Globals.h"
#include "gs/gameobj/Player.h"
#include "robot/aibackdoor/dynarr.h"
#include "ui/aui_ctp2/SelItem.h"            // selitem_Get()
#include "gfx/spritesys/director.h"           // director_Get()
#include "gfx/tilesys/tiledmap.h"           // tiledmap_Get()
#include "gfx/tilesys/BaseTile.h"
#include "gs/world/TileInfo.h"
#include "gfx/tilesys/tileset.h"
#include "gfx/gfx_utils/colorset.h"           // colorset_Get()
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/UnitPool.h"
#include "ui/aui_ctp2/c3_updateaction.h"
#include "gfx/spritesys/Actor.h"
#include "gfx/spritesys/UnitActor.h"
#include "gfx/spritesys/workeractor.h"
#include "gs/gameobj/XY_Coordinates.h"
#include "gs/world/World.h"              // world_Get()
#include "gs/world/Cell.h"
#include "gs/world/MapPoint.h"
#include "WonderRecord.h"
#include "ui/aui_ctp2/c3ui.h"
#include "gfx/spritesys/GoodActor.h"
#include "gs/gameobj/citydata.h"
#include "ui/aui_utils/textutils.h"
#include "gfx/tilesys/maputils.h"
#include "gs/slic/SlicEngine.h"
#include "gs/database/profileDB.h"          // profiledb_Get()
#include "gs/gameobj/CityRadius.h"
#include "gs/database/StrDB.h"              // stringdb_Get()
#include "gs/gameobj/UnitData.h"
#include "gs/events/GameEventManager.h"
#include "gs/gameobj/CityInfluenceIterator.h"

#include "ui/ldl/ldl_data.hpp"

#define k_NUDGE		48

#define k_WORKER_INDEX		88
#define k_SLAVE_INDEX		89

#define k_CITYNAME_PTSIZE		12
#define k_POP_PTSIZE			10
#define k_POP_BOX_XOFFSET		3
#define k_POP_BOX_YOFFSET		5
#define k_POP_BOX_SIZE			10
#define k_POP_BOX_SIZE_MINIMUM	4
#define k_FONT					0

#define k_RES_PTSIZE			8

#define k_OFFSET_WIDTH			62






WorkMap::WorkMap(AUI_ERRCODE *retval,
							sint32 id,
							MBCHAR *ldlBlock,
							ControlActionCallback *ActionFunc,
							void *cookie)
	:
		aui_ImageBase(ldlBlock),
		aui_TextBase(ldlBlock),
		aui_Control(retval, id, ldlBlock, ActionFunc, cookie),
		PatternBase(ldlBlock, nullptr)
{
	InitCommonLdl(ldlBlock);
}

WorkMap::WorkMap(AUI_ERRCODE *retval,
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
		aui_TextBase((MBCHAR *)nullptr),
		aui_Control(retval, id, x, y, width, height, ActionFunc, cookie),
		PatternBase(pattern)
{
	InitCommon( k_WORKMAP_DEFAULT_SCALE );
}

WorkMap::~WorkMap()
{

	for (auto & i : m_worker) {
		if (i != nullptr) {
			delete i;
			i = nullptr;
		}
	}

	if (m_updateAction) {
		delete m_updateAction;
		m_updateAction = nullptr;
	}

	if (m_surface) {
		delete m_surface;
		m_surface = nullptr;
	}
}

void WorkMap::InitCommonLdl(MBCHAR *ldlBlock)
{
	ldl_datablock * block = aui_Ldl::FindDataBlock(ldlBlock);
	Assert( block != nullptr );
	if ( !block ) return;

	sint32 scale = k_WORKMAP_DEFAULT_SCALE;
	if (block->GetAttributeType( k_WORKMAP_LDL_SCALE ) == ATTRIBUTE_TYPE_INT) {
		scale = block->GetInt( k_WORKMAP_LDL_SCALE );
	}

	InitCommon( scale );
}

void WorkMap::InitCommon( sint32 scale)
{
	AUI_ERRCODE			errcode;

	m_scale = scale;
	m_drawHilite = FALSE;

	m_unit = Unit();

	m_updateAction = nullptr;

	for (auto & i : m_worker) {
		i = nullptr;
	}
	m_numWorkers = 0;

	m_surface = aui_Factory::new_Surface(errcode, m_width, m_height);
	Assert( m_surface != nullptr );
	if ( !m_surface ) return;

	m_string = new aui_StringTable( &errcode, "WorkMapStrings" );
	Assert( m_string );
	if ( !m_string ) return;
}

AUI_ERRCODE WorkMap::DrawThis( aui_Surface *surface, sint32 x, sint32 y )
{
	if ( IsHidden() ) return AUI_ERRCODE_OK;

	if ( !surface ) surface = m_window->TheSurface();

	RECT rect = { 0, 0, m_width, m_height };
	OffsetRect( &rect, m_x + x, m_y + y );
	ToWindow( &rect );

	UpdateFromSurface(surface, &rect);
	DrawSprites(surface, &rect);
	DrawLabels(surface);

	if ( m_drawHilite ) {
		DrawHiliteMouseTile(surface, &rect);
	}

	if (surface == m_window->TheSurface()) {
		m_window->AddDirtyRect(&rect);
	}

	return AUI_ERRCODE_OK;
}

void WorkMap::MouseLGrabInside( aui_MouseEvent *mouseData )
{
	if (IsDisabled()) return;

	if (GetWhichSeesMouse() && GetWhichSeesMouse() != this) return;

	SetWhichSeesMouse(this);

	mouseData->position.x -= X();
	mouseData->position.y -= Y();

	Click(mouseData);
	DrawSurface();
}

void WorkMap::MouseMoveInside(aui_MouseEvent *data)

{
	if (IsDisabled()) return;

	if (GetWhichSeesMouse() && GetWhichSeesMouse() != this) return;
	SetWhichSeesMouse(this);

	Assert(data);

	POINT temp = data->position;
	temp.x -= X();
	temp.y -= Y();

	MapPoint tmp;
	if (MousePointToTilePos(temp, tmp))
	{
		m_current_mouse_tile = tmp;
	}

	SetHiliteMouseTile(m_current_mouse_tile);

	m_drawHilite = TRUE;
	tiledmap_Get()->DrawHilite( FALSE );
}

void WorkMap::MouseMoveAway(aui_MouseEvent *data)
{
	if (IsDisabled()) return;

	if (GetWhichSeesMouse() && GetWhichSeesMouse() != this) return;
	SetWhichSeesMouse(this);

	Assert(data);

	m_drawHilite = FALSE;
}

void WorkMap::NotifyPopChanged()
{
	DrawSurface();
}

sint32 WorkMap::DrawSurface()
{
	sint32 width = m_surface->Width();
	sint32 height = m_surface->Height();

	RECT rect = {0,0,width,height};
	primitives_PaintRect16(m_surface,&rect,0x0000);

	MapPoint pos;
	MapPoint newpos;

	Assert( unitpool_Get()->IsValid(m_unit) );
	if ( !unitpool_Get()->IsValid(m_unit) ) return -1;

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

	sint32 i;




	MapPoint	myPos1 = pos,
				myPos2;
	sint32		leftEdge, topEdge, temp;

	tiledmap_Get()->RecalculateViewRect(m_normalizedViewRect);

	if (myPos1.GetNeighborPosition(NORTHWEST, myPos2)) {
		myPos2.GetNeighborPosition(WEST, myPos1);
		maputils_MapXY2PixelXY(myPos1.x, myPos1.y, &leftEdge, &temp, &m_normalizedViewRect);
		m_topLeftPos.x = myPos1.x;
	} else {

		maputils_MapXY2PixelXY(myPos1.x, myPos1.y, &leftEdge, &temp, &m_normalizedViewRect);
		leftEdge -= (tiledmap_Get()->GetZoomTilePixelWidth() + tiledmap_Get()->GetZoomTilePixelWidth()/2);
		m_topLeftPos.x = myPos1.x;
	}

	myPos1 = pos;
	if (myPos1.GetNeighborPosition(NORTHWEST, myPos2)) {
		if (myPos2.GetNeighborPosition(NORTH, myPos1)) {
			maputils_MapXY2PixelXY(myPos1.x, myPos1.y, &temp, &topEdge, &m_normalizedViewRect);
			m_topLeftPos.y = myPos1.y;
		} else {

			maputils_MapXY2PixelXY(myPos2.x, myPos2.y, &temp, &topEdge, &m_normalizedViewRect);
			topEdge -= tiledmap_Get()->GetZoomTilePixelHeight();
			m_topLeftPos.y = myPos2.y;
		}
	} else {

		maputils_MapXY2PixelXY(myPos1.x, myPos1.y, &temp, &topEdge, &m_normalizedViewRect);
		topEdge -= (tiledmap_Get()->GetZoomTilePixelHeight() + tiledmap_Get()->GetZoomTilePixelHeight()/2);
		m_topLeftPos.y = myPos1.y;
	}

	m_leftEdge = leftEdge;
	m_topEdge = topEdge;

	for (i=0; i<k_MAX_WORKERS; i++) {
		if (m_worker[i] != nullptr) {
			delete m_worker[i];
			m_worker[i] = nullptr;
		}
	}
	m_numWorkers = 0;

	tiledmap_Get()->LockThisSurface(m_surface);

	DrawWorkMapThing(m_surface, DrawATile);

	tiledmap_Get()->UnlockSurface();

	tiledmap_Get()->SetZoomLevel(zoomLevel);
	tiledmap_Get()->SetScale( scale );

	tiledmap_Get()->GetMapBounds( m_mapBounds );

	m_mapViewRect.left = 3;
	m_mapViewRect.right = 7;
	m_mapViewRect.top = 2;
	m_mapViewRect.bottom = 9;

return 0;

#if 0   // Unreachable
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

	sint32 nudge;
	sint32 index = 0;

	tiledmap_Get()->LockThisSurface(m_surface);

	for (sint32 y = 0;y < 7;y++) {
		if (y & 0x01) {
			if ( !m_scale ) {
				nudge = k_NUDGE;
			}
			else {
				nudge = k_NUDGE / 2;
			}
			if (!pos.GetNeighborPosition(SOUTHWEST, newpos))
				continue;
		}
		else {
			if (!pos.GetNeighborPosition(SOUTHEAST, newpos))
				continue;

			nudge = 0;
		}
		maputils_MapX2TileX(pos.x,pos.y,&i);

		sint32 x;
		for (x = 0;x < 3;x++) {
			if (x==0 && (y==0 || y==6)) continue;
			if ( !m_scale )
				CalculateWrap(m_surface, pos.y, i+x, x*96+nudge,y*24);
			else
				CalculateWrap(m_surface, pos.y, i+x, x*48+nudge,y*12);


			sint32 mapX = maputils_TileX2MapX(i+x,pos.y);
			MapPoint tempPos( mapX, pos.y);
			Cell *cell = world_Get()->GetCell(tempPos);

			BOOL drawBorder = FALSE;

			if (cell->GetOwner() != m_unit.GetOwner()) {
				if ( !m_scale ) {
					tiledmap_Get()->DrawTileBorder(m_surface, x*96+nudge, y*24, colorset_Get()->GetPlayerColor(cell->GetOwner()));
				}
				else {
					tiledmap_Get()->DrawTileBorderScaled(m_surface, pos, x*48+nudge, y*12,
							tiledmap_Get()->GetZoomTilePixelWidth(),
							tiledmap_Get()->GetZoomTilePixelHeight(),
							colorset_Get()->GetPlayerColor(cell->GetOwner()));
				}

				drawBorder = TRUE;
			}

			delete m_worker[index];
			m_worker[index] = NULL;
			index++;






















		}
		if (y==2 || y==4) {
			if ( !m_scale )
				CalculateWrap(m_surface, pos.y, i+x, x*96+nudge,y*24);
			else
				CalculateWrap(m_surface, pos.y, i+x, x*48+nudge,y*12);

			sint32 mapX = maputils_TileX2MapX(i+x,pos.y);
			MapPoint tempPos (mapX, pos.y);
			Cell *cell = world_Get()->GetCell(tempPos);

			BOOL drawBorder = FALSE;

			if (cell->GetOwner() != m_unit.GetOwner()) {
				if ( !m_scale ) {
					tiledmap_Get()->DrawTileBorder(m_surface, x*96+nudge, y*24, colorset_Get()->GetPlayerColor(cell->GetOwner()));
				}
				else {
					tiledmap_Get()->DrawTileBorderScaled(m_surface, pos, x*48+nudge, y*12,
									tiledmap_Get()->GetZoomTilePixelWidth(),
									tiledmap_Get()->GetZoomTilePixelHeight(),
									colorset_Get()->GetPlayerColor(cell->GetOwner()));
				}
				drawBorder = TRUE;
			}

			delete m_worker[index];
			m_worker[index] = NULL;
			index++;





















		}
		pos = newpos;
	}

	tiledmap_Get()->UnlockSurface();

	tiledmap_Get()->SetZoomLevel(zoomLevel);
	tiledmap_Get()->SetScale( scale );

	return TRUE;
#endif
}

sint32 WorkMap::DrawSpaceImprovements( aui_Surface *pSurface, sint32 xOff, sint32 yOff )
{
	MapPoint pos;
	MapPoint newpos;

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

	sint32 i;

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

	sint32 nudge;
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
		maputils_MapX2TileX(pos.x,pos.y,&i);

		sint32 x;
		for (x = 0;x < 3;x++) {
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

BOOL WorkMap::DrawACity(aui_Surface *pSurface, MapPoint const & pos, void *context)
{
	WorkMap		*workMap = (WorkMap *)context;
	sint32		x, y;
	Unit		city;
	sint32		mapWidth, mapHeight;

	city = world_Get()->GetCell(pos)->GetCity();
	if (city.m_id == 0) return FALSE;

  UnitActorPtr actor = city.GetActor();
	if (!actor) return FALSE;

	tiledmap_Get()->GetMapMetrics(&mapWidth,&mapHeight);

	maputils_MapXY2PixelXY(pos.x, pos.y, &x, &y, workMap->GetNormalizedViewRect());

	x -= workMap->GetLeftEdge();
	y -= workMap->GetTopEdge();

	if (x < 0) x += (mapWidth * tiledmap_Get()->GetZoomTilePixelWidth());
	if (y < 0) y += (mapHeight * tiledmap_Get()->GetZoomTilePixelHeight()/2);

	POINT p = {workMap->X(), workMap->Y()};
	workMap->ToWindow(&p);
	x += p.x;
	y += p.y;






	if ( !workMap->GetScale() ) {
		actor->DrawDirect(pSurface, x, y, tiledmap_Get()->GetZoomScale(k_ZOOM_LARGEST));
	}
	else {
		actor->DrawDirect(pSurface, x, y, tiledmap_Get()->GetZoomScale(k_ZOOM_SMALLEST));
	}

	return TRUE;
}

BOOL WorkMap::DrawALandCity(aui_Surface *pSurface, MapPoint const & pos, void *context)
{
	WorkMap		*workMap = (WorkMap *)context;
	sint32		x, y;
	Unit		city;
	sint32		mapWidth, mapHeight;

	city = world_Get()->GetCell(pos)->GetCity();
	if (city.m_id == 0) return FALSE;

  UnitActorPtr actor = city.GetActor();
	if (!actor) return FALSE;

	tiledmap_Get()->GetMapMetrics(&mapWidth,&mapHeight);

	maputils_MapXY2PixelXY(pos.x, pos.y, &x, &y, workMap->GetNormalizedViewRect());

	x -= workMap->GetLeftEdge();
	y -= workMap->GetTopEdge();

	if (x < 0) x += (mapWidth * tiledmap_Get()->GetZoomTilePixelWidth());
	if (y < 0) y += (mapHeight * tiledmap_Get()->GetZoomTilePixelHeight()/2);

	POINT p = {workMap->X(), workMap->Y()};
	workMap->ToWindow(&p);
	x += p.x;
	y += p.y;






	if ( !workMap->GetScale() ) {
		actor->DrawDirect(pSurface, x, y, tiledmap_Get()->GetZoomScale(k_ZOOM_LARGEST));
	}
	else {
		actor->DrawDirect(pSurface, x, y, tiledmap_Get()->GetZoomScale(k_ZOOM_SMALLEST));
	}

	return TRUE;
}

BOOL WorkMap::DrawAGood(aui_Surface *pSurface, MapPoint const &pos, void *context)
{
	WorkMap		*workMap = (WorkMap *)context;
	GoodActor	*goodActor;
	sint32		mapWidth, mapHeight;
	sint32		x, y;

	TileInfo *curTileInfo = tiledmap_Get()->GetTileInfo(pos);
	Assert(curTileInfo != nullptr);
	if(!curTileInfo || !curTileInfo->HasGoodActor()) return FALSE;

	goodActor = curTileInfo->GetGoodActor();

	if (!goodActor) return FALSE;

	tiledmap_Get()->GetMapMetrics(&mapWidth,&mapHeight);

	maputils_MapXY2PixelXY(pos.x, pos.y, &x, &y, workMap->GetNormalizedViewRect());

	x -= workMap->GetLeftEdge();
	y -= workMap->GetTopEdge();

	if (x < 0) x += (mapWidth * tiledmap_Get()->GetZoomTilePixelWidth());
	if (y < 0) y += (mapHeight * tiledmap_Get()->GetZoomTilePixelHeight()/2);

	POINT p = {workMap->X(), workMap->Y()};
	workMap->ToWindow(&p);
	x += p.x;
	y += p.y;





	if ( !workMap->GetScale() ) {
		goodActor->DrawDirect(pSurface, x, y, tiledmap_Get()->GetZoomScale(k_ZOOM_LARGEST));
	}
	else {
		goodActor->DrawDirect(pSurface, x, y, tiledmap_Get()->GetZoomScale(k_ZOOM_SMALLEST));
	}

	return TRUE;
}

BOOL WorkMap::DrawATile(aui_Surface *pSurface, MapPoint const & pos, void *context)
{
	WorkMap		*workMap = (WorkMap *)context;
	sint32		x, y;
	sint32		mapWidth, mapHeight;

	tiledmap_Get()->GetMapMetrics(&mapWidth,&mapHeight);

	maputils_MapXY2PixelXY(pos.x, pos.y, &x, &y, workMap->GetNormalizedViewRect());

	x -= workMap->GetLeftEdge();
	y -= workMap->GetTopEdge();

	if (x < 0) x += (mapWidth * tiledmap_Get()->GetZoomTilePixelWidth());
	if (y < 0) y += (mapHeight * tiledmap_Get()->GetZoomTilePixelHeight()/2);




























	sint32 tileX;
	maputils_MapX2TileX(pos.x, pos.y, &tileX);

	workMap->CalculateWrap(pSurface, pos.y, tileX, x, y);

	Cell	*cell = world_Get()->GetCell(pos);

	if (cell) {
		Unit	c;

		workMap->GetOwningCity(c);

		if (cell->GetOwner() != c.GetOwner()) {
			if ( !workMap->GetScale() ) {
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

BOOL WorkMap::DrawWorkMapThing(aui_Surface *pSurface, WorkMapDrawFunc *func)
{
	MapPoint	pos = m_unit.RetPos();
	MapPoint	wpos;




	sint32 zoomLevel = tiledmap_Get()->GetZoomLevel();

	if ( m_scale ) tiledmap_Get()->SetZoomLevel(k_ZOOM_SMALLEST );
	else tiledmap_Get()->SetZoomLevel(k_ZOOM_NORMAL );

	SquareIterator it(pos, 1);
	for(it.Start(); !it.End(); it.Next()) {
		func(pSurface, it.Pos(), (void *)this);
	}


	tiledmap_Get()->SetZoomLevel(zoomLevel);

	return TRUE;
}







BOOL WorkMap::DrawSprites(aui_Surface *pSurface, RECT *destRect)
{
	Assert(pSurface);
	if (!pSurface) return FALSE;

	if (!m_unit) return FALSE;

	if(!unitpool_Get()->IsValid(m_unit)) return FALSE;

	MapPoint pos;
	sint32 i;

	m_unit.GetData()->GetPos(pos);

	for (i = 0;i < k_MAX_WORKERS;i++) {
		if (m_worker[i]) {
			m_worker[i]->Process();

			if ( !m_scale ) {
				m_worker[i]->DrawDirect(pSurface, m_worker[i]->GetX()+destRect->left, m_worker[i]->GetY()+destRect->top, tiledmap_Get()->GetZoomScale(k_ZOOM_LARGEST));
			}
			else {
				m_worker[i]->DrawDirect(pSurface, m_worker[i]->GetX()+destRect->left, m_worker[i]->GetY()+destRect->top, tiledmap_Get()->GetZoomScale(k_ZOOM_SMALLEST));
			}
		}
	}


	DrawWorkMapThing(pSurface, DrawAGood);

	DrawWorkMapThing(pSurface, DrawACity);

	m_totalFood = m_totalProd = m_totalGold = 0;

	Cell *cell = world_Get()->GetCell( pos );
	m_totalFood += cell->GetFoodProduced();
	m_totalProd += cell->GetShieldsProduced();
	m_totalGold += 0;

	for (i = 0;i < k_MAX_WORKERS;i++) {
		if (m_worker[i]) {

			Assert(FALSE);
		}
	}












	return TRUE;
}







sint32 WorkMap::CalculateWrap(
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

	if ( !m_scale ) {

		tiledmap_Get()->DrawTransitionTile(nullptr, pos, x, y);


		tiledmap_Get()->DrawOverlay(nullptr, baseTile->GetHatData(), x, y);

		if (river != -1)

			tiledmap_Get()->DrawOverlay(nullptr, tiledmap_Get()->GetTileSet()->GetRiverData(river), x, y);

		tiledmap_Get()->DrawImprovementsLayer(nullptr, pos, x, y);

	}
	else {

		tiledmap_Get()->DrawTransitionTileScaled(nullptr, pos, x, y, tiledmap_Get()->GetZoomTilePixelWidth(), tiledmap_Get()->GetZoomTilePixelHeight() );

		tiledmap_Get()->DrawScaledOverlay(nullptr, baseTile->GetHatData(), x, y,
										tiledmap_Get()->GetZoomTilePixelWidth(),
										tiledmap_Get()->GetZoomTileGridHeight());

		if ( river != -1 )
			tiledmap_Get()->DrawScaledOverlay(nullptr, tiledmap_Get()->GetTileSet()->GetRiverData(river), x, y,
											tiledmap_Get()->GetZoomTilePixelWidth(),
											tiledmap_Get()->GetZoomTileGridHeight());

		tiledmap_Get()->DrawImprovementsLayer(nullptr, pos, x, y);
	}

	return 0;
}

sint32 WorkMap::DrawImprovements(
			aui_Surface *surface,
			sint32 i,
			sint32 j,
			sint32 x,
			sint32 y
			)
{
	maputils_WrapPoint(j, i, &j, &i);
	MapPoint    pos(maputils_TileX2MapX(j, i), i);
	tiledmap_Get()->DrawImprovementsLayer(nullptr, pos, x, y);

	return 0;
}

void WorkMap::DrawCityName(aui_Surface *surface, sint32 x, sint32 y, const Unit &unit)
{
	Assert(unit.IsCity());
	if (!unit.IsCity()) return;

	sint32 const    yoffset     = y - k_TILE_PIXEL_HEADROOM;
	CityData *      cityData    = unit.GetData()->GetCityData();
	MBCHAR *        name        = cityData->GetName();

	if ((x >= 0) && (x < surface->Width()) &&
        (yoffset >= 0) && (yoffset < surface->Height())
       )
    {
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

	sint32 const pop = cityData->PopCount();
	MBCHAR str[80];
	snprintf(str, sizeof(str),"%i",pop);

    sint32  popEdgeSize = std::max<sint32>(k_POP_BOX_SIZE_MINIMUM, k_POP_BOX_SIZE);
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

void WorkMap::DrawALabel( aui_Surface *surface, MBCHAR *label, sint32 x, sint32 y, sint32 width, sint32 height )
{
	RECT rect = {0, 0, width, height};

	OffsetRect(&rect, X() + Width() - x, Y() + y );
	ToWindow( &rect );

	if (rect.left < 0)
		rect.left = 0;

	if (rect.top < 0)
		rect.top = 0;

	if (rect.right >= surface->Width()) {
		rect.right = surface->Width() - 1;
	}

	if (rect.bottom >= surface->Height())  {
		rect.bottom = surface->Height() - 1;
	}

	tiledmap_Get()->GetFont()->DrawString(surface, &rect, &rect, label, 0,
		colorset_Get()->GetColorRef(COLOR_WHITE), 0);
}

void WorkMap::DrawLabels( aui_Surface *surface )
{
	sint32 foodWidth, foodHeight;
	sint32 prodWidth, prodHeight;
	sint32 goldWidth, goldHeight;

	MBCHAR foodStr[_MAX_PATH];
	MBCHAR prodStr[_MAX_PATH];
	MBCHAR goldStr[_MAX_PATH];

	if ( m_totalFood < 0 || m_totalProd < 0 || m_totalGold < 0 ) return;

	snprintf(foodStr, sizeof(foodStr), "%s %d", m_string->GetString(WM_FOOD), m_totalFood );
	snprintf(prodStr, sizeof(prodStr), "%s %d", m_string->GetString(WM_PROD), m_totalProd );
	snprintf(goldStr, sizeof(goldStr), "%s %d", m_string->GetString(WM_GOLD), m_totalGold );

	if (tiledmap_Get() && tiledmap_Get()->GetFont()) {
		foodWidth = tiledmap_Get()->GetFont()->GetStringWidth(foodStr);
		foodHeight = tiledmap_Get()->GetFont()->GetMaxHeight();
		prodWidth = tiledmap_Get()->GetFont()->GetStringWidth(prodStr);
		prodHeight = tiledmap_Get()->GetFont()->GetMaxHeight();
		goldWidth = tiledmap_Get()->GetFont()->GetStringWidth(goldStr);
		goldHeight = tiledmap_Get()->GetFont()->GetMaxHeight();

		sint32 offsetWidth = foodWidth, offsetHeight;

		if ( prodWidth > offsetWidth )
			offsetWidth = prodWidth;
		if ( goldWidth > offsetWidth )
			offsetWidth = prodWidth;





		offsetHeight = 2;
		DrawALabel( surface, foodStr, k_OFFSET_WIDTH, offsetHeight, foodWidth, foodHeight );

		offsetHeight += foodHeight;
		DrawALabel( surface, prodStr, k_OFFSET_WIDTH, offsetHeight, prodWidth, prodHeight );

		offsetHeight += prodHeight;
		DrawALabel( surface, goldStr, k_OFFSET_WIDTH, offsetHeight, goldWidth, goldHeight );

		MBCHAR tiStr[_MAX_PATH];
		strncpy( tiStr, stringdb_Get()->GetNameStr("str_ldl_TILE_RESOURCES_Colon"), sizeof(tiStr) - 1 );
		tiStr[sizeof(tiStr) - 1] = '\0';
		sint32 tiLabelWidth = tiledmap_Get()->GetFont()->GetStringWidth(tiStr);
		RECT rect = {0, 0, tiLabelWidth, tiledmap_Get()->GetFont()->GetMaxHeight()};

		OffsetRect(&rect, X() + 2, Y() + 2 );
		ToWindow( &rect );

		tiledmap_Get()->GetFont()->DrawString(surface, &rect, &rect, tiStr, 0,
			colorset_Get()->GetColorRef(COLOR_WHITE), 0);

	}
}

void WorkMap::CenterNumber( RECT *rect, sint32 &x, sint32 &y, sint32 width, sint32 height )
{
	sint32 rectWidth = rect->right - rect->left;
	sint32 rectHeight = rect->bottom - rect->top;

	Assert( rectWidth >= width );
	Assert( rectHeight >= height );

	x = rect->left + (( rectWidth - width ) >> 1);
	y = rect->top + (( rectHeight - height ) >> 1);
}

void WorkMap::DrawResourceIcons(aui_Surface *surface, sint32 x, sint32 y, MapPoint &pos)
{
	RECT		iconRect;
	TileSet		*tileSet = tiledmap_Get()->GetTileSet();
	MBCHAR		str[80];
	sint32		width, height;

	sint32		xcenter, ycenter;

	sint32 prod, food, gold;
	Cell *cell = world_Get()->GetCell(pos);

	prod = cell->GetShieldsProduced();
	food = cell->GetFoodProduced();




	gold = 0;

	m_totalFood += food;
	m_totalProd += prod;
	m_totalGold += gold;

	POINT iconDim = tileSet->GetMapIconDimensions(MAPICON_RESOURCE1);
	Pixel16 color = colorset_Get()->GetPlayerColor(m_unit.GetOwner());

	sint32	popEdgeSize = k_POP_BOX_SIZE;
	iconRect.left = x + (popEdgeSize*2);
	iconRect.top = y;
	iconRect.right = iconRect.left + iconDim.x + 1;
	iconRect.bottom = iconRect.top + iconDim.y + 1;

	if (iconRect.left < 0 || iconRect.top < 0 || iconRect.right >= surface->Width() ||
		iconRect.bottom >= surface->Height())
		return;

	Pixel16 *resourceIcon;

	resourceIcon = tileSet->GetMapIconData(MAPICON_RESOURCE1);
	Assert(resourceIcon); if (!resourceIcon) return;
	tiledmap_Get()->DrawColorizedOverlay(resourceIcon, surface, iconRect.left, iconRect.top, color);

	snprintf(str, sizeof(str), "%ld", prod);

	if (tiledmap_Get() && tiledmap_Get()->GetFont()) {
		width = tiledmap_Get()->GetFont()->GetStringWidth(str);
		height = tiledmap_Get()->GetFont()->GetMaxHeight();

		RECT		rect = {0, 0, width, height};

		CenterNumber( &iconRect, xcenter, ycenter, width, height );
		OffsetRect( &rect, xcenter, ycenter );

		tiledmap_Get()->GetFont()->DrawString(surface, &rect, &rect, str, 0,
			colorset_Get()->GetColorRef(COLOR_WHITE), 0);

		OffsetRect(&rect, -1, -1);

		tiledmap_Get()->GetFont()->DrawString(surface, &rect, &rect, str, 0,
			colorset_Get()->GetColorRef(COLOR_BLACK), 0);
	}

	iconRect.left += iconDim.x;
	iconRect.right += iconDim.x;

	iconDim = tileSet->GetMapIconDimensions(MAPICON_RESOURCE2);
	iconRect.bottom = iconRect.top + iconDim.y + 1;

	if (iconRect.left < 0 || iconRect.top < 0 || iconRect.right >= surface->Width() ||
		iconRect.bottom >= surface->Height())
		return;

	resourceIcon = tileSet->GetMapIconData(MAPICON_RESOURCE2);
	Assert(resourceIcon); if (!resourceIcon) return;
	tiledmap_Get()->DrawColorizedOverlay(resourceIcon, surface, iconRect.left, iconRect.top, color);

	snprintf(str, sizeof(str), "%ld", food);

	if (tiledmap_Get() && tiledmap_Get()->GetFont()) {
		width = tiledmap_Get()->GetFont()->GetStringWidth(str);
		height = tiledmap_Get()->GetFont()->GetMaxHeight();

		RECT		rect = {0, 0, width, height};

		CenterNumber( &iconRect, xcenter, ycenter, width, height );
		OffsetRect( &rect, xcenter, ycenter );

		tiledmap_Get()->GetFont()->DrawString(surface, &rect, &rect, str, 0,
			colorset_Get()->GetColorRef(COLOR_WHITE), 0);

		OffsetRect(&rect, -1, -1);

		tiledmap_Get()->GetFont()->DrawString(surface, &rect, &rect, str, 0,
			colorset_Get()->GetColorRef(COLOR_BLACK), 0);
	}

	iconDim = tileSet->GetMapIconDimensions(MAPICON_RESOURCE1);

	iconRect.left = x + (popEdgeSize*2);
	iconRect.top = y + iconDim.y;

	iconDim = tileSet->GetMapIconDimensions(MAPICON_RESOURCE3);

	iconRect.right = iconRect.left + iconDim.x + 1;
	iconRect.bottom = iconRect.top + iconDim.y + 1;

	resourceIcon = tileSet->GetMapIconData( MAPICON_RESOURCE3 );
	Assert( resourceIcon ); if ( !resourceIcon ) return;
	tiledmap_Get()->DrawColorizedOverlay( resourceIcon, surface, iconRect.left, iconRect.top, color );

	snprintf(str, sizeof(str), "%d", gold );

	if (tiledmap_Get() && tiledmap_Get()->GetFont()) {
		width = tiledmap_Get()->GetFont()->GetStringWidth(str);
		height = tiledmap_Get()->GetFont()->GetMaxHeight();

		RECT		rect = {0, 0, width, height};

		CenterNumber( &iconRect, xcenter, ycenter, width, height );
		OffsetRect( &rect, xcenter, ycenter );

		tiledmap_Get()->GetFont()->DrawString(surface, &rect, &rect, str, 0,
			colorset_Get()->GetColorRef(COLOR_WHITE), 0);

		OffsetRect(&rect, -1, -1);

		tiledmap_Get()->GetFont()->DrawString(surface, &rect, &rect, str, 0,
			colorset_Get()->GetColorRef(COLOR_BLACK), 0);
	}
}

sint32 WorkMap::UpdateFromSurface(aui_Surface *destSurface, RECT *destRect)
{
	RECT rect = {0,0,m_surface->Width(),m_surface->Height()};
	c3ui_Get()->TheBlitter()->Blt(destSurface, destRect->left, destRect->top, m_surface, &rect, k_AUI_BLITTER_FLAG_COPY);
	return 0;
}

void WorkMap::Click(aui_MouseEvent *data)
{
	MapPoint		pos;
	POINT			point = data->position;

	if (MousePointToTilePos(point, pos)) {
		if (data->lbutton && !data->rbutton) {

			HandlePop(pos);
		} else {
			if (data->rbutton && !data->lbutton) {

			} else {

			}
		}
	}
}

BOOL WorkMap::PointInMask(POINT hitPt)
{
	TILEHITMASK *	thm			= tiledmap_Get()->GetTileHitMask();
	double const	scale		= (m_scale) ? 0.5 : 1.0;
	sint32 const	x			= (sint32)((double)hitPt.x / scale);
	sint32 const 	y			= (sint32)(((double)hitPt.y / scale) + k_TILE_PIXEL_HEADROOM);

	return (x >= thm[y].start) && (x <= thm[y].end);
}

BOOL WorkMap::MousePointToTilePos(POINT point, MapPoint &tilePos)
{

	double const	scale		= (m_scale) ? 0.5 : 1.0;
	sint32			headroom	= static_cast<sint32>(k_TILE_PIXEL_HEADROOM * scale);

	sint32			width		= static_cast<sint32>(k_TILE_GRID_WIDTH * scale);
	sint32			height		= static_cast<sint32>
		((k_TILE_GRID_HEIGHT-k_TILE_PIXEL_HEADROOM) * scale);

	sint32			x			= point.x;
	sint32			y			= point.y;

	if (!(m_mapViewRect.top & 1))
		y -= headroom;

	MapPoint		pos	((x / width) + m_mapViewRect.left,
						 (y / height) + m_mapViewRect.top / 2
						);

	POINT			hitPt;
	hitPt.x = x % width;
	hitPt.y = y % height;

	sint32			maxX		= m_mapBounds.right;

	if (!PointInMask(hitPt)) {

		pos.x = (sint16)((x + (width/2)) / width - 1 + m_mapViewRect.left);
		pos.y = (sint16)(((y + (height)/2) / height - 1) + m_mapViewRect.top/2);

		hitPt.x = (x + (width/2)) % width;
		hitPt.y = (y + (height)/2) % height;

		if (!PointInMask(hitPt))
			return FALSE;
		else {
			if (pos.x >= pos.y) {
				tilePos.x = pos.x - pos.y;
				tilePos.y = pos.y * 2 + 1;
			} else {
				tilePos.x = static_cast<sint16>(maxX + pos.x - pos.y);
				tilePos.y = pos.y * 2 + 1;
			}
		}
	} else {
		if (pos.x >= pos.y) {
			tilePos.x = pos.x - pos.y;
			tilePos.y = pos.y * 2;
		} else {
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
			tilePos.y = sy + tilePos.y;
			tilePos.x = (tilePos.x + (sx - (sy/2))) % sx;
		} else if (world_Get()->GetHeight() <= tilePos.y) {
			sx = (sint16)world_Get()->GetWidth();
			sy = (sint16)world_Get()->GetHeight();
			tilePos.y = tilePos.y - sy;
			tilePos.x = (tilePos.x - (sx - (sy/2))) % sx;
		}
	} else {

		if (tilePos.y <0) {
			tilePos.y = 0;
			return FALSE;
		} else if (world_Get()->GetHeight() <= tilePos.y) {
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

	if (m_unit.m_id)
	{
		MapPoint tempPos;
		m_unit.GetData()->GetPos(tempPos);
	}

	return TRUE;
}

void WorkMap::SetHiliteMouseTile(MapPoint &pos)
{
	m_hiliteMouseTile = pos;
}

void WorkMap::DrawHiliteMouseTile(aui_Surface *destSurf, RECT *destRect)
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

void WorkMap::HandlePop( MapPoint point )
{
/// @todo Find out what this function is supposed to do, because it is now
///       only updating local variables.

	MapPoint mp;

	m_unit.GetData()->GetPos(mp);

	sint32 diffX, diffY;
	diffY = 5 - point.y;
	diffX = 2 - point.x;

	sint32 x = mp.x - diffX;
	sint32 y = mp.y - diffY;


	sint32 xx, yy;
	maputils_WrapPoint( x, y, &xx, &yy);
	point.x = (sint16)xx;
	point.y = (sint16)yy;


	Cell *cell;
	cell = world_Get()->GetCell(point);

#if 0   // Unreachable
	PLAYER_INDEX	player ;
	ID	item ;
	SELECT_TYPE	state ;

	selitem_Get()->GetTopCurItem(player, item, state);
	Assert(player == selitem_Get()->GetVisiblePlayer());
	if(player != selitem_Get()->GetVisiblePlayer())
		return;

	Assert(m_unit != Unit());
#endif
}




AUI_ERRCODE WorkMap::Idle( )
{
	static uint32 lastDraw = 0;
	if (GetTickCount() - lastDraw > 100) lastDraw = GetTickCount();
	else return AUI_ERRCODE_OK;

	DrawThis(nullptr, 0, 0);

	return AUI_ERRCODE_OK;
}
