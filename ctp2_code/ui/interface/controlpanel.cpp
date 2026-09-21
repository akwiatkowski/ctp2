#include "ctp/c3.h"

#include <memory>

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_static.h"
#include "ui/aui_common/aui_stringtable.h"

#include "gfx/gfx_utils/pixelutils.h"

#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/c3_coloredswitch.h"

#include "ui/aui_ctp2/statuswindow.h"
#include "ui/interface/controlpanelwindow.h"

#include "ui/interface/workwin.h"
#include "ui/interface/debugwindow.h"

#include "ui/aui_ctp2/textbutton.h"
#include "ui/aui_ctp2/iconbutton.h"
#include "ui/aui_ctp2/picturebutton.h"
#include "ui/aui_ctp2/coloriconbutton.h"

#include "ui/aui_ctp2/c3thumb.h"
#include "ui/aui_ctp2/c3slider.h"
#include "ui/aui_ctp2/c3scroller.h"
#include "ui/aui_ctp2/c3spinner.h"

#include "ui/aui_ctp2/checkbox.h"
#include "ui/aui_ctp2/textswitch.h"
#include "ui/aui_ctp2/coloriconswitch.h"

#include "ui/aui_ctp2/texttab.h"
#include "ui/aui_common/aui_tabgroup.h"

#include "ui/aui_common/aui_static.h"

#include "ui/aui_common/aui_item.h"
#include "ui/aui_ctp2/c3listbox.h"
#include "ui/aui_ctp2/textbox.h"

#include "ui/aui_ctp2/c3dropdown.h"

#include "ui/aui_ctp2/c3textfield.h"

#include "ui/aui_ctp2/c3listbox.h"
#include "ui/aui_ctp2/cityinventorylistbox.h"

#include "ui/aui_common/aui_progressbar.h"

#include "gfx/tilesys/tiledmap.h"
#include "ui/aui_ctp2/c3windows.h"
#include "ui/interface/workwindow.h"
#include "gs/fileio/CivPaths.h"
#include "gfx/gfx_utils/videoutils.h"

#include "toolbar.h"

#include "ui/aui_ctp2/controlsheet.h"
#include "gs/gameobj/TerrImproveData.h"
#include "gs/gameobj/TerrImprovePool.h"
#include "gs/world/MapPoint.h"
#include "gfx/tilesys/tiledmap.h"
#include "gs/gameobj/player.h"
#include "ui/aui_ctp2/SelItem.h"
#include "InstDB.h"

#include "ui/aui_ctp2/bevellesswindow.h"
#include "ui/interface/radarwindow.h"

#include "ui/interface/UIUtils.h"

#include "ui/aui_ctp2/ctp2_button.h"

#include "ui/interface/citymanager.h"
#include "ui/interface/citywindow.h"
#include "ui/interface/screenutils.h"

#include "ui/aui_ctp2/c3_popupwindow.h"

#include "gs/utility/TurnCnt.h"


extern sint32		g_ScreenWidth;
extern sint32		g_ScreenHeight;
extern sint32		g_isCheatModeOn;


extern C3Window		*g_testWindow;
extern C3Window		*g_standardWindow;
extern C3Window		*g_floatingWindow;
extern DebugWindow	*g_debugWindow;

extern SelectedItem				*selitem_Get();


extern ProductionTabControl		*g_cp_productionTab;
extern CityTabControl			*g_cp_cityTab;
extern UnitsTabControl			*g_cp_unitsTab;






static TextButton	*s_button;
static TextButton	*s_debugButton;
static TextButton	*s_resourceButton;

static TextButton	*cheatButton;

static std::unique_ptr<ControlSheet>	s_tileMenuControl;
static std::unique_ptr<ControlSheet>	s_landTileControl;
static std::unique_ptr<ControlSheet>	s_seaTileControl;
static std::unique_ptr<ControlSheet>	s_spaceTileControl;
static std::unique_ptr<ControlSheet>	s_terraTileControl;

static std::unique_ptr<ColorIconButton>	s_donkeys;

