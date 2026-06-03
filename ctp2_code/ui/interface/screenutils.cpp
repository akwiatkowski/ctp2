//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Screen utilities
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
// - Generate debug version when set.
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Start the great library with the current research project of the player.
// - Prevent production errors when pressing F3 after end of turn.
// - Added close_AllScreensAndUpdateInfoScreen so that on a new turn the
//   information window can stay open. (Aug. 7th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/interface/screenutils.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_ctp2/c3ui.h"

#include "ui/aui_common/aui_button.h"

#include "ui/aui_ctp2/c3window.h"
#include "ui/interface/workwindow.h"
#include "ui/interface/workwin.h"
#include "ui/interface/sciencewin.h"
#include "ui/interface/infowindow.h"
#include "ui/interface/victorywindow.h"
#include "ui/interface/victorywin.h"
#include "ui/interface/greatlibrary.h"
#include "ui/interface/optionswindow.h"
#include "ui/interface/citywindow.h"

#include "ui/interface/km_screen.h"
#include "ui/interface/EndgameWindow.h"
#include "ui/interface/creditsscreen.h"

#include "ui/interface/tutorialwin.h"

#include "ui/interface/debugwindow.h"

#include "ctp/debugtools/debugmemory.h"

#include "gs/gameobj/Player.h"
#include "ui/aui_ctp2/SelItem.h"
#include "gs/database/profileDB.h"
#include "net/general/network.h"

#include "gfx/spritesys/director.h"

#include "ui/interface/diplomacywindow.h"
#include "ui/interface/DomesticManagementDialog.h"
#include "ui/interface/NationalManagementDialog.h"

#include "ui/interface/trademanager.h"
#include "ui/interface/ScienceManagementDialog.h"
#include "ui/interface/unitmanager.h"
#include "ui/interface/sciencevictorydialog.h"

#include "ui/interface/battleviewwindow.h"
#include "ui/interface/scenarioeditor.h"
#include "ui/interface/EditQueue.h"

#include "ui/interface/dipwizard.h"

#include "ui/interface/sciencevictorydialog.h"


extern ScienceWin           *g_scienceWin;

extern DebugWindow          *g_debugWindow;

extern SelectedItem         *selitem_Get();

extern Network              g_network;
extern sint32               g_modalWindow;

double	g_screenTime = 0.0;

sint32 open_WorkView( )
{
	sint32      err     = workwin_Initialize();
	Assert( !err );
	if ( err ) return -1;

	AUI_ERRCODE auiErr  = c3ui_Get()->AddWindow(workwindow_Get());
	Assert( auiErr == AUI_ERRCODE_OK );
	if ( auiErr != AUI_ERRCODE_OK ) return -1;

	return 0;
}

void close_WorkView()
{
	if (workwindow_Get())
    {
		c3ui_Get()->RemoveWindow(workwindow_Get()->Id());
	}
}

sint32 open_CityView( )
{
	AUI_ERRCODE auiErr  = CityWindow::Display(NULL);

	Assert( auiErr == AUI_ERRCODE_OK );
	if ( auiErr != AUI_ERRCODE_OK ) return -1;

	return 0;
}

void close_CityView()
{
	CityWindow::Close(NULL, AUI_BUTTON_ACTION_EXECUTE, 0, NULL);
}

sint32 open_CityStatus( )
{
#ifdef _DEBUG
	SET_TIME
#endif // _DEBUG

		NationalManagementDialog::Open();

#ifdef _DEBUG
	GET_ELAPSED_TIME( "City Status" );
#endif // _DEBUG

	return 0;
}

void close_CityStatus()
{
	NationalManagementDialog::Close();
}

sint32 open_CivStatus()
{
#ifdef _DEBUG
	SET_TIME
#endif // _DEBUG

	DomesticManagementDialog::Open();

#ifdef _DEBUG
	GET_ELAPSED_TIME( "Civ Status" );
#endif // _DEBUG

	return 0;
}

void close_CivStatus()
{
	DomesticManagementDialog::Close();
}

