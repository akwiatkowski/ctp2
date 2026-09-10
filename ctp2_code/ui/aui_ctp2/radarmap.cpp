//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : User interface radar map
// Id           : $Id$
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
// - None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - #01 Allow shifing the X and Y axis in the radar map with RMouse clicks
//   (L. Hirth 6/2004)
// - Standardised ceil/min/max usage.
// - Radar tile boarder color determined by the visual cell owner instead by
//   the actual cell owner. - Nov 1st 2004 - Martin G�hmann
// - Radar tile boarder is now fully determined by the visible tile onwer
//   instead of being determined half by the actual tile owner and half by the
//   the the visible tile owner this fixes the bug that appears after conquest
//   of a city. - Nov. 1st 2004 - Martin G�hmann
// - The radar map now shows the current terrain and the current units and
//   cities if fog of war is off, otherwise it only displays the kind of
//   information it should display. - Dec. 25th 2004 - Martin G�hmann
// - Borders on the minimap are now shown if fog of war is off or god mode
//   is on, even if the there is no contact to that civilisation.
//   - Mar. 4th 2005 Martin G�hmann
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
// - Added political map functionality (6-Jul-2009 EPW)
// - Added View capitol on minimap (5-Jan-10 EPW)
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gfx/gfx_utils/colorset.h"               // colorset_Get()
#include "ui/aui_ctp2/radarmap.h"

#include <algorithm>

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_Factory.h"
#include "ui/aui_common/aui_blitter.h"
#include "ui/aui_common/aui_window.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_action.h"
#include "ui/aui_ctp2/c3ui.h"
#include "gs/gameobj/Player.h"                 // Player, player_Get
#include "gs/world/World.h"                  // world_Get()
#include "gs/world/Cell.h"
#include "gs/world/UnseenCell.h"
#include "gs/gameobj/citydata.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/UnitData.h"
#include "gfx/gfx_utils/pixelutils.h"
#include "ui/aui_ctp2/SelItem.h"                // selitem_Get()
#include "gfx/tilesys/tiledmap.h"               // tiledmap_Get()
#include "gfx/spritesys/director.h"
#include "gfx/tilesys/maputils.h"
#include "ui/aui_utils/primitives.h"
#include "gs/database/profileDB.h"              // profiledb_Get()
#include "ctp/ctp2_utils/pointerlist.h"
#include "gs/gameobj/terrainutil.h"
#include "ai/strategy/scheduler/Scheduler.h"

extern PointerList<Player> *g_deadPlayer;

extern sint32 g_fog_toggle;
extern sint32 g_god;

static const unsigned char k_EAST_BORDER_FLAG		= 0x01;
static const unsigned char k_WEST_BORDER_FLAG		= 0x02;
static const unsigned char k_NORTH_EAST_BORDER_FLAG	= 0x04;
static const unsigned char k_NORTH_WEST_BORDER_FLAG	= 0x08;
static const unsigned char k_SOUTH_EAST_BORDER_FLAG	= 0x10;
static const unsigned char k_SOUTH_WEST_BORDER_FLAG	= 0x20;