static std::unique_ptr<ctp2_Button>	s_zoomPlusButton;
static std::unique_ptr<ctp2_Button>	s_zoomMinusButton;
static std::unique_ptr<ctp2_Button>	s_cityManagerButton;
static std::unique_ptr<ctp2_Button>	s_tileImprovementButton;
static std::unique_ptr<ctp2_Button>	s_turnButton;

static std::unique_ptr<c3_Static>	s_pwBox;
static std::unique_ptr<c3_Static>	s_goldBox;
static std::unique_ptr<c3_Static>	s_yearBoxHolder;
static std::unique_ptr<c3_ColoredSwitch>	s_yearBox;

static std::unique_ptr<c3_Static>	s_populationLabel;
static std::unique_ptr<c3_Static>	s_populationBox;

static std::unique_ptr<aui_ProgressBar>	s_progressBar;

static std::unique_ptr<aui_StringTable>	s_yearString;

extern sint32		g_tileImprovementMode;

#define k_STATUS_WINDOW_HEIGHT		30
#define k_CONTROL_PANEL_WIDTH		709
#define k_CONTROL_PANEL_HEIGHT		150

class c3_NormalProgressBar : public aui_ProgressBar {
public:

	c3_NormalProgressBar(AUI_ERRCODE *retval, uint32 id, MBCHAR *ldlBlock) :
	aui_ProgressBar( retval, id, ldlBlock ),
	aui_ImageBase( ldlBlock ),
	aui_TextBase( ldlBlock, (const MBCHAR *)NULL ) {}

	c3_NormalProgressBar(AUI_ERRCODE *retval, uint32 id, sint32 x, sint32 y, sint32 width, sint32 height) :
	aui_ProgressBar( retval, id, x, y, width, height ),
	aui_ImageBase( (sint32)0 ),
	aui_TextBase( NULL ) {}

protected:
	c3_NormalProgressBar() : aui_ProgressBar() {}

	virtual AUI_ERRCODE CalculateIntervals( double *start, double *stop ) {

		double x = (double)m_curValue / (double)m_maxValue;

		*start = 0.0;
		*stop = x;

		return AUI_ERRCODE_OK;
	}
};

extern sint32 g_modalWindow;

void CityManagerButtonCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if(action != (uint32)AUI_BUTTON_ACTION_EXECUTE) return;

	CityWindow::Initialize();

	Unit city;
	if(selitem_Get()->GetSelectedCity(city))
		CityWindow::Display(city.CD());
	else
		CityWindow::Display(NULL);

}

void TileImprovementButtonCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if(action != (uint32)AUI_BUTTON_ACTION_EXECUTE) return;

}

void ZoomPlusButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;
    if (tiledmap_Get()) {
		tiledmap_Get()->ZoomIn();
	}
}

void ZoomMinusButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;
	if (tiledmap_Get()) {
		tiledmap_Get()->ZoomOut();
	}
}

void DonkeyCallback(aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	MBCHAR filename[_MAX_PATH];
    civpaths_Get()->FindFile(C3DIR_SOUNDS, "donkey.wav", filename);
    PlaySound (filename, NULL, SND_ASYNC | SND_FILENAME);
}

void TurnButtonCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if(action != (uint32)AUI_BUTTON_ACTION_EXECUTE) return;

	if(turn_Get()) {
		turn_Get()->NextRound();
	}
}

sint32 controlpanelwindow_Initialize()
{
	AUI_ERRCODE		errcode;
	MBCHAR			windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	MBCHAR			controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	MBCHAR			buttonBlock[ k_AUI_LDL_MAXBLOCK + 1];

	if (controlpanel_Get()) return 0;

	sint32 windowWidth = k_CONTROL_PANEL_WIDTH;
	sint32 windowHeight = k_CONTROL_PANEL_HEIGHT;
	sint32 windowX = 264;
	sint32 windowY = 618;

	sint32 controlSheetX = 91;
	sint32 controlSheetY = 1;

	strlcpy(windowBlock, "ControlPanelWindow", sizeof(windowBlock));

	controlpanel_Set(std::make_unique<ControlPanelWindow>(&errcode, k_ID_WINDOW_CONTROLPANEL, windowBlock, 16 ));
	Assert( AUI_NEWOK(controlpanel_Get(), errcode) );
	if ( !AUI_NEWOK(controlpanel_Get(), errcode) ) return -1;





	controlpanel_Get()->Resize	(800, controlpanel_Get()->Height());
	controlpanel_Get()->Move	(controlpanel_Get()->X(), g_ScreenHeight - controlpanel_Get()->Height());








































































































































































	return 0;
}

