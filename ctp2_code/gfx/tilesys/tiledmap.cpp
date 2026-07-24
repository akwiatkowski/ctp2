//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Tile map handling
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
// _DEBUG
// - Set when generating the debug version
//
// _SMALL_MAPPOINTS
// - Use 2D world when set, add 3rd dimension (CTP1 space layer) when not set.
//
// __BIG_DIRTY_BLITS__
// __USING_SPANS__
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Make sure that cities created by the scenario editor keep their style and
//   their size. The last created city by the scenario editor is now selected.
//	 By Martin G�hmann.
// - Map wrapping corrected.
// - Possible leaks/invalid accesses corrected.
// - Current terrain improvements are displayed instead of those from the
//   last visit if the fog of war is toggled off. - Dec 24th 2004 - Martin G�hmann
// - With fog of war off the current city sprites and unit sprites at the
//   right position are displayed. - Dec. 25th 2004 - Martin G�hmann
// - Improved destructor (useless code removed, corrected delete [])
// - Removed .NET compiler warnings. - April 23rd 2005 Martin G�hmann
// - Prevented crashes on game startup and exit.
// - The good sprite index is now retrieved from the resource database
//   instaed of good sprite state database. (Aug 29th 2005 Martin G�hmann)
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
// - Made government modified for units work here. (July 29th 2006 Martin G�hmann)
// - added debugai profile switch - E 4-3-2007
// - When yes the debugai switch causes a crash
// - Full city radius is now drawn around settlers. (30-Jan-2008 Martin G�hmann)
// - Changed colour of maximum zoom grid from white to black. (12-Mar-2009 Maq)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/interface/backgroundwin.h"
#include "gfx/tilesys/tiledmap.h"
#include "gfx/tilesys/tiledmap_observer_adapter.h"  // RegisterTiledMapObserverAdapter

#include "gs/outcom/AICause.h"
#include <algorithm>                    // std::fill
#include "gs/gameobj/ArmyData.h"
#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_blitter.h"
#include "ui/aui_common/aui_surface.h"
#include "ui/aui_common/aui_Factory.h"
#include "ui/aui_common/aui_dirtylist.h"
#include "ui/aui_common/aui_stringtable.h"
#include "ui/aui_ctp2/background.h"
#include "gfx/tilesys/BaseTile.h"
#include "ctp/ctp2_utils/c3errors.h"
#include "ui/aui_ctp2/c3_popupwindow.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/c3window.h"
#include "gs/world/Cell.h"
#include "gs/world/cellunitlist.h"
#include "gs/gameobj/citydata.h"
#include "gs/gameobj/CityInfluenceIterator.h"
#include "gs/gameobj/CityRadius.h"
#include "gs/fileio/CivPaths.h"                   // civpaths_Get()
#include "gfx/gfx_utils/colorset.h"                   // colorset_Get()
#include "ui/interface/controlpanelwindow.h"         // controlpanel_Get()
#include "ui/aui_ctp2/ctp2_button.h"
#include "ui/aui_ctp2/ctp2_Window.h"
#include "gfx/spritesys/director.h"                   // director_Get()
#include "gfx/spritesys/EffectActor.h"
#include "gs/fileio/gamefile.h"
#include "gfx/gfx_utils/gfx_options.h"
#include "gfx/spritesys/GoodActor.h"
#include "gs/gameobj/GoodyHuts.h"
#include "ui/aui_ctp2/grabitem.h"
#include "gs/world/MapPoint.h"
#include "gfx/tilesys/maputils.h"
#include "gs/utility/MoveFlags.h"
#include "net/general/network.h"
#include "gfx/gfx_utils/pixelutils.h"
#include "gs/gameobj/Player.h"                     // player_Get()
#include "ctp/ctp2_utils/pointerlist.h"
#include "ui/aui_utils/primitives.h"
#include "gs/database/profileDB.h"                  // profiledb_Get()
#include "ui/aui_ctp2/radarmap.h"                   // radar_map_Get()
#include "ui/interface/radarwindow.h"
#include "ResourceRecord.h"
#include "ui/interface/scenarioeditor.h"
#include "gfx/spritesys/screenmanager.h"
#include "ui/aui_ctp2/SelItem.h"                    // selitem_Get()
#include "gs/slic/SlicEngine.h"
#include "gfx/spritesys/Sprite.h"
#include "ui/interface/spriteeditor.h"
#include "gs/database/StrDB.h"                      // stringdb_Get()
#include "TerrainImprovementRecord.h"
#include "TerrainRecord.h"
#include "gs/gameobj/terrainutil.h"
#include "gs/gameobj/TerrImprove.h"
#include "gs/gameobj/TerrImproveData.h"
#include "gs/gameobj/TerrImprovePool.h"
#include "gfx/gfx_utils/tiffutils.h"
#include "gfx/tilesys/TileDrawRoad.h"
#include "gs/world/TileInfo.h"
#include "gfx/tilesys/tileset.h"
#include "gfx/tilesys/GpuTileCache.h"   // P11 G1: terrain quad cache + signature
#include "ui/aui_sdl/aui_sdl.h"         // P11 G1: GPU quad atlas + draw list
#include "gfx/tilesys/tileutils.h"
#include "gs/gameobj/TradeRoute.h"
#include "gs/gameobj/TradeRouteData.h"
#include "gs/utility/TurnCnt.h"                    // turn_Get()
#include "gfx/spritesys/UnitActor.h"
#include "gs/gameobj/UnitData.h"
#include "UnitRecord.h"
#include "gfx/spritesys/UnitSpriteGroup.h"
#include "gs/world/UnseenCell.h"
#include "gs/world/World.h"                      // world_Get()

extern RECT             g_backgroundViewport;
extern SpriteEditWindow *g_spriteEditWindow;
extern GrabItem         *g_grabbedItem;
extern sint32           g_tradeSelectedState;
extern sint32           g_fog_toggle;
extern sint32           g_god;
extern sint32           g_isCheatModeOn;
extern BOOL             g_show_ai_dbg;
extern sint32           g_modalWindow;

sint32      g_unitNum               = -1;
sint32      g_cityNum               = -1;
BOOL        g_killMode              = FALSE;
sint32      g_tileImprovementMode   = 0;
BOOL        g_isTransportOn         = FALSE;
sint32      g_isFastCpu             = 1;
sint32      g_isGridOn              = 0;
sint32      g_placeGoodsMode        = FALSE;
BOOL	    g_drawArmyClumps;

sint32 s_zoomTilePixelWidth[k_MAX_ZOOM_LEVELS]	=	{48,56,68,76,88,96};
sint32 s_zoomTilePixelHeight[k_MAX_ZOOM_LEVELS]	=	{24,28,34,38,44,48};
sint32 s_zoomTileGridHeight[k_MAX_ZOOM_LEVELS] =	{36,42,51,57,66,72};
sint32 s_zoomTileHeadroom[k_MAX_ZOOM_LEVELS] =		{12,14,17,19,22,24};
double s_zoomTileScale[k_MAX_ZOOM_LEVELS] =			{0.50526, 0.58947, 0.71578, 0.8, 0.92631, 1.0};

#define k_GRID_CHUNKS_X		10
#define k_GRID_CHUNKS_Y		6

namespace
{
    RECT const          RECT_INVISIBLE      = {0, 0, 0, 0};

    bool AddGpuCityNamesQuad(TiledMap *map, sint32 w, sint32 h)
    {
        static std::unique_ptr<aui_Surface> s_surface;
        static SDL_Texture *s_texture = nullptr;
        static sint32 s_w = 0;
        static sint32 s_h = 0;

        auto fail = [](char const *reason) {
            aui_SDL::MarkQuadFrameIncomplete(reason);
            return false;
        };

        if (!map || !aui_SDL::Renderer() || w <= 0 || h <= 0)
            return fail("city-names-setup");
        if (!s_surface || s_w != w || s_h != h)
        {
            if (s_texture)
            {
                SDL_DestroyTexture(s_texture);
                s_texture = nullptr;
            }
            AUI_ERRCODE err = AUI_ERRCODE_OK;
            s_surface.reset(aui_Factory::new_Surface(err, w, h, nullptr, FALSE, FALSE, FALSE, 32));
            s_w = w;
            s_h = h;
        }
        if (!s_surface)
            return fail("city-names-surface");

        LPVOID bits = nullptr;
        if (s_surface->Lock(nullptr, &bits, 0) != AUI_ERRCODE_OK || !bits)
            return fail("city-names-lock");
        memset(bits, 0, static_cast<size_t>(s_surface->Pitch()) * static_cast<size_t>(h));
        s_surface->Unlock(bits);

        map->DrawCityNames(s_surface.get(), 0);

        if (!s_texture)
        {
            s_texture = SDL_CreateTexture(aui_SDL::Renderer(), SDL_PIXELFORMAT_ARGB8888,
                                          SDL_TEXTUREACCESS_STREAMING, w, h);
            if (!s_texture)
                return fail("city-names-texture");
            SDL_SetTextureBlendMode(s_texture, SDL_BLENDMODE_BLEND);
        }

        if (s_surface->Lock(nullptr, &bits, 0) != AUI_ERRCODE_OK || !bits)
            return fail("city-names-lock-upload");
        CTP2_SDL_UpdateTexture(s_texture, nullptr, bits, s_surface->Pitch());
        s_surface->Unlock(bits);

        aui_SDL::GpuSpriteQuad q;
        q.texture = s_texture;
        q.sx = 0; q.sy = 0; q.sw = w; q.sh = h;
        q.dx = aui_SDL::WorldContentOffX(); q.dy = aui_SDL::WorldContentOffY();
        q.dw = w; q.dh = h;
        q.mirror = false;
        q.alpha = 255;
        aui_SDL::AddSpriteQuad(q);
        return true;
    }
}

TiledMap::TiledMap(MapPoint &size)
:
#if defined(_DEBUG)
	num_loops               (1.0f),
	num_rects               (0.0f),
#endif
    m_zoomLevel             (k_ZOOM_NORMAL),
	m_zoomCallback          (nullptr),
	m_isScrolling           (false),
	m_surface               (nullptr),
	m_mapSurface            (nullptr),
	m_lockedSurface         (nullptr),
	m_surfBase              (nullptr),
	m_surfWidth             (0),
	m_surfHeight            (0),
	m_surfPitch             (0),
	m_surfIsLocked          (false),
	m_renderEverything      (false),
	m_renderExploredAsVisible (false),
    m_displayRect           (RECT_INVISIBLE),
    m_surfaceRect           (RECT_INVISIBLE),
	m_mapBounds             (RECT_INVISIBLE),
	m_mapViewRect           (RECT_INVISIBLE),
    m_scale                 (1.0),
	m_smoothOffsetX         (0),
    m_smoothOffsetY         (0),
	m_smoothLastX           (0),
    m_smoothLastY           (0),
	m_overlayActive         (false),
	m_overlayRec            (nullptr),
	m_overlayPos            (),
	m_overlayColor          (0),
	m_tileSet               (nullptr),
    m_hiliteMouseTile       (),
	m_drawHilite            (false),
#ifdef __USING_SPANS__
    m_mapDirtyList          (NULL),
	m_mixDirtyList          (NULL),
	m_oldMixDirtyList       (NULL),
#else
    m_mapDirtyList          (new aui_DirtyList),
	m_mixDirtyList          (new aui_DirtyList),
	m_oldMixDirtyList       (new aui_DirtyList),
#endif
	m_localVision           (nullptr),
	m_nextPlayer            (false),
	m_oldPlayer             (PLAYER_INDEX_INVALID),
	m_font                  (nullptr),
	m_gridWidth             (0),
	m_gridHeight            (0),
	m_one_over_gridWidth    (1.0f),
	m_one_over_gridHeight   (1.0f),
	m_chatRect              (RECT_INVISIBLE)
{
    std::fill(m_fortifyString, m_fortifyString + 4, 0);
    std::copy(s_zoomTilePixelWidth, s_zoomTilePixelWidth + k_MAX_ZOOM_LEVELS,
              m_zoomTilePixelWidth
             );
    std::copy(s_zoomTilePixelHeight, s_zoomTilePixelHeight + k_MAX_ZOOM_LEVELS,
              m_zoomTilePixelHeight
             );
    std::copy(s_zoomTilePixelWidth, s_zoomTilePixelWidth + k_MAX_ZOOM_LEVELS,
              m_zoomTilePixelWidth
             );
    std::copy(s_zoomTileGridHeight, s_zoomTileGridHeight + k_MAX_ZOOM_LEVELS,
              m_zoomTileGridHeight
             );
    std::copy(s_zoomTileHeadroom, s_zoomTileHeadroom + k_MAX_ZOOM_LEVELS,
              m_zoomTileHeadroom
             );
    std::copy(s_zoomTileScale, s_zoomTileScale + k_MAX_ZOOM_LEVELS,
              m_zoomTileScale
             );

	SetRect(&m_mapBounds, 0, 0, size.x, size.y);
	SetZoomLevel(k_ZOOM_NORMAL);
	GenerateHitMask();  // fills m_tileHitMask[]

	// Bridge tiledmap_Get() → tiledmap_observer interface so gs/ and ai/ code
	// can call tiledmap_observer::RedrawTile(...) etc. without depending
	// on gfx/.  Headless never constructs a TiledMap → observer stays
	// unregistered → all calls become no-ops.  Idempotent across the
	// gameinit_ResetMapSize() delete/new cycle.
	RegisterTiledMapObserverAdapter();

	AUI_ERRCODE         errcode     = AUI_ERRCODE_OK;
	aui_StringTable	*   stringTable =
        new aui_StringTable(&errcode, "TiledMapFontStringTable");

	if (AUI_NEWOK(stringTable, errcode))
    {
		MBCHAR *    fontNameString  = stringTable->GetString(0);
		MBCHAR *    fontSizeString  = stringTable->GetString(1);

		m_font = c3ui_Get()->LoadBitmapFont(fontNameString);
		Assert(m_font);
		m_font->SetPointSize(atoi(fontSizeString));

		MBCHAR *    fString         = stringTable->GetString(2);
		strlcpy(m_fortifyString, fString, sizeof(m_fortifyString));
	}

	delete stringTable;
}

TiledMap::~TiledMap()
{
	DeleteGrid();

	if (c3ui_Get() && m_font)
	{
		c3ui_Get()->UnloadBitmapFont(m_font);
	}
	delete m_mapSurface;
	delete m_mixDirtyList;
	delete m_oldMixDirtyList;
	delete m_mapDirtyList;
	delete m_tileSet;
	// m_gpuTileCache / m_gpuScratchTile are unique_ptr — freed automatically
	// m_localVision    not deleted: reference only
	// m_surface        not deleted: reference only
	// m_surfBase       not deleted: reference only
	// m_overlayRec     not deleted: reference only
	// m_lockedSurface  not deleted: reference only
}

sint32 TiledMap::Initialize(RECT *viewRect)
{
	sint32			w = viewRect->right - viewRect->left;
	sint32			h = viewRect->bottom - viewRect->top;
	AUI_ERRCODE		errcode;

	// P11 Stage 2 B2.3: the world surface is 32-bit ARGB8888. The tile, sprite
	// and primitive writers all expand-at-store into 32-bit now (dormant paths
	// activated by this flip), and the implicit m_mapSurface->secondary
	// SDL_BlitSurface 565->8888 convert self-neutralizes into a 32->32 copy.
	m_mapSurface = aui_Factory::new_Surface(errcode, w, h, nullptr, FALSE, FALSE, FALSE, 32);
	Assert(m_mapSurface);
	if (!m_mapSurface) return AUI_ERRCODE_MEMALLOCFAILED;




	m_surface = m_mapSurface;

#ifdef __USING_SPANS__

	m_mixDirtyList = new aui_DirtyList( TRUE, w, h );
	m_oldMixDirtyList = new aui_DirtyList( TRUE, w, h );
	m_mapDirtyList = new aui_DirtyList( TRUE, w, h );
#endif

	m_displayRect = m_surfaceRect = *viewRect;
	OffsetRect(&m_surfaceRect, -m_surfaceRect.left, -m_surfaceRect.top);

	CalculateMetrics();

	m_localVision = player_Get(selitem_Get()->GetVisiblePlayer())->m_vision;

	Assert(m_localVision);

	m_localVision->SetAmOnScreen(true);

	InitGrid( 128, 128);

	return AUI_ERRCODE_OK;
}

void TiledMap::InitGrid(sint32 maxPixelsPerGridRectX, sint32 maxPixelsPerGridRectY)
{
	RECT		tempRect;

	maxPixelsPerGridRectX &= ~0x01;

	m_gridWidth  = (m_surfaceRect.right /maxPixelsPerGridRectX)+1;
	m_gridHeight = (m_surfaceRect.bottom/maxPixelsPerGridRectY)+1;





	m_one_over_gridWidth  = 1.0f/(float)maxPixelsPerGridRectX;
	m_one_over_gridHeight = 1.0f/(float)maxPixelsPerGridRectY;

	m_gridRects.resize(m_gridHeight);

	for (sint32 i=0; i<m_gridHeight; i++)
	{
		m_gridRects[i] = new GridRect[m_gridWidth];

		for(sint32 j=0; j<m_gridWidth; j++)
		{

			tempRect.left  = j * maxPixelsPerGridRectX;
		    tempRect.right = tempRect.left + maxPixelsPerGridRectX;

			tempRect.top    = i * maxPixelsPerGridRectY;
			tempRect.bottom = tempRect.top + maxPixelsPerGridRectY;

			if(tempRect.right > m_surfaceRect.right)
			   tempRect.right = m_surfaceRect.right;


			if(tempRect.bottom > m_surfaceRect.bottom)
			   tempRect.bottom = m_surfaceRect.bottom;

			m_gridRects[i][j].rect = tempRect;
			m_gridRects[i][j].dirty = FALSE;
		}
	}
}

void TiledMap::DeleteGrid()
{
	for (sint32 i=0; i<m_gridHeight; i++) {
		delete m_gridRects[i];
	}

	m_gridRects.clear();
}






void TiledMap::CheckRectAgainstGrid
(
    RECT &          rect,
    aui_DirtyList * a_List
)
{
	sint32 x_start = (sint32)((float)rect.left   * m_one_over_gridWidth )-1;
	sint32 x_end   = (sint32)((float)rect.right  * m_one_over_gridWidth )+1;
	sint32 y_start = (sint32)((float)rect.top    * m_one_over_gridHeight)-1;
	sint32 y_end   = (sint32)((float)rect.bottom * m_one_over_gridHeight)+1;

	if(x_start<0)
	   x_start = 0;
	else
	   if(x_start>=m_gridWidth)
		  return;

	if(x_end>=m_gridWidth)
	   x_end =m_gridWidth-1;
	else
	   if(x_end<0)
		  return;

	if(y_start<0)
	   y_start = 0;
	else
	   if(y_start>=m_gridHeight)
		  return;

	if(y_end>=m_gridHeight)
	   y_end =m_gridHeight-1;
	else
	   if(y_end<0)
		  return;

	for (sint32 i=y_start; i<y_end; i++)
	{
		for (sint32 j=x_start; j<x_end; j++)
		{
			GridRect * gr = &m_gridRects[i][j];

			if(gr->dirty)
		       continue;

			if(rect.left>=gr->rect.right)
			   continue;

			if(rect.right<=gr->rect.left)
			   continue;

		    if(rect.top>=gr->rect.bottom)
			   continue;

		    if(rect.bottom<=gr->rect.top)
			   continue;

			gr->dirty = TRUE;
			a_List->AddRect(&gr->rect);

#ifdef _DEBUG
		    IncRectMetric();
#endif

		}
	}
}

void TiledMap::ClearGrid()
{
	for (sint32 i=0; i<m_gridHeight; i++) {
		for (sint32 j=0; j<m_gridWidth; j++) {
			m_gridRects[i][j].dirty = FALSE;
		}
	}
}

void TiledMap::LockSurface()
{
	LockThisSurface(m_surface);
}

void TiledMap::LockThisSurface(aui_Surface *surface)
{

	m_lockedSurface = surface;

	AUI_ERRCODE	errcode = surface->Lock(nullptr, (LPVOID *)&m_surfBase, 0);
	Assert(errcode == AUI_ERRCODE_OK);
	if ( errcode != AUI_ERRCODE_OK ) return;

	m_surfWidth = surface->Width();
	m_surfHeight = surface->Height();
	m_surfPitch = surface->Pitch();
	m_surfIsLocked = TRUE;
}

void TiledMap::UnlockSurface()
{
	AUI_ERRCODE	errcode = m_lockedSurface->Unlock((LPVOID)m_surfBase);
	Assert(errcode == AUI_ERRCODE_OK);
	if ( errcode != AUI_ERRCODE_OK ) return;

	m_surfBase = nullptr;
	m_surfWidth = 0;
	m_surfHeight = 0;
	m_surfPitch = 0;
	m_surfIsLocked = FALSE;
}

//----------------------------------------------------------------------------
// FullMapPixelSize / RenderFullMap — offscreen UNFOGGED full-map export for the
// empire-timelapse tooling. The UI layer owns the concrete 16bpp surface and
// the BMP write (the tile blitter writes 16-bit pixels); here we just size the
// surface and drive the existing per-tile renderer over the WHOLE map with fog
// disabled, restoring all render state afterwards.
//----------------------------------------------------------------------------
void TiledMap::FullMapPixelSize(sint32 zoomLevel, sint32 *width, sint32 *height)
{
	if (zoomLevel < 0)              zoomLevel = 0;
	if (zoomLevel > k_ZOOM_LARGEST) zoomLevel = k_ZOOM_LARGEST;

	sint32 const tw = m_zoomTilePixelWidth[zoomLevel];
	sint32 const th = m_zoomTilePixelHeight[zoomLevel];
	sint32 const hr = m_zoomTileHeadroom[zoomLevel];
	World * w = world_Get();
	sint32 const mw = w ? w->GetXWidth()  : 0;
	sint32 const mh = w ? w->GetYHeight() : 0;

	// Iso layout: odd rows shift right by tw/2, each map row steps down th/2.
	// One tile of slack on each axis so edge tiles + headroom are not clipped.
	if (width)  *width  = (mw + 1) * tw + tw;
	if (height) *height = (mh + 2) * (th / 2) + th + hr;
}

AUI_ERRCODE TiledMap::RenderFullMap(aui_Surface *dest, sint32 zoomLevel)
{
	World * w = world_Get();
	if (!dest || !w || !m_tileSet) return AUI_ERRCODE_INVALIDPARAM;

	if (zoomLevel < 0)              zoomLevel = 0;
	if (zoomLevel > k_ZOOM_LARGEST) zoomLevel = k_ZOOM_LARGEST;

	// --- save the state we are about to clobber ---
	sint32 const  savedZoom       = m_zoomLevel;
	aui_Surface * savedSurface    = m_surface;
	RECT const    savedView       = m_mapViewRect;
	RECT const    savedSurfRect   = m_surfaceRect;
	bool const    savedEverything = m_renderEverything;

	SetZoomLevel(zoomLevel);

	sint32 const mw = w->GetXWidth();
	sint32 const mh = w->GetYHeight();

	// Project from map origin so maputils_MapXY2PixelXY yields absolute
	// (whole-map) pixel coords; widen the clip rect to the whole surface.
	m_mapViewRect.left = 0; m_mapViewRect.top = 0;
	m_mapViewRect.right = mw; m_mapViewRect.bottom = mh;
	m_surfaceRect.left = 0; m_surfaceRect.top = 0;
	m_surfaceRect.right = dest->Width(); m_surfaceRect.bottom = dest->Height();
	m_renderEverything = true;

	RetargetTileSurface(dest);
	LockThisSurface(dest);

	// Black background behind the iso diamonds (gaps between tiles).
	if (m_surfBase) memset(m_surfBase, 0, (size_t) m_surfHeight * m_surfPitch);

	RECT fullMap;
	fullMap.left = 0; fullMap.top = 0; fullMap.right = mw; fullMap.bottom = mh;
	RepaintTiles(&fullMap);

	// City markers: a filled square per city in its owner's colour, drawn while
	// the viewport is still at origin (absolute projection) and the surface is
	// locked. Terrain alone barely changes turn to turn — the cities are what
	// make the timelapse move. Same maputils projection as the tiles, so the
	// markers sit on the correct iso tiles.
	{
		bool const   bpp32 = m_lockedSurface && m_lockedSurface->BitsPerPixel() == 32;
		sint32 const step  = bpp32 ? 4 : 2;
		sint32 const tw   = GetZoomTilePixelWidth();
		sint32 const th   = GetZoomTilePixelHeight();
		sint32 const hr   = GetZoomTileHeadroom();
		sint32 const half = th / 2;
		for (sint32 p = 0; p < k_MAX_PLAYERS; ++p) {
			Player * pl = player_Get(p);
			if (!pl) continue;
			Pixel16 const col = colorset_Get()->GetPlayerColor(p);
			UnitDynamicArray * cl = pl->GetAllCitiesList();
			for (sint32 i = 0; cl && i < cl->Num(); ++i) {
				Unit u = cl->Access(i);
				if (!u.IsValid()) continue;
				MapPoint cp;
				u.GetPos(cp);
				sint32 cx, cy;
				maputils_MapXY2PixelXY(cp.x, cp.y, &cx, &cy);
				sint32 const mx = cx + tw / 2;       // tile centre x
				sint32 const my = cy + hr + half;    // tile centre y
				for (sint32 dy = -half; dy <= half; ++dy) {
					sint32 const yy = my + dy;
					if (yy < 0 || yy >= m_surfHeight) continue;
					uint8 * row = m_surfBase + yy * m_surfPitch;
					for (sint32 dx = -half; dx <= half; ++dx) {
						sint32 const xx = mx + dx;
						if (xx < 0 || xx >= m_surfWidth) continue;
						pixelutils_StorePixel(row + xx * step, col, bpp32);
					}
				}
			}
		}
	}

	UnlockSurface();
	RetargetTileSurface(savedSurface);

	// --- restore ---
	m_renderEverything = savedEverything;
	m_mapViewRect      = savedView;
	m_surfaceRect      = savedSurfRect;
	SetZoomLevel(savedZoom);

	return AUI_ERRCODE_OK;
}

