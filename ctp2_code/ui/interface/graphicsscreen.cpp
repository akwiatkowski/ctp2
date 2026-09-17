#include "ctp/c3.h"
#include "ui/interface/graphicsresscreen.h"

#include "gs/database/profileDB.h"      // profiledb_Get()
#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_ctp2/c3_popupwindow.h"
#include "ui/aui_ctp2/ctp2_button.h"
#include "ui/aui_ctp2/c3_listitem.h"
#include "ui/aui_ctp2/c3_dropdown.h"
#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/c3slider.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_common/aui_switch.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/interface/spnewgamewindow.h"
#include "ui/interface/graphicsscreen.h"
#include "ui/aui_ctp2/keypress.h"
#include "gfx/tilesys/tiledmap.h"       // tiledmap_Get();
#include "ctp/civapp.h"
#include "gfx/spritesys/SpriteGroupList.h"

extern sint32				g_isGridOn;
extern SpriteGroupList		*g_unitSpriteGroupList;
extern SpriteGroupList		*g_goodSpriteGroupList;

static ctp2_Button	*s_resScreenButton;
static c3_PopupWindow *s_graphicsWindow			= nullptr;
static C3Slider		*s_bright					= nullptr,
					*s_gamma					= nullptr,
					*s_color					= nullptr,
					*s_contrast					= nullptr;

static c3_Static	*s_unitSpeedN				= nullptr;
static C3Slider		*s_unitSpeed				= nullptr;

static aui_Switch	*s_walk						= nullptr,

					*s_trade					= nullptr,
					*s_wonder					= nullptr,

					*s_politicalBorders			= nullptr,
					*s_tradeRoutes				= nullptr,

					*s_cityInfluence			= nullptr,
					*s_grid						= nullptr,

					*s_cityNames				= nullptr,
					*s_civflags					= nullptr,
					*s_smooth					= nullptr,
					*s_armyNames				= nullptr,
					*s_goodAnims				= nullptr,
					*s_cityProd					= nullptr;

static BOOL			s_gridToggled				= FALSE;
static BOOL			s_cityInfluenceToggled		= FALSE;
static BOOL			s_politicalBordersToggled	= FALSE;
static BOOL			s_unitAnimToggled			= FALSE;
static BOOL			s_goodAnimToggled			= FALSE;
//static BOOL			s_showCityProdToggled		= FALSE;

enum
{
	GS_WALK,

	GS_TRADE,
	GS_WONDER,

	GS_POLITICALBORDERS,
	GS_TRADEROUTES,
	GS_WATER,

	GS_CITYINFLUENCE,
	GS_GRID,
	GS_CITYNAMES,
	GS_ARMYNAMES,
	GS_CIVFLAGS,
	GS_SMOOTH,
	GS_GOODANIMS,
	GS_CITYPROD,

	GS_TOTAL
};

static uint32 check[] =
{
	GS_WALK,

	GS_TRADE,
	GS_WONDER,

	GS_POLITICALBORDERS,
	GS_TRADEROUTES,
	GS_WATER,

	GS_CITYINFLUENCE,
	GS_GRID,

	GS_CITYNAMES,
	GS_ARMYNAMES,
	GS_CIVFLAGS,
	GS_SMOOTH,
	GS_GOODANIMS,
	GS_CITYPROD,

	GS_TOTAL
};




sint32	graphicsscreen_displayMyWindow()
{
	sint32 retval=0;
	if (!s_graphicsWindow) { retval = graphicsscreen_Initialize(); }

	AUI_ERRCODE auiErr  = c3ui_Get()->AddWindow(s_graphicsWindow);
	Assert( auiErr == AUI_ERRCODE_OK );
	keypress_RegisterHandler(s_graphicsWindow);

	s_unitAnimToggled = FALSE;
	s_goodAnimToggled = FALSE;

	return retval;
}
sint32 graphicsscreen_removeMyWindow(uint32 action)
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return 0;

	AUI_ERRCODE auiErr = c3ui_Get()->RemoveWindow( s_graphicsWindow->Id() );
	Assert( auiErr == AUI_ERRCODE_OK );
	keypress_RemoveHandler(s_graphicsWindow);

	return 1;
}


