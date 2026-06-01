//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Radar window (mini map)
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
// - Separate creation and display of the radar window for hotseat play.
// - Added Political map button (EPW July-6th 09
//- Added View capitol button (5-Jan-10 EPW)
//-Added view relations button (7-Jan-10 EPW)
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "ui/interface/radarwindow.h"

#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/ctp2_button.h"
#include "ui/aui_ctp2/ctp2_Static.h"
#include "ui/aui_ctp2/ctp2_Window.h"
#include "ui/aui_ctp2/ctp2_Switch.h"
#include "ui/aui_ctp2/radarmap.h"
#include "gs/utility/gameinit.h"		// g_startHotseatGame

static ctp2_Window *g_radarWindow = NULL;

ctp2_Window * radarwindow_Get(void)
{
	return g_radarWindow;
}


extern sint32		g_ScreenHeight;
extern sint32       g_ScreenWidth;
extern C3UI			*g_c3ui;
void UnitsToggleButtonActionCallback
(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void *cookie
)
{
	switch (action)
	{
	case AUI_SWITCH_ACTION_ON:
		radar_map_Get()->DisplayUnits( true );
		break;
	case AUI_SWITCH_ACTION_OFF:
		radar_map_Get()->DisplayUnits( false );
		break;
	}
}

void CitiesToggleButtonActionCallback
(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void *cookie
)
{
	switch (action)
	{
	case AUI_SWITCH_ACTION_ON:
		radar_map_Get()->DisplayCities( true );
		break;
	case AUI_SWITCH_ACTION_OFF:
		radar_map_Get()->DisplayCities( false );
		break;
	}
}

void BordersToggleButtonActionCallback
(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void *cookie
)
{
	switch (action)
	{
	case AUI_SWITCH_ACTION_ON:
		radar_map_Get()->DisplayBorders( true );
		break;
	case AUI_SWITCH_ACTION_OFF:
		radar_map_Get()->DisplayBorders( false );
		break;
	}
}

void FilterToggleButtonActionCallback(aui_Control *control,
	uint32 action, uint32 data, void *cookie)
{
	switch (action)
	{
	case AUI_SWITCH_ACTION_ON:
		radar_map_Get()->Filter( true );
		break;
	case AUI_SWITCH_ACTION_OFF:
		radar_map_Get()->Filter( false );
		break;
	}
}

void TradeToggleButtonActionCallback(aui_Control *control,
	uint32 action, uint32 data, void *cookie)
{
	switch (action)
	{
	case AUI_SWITCH_ACTION_ON:
		radar_map_Get()->DisplayTrade( true );
		break;
	case AUI_SWITCH_ACTION_OFF:
		radar_map_Get()->DisplayTrade( false );
		break;
	}
}

void TerrainToggleButtonActionCallback(aui_Control *control,
	uint32 action, uint32 data, void *cookie)
{
	switch (action)
	{
	case AUI_SWITCH_ACTION_ON:
		radar_map_Get()->DisplayTerrain( true );
		break;
	case AUI_SWITCH_ACTION_OFF:
		radar_map_Get()->DisplayTerrain( false );
		break;
	}
}

void MinimizeCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	ctp2_Static *topBorder		= static_cast<ctp2_Static*>(aui_Ldl::GetObject(
		"RadarWindow.TopBorder"));

	switch(action) {
	case AUI_SWITCH_ACTION_ON:
		g_radarWindow->Move(0, g_ScreenHeight - topBorder->Height());
		break;
	case AUI_SWITCH_ACTION_OFF:
		g_radarWindow->Move(0, g_ScreenHeight - g_radarWindow->Height());
		break;
	}
}

void PoliticalToggleButtonActionCallback(aui_Control *control,
	uint32 action, uint32 data, void *cookie)
{
	switch (action)
	{
	case AUI_SWITCH_ACTION_ON:
		radar_map_Get()->DisplayPolitical( true );
		break;
	case AUI_SWITCH_ACTION_OFF:
		radar_map_Get()->DisplayPolitical( false );
		break;
	}
}