AUI_ERRCODE TiledMap::RenderPlayerView(aui_Surface *dest, sint32 zoomLevel,
                                       sint32 playerIndex, RECT *exploredPixelRect,
                                       std::vector<CityLabel> *cityLabels)
{
	World * w   = world_Get();
	Player * pl = player_Get(playerIndex);
	if (!dest || !w || !m_tileSet || !pl || !pl->m_vision)
		return AUI_ERRCODE_INVALIDPARAM;

	if (zoomLevel < 0)              zoomLevel = 0;
	if (zoomLevel > k_ZOOM_LARGEST) zoomLevel = k_ZOOM_LARGEST;

	// --- save state ---
	sint32 const  savedZoom    = m_zoomLevel;
	aui_Surface * savedSurface = m_surface;
	RECT const    savedView    = m_mapViewRect;
	RECT const    savedSurfRect= m_surfaceRect;
	bool const    savedEvery   = m_renderEverything;
	Vision *      savedVision  = m_localVision;

	SetZoomLevel(zoomLevel);
	sint32 const mw    = w->GetXWidth();
	sint32 const mh    = w->GetYHeight();
	double const scale = GetZoomScale(zoomLevel);

	m_mapViewRect.left = 0; m_mapViewRect.top = 0;
	m_mapViewRect.right = mw; m_mapViewRect.bottom = mh;
	m_surfaceRect.left = 0; m_surfaceRect.top = 0;
	m_surfaceRect.right = dest->Width(); m_surfaceRect.bottom = dest->Height();
	m_renderEverything = false;          // unexplored tiles stay black
	m_renderExploredAsVisible = true;    // explored tiles at full brightness

	// AI players don't maintain a fog/Vision explored map in autoplay, so
	// reconstruct "what this empire can see" from its units' and cities'
	// vision ranges into a throwaway Vision and render through that. (amOnScreen
	// = false so AddVisible doesn't fire tiledmap redraw side effects.)
	Vision sightVision(playerIndex, false);
	{
		UnitDynamicArray * units = pl->m_all_units;
		for (sint32 u = 0; units && u < units->Num(); ++u) {
			Unit unit = units->Access(u);
			if (!unit.IsValid()) continue;
			MapPoint up; unit.GetPos(up);
			double const r = unit.GetVisionRange();
			sightVision.AddVisible(up, (r > 0.0) ? r : 1.0);
		}
		UnitDynamicArray * cities = pl->GetAllCitiesList();
		for (sint32 c = 0; cities && c < cities->Num(); ++c) {
			Unit city = cities->Access(c);
			if (!city.IsValid()) continue;
			MapPoint cp; city.GetPos(cp);
			double const r = city.GetVisionRange();
			sightVision.AddVisible(cp, (r > 0.0) ? r : 2.0);
		}
	}
	m_localVision = &sightVision;        // render from this empire's sight

	RetargetTileSurface(dest);
	LockThisSurface(dest);
	if (m_surfBase) memset(m_surfBase, 0, (size_t) m_surfHeight * m_surfPitch);

	// 1) fogged terrain — use RepaintTiles (the proven path): it calls
	//    CalculateWrap with a NULL surface, which draws into the locked
	//    m_surfBase. Passing a non-null surface here suppresses the tile draw.
	RECT fullMap;
	fullMap.left = 0; fullMap.top = 0; fullMap.right = mw; fullMap.bottom = mh;
	RepaintTiles(&fullMap);

	// 2) infrastructure — same NULL-surface convention.
	for (sint32 i = 0; i < mh; ++i)
		for (sint32 j = 0; j < mw; ++j)
			DrawImprovements(nullptr, i, j, false);

	// Sprites lock the surface THEMSELVES (Sprite::DrawDirect -> LockSurface),
	// so release our lock first or the re-lock fails -> null buffer -> crash.
	UnlockSurface();

	// 3) unit/city sprites the player can see; also accumulate the explored
	//    pixel bounds so the caller can crop to "what the player sees".
	sint32 pminX = dest->Width(), pminY = dest->Height(), pmaxX = 0, pmaxY = 0;
	bool   any   = false;
	sint32 const tw = GetZoomTilePixelWidth();
	sint32 const th = GetZoomTilePixelHeight();
	sint32 const hr = GetZoomTileHeadroom();
	for (sint32 y = 0; y < mh; ++y) {
		for (sint32 x = 0; x < mw; ++x) {
			MapPoint pos(x, y);
			if (!m_localVision->IsExplored(pos)) continue;

			sint32 px, py;
			maputils_MapXY2PixelXY(pos.x, pos.y, &px, &py);
			any = true;
			if (px < pminX) pminX = px;
			if (py < pminY) pminY = py;
			if (px + tw > pmaxX) pmaxX = px + tw;
			if (py + th + hr > pmaxY) pmaxY = py + th + hr;

			Unit top;
			if (w->GetTopVisibleUnit(playerIndex, pos, top)) {
				std::shared_ptr<UnitActor> actor = top.GetActor();
				if (actor)
					actor->DrawDirect(dest, px, py, scale);

				if (cityLabels && top.IsCity()) {
					CityData * cd = top.GetData() ? top.GetData()->GetCityData() : nullptr;
					CityLabel cl;
					cl.px    = px;
					cl.py    = py;
					cl.owner = (sint32) top.GetOwner();
					cl.pop   = cd ? cd->PopCount() : 0;
					snprintf(cl.name, sizeof(cl.name), "%s",
					         top.GetName() ? top.GetName() : "");
					cityLabels->push_back(cl);
				}
			}
		}
	}

	RetargetTileSurface(savedSurface);

	if (exploredPixelRect) {
		if (!any) {
			exploredPixelRect->left = 0; exploredPixelRect->top = 0;
			exploredPixelRect->right = dest->Width();
			exploredPixelRect->bottom = dest->Height();
		} else {
			sint32 const m = tw;   // a tile of margin around the explored region
			exploredPixelRect->left   = (pminX - m < 0) ? 0 : pminX - m;
			exploredPixelRect->top    = (pminY - m < 0) ? 0 : pminY - m;
			exploredPixelRect->right  = (pmaxX + m > dest->Width())  ? dest->Width()  : pmaxX + m;
			exploredPixelRect->bottom = (pmaxY + m > dest->Height()) ? dest->Height() : pmaxY + m;
		}
	}

	// --- restore ---
	m_renderEverything = savedEvery;
	m_renderExploredAsVisible = false;
	m_mapViewRect      = savedView;
	m_surfaceRect      = savedSurfRect;
	m_localVision      = savedVision;
	SetZoomLevel(savedZoom);

	return AUI_ERRCODE_OK;
}

void TiledMap::AddDirty(sint32 left, sint32 top, sint32 width, sint32 height, aui_DirtyList * a_List)
{
	RECT		rect;

	rect.left = left;
	rect.top = top;
	rect.right = left + width;
	rect.bottom = top + height;

	AddDirtyRect(rect, a_List);
}

#define __GRIDDED_BLITS__

void TiledMap::AddDirtyRect(RECT &rect, aui_DirtyList * a_List)
{
	if (a_List) {
		RECT tempRect = rect;

#ifdef __GRIDDED_BLITS__
	if (a_List == m_mixDirtyList) {
		CheckRectAgainstGrid(rect, a_List);
	} else {

		tempRect.left = tempRect.left & 0xFFFFFFFC;
		tempRect.right = (tempRect.right & 0xFFFFFFFC)+4;

		tempRect.top = tempRect.top & 0xFFFFFFFE;
		tempRect.bottom = (tempRect.bottom & 0xFFFFFFFE) + 2;

		a_List->AddRect(&tempRect);
	}
#else
	#ifdef __BIG_DIRTY_BLITS__

		tempRect.left = tempRect.left & 0xFFFFFF00;
		tempRect.right = (tempRect.right & 0xFFFFFF00)+256;

		tempRect.top = tempRect.top & 0xFFFFFFF0;
		tempRect.bottom = (tempRect.bottom & 0xFFFFFFF0) + 16;

		a_List->AddRect(&tempRect);
	#else

		tempRect.left = tempRect.left & 0xFFFFFFFC;
		tempRect.right = (tempRect.right & 0xFFFFFFFC)+4;

		tempRect.top = tempRect.top & 0xFFFFFFFE;
		tempRect.bottom = (tempRect.bottom & 0xFFFFFFFE) + 2;

		a_List->AddRect(&tempRect);
	#endif
#endif

	}
}

void TiledMap::AddDirtyTile(MapPoint &pos, aui_DirtyList * a_List)
{
	sint32		 x;
	sint32		 y;

	maputils_MapXY2PixelXY(pos.x, pos.y, &x, &y);

	RECT		rect;

	rect.left = x;
	rect.top = y;
	rect.right = x + k_TILE_GRID_WIDTH;
	rect.bottom = y + k_TILE_GRID_HEIGHT;

	AddDirtyRect(rect, a_List);
}

void TiledMap::AddDirtyToMap(sint32 left, sint32 top, sint32 width, sint32 height)
{
	AddDirty(left, top, width, height, m_mapDirtyList);
}

void TiledMap::AddDirtyRectToMap(RECT &rect)
{
	AddDirtyRect(rect, m_mapDirtyList);
}

void TiledMap::AddDirtyTileToMap(MapPoint &pos)
{
	AddDirtyTile(pos, m_mapDirtyList);
}

void TiledMap::AddDirtyToMix(sint32 left, sint32 top, sint32 width, sint32 height)
{
	AddDirty(left, top, width, height, m_mixDirtyList);
}

void TiledMap::AddDirtyRectToMix(RECT &rect)
{
	AddDirtyRect(rect, m_mixDirtyList);
}

void TiledMap::AddDirtyTileToMix(MapPoint &pos)
{
	AddDirtyTile(pos, m_mixDirtyList);
}


void TiledMap::ClearMixDirtyRects()
{
	Assert (m_mixDirtyList);

	if (m_mixDirtyList)
		m_mixDirtyList->Flush();

	ClearGrid();
}

void TiledMap::CopyMixDirtyRects(aui_DirtyList *dest)
{
	dest->Flush();

	ListPos	position = m_oldMixDirtyList->GetHeadPosition();;
	sint32	i;
	for (i = m_oldMixDirtyList->L(); i > 0; --i)
	{
		dest->AddRect(m_oldMixDirtyList->GetNext(position));
	}

	m_oldMixDirtyList->Flush();

#ifdef __USING_SPANS__
	m_oldMixDirtyList->SetSpans(m_mixDirtyList);
#endif

	position = m_mixDirtyList->GetHeadPosition();
	for (i = m_mixDirtyList->L(); i > 0; --i)
	{
		RECT *rect = m_mixDirtyList->GetNext(position);
		dest->AddRect(rect);
		m_oldMixDirtyList->AddRect(rect);
	}
}






void TiledMap::RestoreMixFromMap(aui_Surface *destSurf)
{
#ifdef __USING_SPANS__
	c3ui_Get()->TheBlitter()->SpanBlt(
		destSurf,
		0,
		0,
		m_surface,
		m_oldMixDirtyList,
		k_AUI_BLITTER_FLAG_COPY );
#else
	m_oldMixDirtyList->Minimize();

	ListPos position = m_oldMixDirtyList->GetHeadPosition();

	for (sint32 i = m_oldMixDirtyList->L(); i > 0; --i)
	{
		RECT * rect = m_oldMixDirtyList->GetNext(position);
		c3ui_Get()->TheBlitter()->Blt(destSurf, rect->left, rect->top, m_surface, rect, k_AUI_BLITTER_FLAG_COPY);
	}
#endif
}

void TiledMap::OffsetMixDirtyRects(sint32 deltaX, sint32 deltaY)
{
#ifdef __USING_SPANS__

	deltaX = -deltaX, deltaY = -deltaY;

	aui_SpanList *curSpanList = m_oldMixDirtyList->GetSpans();

	sint32 height = m_oldMixDirtyList->GetHeight();
	if ( deltaY >= height || deltaY <= -height )
		memset( curSpanList, 0, height * sizeof( aui_SpanList ) );
	else
	{
		if ( deltaY > 0 )
		{
			memmove(
				curSpanList + deltaY,
				curSpanList,
				height - deltaY );
			memset(
				curSpanList,
				0,
				deltaY * sizeof( aui_SpanList ) );
		}
		else if ( deltaY < 0 )
		{
			memmove(
				curSpanList,
				curSpanList - deltaY,
				height + deltaY );
			memset(
				curSpanList + height + deltaY,
				0,
				-deltaY * sizeof( aui_SpanList ) );
		}
	}

	sint32 width = m_oldMixDirtyList->GetWidth();
	if ( deltaX >= width || deltaX <= -width )
		memset( curSpanList, 0, height * sizeof( aui_SpanList ) );
	else
	{
		if ( deltaX > 0 )
		{
			for ( sint32 i = height; i; i-- )
			{
				if ( curSpanList->num > 0 )
				{
					aui_Span *curSpan = curSpanList->spans;
					curSpan->run += (sint16)deltaX;

					sint32 right = 0;
					for ( sint32 num = curSpanList->num; num; num-- )
					{
						right += curSpan->run + curSpan->length;
						curSpan++;
					}

					curSpan = curSpanList->spans;

					if ( right > width )
					{

						curSpanList->num = 1;

						curSpan->run = 0;
						curSpan->length = (sint16)width;
					}
				}

				curSpanList++;
			}
		}
		else if ( deltaX < 0 )
		{
			for ( sint32 i = height; i; i-- )
			{
				if ( curSpanList->num > 0 )
				{
					aui_Span *curSpan = curSpanList->spans;
					if ( ( curSpan->run += (sint16)deltaX ) < 0 )
					{

						curSpanList->num = 1;

						curSpan->run = 0;
						curSpan->length = (sint16)width;
					}
				}

				curSpanList++;
			}
		}
	}
#else
	ListPos position = m_oldMixDirtyList->GetHeadPosition();

	for (sint32 i = m_oldMixDirtyList->L(); i > 0; --i)
	{
		RECT * rect = m_oldMixDirtyList->GetNext(position);

		OffsetRect(rect, -deltaX, -deltaY);
		if (rect->left < m_displayRect.left) rect->left = m_displayRect.left;
		if (rect->top < m_displayRect.top) rect->top = m_displayRect.top;
		if (rect->right > m_displayRect.right) rect->right = m_displayRect.right;
		if (rect->bottom > m_displayRect.bottom) rect->bottom = m_displayRect.bottom;
	}
#endif
}

void TiledMap::InvalidateMap()
{

	RECT tempRect = m_surfaceRect;

	m_mapDirtyList->Flush();

	AddDirtyRect(tempRect, m_mapDirtyList);
}

void TiledMap::ValidateMap()
{
	m_mapDirtyList->Flush();
}

void TiledMap::InvalidateMix()
{
	RECT tempRect = g_backgroundViewport;

	m_mixDirtyList->Flush();

	AddDirtyRect(tempRect, m_mixDirtyList);

	for (sint32 i=0; i<m_gridHeight; i++) {
		for (sint32 j=0; j<m_gridWidth; j++) {
			m_gridRects[i][j].dirty = TRUE;
		}
	}


m_oldMixDirtyList->Flush();
AddDirtyRect(g_backgroundViewport, m_oldMixDirtyList);
}

void TiledMap::ValidateMix()
{
	m_mixDirtyList->Flush();

	for (sint32 i=0; i<m_gridHeight; i++) {
		for (sint32 j=0; j<m_gridWidth; j++) {
			m_gridRects[i][j].dirty = FALSE;
		}
	}
	m_oldMixDirtyList->Flush();
}





void TiledMap::UpdateMixFromMap(aui_Surface *mixSurf)
{
#ifdef __USING_SPANS__

	ListPos position = m_mapDirtyList->GetHeadPosition();

	for (sint32 i = m_mapDirtyList->L(); i > 0; --i)
	{
		RECT * rect = m_mapDirtyList->GetNext(position);
		m_mixDirtyList->AddRect(rect);
	}

	c3ui_Get()->TheBlitter()->SpanBlt(
		mixSurf,
		0,
		0,
		m_surface,
		m_mapDirtyList,
		k_AUI_BLITTER_FLAG_COPY );

#else

	if (m_mapDirtyList->L() > 500) {

		InvalidateMap();
	} else {
		m_mapDirtyList->Minimize();
	}

	ListPos position = m_mapDirtyList->GetHeadPosition();

	for (sint32 i = m_mapDirtyList->L(); i > 0; --i)
	{
		RECT * rect = m_mapDirtyList->GetNext(position);
		c3ui_Get()->TheBlitter()->Blt(mixSurf, rect->left, rect->top, m_surface, rect, k_AUI_BLITTER_FLAG_COPY);
		m_mixDirtyList->AddRect(rect);
	}

#endif

	m_mapDirtyList->Flush();
}





void TiledMap::LoadTileset()
{
	TileSet *tileSet = new TileSet;
	tileSet->QuickLoadMapped();
	delete m_tileSet;
	m_tileSet = tileSet;
}

sint16 TiledMap::TryRiver(BOOL bc, BOOL bn, BOOL bne, BOOL be, BOOL bse, BOOL bs, BOOL bsw, BOOL bw, BOOL bnw, BOOL cwater)
{
	if (!bc) return -1;

	sint16		 tc;
	sint16		 tn;
	sint16		 tne;
	sint16		 te;
	sint16		 tse;
	sint16		 ts;
	sint16		 tsw;
	sint16		 tw;
	sint16		 tnw;

	for (uint16 i = 0; i < m_tileSet->GetNumRiverTransforms(); i++)
    {
		tn =	m_tileSet->GetRiverTransform(i, 2);
		tne =	m_tileSet->GetRiverTransform(i, 5);
		te =	m_tileSet->GetRiverTransform(i, 6);
		tse =	m_tileSet->GetRiverTransform(i, 7);
		tc =	m_tileSet->GetRiverTransform(i, 4);
		ts =	m_tileSet->GetRiverTransform(i, 6);
		tsw =	m_tileSet->GetRiverTransform(i, 3);
		tw =	m_tileSet->GetRiverTransform(i, 0);
		tnw =	m_tileSet->GetRiverTransform(i, 1);

		if (	((cwater && (tc == 2)) || (bc && (tc==1)))
			&&	((bne && tne)	|| (!bne && !tne))
			&&	((bse && tse)	|| (!bse && !tse))
			&&	((bsw && tsw)	|| (!bsw && !tsw))
			&&	((bnw && tnw)	|| (!bnw && !tnw))
			) {

			return (sint16)i;
		}
	}

	return -1;
}

#define WILDCARD_ALL			-1
#define WILDCARD_LAND			-2
#define WILDCARD_WATER			-3
#define WILDCARD_DEEP			-4
#define WILDCARD_SHALLOW		-5
#define WILDCARD_TRENCHSHELF	-6
#define WILDCARD_ALL_BUT_SELF   -7
#define WILDCARD_LAND_NOT_SELF  -8
#define WILDCARD_WATER_NOT_SELF -9

#define IS_WATER(x) ((g_theTerrainDB->Get(x)->GetMovementTypeSea()) || (g_theTerrainDB->Get(x)->GetMovementTypeShallowWater()))

#define IS_LAND(x)          ((g_theTerrainDB->Get(x)->GetMovementTypeLand()) || (g_theTerrainDB->Get(x)->GetMovementTypeMountain()))

#define IS_DEEP(x)		((x == TERRAIN_WATER_DEEP) || (x == TERRAIN_WATER_VOLCANO) || (x == TERRAIN_WATER_SHELF) || (x == TERRAIN_WATER_TRENCH))
#define IS_SHALLOW(x)	((g_theTerrainDB->Get(x)->GetMovementTypeShallowWater()))
#define IS_TRENCHSHELF(x) ((x == TERRAIN_WATER_SHELF) || (x == TERRAIN_WATER_TRENCH))

#define MATCH_ALL(transform)				(transform == WILDCARD_ALL)
#define MATCH_WATER(transform, map)			((transform == WILDCARD_WATER) && IS_WATER(map))
#define MATCH_LAND(transform, map)			((transform == WILDCARD_LAND) && IS_LAND(map))
#define MATCH_DEEP(transform, map)			((transform == WILDCARD_DEEP) && IS_DEEP(map))
#define MATCH_SHALLOW(transform, map)		((transform == WILDCARD_SHALLOW) && IS_SHALLOW(map))
#define MATCH_TRENCHSHELF(transform, map)	((transform == WILDCARD_TRENCHSHELF) && IS_TRENCHSHELF(map))
#define MATCH_ALL_BUT_SELF(transform, map)  ((transform == WILDCARD_ALL_BUT_SELF) && ((c) != (map)))
#define MATCH_LAND_NOT_SELF(transform, map) ((transform == WILDCARD_LAND_NOT_SELF) && ((c) != (map)) && IS_LAND(map))
#define MATCH_WATER_NOT_SELF(transform, map) ((transform == WILDCARD_WATER_NOT_SELF) && ((c) != (map)) && IS_WATER(map))

#define MATCH_GLOB(t, m) (MATCH_ALL(t) || (t == g_theTerrainDB->Get(m)->GetTilesetIndex()) || MATCH_LAND(t, m) || MATCH_WATER(t, m) || MATCH_DEEP(t,m) || MATCH_SHALLOW(t,m) || MATCH_TRENCHSHELF(t,m) || MATCH_ALL_BUT_SELF(t,m) || MATCH_LAND_NOT_SELF(t,m) || MATCH_WATER_NOT_SELF(t,m))

bool TiledMap::TryTransforms(MapPoint &pos, uint16 c, uint16 n, uint16 ne, uint16 e, uint16 se,
							 uint16 s, uint16 sw, uint16 w, uint16 nw, uint16 *newIndex)
{
	for (uint16 i = 0; i < m_tileSet->GetNumTransforms(); i++)
    {
		sint16  tn      = m_tileSet->GetTransform(i,2);
		sint16  tne     = m_tileSet->GetTransform(i,5);
		sint16  te      = m_tileSet->GetTransform(i,8);
		sint16  tse     = m_tileSet->GetTransform(i,7);
		sint16  tc      = m_tileSet->GetTransform(i,4);
		sint16  ts      = m_tileSet->GetTransform(i,6);
		sint16  tsw     = m_tileSet->GetTransform(i,3);
		sint16  tw      = m_tileSet->GetTransform(i,0);
		sint16  tnw     = m_tileSet->GetTransform(i,1);
		sint16  tNew    = m_tileSet->GetTransform(i,9);

		if (MATCH_GLOB(tc,c) &&
			MATCH_GLOB(tn, n) &&
			MATCH_GLOB(tne, ne) &&
			MATCH_GLOB(te, e) &&
			MATCH_GLOB(tse, se) &&
			MATCH_GLOB(ts, s) &&
			MATCH_GLOB(tsw, sw) &&
			MATCH_GLOB(tw, w) &&
			MATCH_GLOB(tnw, nw)
           )
		{
			if (tNew == k_TRANSFORM_TO_LIST_ID) {

				sint16		xform;
				bool		legal = false;

                for (uint16 j = 0; j < k_MAX_TRANSFORM_TO_LIST; j++)
                {
					xform = m_tileSet->GetTransform(i, k_TRANSFORM_TO_LIST_FIRST + j);
					if (xform != k_TRANSFORM_TO_LIST_ID) legal = true;
				}

				if (!legal) return false;

				TileInfo *  ti      = GetTileInfo(pos);
				sint32      which   = ti->GetTransform() % k_MAX_TRANSFORM_TO_LIST;

				do {
					xform = (sint16)m_tileSet->GetTransform((uint16)i, (uint16)(k_TRANSFORM_TO_LIST_FIRST + which));
					which--;
					if(which < 0) {
						which = k_MAX_TRANSFORM_TO_LIST;
					}
					if(which == ti->GetTransform() % k_MAX_TRANSFORM_TO_LIST) {
						return false;
					}
				} while (xform == k_TRANSFORM_TO_LIST_ID || m_tileSet->GetBaseTile(xform) == nullptr);

				*newIndex = xform;
			} else {

				*newIndex = tNew;
			}
			return true;
		}

	}

	return false;
}