AUI_ERRCODE graphicsscreen_Initialize( )
{
	s_gridToggled = FALSE;
	s_cityInfluenceToggled = FALSE;
	s_politicalBordersToggled = FALSE;

	if ( s_graphicsWindow ) return AUI_ERRCODE_OK;

	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	strlcpy(windowBlock, "GraphicsWindow", sizeof(windowBlock));
	s_graphicsWindow = new c3_PopupWindow(
		&errcode,
		aui_UniqueId(),
		windowBlock,
		16,
		AUI_WINDOW_TYPE_FLOATING,
		false );
	Assert( AUI_NEWOK(s_graphicsWindow, errcode) );
	if ( !AUI_NEWOK(s_graphicsWindow, errcode) ) return errcode;

	s_graphicsWindow->SetStronglyModal(TRUE);


	s_walk				= spNew_aui_Switch(&errcode,windowBlock,"WalkButton",graphicsscreen_checkPress,&check[GS_WALK]);
	s_trade				= spNew_aui_Switch(&errcode,windowBlock,"TradeButton",graphicsscreen_checkPress,&check[GS_TRADE]);
	s_wonder			= spNew_aui_Switch(&errcode,windowBlock,"WonderButton",graphicsscreen_checkPress,&check[GS_WONDER]);
	s_politicalBorders	= spNew_aui_Switch(&errcode,windowBlock,"PoliticalBordersButton",graphicsscreen_checkPress,&check[GS_POLITICALBORDERS]);
	s_tradeRoutes		= spNew_aui_Switch(&errcode,windowBlock,"TradeRoutesButton",graphicsscreen_checkPress,&check[GS_TRADEROUTES]);
	s_cityInfluence		= spNew_aui_Switch(&errcode,windowBlock,"CityInflenceButton",graphicsscreen_checkPress,&check[GS_CITYINFLUENCE]);
	s_grid				= spNew_aui_Switch(&errcode,windowBlock,"GridButton",graphicsscreen_checkPress,&check[GS_GRID]);
	s_cityNames			= spNew_aui_Switch(&errcode,windowBlock,"CityNamesButton", graphicsscreen_checkPress, &check[GS_CITYNAMES]);
	s_armyNames			= spNew_aui_Switch(&errcode,windowBlock,"ArmyNamesButton", graphicsscreen_checkPress, &check[GS_ARMYNAMES]);
	s_civflags			= spNew_aui_Switch(&errcode,windowBlock,"CivFlagButton", graphicsscreen_checkPress, &check[GS_CIVFLAGS]);

	s_resScreenButton	= spNew_ctp2_Button( &errcode, windowBlock, "ResolutionButton", graphicsscreen_selectResolution );

	s_smooth			= spNew_aui_Switch(&errcode,windowBlock,"SmoothButton", graphicsscreen_checkPress, &check[GS_SMOOTH]);
	s_goodAnims			= spNew_aui_Switch(&errcode,windowBlock,"GoodsButton",graphicsscreen_checkPress,&check[GS_GOODANIMS]);
	s_cityProd			= spNew_aui_Switch(&errcode,windowBlock,"ShowCityProdButton",graphicsscreen_checkPress,&check[GS_CITYPROD]);

	s_unitSpeed			= spNew_C3Slider(&errcode, windowBlock, "UnitSpeedSlider", graphicsscreen_unitSpeedSlide);
	s_unitSpeedN		= spNew_c3_Static(&errcode, windowBlock, "UnitSpeedName");

	if(profiledb_Get()) {
		s_unitSpeed->SetValue(profiledb_Get()->GetUnitSpeed(), 0);
	} else {
		s_unitSpeed->SetValue(0,0);
	}

	s_walk				->SetState(profiledb_Get()->IsUnitAnim());
	s_trade				->SetState(profiledb_Get()->IsTradeAnim());
	s_wonder			->SetState(profiledb_Get()->IsWonderMovies());
	s_politicalBorders	->SetState(profiledb_Get()->GetShowPoliticalBorders());
	s_tradeRoutes		->SetState(profiledb_Get()->GetShowTradeRoutes());
	s_cityNames			->SetState(profiledb_Get()->GetShowCityNames());
	s_armyNames			->SetState(profiledb_Get()->GetShowArmyNames());
	s_civflags			->SetState(profiledb_Get()->IsCivFlags());
	s_smooth			->SetState(profiledb_Get()->IsSmoothBorders());
	s_goodAnims			->SetState(profiledb_Get()->IsGoodAnim());
	s_cityInfluence		->SetState(profiledb_Get()->IsShowCityInfluence());
	s_grid				->SetState(g_isGridOn);
	s_cityProd			->SetState(profiledb_Get()->IsShowCityProduction());

	MBCHAR block[ k_AUI_LDL_MAXBLOCK + 1 ];
	snprintf(block, sizeof(block), "%s.%s", windowBlock, "Name" );
	s_graphicsWindow->AddTitle( block );
	s_graphicsWindow->AddClose( graphicsscreen_exitPress );

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );

	return AUI_ERRCODE_OK;
}


void graphicsscreen_Cleanup()
{
	if (c3ui_Get() && s_graphicsWindow)
    {
    	c3ui_Get()->RemoveWindow(s_graphicsWindow->Id());
	    keypress_RemoveHandler(s_graphicsWindow);
    }

#define mycleanup(mypointer) { delete mypointer; mypointer = nullptr; }
	mycleanup(s_walk);
	mycleanup(s_trade);
	mycleanup(s_wonder);
	mycleanup(s_cityInfluence);
	mycleanup(s_grid);
	mycleanup(s_politicalBorders);
	mycleanup(s_tradeRoutes);
	mycleanup(s_cityNames);
	mycleanup(s_resScreenButton);
	mycleanup(s_unitSpeed);
	mycleanup(s_unitSpeedN);
	mycleanup(s_graphicsWindow);
	mycleanup(s_armyNames);
	mycleanup(s_civflags);
	mycleanup(s_smooth);
	mycleanup(s_goodAnims);
	mycleanup(s_cityProd);
#undef mycleanup
}