void CapitolsToggleButtonActionCallback(aui_Control *control,
	uint32 action, uint32 data, void *cookie)
{
	switch (action)
	{
	case AUI_SWITCH_ACTION_ON:
		radar_map_Get()->DisplayCapitols( true );
		break;
	case AUI_SWITCH_ACTION_OFF:
		radar_map_Get()->DisplayCapitols( false );
		break;
	}
}

void RelationsToggleButtonActionCallback(aui_Control *control,
	uint32 action, uint32 data, void *cookie)
{
	switch (action)
	{
	case AUI_SWITCH_ACTION_ON:
		radar_map_Get()->DisplayRelations( true );
		break;
	case AUI_SWITCH_ACTION_OFF:
		radar_map_Get()->DisplayRelations( false );
		break;
	}
}

sint32 radarwindow_Initialize()
{

	if(g_radarWindow)
		return(0);

	sint32 radarSize = g_ScreenHeight / 3;

	g_radarWindow = static_cast<ctp2_Window*>(
		 aui_Ldl::BuildHierarchyFromRoot("RadarWindow"));

	g_radarWindow->SetType(AUI_WINDOW_TYPE_SINKING);

	radar_map_Set(static_cast<RadarMap*>(aui_Ldl::GetObject(
		"RadarWindow.RadarMap")));

	ctp2_Static *topBorder		= static_cast<ctp2_Static*>(aui_Ldl::GetObject(
		"RadarWindow.TopBorder"));
	ctp2_Static *leftBorder		= static_cast<ctp2_Static*>(aui_Ldl::GetObject(
		"RadarWindow.LeftBorder"));
	ctp2_Static *rightBorder	= static_cast<ctp2_Static*>(aui_Ldl::GetObject(
		"RadarWindow.RightBorder"));
	ctp2_Static *bottomBorder	= static_cast<ctp2_Static*>(aui_Ldl::GetObject(
		"RadarWindow.BottomBorder"));

	sint32 topSize		= topBorder->Height();
	sint32 leftSize		= leftBorder->Width();
	sint32 rightSize	= rightBorder->Width();
	sint32 bottomSize	= bottomBorder->Height();

	if(g_ScreenWidth <= 800) {
		radarSize = 198 - (leftSize + rightSize);
	}

	g_radarWindow->Resize(leftSize + radarSize + rightSize,
		topSize + radarSize + bottomSize);


	g_radarWindow->Move(0, g_ScreenHeight - g_radarWindow->Height());

	topBorder->Resize(leftSize + radarSize + rightSize, topSize);
	leftBorder->Resize(leftSize, radarSize);
	rightBorder->Resize(rightSize, radarSize);
	bottomBorder->Resize(leftSize + radarSize + rightSize, bottomSize);

	leftBorder->Move(0, topSize);
	rightBorder->Move(leftSize + radarSize, topSize);
	bottomBorder->Move(0, topSize + radarSize);

	radar_map_Get()->Resize(radarSize, radarSize);
	radar_map_Get()->Move(leftSize, topSize);




#define k_RADAR_BUTTONS_PER_ROW 7

	sint32 childWidth = radarSize / k_RADAR_BUTTONS_PER_ROW;

	sint32 col = 0;
	sint32 row = 0;


	for(sint32 childIndex = 0; childIndex < topBorder->NumChildren(); childIndex++) {

		aui_Region *child = topBorder->GetChildByIndex(childIndex);

		child->Resize(childWidth, child->Height());

		child->Move(leftSize + (childWidth * col), child->Y() + row * child->Height());
		col++;
		if(col >= k_RADAR_BUTTONS_PER_ROW) {
			col = 0;
			row++;
		}
	}

	ctp2_Switch *unitsButton	= static_cast<ctp2_Switch*>(aui_Ldl::GetObject(
		"RadarWindow.TopBorder.Button1"));
	ctp2_Switch *citiesButton	= static_cast<ctp2_Switch*>(aui_Ldl::GetObject(
		"RadarWindow.TopBorder.Button2"));
	ctp2_Switch *bordersButton	= static_cast<ctp2_Switch*>(aui_Ldl::GetObject(
		"RadarWindow.TopBorder.Button3"));
	ctp2_Switch *filterButton	= static_cast<ctp2_Switch*>(aui_Ldl::GetObject(
		"RadarWindow.TopBorder.Button4"));
	ctp2_Switch *tradeButton = static_cast<ctp2_Switch *>(aui_Ldl::GetObject(
		"RadarWindow.TopBorder.Button5"));
	ctp2_Switch *terrainButton = static_cast<ctp2_Switch *>(aui_Ldl::GetObject(
	 	"RadarWindow.TopBorder.Button6"));
	ctp2_Switch *minimizeButton = static_cast<ctp2_Switch *>(aui_Ldl::GetObject(
		"RadarWindow.TopBorder.Button7"));
	ctp2_Switch *politicalButton = static_cast<ctp2_Switch *>(aui_Ldl::GetObject(
		"RadarWindow.TopBorder.Button8"));
	ctp2_Switch *capitolsButton = static_cast<ctp2_Switch *>(aui_Ldl::GetObject(
		"RadarWindow.TopBorder.Button9"));
	ctp2_Switch *relationsButton = static_cast<ctp2_Switch *>(aui_Ldl::GetObject(
		"RadarWindow.TopBorder.Button10"));

	minimizeButton->Move(topBorder->Width() - minimizeButton->Width() - 15, minimizeButton->Y());

	Assert(unitsButton);
	Assert(citiesButton);
	Assert(bordersButton);

	unitsButton->SetState( radar_map_Get()->IsDisplayUnits() );
	citiesButton->SetState( radar_map_Get()->IsDisplayCities() );
	bordersButton->SetState( radar_map_Get()->IsDisplayBorders() );
	filterButton->SetState( radar_map_Get()->IsFilter() );
	tradeButton->SetState( radar_map_Get()->IsDisplayTrade());
	terrainButton->SetState( radar_map_Get()->IsDisplayTerrain());
	politicalButton->SetState( radar_map_Get()->IsDisplayPolitical());
	capitolsButton->SetState( radar_map_Get()->IsDisplayCapitols());
	relationsButton->SetState( radar_map_Get()->IsDisplayRelations());

	unitsButton->SetActionFuncAndCookie(UnitsToggleButtonActionCallback, NULL);
	citiesButton->SetActionFuncAndCookie(CitiesToggleButtonActionCallback, NULL);
	bordersButton->SetActionFuncAndCookie(BordersToggleButtonActionCallback, NULL);
	filterButton->SetActionFuncAndCookie(FilterToggleButtonActionCallback, NULL);
	tradeButton->SetActionFuncAndCookie(TradeToggleButtonActionCallback, NULL);
	terrainButton->SetActionFuncAndCookie(TerrainToggleButtonActionCallback, NULL);
	minimizeButton->SetActionFuncAndCookie(MinimizeCallback, NULL);
	politicalButton->SetActionFuncAndCookie(PoliticalToggleButtonActionCallback, NULL);
	capitolsButton->SetActionFuncAndCookie(CapitolsToggleButtonActionCallback, NULL);
	relationsButton->SetActionFuncAndCookie(RelationsToggleButtonActionCallback, NULL);

	g_c3ui->AddWindow(g_radarWindow);

	if (gameinit_IsHotseatGame())
	{
		// Do not display map information before the user has pressed Ready.
		g_radarWindow->Hide();
	}

	return(0);
}

void radarwindow_Display(void)
{

	Assert(g_radarWindow != NULL);
	if(!g_radarWindow)
		return;

	g_radarWindow->Show();
}

sint32 radarwindow_Cleanup( void )
{

	if(g_radarWindow) {
		g_c3ui->RemoveWindow(g_radarWindow->Id());
		aui_Ldl::DeleteHierarchyFromRoot("RadarWindow");
	}

	g_radarWindow = NULL;
	radar_map_Set(NULL);

	return(0);
}

void    radarwindow_Hide()
{
	if(!g_radarWindow->IsHidden()) {
		g_radarWindow->Hide();
	}
}

void    radarwindow_Show()
{
	if(g_radarWindow->IsHidden()) {
		g_radarWindow->Show();
	}
}

void radarwindow_Toggle()
{
	if(g_radarWindow->IsHidden()) {
		g_radarWindow->Show();
	} else {
		g_radarWindow->Hide();
	}
}