sint32 open_ScienceStatus( )
{
	ScienceManagementDialog::Open();
	return 0;
}

void close_ScienceStatus()
{
	ScienceManagementDialog::Close();
}

sint32 open_ScienceVictory( )
{
	ScienceVictoryDialog::Open();
	return 0;
}

void close_ScienceVictory( )
{
	ScienceVictoryDialog::Close();
}

sint32 open_UnitStatus( )
{
	UnitManager::Display();
	return 0;
}

void close_UnitStatus( )
{
	UnitManager::Hide();
}

sint32 open_TradeStatus( )
{
#ifdef _DEBUG
	SET_TIME
#endif // _DEBUG




	TradeManager::Display();

#ifdef _DEBUG
	GET_ELAPSED_TIME( "Trade" );
#endif // _DEBUG

	return 0;
}

void close_TradeStatus( )
{
	TradeManager::Hide();
}

sint32 open_VictoryWindow( )
{
#ifdef _DEBUG
	SET_TIME
#endif // _DEBUG

	victorywin_Initialize(0);
	victorywin_DisplayWindow(0);

#ifdef _DEBUG
	GET_ELAPSED_TIME( "Victory" );
#endif // _DEBUG

	return 0;
}

void close_VictoryWindow( )
{
	victorywin_RemoveWindow();
}

sint32 open_Diplomacy( )
{
#ifdef _DEBUG
	SET_TIME
#endif // _DEBUG

	DiplomacyWindow::Display();

#ifdef _DEBUG
	GET_ELAPSED_TIME( "Diplomacy" );
#endif // _DEBUG

	return 0;
}

void close_Diplomacy()
{
	DiplomacyWindow::Hide();
}

sint32 open_InfoScreen( )
{
#ifdef _DEBUG
	SET_TIME
#endif // _DEBUG

	InfoWindow::Open();

#ifdef _DEBUG
	GET_ELAPSED_TIME( "Info" );
#endif // _DEBUG

	return 0;
}

void close_InfoScreen( )
{
	InfoWindow::Close();
}

//----------------------------------------------------------------------------
//
// Name       : open_GreatLibrary
//
// Description: (Re)open the great library window.
//
// Parameters : index               : advance to display on opening
//              sci                 : force display of index, even when the
//                                    library was open
//
// Returns    : sint32              : opening succeeded
//
// Remark(s)  : When the library is not open already, or the sci parameter is
//              set, it will open with the indicated advance.
//
//----------------------------------------------------------------------------
bool open_GreatLibrary(sint32 index, bool sci)
{
#ifdef _DEBUG
	SET_TIME
#endif // _DEBUG

	sint32 const	err	= greatlibrary_Initialize(index, sci);
	Assert(!err);
	if (err) return false;

	greatlibrary_Get()->Display();

#ifdef _DEBUG
	GET_ELAPSED_TIME("Great Library");
#endif // _DEBUG

	return true;
}

//----------------------------------------------------------------------------
//
// Name       : open_GreatLibrary
//
// Description: (Re)open the great library window.
//
// Parameters : -
//
// Globals    : selitem_Get()     : object selected at screen
//              player_arr_Get()    : list of players
//
// Returns    : sint32              : opening succeeded
//
// Remark(s)  : When the library is not open already, it will open with the
//              advance that is being researched by the current player.
//
//----------------------------------------------------------------------------
bool open_GreatLibrary( )
{
	sint32 const		player	= selitem_Get()->GetVisiblePlayer();
	AdvanceType const	advance	= player_Get(player)->m_advances->GetResearching();

	return open_GreatLibrary(advance);
}

void close_GreatLibrary()
{
	if (GreatLibrary *gl = greatlibrary_Get())
    {
		gl->Remove();
	}
}

sint32 open_OptionsScreen( sint32 fromWhichScreen )
{
#ifdef _DEBUG
	SET_TIME
#endif // _DEBUG

	sint32 err = optionsscreen_displayMyWindow( fromWhichScreen );

#ifdef _DEBUG
	GET_ELAPSED_TIME( "Options Screen" );
#endif // _DEBUG

	return err;
}