sint32 controlpanelwindow_InitializeHats()
{
	AUI_ERRCODE		errcode;
	MBCHAR			windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	MBCHAR			controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	strlcpy(windowBlock, "ControlPanelRightHat", sizeof(windowBlock));

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "ZoomPlusButton" );
	s_zoomPlusButton = std::make_unique<ctp2_Button>( &errcode, aui_UniqueId(), controlBlock, ZoomPlusButtonActionCallback );
	Assert( AUI_NEWOK(s_zoomPlusButton, errcode) );
	if ( !AUI_NEWOK(s_zoomPlusButton, errcode) ) return -3;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "ZoomMinusButton" );
	s_zoomMinusButton = std::make_unique<ctp2_Button>( &errcode, aui_UniqueId(), controlBlock, ZoomMinusButtonActionCallback );
	Assert( AUI_NEWOK(s_zoomMinusButton, errcode) );
	if ( !AUI_NEWOK(s_zoomMinusButton, errcode) ) return -3;

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );
	if ( !AUI_SUCCESS(errcode) ) return -1;

	return 0;
}

sint32 controlpanelwindow_Cleanup( void )
{

	if (!controlpanel_Get()) return 0;




	s_donkeys.reset();

	ControlPanelWindow_TileImp_Cleanup();
	ControlPanelWindow_Land_Cleanup();
	ControlPanelWindow_Sea_Cleanup();
	ControlPanelWindow_Space_Cleanup();
	ControlPanelWindow_Terra_Cleanup();

	cp_tileimp_trackerCleanup();

	specialattackwindow_Cleanup();

	s_tileMenuControl.reset();
	s_landTileControl.reset();
	s_seaTileControl.reset();
	s_spaceTileControl.reset();
	s_terraTileControl.reset();

	controlpanelwindow_CleanupCitySelectedTabGroup();

	s_yearBox.reset();

	s_progressBar.reset();

	s_goldBox.reset();

	s_pwBox.reset();

	s_zoomMinusButton.reset();

	s_zoomPlusButton.reset();

	s_tileImprovementButton.reset();

	s_cityManagerButton.reset();

	s_turnButton.reset();

	s_populationLabel.reset();
	s_populationBox.reset();
	s_yearString.reset();
	s_yearBoxHolder.reset();

	CityWindow::Cleanup();

	controlpanel_Set(nullptr);

	return 0;
}





void
HideElement(aui_Region *element)
{
	if (element!=NULL)
		element->Hide();
}




void
HideControlPanel()
{
	HideElement(g_testWindow            );
	HideElement(g_standardWindow        );
	HideElement(g_floatingWindow        );
	HideElement(g_debugWindow           );
	HideElement(workwindow_Get()            );

	HideElement(radarwindow_Get()       );


	HideElement((aui_Window*)controlpanel_Get()->GetWindow());
	HideElement(s_button                );
	HideElement(s_debugButton           );
	HideElement(s_resourceButton        );
	HideElement(cheatButton             );

	HideElement(s_tileMenuControl.get()   );
	HideElement(s_landTileControl.get()   );
	HideElement(s_seaTileControl.get()    );
	HideElement(s_spaceTileControl.get()  );
	HideElement(s_terraTileControl.get()  );
	HideElement(s_donkeys.get()           );
	HideElement(s_zoomPlusButton.get()    );
	HideElement(s_zoomMinusButton.get()   );

	HideElement(s_cityManagerButton.get() );
	HideElement(s_tileImprovementButton.get());
	HideElement(s_turnButton.get()        );
	HideElement(s_pwBox.get()             );
	HideElement(s_goldBox.get()           );
	HideElement(s_yearBoxHolder.get()     );
	HideElement(s_yearBox.get()           );
	HideElement(s_populationLabel.get()   );
	HideElement(s_populationBox.get()     );
}