void TiledMap::TryMegaTiles(MapPoint &pos, BOOL regenTilenum)
{
	if (m_tileSet->GetNumMegaTiles() < 1)
		return;

	MapPoint	newPos = pos;

    MapPoint	goodPath[k_MAX_MEGATILE_STEPS];
	uint16		goodPathTiles[k_MAX_MEGATILE_STEPS];
	uint8		goodPathLastDirs[k_MAX_MEGATILE_STEPS];
	uint8		goodPathNextDirs[k_MAX_MEGATILE_STEPS];

	MegaTileStep	step;

	sint32 i = rand() % m_tileSet->GetNumMegaTiles();

	for
    (
        sint32 tilesTried = 0;
        tilesTried < m_tileSet->GetNumMegaTiles();
        ++tilesTried
    )
    {
        std::fill(goodPath, goodPath + k_MAX_MEGATILE_STEPS, MapPoint());
        std::fill(goodPathTiles, goodPathTiles + k_MAX_MEGATILE_STEPS, 0);
        std::fill(goodPathLastDirs, goodPathLastDirs + k_MAX_MEGATILE_STEPS, 0);
        std::fill(goodPathNextDirs, goodPathNextDirs + k_MAX_MEGATILE_STEPS, 0);

		uint16  pathLen = m_tileSet->GetMegaTileLength(i);
		uint16  pathPos = 0;
		step = m_tileSet->GetMegaTileStep(i, pathPos);

		while (pathPos < pathLen)
        {
			TileInfo *  tileInfo    = GetTileInfo(newPos);
			if (tileInfo->GetTileNum() >= k_FIRST_VARIATION) break;

			sint32      terrainType = tileInfo->GetTerrainType();
			if (terrainType != (sint32)step.terrainType || tileInfo->IsMega())
				break;

			goodPath[pathPos] = newPos;
			goodPathTiles[pathPos] = (uint16)step.tileNum;

			pathPos++;

			if (pathPos < pathLen) {
				step = m_tileSet->GetMegaTileStep(i, pathPos);

				goodPathNextDirs[pathPos-1] = (uint8)step.direction;

				switch(step.direction) {
				case k_MEGATILE_DIRECTION_N :
					newPos.GetNeighborPosition(NORTHWEST, newPos);
					break;
				case k_MEGATILE_DIRECTION_E :
					newPos.GetNeighborPosition(NORTHEAST, newPos);
					break;
				case k_MEGATILE_DIRECTION_S :
					newPos.GetNeighborPosition(SOUTHEAST, newPos);
					break;
				case k_MEGATILE_DIRECTION_W :
					newPos.GetNeighborPosition(SOUTHWEST, newPos);
					break;
				}

				goodPathLastDirs[pathPos] = m_tileSet->ReverseDirection((sint32)step.direction);
			}
		}

		if ((pathPos == pathLen) && regenTilenum)
        {
			for (uint16 j = 0; j < pathLen; j++)
            {
				TileInfo *	theTileInfo = world_Get()->GetTileInfo(goodPath[j]);

				theTileInfo->SetTileNum(goodPathTiles[j]);
				theTileInfo->SetLastMega(goodPathLastDirs[j]);
				theTileInfo->SetNextMega(goodPathNextDirs[j]);
			}
		}

		if (++i >= m_tileSet->GetNumMegaTiles()) i = 0;
	}
}

void TiledMap::PostProcessTile(MapPoint &pos, TileInfo *theTileInfo,
							   BOOL regenTilenum)
{
	// Headless builds construct a TiledMap without LoadTileset (see
	// CivApp::InitializeGameHeadless). Without a tileset, tile-graphics
	// post-processing has nothing to compute against and TryTransforms
	// would deref a null TileSet. Skip in that case — the visible-on-screen
	// transformations are UI concerns, irrelevant for game logic.
	if (!m_tileSet) return;

	if (theTileInfo->HasGoodActor())
		theTileInfo->DeleteGoodActor();

	sint32			goodIndex;
	if(world_Get()->GetGood(pos, goodIndex)) {
		theTileInfo->SetGoodActor(g_theResourceDB->Get(goodIndex)->GetSpriteID(), pos);
	}

	uint8 index = static_cast<uint8>(world_Get()->GetTerrain(pos.x, pos.y));
	if(regenTilenum)
		theTileInfo->SetTileNum(static_cast<TILEINDEX>(g_theTerrainDB->Get(index)->GetTilesetIndex()));

	theTileInfo->SetTerrainType(index);




	uint16	 n;
	uint16	 ne;
	uint16	 e;
	uint16	 se;
	uint16	 s;
	uint16	 sw;
	uint16	 w;
	uint16	 nw;
	BOOL		rn, rne, re, rse, rs, rsw, rw, rnw;

	uint16 c = static_cast<uint16>(g_theTerrainDB->Get(index)->GetTilesetIndex());
	BOOL	rc = world_Get()->IsRiver(pos.x, pos.y);

	MapPoint	newPos;
	if(pos.GetNeighborPosition(NORTH, newPos)) {

		n = static_cast<uint16>(world_Get()->GetTerrain(newPos.x, newPos.y));
		rn = world_Get()->IsRiver(newPos.x, newPos.y);
	} else {
		n = index;
		rn = FALSE;
	}

	if(pos.GetNeighborPosition(SOUTH, newPos)) {

		s = static_cast<uint16>(world_Get()->GetTerrain(newPos.x, newPos.y));
		rs = world_Get()->IsRiver(newPos.x, newPos.y);
	} else {
		s = index;
		rs = FALSE;
	}

	if(pos.GetNeighborPosition(EAST, newPos)) {

		e = static_cast<uint16>(world_Get()->GetTerrain(newPos.x, newPos.y));
		re = world_Get()->IsRiver(newPos.x, newPos.y);
	} else {
		e = index;
		re = FALSE;
	}

	if(pos.GetNeighborPosition(WEST, newPos)) {

		w = static_cast<uint16>(world_Get()->GetTerrain(newPos.x, newPos.y));
		rw = world_Get()->IsRiver(newPos.x, newPos.y);
	} else {
		w = index;
		rw = FALSE;
	}

	if(pos.GetNeighborPosition(SOUTHWEST, newPos)) {

		sw = static_cast<uint16>(world_Get()->GetTerrain(newPos.x, newPos.y));
		rsw = world_Get()->IsRiver(newPos.x, newPos.y);
	} else {
		sw = index;
		rsw = FALSE;
	}

	if(pos.GetNeighborPosition(NORTHWEST, newPos)) {

		nw = static_cast<uint16>(world_Get()->GetTerrain(newPos.x, newPos.y));
		rnw = world_Get()->IsRiver(newPos.x, newPos.y);
	} else {
		nw = index;
		rnw = FALSE;
	}

	if(pos.GetNeighborPosition(NORTHEAST, newPos)) {

		ne = static_cast<uint16>(world_Get()->GetTerrain(newPos.x, newPos.y));
		rne = world_Get()->IsRiver(newPos.x, newPos.y);
	} else {
		ne = index;
		rne = FALSE;
	}

	if(pos.GetNeighborPosition(SOUTHEAST, newPos)) {

		se = static_cast<uint16>(world_Get()->GetTerrain(newPos.x, newPos.y));
		rse = world_Get()->IsRiver(newPos.x, newPos.y);
	} else {
		se = index;
		rse = FALSE;
	}

	theTileInfo->SetTransition(0, static_cast<uint16>(g_theTerrainDB->Get(sw)->GetTilesetIndex()));
	theTileInfo->SetTransition(1, static_cast<uint16>(g_theTerrainDB->Get(nw)->GetTilesetIndex()));
	theTileInfo->SetTransition(2, static_cast<uint16>(g_theTerrainDB->Get(ne)->GetTilesetIndex()));
	theTileInfo->SetTransition(3, static_cast<uint16>(g_theTerrainDB->Get(se)->GetTilesetIndex()));






	uint16 newIndex;

	if (TryTransforms(pos, index, n, ne, e, se, s, sw, w, nw, &newIndex)) {
		if(regenTilenum)
			theTileInfo->SetTileNum(newIndex);
	}

	if (rc) {
		BOOL	cwater = FALSE;
		if (c == TERRAIN_WATER_BEACH) {
			if (n == TERRAIN_WATER_BEACH) rn = FALSE;
			if (ne == TERRAIN_WATER_BEACH) rne = FALSE;
			if (e == TERRAIN_WATER_BEACH) re = FALSE;
			if (se == TERRAIN_WATER_BEACH) rse = FALSE;
			if (s == TERRAIN_WATER_BEACH) rs = FALSE;
			if (sw == TERRAIN_WATER_BEACH) rsw = FALSE;
			if (w == TERRAIN_WATER_BEACH) rw = FALSE;
			if (nw == TERRAIN_WATER_BEACH) rnw = FALSE;

			cwater = TRUE;
		}

		sint16	river = TryRiver(rc, rn, rne, re, rse, rs, rsw, rw, rnw, cwater);

		theTileInfo->SetRiverPiece(river);
	} else {

		theTileInfo->SetRiverPiece(-1);
	}

}




void TiledMap::PostProcessMap(BOOL regenTilenums)
{
	MapPoint		pos;
	sint16			 i;
	sint16			 j;
	TILEINDEX       origTilenum = 0;
	uint8           origMega = 0;

	Assert (world_Get() != nullptr);
	if (world_Get() == nullptr) return;

//	MapPoint * size = world_Get()->GetSize();

	TileInfo		*theTileInfo = nullptr;

	for (i=0; i<m_mapBounds.bottom; i++) {
		for (j=0; j<m_mapBounds.right; j++) {

			pos.x = j;
			pos.y = i;

			theTileInfo = world_Get()->GetTileInfoStoragePtr(pos);

			if(!regenTilenums) {
				origTilenum = theTileInfo->GetTileNum();
				origMega = theTileInfo->GetMega();
			}

			PostProcessTile(pos, theTileInfo, regenTilenums);

			if(!regenTilenums) {
				theTileInfo->SetTileNum(origTilenum);
				theTileInfo->SetMega(origMega);
			}










		}
	}

	for (i=0; i<m_mapBounds.bottom; i++) {
		for (j=0; j<m_mapBounds.right; j++) {

			pos.x = j;
			pos.y = i;

			if(!regenTilenums) {
				theTileInfo = world_Get()->GetTileInfoStoragePtr(pos);
				origTilenum = theTileInfo->GetTileNum();
				origMega = theTileInfo->GetMega();
			}
			TryMegaTiles(pos, regenTilenums);
			if(!regenTilenums) {
				theTileInfo->SetTileNum(origTilenum);
				theTileInfo->SetMega(origMega);
			}
		}
	}
}

void TiledMap::BreakMegaTile(MapPoint &pos)
{
	TileInfo * tileInfo = GetTileInfo(pos);
	Assert(tileInfo);
	if (tileInfo == nullptr) return;

	tileInfo->SetTileNum((TILEINDEX)g_theTerrainDB->Get(tileInfo->GetTerrainType())->GetTilesetIndex());
	PostProcessTile(pos, tileInfo);
	RedrawTile(&pos);

	MapPoint	curPos = pos;
	while (sint32 next = tileInfo->GetNextMega())
    {
		switch (next)
        {
		case k_MEGATILE_DIRECTION_N :
			curPos.GetNeighborPosition(NORTHWEST, curPos);
			break;
		case k_MEGATILE_DIRECTION_E :
			curPos.GetNeighborPosition(NORTHEAST, curPos);
			break;
		case k_MEGATILE_DIRECTION_S :
			curPos.GetNeighborPosition(SOUTHEAST, curPos);
			break;
		case k_MEGATILE_DIRECTION_W :
			curPos.GetNeighborPosition(SOUTHWEST, curPos);
			break;
		default:
			Assert(FALSE);
		}

		tileInfo = GetTileInfo(curPos);
		tileInfo->SetTileNum((TILEINDEX)g_theTerrainDB->Get(tileInfo->GetTerrainType())->GetTilesetIndex());
		PostProcessTile(curPos, tileInfo);
		RedrawTile(&curPos);
	}

	curPos = pos;
	while (sint32 last = tileInfo->GetLastMega())
    {
		switch (last)
        {
		case k_MEGATILE_DIRECTION_N :
			curPos.GetNeighborPosition(NORTHWEST, curPos);
			break;
		case k_MEGATILE_DIRECTION_E :
			curPos.GetNeighborPosition(NORTHEAST, curPos);
			break;
		case k_MEGATILE_DIRECTION_S :
			curPos.GetNeighborPosition(SOUTHEAST, curPos);
			break;
		case k_MEGATILE_DIRECTION_W :
			curPos.GetNeighborPosition(SOUTHWEST, curPos);
			break;
		default:
			Assert(FALSE);
		}

		tileInfo = GetTileInfo(curPos);
		tileInfo->SetTileNum((TILEINDEX)g_theTerrainDB->Get(tileInfo->GetTerrainType())->GetTilesetIndex());
		PostProcessTile(curPos, tileInfo);
		RedrawTile(&curPos);
	}
}

void TiledMap::TileChanged(MapPoint &pos)
{
	TileInfo * tileInfo = GetTileInfo(pos);

	if (tileInfo && tileInfo->IsMega())
	{
		BreakMegaTile(pos);
	}
}


void TiledMap::ReloadGoodActors()
{
	Assert (world_Get());
	if (world_Get() == nullptr) return;

	LOADTYPE const	loadType = (profiledb_Get()->IsGoodAnim()) ? LOADTYPE_FULL : LOADTYPE_BASIC;

	for (sint16 i = 0; i < m_mapBounds.bottom; ++i)
	{
		for (sint16 j = 0; j < m_mapBounds.right; ++j)
		{
			TileInfo *  theTileInfo = world_Get()->GetTileInfoStoragePtr(MapPoint(j, i));

			if (theTileInfo)
			{
				GoodActor * goodActor = theTileInfo->GetGoodActor();
				if (goodActor && (goodActor->GetLoadType() != loadType))
                {
					if (loadType == LOADTYPE_FULL)
                    {
						goodActor->FullLoad();
					}
                    else
                    {
						goodActor->DumpFullLoad();
					}
				}
			}
		}
	}
}

void TiledMap::GenerateHitMask()
{
 	sint32 const    startLine = k_TILE_PIXEL_HEADROOM;
	sint32 const    midLine = k_TILE_PIXEL_HEADROOM + (k_TILE_GRID_HEIGHT - k_TILE_PIXEL_HEADROOM) / 2;
	sint32 const    endLine = k_TILE_GRID_HEIGHT;

	sint32		i;
	for (i = 0; i < k_TILE_PIXEL_HEADROOM; ++i)
	{
		m_tileHitMask[i].start = 1;
		m_tileHitMask[i].end = 0;
		m_tileHitMask[i].d_start = 1.0;
		m_tileHitMask[i].d_end   = 0.0;
	}

	uint16	startPos = k_TILE_GRID_WIDTH/2 - 1;
	uint16 	endPos = k_TILE_GRID_WIDTH/2;

	for (i = startLine; i < midLine; ++i)
	{
		m_tileHitMask[i].start = startPos;
		m_tileHitMask[i].end = endPos;
		m_tileHitMask[i].d_start = (double)startPos;
		m_tileHitMask[i].d_end   = (double)endPos;

		startPos -= 2;
		endPos += 2;
	}

	startPos = 0;
	endPos = k_TILE_GRID_WIDTH - 1;

	for (i = midLine; i < endLine; ++i)
	{
		m_tileHitMask[i].start = startPos;
		m_tileHitMask[i].end = endPos;
		m_tileHitMask[i].d_start = (double)startPos;
		m_tileHitMask[i].d_end   = (double)endPos;

		startPos += 2;
		endPos -= 2;
	}
}

void TiledMap::SetHiliteMouseTile(MapPoint &pos)
{
	m_hiliteMouseTile = pos;
}

void TiledMap::DrawHiliteMouseTile(aui_Surface *destSurf)
{
	if ( !m_drawHilite ) return;

	if (ScenarioEditor::DrawRegion())
	{
		MapPoint ul = ScenarioEditor::GetRegionUpperLeft();
		sint32 w = ScenarioEditor::GetRegionWidth();
		sint32 h = ScenarioEditor::GetRegionHeight();

		// x and y are orthogonal coordinates now
		for (sint16 y = 0; y < h; ++y)
		{
			for (sint16 x = (y & 1); x < (2 * w); x += 2)
			{
				OrthogonalPoint	cur(ul);
				cur.Move(MapPointData(x, y));
				if (cur.IsValid())
				{
					DrawHitMask(destSurf, cur.GetRC());
				}
			}
		}
	}

	DrawHitMask(destSurf, m_hiliteMouseTile);
}

sint32 TiledMap::RecalculateViewRect(RECT &myRect)
{

	CalculateZoomViewRectangle(GetZoomLevel(), myRect);

	return(0);
}

sint32 TiledMap::CalculateMetrics()
{
	sint32			w = m_displayRect.right - m_displayRect.left;
	sint32			h = m_displayRect.bottom - m_displayRect.top;

	m_mapViewRect.left = 0;
	m_mapViewRect.top = 0;
	m_mapViewRect.right = (sint32)((w-(GetZoomTilePixelWidth()/2))/(GetZoomTilePixelWidth()));
	m_mapViewRect.bottom = (sint32)((h-GetZoomTileHeadroom())/(GetZoomTilePixelHeight()/2)-1);

	return 0;
}








sint32 g_is_debug_map_color = 0;
extern uint16 myRGB(sint32 r,  sint32 g, sint32 b);

sint32 TiledMap::CalculateWrap
(
    aui_Surface *   surface,
    sint32          i,
    sint32          j
)
{
	Assert(m_localVision != nullptr);

	maputils_WrapPoint(j, i, &j, &i);
	MapPoint tempPos    = MapPoint(maputils_TileX2MapX(j, i), i);

	if (!ReadyToDraw() || (!m_renderEverything && !m_localVision->IsExplored(tempPos)))
	{
		BlackTile(surface, &tempPos);
		return 0;
	}

	MapPoint	pos = tempPos;
	sint32		x;
	sint32      y;
	maputils_MapXY2PixelXY(pos.x,pos.y,&x,&y);

	if (    (x < m_surfaceRect.left)
	     || (x > (m_surfaceRect.right - GetZoomTilePixelWidth()))
	     || (y < m_surfaceRect.top)
	     || (y > (m_surfaceRect.bottom - (GetZoomTilePixelHeight() + GetZoomTileHeadroom())))
	   )
	{
		// Outside displayed surface
		return 0;
	}

	TileInfo * tileInfo = GetTileInfo(pos);
	if (tileInfo == nullptr) return -1;

	BaseTile * baseTile = m_tileSet->GetBaseTile(tileInfo->GetTileNum());
	if (baseTile == nullptr) return -1;

	sint32  terrainType;
	bool    fog = !m_renderEverything && !m_renderExploredAsVisible
	              && !m_localVision->IsVisible(tempPos)
	              && !GpuFogActive();   // P11 C: GPU fog composites the mask instead
	if (fog)
	{
		UnseenCellCarton ucell;
		if (m_localVision->GetLastSeen(tempPos, ucell))
		{
			terrainType = ucell.m_unseenCell->GetTerrainType();
		}
		else
		{
			terrainType = world_Get()->GetTerrain(tempPos.x,tempPos.y);
		}
	}
	else
	{
		terrainType = world_Get()->GetTerrain(tempPos.x, tempPos.y);
	}

	sint16		river = tileInfo->GetRiverPiece();

	if (m_zoomLevel == k_ZOOM_LARGEST)
	{
		if (!fog)
		{
			DrawTransitionTile(surface, pos, x, y);

			if (river != -1)
				DrawOverlay(surface, m_tileSet->GetRiverData(river), x, y);
		}
		else
		{
			if (g_isFastCpu) {

				DrawBlendedTile(surface, pos,x,y,k_FOW_COLOR,k_FOW_BLEND_VALUE);
				if (river != -1)
					DrawBlendedOverlay(surface, m_tileSet->GetRiverData(river),x,y,k_FOW_COLOR,k_FOW_BLEND_VALUE);
			} else {

				DrawDitheredTile(surface, x,y,k_FOW_COLOR);

				if (river != -1)
					DrawDitheredOverlay(surface, m_tileSet->GetRiverData(river),x,y,k_FOW_COLOR);
			}
		}

		if (g_isGridOn)
			DrawTileBorder(surface, x,y,(colorset_Get()->GetColor(COLOR_BLACK)));


		AddDirtyToMap(x, y, k_TILE_PIXEL_WIDTH, k_TILE_GRID_HEIGHT);

	} else {

		if (!fog) {

			DrawTransitionTileScaled(surface, pos, x, y, GetZoomTilePixelWidth(),
														GetZoomTilePixelHeight());

			if (river != -1)
				DrawScaledOverlay(surface, m_tileSet->GetRiverData(river),
									x, y, GetZoomTilePixelWidth(), GetZoomTileGridHeight());
		} else {

			if (g_isFastCpu) {

				DrawBlendedTileScaled(surface, pos, x, y, GetZoomTilePixelWidth(), GetZoomTilePixelHeight(),
										k_FOW_COLOR,k_FOW_BLEND_VALUE);

				if (river != -1)
					DrawBlendedOverlayScaled(surface, m_tileSet->GetRiverData(river),
								x, y, GetZoomTilePixelWidth(), GetZoomTileGridHeight(),
								k_FOW_COLOR, k_FOW_BLEND_VALUE);
			} else {

				DrawDitheredTileScaled(surface, pos, x, y, GetZoomTilePixelWidth(),
										GetZoomTilePixelHeight(),k_FOW_COLOR);

				if (river != -1)
					DrawDitheredOverlayScaled(surface, m_tileSet->GetRiverData(river),
								x, y, GetZoomTilePixelWidth(), GetZoomTileGridHeight(),
								k_FOW_COLOR);
			}
		}

		if (g_isGridOn)
			DrawTileBorderScaled(surface, pos, x, y, GetZoomTilePixelWidth(), GetZoomTilePixelHeight(), colorset_Get()->GetColor(COLOR_BLACK));




		AddDirtyToMap(x, y, GetZoomTilePixelWidth(), GetZoomTileGridHeight());
	}

	if (graphicsoptions_Get())
	{
		if (graphicsoptions_Get()->IsCellTextOn())
		{
			CellText *cellText = graphicsoptions_Get()->GetCellText(pos);
			if (cellText != nullptr)
			{
				sint32 r;
				sint32 g;
				sint32 b;
				ColorMagnitudeToRGB(cellText->m_color, &r, &g, &b);

				COLORREF fgColor = RGB(r, g, b);
				COLORREF bgColor = RGB(0, 0, 0);

				DrawSomeText(false,
							 cellText->m_text,
							 x + GetZoomTilePixelWidth()/2,
							 y + GetZoomTilePixelHeight(),
							 bgColor,
							 fgColor
							);
			}
		}
	}

#if defined(_DEBUG) && defined(CELL_COLOR)

	if (g_is_debug_map_color)
	{
		sint32 color = world_Get()->GetColor(pos);
		if (0 < color)
		{
			sint32 r=0;
			sint32 g=0;
			sint32 b;
			b = color * 2;
			if (127 < b) {
				g = b - 127;
				b = 0;
			}
			if (127 < g) {
				r = g - 127;
				g = 0;
			}

			uint16 c = myRGB(r, g, b);
			DrawNumber(surface,
			           color,
			           c,
			           (sint32)(x+(k_TILE_PIXEL_WIDTH*m_scale)/2),
			           (sint32)(y+(k_TILE_PIXEL_HEIGHT*m_scale))
			          );
		}
	}
#endif

	return 0;
}