//---------------------------------------------------------------------------
//
//	RadarMap::RadarMap
//
//---------------------------------------------------------------------------
RadarMap::RadarMap(AUI_ERRCODE *retval,
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

//---------------------------------------------------------------------------
//
//	RadarMap::RadarMap
//
//---------------------------------------------------------------------------
RadarMap::RadarMap(AUI_ERRCODE *retval,
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
	InitCommon();
}

//---------------------------------------------------------------------------
//
//	RadarMap::~RadarMap
//
//---------------------------------------------------------------------------
RadarMap::~RadarMap()
{
	delete m_mapSurface;
	delete m_tempSurface;
}

//---------------------------------------------------------------------------
//
//	RadarMap::InitCommonLdl
//
//---------------------------------------------------------------------------
void RadarMap::InitCommonLdl(MBCHAR *ldlBlock)
{
    ldl_datablock * block = aui_Ldl::FindDataBlock(ldlBlock);
	Assert( block != nullptr );
	if ( !block ) return;

	InitCommon();
}

//---------------------------------------------------------------------------
//
//	RadarMap::InitCommon
//
//---------------------------------------------------------------------------
void RadarMap::InitCommon()
{
	m_mapSurface = nullptr;
	m_mapSize = nullptr;
	m_tempSurface = nullptr;
	m_tempBuffer.clear();

	m_tilePixelWidth = 0.0;
	m_tilePixelHeight = 0.0;

	m_displayUnits = profiledb_Get()->GetDisplayUnits() != FALSE;
	m_displayCities = profiledb_Get()->GetDisplayCities() != FALSE;
	m_displayBorders = profiledb_Get()->GetDisplayBorders() != FALSE;
	m_displayOverlay = true;
	m_filter = profiledb_Get()->GetDisplayFilter() != FALSE;
	m_displayTrade = profiledb_Get()->GetDisplayTrade() != FALSE;
	m_displayTerrain = profiledb_Get()->GetDisplayTerrain() != FALSE;
	m_displayPolitical = profiledb_Get()->GetDisplayPolitical() != FALSE;
	m_displayCapitols = profiledb_Get()->GetDisplayCapitols() != FALSE;
	m_displayRelations = profiledb_Get()->GetDisplayRelations() != FALSE;

	// m_mapOverlay default constructed (empty)

	MapPoint resetPos (0,0);
	m_lastCenteredPoint = resetPos;

	m_isInteractive = true;

	m_selectedCity.m_id = 0;

	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	m_mapSurface = aui_Factory::new_Surface(errcode, m_width, m_height);
	Assert(AUI_NEWOK(m_mapSurface, errcode));

	RECT rect = { 0, 0, m_width, m_height };

	if ( m_pattern ) {
		m_pattern->Draw( m_mapSurface, &rect );
	}

	if ( world_Get() ) {

		CalculateMetrics();

		RenderMap(m_mapSurface);
	}
}

//---------------------------------------------------------------------------
//
//	RadarMap::ClearMapOverlay
//
//---------------------------------------------------------------------------
void RadarMap::ClearMapOverlay()
{
	m_mapOverlay.clear();
}

//---------------------------------------------------------------------------
//
//	RadarMap::SetMapOverlayCell
//
//---------------------------------------------------------------------------
//	- Sets a position for the Gaia controller overlay and creates the over-
//    lay it does not exists yet
//
//---------------------------------------------------------------------------
void RadarMap::SetMapOverlayCell(MapPoint const & pos, COLOR color)
{
	if (m_mapOverlay.empty()) {
		sint32 len = m_mapSize->x * m_mapSize->y;
		m_mapOverlay.resize(len, COLOR_MAX);
	}

	m_mapOverlay[pos.x + (pos.y * m_mapSize->x)] = color;
}

//---------------------------------------------------------------------------
//
//	RadarMap::Resize
//
//---------------------------------------------------------------------------
//	- resize the radar map
//
//---------------------------------------------------------------------------
AUI_ERRCODE	RadarMap::Resize( sint32 width, sint32 height )
{
	AUI_ERRCODE		errcode = aui_Region::Resize(width, height);
	Assert(errcode == AUI_ERRCODE_OK);

	delete m_mapSurface;
	m_mapSurface = aui_Factory::new_Surface(errcode, width, height);
	Assert( AUI_NEWOK(m_mapSurface, errcode) );

	CalculateMetrics();

	RenderMap(m_mapSurface);

	return errcode;
}

//---------------------------------------------------------------------------
//
//	RadarMap::CalculateMetrics
//
//---------------------------------------------------------------------------
//	- calculate some values depending on the current radar map size
//
//---------------------------------------------------------------------------
void RadarMap::CalculateMetrics()
{
	if (!world_Get()) return;

	delete m_tempSurface;

	m_mapSize = world_Get()->GetSize();

	m_tilePixelWidth = ((double )m_width) / m_mapSize->x;
	m_tilePixelHeight = ((double )m_height) / m_mapSize->y;

	uint32 width = m_mapSize->x * 2;
	uint32 height = m_mapSize->y;

	// Zeroed on every resize (as the old calloc was); the surface borrows a
	// pointer past the 1-pixel guard row/column.
	m_tempBuffer.assign(static_cast<size_t>(width + 2) * (height + 2) * 2, 0);

	AUI_ERRCODE err;
	m_tempSurface = new aui_Surface(&err, width, height, 16, 2*(width + 2), &m_tempBuffer[2*((width + 2) + (1))]);
}

//---------------------------------------------------------------------------
//
//	RadarMap::MapToPixel
//
//---------------------------------------------------------------------------
//	- Seems unused
//
//---------------------------------------------------------------------------
POINT RadarMap::MapToPixel(sint32 x, sint32 y)
{
	sint32		k       = ((y / 2) + x) % m_mapSize->x;

    POINT		pt;
	pt.x = (sint32)(k * m_tilePixelWidth);
	pt.y = (sint32)(y * m_tilePixelHeight);

	return pt;
}

//---------------------------------------------------------------------------
//
//	RadarMap::MapToPixel
//
//---------------------------------------------------------------------------
//	- Seems unused because calling function is unused
//
//---------------------------------------------------------------------------
POINT RadarMap::MapToPixel(MapPoint *pos)
{
	return MapToPixel(pos->x, pos->y);
}




//---------------------------------------------------------------------------
//
//	RadarMap::GetVisiblePlayerToRender
//
//---------------------------------------------------------------------------
//	- Gives back the player that shall be shown currently
//
//---------------------------------------------------------------------------
Player *RadarMap::GetVisiblePlayerToRender()
{


	if(!tiledmap_Get() || !tiledmap_Get()->ReadyToDraw() ||
		!world_Get() || !selitem_Get() || !m_mapSize)
		return(nullptr);






	Assert(m_mapSize->x < 0 || m_mapSize->y > 0);
	if(m_mapSize->x <= 0 || m_mapSize->y <= 0)
		return(nullptr);

	return(player_Get(selitem_Get()->GetVisiblePlayer()));
}

//---------------------------------------------------------------------------
//
//	RadarMap::RadarTileColor
//
//---------------------------------------------------------------------------
//	- Determines the color that has to be set for a position of the
//    RadarMap
//
//---------------------------------------------------------------------------
Pixel16 RadarMap::RadarTileColor(const Player *player, const MapPoint &position,
								 const MapPoint &worldpos, uint32 &flags)
{
	Unit unit;

	flags = 0;

	if(player->IsExplored(worldpos))
	{
		sint32 owner = world_Get()->GetOwner(worldpos);

		if(m_displayTrade && world_Get()->GetCell(worldpos)->GetNumTradeRoutes() > 0)
		{
			flags = 1;
		}

		if(m_displayOverlay && !m_mapOverlay.empty())
		{
			COLOR color = m_mapOverlay[worldpos.y * m_mapSize->x + worldpos.x];
			if(color != COLOR_MAX)
				return(colorset_Get()->GetColor(color));
		}

		if(m_displayCities && tiledmap_Get()->HasVisibleCity(worldpos))
		{
			return(colorset_Get()->GetColor(COLOR_WHITE));
		}

		if(m_displayUnits && (world_Get()->GetTopVisibleUnit(worldpos, unit) || world_Get()->GetTopRadarUnit(worldpos, unit)))
		{
			if(m_displayRelations)
			{
				if(m_displayPolitical && unit.GetOwner() == owner)
					return RadarTileRelationsDarkColor(worldpos, player, unit.GetOwner());
				else
					return RadarTileRelationsColor(worldpos, player, unit.GetOwner());
			}
			else
			{
				if(m_displayPolitical && unit.GetOwner() == owner)
					return colorset_Get()->GetDarkPlayerColor(unit.GetOwner());
				else
					return colorset_Get()->GetPlayerColor(unit.GetOwner());
			}
		}

		if(m_displayPolitical && owner >= 0 && !world_Get()->IsWater(worldpos) )
		{
			if(m_displayRelations)
				return RadarTileRelationsColor(worldpos, player);
			else
				return colorset_Get()->GetPlayerColor(tiledmap_Get()->GetVisibleCellOwner(worldpos));
		}

		if(m_displayTerrain)
		{
			return(colorset_Get()->GetColor(static_cast<COLOR>(COLOR_TERRAIN_0 + tiledmap_Get()->GetVisibleTerrainType(worldpos))));
		}
		else
		{
			if(world_Get()->IsLand(worldpos) || world_Get()->IsMountain(worldpos))
			{
				return colorset_Get()->GetColor(static_cast<COLOR>(COLOR_TERRAIN_0 +
														   TERRAIN_GRASSLAND));
			}
			else
			{
				return colorset_Get()->GetColor(static_cast<COLOR>(COLOR_TERRAIN_0 +
														   TERRAIN_WATER_DEEP));
			}
		}
	}

	if(world_Get()->GetTopRadarUnit(worldpos, unit))
		return(colorset_Get()->GetPlayerColor(unit.GetOwner()));

	return(colorset_Get()->GetColor(COLOR_BLACK));
}

//---------------------------------------------------------------------------
//
//	RadarMap::RadarTileBorderColor
//
//---------------------------------------------------------------------------
//	- Checks which color a border must be drawn for the current tile
//
//---------------------------------------------------------------------------
Pixel16 RadarMap::RadarTileBorderColor(const MapPoint &position, const Player *player)
{
	sint32 owner = tiledmap_Get()->GetVisibleCellOwner(position);
	if(owner < 0)
		return(colorset_Get()->GetColor(COLOR_BLACK));

	if(m_displayRelations)
		return RadarTileRelationsColor(position, player);
	else
		return(colorset_Get()->GetPlayerColor(owner));
}

//---------------------------------------------------------------------------
//
//	RadarMap::RadarTileBorderColor
//
//---------------------------------------------------------------------------
//	- Checks which color a border must be drawn for the current tile
//
//---------------------------------------------------------------------------
Pixel16 RadarMap::RadarTileRelationsColor(const MapPoint &position, const Player *player, sint32 unitOwner)
{
	Assert(m_displayRelations);

	sint32 owner = unitOwner < 0 ? tiledmap_Get()->GetVisibleCellOwner(position) : unitOwner;
	if(owner < 0)
		return(colorset_Get()->GetColor(COLOR_WHITE));
	else if(player->m_owner == owner || player->HasAllianceWith(owner))
		return(colorset_Get()->GetColor(COLOR_BLUE));
	else if(player->HasWarWith(owner))
		return(colorset_Get()->GetColor(COLOR_RED));
	else if(player->HasPeaceTreatyWith(owner) || player->HasAnyPactWith(owner))
		return(colorset_Get()->GetColor(COLOR_GREEN));
	else if(!player->HasContactWith(owner))
		return(colorset_Get()->GetColor(COLOR_WHITE));
	else
		return(colorset_Get()->GetColor(COLOR_YELLOW));
}

//---------------------------------------------------------------------------
//
//	RadarMap::RadarTileBorderDarkColor
//
//---------------------------------------------------------------------------
//	- Dark alternative to RadarTileBorderColor
//
//---------------------------------------------------------------------------
Pixel16 RadarMap::RadarTileRelationsDarkColor(const MapPoint &position, const Player *player, sint32 unitOwner)
{
	Assert(m_displayRelations);

	sint32 owner = unitOwner < 0 ? tiledmap_Get()->GetVisibleCellOwner(position) : unitOwner;
	if(owner < 0)
		return(colorset_Get()->GetDarkColor(COLOR_WHITE));
	else if(player->m_owner == owner || player->HasAllianceWith(owner))
		return(colorset_Get()->GetDarkColor(COLOR_BLUE));
	else if(player->HasWarWith(owner))
		return(colorset_Get()->GetDarkColor(COLOR_RED));
	else if(player->HasPeaceTreatyWith(owner) || player->HasAnyPactWith(owner))
		return(colorset_Get()->GetDarkColor(COLOR_GREEN));
	else if(!player->HasContactWith(owner))
		return(colorset_Get()->GetDarkColor(COLOR_WHITE));
	else
		return(colorset_Get()->GetDarkColor(COLOR_YELLOW));
}

//---------------------------------------------------------------------------
//
//	RadarMap::RadarTileBorder
//
//---------------------------------------------------------------------------
//	- Checks which borders must be drawn for the current tile
//
//---------------------------------------------------------------------------
uint8 RadarMap::RadarTileBorder(const Player *player, const MapPoint &position)
{

	uint8 borderFlags = 0;

	if(!m_displayBorders)
		return(borderFlags);

	if(!player->m_vision->IsExplored(position))
		return(borderFlags);

// Added by Martin G�hmann
	sint32 owner = tiledmap_Get()->GetVisibleCellOwner(const_cast<MapPoint&>(position));

	if(owner < 0)
		return(borderFlags);

	if(owner != player->m_owner
	&& !player->m_hasGlobalRadar
	&& !Scheduler::CachedHasContactWithExceptSelf(player->m_owner, owner)
	&& !g_fog_toggle // Don't forget if fog of war is off
	&& !g_god
	)
		return(borderFlags);

	MapPoint neighborPosition;

	if(position.GetNeighborPosition(EAST, neighborPosition) &&
		(tiledmap_Get()->GetVisibleCellOwner(neighborPosition) != owner))
		borderFlags |= k_EAST_BORDER_FLAG;
	if(position.GetNeighborPosition(WEST, neighborPosition) &&
		(tiledmap_Get()->GetVisibleCellOwner(neighborPosition) != owner))
		borderFlags |= k_WEST_BORDER_FLAG;
	if(position.GetNeighborPosition(NORTHEAST, neighborPosition) &&
		(tiledmap_Get()->GetVisibleCellOwner(neighborPosition) != owner))
		borderFlags |= k_NORTH_EAST_BORDER_FLAG;
	if(position.GetNeighborPosition(NORTHWEST, neighborPosition) &&
		(tiledmap_Get()->GetVisibleCellOwner(neighborPosition) != owner))
		borderFlags |= k_NORTH_WEST_BORDER_FLAG;
	if(position.GetNeighborPosition(SOUTHEAST, neighborPosition) &&
		(tiledmap_Get()->GetVisibleCellOwner(neighborPosition) != owner))
		borderFlags |= k_SOUTH_EAST_BORDER_FLAG;
	if(position.GetNeighborPosition(SOUTHWEST, neighborPosition) &&
		(tiledmap_Get()->GetVisibleCellOwner(neighborPosition) != owner))
		borderFlags |= k_SOUTH_WEST_BORDER_FLAG;

	return(borderFlags);
}

//---------------------------------------------------------------------------
//
//	RadarMap::RenderTradeRoute
//
//---------------------------------------------------------------------------
//	- Draws a trade route sign for the current tile
//
//---------------------------------------------------------------------------
void RadarMap::RenderTradeRoute(aui_Surface *surface,
								const RECT &tileRectangle)
{
	sint32 tileWidth = tileRectangle.right - tileRectangle.left;
	sint32 tileHeight = tileRectangle.bottom - tileRectangle.top;

	RECT tradeRect = {
		tileRectangle.left + (tileWidth / 2),
		tileRectangle.top + (tileHeight / 2),
		tileRectangle.left + (tileWidth / 2) + 1,
		tileRectangle.top + (tileHeight / 2) + 1
	};

	primitives_PaintRect16(surface, &tradeRect, colorset_Get()->GetColor(COLOR_YELLOW));
}

//---------------------------------------------------------------------------
//
//	RadarMap::RenderCapitol
//
//---------------------------------------------------------------------------
//	- Draws a Capitol marker for the current tile
//
//---------------------------------------------------------------------------
void RadarMap::RenderCapitol(aui_Surface *surface, const MapPoint &position, const MapPoint &worldpos, Player *player)
{
	if(!m_displayCapitols)
		return;

	if(!player->m_vision->IsExplored(worldpos))
		return;

	Unit unit;

	if(!world_Get()->GetTopVisibleUnit(worldpos, unit))
		if(!world_Get()->GetTopRadarUnit(worldpos, unit))
			return;

	if(!unit.IsValid() || !unit.IsCity() || !unit.IsCapitol())
		return;

	MapPoint screenPosition(((worldpos.y / 2) + position.x) % (m_mapSize->x), position.y);

	double xPosition = screenPosition.x * m_tilePixelWidth;
	double yPosition = screenPosition.y * m_tilePixelHeight;

	if(screenPosition.y & 1)
		xPosition += m_tilePixelWidth / 2.0;

	//Now to build the "star"
	RECT vertical = {
		static_cast<sint32>(ceil(xPosition - m_tilePixelWidth)),
		static_cast<sint32>(ceil(yPosition)),
		static_cast<sint32>(ceil(xPosition + 2*m_tilePixelWidth)),
		static_cast<sint32>(ceil(yPosition + m_tilePixelHeight))
	};

	RECT horizontal = {
		static_cast<sint32>(ceil(xPosition)),
		static_cast<sint32>(ceil(yPosition  - m_tilePixelHeight)),
		static_cast<sint32>(ceil(xPosition + m_tilePixelWidth)),
		static_cast<sint32>(ceil(yPosition + 2*m_tilePixelHeight))
	};

	primitives_PaintRect16(surface, &vertical, colorset_Get()->GetColor(COLOR_ORANGE));
	primitives_PaintRect16(surface, &horizontal, colorset_Get()->GetColor(COLOR_ORANGE));

}


//---------------------------------------------------------------------------
//
//	RadarMap::RenderSpecialTile
//
//---------------------------------------------------------------------------
//	- Renders the current special radar map area, that means tiles that are
//    on the max or min x value.
//
//---------------------------------------------------------------------------
void RadarMap::RenderSpecialTile(aui_Surface *surface,
								 const MapPoint &screenPosition,
								 Pixel16 color, uint32 flags)
{
	RECT tileRectangle = {
		2*screenPosition.x + 1,
		screenPosition.y,
		2*screenPosition.x + 2,
		screenPosition.y + 1,
	};

	primitives_PaintRect16(surface, &tileRectangle, color);


	tileRectangle.left = 0;
	tileRectangle.right = 1;
	primitives_PaintRect16(surface, &tileRectangle, color);

}


//---------------------------------------------------------------------------
//
//	RadarMap::RenderSpecialTileBorder
//
//---------------------------------------------------------------------------
//	- Renders the current special radar map borders, that means tiles that are
//    on the max or min x value.
//
//---------------------------------------------------------------------------
void RadarMap::RenderSpecialTileBorder(aui_Surface *surface,
								 const MapPoint &screenPosition,
								 uint8 borderFlags,
								 Pixel16 borderColor)
{

	double xPosition = screenPosition.x * m_tilePixelWidth;
	double yPosition = screenPosition.y * m_tilePixelHeight;

	RECT tileRectangle = {
		static_cast<sint32>(ceil(xPosition + (m_tilePixelWidth / 2.0))),
		static_cast<sint32>(ceil(yPosition)),
		static_cast<sint32>(ceil(xPosition + m_tilePixelWidth)),
		static_cast<sint32>(ceil(yPosition + m_tilePixelHeight))
	};

	tileRectangle.right		= std::max<sint32>(tileRectangle.left, (tileRectangle.right - 1L));
	tileRectangle.bottom	= std::max<sint32>(tileRectangle.top, (tileRectangle.bottom - 1L));

	if(borderFlags & k_WEST_BORDER_FLAG)
		primitives_DrawLine16(surface, tileRectangle.left, tileRectangle.top,
			tileRectangle.left, tileRectangle.bottom, borderColor);
	if(borderFlags & k_NORTH_WEST_BORDER_FLAG)
		primitives_DrawLine16(surface, tileRectangle.right, tileRectangle.top,
			tileRectangle.left, tileRectangle.top, borderColor);
	if(borderFlags & k_SOUTH_WEST_BORDER_FLAG)
		primitives_DrawLine16(surface, tileRectangle.right, tileRectangle.bottom,
			tileRectangle.left, tileRectangle.bottom, borderColor);

	tileRectangle.left  = 0;
	tileRectangle.right	=
        std::max<LONG>(0, static_cast<LONG>(ceil(m_tilePixelWidth / 2.0)) - 1);

	if(borderFlags & k_EAST_BORDER_FLAG)
		primitives_DrawLine16(surface, tileRectangle.right, tileRectangle.top,
			tileRectangle.right, tileRectangle.bottom, borderColor);
	if(borderFlags & k_NORTH_EAST_BORDER_FLAG)
		primitives_DrawLine16(surface, tileRectangle.left, tileRectangle.top,
			tileRectangle.right, tileRectangle.top, borderColor);
	if(borderFlags & k_SOUTH_EAST_BORDER_FLAG)
		primitives_DrawLine16(surface, tileRectangle.left, tileRectangle.bottom,
			tileRectangle.right, tileRectangle.bottom, borderColor);
}

//---------------------------------------------------------------------------
//
//	RadarMap::RenderNormalTile
//
//---------------------------------------------------------------------------
//	- Renders the current normal radar map area
//
//---------------------------------------------------------------------------
void RadarMap::RenderNormalTile(aui_Surface *surface,
								const MapPoint &screenPosition,
								Pixel16 color, uint32 flags)
{
	RECT tileRectangle;
	tileRectangle.left = 2 * screenPosition.x + (screenPosition.y&1);
	tileRectangle.right = tileRectangle.left + 2;
	tileRectangle.top = screenPosition.y;
	tileRectangle.bottom = screenPosition.y + 1;
	primitives_PaintRect16(surface, &tileRectangle, color);

}


//---------------------------------------------------------------------------
//
//	RadarMap::RenderNormalTileBorder
//
//---------------------------------------------------------------------------
//	- Renders the current normal radar map borders
//
//---------------------------------------------------------------------------
void RadarMap::RenderNormalTileBorder(aui_Surface *surface,
		const MapPoint &screenPosition,
		uint8 borderFlags, Pixel16 borderColor)
{

	double xPosition = screenPosition.x * m_tilePixelWidth;
	double yPosition = screenPosition.y * m_tilePixelHeight;

	if(screenPosition.y & 1)
		xPosition += m_tilePixelWidth / 2.0;

	RECT tileRectangle = {
		static_cast<LONG>(ceil(xPosition)),
		static_cast<LONG>(ceil(yPosition)),
		static_cast<LONG>(ceil(xPosition + m_tilePixelWidth)),
		static_cast<LONG>(ceil(yPosition + m_tilePixelHeight))
	};

	tileRectangle.right		= std::max<sint32>(tileRectangle.left, (tileRectangle.right - 1L));
	tileRectangle.bottom	= std::max<sint32>(tileRectangle.top, (tileRectangle.bottom - 1L));
	LONG    middle			=
        std::min<LONG>(static_cast<LONG>(ceil(xPosition + m_tilePixelWidth/2)),
                       tileRectangle.right
                      );

	if(tileRectangle.right >= surface->Width())
		tileRectangle.right = surface->Width() - 1;
	if(tileRectangle.bottom >= surface->Height())
		tileRectangle.bottom = surface->Height() - 1;

	if(borderFlags & k_EAST_BORDER_FLAG)
		primitives_DrawLine16(surface, tileRectangle.right, tileRectangle.top,
			tileRectangle.right, tileRectangle.bottom, borderColor);
	if(borderFlags & k_NORTH_EAST_BORDER_FLAG)
		primitives_DrawLine16(surface, middle, tileRectangle.top,
			tileRectangle.right, tileRectangle.top, borderColor);
	if(borderFlags & k_SOUTH_EAST_BORDER_FLAG)
		primitives_DrawLine16(surface, middle, tileRectangle.bottom,
			tileRectangle.right, tileRectangle.bottom, borderColor);

	if(borderFlags & k_WEST_BORDER_FLAG)
		primitives_DrawLine16(surface, tileRectangle.left, tileRectangle.top,
			tileRectangle.left, tileRectangle.bottom, borderColor);
	if(borderFlags & k_NORTH_WEST_BORDER_FLAG)
		primitives_DrawLine16(surface, middle, tileRectangle.top,
			tileRectangle.left, tileRectangle.top, borderColor);
	if(borderFlags & k_SOUTH_WEST_BORDER_FLAG)
		primitives_DrawLine16(surface, middle, tileRectangle.bottom,
			tileRectangle.left, tileRectangle.bottom, borderColor);
}

//---------------------------------------------------------------------------
//
//	RadarMap::RenderMapTile
//
//---------------------------------------------------------------------------
//	- Controls which method renders the current radar map
//    tile
//
//---------------------------------------------------------------------------
void RadarMap::RenderMapTile(aui_Surface *surface, const MapPoint &screenPosition, Pixel16 color, uint32 flags)
{

	if((screenPosition.y & 1) && (screenPosition.x == (m_mapSize->x - 1)))
		RenderSpecialTile(surface, screenPosition, color, flags);
	else
		RenderNormalTile(surface, screenPosition, color, flags);
}

//---------------------------------------------------------------------------
//
//	RadarMap::RenderMapTileBorder
//
//---------------------------------------------------------------------------
//	- Controls which method renders the current radar map
//    borders
//
//---------------------------------------------------------------------------
void RadarMap::RenderMapTileBorder(aui_Surface *surface, const MapPoint &screenPosition,
							 uint8 borderFlags, Pixel16 borderColor)
{

	if((screenPosition.y & 1) && (screenPosition.x == (m_mapSize->x - 1)))
		RenderSpecialTileBorder(surface, screenPosition, borderFlags, borderColor);
	else
		RenderNormalTileBorder(surface, screenPosition, borderFlags, borderColor);
}

//---------------------------------------------------------------------------
//
//	RadarMap::RenderTile
//
//---------------------------------------------------------------------------
//	- Controls the rendering of the area for the current radar map
//    tile
//
//---------------------------------------------------------------------------
void RadarMap::RenderTile(aui_Surface *surface, const MapPoint &position,
						  const MapPoint &worldpos, Player *player)

{
	uint32 flags;

	Pixel16 const	color	=
		RadarTileColor(player, MapPoint(position.x, position.y), worldpos, flags);
	sint32 const	x		= ((position.y / 2) + position.x) % m_mapSize->x;




	RenderMapTile(surface, MapPoint(x, position.y), color, flags);

}

//---------------------------------------------------------------------------
//
//	RadarMap::RenderTileBorder
//
//---------------------------------------------------------------------------
//	- Controls the rendering of the borders for the current radar map
//    position
//---------------------------------------------------------------------------
void RadarMap::RenderTileBorder(aui_Surface *surface, const MapPoint &position,
						  const MapPoint &worldpos, Player *player)
{
	uint8 const	borderFlags =
		RadarTileBorder(player, MapPoint(worldpos.x, worldpos.y));

	if (borderFlags)
	{
		Pixel16 const	borderColor =
			RadarTileBorderColor(MapPoint(worldpos.x, worldpos.y), player);
		sint32 const	x	= ((position.y / 2) + position.x) % m_mapSize->x;
		RenderMapTileBorder(surface, MapPoint(x, position.y), borderFlags, borderColor);
	}
}

//---------------------------------------------------------------------------
//
//	RadarMap::RenderTrade
//
//---------------------------------------------------------------------------
//  - Prepares and call the rendering of the trade routes
//
//---------------------------------------------------------------------------
void RadarMap::RenderTrade(aui_Surface *surface, const MapPoint &position, const MapPoint &worldpos, Player *player)
{
	if(!m_displayTrade)
		return;




	MapPoint screenPosition(((worldpos.y / 2) + position.x) % (m_mapSize->x), position.y);

	if(!world_Get()->GetCell(worldpos)->GetNumTradeRoutes() ||
	   !player->m_vision->IsExplored(worldpos)) {
		return;
	}

	double xPosition = screenPosition.x * m_tilePixelWidth;
	double yPosition = screenPosition.y * m_tilePixelHeight;

	if(screenPosition.y & 1)
		xPosition += m_tilePixelWidth / 2.0;

	RECT tileRectangle = {
		static_cast<sint32>(ceil(xPosition)),
		static_cast<sint32>(ceil(yPosition)),
		static_cast<sint32>(ceil(xPosition + m_tilePixelWidth)),
		static_cast<sint32>(ceil(yPosition + m_tilePixelHeight))
	};

	RenderTradeRoute(surface, tileRectangle);
}

//---------------------------------------------------------------------------
//
//	RadarMap::RenderMap
//
//---------------------------------------------------------------------------
//	- Redraws the complete radarmap
//
//---------------------------------------------------------------------------
void RadarMap::RenderMap(aui_Surface *surface)
{

	Player *player = GetVisiblePlayerToRender();

	if(!player) {

		RECT destRect = { 0, 0, surface->Width(), surface->Height() };
		primitives_PaintRect16(surface, &destRect, colorset_Get()->GetColor(COLOR_BLACK));
		return;
	}

	sint32 x;
	sint32 y;

	for(y = 0; y < m_mapSize->y; y++)
		for(x = 0; x < m_mapSize->x; x++)
		{
			RenderTile(m_tempSurface, PosWorldToPosRadar(MapPoint(x, y)), MapPoint(x, y), player);
		}

    fRect const sRect = { 0.0,
                          0.0,
                          static_cast<float>(m_tempSurface->Width()),
                          static_cast<float>(m_tempSurface->Height())
                        };
    fRect const dRect = { 0.0,
                          0.0,
                          static_cast<float>((m_tilePixelWidth/2) * sRect.right),
                          static_cast<float>(m_tilePixelHeight * sRect.bottom)
                        };

	primitives_Scale16(m_tempSurface, surface,
		sRect,
		dRect,
		m_filter);

	for(y = 0; y < m_mapSize->y; y++)
		for(x = 0; x < m_mapSize->x; x++)
		{
			RenderTileBorder(surface, PosWorldToPosRadar(MapPoint(x, y)), MapPoint(x, y), player);
			RenderCapitol(surface, PosWorldToPosRadar(MapPoint(x, y)), MapPoint(x, y), player);
			RenderTrade(surface, PosWorldToPosRadar(MapPoint(x, y)), MapPoint(x, y), player);
		}
}

//---------------------------------------------------------------------------
//
//	RadarMap::RenderViewRect
//
//---------------------------------------------------------------------------
//  - Draws the rectangle on the radar map that corresponds to the view of
//    the main tile map.
//
//---------------------------------------------------------------------------
void RadarMap::RenderViewRect
(
	aui_Surface *surf,
	sint32 x,
	sint32 y
)
{
    RECT offsetRect = {0, 0, 0, 0};

	if (tiledmap_Get())
    {
		RECT *  temp        = tiledmap_Get()->GetMapViewRect();

        m_mapViewRect = *temp;

		if(!tiledmap_Get()->ReadyToDraw())
			return;

	    sint32  nrplayer    = selitem_Get()->GetVisiblePlayer();

		offsetRect.bottom = m_mapViewRect.bottom;
		offsetRect.top = m_mapViewRect.top;
		offsetRect.left = m_mapViewRect.left;
		offsetRect.right = m_mapViewRect.right;

		offsetRect.top += m_displayOffset[nrplayer].y;
		offsetRect.bottom += m_displayOffset[nrplayer].y;
		if (offsetRect.top + ((offsetRect.bottom - offsetRect.top)/2) > m_mapSize->y) {
			// if view is now too far to the bottom subtract the y-mapsize
			offsetRect.top -= m_mapSize->y;
			offsetRect.bottom -= m_mapSize->y;
		}

		offsetRect.left += m_displayOffset[nrplayer].x;
		offsetRect.right += m_displayOffset[nrplayer].x;
		if (offsetRect.left + ((offsetRect.right - offsetRect.left)/2) > m_mapSize->x) {
			// if view is now too far to the right subtract the x-mapsize
			offsetRect.left -= m_mapSize->x;
			offsetRect.right -= m_mapSize->x;
		}
	}


	sint32 x1;
	sint32 x2;
	sint32 x3;
	sint32 x4;
	sint32 y1;
	sint32 y2;
	sint32 y3;
	sint32 y4;

	if ( m_mapSize )
	{
		sint32 mapWidth = m_mapSize->x;
		sint32 mapHeight = m_mapSize->y;

		// Set the X coordinate points
		if (!world_Get()->IsXwrap() && (offsetRect.left < 0 || offsetRect.right > mapWidth))
		{
			x1 = x3 = offsetRect.left;
			x2 = x4 = offsetRect.right;

			if (offsetRect.left < 0)
			{
				x1 = x3 = 0;
			}
			if (offsetRect.right > mapWidth)
			{
				x2 = x4 = mapWidth;
			}
		}

		else
		{
			if (offsetRect.left < 0)
			{
				x1 = offsetRect.left + mapWidth;
				x2 = mapWidth;

				if (offsetRect.right <= 0)
				{
					x3 = mapWidth;
					x4 = mapWidth;
				}
				else
				{
					x3 = 0;
					x4 = offsetRect.right;
				}
			}

			else
			{
				if (offsetRect.right > mapWidth)
				{
					x1 = offsetRect.left;
					x2 = mapWidth;
					x3 = 0;
					x4 = offsetRect.right - mapWidth;
				}
				else
				{
					x1 = offsetRect.left;
					x2 = offsetRect.right;
					x3 = offsetRect.left;
					x4 = offsetRect.right;
				}
			}
		}

		x1 = (sint32) (x1 * m_tilePixelWidth) + x;
		x2 = (sint32) (x2 * m_tilePixelWidth) + x - 1;
		x3 = (sint32) (x3 * m_tilePixelWidth) + x;
		x4 = (sint32) (x4 * m_tilePixelWidth) + x - 1;

		// Set the Y points
		if ( !world_Get()->IsYwrap() && ( offsetRect.top < 0 || offsetRect.bottom >= mapHeight))
		{
            y1 = y3 = std::max<sint32>(0, offsetRect.top);
            y2 = y4 = std::min<sint32>(mapHeight, offsetRect.bottom);
		}
		else
		{
			if ( offsetRect.top < 0 )
			{
				y1 = offsetRect.top + mapHeight;
				y2 = mapHeight;
				if (offsetRect.bottom <= 0)
				{
					y3 = mapHeight;
					y4 = mapHeight;
				}
				else
				{
					y3 = 0;
					y4 = offsetRect.bottom;
				}
			}
			else
			{
				if (offsetRect.bottom > mapHeight)
				{
					y1 = offsetRect.top;
					y2 = mapHeight;
					y3 = 0;
					y4 = offsetRect.bottom - mapHeight;
				}
				else
				{
					y1 = offsetRect.top;
					y2 = offsetRect.bottom;
					y3 = offsetRect.top;
					y4 = offsetRect.bottom;
				}
			}
		}

		y1 = (sint32) (y1 * m_tilePixelHeight) + y;
		y2 = (sint32) (y2 * m_tilePixelHeight) + y - 1;
		y3 = (sint32) (y3 * m_tilePixelHeight) + y;
		y4 = (sint32) (y4 * m_tilePixelHeight) + y - 1;

	}
	else
	{ // no map size
		x1 = x3 = 0;
		x2 = x4 = 0;
		y1 = y3 = 0;
		y2 = y4 = 0;
	}

	// Draw the rectangle
	primitives_DrawLine16(surf,x1,y1,x2,y1,0xffff);
	primitives_DrawLine16(surf,x1,y1,x1,y2,0xffff);
	primitives_DrawLine16(surf,x3,y1,x4,y1,0xffff);
	primitives_DrawLine16(surf,x4,y1,x4,y2,0xffff);
	primitives_DrawLine16(surf,x1,y4,x2,y4,0xffff);
	primitives_DrawLine16(surf,x1,y3,x1,y4,0xffff);
	primitives_DrawLine16(surf,x3,y4,x4,y4,0xffff);
 	primitives_DrawLine16(surf,x4,y3,x4,y4,0xffff);
}

//---------------------------------------------------------------------------
//
//	RadarMap::UpdateMap
//
//---------------------------------------------------------------------------
void RadarMap::UpdateMap(aui_Surface *surf, sint32 x, sint32 y)
{
	RECT		destRect = {x, y, x + Width(), y + Height() };
	RECT		srcRect = {0, 0, Width(), Height()};

	c3ui_Get()->TheBlitter()->StretchBlt(surf, &destRect, m_mapSurface, &srcRect, k_AUI_BLITTER_FLAG_COPY);

	if(IsInteractive())
		RenderViewRect(surf, x, y);
}


//---------------------------------------------------------------------------
//
//	RadarMap::ComputeCenteredMap
//
//---------------------------------------------------------------------------
MapPoint RadarMap::ComputeCenteredMap(MapPoint const & pos, RECT *viewRect)
{
	LONG const  w   = viewRect->right - viewRect->left;
	LONG const  h   = viewRect->bottom - viewRect->top;

	sint32 tileX;
	maputils_MapX2TileX(pos.x, pos.y, &tileX);

	viewRect->left      = tileX - (w>>1);
	viewRect->top       = (pos.y - (h>>1)) & (~1);
	viewRect->right     = viewRect->left + w;
	viewRect->bottom    = viewRect->top + h;

	return pos;
}

//---------------------------------------------------------------------------
//
//	RadarMap::CenterMap
//
//---------------------------------------------------------------------------
//  - Used to focus the RadarMap to a specific point
//
//---------------------------------------------------------------------------
MapPoint RadarMap::CenterMap(MapPoint const & pos)
{
	MapPoint LastPT = m_lastCenteredPoint;
	if(!LastPT.IsValid())
		LastPT = pos;

	m_lastCenteredPoint = pos;

	RECT *mapViewRect = tiledmap_Get()->GetMapViewRect();

	ComputeCenteredMap(pos, mapViewRect);
	m_mapViewRect = *mapViewRect;
	RenderMap(m_mapSurface);

	return LastPT;
}


//---------------------------------------------------------------------------
//
//	RadarMap::IncludePointInView
//
//---------------------------------------------------------------------------
//	- seems unused
//
//---------------------------------------------------------------------------
BOOL RadarMap::IncludePointInView(MapPoint &pos, sint32 radius)
{
	RECT		*mapViewRect = tiledmap_Get()->GetMapViewRect();
	RECT		adjustedRect = *mapViewRect;

	sint32		tileX;
	maputils_MapX2TileX(pos.x, pos.y, &tileX);
	sint32  tileY = pos.y;

	sint32		 wrappedLeft;
	sint32		 wrappedTop;
	maputils_WrapPoint(mapViewRect->left, mapViewRect->top, &wrappedLeft, &wrappedTop);

	InflateRect(&adjustedRect, -radius, -radius);
	adjustedRect.top += 1;
	adjustedRect.bottom -= 3;

	if (tileY >= adjustedRect.top && tileY < adjustedRect.bottom &&
			tileX >=adjustedRect.left && tileX < adjustedRect.right)
		return FALSE;

	sint32	 newLeft=wrappedLeft;
	sint32	 newTop=wrappedTop;

	if (tileX < adjustedRect.left)
		newLeft = wrappedLeft - (adjustedRect.left - tileX);
	else
		if (tileX > adjustedRect.right)
			newLeft = wrappedLeft + (tileX - adjustedRect.right);

	if (tileY < adjustedRect.top)
		newTop = wrappedTop - (adjustedRect.top - tileY);
	else
		if (tileY >= adjustedRect.bottom)
			newTop = wrappedTop + (tileY - adjustedRect.bottom);

	sint32 newX;
	sint32 newY;
	maputils_WrapPoint(newLeft, newTop, &newX, &newY);

	sint32 w = mapViewRect->right - mapViewRect->left;
	sint32 h = mapViewRect->bottom - mapViewRect->top;

	mapViewRect->left = newX;
	mapViewRect->top = newY & ~0x1;
	mapViewRect->right = newX + w;
	mapViewRect->bottom = newY + h;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	RadarMap::Setup
//
//---------------------------------------------------------------------------
void RadarMap::Setup()
{
	CalculateMetrics();

	RenderMap(m_mapSurface);

	if (tiledmap_Get())
    {
		m_mapViewRect = *tiledmap_Get()->GetMapViewRect();
	}
}

//---------------------------------------------------------------------------
//
//	RadarMap::Update
//
//---------------------------------------------------------------------------
void RadarMap::Update( )
{

	m_mapSize = world_Get()->GetSize();

	RenderMap(m_mapSurface);
}

//---------------------------------------------------------------------------
//
//	RadarMap::RedrawTile
//
//---------------------------------------------------------------------------
//  - Refreshes the part of the radar map from and around the given point
//
//---------------------------------------------------------------------------
void RadarMap::RedrawTile( const MapPoint *point )
{
	Player *player = GetVisiblePlayerToRender();
	if(!player)
		return;

	// modify with the offset values
    MapPoint offsetpos = PosWorldToPosRadar( *point);
    RenderTile(m_tempSurface, offsetpos, *point, player);

	fRect sRect;
	fRect dRect;
	sint32 x0 = (offsetpos.y + 2 * offsetpos.x) % (2 * m_mapSize->x);




	float adjust = m_filter ? 0.5f : 0.0f;
	if (x0 == 2 * m_mapSize->x - 1)
	{

		sRect.left      = x0 - adjust;
		sRect.right     = x0 + 1.0f;
        sRect.top       = std::max(0.0f, offsetpos.y - adjust);
        sRect.bottom    = std::min(static_cast<float>(m_tempSurface->Height()),
                                   offsetpos.y + 1 + adjust
                                  );

		dRect.left   = sRect.left   * static_cast<float>(m_tilePixelWidth/2);
		dRect.right  = sRect.right  * static_cast<float>(m_tilePixelWidth/2);
		dRect.top    = sRect.top    * static_cast<float>(m_tilePixelHeight);
		dRect.bottom = sRect.bottom * static_cast<float>(m_tilePixelHeight);

		primitives_Scale16(m_tempSurface, m_mapSurface, sRect, dRect, m_filter);

		sRect.left = 0;
		sRect.right = 1.0f + adjust;
		dRect.left = 0;
		dRect.right = sRect.right * static_cast<float>(m_tilePixelWidth/2);

		primitives_Scale16(m_tempSurface, m_mapSurface, sRect, dRect, m_filter);
	}
	else
	{

        sRect.left      = std::max(0.0f, x0 - adjust);
        sRect.right     = std::min(static_cast<float>(m_tempSurface->Width()),
                                   x0 + 2 + adjust
                                  );
        sRect.top       = std::max(0.0f, offsetpos.y - adjust);
        sRect.bottom    = std::min(static_cast<float>(m_tempSurface->Height()),
                                   offsetpos.y + 1 + adjust
                                  );

		dRect.left   = sRect.left   * static_cast<float>(m_tilePixelWidth/2);
		dRect.right  = sRect.right  * static_cast<float>(m_tilePixelWidth/2);
		dRect.top    = sRect.top    * static_cast<float>(m_tilePixelHeight);
		dRect.bottom = sRect.bottom * static_cast<float>(m_tilePixelHeight);

		primitives_Scale16(m_tempSurface, m_mapSurface, sRect, dRect, m_filter);
	}

	RenderTileBorder(m_mapSurface, offsetpos, *point, player);
	RenderCapitol(m_mapSurface, offsetpos, *point, player);
	RenderTrade(m_mapSurface, offsetpos, *point, player);

	if (m_filter)
	{
		MapPoint neighbor;
		if (point->GetNeighborPosition(NORTHEAST, neighbor))
				RenderTileBorder(m_mapSurface, PosWorldToPosRadar(neighbor),neighbor, player);
		if (point->GetNeighborPosition(NORTHWEST, neighbor))
				RenderTileBorder(m_mapSurface, PosWorldToPosRadar(neighbor),neighbor, player);
		if (point->GetNeighborPosition(EAST, neighbor))
				RenderTileBorder(m_mapSurface, PosWorldToPosRadar(neighbor),neighbor, player);
		if (point->GetNeighborPosition(WEST, neighbor))
				RenderTileBorder(m_mapSurface, PosWorldToPosRadar(neighbor),neighbor, player);
		if (point->GetNeighborPosition(SOUTHEAST, neighbor))
				RenderTileBorder(m_mapSurface, PosWorldToPosRadar(neighbor),neighbor, player);
		if (point->GetNeighborPosition(SOUTHWEST, neighbor))
				RenderTileBorder(m_mapSurface, PosWorldToPosRadar(neighbor),neighbor, player);

	}
}

//---------------------------------------------------------------------------
//
//	RadarMap::DrawThis
//
//---------------------------------------------------------------------------
AUI_ERRCODE RadarMap::DrawThis(aui_Surface *surface, sint32 x,	sint32 y )
{

	if ( IsHidden() ) return AUI_ERRCODE_OK;

	if ( !surface )
		surface = m_window->TheSurface();

	RECT rect = { 0, 0, m_width, m_height };
	OffsetRect( &rect, m_x + x, m_y + y );
	ToWindow( &rect );

	UpdateMap(surface, rect.left, rect.top);

	if ( surface == m_window->TheSurface() )
		m_window->AddDirtyRect( &rect );

	return AUI_ERRCODE_OK;
}

//---------------------------------------------------------------------------
//
//	RadarMap::MouseLGrabInside
//
//---------------------------------------------------------------------------
//  - Processes a left mouse click in the radarmap region. Sets therefore
//	  a rectangle which region is seen centered by the point the mouse click
//    was located and call tiledmap methods to refresh the current view.
//---------------------------------------------------------------------------
void RadarMap::MouseLGrabInside(aui_MouseEvent *data)
{

	if(IsDisabled() || !IsInteractive())
		return;

	if (GetWhichSeesMouse() && GetWhichSeesMouse() != this) return;
	SetWhichSeesMouse(this);

	Assert(tiledmap_Get() != nullptr);
	if (tiledmap_Get() == nullptr) return;

	data->position.x -= X();
	data->position.y -= Y();

	RECT mapRect = {0, 0, Width(), Height()};
	if ( !PtInRect(&mapRect, data->position) ) return;

	tiledmap_Get()->SetSmoothScrollOffsets(0,0);

	sint32		 mapWidth;
	sint32		 mapHeight;
	tiledmap_Get()->GetMapMetrics(&mapWidth, &mapHeight);

	sint32  tileY   = (sint32) (data->position.y / m_tilePixelHeight);
    double  nudge   = (tileY & 1) ? m_tilePixelWidth / 2.0 : 0.0;
    sint32  tileX   = (sint32) ( ceil(((double)(data->position.x - nudge) / m_tilePixelWidth)) );

	tileX = (sint32) ((tileX - m_displayOffset[selitem_Get()->GetVisiblePlayer()].x
									+ m_mapSize->x) % m_mapSize->x);
	tileY = (sint32) ((tileY - m_displayOffset[selitem_Get()->GetVisiblePlayer()].y
									+ m_mapSize->y) % m_mapSize->y);

	sint32 width = m_mapViewRect.right - m_mapViewRect.left;
	sint32 height = m_mapViewRect.bottom - m_mapViewRect.top;

	m_mapViewRect.left = tileX - (width / 2);
	m_mapViewRect.right = m_mapViewRect.left + width;
	m_mapViewRect.top = (tileY - (height / 2)) & ~0x01;
	m_mapViewRect.bottom = m_mapViewRect.top + height;

	RECT *  realMapViewRect = tiledmap_Get()->GetMapViewRect();
	*realMapViewRect = m_mapViewRect;

	tiledmap_Get()->Refresh();
	tiledmap_Get()->InvalidateMap();
}

//---------------------------------------------------------------------------
//
//	RadarMap::MouseRGrabInside
//
//---------------------------------------------------------------------------
//	- Handling of a right mouseclick over the radar map
//
//---------------------------------------------------------------------------
void RadarMap::MouseRGrabInside(aui_MouseEvent *data)
{

	if(IsDisabled() || !IsInteractive())
		return;

	if (GetWhichSeesMouse() && GetWhichSeesMouse() != this) return;
	SetWhichSeesMouse(this);

	Assert(tiledmap_Get() != nullptr);
	if (tiledmap_Get() == nullptr) return;

	data->position.x -= X();
	data->position.y -= Y();

	sint32 nrplayer = selitem_Get()->GetVisiblePlayer();

	// compute the offsets after the MouseRClick to center the map with the
	// desired point

	if (world_Get()->IsXwrap()) {
		m_displayOffset[nrplayer].x  =
			(m_mapSize->x - ( m_mapSize->x * data->position.x / m_width) + (m_mapSize->x / 2)
			  + m_mapSize->x + m_displayOffset[nrplayer].x) % m_mapSize->x;
	} else {
		m_displayOffset[nrplayer].x = 0;
	}
	if (world_Get()->IsYwrap()) {
		m_displayOffset[nrplayer].y  =
			(m_mapSize->y - ((( m_mapSize->y * data->position.y / m_height) >>1)<<1)
			  + (m_mapSize->y / 2) + m_mapSize->y + m_displayOffset[nrplayer].y) % m_mapSize->y;
	} else {
		m_displayOffset[nrplayer].y = 0;
	}

	RenderMap(m_mapSurface);

}


//---------------------------------------------------------------------------
//
// RadarMap::Idle
//
//---------------------------------------------------------------------------
//  - Constantly called in the idle time
//
//---------------------------------------------------------------------------
AUI_ERRCODE RadarMap::Idle( )
{
	static uint32 lastDraw = 0;
	if (GetTickCount() - lastDraw > 100) lastDraw = GetTickCount();
	else return AUI_ERRCODE_OK;

	DrawThis(nullptr, 0, 0);

	return AUI_ERRCODE_OK;
}

//---------------------------------------------------------------------------
//
// RadarMap::MapOffset
//
//---------------------------------------------------------------------------
// - Gives Back a MapPoint that is modified  with the current X and Y
//   offsets
//
//---------------------------------------------------------------------------
MapPoint RadarMap::MapOffset(MapPoint oldPoint)
{
	MapPoint newPoint;

	newPoint.x =
		(oldPoint.x + m_displayOffset[selitem_Get()->GetVisiblePlayer()].x) % m_mapSize->x;

	newPoint.y =
		(oldPoint.y + m_displayOffset[selitem_Get()->GetVisiblePlayer()].y) % m_mapSize->y;

	return newPoint;
}

//---------------------------------------------------------------------------
//
// RadarMap::PosWorldToPosRadarCorrectedX
//
//---------------------------------------------------------------------------
// - Calculates from a given world point the coresponding point of the
//   radar map for handling the offset of the X and/or the Y axis
//
//---------------------------------------------------------------------------
MapPoint RadarMap::PosWorldToPosRadar(MapPoint worldpos)
{
	sint32 nrplayer = selitem_Get()->GetVisiblePlayer();

	MapPoint posRadar;
	posRadar.x = (worldpos.x - m_displayOffset[nrplayer].y/2 + m_mapSize->x
					+ m_displayOffset[nrplayer].x) % m_mapSize->x;

	posRadar.y = (worldpos.y + m_displayOffset[nrplayer].y) % m_mapSize->y;

	return posRadar;
}