void close_OptionsScreen( )
{
	optionsscreen_removeMyWindow(AUI_BUTTON_ACTION_EXECUTE);
}

sint32 open_KeyMappingScreen( )
{
	return km_screen_displayMyWindow();
}

sint32 open_ScenarioEditor()
{
	return ScenarioEditor::Display();
}

void close_ScenarioEditor()
{
	ScenarioEditor::Hide();
}

void close_KeyMappingScreen( )
{
	km_screen_removeMyWindow(AUI_BUTTON_ACTION_EXECUTE);
}






























/// Open alien life window (removed CTP1 functionality)
sint32 open_EndGame()
{
	return 0;
}

/// Close alien life window (removed CTP1 functionality)
void close_EndGame()
{
}

sint32 open_TutorialWin( )
{
#ifdef _DEBUG
	SET_TIME
#endif // _DEBUG

	sint32 err;

	err = tutorialwin_Initialize();
	Assert( !err );
	if ( err ) return -1;

	tutorialwin_Get()->Display();

#ifdef _DEBUG
	GET_ELAPSED_TIME( "Tutorial Win" );
#endif // _DEBUG

	return 0;
}

void close_TutorialWin()
{
	if (tutorialwin_Get())
    {
    	tutorialwin_Get()->Remove();
    }
}


sint32 open_CreditsScreen()
{
	sint32 err = creditsscreen_Initialize();
	Assert(!err);
	if(err) return(-1);

	AUI_ERRCODE auiErr = c3ui_Get()->AddWindow(creditsscreen_GetWindow());
	Assert(auiErr == AUI_ERRCODE_OK);
    return (auiErr == AUI_ERRCODE_OK) ? 0 : -1;
}

void close_CreditsScreen()
{
	creditsscreen_Cleanup();
}

void battleview_ExitButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie );

//----------------------------------------------------------------------------
//
// Name       : close_AllScreens
//
// Description: Closes all open windows.
//
// Parameters : -
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
void close_AllScreens()
{
	close_CreditsScreen();
	close_WorkView();
	close_CityView();
	close_CityStatus();
	close_CivStatus();
	close_ScienceStatus();
	close_UnitStatus();
	close_TradeStatus();
	close_Diplomacy();
	close_InfoScreen();
	close_GreatLibrary();
	close_OptionsScreen();
	close_KeyMappingScreen();
	close_TutorialWin();
	close_ScenarioEditor();
	EditQueue::Hide();
	DipWizard::Hide();
	ScienceVictoryDialog::Close();

	if (battleviewwindow_Get())
    {
		g_modalWindow = 1;

		battleview_ExitButtonActionCallback
            (NULL, AUI_BUTTON_ACTION_EXECUTE, 0, NULL);
	}
    else
    {
	    g_modalWindow = 0;
    }
}

//----------------------------------------------------------------------------
//
// Name       : close_AllScreensAndUpdateInfoScreen
//
// Description: Closes all open windows except the information window.
//
// Parameters : -
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
void close_AllScreensAndUpdateInfoScreen()
{
	close_CreditsScreen();
	close_WorkView();
	close_CityView();
	close_CityStatus();
	close_CivStatus();
	close_ScienceStatus();
	close_UnitStatus();
	close_TradeStatus();
	close_Diplomacy();
	InfoWindow::Update();
	close_GreatLibrary();
	close_OptionsScreen();
	close_KeyMappingScreen();
	close_TutorialWin();
	close_ScenarioEditor();
	EditQueue::Hide();
	DipWizard::Hide();
	ScienceVictoryDialog::Close();

	if (battleviewwindow_Get())
    {
		g_modalWindow = 1;
		battleview_ExitButtonActionCallback
            (NULL, AUI_BUTTON_ACTION_EXECUTE, 0, NULL);
	}
    else
    {
        g_modalWindow = 0;
    }
}