sint32 TiledMap::CalculateWrapClipped(
			aui_Surface *surface,
			sint32 i,
			sint32 j
			)
{
	Assert(m_localVision != nullptr);

	sint32 drawx = j;
	maputils_WrapPoint(j,i,&j,&i);

	sint32 drawy = i;
	maputils_TileX2MapXAbs(drawx,drawy,&drawx);

	MapPoint tempPos = MapPoint(maputils_TileX2MapX(j, i), i);

	if (!m_localVision->IsExplored(tempPos) || !ReadyToDraw())
	{
		return 0;
	}

	sint32	terrainType;

	bool    fog = !m_localVision->IsVisible(tempPos) && !GpuFogActive();

	if (fog)
	{
		UnseenCellCarton ucell;

		if (m_localVision->GetLastSeen(tempPos, ucell))
        {
			terrainType = ucell.m_unseenCell->GetTerrainType();
        }
		else
        {
			terrainType = world_Get()->GetTerrain(tempPos.x,tempPos.y);
        }
	}
	else
	{
		terrainType = world_Get()->GetTerrain(tempPos.x, tempPos.y);
	}

	MapPoint    pos = tempPos;

	maputils_MapXY2PixelXY(drawx,drawy,&drawx,&drawy);

	sint32      baseX;
	sint32      baseY;
	maputils_TileX2MapXAbs(m_mapViewRect.left,m_mapViewRect.top,&baseX);

	maputils_MapXY2PixelXY(baseX,m_mapViewRect.top,&baseX,&baseY);

	TileInfo *tileInfo = GetTileInfo(pos);

	if (tileInfo == nullptr)
		return -1;

	BaseTile *baseTile = m_tileSet->GetBaseTile(tileInfo->GetTileNum());

	if (baseTile == nullptr)
		return -1;

	if (m_zoomLevel == k_ZOOM_LARGEST)
	{
		if (!fog)
		{
		   	DrawTransitionTileClipped(surface, pos, drawx, drawy);
		}

		AddDirtyToMap(drawx, drawy, k_TILE_PIXEL_WIDTH, k_TILE_GRID_HEIGHT);
	}
	else
	{
		sint16		river = tileInfo->GetRiverPiece();

		if (!fog)
		{
			if (river != -1)
				DrawScaledOverlay(surface, m_tileSet->GetRiverData(river),
									drawx, drawy, GetZoomTilePixelWidth(), GetZoomTileGridHeight());
		}
		else
		{
			if (g_isFastCpu)
			{

				DrawBlendedTileScaled(surface, pos, drawx, drawy, GetZoomTilePixelWidth(), GetZoomTilePixelHeight(),
										k_FOW_COLOR,k_FOW_BLEND_VALUE);

				if (river != -1)
					DrawBlendedOverlayScaled(surface, m_tileSet->GetRiverData(river),
								drawx, drawy, GetZoomTilePixelWidth(), GetZoomTileGridHeight(),
								k_FOW_COLOR, k_FOW_BLEND_VALUE);
			}
			else
			{

				DrawDitheredTileScaled(surface, pos, drawx, drawy, GetZoomTilePixelWidth(),
										GetZoomTilePixelHeight(),k_FOW_COLOR);

				if (river != -1)
					DrawDitheredOverlayScaled(surface, m_tileSet->GetRiverData(river),
								drawx, drawy, GetZoomTilePixelWidth(), GetZoomTileGridHeight(),
								k_FOW_COLOR);
			}
		}

		if (g_isGridOn)
			DrawTileBorderScaled(surface, pos, drawx, drawy, GetZoomTilePixelWidth(), GetZoomTilePixelHeight(), colorset_Get()->GetColor(COLOR_BLACK));

		AddDirtyToMap(drawx, drawy, GetZoomTilePixelWidth(), GetZoomTileGridHeight());
	}

	return 0;
}

sint32 TiledMap::DrawImprovements(aui_Surface *surface,
			sint32 i,
			sint32 j,
			bool clip
			)
{
	maputils_WrapPoint(j,i,&j,&i);

	MapPoint            tempPos = MapPoint(maputils_TileX2MapX(j, i), i);
	MapPoint            pos     = tempPos;
	UnseenCellCarton	ucell;
	Cell *              cell;
	uint32				env;
	sint32				numImprovements;
    sint32              numDBImprovements;
	bool				hasGoody = false;

	bool visiblePlayerOwnsThis = selitem_Get()->GetVisiblePlayer() == world_Get()->GetOwner(pos);

// Added by Martin G�hmann
	if(!g_fog_toggle // The sense of toogling off the fog is to see something
	&& !visiblePlayerOwnsThis
	&& m_localVision->GetLastSeen(pos, ucell)
//	&& ucell.m_unseenCell->GetImprovements()->GetCount() > 0
	){
		env = ucell.m_unseenCell->GetEnv();
		numDBImprovements = 0; // Maybe has to be reconsidered
		numImprovements = ucell.m_unseenCell->GetImprovements()->GetCount();
		hasGoody = ucell.m_unseenCell->HasHut();
	}
	else
	{
		cell = world_Get()->GetCell(pos);

		if (cell==nullptr)
		   return 0;

		env = cell->GetEnv();

		numDBImprovements	= cell->GetNumDBImprovements();
		numImprovements		= cell->GetNumImprovements();

		hasGoody = (world_Get()->GetGoodyHut(pos) != nullptr);
	}

	uint32 mask = (k_MASK_ENV_INSTALLATION |
					k_MASK_ENV_MINE |
					k_MASK_ENV_IRRIGATION |
					k_MASK_ENV_ROAD |
					k_MASK_ENV_CANAL_TUNNEL);

	if (!(env & mask) &&
		(numImprovements==0) &&
		(numDBImprovements==0) &&
		!hasGoody)
		return 0;

	sint32		 x;
	sint32		 y;
	maputils_MapXY2PixelXY(pos.x,pos.y,&x,&y);

	if(x < m_surfaceRect.left || x > (m_surfaceRect.right - GetZoomTilePixelWidth()) ||
		y < m_surfaceRect.top || y > (m_surfaceRect.bottom - GetZoomTileGridHeight())) {
		return 0;
	}

	if (DrawImprovementsLayer(surface, pos, x, y,clip))
	{
		if (m_zoomLevel == k_ZOOM_LARGEST)
		{
			AddDirtyToMap(x, y, k_TILE_PIXEL_WIDTH, k_TILE_GRID_HEIGHT);
		}
		else
		{
			AddDirtyToMap(x, y, GetZoomTilePixelWidth(), GetZoomTileGridHeight());
		}
	}

	return 0;
}









void TiledMap::RetargetTileSurface(aui_Surface *surf)
{
	m_surface = (surf) ? surf : m_mapSurface;
}

sint32 TiledMap::RepaintTiles(RECT *repaintRect)
{
	sint32		 mapWidth;
	sint32		 mapHeight;
	GetMapMetrics(&mapWidth,&mapHeight);

	for (sint32 i=repaintRect->top; i<repaintRect->bottom; i++){
		if (world_Get()->IsYwrap() || (i >= 0 && i < mapHeight)) {
			for (sint32 j=repaintRect->left; j<repaintRect->right; j++) {
				if (world_Get()->IsXwrap() || (j >= 0 && j < mapWidth)) {
					CalculateWrap(nullptr,i,j);
				}
			}
		}
	}

	return 0;
}

sint32 TiledMap::RepaintTilesClipped(RECT *repaintRect)
{
	for (sint32 i=repaintRect->top; i<=repaintRect->bottom; i++)
	{
		for (sint32 j=repaintRect->left; j<=repaintRect->right; j++)
			CalculateWrapClipped(nullptr,i,j);
	}

	return 0;
}

sint32 TiledMap::RepaintImprovements(RECT *repaintRect,bool clip)
{
	sint32		 mapWidth;
	sint32		 mapHeight;
	sint32		i;

	GetMapMetrics(&mapWidth,&mapHeight);

	for (i=repaintRect->top; i<repaintRect->bottom; i++){
		if (world_Get()->IsYwrap() || (i >= 0 && i < mapHeight)) {
			for (sint32 j=repaintRect->left; j<repaintRect->right; j++) {
				if (world_Get()->IsXwrap() || (j >= 0 && j < mapWidth)) {
					DrawImprovements(nullptr,i,j,false);
				}
			}
		}
	}

	return 0;
}

sint32 TiledMap::CalculateHatWrap(
			aui_Surface *surface,
			sint32 i,
			sint32 j
			)
{
	maputils_WrapPoint(i,j,&i,&j);

	sint32 mapX = maputils_TileX2MapX(i,j);

	MapPoint pos(mapX, j);

	TileInfo *tileInfo;

	bool    fog = !m_localVision->IsVisible(pos) && !GpuFogActive();
	if (fog)
	{
		UnseenCellCarton ucell;
		if(m_localVision->GetLastSeen(pos, ucell)) {
			tileInfo = ucell.m_unseenCell->GetTileInfo();
		} else {
			tileInfo = GetTileInfo(pos);
		}
	} else {
		tileInfo = GetTileInfo(pos);
	}

	sint32		 x;
	sint32		 y;
	maputils_MapXY2PixelXY(pos.x,pos.y,&x,&y);

	if (tileInfo == nullptr) return -1;

	BaseTile *baseTile = m_tileSet->GetBaseTile(tileInfo->GetTileNum());
	if (baseTile == nullptr) return -1;

	if (m_zoomLevel == k_ZOOM_LARGEST) {

		if (!fog) {
			DrawOverlay(surface, baseTile->GetHatData(), x, y);
		} else {
			if (g_isFastCpu)
				DrawBlendedOverlay(surface, baseTile->GetHatData(),x,y,k_FOW_COLOR,k_FOW_BLEND_VALUE);
			else
				DrawDitheredOverlay(surface, baseTile->GetHatData(),x,y,k_FOW_COLOR);
		}

		AddDirtyToMap(x, y, k_TILE_PIXEL_WIDTH, k_TILE_GRID_HEIGHT);
	} else {

		if (!fog) {
			DrawScaledOverlay(surface, baseTile->GetHatData(), x, y, GetZoomTilePixelWidth(), GetZoomTileGridHeight());
		} else {
			if (g_isFastCpu) {
				DrawBlendedOverlayScaled(surface, baseTile->GetHatData(), x, y, GetZoomTilePixelWidth(), GetZoomTileGridHeight(),k_FOW_COLOR,k_FOW_BLEND_VALUE);
			} else {
				DrawDitheredOverlayScaled(surface, baseTile->GetHatData(), x, y, GetZoomTilePixelWidth(), GetZoomTileGridHeight(),k_FOW_COLOR);
			}
		}

		AddDirtyToMap(x, y, GetZoomTilePixelWidth(), GetZoomTileGridHeight());

	}
	return 0;
}


sint32 TiledMap::RepaintHats(RECT *repaintRect,bool clip)
{

	sint32 mapWidth;
	sint32 mapHeight;
	GetMapMetrics(&mapWidth, &mapHeight);

	RECT tempRect = *repaintRect;

	for (sint32 j = tempRect.top;j < tempRect.bottom;j++)
	{
		if (world_Get()->IsYwrap() || ((j >= 0) && (j <= mapHeight)) || clip)
		{
			for (sint32 i = tempRect.left;i<=tempRect.right;i++)
			{
				if (world_Get()->IsXwrap() || (i >= 0 && i < mapWidth)||clip)
					RedrawHat(nullptr, j,i,clip);
			}
		}
	}

	return 0;
}

sint32 TiledMap::RepaintBorders(RECT *repaintRect, bool clip)
{
	sint32 mapWidth;
	sint32 mapHeight;
	GetMapMetrics(&mapWidth, &mapHeight);

	RECT tempRect = *repaintRect;

	for (sint32 j = tempRect.top;j < tempRect.bottom;j++)
	{
		if (world_Get()->IsYwrap() || ((j >= 0) && (j <= mapHeight)) || clip)
		{
			for (sint32 i = tempRect.left;i<=tempRect.right;i++)
			{
				if (world_Get()->IsXwrap() || (i >= 0 && i < mapWidth)||clip)
					RedrawBorders(nullptr, j,i,clip);
			}
		}
	}

	return 0;
}







sint32 TiledMap::RepaintEdgeX(RECT *repaintRect)
{
	sint32 mapWidth;
	sint32 mapHeight;
	GetMapMetrics(&mapWidth,&mapHeight);

	if (repaintRect->left < 0) {

		RECT erase = {0,0,(sint32)((k_TILE_PIXEL_WIDTH+k_TILE_PIXEL_WIDTH/2)*m_scale),m_surface->Height()};
		primitives_PaintRect16(m_surface,&erase,0x0000);

		for (sint32 i=m_mapViewRect.top; i<m_mapViewRect.bottom; i++){
			if (world_Get()->IsYwrap() || (i>=0 && i < mapHeight)) {
				for (sint32 j=0; j<1; j++) {
					if (world_Get()->IsXwrap() || (j >= 0 && j < mapWidth)) {
						CalculateWrap(nullptr,i,j);
					}
				}
			}
		}
	}
	if (repaintRect->right > m_mapBounds.right-1) {

		RECT erase = {m_surface->Width() - (sint32)((repaintRect->right-(m_mapBounds.right-2))*k_TILE_PIXEL_WIDTH*m_scale),0,m_surface->Width(),m_surface->Height()};
		primitives_PaintRect16(m_surface,&erase,0x0000);

		for (sint32 i=m_mapViewRect.top; i<m_mapViewRect.bottom; i++){
			if (world_Get()->IsYwrap() || (i>=0 && i < mapHeight)) {
				for (sint32 j=m_mapBounds.right-2; j<m_mapBounds.right; j++) {
					if (world_Get()->IsXwrap() || (j >=0 && j < mapWidth)) {
						CalculateWrap(nullptr,i,j);
					}
				}
			}
		}
	}
	return 0;
}







sint32 TiledMap::RepaintEdgeY(RECT *repaintRect)
{
	sint32 mapWidth;
	sint32 mapHeight;
	GetMapMetrics(&mapWidth,&mapHeight);

	if (repaintRect->top < 0) {

		RECT erase = {0,0,m_surface->Width(),(sint32)(k_TILE_PIXEL_HEIGHT*2*m_scale)};
		primitives_PaintRect16(m_surface,&erase,0x0000);

		for (sint32 i=0; i<1; i++) {
			if (world_Get()->IsYwrap() || (i >= 0 && i < mapHeight)) {
				for (sint32 j=m_mapViewRect.left; j<m_mapViewRect.right; j++) {
					if (world_Get()->IsXwrap() || (j >= 0 && j < mapWidth)) {
						CalculateWrap(nullptr,i,j);
					}
				}
			}
		}
	}
	if (repaintRect->bottom > m_mapBounds.bottom) {

		RECT erase = {0,
						m_surface->Height()-(sint32)((k_TILE_PIXEL_HEIGHT*2)*m_scale),
						m_surface->Width(),
						m_surface->Height()};
		primitives_PaintRect16(m_surface,&erase,0x0000);

		for (sint32 i=m_mapBounds.bottom-3; i<m_mapBounds.bottom; i++) {
			if (world_Get()->IsYwrap() || (i >= 0 && i < mapHeight)) {
				for (sint32 j=m_mapViewRect.left; j<m_mapViewRect.right; j++) {
					if (world_Get()->IsXwrap() || (j >= 0 && j < mapWidth)) {
						CalculateWrap(nullptr,i,j);
					}
				}
			}
		}
	}
	return 0;
}


void TiledMap::ColorMagnitudeToRGB(uint8 col, sint32 *r, sint32 *g, sint32 *b)
{
	if (col < 128)
    {
		*r = 255 - (col * 2);
		*g = (col * 2);
		*b = 0;
	}
    else
    {
        // col >= 128
		*r = 0;
		*g = 255 - ((col-128) * 2);
		*b = (col - 128) * 2;
	}
}

void TiledMap::DrawSomeText
(
    bool            mixingPort,
	MBCHAR const *  text,
	sint32          tx,
    sint32          ty,
	COLORREF        bgColorRef,
	COLORREF        fgColorRef
)
{
	if (text == nullptr) return;
	if (strlen(text) < 1) return;
	if (m_font == nullptr) return;

	aui_Surface		*surface;
	if (mixingPort) {
		if (!screenmanager_Get()) return;

		surface = screenmanager_Get()->GetSurface();
		if (!surface) return;
	} else {
		surface = m_mapSurface;
	}

	sint32  width = m_font->GetStringWidth(text);
	sint32  height = m_font->GetMaxHeight();

	tx = tx - (width / 2);
	ty = ty - (height / 2);

	RECT		tempRect = {0, 0, width, height};
	OffsetRect(&tempRect, tx, ty);

	if (mixingPort) {
		screenmanager_Get()->UnlockSurface();
	}

    m_font->DrawString(surface, &tempRect, &tempRect, text, 0, bgColorRef, 0);
	OffsetRect(&tempRect, -1, -1);
	m_font->DrawString(surface, &tempRect, &tempRect, text, 0, fgColorRef, 0);

	tempRect.right++;
	tempRect.bottom++;

	if (mixingPort) {
		screenmanager_Get()->LockSurface(surface);
		AddDirtyRectToMix(tempRect);
	} else {
		AddDirtyRectToMap(tempRect);
	}
}

#define k_UNIT_1_OFFSET_X		10
#define k_UNIT_1_OFFSET_Y		-10
#define k_UNIT_2_OFFSET_X		-10
#define k_UNIT_2_OFFSET_Y		-10
#define k_UNIT_3_OFFSET_X		0
#define k_UNIT_3_OFFSET_Y		0

void TiledMap::PaintArmyActors(MapPoint &pos)
{
#if 0

	Unit		topUnit;
	UnitActor	*unitActor1 = NULL,
				*unitActor2 = NULL,
				*unitActor3 = NULL;

	if (!world_Get()->GetTopVisibleUnit(pos, topUnit)) return;

	unitActor1 = topUnit.GetActor();

	Army		theArmy = topUnit.GetArmy();

	for (sint32 i=0; i<theArmy.Num(); i++) {
		Unit	unit = theArmy.Get(i);
		if (unit == topUnit)
			continue;

		if (unitActor2 == NULL) {
			unitActor2 = unit.GetActor();
		} else if (unitActor3 == NULL) {
			unitActor3 = unit.GetActor();
		}
	}

	sint32	x, y;
	double	scale = tiledmap_Get()->GetZoomScale(k_ZOOM_SMALLEST);

	maputils_MapXY2PixelXY(pos.x, pos.y, &x, &y);

	Pixel16 *icon = m_tileSet->GetMapIconData(MAPICON_DAIS);

	if (icon) {
		DrawColorizedOverlayIntoMix(icon, x, y+24,
						colorset_Get()->GetPlayerColor(theArmy.GetOwner()));
	}

	x += (k_ACTOR_CENTER_OFFSET_X * scale);
	y += (k_ACTOR_CENTER_OFFSET_Y * scale);

	if (unitActor2) {
		unitActor2->DrawDirect(NULL,
								x + k_UNIT_2_OFFSET_X,
								y + k_UNIT_2_OFFSET_Y,
								scale);
	}
	if (unitActor3) {
		unitActor3->DrawDirect(NULL,
								x + k_UNIT_3_OFFSET_X,
								y + k_UNIT_3_OFFSET_Y,
								scale);
	}
	if (unitActor1) {
		unitActor1->DrawDirect(NULL,
								x + k_UNIT_1_OFFSET_X,
								y + k_UNIT_1_OFFSET_Y,
								scale);
	}
#endif

}

#ifndef _PLAYTEST
BOOL g_show_ai_dbg = 0;
#endif

void TiledMap::PaintUnitActor(std::shared_ptr<UnitActor> actor, bool fog)
{
	Assert(actor != nullptr);
	if (actor == nullptr) return;

	if (actor->GetUnitVisibility() & (1 << selitem_Get()->GetVisiblePlayer()))
	{

		if (actor->Draw(fog)) {

			RECT rect;

			actor->GetBoundingRect(&rect);
			AddDirtyRectToMix(rect);
		} else {





		}

		if (
		   (graphicsoptions_Get() && graphicsoptions_Get()->IsArmyTextOn())
		|| (profiledb_Get()->GetDebugAI()) //emod
		){
				Unit	u = actor->GetUnitID();

				if (u.IsValid() && u.GetArmy().m_id != 0) {

				Army		a = u.GetArmy();

				sint32		tx = actor->GetX() + GetZoomTilePixelWidth()/2;
				sint32 		ty = actor->GetY() + GetZoomTileHeadroom();

				uint8		col = a.GetData()->GetDebugStringColor();
				sint32		 r;
				sint32		 g;
				sint32		 b;
				ColorMagnitudeToRGB(col, &r, &g, &b);

				COLORREF	fgColor = RGB(r, g, b);
				COLORREF    bgColor = RGB(0,0,0);

				DrawSomeText(true, a.GetData()->GetDebugString(), tx, ty, fgColor, bgColor);

			}
		}

		//EMOD to allow option for army names
		if (profiledb_Get()->GetShowArmyNames())
		{
			Unit	u = actor->GetUnitID();

			// Shouldn't this be the visible player?
			if(u.IsValid()
			&& u.GetArmy().m_id != 0
			&& player_Get(u.GetOwner())->IsHuman()
			){
				Army		a = u.GetArmy();

				const MBCHAR		*s = a.GetData()->GetName();

				sint32		 tx = (sint32)(actor->GetX()+GetZoomTilePixelWidth()/2);
				sint32		 ty = (sint32)(actor->GetY()+GetZoomTileHeadroom());

				sint32		 r;
				sint32		 g;
				sint32		 b;
				uint8		col = a.GetData()->GetDebugStringColor();

				ColorMagnitudeToRGB(col, &r, &g, &b);

				COLORREF	 fgColor = RGB(r, g, b);
				COLORREF	 bgColor = RGB(0,0,0);

				DrawSomeText(true, s, tx, ty+40, colorset_Get()->GetColorRef(COLOR_BLACK), colorset_Get()->GetColorRef(COLOR_WHITE));
			}
		}
		//end emod

		if (g_show_ai_dbg || (is_scenario_Get() && show_unit_labels_Get()) )
		{
			MapPoint pos = actor->GetPos();

			char text[80];
			text[0] = '\0';

			sint32	 tx = (sint32)(actor->GetX()+(k_TILE_PIXEL_WIDTH*m_scale)/2);
			sint32	 ty = (sint32)(actor->GetY()+(k_TILE_PIXEL_HEIGHT*m_scale));

			Cell *c = world_Get()->GetCell(pos);
			Unit city = c->GetCity();

			if ((city != Unit()) && g_show_ai_dbg)
			{








				strlcpy(text, city.GetName(), sizeof(text));

				DrawSomeText(TRUE, text, tx, ty+10,
								colorset_Get()->GetColorRef(COLOR_YELLOW),
								colorset_Get()->GetColorRef(COLOR_PURPLE));
			}

			CellUnitList *al = c->UnitArmy();
			if (al) {








				strlcpy(text, al->Access(0).GetName(), sizeof(text));

				DrawSomeText(TRUE, text, tx, ty,
					colorset_Get()->GetColorRef(COLOR_BLACK),
					colorset_Get()->GetColorRef(COLOR_WHITE));
			}

		}

	}
}

void TiledMap::PaintGoodActor(GoodActor *actor, bool fog)
{
	Assert(actor != nullptr);
	if (actor == nullptr) return;

	(void) actor->Draw(fog);

	RECT rect;
	actor->GetBoundingRect(&rect);
	AddDirtyRectToMix(rect);
}
















void TiledMap::PaintEffectActor(EffectActor *actor)
{
	Assert(actor != nullptr);
	if (actor == nullptr) return;

	actor->Draw();

	RECT rect;
	actor->GetBoundingRect(&rect);
	AddDirtyRectToMix(rect);
}

