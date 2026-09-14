//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Tile improvement handling
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
// - Added option to show info for tile improvements that are too expensive
//   and made it modifiable in-game.
// - Added a construction time line to the tileimp tracker window. (Aug 14th 2005 Martin G�hmann)
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/interface/tileimptracker.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_ctp2/c3_static.h"
#include "gs/utility/gstypes.h"            // TERRAIN_TYPES
#include "ui/aui_utils/primitives.h"
#include "ui/aui_ctp2/SelItem.h"
#include "gs/gameobj/TerrImprovePool.h"
#include "gs/gameobj/player.h"
#include "gfx/tilesys/maputils.h"
#include "gs/world/World.h"
#include "gs/world/Cell.h"
#include "gfx/gfx_utils/colorset.h"           // colorset_Get()
#include "gs/database/profileDB.h"          // profiledb_Get()
#include "gs/gameobj/terrainutil.h"
#include "TerrainRecord.h"


namespace
{
    COLOR               s_trackerBorderColor    = COLOR_GREEN;
}

TileimpTrackerWindow    *g_tileImpTrackerWindow = nullptr;
static c3_Static        *s_trackerTimeN         = nullptr;
static c3_Static        *s_trackerTimeV         = nullptr;
static c3_Static        *s_trackerMatN          = nullptr;
static c3_Static        *s_trackerMatV          = nullptr;
static c3_Static        *s_trackerAdvN          = nullptr;
static c3_Static        *s_trackerAdvV          = nullptr;
static c3_Static        *s_trackerFoodN         = nullptr;
static c3_Static        *s_trackerFoodV         = nullptr;
static c3_Static        *s_trackerProductionN   = nullptr;
static c3_Static        *s_trackerProductionV   = nullptr;
static c3_Static        *s_trackerGoldN         = nullptr;
static c3_Static        *s_trackerGoldV         = nullptr;
static c3_Static        *s_trackerBackground    = nullptr;

static sint32 s_tileImprovementNum = -1;

//----------------------------------------------------------------------------
//
// Name       : tileimptracker_Initialize
//
// Description: Initializes the tileimp tracker window.
//
// Parameters : -
//
// Globals    : g_tileImpTrackerWindow: The tile improvement tracker window
//
// Returns    : 0 if initalization was successfull, -1 otherwise.
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
sint32 tileimptracker_Initialize()
{
	MBCHAR          textBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	MBCHAR          controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	AUI_ERRCODE errcode = AUI_ERRCODE_OK;

	strlcpy( textBlock, "cp_tileimp_tracker", sizeof(textBlock));
	g_tileImpTrackerWindow = new TileimpTrackerWindow( &errcode, aui_UniqueId(), textBlock, 16);
	Assert( AUI_NEWOK(g_tileImpTrackerWindow, errcode) );
	if( !AUI_SUCCESS(errcode) ) return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", textBlock, "TimeN");
	s_trackerTimeN = new c3_Static( &errcode, aui_UniqueId(), controlBlock );
	Assert( AUI_NEWOK(s_trackerTimeN, errcode) );
	if( !AUI_SUCCESS(errcode) ) return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", textBlock, "TimeV");
	s_trackerTimeV = new c3_Static( &errcode, aui_UniqueId(), controlBlock );
	Assert( AUI_NEWOK(s_trackerTimeV, errcode) );
	if( !AUI_SUCCESS(errcode) ) return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", textBlock, "MatN");
	s_trackerMatN = new c3_Static( &errcode, aui_UniqueId(), controlBlock );
	Assert( AUI_NEWOK(s_trackerMatN, errcode) );
	if( !AUI_SUCCESS(errcode) ) return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", textBlock, "MatV");
	s_trackerMatV = new c3_Static( &errcode, aui_UniqueId(), controlBlock );
	Assert( AUI_NEWOK(s_trackerMatV, errcode) );
	if( !AUI_SUCCESS(errcode) ) return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", textBlock, "FoodN");
	s_trackerFoodN = new c3_Static( &errcode, aui_UniqueId(), controlBlock );
	Assert( AUI_NEWOK(s_trackerFoodN, errcode) );
	if( !AUI_SUCCESS(errcode) ) return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", textBlock, "FoodV");
	s_trackerFoodV = new c3_Static( &errcode, aui_UniqueId(), controlBlock );
	Assert( AUI_NEWOK(s_trackerFoodV, errcode) );
	if( !AUI_SUCCESS(errcode) ) return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", textBlock, "ProductionN");
	s_trackerProductionN = new c3_Static( &errcode, aui_UniqueId(), controlBlock );
	Assert( AUI_NEWOK(s_trackerProductionN, errcode) );
	if( !AUI_SUCCESS(errcode) ) return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", textBlock, "ProductionV");
	s_trackerProductionV = new c3_Static( &errcode, aui_UniqueId(), controlBlock );
	Assert( AUI_NEWOK(s_trackerProductionV, errcode) );
	if( !AUI_SUCCESS(errcode) ) return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", textBlock, "GoldN");
	s_trackerGoldN = new c3_Static( &errcode, aui_UniqueId(), controlBlock );
	Assert( AUI_NEWOK(s_trackerGoldN, errcode) );
	if( !AUI_SUCCESS(errcode) ) return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", textBlock, "GoldV");
	s_trackerGoldV = new c3_Static( &errcode, aui_UniqueId(), controlBlock );
	Assert( AUI_NEWOK(s_trackerGoldV, errcode) );
	if( !AUI_SUCCESS(errcode) ) return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", textBlock, "Background");
	s_trackerBackground = new c3_Static(&errcode, aui_UniqueId(), controlBlock);
	Assert(AUI_NEWOK(s_trackerBackground, errcode));
	if(!AUI_SUCCESS(errcode)) return -1;

	errcode = aui_Ldl::SetupHeirarchyFromRoot( textBlock );
	Assert( AUI_SUCCESS(errcode) );
	if ( !AUI_SUCCESS(errcode) ) return -1;

	return 0;
}