void graphicsscreen_screensizeSelect(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_LISTBOX_ACTION_SELECT  ) return;

	if(s_graphicsWindow) callbackSetSelected(control,cookie);

}
void graphicsscreen_exitPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	if (civapp_Get()->IsGameLoaded()) {
		if (s_gridToggled) {
			if (tiledmap_Get()) {
				tiledmap_Get()->Refresh();
				tiledmap_Get()->InvalidateMap();
			}
		}
		if (s_cityInfluenceToggled || s_politicalBordersToggled) {
			if (tiledmap_Get()) {
				tiledmap_Get()->Refresh();
				tiledmap_Get()->InvalidateMap();
			}
		}
		if (s_unitAnimToggled) {
			if (civapp_Get()->IsGameLoaded()) {
				g_unitSpriteGroupList->RefreshBasicLoads(GROUPTYPE_UNIT);
			}
		}
		// @todo fix updating good anims option mid-game.
		// This doesn't work for goods, unlike units above.
		if (s_goodAnimToggled) {
			if (civapp_Get()->IsGameLoaded()) {
				g_goodSpriteGroupList->RefreshBasicLoads(GROUPTYPE_GOOD);
			}
		}
	}

	profiledb_Get()->Save();

	graphicsscreen_removeMyWindow(action);
}

void graphicsscreen_checkPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_SWITCH_ACTION_PRESS ) return;

	uint32 checkbox = *((uint32*)cookie);
	void (ProfileDB::*func)(BOOL) = nullptr;
	uint32 state = data;

	switch(checkbox) {
	case GS_WALK:
		func = &ProfileDB::SetUnitAnim;
		s_unitAnimToggled = TRUE;
		break;
	case GS_TRADE:
		func = &ProfileDB::SetTradeAnim;
		break;
	case GS_WONDER:
		func = &ProfileDB::SetWonderMovies;
		break;
	case GS_POLITICALBORDERS:
		func = &ProfileDB::SetShowPoliticalBorders;
		s_politicalBordersToggled = TRUE;
		break;
	case GS_TRADEROUTES:
		func = &ProfileDB::SetShowTradeRoutes;
		break;
	case GS_CITYINFLUENCE:
		func = nullptr;
		profiledb_Get()->SetShowCityInfluence(!state);
		s_cityInfluenceToggled = TRUE;
		break;
	case GS_GRID:
		func = nullptr;
		g_isGridOn = !state;
		s_gridToggled = TRUE;
		break;
	case GS_CITYNAMES:
		func = &ProfileDB::SetShowCityNames;
		break;
	case GS_ARMYNAMES:
		func = &ProfileDB::SetShowArmyNames;
		break;
	case GS_CIVFLAGS:
		func = &ProfileDB::SetShowCivFlags;
		break;
	case GS_SMOOTH:
		func = &ProfileDB::SetShowSmooth;
		break;
	case GS_GOODANIMS:
		func = &ProfileDB::SetGoodAnim;
		//s_goodAnimToggled = TRUE;
		break;
	case GS_CITYPROD:
		func = &ProfileDB::SetShowCityProduction;
		break;
	case GS_TOTAL:  break;
	default:  Assert(0); break;
	};

	if(func)
		(profiledb_Get()->*func)(state ? FALSE : TRUE);
}

void graphicsscreen_selectResolution(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	if ( graphicsscreen_removeMyWindow(action) ) {
		graphicsresscreen_displayMyWindow();
	}
}

void graphicsscreen_brightSlide(aui_Control *control, uint32 action, uint32 data, void *cookie )
{

}
void graphicsscreen_gammaSlide(aui_Control *control, uint32 action, uint32 data, void *cookie )
{

}
void graphicsscreen_colorSlide(aui_Control *control, uint32 action, uint32 data, void *cookie )
{

}
void graphicsscreen_contrastSlide(aui_Control *control, uint32 action, uint32 data, void *cookie )
{

}

void graphicsscreen_unitSpeedSlide(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != AUI_RANGER_ACTION_VALUECHANGE ) return;

	profiledb_Get()->SetUnitSpeed(s_unitSpeed->GetValueX());
}




void graphicsscreen_getValues(sint32 &bright, sint32 &gamma, sint32 &color,sint32 &contrast)
{
	bright			= s_bright->GetValueX();
	gamma			= s_gamma->GetValueX();
	color			= s_color->GetValueX();
	contrast		= s_contrast->GetValueX();
}