sint32 TiledMap::RepaintLayerSprites(RECT *paintRect, sint32 layer)
{
	if(!ReadyToDraw())
		return 0;

#ifdef _DEBUG
	tiledmap_Get()->DrawRectMetrics();
	tiledmap_Get()->RectMetricNewLoop();
#endif

	for (sint32 i=paintRect->top; i<paintRect->bottom; i++) {
		for (sint32 j=paintRect->left; j<paintRect->right; j++) {

			if(!world_Get()->IsXwrap() && (j < 0 || j >= world_Get()->GetXWidth()))
				continue;

			sint32 tileX;
			sint32 tileY;
			maputils_WrapPoint(j,i,&tileX,&tileY);

			sint32      mapY    = tileY;
			sint32      mapX    = maputils_TileX2MapX(tileX,tileY);
			MapPoint    pos     = MapPoint(mapX, mapY);

#if 0
			if(world_Get()->IsCity(pos))
			{
				Unit city=world_Get()->GetCity(pos);

				sint32 pop;

				city.GetPop(pop);
				DrawCityRadius(pos, COLOR_WHITE ,pop);
			}
#endif

			if (world_Get()->IsGood(pos) && m_localVision->IsExplored(pos))
			{
				TileInfo *curTileInfo = GetTileInfo(pos);
				Assert(curTileInfo);
				if (curTileInfo)
				{
                    GoodActor * curGoodActor = curTileInfo->GetGoodActor();
                    if (curGoodActor)
                    {
					    curGoodActor->PositionActor(pos);
					    PaintGoodActor
                            (curGoodActor, !m_localVision->IsVisible(pos));
                    }
				}
			}

			Unit		top;

			if (m_localVision && !m_localVision->IsExplored(pos)
				&& !world_Get()->GetTopVisibleUnit(pos, top))
				continue;

			bool fog =
                (m_localVision && m_localVision->IsExplored(pos) &&
                 !m_localVision->IsVisible(pos)
                );

			UnseenCellCarton		ucell;

			// For visibility god mode and fog of war should be handled equally
			if(!g_fog_toggle
			&&  m_localVision
			&&  m_localVision->GetLastSeen(pos, ucell))
			{

				UnitActorPtr actor = ucell.m_unseenCell->GetActor();

				if (actor)
				{
					PaintUnitActor(actor, fog);
				}
			}
			else
			{


				if (!world_Get()->GetTopVisibleUnit(pos, top)) continue;

				if (g_drawArmyClumps)
                {
                    Army    a   = top.GetArmy();
					if (a.IsValid() && (a.Num() > 1))
                    {
						PaintArmyActors(pos);
						return 0;
					}
				}

				UnitActorPtr actor = top.GetActor();
				if (!actor) continue;

				// SetIsFortifying / SetIsFortified / SetHasCityWalls /
				// SetHasForceField pushes removed — UnitActor::Draw
				// reads m_unitID.IsEntrenching() / IsEntrenched() /
				// HasCityWalls() / HasForceField() directly from gs/.

				actor->SetHiddenUnderStack(FALSE);
				actor->SetUnitVisibility(top.GetVisibility());





				// For visibility god mode and fog of war should be handled equally
				if(!( actor->GetUnitVisibility() & (1 << selitem_Get()->GetVisiblePlayer()))
				&& !g_fog_toggle
				&& !g_god
				&& (player_Get(selitem_Get()->GetVisiblePlayer())
				&& !player_Get(selitem_Get()->GetVisiblePlayer())->m_hasGlobalRadar))
					continue;

				if (top.GetOwner() == selitem_Get()->GetVisiblePlayer()
					&& top.CanSettle(top.RetPos()))
				{
					SELECT_TYPE		selectType;
					ID				selectedID;
					PLAYER_INDEX	selectedPlayer;

					selitem_Get()->GetTopCurItem(selectedPlayer, selectedID, selectType);

					Unit		selectedUnit;
					COLOR		color = COLOR_BLACK;

					if(selectType == SELECT_TYPE_LOCAL_CITY)
					{
						selectedUnit = selectedID;
						color = COLOR_WHITE;
					}
					else
						if(selectType == SELECT_TYPE_LOCAL_ARMY)
						{
							color = COLOR_GREEN;
							selectedUnit = ((Army)selectedID).GetTopVisibleUnit(selectedPlayer);
						}

					if (selectedUnit.m_id == top.m_id)
						DrawCityRadius(top.RetPos(), color);
				}

			   	if (actor->IsActive())
			   	{
					Unit	second;
					if (world_Get()->GetSecondUnit(pos, second))
					{
						top = second;
						actor = top.GetActor();
					}
			   	}

				MapPoint actorCurPos = actor->GetPos();
				if (actor->IsActive())
                {
				    // No action: already busy
                }
                else
                {
					PaintUnitActor
                        (actor, !m_localVision->IsVisible(actor->GetPos()));
				}

				if (top.IsCity()) {

					Unit hypotheticalUnit;

					if (!world_Get()->GetTopVisibleUnitNotCity(pos, hypotheticalUnit)) {


						PLAYER_INDEX	s_player;
						ID				s_item;
                        SELECT_TYPE		s_state;

						selitem_Get()->GetTopCurItem(s_player, s_item, s_state);

						if (s_player != top.GetOwner())
							continue;

						if (s_state != SELECT_TYPE_LOCAL_ARMY &&
							s_state != SELECT_TYPE_LOCAL_ARMY_UNLOADING)
							continue;

						Army		army(s_item);
						if (!army.IsValid())
							continue;

						MapPoint	armyPos;
						army.GetPos(armyPos);

						if (armyPos != top.RetPos())
							continue;

						top = army.Access(0);
					} else {
						top = hypotheticalUnit;
					}

					// For visibility god mode and fog of war should be handled equally
					if (top.GetOwner() != selitem_Get()->GetVisiblePlayer()
					&& !g_fog_toggle
					&& !g_god)
						continue;

					actor = top.GetActor();
					if(!actor) continue;




					if (!(actor->GetUnitVisibility() & (1 << selitem_Get()->GetVisiblePlayer())))
						continue;

					if (!actor->IsActive() && TileIsVisible(actorCurPos.x, actorCurPos.y))
                    {
						actor->SetHiddenUnderStack(FALSE);
						PaintUnitActor
                            (actor, !m_localVision->IsVisible(actor->GetPos()));
					}
				}
			}
		}
	}
	return 0;
}




void
TiledMap::ProcessUnit(Unit unit)
{
    if (unit.IsValid())
    {
        UnitActorPtr actor = unit.GetActor();
        if (actor && !actor->IsActive())
        {
            actor->Process();
        }
    }
}

void
TiledMap::ProcessUnit(CellUnitList * a_List)
{
	if (a_List)
    {
    	for (sint32 index = 0; index < a_List->Num(); index++)
	    	ProcessUnit(a_List->Get(index));
    }
}

void TiledMap::ProcessLayerSprites(RECT *paintRect, sint32 layer)
{
	if(!ReadyToDraw())
		return;

	UnseenCellCarton	ucell;
	sint32				 tileX;
	sint32				 tileY;
	sint32				 i;
	sint32				 j;
	sint32				 mapX;
	sint32				 mapY;
	MapPoint			pos;
	TileInfo			*curTileInfo;
	GoodActor			*curGoodActor;
	UnitActorPtr curUnitActor;
	Unit				unit;
	Cell				*CurrentCell=nullptr;

	for (i=paintRect->top; i<paintRect->bottom; i++)
	{
		for (j=paintRect->left; j<paintRect->right; j++)
		{
			maputils_WrapPoint(j,i,&tileX,&tileY);

			mapY = tileY;
			mapX = maputils_TileX2MapX(tileX,tileY);

			pos.x = (sint16)mapX;
			pos.y = (sint16)mapY;

			curTileInfo = GetTileInfo(pos);

			Assert(curTileInfo != nullptr);

			CurrentCell = world_Get()->GetCell(pos);

			if(world_Get()->IsGood(pos) && m_localVision->IsExplored(pos))
			{
				if(curTileInfo && curTileInfo->HasGoodActor())
				{
					curGoodActor = curTileInfo->GetGoodActor();


					if (world_Get()->GetCity(pos).m_id == 0)
						curGoodActor->Process();
				}
			}




// Added by Martin G�hmann
			// We want to something when we lift the fog of war
			if(!g_fog_toggle
			&&  m_localVision
			&&  m_localVision->GetLastSeen(pos, ucell))
			{

				curUnitActor = ucell.m_unseenCell->GetActor();

				if (curUnitActor)
					curUnitActor->Process();
			}
			else
			{
				ProcessUnit(CurrentCell->GetCity());
				ProcessUnit(CurrentCell->UnitArmy());
#if 0

				unit = world_Get()->GetCell(pos)->GetCity();

				if (unit.IsValid())
				{
					curUnitActor = unit.GetActor();

					if (curUnitActor)
						curUnitActor->Process();
				}

				CellUnitList * unitList = world_Get()->GetCell(pos)->UnitArmy();

				if (unitList)
				{
					for (index=0; index < unitList->Num(); index++)
					{
						unit= unitList->Get(index);

						if (unit.IsValid())
						{
							curUnitActor = unit.GetActor();

							if (curUnitActor)
							{
								if (!curUnitActor->IsActive())
									curUnitActor->Process();
							}
						}
					}
				}
#endif
			}
		}
	}
}

sint32 TiledMap::OffsetLayerSprites(RECT *paintRect, sint32 deltaX, sint32 deltaY, sint32 layer)
{
	if(!ReadyToDraw())
		return 0;

	for (sint32 i=paintRect->top; i<paintRect->bottom; i++) {
		for (sint32 j=paintRect->left; j<paintRect->right; j++) {

			sint32 tileX;
			sint32 tileY;
			maputils_WrapPoint(j,i,&tileX,&tileY);

			sint32 mapX;
			sint32 mapY = tileY;
			mapX = maputils_TileX2MapX(tileX,tileY);

			MapPoint pos;

			pos.x = (sint16)mapX; pos.y = (sint16)mapY;

			sint32 pixelX;
			sint32 pixelY;

			maputils_MapXY2PixelXY(mapX, mapY, &pixelX, &pixelY);

			TileInfo *curTileInfo = GetTileInfo(pos);
			Assert(curTileInfo != nullptr);

			if(world_Get()->IsGood(pos) && m_localVision->IsExplored(pos)) {
				GoodActor *curGoodActor;
				if(curTileInfo && curTileInfo->HasGoodActor()) {
					curGoodActor = curTileInfo->GetGoodActor();

					curGoodActor->SetX(pixelX);
					curGoodActor->SetY(pixelY);
				}
			}

			if (m_localVision && !m_localVision->IsExplored(pos))
				continue;

			UnseenCellCarton		ucell;
			if (m_localVision && m_localVision->GetLastSeen(pos, ucell)) {

				UnitActorPtr actor = ucell.m_unseenCell->GetActor();

				if (actor) {
					actor->SetX(pixelX);
					actor->SetY(pixelY);
				}
			} else {
				Unit		top;

				if (!world_Get()->GetTopVisibleUnit(pos, top)) continue;

				UnitActorPtr actor = top.GetActor();

				if (actor->IsActive()) {
					Unit	second;
					if (world_Get()->GetSecondUnit(pos, second)) {
						top = second;
						actor = top.GetActor();
					}
				}

				MapPoint actorCurPos = actor->GetPos();
				if (!actor->IsActive()) {
					if (actor) {
						actor->SetX(pixelX);
						actor->SetY(pixelY);
					}
				}

				if (top.IsCity()) {

					if (!world_Get()->GetTopVisibleUnitNotCity(pos, top)) continue;

					actor = top.GetActor();

					if (!actor->IsActive()) {
						if (actor) {
							actor->SetX(pixelX);
							actor->SetY(pixelY);
						}
					}
				}
			}
		}
	}
	return 0;
}

sint32 TiledMap::OffsetSprites(RECT *paintRect, sint32 deltaX, sint32 deltaY)
{
	OffsetLayerSprites(paintRect, deltaX, deltaY, 0);

	director_Get()->OffsetActiveUnits(-deltaX, -deltaY);
	director_Get()->OffsetActiveEffects(-deltaX, -deltaY);
	director_Get()->OffsetTradeRouteAnimations(-deltaX, -deltaY);

	return 0;
}

sint32 TiledMap::RepaintSprites(aui_Surface *surf, RECT *paintRect, bool scrolling)
{
	if(!ReadyToDraw())
		return 0;

	sint32	 mapWidth;
	sint32	 mapHeight;
	GetMapMetrics(&mapWidth, &mapHeight);

	Assert(m_localVision);

	if (m_nextPlayer)
	{
		m_nextPlayer = FALSE;
	}

	screenmanager_Get()->LockSurface(surf);

	RepaintLayerSprites(paintRect, 0);
	director_Get()->DrawTradeRouteAnimations(paintRect, 0);
	director_Get()->DrawActiveUnits(paintRect, 0);
	director_Get()->DrawActiveEffects(paintRect, 0);

	if (g_spriteEditWindow)
		g_spriteEditWindow->DrawSprite();

	tiledmap_Get()->DrawTerrainOverlay(surf);

	if (!scrolling)
	{
		DrawChatText();
	}

	screenmanager_Get()->UnlockSurface();

	if (profiledb_Get()->GetShowCityNames())
	{
		tiledmap_Get()->DrawCityNames(surf, 0);
	}

	if (ScenarioEditor::ShowStartFlags())
	{
		tiledmap_Get()->DrawStartingLocations(surf, 0);
	}

	return 0;
}

void TiledMap::DrawStartingLocations(aui_Surface *surf, sint32 layer)
{
	if (ScenarioEditor::GetStartLocMode() == SCEN_START_LOC_MODE_NONE)
		return;

	Pixel16		*icon = GetTileSet()->GetMapIconData(MAPICON_FLAG);
	POINT		iconDim = GetTileSet()->GetMapIconDimensions(MAPICON_FLAG);

	MBCHAR		labelString[MAX_PATH];

	SCEN_START_LOC_MODE mode = ScenarioEditor::GetStartLocMode();

	for (sint32 i=0; i<world_Get()->GetNumStartingPositions(); i++) {
		MapPoint		pos;
		sint32			playerOrCiv;

		pos = world_Get()->GetStartingPoint(i);


		if (mode == SCEN_START_LOC_MODE_CIV) {
			playerOrCiv = world_Get()->GetStartingPointCiv(i);
		} else {

			playerOrCiv = i+1;
		}

		if (TileIsVisible(pos.x, pos.y)) {

			sint32		 x;
			sint32		 y;

			maputils_MapXY2PixelXY(pos.x,pos.y,&x,&y);

			Pixel16		pixelColor;
			COLORREF	colorRef;

			if (mode == SCEN_START_LOC_MODE_PLAYER ||
				mode == SCEN_START_LOC_MODE_PLAYER_WITH_CIV) {

				pixelColor = colorset_Get()->GetPlayerColor(playerOrCiv);
				colorRef = colorset_Get()->GetColorRef(colorset_Get()->ComputePlayerColor(playerOrCiv));
			} else
			if (mode == SCEN_START_LOC_MODE_CIV) {

				pixelColor = colorset_Get()->GetColor(COLOR_WHITE);
				colorRef = colorset_Get()->GetColorRef(COLOR_WHITE);
			} else {
				Assert(FALSE);
				return;
			}

			sint32 destX;
			sint32 destY;

			destX = x + (GetZoomTilePixelWidth()/2) - (iconDim.x / 2);
			destY = y + (GetZoomTileGridHeight()/2) - (iconDim.y / 2);

			DrawColorizedOverlay(icon, surf, destX, destY, pixelColor);

			AddDirtyToMix(destX, destY, iconDim.x, iconDim.y);

			ScenarioEditor::GetLabel(labelString, sizeof(labelString), playerOrCiv);

			if (m_font) {
				RECT		 rect;
				RECT		 clipRect;
				RECT		 boxRect;

				sint32 width = m_font->GetStringWidth(labelString);
				sint32 height = m_font->GetMaxHeight();

				rect.left = x;
				rect.top = y;
				rect.right = x+width;
				rect.bottom = y+height;

				boxRect = rect;

				InflateRect(&boxRect, 2, 1);

				clipRect = boxRect;

				if (clipRect.left < 0) clipRect.left = 0;
				if (clipRect.top < 0) clipRect.top = 0;
				if (clipRect.right >= surf->Width()) clipRect.right = surf->Width() - 1;
				if (clipRect.bottom >= surf->Height()) clipRect.bottom = surf->Height() - 1;

				primitives_PaintRect16(surf, &clipRect, colorset_Get()->GetColor(COLOR_BLACK));

				InflateRect(&boxRect, 1, 1);

				clipRect = boxRect;

				if (clipRect.left < 0) clipRect.left = 0;
				if (clipRect.top < 0) clipRect.top = 0;
				if (clipRect.right >= surf->Width()) clipRect.right = surf->Width() - 1;
				if (clipRect.bottom >= surf->Height()) clipRect.bottom = surf->Height() - 1;

				primitives_FrameRect16(surf, &clipRect, colorset_Get()->GetColor(COLOR_GREEN));

				clipRect = rect;

				if (clipRect.left < 0) clipRect.left = 0;
				if (clipRect.top < 0) clipRect.top = 0;
				if (clipRect.right >= surf->Width()) clipRect.right = surf->Width() - 1;
				if (clipRect.bottom >= surf->Height()) clipRect.bottom = surf->Height() - 1;

				m_font->DrawString(surf, &rect, &clipRect, labelString, 0,	colorRef,	0);

				AddDirtyRectToMix(clipRect);
			}
		}
	}
}

sint32 TiledMap::DrawCityRadius(const MapPoint &cpos, COLOR color, sint32 pop)
{
	if (world_Get()->GetCell(cpos)->HasCity())
	{
		return 0; // Following code not used
#if 0
	    Pixel16 pixelColor = colorset_Get()->GetColor(color);
		CityInfluenceIterator it(cpos, world_Get()->GetCity(cpos).CD()->GetSizeIndex());

		for(it.Start(); !it.End(); it.Next()) {
			MapPoint neighbor;
			Cell *cell = world_Get()->GetCell(it.Pos());

			if(it.Pos().GetNeighborPosition(NORTHWEST, neighbor)) {
				if(world_Get()->GetCell(neighbor)->GetCityOwner().m_id !=
				   cell->GetCityOwner().m_id) {
					DrawColoredHitMaskEdge(screenmanager_Get()->GetSurface(), it.Pos(), pixelColor, NORTHWEST);
				}
			}

			if(it.Pos().GetNeighborPosition(SOUTHWEST, neighbor)) {
				if(world_Get()->GetCell(neighbor)->GetCityOwner().m_id !=
				   cell->GetCityOwner().m_id) {
					DrawColoredHitMaskEdge(screenmanager_Get()->GetSurface(), it.Pos(), pixelColor, SOUTHWEST);
				}
			}

			if(it.Pos().GetNeighborPosition(NORTHEAST, neighbor)) {
				if(world_Get()->GetCell(neighbor)->GetCityOwner().m_id !=
				   cell->GetCityOwner().m_id) {
					DrawColoredHitMaskEdge(screenmanager_Get()->GetSurface(), it.Pos(), pixelColor, NORTHEAST);
				}
			}

			if(it.Pos().GetNeighborPosition(SOUTHEAST, neighbor)) {
				if(world_Get()->GetCell(neighbor)->GetCityOwner().m_id !=
				   cell->GetCityOwner().m_id) {
					DrawColoredHitMaskEdge(screenmanager_Get()->GetSurface(), it.Pos(), pixelColor, SOUTHEAST);
				}
			}

		}
#endif
	}
	else
	{
		DrawCityRadius1(cpos, color);
	}

	return 0;
}

//----------------------------------------------------------------------------
//
// Name       : TiledMap::DrawCityRadius1
//
// Description: Draw a "colored hit mask" in a radius of 1 around a city.
//
// Parameters : cpos    : city location on the map
//              color   : color to use when drawing
//
// Globals    : screenmanager_Get()
//
// Returns    : sint32  : useless value, always 0
//
// Remark(s)  : The tile NORTH of the city is not drawn.
//              The tile of the city itself is drawn.
//              TODO: check whether this is intentional, or the original code
//                    was just wrong.
//
//----------------------------------------------------------------------------
sint32 TiledMap::DrawCityRadius1(const MapPoint &cpos, COLOR color)
{
	for (int dir = NORTH; dir <= NOWHERE; ++dir)
	{
		OrthogonalPoint	neighbour(cpos);
		neighbour.Move(WORLD_DIRECTION(dir));
		if (neighbour.IsValid())
		{
			DrawColoredHitMask
				(screenmanager_Get()->GetSurface(), neighbour.GetRC(), color);
		}
	}

	return 0;
}


sint32 TiledMap::PaintColoredTile(sint32 x, sint32 y, COLOR color)
{
	uint8			*surfBase;
	sint32			surfWidth;
	sint32			surfHeight;
	sint32			surfPitch;
	aui_Surface		*surface;

	surface = screenmanager_Get()->GetSurface();

	surfBase = screenmanager_Get()->GetSurfBase();
	surfWidth = screenmanager_Get()->GetSurfWidth();
	surfHeight = screenmanager_Get()->GetSurfHeight();
	surfPitch = screenmanager_Get()->GetSurfPitch();

	bool const   bpp32 = surface && surface->BitsPerPixel() == 32;
	sint32 const step  = bpp32 ? 4 : 2;
	uint8	*destPixel;

	y+=k_TILE_PIXEL_HEADROOM;

if (x < 0) return 0;
if (x >= surface->Width() - k_TILE_PIXEL_WIDTH) return 0;
if (y < 0) return 0;
if (y >= surface->Height() - k_TILE_PIXEL_HEIGHT) return 0;

	sint32 startX;
	sint32 endX;

	Pixel16		pixelColor = colorset_Get()->GetColor(color);

	for(sint32 j=0; j<k_TILE_PIXEL_HEIGHT; j++) {
		if (j<=23) {
			startX = (23-j)*2;
		} else {
			startX = (j-24)*2;
		}
		endX = k_TILE_PIXEL_WIDTH - startX;

		destPixel = surfBase + ((y + j) * surfPitch) + ((x+startX) * step);

		for (sint32 i=startX; i<endX; i++) {
			if (bpp32) {
				Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel);
				*d = pixelutils_BlendFast8888(*d, pixelutils_16to8888(pixelColor), 20);
			} else {
				Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel);
				*d = pixelutils_BlendFast(*d, pixelColor, 20);
			}
			destPixel += step;
		}
	}

	AddDirtyToMix(x, y, k_TILE_PIXEL_WIDTH, k_TILE_PIXEL_HEIGHT);

	return 0;
}


bool TiledMap::GpuFogActive() const
{
	return c3ui_Get() && c3ui_Get()->GpuFog();
}

sint32 TiledMap::Refresh()
{
	// Headless builds construct a TiledMap with no rendering surface.
	// Refresh is purely a render-pass; no game state lives here.
	if (!m_surface) return AUI_ERRCODE_OK;

	LPVOID      buffer;
	AUI_ERRCODE errcode = m_surface->Lock(nullptr, &buffer, 0);
	Assert(errcode == AUI_ERRCODE_OK);
	if ( errcode == AUI_ERRCODE_OK )
	{
		memset( buffer, 0x00, m_surface->Size() );
		m_surface->Unlock( buffer );
	} else {
		return AUI_ERRCODE_SURFACELOCKFAILED;
	}

	LockSurface();

	if (SmoothScrollAligned())
	{
		RepaintTiles(&m_mapViewRect);
		RepaintHats (&m_mapViewRect);
		RepaintBorders(&m_mapViewRect);
	}
	else
	{
		RECT   altrect=m_mapViewRect;

		altrect.left   -= 1;
		altrect.top    -= 1;
		altrect.right  += 1;
		altrect.bottom -= 1;
		RepaintTilesClipped(&altrect);
		RepaintHats(&altrect,true);
		RepaintBorders(&altrect, true);
	}

	RepaintImprovements(&m_mapViewRect);

	UnlockSurface();

	// P11 Stage 2 C: rebuild the GPU fog mask from the same view, in sync with
	// the world render. Separate surface + lock (not the world map surface), so
	// it runs after UnlockSurface. No-op unless GPU fog is enabled.
	if (c3ui_Get() && c3ui_Get()->GpuFog())
		BuildFogMask(c3ui_Get()->FogSurface());

	// P12: rebuild the GPU world draw list (and fill the atlas on cache misses)
	// from the same view. Runs after UnlockSurface — it does its own scratch-
	// surface locking. CTP2_GPU_QUADS=0 keeps the temporary CPU fallback.
	if (aui_SDL::GpuQuadsEnabled())
		BuildTerrainQuads();

	return 0;
}