//----------------------------------------------------------------------------
//
// Name       : tileimptracker_DisplayData
//
// Description: Displays the food, production and gold boni at the given
//              position that the given tileimp generates. In addition it
//              displays the needed time for construction and the PW costs.
//
// Parameters : MapPoint &p: Position at that the given tileimp should be
//              contructed.
//              sint32 type: The type of the tileimp in question.
//
// Globals    : selitem_Get():           The currently selected item
//              world_Get():                The game world
//              g_theTerrainImprovementDB: The tile improvement database
//              g_theTerrainDB:            The terrain database
//              profiledb_Get():            The player's profile
//              g_tileImpTrackerWindow:    The tile improvement tracker window
//              c3ui_Get():                    The civilization 3 graphical user interface
//
// Returns    : -
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
void tileimptracker_DisplayData(MapPoint const & p, sint32 type)
{
	if(!g_tileImpTrackerWindow) {
		tileimptracker_Initialize();
	}
	Assert(g_tileImpTrackerWindow);
	if(!g_tileImpTrackerWindow)
		return;

	MBCHAR		mytext[256];
	sint32		 x;
	sint32		 y;
	sint32		visPlayer = selitem_Get()->GetVisiblePlayer();

	s_tileImprovementNum = type;
	if (s_tileImprovementNum < 0) {
		c3ui_Get()->RemoveWindow(g_tileImpTrackerWindow->Id());
		return;
	}

	sint32 extraData = 0;







	BOOL alreadyHasIt = terrimprovepool_Get()->HasImprovement(p,
																	TERRAIN_IMPROVEMENT(s_tileImprovementNum),
																	extraData);

	if(player_Get(visPlayer)->IsExplored(p) && !alreadyHasIt) {

		maputils_MapXY2PixelXY(p.x,p.y,&x,&y);


		g_tileImpTrackerWindow->Move(x,y);

//		TERRAIN_TYPES terr = world_Get()->GetTerrainType(p);

		sint32  mat;
		sint32  time;
		sint32  food;
		sint32  production;
		sint32  gold;

		sint32 extraData = 0;

		time = terrainutil_GetProductionTime(s_tileImprovementNum, p, 0);
		mat = terrainutil_GetProductionCost(s_tileImprovementNum, p, 0);

		const TerrainImprovementRecord *rec = g_theTerrainImprovementDB->Get(s_tileImprovementNum);
		Cell *cell = world_Get()->GetCell(p);

		if(rec->GetClassTerraform() || rec->GetClassOceanform())
		{
			sint32 t;
			if(rec->GetTerraformTerrainIndex(t))
			{
				const TerrainRecord *newT = g_theTerrainDB->Get(t);
				sint32 oldFood = cell->GetFoodProduced();
				sint32 oldProd = cell->GetShieldsProduced();
				sint32 oldGold = cell->GetGoldProduced();

				sint32 newFood = newT->GetEnvBase()->GetFood();
				sint32 newProd = newT->GetEnvBase()->GetShield();
				sint32 newGold = newT->GetEnvBase()->GetGold();

				if (cell->HasRiver() && newT->HasEnvRiver())
				{
					newFood += newT->GetEnvRiverPtr()->GetFood();
					newProd += newT->GetEnvRiverPtr()->GetShield();
					newGold += newT->GetEnvRiverPtr()->GetGold();
				}

				food       = newFood - oldFood;
				production = newProd - oldProd;
				gold       = newGold - oldGold;
			}
			else
			{
				Assert(false);
				food = 0;
				production = 0;
				gold = 0;
			}
		}
		else
		{
			const TerrainImprovementRecord::Effect *eff = terrainutil_GetTerrainEffect(g_theTerrainImprovementDB->Get(s_tileImprovementNum), p);

			sint32 dFood = 0;
			if(eff) eff->GetBonusFood(dFood);
			sint32 dProd = 0;
			if(eff) eff->GetBonusProduction(dProd);
			sint32 dGold = 0;
			if(eff) eff->GetBonusGold(dGold);

			food = dFood + cell->GetFoodFromTerrain();
			production = dProd + cell->GetShieldsFromTerrain();
			extraData = 0;

			gold = dGold + cell->GetGoldFromTerrain();
		}

		snprintf(mytext, sizeof(mytext),"%d", (sint32)time);
		s_trackerTimeV->SetText(mytext);
		snprintf(mytext, sizeof(mytext),"%d", (sint32)mat);
		s_trackerMatV->SetText(mytext);

		snprintf(mytext, sizeof(mytext),"%d", food);
		s_trackerFoodV->SetText(mytext);
		snprintf(mytext, sizeof(mytext),"%d", production);
		s_trackerProductionV->SetText(mytext);
		snprintf(mytext, sizeof(mytext),"%d", gold);
		s_trackerGoldV->SetText(mytext);

		ERR_BUILD_INST err;
		bool const	checkMaterials	= !profiledb_Get()->GetValueByName("ShowExpensive");

		if (player_Get(visPlayer)->CanCreateImprovement
				(TERRAIN_IMPROVEMENT(s_tileImprovementNum), p, extraData, checkMaterials, err)
		   )
		{
			if (player_Get(visPlayer)->CanCreateImprovement
					(TERRAIN_IMPROVEMENT(s_tileImprovementNum), p, extraData, true, err)
			   )
			{
				s_trackerBorderColor = COLOR_GREEN;
			}
			else
			{
				s_trackerBorderColor = COLOR_RED;
			}
			c3ui_Get()->AddWindow(g_tileImpTrackerWindow);
		}
		else
		{
			c3ui_Get()->RemoveWindow(g_tileImpTrackerWindow->Id());
		}
		g_tileImpTrackerWindow->ShouldDraw();

		return;
	}
}