// P11 Stage 3 G1 — terrain quad renderer.
//
// Rebuild the per-frame draw list of visible terrain cells as GPU quads. Each
// cell's composited appearance is keyed by TerrainCellSignature; on a cache
// miss the cell is composited exactly once (reusing the legacy tile draw into a
// zoom-sized scratch surface) and uploaded into its atlas slot. On a hit we just
// reference the cached slot. The present (aui_SDLSurface::Flip) then draws each
// quad from the atlas into the world texture. The atlas slot size follows the
// current engine zoom level; changing zoom rebuilds the cache at the new size.
void TiledMap::BuildTerrainQuads()
{
	// Only the singleton main world map drives the (single, global) quad draw
	// list. The radar and thumbnail maps are separate TiledMap instances with
	// their own small views; letting them run would clobber the main map's list
	// (BeginQuadFrame clears it) and blank the world. Guard BEFORE the clear.
	if (this != tiledmap_Get()) return;

	// Clear the list up front so any early return presents an empty world (black)
	// rather than stale quads left at the wrong scale.
	aui_SDL::BeginQuadFrame();
	aui_SDL::BeginSpriteFrame();

	if (!m_tileSet || !m_localVision)   { aui_SDL::MarkQuadFrameIncomplete("world-setup"); return; }

	// Atlas geometry: a cols x rows grid of zoom-sized tile slots. 1024 slots easily
	// holds the distinct edge combinations on a real map (interiors share one
	// signature); LRU absorbs any overflow. Both atlas dims stay < 4096 so any
	// GPU accepts the texture (3008 x 2304).
	int const k_ATLAS_COLS = 32;
	int const k_ATLAS_ROWS = 32;
	int const tileW = GetZoomTilePixelWidth();
	int const tileH = GetZoomTileGridHeight();

	if (!m_gpuTileCache || m_gpuTileCache->TileW() != tileW || m_gpuTileCache->TileH() != tileH)
	{
		m_gpuTileCache = std::make_unique<GpuTileCache>(
			k_ATLAS_COLS, k_ATLAS_ROWS, tileW, tileH);
		AUI_ERRCODE err = AUI_ERRCODE_OK;
		m_gpuScratchTile.reset(aui_Factory::new_Surface(err, tileW, tileH,
			nullptr, FALSE, FALSE, FALSE, /*bpp=*/32));
	}
	if (!m_gpuScratchTile) { aui_SDL::MarkQuadFrameIncomplete("scratch-surface"); return; }

	aui_SDL::EnsureQuadAtlas(m_gpuTileCache->AtlasW(), m_gpuTileCache->AtlasH());

	sint32 mapWidth, mapHeight;
	GetMapMetrics(&mapWidth, &mapHeight);

	sint32 baseX;
	sint32 baseY = m_mapViewRect.top;
	maputils_TileX2MapXAbs(m_mapViewRect.left, m_mapViewRect.top, &baseX);
	maputils_MapXY2PixelXY(baseX, baseY, &baseX, &baseY);

	// Mirror RepaintTiles' visible-cell iteration (m_mapViewRect + wrap/bounds).
	for (sint32 i = m_mapViewRect.top; i < m_mapViewRect.bottom; i++)
	{
		if (!(world_Get()->IsYwrap() || (i >= 0 && i < mapHeight))) continue;
		for (sint32 j = m_mapViewRect.left; j < m_mapViewRect.right; j++)
		{
			if (!(world_Get()->IsXwrap() || (j >= 0 && j < mapWidth))) continue;

			sint32 drawX = j, drawY = i;
			maputils_TileX2MapXAbs(drawX, drawY, &drawX);
			maputils_MapXY2PixelXY(drawX, drawY, &drawX, &drawY);
			drawX -= baseX;
			drawY -= baseY;

			sint32 wj = j, wi = i;
			maputils_WrapPoint(wj, wi, &wj, &wi);
			MapPoint pos = MapPoint(maputils_TileX2MapX(wj, wi), wi);

			// Only explored cells draw terrain (unexplored stays the black the
			// world texture was cleared to — matching CalculateWrap's BlackTile).
			if (!m_renderEverything && !m_localVision->IsExplored(pos)) continue;

			// Same on-surface clip as CalculateWrap.
			if (   (drawX < m_surfaceRect.left)
			    || (drawX > (m_surfaceRect.right  - GetZoomTilePixelWidth()))
			    || (drawY < m_surfaceRect.top)
			    || (drawY > (m_surfaceRect.bottom - (GetZoomTilePixelHeight() + GetZoomTileHeadroom()))))
				continue;

			TileInfo * tileInfo = GetTileInfo(pos);
			if (!tileInfo) continue;
			if (!m_tileSet->GetBaseTile(tileInfo->GetTileNum())) continue;

			sint32 tilesetIndex =
				g_theTerrainDB->Get(tileInfo->GetTerrainType())->GetTilesetIndex();

			uint64_t sig = TerrainCellSignature(
				tileInfo->GetTileNum(),
				(uint8_t) tilesetIndex,
				(uint8_t) tileInfo->GetTransition(0),
				(uint8_t) tileInfo->GetTransition(1),
				(uint8_t) tileInfo->GetTransition(2),
				(uint8_t) tileInfo->GetTransition(3));

			GpuTileSlot slot;
			if (m_gpuTileCache->Get(sig, slot) == GpuTileCache::MISS)
			{
				// Compose this cell once into the scratch tile, then upload it to
				// its atlas slot. Clear scratch to transparent first so the
				// diamond's surround (and headroom) stays alpha 0 and neighbouring
				// quads tessellate cleanly. DrawTransitionTile writes via the
				// locked m_surf* members, so target the scratch through
				// LockThisSurface and place the tile at scratch origin (0,0).
				LockThisSurface(m_gpuScratchTile.get());
				if (m_surfBase)
				{
					memset(m_surfBase, 0, (size_t) m_surfPitch * m_surfHeight);
					if (m_zoomLevel == k_ZOOM_LARGEST)
						DrawTransitionTile(m_gpuScratchTile.get(), pos, 0, 0);
					else
						DrawTransitionTileScaled(m_gpuScratchTile.get(), pos, 0, 0,
							GetZoomTilePixelWidth(), GetZoomTilePixelHeight());
					aui_SDL::UploadQuadAtlasSlot(slot.atlasX, slot.atlasY, tileW, tileH,
						m_surfBase, m_surfPitch);
				}
				UnlockSurface();
			}

			aui_SDL::GpuQuad q;
			q.sx = slot.atlasX; q.sy = slot.atlasY; q.sw = tileW; q.sh = tileH;
			q.dx = drawX + aui_SDL::WorldContentOffX();
			q.dy = drawY + aui_SDL::WorldContentOffY();
			q.dw = tileW; q.dh = tileH;
			aui_SDL::AddQuad(q);
		}
	}

	PLAYER_INDEX const player = selitem_Get()->GetVisiblePlayer();
	double const scale = GetScale();
	for (sint32 i = m_mapViewRect.top; i < m_mapViewRect.bottom; i++)
	{
		if (!(world_Get()->IsYwrap() || (i >= 0 && i < mapHeight))) continue;
		for (sint32 j = m_mapViewRect.left; j < m_mapViewRect.right; j++)
		{
			if (!(world_Get()->IsXwrap() || (j >= 0 && j < mapWidth))) continue;

			sint32 drawX = j, drawY = i;
			maputils_TileX2MapXAbs(drawX, drawY, &drawX);
			maputils_MapXY2PixelXY(drawX, drawY, &drawX, &drawY);
			drawX -= baseX;
			drawY -= baseY;

			sint32 wj = j, wi = i;
			maputils_WrapPoint(wj, wi, &wj, &wi);
			MapPoint pos = MapPoint(maputils_TileX2MapX(wj, wi), wi);
			if (!m_renderEverything && !m_localVision->IsExplored(pos)) continue;

			if (   (drawX < m_surfaceRect.left)
			    || (drawX > (m_surfaceRect.right  - GetZoomTilePixelWidth()))
			    || (drawY < m_surfaceRect.top)
			    || (drawY > (m_surfaceRect.bottom - (GetZoomTilePixelHeight() + GetZoomTileHeadroom()))))
				continue;

			if (world_Get()->IsGood(pos))
			{
				TileInfo * tileInfo = GetTileInfo(pos);
				GoodActor * goodActor = tileInfo ? tileInfo->GetGoodActor() : nullptr;
				if (goodActor)
				{
					goodActor->PositionActor(pos);
					if (!goodActor->AddGpuSpriteQuad(drawX + aui_SDL::WorldContentOffX(),
					                                drawY + aui_SDL::WorldContentOffY(), scale))
						aui_SDL::MarkQuadFrameIncomplete("good-sprite");
				}
			}

			Unit top;
			if (!world_Get()->GetTopVisibleUnit(player, pos, top)) continue;
			UnitActorPtr actor = top.GetActor();
			if (!actor) continue;

			bool const fog = m_localVision && m_localVision->IsExplored(pos) && !m_localVision->IsVisible(pos);
			if (!actor->AddGpuSpriteQuad(drawX + aui_SDL::WorldContentOffX(),
			                         drawY + aui_SDL::WorldContentOffY(), scale, fog))
				aui_SDL::MarkQuadFrameIncomplete(actor->GpuSpriteFallbackReason());
		}
	}

	Director *director = director_Get();
	if (director && !director->AddActiveEffectGpuSpriteQuads(
			&m_mapViewRect,
			aui_SDL::WorldContentOffX() - baseX,
			aui_SDL::WorldContentOffY() - baseY))
		aui_SDL::MarkQuadFrameIncomplete("effect-sprite");

	if (m_overlayActive)
	{
		m_overlayActive = false;
		if (m_overlayRec)
		{
			TerrainImprovementRecord::Effect const * effect =
				(m_overlayRec->GetClassTerraform() || m_overlayRec->GetClassOceanform())
				? m_overlayRec->GetTerrainEffect(0)
				: terrainutil_GetTerrainEffect(m_overlayRec, m_overlayPos);
			Pixel16 *data = effect ? m_tileSet->GetImprovementData((uint16)effect->GetTilesetIndex()) : nullptr;
			SDL_Texture *texture = aui_SDL::EnsureMapIconTexture(data,
				k_TILE_PIXEL_WIDTH, k_TILE_GRID_HEIGHT, m_overlayColor, true, k_FOW_BLEND_VALUE);
			if (texture)
			{
				sint32 x, y;
				maputils_MapXY2PixelXY(m_overlayPos.x, m_overlayPos.y, &x, &y);
				aui_SDL::GpuSpriteQuad q;
				q.texture = texture;
				q.sx = 0; q.sy = 0; q.sw = k_TILE_PIXEL_WIDTH; q.sh = k_TILE_GRID_HEIGHT;
				q.dx = x - baseX + aui_SDL::WorldContentOffX();
				q.dy = y - baseY + aui_SDL::WorldContentOffY();
				q.dw = GetZoomTilePixelWidth();
				q.dh = GetZoomTileGridHeight();
				q.mirror = false;
				q.alpha = 255;
				aui_SDL::AddSpriteQuad(q);
			}
		}
	}

	if (profiledb_Get()->GetShowCityNames()
	    && (!c3ui_Get() || !AddGpuCityNamesQuad(this, c3ui_Get()->SecondaryWidth(), c3ui_Get()->SecondaryHeight())))
		aui_SDL::MarkQuadFrameIncomplete("city-names");

	if (ScenarioEditor::ShowStartFlags())
		aui_SDL::MarkQuadFrameIncomplete("scenario-start-flags");

}

void TiledMap::ScrollPixels(sint32 deltaX, sint32 deltaY, aui_Surface *surf)
{
	char *      buffer;
	AUI_ERRCODE errcode = surf->Lock(nullptr, (LPVOID *)&buffer, 0);
	Assert(errcode == AUI_ERRCODE_OK);
	if (errcode != AUI_ERRCODE_OK)
		return;

	sint32 const h     = surf->Height();
	sint32 const w     = surf->Width();
	sint32 const pitch = surf->Pitch();
	// Pixel size in bytes: 2 (RGB565) or 4 (ARGB8888). The scroll is a plain
	// rectangular shift of existing content + black fill of the newly exposed
	// strip (the caller redraws that strip afterwards), so it works for any
	// depth as a per-row memmove — no more 2-pixels-per-uint32-word math.
	// P11 Stage 2 B: replaced the 16bpp-only word copy so the pan optimization
	// survives the 32-bit world surface. (Not exercised by the pixel oracle;
	// verify by panning a 32-bit map.)
	sint32 const bpp   = surf->BitsPerPixel() / 8;
	uint8 * const base = reinterpret_cast<uint8 *>(buffer);
	// Clamp the shift to the surface dimensions: a shift >= the whole surface
	// degenerates to "everything is newly exposed" (full clear), which the
	// unclamped loops did NOT handle — with dy > h the reveal loop's start row
	// (h - dy) went negative and memset wrote BELOW the buffer (SIGSEGV / silent
	// heap corruption; hit 2026-07-16 when an unbounded camera pan asked
	// ScrollMap for a ~1050px scroll on a ~912px surface).
	sint32 const dx    = std::min<sint32>(abs(deltaX), w);
	sint32 const dy    = std::min<sint32>(abs(deltaY), h);

	if (deltaX)
	{
		sint32 const copyBytes = (w - dx) * bpp;
		sint32 const fillBytes = dx * bpp;
		for (sint32 i = 0; i < h; i++)
		{
			uint8 * const row = base + i * pitch;
			if (deltaX > 0)                     // content moves left; expose the right edge
			{
				memmove(row, row + fillBytes, copyBytes);
				memset(row + copyBytes, 0, fillBytes);
			}
			else                                // content moves right; expose the left edge
			{
				memmove(row + fillBytes, row, copyBytes);
				memset(row, 0, fillBytes);
			}
		}
	}
	else if (deltaY)
	{
		sint32 const rowBytes = w * bpp;
		if (deltaY > 0)                         // content moves up; expose the bottom rows
		{
			for (sint32 i = 0; i < h - dy; i++)
				memcpy(base + i * pitch, base + (i + dy) * pitch, rowBytes);
			for (sint32 i = h - dy; i < h; i++)
				memset(base + i * pitch, 0, rowBytes);
		}
		else                                    // content moves down; expose the top rows
		{
			for (sint32 i = h - 1; i >= dy; i--)
				memcpy(base + i * pitch, base + (i - dy) * pitch, rowBytes);
			for (sint32 i = 0; i < dy; i++)
				memset(base + i * pitch, 0, rowBytes);
		}
	}

	errcode = surf->Unlock(buffer);
	Assert(errcode == AUI_ERRCODE_OK);
	if (errcode != AUI_ERRCODE_OK) return;
}

#define kMV_LeftMin (-1)
#define kMV_RightMax 2
#define kMV_TopMin (-4)
#define kMV_BottomMax 12







bool TiledMap::ScrollMap(sint32 deltaX, sint32 deltaY)
{
	if (g_modalWindow)
		return false;

	// A map scroll is meaningless without a world and a render surface. This
	// can be reached during game teardown (CleanupGame pumps ProcessUI ->
	// ui_CheckForScroll while the world is already destroyed) — bail rather
	// than dereference a freed world (quit-from-game SIGSEGV, fault @ 0x18).
	if (!world_Get() || !background_Get())
		return false;

	RECT	repaintRect;
	RECT	oldMapViewRect;




	RetargetTileSurface(background_Get()->TheSurface());

	sint32		mapWidth = world_Get()->GetWidth();
	sint32		mapHeight = world_Get()->GetHeight();
	sint32		hscroll = GetZoomTilePixelWidth();
	sint32		vscroll = GetZoomTilePixelHeight()/2;

	if (!world_Get()->IsXwrap())
	{
		if ((deltaX < 0) && (m_mapViewRect.left + deltaX < kMV_LeftMin ))
		{
			deltaX = kMV_LeftMin - m_mapViewRect.left;
			if (deltaX > 0) deltaX = 0;
		}
		if ((deltaX > 0) && (m_mapViewRect.right + deltaX > m_mapBounds.right + kMV_RightMax))
		{
			deltaX = m_mapBounds.right + kMV_RightMax - m_mapViewRect.right;
			if (deltaX < 0) deltaX = 0;
		}
	}

	if (!world_Get()->IsYwrap())
	{
		if ((deltaY < 0) && (m_mapViewRect.top + deltaY < kMV_TopMin))
		{
			deltaY = kMV_TopMin - m_mapViewRect.top;
			if (deltaY > 0) deltaY = 0;
		}

		if ((deltaY > 0) && (m_mapViewRect.bottom + deltaY > m_mapBounds.bottom + kMV_BottomMax))
		{
			deltaY = m_mapBounds.bottom + kMV_BottomMax - m_mapViewRect.bottom;
			if (deltaY  < 0) deltaY = 0;
		}
	}

	// If clamped to zero, nothing to do - return false so caller knows
	if (deltaX == 0 && deltaY == 0) {
		return false;
	}

	oldMapViewRect = m_mapViewRect;

	OffsetRect(&m_mapViewRect, deltaX, deltaY);

	SubtractRect(&repaintRect, &m_mapViewRect, &oldMapViewRect);

	OffsetMixDirtyRects(deltaX, deltaY);

	if (m_mapViewRect.right <= 0)
	{
		m_mapViewRect.left += mapWidth;
		m_mapViewRect.right += mapWidth;
		Assert(m_mapViewRect.right > 0);
	}

	if (m_mapViewRect.left >= mapWidth)
	{
		m_mapViewRect.left -= mapWidth;
		m_mapViewRect.right -= mapWidth;
		Assert(m_mapViewRect.left < mapWidth);
	}
	if (m_mapViewRect.bottom <= 0)
	{
		m_mapViewRect.top += mapHeight;
		m_mapViewRect.bottom += mapHeight;
		Assert(m_mapViewRect.bottom > 0);
	}
	if (m_mapViewRect.top >= mapHeight)
	{
		m_mapViewRect.top -= mapHeight;
		m_mapViewRect.bottom -= mapHeight;
		Assert(m_mapViewRect.top < mapHeight);
	}

	RECT tempRect = repaintRect;

	if (deltaX == 1) {
		tempRect.left -= 1;
		tempRect.top += 1;
		tempRect.bottom += 1;
	}
	else if (deltaX == -1) {
		tempRect.right += 1;
		tempRect.top += 1;
		tempRect.bottom += 1;
	}

	if (deltaY == 1) {
		tempRect.right += 1;
		tempRect.top -= 2;
	}
	else if (deltaY == -1) {
		tempRect.right += 1;
		tempRect.bottom += 2;

	}

	OffsetSprites(&tempRect, deltaX*hscroll, deltaY*vscroll);
	ScrollPixels((sint32)(deltaX*hscroll), (sint32)(deltaY*vscroll), m_surface);

	LockSurface();

	RepaintTiles(&repaintRect);

	if (!world_Get()->IsXwrap())
		if (m_mapViewRect.left + deltaX < 0 ||
			m_mapViewRect.right + deltaX > m_mapBounds.right-1)
			RepaintEdgeX(&repaintRect);

	if (!world_Get()->IsYwrap())
		if (m_mapViewRect.top + deltaY < 0 ||
			m_mapViewRect.bottom + deltaY > m_mapBounds.bottom-2)
			RepaintEdgeY(&repaintRect);

	RepaintHats(&tempRect);
	RepaintBorders(&tempRect);
	RepaintImprovements(&tempRect);

	UnlockSurface();

	m_mapDirtyList->Flush();

	InvalidateMix();

	RepaintSprites(m_surface, &tempRect, true);

	return true;
}





bool TiledMap::SmoothScrollAligned() const
{
	return ((!m_smoothOffsetX) && (!m_smoothOffsetY));
}




#ifdef IANSCROLL






bool TiledMap::ScrollMapSmooth(sint32 pdeltaX, sint32 pdeltaY)
{

	if (g_modalWindow)
		return false;

	m_smoothOffsetX += pdeltaX;
	m_smoothOffsetY	+= pdeltaY;

	sint32		hscroll		= GetZoomTilePixelWidth();
	sint32		vscroll		= GetZoomTilePixelHeight()>>1;

	sint32 deltaX =	(m_smoothOffsetX/hscroll);
	sint32 deltaY =	(m_smoothOffsetY/vscroll);

	if (deltaX != 0)
	{

		m_smoothOffsetX -= deltaX * hscroll;


	}

	if (deltaY != 0)
	{

		m_smoothOffsetY -= deltaY * vscroll;


	}

	OffsetRect(&m_mapViewRect , deltaX, deltaY);

	OffsetMixDirtyRects(deltaX,deltaY);


	RetargetTileSurface(background_Get()->TheSurface());

	ScrollPixels((sint32)(pdeltaX), (sint32)(pdeltaY), m_surface);

	LockSurface();

	RECT repaintRect = m_mapViewRect;

	repaintRect.top--;
	repaintRect.left--;
	repaintRect.bottom++;
	repaintRect.right++;


	OffsetSprites(&repaintRect, pdeltaX, pdeltaY);
	RepaintTilesClipped (&repaintRect);
	RepaintImprovements (&repaintRect);
	RepaintHats			(&repaintRect,true);
	RepaintBorders      (&repaintRect, true);

	UnlockSurface();

	InvalidateMix();

 	RepaintSprites(m_surface, &repaintRect, true);

	return true;
}

#else // IANSCROLL

bool TiledMap::ScrollMapSmooth(sint32 pdeltaX, sint32 pdeltaY)
{
	if (g_modalWindow)
		return false;

	m_smoothOffsetX += pdeltaX;
	m_smoothOffsetY	+= pdeltaY;

	sint32 signX  = (!pdeltaX ? 0 :(pdeltaX<0? -1:1));
	sint32 signY  = (!pdeltaY ? 0 :(pdeltaY<0? -1:1));

	RetargetTileSurface(background_Get()->TheSurface());

	sint32	mapWidth	= world_Get()->GetWidth();
	sint32	mapHeight	= world_Get()->GetHeight();
	sint32	hscroll		= GetZoomTilePixelWidth();
	sint32	vscroll		= GetZoomTilePixelHeight() >> 1;
	if(hscroll < 1) hscroll = 1;
	if(vscroll < 1) vscroll = 1;
	sint32  deltaX      = m_smoothOffsetX / hscroll;
	sint32  deltaY      = m_smoothOffsetY / vscroll;

	if (deltaX != 0)
	{
		m_smoothOffsetX -= deltaX * hscroll;
	}

	if (deltaY != 0)
	{
		m_smoothOffsetY -= deltaY * vscroll;
	}

	m_smoothLastX =	pdeltaX;
	m_smoothLastY =	pdeltaY;

   	if ((!world_Get()->IsXwrap())&&signX)
   	{
   		if ((m_mapViewRect.left + signX) <= 0)
		{
			m_smoothOffsetX=0;
			m_mapViewRect.top = 0;
			return false;
		}

		if ((m_mapViewRect.right + signX) >= (m_mapBounds.right))
		{
			m_smoothOffsetX=0;
			m_mapViewRect.top = 0;
			return false;
		}
   	}

   	if ((!world_Get()->IsYwrap())&&(signY))
   	{
   		if ((m_mapViewRect.top+signY) < 0)
		{
			m_mapViewRect.top = 0;
			m_smoothOffsetY=0;
			return false;
		}
		if ((m_mapViewRect.bottom+signY) >= (m_mapBounds.bottom))
		{
			m_smoothOffsetY=0;
			m_mapViewRect.bottom = m_mapBounds.bottom;
			return false;
		}
   	}

	RECT oldMapViewRect = m_mapViewRect;
	RECT repaintRect;

	OffsetRect(&m_mapViewRect, deltaX, deltaY);

	SubtractRect(&repaintRect, &m_mapViewRect, &oldMapViewRect);

	OffsetMixDirtyRects(deltaX, deltaY);

	if (m_mapViewRect.right <= 0)
	{
		m_mapViewRect.left += mapWidth;
		m_mapViewRect.right += mapWidth;
		Assert(m_mapViewRect.right > 0);
	}

	if (m_mapViewRect.left >= mapWidth)
	{
		m_mapViewRect.left -= mapWidth;
		m_mapViewRect.right -= mapWidth;
		Assert(m_mapViewRect.left < mapWidth);
	}
	if (m_mapViewRect.bottom <= 0)
	{
		m_mapViewRect.top += mapHeight;
		m_mapViewRect.bottom += mapHeight;
		Assert(m_mapViewRect.bottom > 0);
	}
	if (m_mapViewRect.top >= mapHeight)
	{
		m_mapViewRect.top -= mapHeight;
		m_mapViewRect.bottom -= mapHeight;
		Assert(m_mapViewRect.top < mapHeight);
	}

	RECT tempRect = repaintRect;

	if (signX == 1) {
		tempRect.left -= 1;
		tempRect.top += 1;
		tempRect.bottom += 1;
	}
	else if (signX == -1) {
		tempRect.right += 1;
		tempRect.top += 1;
		tempRect.bottom += 1;
	}

	if (signY == 1) {
		tempRect.right += 1;
		tempRect.top -= 2;
	}
	else if (signY == -1) {
		tempRect.right += 1;
		tempRect.bottom += 2;

	}

	OffsetSprites(&tempRect, pdeltaX, pdeltaY);

	ScrollPixels((sint32)(pdeltaX), (sint32)(pdeltaY), m_surface);





	LockSurface();

	RepaintTiles(&repaintRect);

	if (!world_Get()->IsXwrap())
		if (m_mapViewRect.left + deltaX < 0 ||
			m_mapViewRect.right + deltaX > m_mapBounds.right-1)
			RepaintEdgeX(&repaintRect);

	if (!world_Get()->IsYwrap())
		if (m_mapViewRect.top + deltaY < 0 ||
			m_mapViewRect.bottom + deltaY > m_mapBounds.bottom-2)
			RepaintEdgeY(&repaintRect);

	RepaintHats(&tempRect);
	RepaintBorders(&tempRect);
	RepaintImprovements(&tempRect);

	UnlockSurface();

	InvalidateMix();

	RepaintSprites(m_surface, &tempRect, true);

	return true;
}

#endif // IANSCROLL







sint32 TiledMap::RedrawHat(
			aui_Surface *surface,
			sint32 i,
			sint32 j,
			bool clip
			)
{
	sint32		drawx = j;

	maputils_WrapPoint(j,i,&j,&i);

	sint32      drawy = i;

	maputils_TileX2MapXAbs(drawx,drawy,&drawx);

	sint32 k = maputils_TileX2MapX(j,i);

	if (!TileIsVisible(k,i))
		return 0;

	MapPoint tempPos (k, i);


	if (!ReadyToDraw())
		return 0;

	if (!m_localVision->IsExplored(tempPos))
    {
		return 0;
	}

	TileInfo		*tileInfo;

	sint32		terrainType;
	bool		fog = !m_localVision->IsVisible(tempPos) && !GpuFogActive();
	if (fog)
    {
		UnseenCellCarton ucell;
		if(m_localVision->GetLastSeen(tempPos, ucell))
		{
			terrainType = ucell.m_unseenCell->GetTerrainType();
			tileInfo = ucell.m_unseenCell->GetTileInfo();
		}
		else
		{
			terrainType = world_Get()->GetTerrain(tempPos.x,tempPos.y);
			tileInfo = GetTileInfo(tempPos);
		}
	}
	else
	{
		terrainType = world_Get()->GetTerrain(tempPos.x, tempPos.y);
		tileInfo = GetTileInfo(tempPos);
	}

	maputils_MapXY2PixelXY(drawx,drawy,&drawx,&drawy);

	if (tileInfo == nullptr)
		return -1;

	BaseTile *baseTile = m_tileSet->GetBaseTile(tileInfo->GetTileNum());

	if (baseTile == nullptr)
		return -1;

	if (m_zoomLevel == k_ZOOM_LARGEST)
	{

		if (!fog)
			DrawOverlayClipped(surface, baseTile->GetHatData(), drawx, drawy);
		else
		{
			if (g_isFastCpu)
				DrawBlendedOverlay(surface, baseTile->GetHatData() ,drawx,drawy,k_FOW_COLOR,k_FOW_BLEND_VALUE);
			else
				DrawDitheredOverlay(surface, baseTile->GetHatData(),drawx,drawy,k_FOW_COLOR);
		}

		if (m_surface == m_mapSurface)
			AddDirtyToMap(drawx,drawy, k_TILE_PIXEL_WIDTH, k_TILE_PIXEL_HEIGHT);
	}
	else
	{


		if (!fog)
		{

			DrawScaledOverlay(surface, baseTile->GetHatData(),drawx,drawy,
									GetZoomTilePixelWidth(),
									GetZoomTileGridHeight());
		}
		else
		{
			if (g_isFastCpu)
			{
				DrawBlendedOverlayScaled(surface, baseTile->GetHatData(),drawx,drawy,
									GetZoomTilePixelWidth(),
									GetZoomTileGridHeight(),
									k_FOW_COLOR,
									k_FOW_BLEND_VALUE);
			}
			else
			{
				DrawDitheredOverlayScaled(surface, baseTile->GetHatData(),drawx,drawy,
									GetZoomTilePixelWidth(),
									GetZoomTileGridHeight(),
									k_FOW_COLOR);
			}
		}

		if (m_surface == m_mapSurface)
			AddDirtyToMap(drawx,drawy, GetZoomTilePixelWidth(), GetZoomTileGridHeight());

	}




	static MapPoint nw;
	static MapPoint ne;
	if(tempPos.GetNeighborPosition(NORTHWEST, nw)) {
		if (m_localVision->IsExplored(nw))
        {

			DrawNationalBorders(surface, nw);
		}
	}

	if(tempPos.GetNeighborPosition(NORTHEAST, ne)) {
		if (m_localVision->IsExplored(ne))
        {
			DrawNationalBorders(surface, ne);
		}
	}

	return 0;
}







sint32 TiledMap::RedrawBorders(
			aui_Surface *surface,
			sint32 i,
			sint32 j,
			bool clip
			)
{
	maputils_WrapPoint(j,i,&j,&i);

	sint32 k = maputils_TileX2MapX(j,i);

	if (!TileIsVisible(k,i))
		return 0;

	MapPoint tempPos (k, i);

	if(!ReadyToDraw())
		return 0;

	if(!m_localVision->IsExplored(tempPos))
    {
		return 0;
	}
	//DrawBorderIcon(surface, tempPos);  //emod?
	DrawNationalBorders(surface, tempPos);

	return 0;
}






void TiledMap::RedrawTile
(
	const MapPoint *    point
)
{
	if (TileIsVisible(point->x, point->y))
    {
		MapPoint    pos;
	    sint32      tileX;

		LockSurface();
		if(point->GetNeighborPosition(NORTH,pos)) {
			maputils_MapX2TileX(pos.x,pos.y,&tileX);
			CalculateWrap(nullptr,pos.y,tileX);
			RedrawHat(nullptr,pos.y,tileX);
			RedrawBorders(nullptr, pos.y, tileX);
			DrawImprovements(nullptr, pos.y, tileX, false);
		}

		if(point->GetNeighborPosition(NORTHWEST,pos)) {
			maputils_MapX2TileX(pos.x,pos.y,&tileX);
			CalculateWrap(nullptr,pos.y,tileX);
			RedrawHat(nullptr,pos.y,tileX);
			RedrawBorders(nullptr, pos.y, tileX);
			DrawImprovements(nullptr, pos.y, tileX, false);
		}

		if(point->GetNeighborPosition(NORTHEAST,pos)) {
			maputils_MapX2TileX(pos.x,pos.y,&tileX);
			CalculateWrap(nullptr,point->y,tileX);
			RedrawHat(nullptr,pos.y,tileX);
			RedrawBorders(nullptr, pos.y, tileX);
			DrawImprovements(nullptr, pos.y, tileX, false);
		}

		if(point->GetNeighborPosition(EAST,pos)) {
			maputils_MapX2TileX(pos.x,pos.y,&tileX);
			RedrawHat(nullptr,pos.y,tileX);
			RedrawBorders(nullptr, pos.y, tileX);
			DrawImprovements(nullptr, pos.y, tileX, false);
		}

		BlackTile(m_surface, (MapPoint *)point);

		maputils_MapX2TileX(point->x,point->y,&tileX);

		CalculateWrap(nullptr,point->y,tileX);

		RedrawHat(nullptr, point->y,tileX);
		RedrawBorders(nullptr, point->y, tileX);

		DrawImprovements(nullptr, point->y, tileX, false);

		if(point->GetNeighborPosition(WEST,pos)) {
			maputils_MapX2TileX(pos.x,pos.y,&tileX);
			RedrawHat(nullptr,pos.y,tileX);
			RedrawBorders(nullptr, pos.y, tileX);
			DrawImprovements(nullptr, pos.y, tileX, false);
		}

		if(point->GetNeighborPosition(SOUTHWEST,pos)) {
			maputils_MapX2TileX(pos.x,pos.y,&tileX);
			RedrawHat(nullptr,pos.y,tileX);
			RedrawBorders(nullptr, pos.y, tileX);
			DrawImprovements(nullptr, pos.y, tileX, false);
		}

		if(point->GetNeighborPosition(SOUTHEAST,pos)) {
			maputils_MapX2TileX(pos.x,pos.y,&tileX);
			RedrawHat(nullptr,pos.y,tileX);
			RedrawBorders(nullptr, pos.y, tileX);
			DrawImprovements(nullptr, pos.y, tileX, false);
		}

		if(point->GetNeighborPosition(SOUTH,pos)) {
			maputils_MapX2TileX(pos.x,pos.y,&tileX);
			RedrawHat(nullptr,pos.y,tileX);
			RedrawBorders(nullptr, pos.y, tileX);
			DrawImprovements(nullptr, pos.y, tileX, false);
		}

		UnlockSurface();
	}

	if (radar_map_Get()) radar_map_Get()->RedrawTile( point );
}







void TiledMap::BlackTile(aui_Surface *surface, const MapPoint *point)
{
	if (!TileIsVisible(point->x, point->y)) return;

	sint32 x;
	sint32 y;
	maputils_MapXY2PixelXY(point->x,point->y,&x,&y);

	if (m_zoomLevel == k_ZOOM_LARGEST)
		DrawBlackTile(surface, x, y);
	else {

		DrawBlackScaledLow(surface, *point, x, y, GetZoomTilePixelWidth(), GetZoomTilePixelHeight());
	}
}

void TiledMap::Blt(aui_Surface *surf)
{
	RECT			rect = {0, 0, surf->Width(), surf->Height()};
	

	c3ui_Get()->TheBlitter()->Blt(surf, 0, 0, m_surface, &rect, 0);
}

bool TiledMap::TileIsCompletelyVisible(sint32 mapX, sint32 mapY, RECT *viewRect)
{
    RECT	shrunkMapViewRect   = viewRect ? *viewRect : m_mapViewRect;

	sint32  shrinkX             =
        (sint32)ceil((double)k_TILE_PIXEL_WIDTH / (double)GetZoomTilePixelWidth());
	sint32  shrinkY             =
        (sint32)ceil((double)k_TILE_GRID_HEIGHT / (double)GetZoomTileGridHeight());

	shrinkX *= 2;
	shrinkY *= 2;

	InflateRect(&shrunkMapViewRect, -shrinkX, -shrinkY);

	if(ctp2_Window *rw = radarwindow_Get(); rw && rw->Height()) {
		shrunkMapViewRect.bottom -= (rw->Height() / (GetZoomTilePixelHeight())) * 2;
	}
	RECT ul = shrunkMapViewRect;
	RECT ll = shrunkMapViewRect;
	RECT ur = shrunkMapViewRect;
	RECT lr = shrunkMapViewRect;

	sint32 mapWidth;
	sint32 mapHeight;
	GetMapMetrics(&mapWidth,&mapHeight);
	sint32 tileX;
	sint32 tileY = mapY;
	maputils_MapX2TileX(mapX,mapY,&tileX);

	if (shrunkMapViewRect.left < 0) {
		ul.left = ll.left = 0;
		ul.right = ll.right = shrunkMapViewRect.right;
		ur.left = lr.left = shrunkMapViewRect.left + mapWidth;
		ur.right = lr.right = mapWidth;
	}
	if (shrunkMapViewRect.right >= mapWidth) {
		ul.left = ll.left = 0;
		ul.right = ll.right = shrunkMapViewRect.right - mapWidth;
		ur.left = lr.left = shrunkMapViewRect.left;
		ur.right = lr.right = mapWidth;
	}
	if (shrunkMapViewRect.top < 0) {
		ul.top = ur.top = 0;
		ul.bottom = ur.bottom = shrunkMapViewRect.bottom;
		ll.top = lr.top = shrunkMapViewRect.top + mapHeight;
		ll.bottom = lr.bottom = mapHeight;
	}
	if (shrunkMapViewRect.bottom >= mapHeight) {
		ll.top = lr.top = shrunkMapViewRect.top;
		ll.bottom = lr.bottom = mapHeight;
		ul.top = ur.top = 0;
		ul.bottom = ur.bottom = shrunkMapViewRect.bottom - mapHeight;
	}

	POINT point = {tileX,tileY};

	return (PtInRect(&ul,point) ||
			PtInRect(&ll,point) ||
			PtInRect(&ur,point) ||
			PtInRect(&lr,point)
           );
}

/// Determine whether a tile is visible
/// \param mapX X coordinate of tile
/// \param mapY Y coordinate of tile
/// \remarks The mapZ coordinate parameter is not used
#if defined(_SMALL_MAPPOINTS)
bool TiledMap::TileIsVisible(sint32 mapX, sint32 mapY)
#else
bool TiledMap::TileIsVisible(sint32 mapX, sint32 mapY, sint32 /* mapZ */)
#endif
{
	RECT ul = m_mapViewRect;
	RECT ll = m_mapViewRect;
	RECT ur = m_mapViewRect;
	RECT lr = m_mapViewRect;

	sint32 mapWidth;
	sint32 mapHeight;
	GetMapMetrics(&mapWidth,&mapHeight);
	sint32 tileX;
	sint32 tileY = mapY;
	maputils_MapX2TileX(mapX,mapY,&tileX);

	if (m_mapViewRect.left < 0) {
		ul.left = ll.left = 0;
		ul.right = ll.right = m_mapViewRect.right;
		ur.left = lr.left = m_mapViewRect.left + mapWidth;
		ur.right = lr.right = mapWidth;
	}
	if (m_mapViewRect.right >= mapWidth) {
		ul.left = ll.left = 0;
		ul.right = ll.right = m_mapViewRect.right - mapWidth;
		ur.left = lr.left = m_mapViewRect.left;
		ur.right = lr.right = mapWidth;
	}
	if (m_mapViewRect.top < 0) {
		ul.top = ur.top = 0;
		ul.bottom = ur.bottom = m_mapViewRect.bottom;
		ll.top = lr.top = m_mapViewRect.top + mapHeight;
		ll.bottom = lr.bottom = mapHeight;
	}
	if (m_mapViewRect.bottom >= mapHeight) {
		ll.top = lr.top = m_mapViewRect.top;
		ll.bottom = lr.bottom = mapHeight;
		ul.top = ur.top = 0;
		ul.bottom = ur.bottom = m_mapViewRect.bottom - mapHeight;
	}

	POINT point = {tileX,tileY};

      /// @todo Check IsYwrap
	if(!world_Get()->IsXwrap()) {
		if(m_mapViewRect.left < 0) {
			return PtInRect(&ul, point) || PtInRect(&ll, point);
		}

		if(m_mapViewRect.right >= mapWidth) {
			return PtInRect(&ur, point) || PtInRect(&lr, point);
		}
	}

	return (PtInRect(&ul,point) || PtInRect(&ll,point) ||
			PtInRect(&ur,point) || PtInRect(&lr,point));
}

Pixel16 TiledMap::average(Pixel16 pixel1, Pixel16 pixel2, Pixel16 pixel3, Pixel16 pixel4)
{
	short		 r1;
	short		 g1;
	short		 b1;
	short		 r2;
	short		 g2;
	short		 b2;
	short		 r3;
	short		 g3;
	short		 b3;
	short		 r4;
	short		 g4;
	short		 b4;
	short		 r0;
	short		 g0;
	short		 b0;

	if (is_565_Get()) {
		r1 = (pixel1 & 0xF800) >> 11;
		g1 = (pixel1 & 0x07E0) >> 5;
		b1 = (pixel1 & 0x001F);

		r2 = (pixel2 & 0xF800) >> 11;
		g2 = (pixel2 & 0x07E0) >> 5;
		b2 = (pixel2 & 0x001F);

		r3 = (pixel3 & 0xF800) >> 11;
		g3 = (pixel3 & 0x07E0) >> 5;
		b3 = (pixel3 & 0x001F);

		r4 = (pixel4 & 0xF800) >> 11;
		g4 = (pixel4 & 0x07E0) >> 5;
		b4 = (pixel4 & 0x001F);

		r0 = (r1 + r2 + r3 + r4) >> 2;
		g0 = (g1 + g2 + g3 + g4) >> 2;
		b0 = (b1 + b2 + b3 + b4) >> 2;

		return (r0 << 11) | (g0 << 5) | b0;
	} else {
		r1 = (pixel1 & 0x7C00) >> 10;
		g1 = (pixel1 & 0x03E0) >> 5;
		b1 = (pixel1 & 0x001F);

		r2 = (pixel2 & 0x7C00) >> 10;
		g2 = (pixel2 & 0x03E0) >> 5;
		b2 = (pixel2 & 0x001F);

		r3 = (pixel3 & 0x7C00) >> 10;
		g3 = (pixel3 & 0x03E0) >> 5;
		b3 = (pixel3 & 0x001F);

		r4 = (pixel4 & 0x7C00) >> 10;
		g4 = (pixel4 & 0x03E0) >> 5;
		b4 = (pixel4 & 0x001F);

		r0 = (r1 + r2 + r3 + r4) >> 2;
		g0 = (g1 + g2 + g3 + g4) >> 2;
		b0 = (b1 + b2 + b3 + b4) >> 2;

		return (r0 << 10) | (g0 << 5) | b0;
	}

}

void TiledMap::ProcessRun(Pixel16 **rowData1, Pixel16 **rowData2, Pixel16 *pix1, Pixel16 *pix2,
					sint32 pos, Pixel16 destPixel, short transparency, Pixel16 outlineColor,
					sint32 flags)
{
	static sint32		mode1;
	static sint32		mode2;
	static sint32		pos1;
	static sint32		pos2;
	static sint32		 end1;
	static sint32		 end2;
	static sint32		 alpha1;
	static sint32		 alpha2;
	static sint32		 oldend1;
	static sint32		 oldend2;

	Pixel16				pixel1 = 0;
	Pixel16				pixel2 = 0;

	if (pos == -1) {
		end1 = ReadTag(&mode1, rowData1, &alpha1);
		end2 = ReadTag(&mode2, rowData2, &alpha2);

		pos1 = 0;
		pos2 = 0;

		oldend1 = 0;
		oldend2 = 0;

		*pix1 = k_MEDIUM_KEY;
		*pix2 = k_MEDIUM_KEY;

		return;
	}

	while (pos1 <= pos) {
		switch (mode1) {
			case k_TILE_SKIP_RUN_ID	:
					pixel1 = k_MEDIUM_KEY;
				break;
			case k_TILE_COPY_RUN_ID			: {
					if (!(flags & k_OVERLAY_FLAG_SHADOWSONLY)) {
						pixel1 = **rowData1;
					} else {
						pixel1 = k_MEDIUM_KEY;
					}
					(*rowData1)++;
				}
				break;
			case k_TILE_SHADOW_RUN_ID : {
					if (!(flags & k_OVERLAY_FLAG_NOSHADOWS)) {
						pixel1 = pixelutils_Shadow(**rowData1);
					} else {
						pixel1 = k_MEDIUM_KEY;
					}
				}
				break;

			default :
				Assert(mode1 == k_TILE_SKIP_RUN_ID || mode1 == k_TILE_COPY_RUN_ID || mode1 == k_TILE_SHADOW_RUN_ID);
		}

		pos1++;

		if (pos1 >= end1) {
			oldend1 = end1;
			end1 = oldend1 + ReadTag(&mode1, rowData1, &alpha1);
		}
	}

	while (pos2 <= pos) {

		switch (mode2) {
			case k_TILE_SKIP_RUN_ID	:
					pixel2 = k_MEDIUM_KEY;
				break;
			case k_TILE_COPY_RUN_ID			:
				{
					if (!(flags & k_OVERLAY_FLAG_SHADOWSONLY)) {
						pixel2 = **rowData2;
					} else {
						pixel2 = k_MEDIUM_KEY;
					}
					(*rowData2)++;
				}
				break;
			case k_TILE_SHADOW_RUN_ID		:
				{
					if (!(flags & k_OVERLAY_FLAG_SHADOWSONLY)) {
						pixel2 = pixelutils_Shadow(**rowData2);
					} else {
						pixel2 = k_MEDIUM_KEY;
					}
				}
				break;

			default :
				Assert(mode2 == k_TILE_SKIP_RUN_ID || mode2 == k_TILE_COPY_RUN_ID || mode2 == k_TILE_SHADOW_RUN_ID);
		}

		pos2++;

		if (pos2 >= end2) {
			oldend2 = end2;
			end2 = oldend2 + ReadTag(&mode2, rowData2, &alpha2);
		}
	}

	*pix1 = pixel1;
	*pix2 = pixel2;
}

sint32 TiledMap::ReadTag(sint32 *mode, Pixel16 **rowData, sint32 *alpha)
{
	sint32			len;
	Pixel16		tag = **rowData;

	*mode = (tag & 0x0F00) >> 8;
	(*rowData)++;

	switch (*mode) {
		case k_TILE_SKIP_RUN_ID	:
			len = tag & 0x00FF;
			break;
		case k_TILE_COPY_RUN_ID :
			len = tag & 0x00FF;
		break;
		case k_TILE_SHADOW_RUN_ID :
			len = tag & 0x00FF;
			break;
		default :
			len = 1;
	}
	return len;
}

UnitActorPtr TiledMap::GetClickedUnit(aui_MouseEvent *data)
{
	sint32				 mapWidth;
	sint32				 mapHeight;
	GetMapMetrics(&mapWidth, &mapHeight);

	sint32				 x;
	sint32				 y;
	POINT       point = data->position;

	for (sint32 i=m_mapViewRect.top; i<m_mapViewRect.bottom; i++) {
		for (sint32 j=m_mapViewRect.left; j<m_mapViewRect.right; j++) {

			sint32 tileX;
			sint32 tileY;
			maputils_WrapPoint(j,i,&tileX,&tileY);

			sint32 mapX;
			sint32 mapY = tileY;
			mapX = maputils_TileX2MapX(tileX,tileY);

			maputils_MapXY2PixelXY(mapX,mapY,&x,&y);

			MapPoint pos(mapX,mapY);

			Unit		top;

			if (!world_Get()->GetTopVisibleUnit(pos, top)) continue;

			UnitActorPtr actor = top.GetActor();

			if (actor->IsActive()) {
				Unit	second;
				if (world_Get()->GetSecondUnit(pos, second)) {
					top = second;
					actor = top.GetActor();
				}
			}

			if (!actor->IsActive()) {
				RECT	actorRect;

				SetRect(&actorRect, x, y, x+(sint32)actor->GetWidth(), y+(sint32)actor->GetHeight());

				if (PtInRect(&actorRect, point)) {
					return actor;
				}
			}
		}
	}

	return director_Get()->GetClickedActiveUnit(data);
}

bool TiledMap::PointInMask(POINT hitPt) const
{
	sint32 x = (sint32)((double)hitPt.x / GetZoomScale(GetZoomLevel()));
	sint32 y = (sint32)(((double)hitPt.y / GetZoomScale(GetZoomLevel())) + k_TILE_PIXEL_HEADROOM);

	return (x >= m_tileHitMask[y].start) && (x <= m_tileHitMask[y].end);
}

bool TiledMap::MousePointToTilePos(POINT point, MapPoint &tilePos) const
{
	// No world -> a screen point maps to no tile. Reached during game teardown
	// (CleanupGame pumps ProcessUI -> aui_UI Idle -> TiledMap::Idle) after the
	// world is destroyed but before the UI is; bail rather than deref a freed
	// world (close-game SIGSEGV in World::IsXwrap, fault @0xc).
	if (!world_Get())
		return false;

	sint32      width   = GetZoomTilePixelWidth();
	sint32      height  = GetZoomTilePixelHeight();

    sint32  xoff;
    sint32  yoff;
	GetSmoothScrollOffsets(xoff,yoff);
  	sint32  x = point.x + xoff;
	sint32  y = point.y + yoff;

	// P11 2c (ADR-001): the sub-tile GPU pan slides the visible world by CameraOff
	// while the engine view stays tile-aligned, so a pick must shift by the same
	// offset to hit the tile the user sees. No-op unless the GPU camera is on.
	if (aui_SDL::GpuCameraEnabled())
	{
		x -= static_cast<sint32>(aui_SDL::CameraOffX());
		y -= static_cast<sint32>(aui_SDL::CameraOffY());
	}

	if (!(m_mapViewRect.top & 1)) y -= GetZoomTileHeadroom();

	MapPoint		pos ((x / width) + m_mapViewRect.left,
                         (y / height) + m_mapViewRect.top/2
                        );

	POINT			hitPt;
 	hitPt.x = x % width;
	hitPt.y = y % height;

	sint16			maxX = static_cast<sint16>(m_mapBounds.right);

	if (!PointInMask(hitPt)) {

		pos.x = (sint16)((x + (width/2)) / width - 1 + m_mapViewRect.left);
		pos.y = (sint16)(((y + (height)/2) / height - 1) + m_mapViewRect.top/2);

		hitPt.x = (x + (width/2)) % width;
		hitPt.y = (y + (height)/2) % height;

		if (!PointInMask(hitPt))
			return false;
		else {
			if (pos.x >= pos.y) {
				tilePos.x = pos.x - pos.y;
			} else {
				tilePos.x = maxX + pos.x - pos.y;
			}
			tilePos.y = pos.y * 2 + 1;
		}
	} else {
		if (pos.x >= pos.y) {
			tilePos.x = pos.x - pos.y;
		} else {
			tilePos.x = maxX + pos.x - pos.y;
		}
		tilePos.y = pos.y * 2;
	}

	if(!world_Get()->IsXwrap()) {
		if(pos.x < 0)
			return false;
		if(pos.x >= world_Get()->GetXWidth())
			return false;
	}


	if((m_mapViewRect.top) & 1 && (m_mapViewRect.top < 0)) {
		tilePos.y -= 2;
    	tilePos.x++;
	}





	if (tilePos.x <0) tilePos.x = static_cast<sint16>(world_Get()->GetWidth()) + tilePos.x;
	else if (world_Get()->GetWidth() <= tilePos.x) tilePos.x = tilePos.x - static_cast<sint16>(world_Get()->GetWidth());

	if (world_Get()->IsYwrap()) {
		sint16 sx;
		sint16 sy;

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
			return false;
		} else if (world_Get()->GetHeight() <= tilePos.y) {
			tilePos.y = static_cast<sint16>(world_Get()->GetHeight() -1);
			return false;
		}
	}


    Assert (m_mapBounds.left <= tilePos.x);
    Assert (tilePos.x < m_mapBounds.right);
    Assert (m_mapBounds.top <= tilePos.y);
    Assert (tilePos.y < m_mapBounds.bottom);

	return true;
}