static void mycleanup(c3_Static * & mypointer)
{ delete mypointer; mypointer = nullptr; }

//----------------------------------------------------------------------------
//
// Name       : tileimptracker_Cleanup
//
// Description: Cleans up the memory that was occupied by the tileimp
//              tracker window.
//
// Parameters : -
//
// Globals    : g_tileImpTrackerWindow:    The tile improvement tracker window
//              c3ui_Get():                    The civilization 3 graphical user interface
//
// Returns    : 1 if there is nothing to cleanup, otherwise 0.
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
void tileimptracker_Cleanup()
{
	if (g_tileImpTrackerWindow && c3ui_Get())
    {
    	c3ui_Get()->RemoveWindow(g_tileImpTrackerWindow->Id());
    }

	mycleanup(s_trackerBackground);
	mycleanup(s_trackerTimeN);
	mycleanup(s_trackerTimeV);
	mycleanup(s_trackerMatN);
	mycleanup(s_trackerMatV);
	mycleanup(s_trackerFoodN);
	mycleanup(s_trackerFoodV);
	mycleanup(s_trackerProductionN);
	mycleanup(s_trackerProductionV);
	mycleanup(s_trackerGoldN);
	mycleanup(s_trackerGoldV);

	delete g_tileImpTrackerWindow;
	g_tileImpTrackerWindow = nullptr;
}

//----------------------------------------------------------------------------
//
// Name       : TileimpTrackerWindow::DrawThis
//
// Description: Draws the tileimp tracker window
//
// Parameters : aui_Surface *surface: The surface to draw on
//              sint32 x:             The x coordinate
//              sint32 y:             The y coordinate
//
// Globals    : colorset_Get(): The color set
//
// Returns    : Returns always AUI_ERRCODE_OK.
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
AUI_ERRCODE TileimpTrackerWindow::DrawThis(aui_Surface *surface, sint32 x, sint32 y)
{
	if(!surface) surface = m_surface;

	RECT rect = { 0, 0, m_width, m_height };

	primitives_PaintRect16(surface, &rect, 0x0000);

	C3Window::DrawThis(surface,x,y);

	primitives_FrameRect16(surface, &rect, colorset_Get()->GetColor(s_trackerBorderColor));

	return AUI_ERRCODE_OK;
}