void TiledMap::HandleCheat(MapPoint &pos)
{
	if (ScenarioEditor::PlaceStartFlags())
    {
		ScenarioEditor::PlaceFlag(pos);
		return;
	}

	bool needPostProcess = false;
	if(ScenarioEditor::PaintHutMode()) {
		if(world_Get()->GetCell(pos)->GetGoodyHut()) {
			world_Get()->GetCell(pos)->DeleteGoodyHut();
		} else {
			world_Get()->GetCell(pos)->CreateGoodyHut();
		}
		needPostProcess = true;
	}

	if(ScenarioEditor::PaintRiverMode()) {
		if (world_Get()->IsRiver(pos)) {
			world_Get()->UnsetRiver(pos.x, pos.y);
		} else {
			world_Get()->SetRiver( pos );
		}

		needPostProcess = true;
	}

	if(ScenarioEditor::PaintGoodsMode()) {
		sint32 curGood;
		if(world_Get()->GetGood(pos, curGood)) {
			world_Get()->ClearGoods(pos.x, pos.y);
		} else {
			world_Get()->SetGood(pos.x, pos.y, ScenarioEditor::PaintGood() + 1);
		}
		needPostProcess = true;
	}

	if(needPostProcess) {
		RadiusIterator it(pos, 1, 2.0);
		MapPoint mpos;
		for(it.Start(); !it.End(); it.Next()) {
			mpos = it.Pos();
			PostProcessTile(mpos, GetTileInfo(mpos));
			RedrawTile(&mpos);
		}
		return;
	}

	if ( ScenarioEditor::PaintTerrainImprovementMode() )
	{
		Player * p = player_Get(selitem_Get()->GetVisiblePlayer());
		if (!p) return;
		TerrainImprovement theImprovement =
            terrimprovepool_Get()->Create
                (p->m_owner,
				 pos,
				 ScenarioEditor::PaintTerrainImprovement(),
				 0
                );

		if (terrimprovepool_Get()->IsValid(theImprovement.m_id))
        {
			p->m_terrainImprovements->Insert(theImprovement);
			theImprovement.Complete();
		}
	}

	if (g_placeGoodsMode || ScenarioEditor::PaintTerrainMode())
    {
		sint32      tileNum     = ScenarioEditor::PaintTerrainMode()
                                  ? ScenarioEditor::PaintTerrain()
                                  : 0;
        bool        river       = false;

		if (g_placeGoodsMode)
        {
			sint32 curGood;
			if(world_Get()->GetGood(pos, curGood))
            {
				world_Get()->ClearGoods(pos.x, pos.y);
			}
            else
            {
				world_Get()->SetGood(pos.x, pos.y, g_placeGoodsMode);
			}
		}
        else
        {
			switch (tileNum)
            {
			case TILEPAD_TYPE_GOODY:
				if (world_Get()->IsLand(pos))
                {
                    GoodyHut * hut = world_Get()->GetGoodyHut(pos);
					if (hut)
                    {
						world_Get()->GetCell(pos)->DeleteGoodyHut();
					}
                    else
                    {
						world_Get()->GetCell(pos)->CreateGoodyHut();
					}
					river = true;
				}
				else
                {
					return;
				}
				break;

			case TILEPAD_TYPE_RIVER:
				if (world_Get()->IsRiver(pos))
                {
					world_Get()->UnsetRiver(pos.x, pos.y);
				}
                else
                {
					world_Get()->SetRiver( pos );
				}
				river = true;
				break;

			default:
				// No action
				break;
			}
		}

		world_Get()->CutImprovements(pos);

		if (!river && !g_placeGoodsMode)
        {
			sint32 radius = 1;
			switch(ScenarioEditor::BrushSize())
            {
			default:
            // case 2:
				break;
			case 1:
				radius = 0;
				break;
			case 4:
				radius = 2;
				break;
			}

			world_Get()->SmartSetTerrain(pos, tileNum, radius);
		}

#if 0

		world_Get()->GetCell(pos)->CalcTerrainMoveCost();

		if (world_Get()->GetCell(pos)->IsAnyUnitInCell()) {
			if (!world_Get()->GetCell(pos)->UnitArmy()->CanEnter(pos)) {
				world_Get()->GetCell(pos)->UnitArmy()->KillList(CAUSE_REMOVE_ARMY_CHEAT, -1);
			}
		}

		if (world_Get()->HasCity(pos)) {
			if (!world_Get()->CanEnter(pos, world_Get()->GetCell(pos)->GetCity().GetMovementType())) {
				world_Get()->GetCell(pos)->GetCity().KillUnit(CAUSE_REMOVE_ARMY_CHEAT, -1);
			}
		}
#endif





	}

	if ( ScenarioEditor::PlaceUnitsMode() || ScenarioEditor::PlaceCityMode()) {

		sint32 unitNum = ScenarioEditor::PlaceUnitsMode() ? ScenarioEditor::UnitIndex() : g_unitNum;

		Player *p = player_Get(selitem_Get()->GetVisiblePlayer());
		if (!p) return;
		sint32 govType = p->GetGovernmentType();

		if(ScenarioEditor::PlaceCityMode()) {
			sint32 ui;
			for(ui = 0; ui < g_theUnitDB->NumRecords(); ui++) {
				if(g_theUnitDB->Get(ui, govType)->GetHasPopAndCanBuild() &&
				   g_theUnitDB->Get(ui, govType)->GetMovementTypeLand()) {
					unitNum = ui;
					break;
				}
			}
			Assert(ui < g_theUnitDB->NumRecords());
		}

		if (g_killMode) {

			Cell *cell = world_Get()->GetCell(pos);
			if(cell->UnitArmy()) {
				cell->UnitArmy()->KillList(CAUSE_REMOVE_ARMY_TOE, -1);
				selitem_Get()->Deselect(selitem_Get()->GetVisiblePlayer());
			}

			if (0 != cell->GetCity().m_id) {
				cell->GetCity().KillUnit(CAUSE_REMOVE_ARMY_TOE, -1);
			}
		} else
		if (unitNum != -1) {
			if (world_Get()->CanEnter(pos, g_theUnitDB->Get(unitNum, govType)->GetMovementType()) ||
				g_theUnitDB->Get(unitNum, govType)->GetHasPopAndCanBuild() ||
				g_theUnitDB->Get(unitNum, govType)->GetIsTrader()) {

				if(g_theUnitDB->Get(unitNum, govType)->GetHasPopAndCanBuild()) {
					if(world_Get()->IsWater(pos) || world_Get()->IsShallowWater(pos)) {
						sint32 i;
						for(i = 0; i < g_theUnitDB->NumRecords(); i++) {
							if(g_theUnitDB->Get(i, govType)->GetHasPopAndCanBuild() &&
								g_theUnitDB->Get(i, govType)->GetMovementTypeSea()) {
								unitNum = i;
								break;
							}
						}
					}
					Unit id1 = p->CreateCity(unitNum, pos, CAUSE_NEW_CITY_CHEAT, nullptr, -1);
					//Added by Martin G�hmann to make the created city selected.
					selitem_Get()->SetSelectCity(id1);
					//End Add
				} else {

					if (world_Get()->HasCity(pos)) {
						if (world_Get()->GetCell(pos)->GetCity().GetOwner() == selitem_Get()->GetVisiblePlayer()) {
							Unit id1 = p->CreateUnit(unitNum, pos, Unit(), FALSE, CAUSE_NEW_ARMY_CHEAT);
						}
					} else {

						Unit id1 = p->CreateUnit(unitNum, pos, Unit(), FALSE, CAUSE_NEW_ARMY_CHEAT);
					}
				}
			}
		}
	}
}







void TiledMap::AdjustForOverlappingSprite(POINT mousePt, MapPoint &pos)
{
	MapPoint		newPos;
	Unit			top;

	pos.GetNeighborPosition(SOUTH, newPos);
	if (world_Get()->GetTopVisibleUnit(newPos, top)) {
		if (top.GetActor()) {
			if (top.GetActor()->HitTest(mousePt)) {
				pos = newPos;
				return;
			}
		}
	}

	pos.GetNeighborPosition(SOUTHWEST, newPos);
	if (world_Get()->GetTopVisibleUnit(newPos, top)) {
		if (top.GetActor()) {
			if (top.GetActor()->HitTest(mousePt)) {
				pos = newPos;
				return;
			}
		}
	}

	pos.GetNeighborPosition(SOUTHEAST, newPos);
	if (world_Get()->GetTopVisibleUnit(newPos, top)) {
		if (top.GetActor()) {
			if (top.GetActor()->HitTest(mousePt)) {
				pos = newPos;
				return;
			}
		}
	}

}

void TiledMap::MouseDrag(aui_MouseEvent *data)
{
	MapPoint		pos;

    if (GetMouseTilePos(pos)) {




		if (data->lbutton && !data->rbutton) {

			if (g_isCheatModeOn || ScenarioEditor::HandleClicks()) {

				if(ScenarioEditor::SelectRegion()) {
					ScenarioEditor::ExpandRegion(pos);
				} else {
					HandleCheat(pos);
				}
			} else {
				selitem_Get()->RegisterClick(pos, data, FALSE,
											   true, false);
			}
		}
	}
}

void TiledMap::Click(aui_MouseEvent *data, bool doubleClick)
{
	MapPoint		pos;
	POINT			point = data->position;

	if (MousePointToTilePos(point, pos))
	{
		if (data->lbutton && !data->rbutton)
		{
			if (g_isCheatModeOn || ScenarioEditor::HandleClicks())
			{
				if(ScenarioEditor::SelectRegion())
				{
					ScenarioEditor::StartRegion(pos);
				}
				else if(ScenarioEditor::PasteMode())
				{
					ScenarioEditor::Paste(pos);
				}
				else
				{
					HandleCheat(pos);
				}
			}
			else if ( g_isTransportOn )
			{
				g_isTransportOn = FALSE;
				selitem_Get()->Deselect( selitem_Get()->GetVisiblePlayer() );
			}
			else if (g_tileImprovementMode)
			{
				// Nothing
			}
			else
			{
				selitem_Get()->RegisterClick(pos, data, doubleClick,
											   false, false);
			}
		}
		else
		{
			if (data->rbutton && !data->lbutton)
			{
				if ( g_tileImprovementMode )
				{
					selitem_Get()->Deselect( selitem_Get()->GetVisiblePlayer() );
				}
				else
				{
					selitem_Get()->RegisterClick(pos, data, doubleClick, false, false);
				}
			}
			else
			{
				// Nothing
			}
		}
	}
}

void TiledMap::Drop(aui_MouseEvent *data)
{
	MapPoint		pos;
	POINT           point = data->position;

	if (MousePointToTilePos(point, pos)) {

		if (g_isCheatModeOn) {

		}
		else if(ScenarioEditor::SelectRegion()) {
			ScenarioEditor::EndRegion(pos);
		}
		else if (g_tradeSelectedState)
		{

			g_tradeSelectedState = FALSE;
	        TradeRoute * route;
			g_grabbedItem->GetGrabbedItem(&route);

			route->SetPathSelectionState(k_TRADEROUTE_NO_PATH);

			if (!route->IsSelectedPathSame())
			{

				route->UpdateSelectedCellData(*route);
			}
			else route->ClearSelectedPath();
		}
		else {
			selitem_Get()->RegisterClick(pos, data, FALSE,
										   false, true);
		}
	}
}

void TiledMap::Idle()
{
	// Nothing to idle without a world (UI idle pump during game teardown).
	if (!world_Get()) return;

	MapPoint point;
	if (!GetMouseTilePos(point)) return;

	if (g_tradeSelectedState)
	{
	    TradeRoute * route;
		g_grabbedItem->GetGrabbedItem(&route);

		route->SetPathSelectionState(k_TRADEROUTE_SELECTED_PATH);
		route->GenerateSelectedPath(point);
	}
}

bool TiledMap::GetMousePos(POINT &pos) const
{
	Assert(c3ui_Get() && background_Get());
	if (!c3ui_Get() || !background_Get()) return false;

	aui_Mouse * mouse = c3ui_Get()->TheMouse();
	if (mouse == nullptr) return false;

	pos.x = mouse->X() - background_Get()->X();
	pos.y = mouse->Y() - background_Get()->Y();
	return true;
}

bool TiledMap::GetMouseTilePos(MapPoint &pt) const
{
	POINT   pos;
	return GetMousePos(pos) && MousePointToTilePos(pos, pt);
}

TileInfo *TiledMap::GetTileInfo(const MapPoint &pos)
{

	if (m_localVision != nullptr) {
		if(!m_localVision->IsVisible(pos)) {
			UnseenCellCarton ucell;
			if(m_localVision->GetLastSeen(pos, ucell))
				return ucell.m_unseenCell->GetTileInfo();
		}
	}
	return world_Get()->GetTileInfo(pos);
}

void TiledMap::NextPlayer()
{
	m_nextPlayer = TRUE;
}

void TiledMap::CopyVision()
{
	sint32  newPlayer   = selitem_Get()->GetVisiblePlayer();
	if (player_Get(newPlayer))
	{
		m_localVision->SetAmOnScreen(false);
		m_localVision = player_Get(newPlayer)->m_vision;
		m_oldPlayer   = newPlayer;
		m_localVision->SetAmOnScreen(true);
	}

	Refresh();
	InvalidateMap();
}

//----------------------------------------------------------------------------
//
// Name       : TiledMap::ReadyToDraw
//
// Description: Determine whether the display should contain visible tiles.
//
// Parameters : -
//
// Globals    : g_network       : multiplayer information
//              slicengine_Get(): general game engine
//              selitem_Get() : selected item on screen
//              turn_Get()      : turn information
//
// Returns    : bool            : tiles may be drawn
//
// Remark(s)  : The tiles may not be drawn in the following cases:
//              - For multiplayer games, when the network is not ready.
//              - When the game engine says so (e.g. between turns in hotseat).
//              - When there is no selected item (yet).
//              - Before the actual start of the game.
//
//----------------------------------------------------------------------------
bool TiledMap::ReadyToDraw() const
{
	if ((network_Get().IsActive() || network_Get().IsNetworkLaunch()) &&
	    !network_Get().ReadyToStart()
       )
    {
		return false;
    }

    return slicengine_Get() && !slicengine_Get()->ShouldScreenBeBlank() &&
           selitem_Get()  &&
           turn_Get()       &&
                ((turn_Get()->GetRound() > 0) || (m_localVision->GetOwner() > 0));
}

sint32
TiledMap::DrawOverlayClipped(aui_Surface *surface, Pixel16 *data, sint32 x, sint32 y, sint32 flags)
{
	uint8			*surfBase;
	sint32			surfWidth;
	sint32			surfHeight;
	sint32			surfPitch;
	sint32			errcode;

	if (data == nullptr)
		return 0;

	if (surface)
	{
		errcode = surface->Lock(nullptr, (LPVOID *)&surfBase, 0);
		Assert(errcode == AUI_ERRCODE_OK);

		if ( errcode != AUI_ERRCODE_OK )
			return AUI_ERRCODE_SURFACELOCKFAILED;

		surfWidth	= surface->Width();
		surfHeight	= surface->Height();
		surfPitch	= surface->Pitch();
	}
	else
	{
		surfBase	= m_surfBase;
		surfWidth	= m_surfWidth;
		surfHeight	= m_surfHeight;
		surfPitch	= m_surfPitch;
	}

	if ((x>=surfPitch)||(y>=surfHeight))
	   return 0 ;

	if ((x < 0) || (y < 0))
		return 0;

	bool const bpp32 = surface ? (surface->BitsPerPixel() == 32)
	                           : (m_lockedSurface && m_lockedSurface->BitsPerPixel() == 32);
	sint32 const step = bpp32 ? 4 : 2;
	uint8		*destPixel;

	uint16		start	= (uint16)*data++;
	uint16		end		= (uint16)*data++;
	Pixel16		*table	= data;
	Pixel16		*dataStart = table + (end - start + 1);

	sint32 len;
	sint32 looplen;

	sint32 xoff=x;
	sint32 i;

	Pixel16		*rowData;
	Pixel16		tag;

	for (sint32 j = start; j <= end; j++)
	{
		destPixel = surfBase + ((y + j) * surfPitch) + (x * step);

		if ((y+j) >= surfHeight)
			return 0;

		if ((sint16)table[j-start] == -1)
			continue;

		rowData = dataStart + table[j-start];

		do
		{
			tag = *rowData++;
			len = (tag & 0x00FF);

			switch ((tag & 0x0F00) >> 8)
			{
				case	k_TILE_SKIP_RUN_ID	:
						destPixel	+= len * step;
						xoff		+= len;
						break;

				case	k_TILE_COPY_RUN_ID			:

						looplen = len;

						if (xoff<0)
						{
							looplen   += xoff;
							destPixel -= xoff * step;
							rowData	  -= xoff;
						}
						else
							if (xoff>surfPitch)
								looplen -= (xoff-surfPitch);

						for (i=0; i<looplen; i++)
						{
							if (!(flags & k_OVERLAY_FLAG_SHADOWSONLY))
								pixelutils_StorePixel(destPixel + i * step, rowData[i], bpp32);
						}

						destPixel += len * step;
						rowData   += len;
						break;

				case	k_TILE_SHADOW_RUN_ID		:

						looplen = len;

						if (xoff<0)
						{
							looplen += xoff;
							destPixel -= xoff * step;
						}
						else
							if (xoff>surfPitch)
								looplen -= (xoff-surfPitch);

						for (i=0; i<looplen; i++)
						{
					  		if (!(flags & k_OVERLAY_FLAG_NOSHADOWS))
							{
								if (bpp32)
								{
									Pixel32 * d = reinterpret_cast<Pixel32 *>(destPixel + i * step);
									*d = pixelutils_Shadow8888(*d);
								}
								else
								{
									Pixel16 * d = reinterpret_cast<Pixel16 *>(destPixel + i * step);
									*d = pixelutils_Shadow(*d);
								}
							}
						}

						destPixel += len * step;
						break;
			}

		} while ((tag & 0xF000) == 0);
	}

	if (surface)
	{
		errcode = surface->Unlock((LPVOID *)surfBase);
		Assert(errcode == AUI_ERRCODE_OK);
		if ( errcode != AUI_ERRCODE_OK ) return AUI_ERRCODE_SURFACEUNLOCKFAILED;
	}

	return 0;
}

void
TiledMap::DrawTransitionTileClipped(aui_Surface *surface, MapPoint &pos, sint32 xpos, sint32 ypos)
{
	if (!surface)
		surface = m_surface;

	uint8 * pSurfBase			= m_surfBase;
	sint32 surfWidth	= m_surfWidth;
	sint32 surfHeight	= m_surfHeight;
	sint32 surfPitch	= m_surfPitch;
	bool const bpp32 = m_lockedSurface && m_lockedSurface->BitsPerPixel() == 32;
	sint32 const step = bpp32 ? 4 : 2;

	ypos+=k_TILE_PIXEL_HEADROOM;

	if (xpos > (surfWidth-k_TILE_PIXEL_WIDTH))
		return;
	if (ypos > (surfPitch-k_TILE_PIXEL_HEIGHT))
		return;

	TileInfo * tileInfo = GetTileInfo(pos);
	Assert(tileInfo);
	if (tileInfo == nullptr)
		return;

	uint16 index = tileInfo->GetTileNum();

	BaseTile * baseTile = m_tileSet->GetBaseTile(index);
	if (baseTile == nullptr)
		return;

	Pixel16 *data = baseTile->GetTileData();

	Pixel16	*t[4];
	Pixel16  defaults[4];

	t[0] = m_tileSet->GetTransitionData(tileInfo->GetTerrainType(), tileInfo->GetTransition(0), 0);
	t[1] = m_tileSet->GetTransitionData(tileInfo->GetTerrainType(), tileInfo->GetTransition(1), 1);
	t[2] = m_tileSet->GetTransitionData(tileInfo->GetTerrainType(), tileInfo->GetTransition(2), 2);
	t[3] = m_tileSet->GetTransitionData(tileInfo->GetTerrainType(), tileInfo->GetTransition(3), 3);

	defaults[0] = 0xF800;
	defaults[1] = 0x07E0;
	defaults[2] = 0x001F;
	defaults[3] = 0xF81F;

	Pixel16 * dataPtr = data;

	for (sint32 y = 0; y < k_TILE_PIXEL_HEIGHT; ++y)
	{
		sint32 const startX  = (y <= 23) ? (23-y)*2 : (y-24)*2;
		sint32 const endX    = k_TILE_PIXEL_WIDTH - startX;

		for (sint32 x = startX; x < endX; ++x)
		{
			Pixel16 srcPixel    = *dataPtr++;
			Pixel16 tindex      = srcPixel;

			if (tindex<4)
			{
				if (t[tindex])
				{
					srcPixel = *(t[tindex]);
					t[tindex] ++;
				}
				else
					srcPixel = defaults[tindex];
			}

			int ysrc = (y+ypos);

			if (ysrc<0)
				continue;

			if (ysrc>=surfHeight)
				return;

			int xsrc = (x+xpos);

			if (xsrc<0)
				continue;

			sint32 const xbyte = xsrc * step;

		  	if (xbyte>=surfPitch)
				continue;

			uint8 * pDestPixel = pSurfBase + (ysrc*surfPitch + xbyte);

			pixelutils_StorePixel(pDestPixel, srcPixel, bpp32);
		}
	}
}






void TiledMap::SetZoomLevel(sint32 level)
{

	Assert(level >= 0 && level < k_MAX_ZOOM_LEVELS);

	m_zoomLevel = level;

	m_scale = m_zoomTileScale[m_zoomLevel];

	if(m_zoomCallback)
		m_zoomCallback();
}

bool TiledMap::CanZoomIn() const
{

	return(GetZoomLevel() < k_ZOOM_LARGEST);
}

bool TiledMap::ZoomIn()
{

	if(CanZoomIn()) {

		ZoomUpdate(GetZoomLevel() + 1);

		return(true);
	}

	return(false);
}

bool TiledMap::CanZoomOut() const
{

	if(GetZoomLevel() > k_ZOOM_SMALLEST) {

		RECT zoomViewRectangle;
		CalculateZoomViewRectangle(GetZoomLevel() - 1, zoomViewRectangle);

		sint32 width;
		sint32 height;
		GetMapMetrics(&width, &height);

		if((zoomViewRectangle.right <= width) &&
			(zoomViewRectangle.bottom <= height))
			return(true);
	}

	return(false);
}

bool TiledMap::ZoomOut()
{

	if(CanZoomOut()) {

		ZoomUpdate(GetZoomLevel() - 1);

		return(true);
	}

	return(false);
}

void TiledMap::CalculateZoomViewRectangle(sint32 zoomLevel, RECT &rectangle) const
{

	sint32 width = m_displayRect.right - m_displayRect.left;
	sint32 height = m_displayRect.bottom - m_displayRect.top;

	rectangle.left = rectangle.top = 0;

	rectangle.right = (width - (m_zoomTilePixelWidth[zoomLevel] / 2)) /
		m_zoomTilePixelWidth[zoomLevel];
	rectangle.bottom = (height - m_zoomTileHeadroom[zoomLevel]) /
		(m_zoomTilePixelHeight[zoomLevel] / 2) - 1;
}

void TiledMap::ZoomHitMask()
{
	for (auto & i : m_tileHitMask) {
   		i.start = (sint16)(i.d_start * m_scale);
		i.end   = (sint16)(i.d_end * m_scale);
	}
}

void TiledMap::ZoomUpdate(sint32 zoomLevel)
{

	sint32 mapViewCenterX = (m_mapViewRect.left + m_mapViewRect.right) / 2;
	sint32 mapViewCenterY = (m_mapViewRect.top + m_mapViewRect.bottom) / 2;
	sint32 mapViewCenterXWrap = 0;
	sint32 mapViewCenterYWrap = 0;
	maputils_WrapPoint(mapViewCenterX, mapViewCenterY,
		&mapViewCenterXWrap, &mapViewCenterYWrap);





	sint32 mapViewCenterXTile = maputils_TileX2MapX(mapViewCenterXWrap, mapViewCenterYWrap);

	SetZoomLevel(zoomLevel);

	CalculateMetrics();

	radar_map_Get()->CenterMap(MapPoint(mapViewCenterXTile, mapViewCenterYWrap));

	Refresh();

	InvalidateMap();

	ZoomHitMask();
}
