//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Collection of all setup windows.
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
// LOCKSETTINGSONLAUNCH
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Memory leak repaired.
// - Replaced old civilisation database by new one. (Aug 21st 2005 Martin G�hmann)
// - The ages in the summary are now displayed correctly.
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
// - Standardized code. (May 29th 2006 Martin G�hmann)
// - Replaced old civ selection button bank by list box. (2-Jan-2008 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/netshell/allinonewindow.h"

#include <algorithm>
#include <memory>
#include <chrono>
#include <thread>

#include "AgeRecord.h"
#include "ui/interface/agesscreen.h"
#include "ui/aui_common/aui_blitter.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_radio.h"
#include "ui/aui_common/aui_screen.h"
#include "ui/aui_common/aui_static.h"
#include "ui/aui_common/aui_stringtable.h"
#include "ui/aui_common/aui_tabgroup.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_ctp2/c3_button.h"
#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/c3textfield.h"
#include "gs/gameobj/CivilisationPool.h"
#include "CivilisationRecord.h"
#include "gs/fileio/CivPaths.h"                      // civpaths_Get()
#include "gs/fileio/civscenarios.h"
#include "ui/aui_ctp2/ctp2_dropdown.h"
#include "ui/aui_ctp2/ctp2_spinner.h"
#include "ui/aui_ctp2/ctp2_Static.h"
#include "ui/aui_ctp2/ctp2_Switch.h"
#include "ui/interface/custommapscreen.h"
#include "ui/netshell/dialogboxwindow.h"
#include "gs/gameobj/Exclusions.h"
#include "gs/fileio/gamefile.h"
#include "ui/netshell/gameselectwindow.h"
#include "ui/interface/loadsavewindow.h"
#include "ui/netshell/lobbywindow.h"
#include "ui/netshell/netshell.h"
#include "net/general/network.h"
#include "ui/netshell/ns_chatbox.h"
#include "ui/netshell/ns_civlistbox.h"
#include "ui/netshell/ns_customlistbox.h"
#include "ui/netshell/ns_item.h"
#include "ui/netshell/ns_string.h"
#include "ui/netshell/ns_tribes.h"
#include "ui/netshell/passwordscreen.h"
#include "gfx/gfx_utils/pixelutils.h"
#include "ui/netshell/playereditwindow.h"
#include "ui/netshell/playerselectwindow.h"
#include "ui/aui_utils/primitives.h"
#include "gs/database/profileDB.h"                     // profiledb_Get()
#include "ui/interface/spnewgamediffscreen.h"
#include "ui/interface/spnewgamemapsizescreen.h"
#include "ui/interface/spnewgamemapshapescreen.h"
#include "ui/interface/spnewgametribescreen.h"
#include "ui/interface/spnewgamewindow.h"
#include "gs/database/StrDB.h"                         // stringdb_Get()
#include "ui/aui_ctp2/textradio.h"
#include "ui/aui_ctp2/textswitch.h"
#include "ui/aui_ctp2/texttab.h"


extern aui_Radio *s_maleRadio;

static DialogBoxWindow *s_dbw = nullptr;
static AllinoneWindow * g_allinoneWindow = nullptr;

AllinoneWindow * allinonewindow_Get()
{
    return g_allinoneWindow;
}
static DialogBoxWindow *g_rulesWindow = nullptr;
static DialogBoxWindow *g_exclusionsWindow = nullptr;

#ifdef _DEBUG
#define DEBUG_PushChatMessage(arg) (netfunc_Get()->PushChatMessage("DEBUG: " arg))
#else
#define DEBUG_PushChatMessage(arg) ((void)0)
#endif




#define k_GUIDDATA_ID 2
#define k_SCENARIO_INFO_ID 3

#define LOCKSETTINGSONLAUNCH 1

void AllinoneWindow_SetupGameForLaunch( );

namespace
{

bool IsValidTribeIndex(sint32 a_Index)
{
    Assert((a_Index >= 0) && (a_Index < k_TRIBES_MAX));
    return (a_Index >= 0) && (a_Index < k_TRIBES_MAX);
}

} // namespace

AllinoneWindow::AllinoneWindow(
	AUI_ERRCODE *retval )
	:
	ns_Window(
		retval,
		aui_UniqueId(),
		"allinonewindow",
		0,
		AUI_WINDOW_TYPE_STANDARD )
{
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommon();
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = CreateControls();
	Assert( AUI_SUCCESS(*retval) );
}


AUI_ERRCODE AllinoneWindow::InitCommon( )
{
    if (!g_allinoneWindow)
    {
	    g_allinoneWindow = this;
    }
	m_numControls = CONTROL_MAX;
	m_controls = std::make_unique<aui_Control *[]>( CONTROL_MAX );

	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	g_rulesWindow = std::make_unique<DialogBoxWindow>(
		&errcode,
		"ruleswindow",
		nullptr ).release();

	g_exclusionsWindow = std::make_unique<DialogBoxWindow>(
		&errcode,
		"exclusionswindow",
		nullptr ).release();

	memset( m_lname, 0, sizeof( m_lname ) );





	m_isScenarioGame = FALSE;

	m_createdExclusions = false;
	m_shouldUpdateGame = false;
	m_shouldUpdatePlayer = false;
	m_shouldUpdateAIPlayer = false;
	m_shouldAddAIPlayer = 0;
	m_joinedGame = false;

	m_tickGame = m_tickPlayer = m_tickAIPlayer = GetTickCount();

	m_aiplayerList = std::make_unique<tech_WLList<nf_AIPlayer *>>();

	m_messageRequestDenied = std::make_unique<ns_String>("strings.system.requestdenied");
	m_messageKicked = std::make_unique<ns_String>("strings.system.kicked");
	m_messageGameSetup = std::make_unique<ns_String>("strings.system.gamesetup");
	m_messageGameEnter = std::make_unique<ns_String>("strings.system.gameenter");
	m_messageGameHost = std::make_unique<ns_String>("strings.system.gamehost");
	m_messageGameCreate = std::make_unique<ns_String>("strings.system.gamecreate");
	m_messageLaunched = std::make_unique<ns_String>("strings.system.launched");

	m_receivedGuids = false;

	m_playStyleValueStrings = std::make_unique<aui_StringTable>(
		&errcode,
		"strings.playstylevalues" );
	Assert( AUI_SUCCESS(errcode) );
	if ( !AUI_SUCCESS(errcode) ) return AUI_ERRCODE_HACK;

	m_PPTStrings = std::make_unique<aui_StringTable>(
		&errcode,
		"strings.ppt" );
	Assert( AUI_SUCCESS(errcode) );
	if ( !AUI_SUCCESS(errcode) ) return AUI_ERRCODE_HACK;

	m_dbActionArray[ 0 ] = std::make_unique<DialogBoxPopDownAction>();

	agesscreen_Initialize( AllinoneAgesCallback );
	spnewgamemapsizescreen_Initialize( AllinoneMapSizeCallback );
	spnewgamemapshapescreen_Initialize( AllinoneWorldShapeCallback );
	spnewgametribescreen_Initialize( AllinoneTribeCallback );

	custommapscreen_Initialize( AllinoneWorldTypeCallback );
	spnewgamediffscreen_Initialize( AllinoneDifficultyCallback );

	m_numAvailUnits = 0;
	memset( m_units, 0, sizeof( m_units ) );
	m_numAvailImprovements = 0;
	memset( m_improvements, 0, sizeof( m_improvements ) );
	m_numAvailWonders = 0;
	memset( m_wonders, 0, sizeof( m_wonders ) );

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE AllinoneWindow::CreateControls( )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;




	aui_Control *control;









	control = std::make_unique<ns_ChatBox>(
		&errcode,
		aui_UniqueId(),
		"allinonewindow.chatbox" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_CHATBOX ] = control;

	control = std::make_unique<aui_Static>(
		&errcode,
		aui_UniqueId(),
		"allinonewindow.playerssheet" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_PLAYERSSHEET ] = control;

	control = std::make_unique<aui_Static>(
		&errcode,
		aui_UniqueId(),
		"allinonewindow.playerssheet.titlestatictext" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_PLAYERSTITLESTATICTEXT ] = control;

	control = std::make_unique<aui_Static>(
		&errcode,
		aui_UniqueId(),
		"allinonewindow.playerssheet.gamenamestatictext" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_GAMENAMESTATICTEXT ] = control;

	control = std::make_unique<C3TextField>(
		&errcode,
		aui_UniqueId(),
		"allinonewindow.playerssheet.gamenametextfield" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_GAMENAMETEXTFIELD ] = control;

	control = std::make_unique<aui_Static>(
		&errcode,
		aui_UniqueId(),
		"allinonewindow.playerssheet.playstylestatictext" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_PLAYSTYLESTATICTEXT ] = control;

	control = std::make_unique<ctp2_DropDown>(
		&errcode,
		aui_UniqueId(),
		"allinonewindow.playerssheet.playstyledropdown" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_PLAYSTYLEDROPDOWN ] = control;

	control = std::make_unique<aui_Static>(
		&errcode,
		aui_UniqueId(),
		"allinonewindow.playerssheet.playstylevaluestatictext" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_PLAYSTYLEVALUESTATICTEXT ] = control;

	control = std::make_unique<ctp2_Spinner>(
		&errcode,
		aui_UniqueId(),
		"allinonewindow.playerssheet.playstylevaluespinner" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_PLAYSTYLEVALUESPINNER ] = control;

	control = spNew_ctp2_Button(
		&errcode,
		"allinonewindow",
		"reviewbutton",
		nullptr);
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_REVIEWBUTTON ] = control;

	control = std::make_unique<ctp2_Switch>(
		&errcode,
		aui_UniqueId(),
		"allinonewindow.lockswitch" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_LOCKSWITCH ] = control;

	control = spNew_ctp2_Button(
		&errcode,
		"allinonewindow",
		"playerssheet.addaibutton",
		nullptr);
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_ADDAIBUTTON ] = control;

	control = spNew_ctp2_Button(
		&errcode,
		"allinonewindow",
		"playerssheet.rulesbutton",
		nullptr);
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_RULESBUTTON ] = control;

	control = spNew_ctp2_Button(
		&errcode,
		"allinonewindow",
		"playerssheet.exclusionsbutton",
		nullptr);
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_EXCLUSIONSBUTTON ] = control;

	control = std::make_unique<ns_HPlayerListBox>(
		&errcode,
		aui_UniqueId(),
		"allinonewindow.playerssheet.hplayerslistbox" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_HPLAYERSLISTBOX ] = control;

	control = std::make_unique<ns_GPlayerListBox>(
		&errcode,
		aui_UniqueId(),
		"hack.aiplayerslistbox",
		(ns_HPlayerListBox *)m_controls[ CONTROL_HPLAYERSLISTBOX ] ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_GPLAYERSLISTBOX ] = control;

	control = std::make_unique<ns_AIPlayerListBox>(
		&errcode,
		aui_UniqueId(),
		"hack.aiplayerslistbox",
		(ns_HPlayerListBox *)m_controls[ CONTROL_HPLAYERSLISTBOX ] ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_AIPLAYERSLISTBOX ] = control;

	control = std::make_unique<ctp2_Switch>(
		&errcode,
		aui_UniqueId(),
		"allinonewindow.publicprivateteamswitch" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_PPTSWITCH ] = control;

	control = spNew_ctp2_Button(
		&errcode,
		"allinonewindow",
		"kickbutton",
		nullptr);
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_KICKBUTTON ] = control;

	control = spNew_ctp2_Button(
		&errcode,
		"allinonewindow",
		"infobutton",
		nullptr);
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_INFOBUTTON ] = control;

	control = std::make_unique<aui_Static>(
		&errcode,
		aui_UniqueId(),
		"ruleswindow.rulessheet" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_RULESSHEET ] = control;

	control = std::make_unique<aui_Button>(
		&errcode,
		aui_UniqueId(),
		"ruleswindow.rulesokbutton" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_RULESOKBUTTON ] = control;

	control = std::make_unique<aui_Static>(
		&errcode,
		aui_UniqueId(),
		"ruleswindow.rulessheet.titlestatictext" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_RULESTITLESTATICTEXT ] = control;

	control = std::make_unique<aui_Static>(
		&errcode,
		aui_UniqueId(),
		"ruleswindow.rulessheet.titlestatictext2" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_RULESTITLESTATICTEXT2 ] = control;

	control = spNew_ctp2_Button(
		&errcode,
		"ruleswindow",
		"rulessheet.agesbutton",
		nullptr);

	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_AGESBUTTON ] = control;

	control = spNew_ctp2_Button(
		&errcode,
		"ruleswindow",
		"rulessheet.mapsizebutton",
		nullptr);
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_MAPSIZEBUTTON ] = control;

	control = spNew_ctp2_Button(
		&errcode,
		"ruleswindow",
		"rulessheet.worldtypebutton",
		nullptr);
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_WORLDTYPEBUTTON ] = control;

	control = spNew_ctp2_Button(
		&errcode,
		"ruleswindow",
		"rulessheet.worldshapebutton",
		nullptr);
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_WORLDSHAPEBUTTON ] = control;

	control = spNew_ctp2_Button(
		&errcode,
		"ruleswindow",
		"rulessheet.difficultybutton",
		nullptr);
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_DIFFICULTYBUTTON ] = control;

	control = std::make_unique<aui_Switch>(
		&errcode,
		aui_UniqueId(),
		"ruleswindow.rulessheet.dynamicjoinswitch" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_DYNAMICJOINSWITCH ] = control;

	control = std::make_unique<aui_Switch>(
		&errcode,
		aui_UniqueId(),
		"ruleswindow.rulessheet.handicappingswitch" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_HANDICAPPINGSWITCH ] = control;

	control = std::make_unique<aui_Switch>(
		&errcode,
		aui_UniqueId(),
		"ruleswindow.rulessheet.bloodlustswitch" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_BLOODLUSTSWITCH ] = control;

	control = std::make_unique<aui_Switch>(
		&errcode,
		aui_UniqueId(),
		"ruleswindow.rulessheet.pollutionswitch" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_POLLUTIONSWITCH ] = control;

	control = std::make_unique<c3_Static>(
		&errcode,
		aui_UniqueId(),
		"ruleswindow.rulessheet.goldstatictext" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_GOLDSTATICTEXT ] = control;

	control = std::make_unique<c3_EditButton>(
		&errcode,
		aui_UniqueId(),
		"ruleswindow.rulessheet.civpointsbutton",
		nullptr,
		std::make_unique<CivPointsButtonAction>().release()).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_CIVPOINTSBUTTON ] = control;

	control = std::make_unique<c3_Static>(
		&errcode,
		aui_UniqueId(),
		"ruleswindow.rulessheet.pwstatictext" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_PWSTATICTEXT ] = control;

	control = std::make_unique<c3_EditButton>(
		&errcode,
		aui_UniqueId(),
		"ruleswindow.rulessheet.pwpointsbutton",
		nullptr,
		std::make_unique<PwPointsButtonAction>().release()).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_PWPOINTSBUTTON ] = control;

	control = std::make_unique<aui_Static>(
		&errcode,
		aui_UniqueId(),
		"exclusionswindow.exclusionssheet" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_EXCLUSIONSSHEET ] = control;

	control = std::make_unique<aui_Button>(
		&errcode,
		aui_UniqueId(),
		"exclusionswindow.exclusionsokbutton" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_EXCLUSIONSOKBUTTON ] = control;

	control = std::make_unique<aui_Static>(
		&errcode,
		aui_UniqueId(),
		"exclusionswindow.exclusionssheet.titlestatictext" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_EXCLUSIONSTITLESTATICTEXT ] = control;

	control = std::make_unique<aui_TabGroup>(
		&errcode,
		aui_UniqueId(),
		"exclusionswindow.exclusionssheet.smallnastytabgroup" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_SMALLNASTYTABGROUP ] = control;

	control = std::make_unique<aui_Tab>(
		&errcode,
		aui_UniqueId(),
		"exclusionswindow.exclusionssheet.smallnastytabgroup.unitstab" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_UNITSTAB ] = control;

	control = std::make_unique<ns_CivListBox>(
		&errcode,
		aui_UniqueId(),
		"exclusionswindow.exclusionssheet.smallnastytabgroup.unitstab.pane.unitslistbox" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_UNITSLISTBOX ] = control;

	control = std::make_unique<aui_Tab>(
		&errcode,
		aui_UniqueId(),
		"exclusionswindow.exclusionssheet.smallnastytabgroup.improvementstab" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_IMPROVEMENTSTAB ] = control;

	control = std::make_unique<ns_CivListBox>(
		&errcode,
		aui_UniqueId(),
		"exclusionswindow.exclusionssheet.smallnastytabgroup.improvementstab.pane.improvementslistbox" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_IMPROVEMENTSLISTBOX ] = control;

	control = std::make_unique<aui_Tab>(
		&errcode,
		aui_UniqueId(),
		"exclusionswindow.exclusionssheet.smallnastytabgroup.wonderstab" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_WONDERSTAB ] = control;

	control = std::make_unique<ns_CivListBox>(
		&errcode,
		aui_UniqueId(),
		"exclusionswindow.exclusionssheet.smallnastytabgroup.wonderstab.pane.wonderslistbox" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_WONDERSLISTBOX ] = control;


	control = std::make_unique<aui_Button>(
		&errcode,
		aui_UniqueId(),
		"allinonewindow.okbutton" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_OKBUTTON ] = control;

	control = std::make_unique<aui_Button>(
		&errcode,
		aui_UniqueId(),
		"allinonewindow.cancelbutton" ).release();
	Assert( AUI_NEWOK(control,errcode) );
	if ( !AUI_NEWOK(control,errcode) ) return errcode;
	m_controls[ CONTROL_CANCELBUTTON ] = control;

	aui_Ldl::SetupHeirarchyFromRoot( "allinonewindow" );
	aui_Ldl::SetupHeirarchyFromRoot( "ruleswindow" );
	aui_Ldl::SetupHeirarchyFromRoot( "exclusionswindow" );

	m_controls[ CONTROL_GAMENAMETEXTFIELD ]->SetAction(std::make_unique<GameNameTextFieldAction>().release());
	m_controls[ CONTROL_PPTSWITCH ]->SetAction(std::make_unique<PPTSwitchAction>().release());
	m_controls[ CONTROL_KICKBUTTON ]->SetAction(std::make_unique<KickButtonAction>().release());
	m_controls[ CONTROL_INFOBUTTON ]->SetAction(std::make_unique<InfoButtonAction>().release());
	m_controls[ CONTROL_OKBUTTON ]->SetAction(std::make_unique<OKButtonAction>().release());
	m_controls[ CONTROL_REVIEWBUTTON ]->SetAction(std::make_unique<ReviewButtonAction>().release());
	m_controls[ CONTROL_RULESBUTTON ]->SetAction(std::make_unique<RulesButtonAction>().release());
	m_controls[ CONTROL_EXCLUSIONSBUTTON ]->SetAction(std::make_unique<ExclusionsButtonAction>().release());
	m_controls[ CONTROL_RULESOKBUTTON ]->SetAction(std::make_unique<RulesOKButtonAction>().release());
	m_controls[ CONTROL_EXCLUSIONSOKBUTTON ]->SetAction(std::make_unique<ExclusionsOKButtonAction>().release());
	m_controls[ CONTROL_LOCKSWITCH ]->SetAction(std::make_unique<LockSwitchAction>().release());
	m_controls[ CONTROL_ADDAIBUTTON ]->SetAction(std::make_unique<AddAIButtonAction>().release());
	m_controls[ CONTROL_CANCELBUTTON ]->SetAction(std::make_unique<CancelButtonAction>().release());
	m_controls[ CONTROL_HPLAYERSLISTBOX ]->SetAction(std::make_unique<PlayersListBoxAction>().release());
	m_controls[ CONTROL_PLAYSTYLEDROPDOWN ]->SetAction(std::make_unique<PlayStyleDropDownAction>().release());

	((ctp2_Spinner *)m_controls[CONTROL_PLAYSTYLEVALUESPINNER])->SetSpinnerCallback(PlayStyleValueSpinnerCallback, nullptr);

	m_controls[ CONTROL_DYNAMICJOINSWITCH ]->SetAction(std::make_unique<DynamicJoinSwitchAction>().release());
	m_controls[ CONTROL_HANDICAPPINGSWITCH ]->SetAction(std::make_unique<HandicappingSwitchAction>().release());
	m_controls[ CONTROL_BLOODLUSTSWITCH ]->SetAction(std::make_unique<BloodlustSwitchAction>().release());
	m_controls[ CONTROL_POLLUTIONSWITCH ]->SetAction(std::make_unique<PollutionSwitchAction>().release());
	m_controls[ CONTROL_AGESBUTTON ]->SetAction(std::make_unique<AgesButtonAction>().release());
	m_controls[ CONTROL_MAPSIZEBUTTON ]->SetAction(std::make_unique<MapSizeButtonAction>().release());
	m_controls[ CONTROL_WORLDTYPEBUTTON ]->SetAction(std::make_unique<WorldTypeButtonAction>().release());
	m_controls[ CONTROL_WORLDSHAPEBUTTON ]->SetAction(std::make_unique<WorldShapeButtonAction>().release());
	m_controls[ CONTROL_DIFFICULTYBUTTON ]->SetAction(std::make_unique<DifficultyButtonAction>().release());

	((aui_ListBox *)m_controls[ CONTROL_HPLAYERSLISTBOX ])->GetHeader()->
		Enable( FALSE );

	((ns_CivListBox *)m_controls[ CONTROL_UNITSLISTBOX ])->
		m_selectableList = FALSE;
	((ns_CivListBox *)m_controls[ CONTROL_IMPROVEMENTSLISTBOX ])->
		m_selectableList = FALSE;
	((ns_CivListBox *)m_controls[ CONTROL_WONDERSLISTBOX ])->
		m_selectableList = FALSE;

	((aui_ListBox *)m_controls[ CONTROL_HPLAYERSLISTBOX ])->
		SetForceSelect( TRUE );

	m_controls[ CONTROL_PLAYERSSHEET ]->SetBlindness( true );

	m_controls[ CONTROL_RULESSHEET ]->SetBlindness( true );
	m_controls[ CONTROL_EXCLUSIONSSHEET ]->SetBlindness( true );

	ctp2_DropDown *playstyleDropDown =
		(ctp2_DropDown *)m_controls[ CONTROL_PLAYSTYLEDROPDOWN ];

	aui_StringTable st( &errcode, "strings.playstyles" );
	Assert( AUI_SUCCESS(errcode) );
	if ( !AUI_SUCCESS(errcode) ) return AUI_ERRCODE_HACK;

	sint32 numStyles = st.GetNumStrings();

	for ( sint32 i = 0; i < numStyles; i++ )
	{
		ns_ListItem *item = std::make_unique<ns_ListItem>(
			&errcode,
			st.GetString( i ),
			"listitems.playstyleitem" ).release();
		Assert( AUI_NEWOK(item,errcode) );
		if ( !AUI_NEWOK(item,errcode) ) return AUI_ERRCODE_MEMALLOCFAILED;









		playstyleDropDown->AddItem( (ctp2_ListItem *)item );

	}

	aui_Switch *s = (aui_Switch *)m_controls[ CONTROL_PPTSWITCH ];
	s->SetState( s->GetState() );


	aui_Window *window = (aui_Window *)aui_Ldl::GetObject("ruleswindow");
	ctp2_Button *button = (ctp2_Button *)aui_Ldl::GetObject("ruleswindow", "rulesokbutton");
	if (button) {
		button->Move( window->Width() - button->Width() - 14, window->Height() - button->Height() - 17);
	}

	return AUI_ERRCODE_OK;
}


AllinoneWindow::~AllinoneWindow()
{
    allocated::clear(g_rulesWindow);
    allocated::clear(g_exclusionsWindow);

	sint32 numActions = sizeof( m_dbActionArray ) / sizeof( m_dbActionArray[0] );
	sint32 i;
	for ( i = 0; i < numActions; i++ )
	{
		m_dbActionArray[i].reset();
	}

	((aui_DropDown *)m_controls[ CONTROL_PLAYSTYLEDROPDOWN ])->
		GetListBox()->RemoveItems( TRUE );

	aui_ListBox * listbox =
		(aui_ListBox *)m_controls[ CONTROL_UNITSLISTBOX ];

	for ( i = listbox->NumItems(); i; i-- )
	{
		aui_Item *item = listbox->GetItemByIndex( 0 );
		listbox->RemoveItem( item->Id() );

		ListPos pos = item->ChildList()->GetHeadPosition();
		for ( sint32 j = item->ChildList()->L(); j; j-- )
		{
			aui_Item *subitem = (aui_Item *)item->ChildList()->GetNext( pos );

            std::unique_ptr<aui_Action>( subitem->GetAction() ).reset();
			std::unique_ptr<aui_Item>( subitem ).reset();
		}

		std::unique_ptr<aui_Action>( item->GetAction() ).reset();
		std::unique_ptr<aui_Item>( item ).reset();
	}

	listbox =
		(aui_ListBox *)m_controls[ CONTROL_IMPROVEMENTSLISTBOX ];
	for ( i = listbox->NumItems(); i; i-- )
	{
		aui_Item *item = listbox->GetItemByIndex( 0 );
		listbox->RemoveItem( item->Id() );

		ListPos pos = item->ChildList()->GetHeadPosition();
		for ( sint32 j = item->ChildList()->L(); j; j-- )
		{
			aui_Item *subitem = (aui_Item *)item->ChildList()->GetNext( pos );

			std::unique_ptr<aui_Action>( subitem->GetAction() ).reset();
			std::unique_ptr<aui_Item>( subitem ).reset();
		}

		std::unique_ptr<aui_Action>( item->GetAction() ).reset();
		std::unique_ptr<aui_Item>( item ).reset();
	}

	listbox =
		(aui_ListBox *)m_controls[ CONTROL_WONDERSLISTBOX ];
	for ( i = listbox->NumItems(); i; i-- )
	{
		aui_Item *item = listbox->GetItemByIndex( 0 );
		listbox->RemoveItem( item->Id() );

		ListPos pos = item->ChildList()->GetHeadPosition();
		for ( sint32 j = item->ChildList()->L(); j; j-- )
		{
			aui_Item *subitem = (aui_Item *)item->ChildList()->GetNext( pos );

			std::unique_ptr<aui_Action>( subitem->GetAction() ).reset();
			std::unique_ptr<aui_Item>( subitem ).reset();
		}

		std::unique_ptr<aui_Action>( item->GetAction() ).reset();
		std::unique_ptr<aui_Item>( item ).reset();
	}

	m_aiplayerList.reset();

	agesscreen_Cleanup();
	spnewgamemapsizescreen_Cleanup();
	spnewgamemapshapescreen_Cleanup();
	spnewgametribescreen_Cleanup();

	custommapscreen_Cleanup();
	spnewgamediffscreen_Cleanup();

    if (this == g_allinoneWindow)
    {
	    g_allinoneWindow = nullptr;
    }
}


AUI_ERRCODE AllinoneWindow::DrawThis(
	aui_Surface *surface,
	sint32 x,
	sint32 y )
{
	(void) ns_Window::DrawThis( surface, x, y );
	return AUI_ERRCODE_OK;
}


AUI_ERRCODE AllinoneWindow::CreateExclusions( )
{
	if ( m_createdExclusions )
	{
		sint32			i;
		aui_ListBox *	listbox = (aui_ListBox *)m_controls[CONTROL_UNITSLISTBOX];

		for (i = listbox->NumItems(); i; i-- )
		{
			aui_Item *item = listbox->GetItemByIndex( 0 );
			listbox->RemoveItem( item->Id() );

			ListPos pos = item->ChildList()->GetHeadPosition();
			for ( sint32 j = item->ChildList()->L(); j; j-- )
			{
				aui_Item *subitem =
					(aui_Item *)item->ChildList()->GetNext( pos );

				std::unique_ptr<aui_Action>( subitem->GetAction() ).reset();
				std::unique_ptr<aui_Item>( subitem ).reset();
			}

			std::unique_ptr<aui_Action>( item->GetAction() ).reset();
			std::unique_ptr<aui_Item>( item ).reset();
		}

		listbox =
			(aui_ListBox *)m_controls[ CONTROL_IMPROVEMENTSLISTBOX ];
		for ( i = listbox->NumItems(); i; i-- )
		{
			aui_Item *item = listbox->GetItemByIndex( 0 );
			listbox->RemoveItem( item->Id() );

			ListPos pos = item->ChildList()->GetHeadPosition();
			for ( sint32 j = item->ChildList()->L(); j; j-- )
			{
				aui_Item *subitem =
					(aui_Item *)item->ChildList()->GetNext( pos );

				std::unique_ptr<aui_Action>( subitem->GetAction() ).reset();
				std::unique_ptr<aui_Item>( subitem ).reset();
			}

			std::unique_ptr<aui_Action>( item->GetAction() ).reset();
			std::unique_ptr<aui_Item>( item ).reset();
		}

		listbox =
			(aui_ListBox *)m_controls[ CONTROL_WONDERSLISTBOX ];
		for ( i = listbox->NumItems(); i; i-- )
		{
			aui_Item *item = listbox->GetItemByIndex( 0 );
			listbox->RemoveItem( item->Id() );

			ListPos pos = item->ChildList()->GetHeadPosition();
			for ( sint32 j = item->ChildList()->L(); j; j-- )
			{
				aui_Item *subitem =
					(aui_Item *)item->ChildList()->GetNext( pos );

				std::unique_ptr<aui_Action>( subitem->GetAction() ).reset();
				std::unique_ptr<aui_Item>( subitem ).reset();
			}

			std::unique_ptr<aui_Action>( item->GetAction() ).reset();
			std::unique_ptr<aui_Item>( item ).reset();
		}

		m_numAvailUnits = 0;
		memset( m_units, 0, sizeof( m_units ) );
		m_numAvailImprovements = 0;
		memset( m_improvements, 0, sizeof( m_improvements ) );
		m_numAvailWonders = 0;
		memset( m_wonders, 0, sizeof( m_wonders ) );

		m_createdExclusions = false;
	}

	aui_ListBox *listbox =
		(aui_ListBox *)m_controls[ CONTROL_UNITSLISTBOX ];
	listbox->SetAbsorbancy( FALSE );
	sint32 height = listbox->Height();

	m_numAvailUnits = nsunits_Get()->GetStrings()->GetNumStrings();
	gamesetup_Get().SetNumAvailUnits( m_numAvailUnits );

	aui_Switch *item = nullptr;
	tech_WLList<aui_Switch *> unitList;
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	sint32 i;
	for ( i = 0; i < m_numAvailUnits; i++ )
	{
		if ( !nsunits_Get()->m_noIndex[ i ] )
		{
			item = std::make_unique<aui_Switch>(
				&errcode,
				aui_UniqueId(),
				"listitems.exclusioncheckbox" ).release();
			Assert( AUI_NEWOK(item,errcode) );
			if ( !AUI_NEWOK(item,errcode) ) return AUI_ERRCODE_MEMALLOCFAILED;

			item->SetText( nsunits_Get()->GetStrings()->GetString( i ) );
			item->SetAction(std::make_unique<UnitExclusionAction>(i).release());

			m_units[ i ] = item;

			if ( (height -= item->Height()) < 0 )
				break;

			unitList.AddTail( item );
		}
	}

	ListPos pos = unitList.GetHeadPosition();

	for ( i++; i < m_numAvailUnits; i++ )
	{
		if ( !nsunits_Get()->m_noIndex[ i ] )
		{
			unitList.GetNext( pos )->AddChild( item );

			item = std::make_unique<aui_Switch>(
				&errcode,
				aui_UniqueId(),
				"listitems.exclusioncheckbox" ).release();
			Assert( AUI_NEWOK(item,errcode) );
			if ( !AUI_NEWOK(item,errcode) ) return AUI_ERRCODE_MEMALLOCFAILED;

			item->SetText( nsunits_Get()->GetStrings()->GetString( i ) );
			item->SetAction(std::make_unique<UnitExclusionAction>(i).release());

			m_units[ i ] = item;

			if ( !pos )
				pos = unitList.GetHeadPosition();
		}
	}

	unitList.GetAt( pos )->AddChild( item );
	pos = unitList.GetHeadPosition();
	for ( i = unitList.L(); i; i-- )
		listbox->AddItem( (aui_Item *)unitList.GetNext( pos ) );




	listbox = (aui_ListBox *)m_controls[ CONTROL_IMPROVEMENTSLISTBOX ];
	listbox->SetAbsorbancy( FALSE );
	height = listbox->Height();

	tech_WLList<aui_Switch *> improvementList;

	m_numAvailImprovements = nsimprovements_Get()->GetStrings()->GetNumStrings();
	gamesetup_Get().SetNumAvailImprovements( m_numAvailImprovements );
	for ( i = 0; i < m_numAvailImprovements; i++ )
	{
		item = std::make_unique<aui_Switch>(
			&errcode,
			aui_UniqueId(),
			"listitems.exclusioncheckbox" ).release();
		Assert( AUI_NEWOK(item,errcode) );
		if ( !AUI_NEWOK(item,errcode) ) return AUI_ERRCODE_MEMALLOCFAILED;

		item->SetText( nsimprovements_Get()->GetStrings()->GetString( i ) );
		item->SetAction(std::make_unique<ImprovementExclusionAction>(i).release());

		m_improvements[ i ] = item;

		if ( (height -= item->Height()) < 0 )
			break;

		improvementList.AddTail( item );
	}

	pos = improvementList.GetHeadPosition();

	for ( i++; i < m_numAvailImprovements; i++ )
	{
		improvementList.GetNext( pos )->AddChild( item );

		item = std::make_unique<aui_Switch>(
			&errcode,
			aui_UniqueId(),
			"listitems.exclusioncheckbox" ).release();
		Assert( AUI_NEWOK(item,errcode) );
		if ( !AUI_NEWOK(item,errcode) ) return AUI_ERRCODE_MEMALLOCFAILED;

		item->SetText( nsimprovements_Get()->GetStrings()->GetString( i ) );
		item->SetAction(std::make_unique<ImprovementExclusionAction>(i).release());

		m_improvements[ i ] = item;

		if ( !pos )
			pos = improvementList.GetHeadPosition();
	}

	improvementList.GetAt( pos )->AddChild( item );
	pos = improvementList.GetHeadPosition();
	for ( i = improvementList.L(); i; i-- )
		listbox->AddItem( (aui_Item *)improvementList.GetNext( pos ) );




	listbox = (aui_ListBox *)m_controls[ CONTROL_WONDERSLISTBOX ];
	listbox->SetAbsorbancy( FALSE );
	height = listbox->Height();

	tech_WLList<aui_Switch *> wonderList;

	m_numAvailWonders = nswonders_Get()->GetStrings()->GetNumStrings();
	gamesetup_Get().SetNumAvailWonders( m_numAvailWonders );
	for ( i = 0; i < m_numAvailWonders; i++ )
	{
		item = std::make_unique<aui_Switch>(
			&errcode,
			aui_UniqueId(),
			"listitems.exclusioncheckbox" ).release();
		Assert( AUI_NEWOK(item,errcode) );
		if ( !AUI_NEWOK(item,errcode) ) return AUI_ERRCODE_MEMALLOCFAILED;

		item->SetText( nswonders_Get()->GetStrings()->GetString( i ) );
		item->SetAction(std::make_unique<WonderExclusionAction>(i).release());

		m_wonders[ i ] = item;

		if ( (height -= item->Height()) < 0 )
			break;

		wonderList.AddTail( item );
	}

	pos = wonderList.GetHeadPosition();

	for ( i++; i < m_numAvailWonders; i++ )
	{
		wonderList.GetNext( pos )->AddChild( item );

		item = std::make_unique<aui_Switch>(
			&errcode,
			aui_UniqueId(),
			"listitems.exclusioncheckbox" ).release();
		Assert( AUI_NEWOK(item,errcode) );
		if ( !AUI_NEWOK(item,errcode) ) return AUI_ERRCODE_MEMALLOCFAILED;

		item->SetText( nswonders_Get()->GetStrings()->GetString( i ) );
		item->SetAction(std::make_unique<WonderExclusionAction>(i).release());

		m_wonders[ i ] = item;

		if ( !pos )
			pos = wonderList.GetHeadPosition();
	}

	wonderList.GetAt( pos )->AddChild( item );
	pos = wonderList.GetHeadPosition();
	for ( i = wonderList.L(); i; i-- )
		listbox->AddItem( (aui_Item *)wonderList.GetNext( pos ) );

	m_createdExclusions = true;

	uint8 isScenario = m_scenInfo.isScenario;

	if ( m_mode == JOIN ||
		 m_mode == CONTINUE_CREATE ||
		 m_mode == CONTINUE_JOIN )
		SetMode( m_mode );
	m_scenInfo.isScenario = isScenario;

	return AUI_ERRCODE_OK;
}


void AllinoneWindow::SetMode( Mode m )
{

	aui_Control *pane;
	sint32 i;
	ListPos pos;













	m_scenInfo.isScenario = FALSE;

	switch( m_mode = m )
	{
	case CREATE:
		m_controls[ CONTROL_KICKBUTTON ]->Enable( !playersetup_Get().IsReadyToLaunch() );;
		m_controls[ CONTROL_GAMENAMETEXTFIELD ]->Enable( !playersetup_Get().IsReadyToLaunch() );
		m_controls[ CONTROL_PLAYSTYLEDROPDOWN ]->Enable( !playersetup_Get().IsReadyToLaunch() );
		m_controls[ CONTROL_PLAYSTYLEVALUESPINNER ]->Enable( !playersetup_Get().IsReadyToLaunch() );
		m_controls[ CONTROL_LOCKSWITCH ]->Enable( !playersetup_Get().IsReadyToLaunch() );
		m_controls[ CONTROL_ADDAIBUTTON ]->Enable( !playersetup_Get().IsReadyToLaunch() );

		m_controls[ CONTROL_CIVPOINTSBUTTON ]->Enable( !playersetup_Get().IsReadyToLaunch() );
		m_controls[ CONTROL_PWPOINTSBUTTON ]->Enable( !playersetup_Get().IsReadyToLaunch() );


		m_controls[ CONTROL_RULESSHEET ]->Enable( !playersetup_Get().IsReadyToLaunch() );

		pane = ((aui_ListBox *)m_controls[ CONTROL_UNITSLISTBOX ])
			->GetPane();
		pos = pane->ChildList()->GetHeadPosition();
		for ( i = pane->ChildList()->L(); i; i-- )
			pane->ChildList()->GetNext( pos )->Enable( !playersetup_Get().IsReadyToLaunch() );

		pane = ((aui_ListBox *)m_controls[ CONTROL_IMPROVEMENTSLISTBOX ])
			->GetPane();
		pos = pane->ChildList()->GetHeadPosition();
		for ( i = pane->ChildList()->L(); i; i-- )
			pane->ChildList()->GetNext( pos )->Enable( !playersetup_Get().IsReadyToLaunch() );

		pane = ((aui_ListBox *)m_controls[ CONTROL_WONDERSLISTBOX ])
			->GetPane();
		pos = pane->ChildList()->GetHeadPosition();
		for ( i = pane->ChildList()->L(); i; i-- )
			pane->ChildList()->GetNext( pos )->Enable( !playersetup_Get().IsReadyToLaunch() );

		break;

	case JOIN:
	case CONTINUE_JOIN:
	case CONTINUE_CREATE:
		m_controls[ CONTROL_KICKBUTTON ]->Enable( false );
		if(m_mode != CONTINUE_CREATE) {
			m_controls[ CONTROL_GAMENAMETEXTFIELD ]->Enable( false );
		} else {
			m_controls[ CONTROL_GAMENAMETEXTFIELD ]->Enable( !playersetup_Get().IsReadyToLaunch() );
		}
		m_controls[ CONTROL_PLAYSTYLEDROPDOWN ]->Enable( false );
		m_controls[ CONTROL_PLAYSTYLEVALUESPINNER ]->Enable( false );
		m_controls[ CONTROL_LOCKSWITCH ]->Enable( false );
		m_controls[ CONTROL_ADDAIBUTTON ]->Enable( false );

		m_controls[ CONTROL_CIVPOINTSBUTTON ]->Enable( false );
		m_controls[ CONTROL_PWPOINTSBUTTON ]->Enable( false );


		m_controls[ CONTROL_RULESSHEET ]->Enable( false );

		if(m_mode == CONTINUE_CREATE) {

			m_controls[ CONTROL_DYNAMICJOINSWITCH ]->Enable(true);
		}

		m_controls[ CONTROL_RULESOKBUTTON ]->Enable( true );

		m_controls[ CONTROL_MAPSIZEBUTTON ]->Enable( true );
		m_controls[ CONTROL_WORLDTYPEBUTTON ]->Enable( true );
		m_controls[ CONTROL_WORLDSHAPEBUTTON ]->Enable( true );
		m_controls[ CONTROL_DIFFICULTYBUTTON ]->Enable( true );
		m_controls[ CONTROL_AGESBUTTON ]->Enable( true );

		pane = ((aui_ListBox *)m_controls[ CONTROL_UNITSLISTBOX ])
			->GetPane();
		pos = pane->ChildList()->GetHeadPosition();
		for ( i = pane->ChildList()->L(); i; i-- )
			pane->ChildList()->GetNext( pos )->Enable( false );

		pane = ((aui_ListBox *)m_controls[ CONTROL_IMPROVEMENTSLISTBOX ])
			->GetPane();
		pos = pane->ChildList()->GetHeadPosition();
		for ( i = pane->ChildList()->L(); i; i-- )
			pane->ChildList()->GetNext( pos )->Enable( false );

		pane = ((aui_ListBox *)m_controls[ CONTROL_WONDERSLISTBOX ])
			->GetPane();
		pos = pane->ChildList()->GetHeadPosition();
		for ( i = pane->ChildList()->L(); i; i-- )
			pane->ChildList()->GetNext( pos )->Enable( false );

		break;

	default:

		Assert( FALSE );
		break;
	}

	if ( m_mode == CONTINUE_CREATE )
	{
		m_controls[ CONTROL_PLAYSTYLEDROPDOWN ]->Enable( true );
		m_controls[ CONTROL_PLAYSTYLEVALUESPINNER ]->Enable( true );




	}





}


BOOL AllinoneWindow::IsMine( NETFunc::Player *player )
{
	return netfunc_Get()->GetPlayer()->Equals( player );
}


ns_HPlayerItem *AllinoneWindow::GetHPlayerFromId( dpid_t id )
{
	ns_HPlayerListBox *hplistbox = (ns_HPlayerListBox *)
		m_controls[ CONTROL_HPLAYERSLISTBOX ];

	ListPos pos = hplistbox->GetPane()->ChildList()->GetHeadPosition();
	for ( sint32 i = hplistbox->GetPane()->ChildList()->L(); i; i-- )
	{
		ns_HPlayerItem *hplayer = (ns_HPlayerItem *)hplistbox->GetPane()->
			ChildList()->GetNext( pos );

		if ( !hplayer->IsAI() )
			if ( hplayer->GetPlayer()->GetId() == id )
				return hplayer;
	}

	return nullptr;
}


NETFunc::Player *AllinoneWindow::GetPlayerFromKey( uint16 key )
{
	ns_GPlayerListBox *listbox = (ns_GPlayerListBox *)
		m_controls[ CONTROL_GPLAYERSLISTBOX ];

	ListPos pos = listbox->GetPane()->ChildList()->GetHeadPosition();
	for ( sint32 i = listbox->GetPane()->ChildList()->L(); i; i-- )
	{
		NETFunc::Player *player = (NETFunc::Player *)
			((ns_Item<NETFunc::Player, ns_Player> *)
			 listbox->GetPane()->ChildList()->GetNext( pos ))->
			GetNetShellObject()->GetNETFuncObject();

		if ( *(uint16 *)player->GetKey()->buf == key )
			return player;
	}

	return nullptr;
}


nf_AIPlayer *AllinoneWindow::GetAIPlayerFromKey( uint16 key )
{
	ns_AIPlayerListBox *listbox = (ns_AIPlayerListBox *)
		m_controls[ CONTROL_AIPLAYERSLISTBOX ];

	ListPos pos = listbox->GetPane()->ChildList()->GetHeadPosition();
	for ( sint32 i = listbox->GetPane()->ChildList()->L(); i; i-- )
	{
		nf_AIPlayer *player = (nf_AIPlayer *)
			((ns_Item<nf_AIPlayer, ns_AIPlayer> *)
			 listbox->GetPane()->ChildList()->GetNext( pos ))->
			GetNetShellObject()->GetNETFuncObject();

		if ( *(uint16 *)player->GetKey()->buf == key )
			return player;
	}

	return nullptr;
}


BOOL AllinoneWindow::WhoHasTribe( sint32 index, uint16 *curKey, BOOL *curIsAI, BOOL *curIsFemale )
{
	Assert( curKey != nullptr );
	if ( !curKey ) return FALSE;

	*curKey = 0;

	Assert( curIsAI != nullptr );
	if ( !curIsAI ) return FALSE;

	*curIsAI = FALSE;

	Assert( curIsFemale != nullptr );
	if ( !curIsFemale ) return FALSE;

	*curIsFemale = FALSE;

	if (!IsValidTribeIndex(index))
	{
		return FALSE;
	}

	Assert( netfunc_Get()->IsHost() );
	if ( !NETFunc::IsHost() ) return FALSE;

	if ( index == 0 ) return TRUE;

	if ( index == 1 )
	{
		*curKey = 0xffff;
		*curIsAI = TRUE;
		*curIsFemale = FALSE;
		return TRUE;
	}

	TribeSlot *tribeSlots = gamesetup_Get().GetTribeSlots();
	for (int i = 0; i < k_NS_MAX_PLAYERS; i++ )
	{
		if ( tribeSlots[ i ].tribe == index )
		{
			*curKey = tribeSlots[ i ].key;
			*curIsAI = tribeSlots[ i ].isAI;
			*curIsFemale = tribeSlots[ i ].isFemale;
			break;
		}
	}

	return TRUE;
}


sint32 AllinoneWindow::FindTribe( uint16 key, BOOL isAI, BOOL *isFemale )
{

	Assert( netfunc_Get()->IsHost() );
	if ( !NETFunc::IsHost() ) return 0;

	if ( key )
	{
		TribeSlot *tribeSlots = gamesetup_Get().GetTribeSlots();

		for (int i = 0; i < k_NS_MAX_PLAYERS; i++ )
			if ( tribeSlots[ i ].key == key && tribeSlots[ i ].isAI == isAI )
			{
				if ( isFemale ) *isFemale = tribeSlots[ i ].isFemale;
				return tribeSlots[ i ].tribe;
			}
	}

	else
	{
		if ( m_mode == CONTINUE_CREATE || m_mode == CONTINUE_JOIN )
		{

			for ( sint32 i = 2; i < nstribes_Get()->GetNumTribes(); i++ )
			{
				uint16 curKey;
				BOOL curIsAI;
				BOOL curIsFemale;
				BOOL success = WhoHasTribe( i, &curKey, &curIsAI, &curIsFemale );
				Assert( success );

				if ( curKey == 0 )
				{
					if(!m_scenInfo.isScenario)
					{
						for ( sint32 j = 0; j < k_NS_MAX_PLAYERS; j++ )
						{
							if (
								gamesetup_Get().GetSavedTribeSlots()[ j ].tribe == i &&
								gamesetup_Get().GetSavedTribeSlots()[ j ].isAI == isAI )
							{
								if ( isFemale ) *isFemale = gamesetup_Get().GetSavedTribeSlots()[ j ].isFemale;
								return i;
							}
						}
					} else {

						if((m_scenInfo.m_startInfoType == (uint8)STARTINFOTYPE_POSITIONSFIXED) ||

						   (m_scenInfo.m_startInfoType == (uint8)STARTINFOTYPE_NOLOCS)) {

							if ( isFemale ) *isFemale = curIsFemale;
							return i;
						}

						for( sint32 j = 0; j < m_scenInfo.m_numStartPositions; j++) {
							if(m_scenInfo.m_civs[j] == i - 1) {
								if(isFemale) *isFemale = curIsFemale;
								return i;
							}
						}
					}
				}
			}
		}
		else if(m_scenInfo.isScenario && m_scenInfo.m_haveSavedGame) {
			for(int m_legalCiv : m_scenInfo.m_legalCivs) {
				if(m_legalCiv > 0) {
					uint16 curKey;
					BOOL curIsAI;
					BOOL curIsFemale;
					BOOL success = WhoHasTribe( m_legalCiv + 1,
												&curKey, &curIsAI,
												&curIsFemale );
					Assert(success);
					if(curKey == 0) {
						if(isFemale) *isFemale = curIsFemale;
						return m_legalCiv + 1;
					}
				}
			}
		}
		else
		{

			for ( sint32 i = 2; i < nstribes_Get()->GetNumTribes(); i++ )
			{
				uint16 curKey;
				BOOL curIsAI;
				BOOL curIsFemale;
				BOOL success = WhoHasTribe( i, &curKey, &curIsAI, &curIsFemale );
				Assert( success );

				if ( curKey == 0 )
				{
					if ( isFemale ) *isFemale = curIsFemale;
					return i;
				}
			}
		}
	}

	return 0;
}


BOOL AllinoneWindow::AssignTribe(
	sint32 index,
	uint16 key,
	BOOL isAI,
	BOOL isFemale,
	BOOL unassign )
{

	Assert( index != 1 );
	if ( index == 1 ) return FALSE;

	if (!IsValidTribeIndex(index))
	{
		return FALSE;
	}

	Assert( netfunc_Get()->IsHost() );
	if ( !NETFunc::IsHost() ) return FALSE;

	uint16 curKey;
	BOOL curIsAI;
	BOOL curIsFemale;
	BOOL success = WhoHasTribe( index, &curKey, &curIsAI, &curIsFemale );
	Assert( success );

	Assert( curKey == 0 || ( curKey == key && curIsAI == isAI ) );
	if ( curKey != 0 && ( curKey != key || curIsAI != isAI ) ) return FALSE;

	TribeSlot *tribeSlots = gamesetup_Get().GetTribeSlots();
	sint32 i;
	for ( i = 0; i < k_NS_MAX_PLAYERS; i++ )
	{
		if ( tribeSlots[ i ].key == key && tribeSlots[ i ].isAI == isAI )
		{
			memset( tribeSlots + i, 0, sizeof( TribeSlot ) );
			break;
		}
	}

	if ( !unassign )
	{

		PLAYER_INDEX playerIndex = i;
		if ( playerIndex == k_NS_MAX_PLAYERS )
		{
			for ( i = 0; i < k_NS_MAX_PLAYERS; i++ )
			{
				if ( tribeSlots[ i ].key == 0 )
				{
					playerIndex = i;
					break;
				}
			}
		}

		Assert( playerIndex < k_NS_MAX_PLAYERS );
		if ( playerIndex >= k_NS_MAX_PLAYERS ) return FALSE;



























		tribeSlots[ playerIndex ].isFemale  = static_cast<sint8>(isFemale);
		tribeSlots[ playerIndex ].isAI      = static_cast<sint8>(isAI);
		tribeSlots[ playerIndex ].key       = key;
		tribeSlots[ playerIndex ].tribe     = index;

		if ( !isAI )
		{

			if ( *(uint16 *)playersetup_Get().GetKey()->buf == key )
			{
				playersetup_Get().SetTribe( index );
				UpdatePlayerSetup();
			}
			else
			{

			}
		}
		else
		{
			nf_AIPlayer *aiplayer = GetAIPlayerFromKey( key );
			Assert( aiplayer != nullptr );
			if ( !aiplayer ) return FALSE;

			Assert( index > 0 );
			if ( index > 0 )
			{
				sint32 civ = index - 1;
				StringId sid = isFemale ?
					g_theCivilisationDB->Get(civ)->GetLeaderNameFemale():
					g_theCivilisationDB->Get(civ)->GetLeaderNameMale();
				MBCHAR leader[ dp_PNAMELEN + 1 ];
				strlcpy( leader, stringdb_Get()->GetNameStr( sid ), sizeof(leader));
				aiplayer->SetName( leader );
			}

			aiplayer->SetTribe( index );
			UpdateAIPlayerSetup( aiplayer );
		}
	}

	UpdateTribeSwitches();
	UpdateGameSetup();

	return TRUE;
}


void AllinoneWindow::RequestTribe( sint32 index )
{

	Assert( index != 1 );
	if ( index == 1 ) return;

	if (!IsValidTribeIndex(index)) return;

	Assert( !netfunc_Get()->IsHost() );
	if ( NETFunc::IsHost() ) return;

	NETFunc::Message message = NETFunc::Message(
		(NETFunc::Message::Code)CUSTOMCODE_REQUESTTRIBE,
		&index,
		sizeof( index ) );

	NETFunc::STATUS err = NETFunc::Send(
		netfunc_Get()->GetDP(),
		&message,
		0 );
	Assert( err == NETFunc::OK );
}























































BOOL AllinoneWindow::LoadGUIDs(SaveInfo *info)
{
	int res;
	res = dpSetPlayerData(
		netfunc_Get()->GetDP(),
		netfunc_Get()->GetPlayer()->GetId(),
		k_GUIDDATA_ID,
		&info->networkGUID[0],
		sizeof(CivGuid) * k_MAX_PLAYERS,
		0);
	Assert(res == dp_RES_OK);
	if(res != dp_RES_OK) return FALSE;
	return TRUE;
}

BOOL AllinoneWindow::SetScenarioInfo(SaveInfo *info)
{
	int res;

	m_scenInfo.isScenario = (uint8)info->isScenario;
	m_scenInfo.m_haveSavedGame = TRUE;
	strlcpy(m_scenInfo.m_fileName, info->fileName, sizeof(m_scenInfo.m_fileName));
	strlcpy(m_scenInfo.m_gameName, scenario_name_buf(), sizeof(m_scenInfo.m_gameName));
	if(info->isScenario && !info->scenarioName.empty()) {
		strlcpy(m_scenInfo.m_scenarioName, info->scenarioName.c_str(), sizeof(m_scenInfo.m_scenarioName));
		ScenarioPack *pack;
		Scenario *scen;
		if(!civscenarios_Get()->FindScenario(m_scenInfo.m_scenarioName,
										 &pack, &scen)) {
			return FALSE;
		}
	} else {
		m_scenInfo.m_scenarioName[0] = 0;
	}

	sint32 i;
	for(i = 0; i < k_MAX_START_POINTS; i++) {
		m_scenInfo.m_civs[i] = (uint8)info->positions[i].civIndex;
	}
	m_scenInfo.m_numStartPositions = (uint8)info->numPositions;
	m_scenInfo.m_startInfoType = (uint8)info->startInfoType;

	for(i = 0; i < k_MAX_PLAYERS; i++) {
		if(info->playerCivIndexList[i] != 0) {
			m_scenInfo.m_legalCivs[i] = info->playerCivIndexList[i];
		} else {

			m_scenInfo.m_legalCivs[i] = 0;
		}
	}

	res = dpSetPlayerData(
		netfunc_Get()->GetDP(),
		netfunc_Get()->GetPlayer()->GetId(),
		k_SCENARIO_INFO_ID,
		&m_scenInfo,
		sizeof(m_scenInfo),
		0);

	if(m_scenInfo.isScenario) {
		m_controls[ CONTROL_ADDAIBUTTON ]->Enable( true );

		if((m_scenInfo.m_startInfoType == (uint8)STARTINFOTYPE_CIVS) ||
		   (m_scenInfo.m_startInfoType == (uint8)STARTINFOTYPE_CIVSFIXED)) {


			if(m_scenInfo.m_numStartPositions < k_NS_MAX_HUMANS) {

				gamesetup_Get().SetSize(m_scenInfo.m_numStartPositions);
				UpdateGameSetup();
			}
		}
	}
	Assert(res == dp_RES_OK);
	if(res != dp_RES_OK) return FALSE;
	return TRUE;
}

void AllinoneWindow::SetupNewScenario()
{
	m_scenInfo.isScenario = TRUE;
	strlcpy(m_scenInfo.m_gameName, scenario_name_buf(), sizeof(m_scenInfo.m_gameName));
	m_scenInfo.m_fileName[0] = 0;
	m_scenInfo.m_startInfoType = STARTINFOTYPE_NONE;
	m_scenInfo.m_haveSavedGame = FALSE;
	m_scenInfo.m_scenarioName[0] = 0;

	dp_result_t res = dpSetPlayerData(
		netfunc_Get()->GetDP(),
		netfunc_Get()->GetPlayer()->GetId(),
		k_SCENARIO_INFO_ID,
		&m_scenInfo,
		sizeof(m_scenInfo),
		0);
    Assert(dp_RES_OK == res);

	if(m_scenInfo.isScenario) {
		m_controls[ CONTROL_ADDAIBUTTON ]->Enable( true );

		if((m_scenInfo.m_startInfoType == (uint8)STARTINFOTYPE_CIVS) ||
		   (m_scenInfo.m_startInfoType == (uint8)STARTINFOTYPE_CIVSFIXED)) {


			if(m_scenInfo.m_numStartPositions < k_NS_MAX_HUMANS) {

				gamesetup_Get().SetSize(m_scenInfo.m_numStartPositions);
				UpdateGameSetup();
			}
		}
	}
}

































AUI_ERRCODE AllinoneWindow::Idle( )
{

	if ( m_mode == CREATE ||
		 m_mode == CONTINUE_CREATE )
	{
	bool n = true;
	NETFunc::KeyStruct gameKey;
	bool launchGame = false;
	bool joinedgame = false;

	while (n)
    {
		if (NETFunc::Message * m = netfunc_Get()->GetMessage())
        {
			netfunc_Get()->HandleMessage(m);

			switch (m->GetCode())
			{
			case NETFunc::Message::UNLAUNCH:
				EnableButtonsForUnlaunch();
				playersetup_Get().SetReadyToLaunch(false);
				break;
			case NETFunc::Message::NETWORKERR:
				passwordscreen_displayMyWindow( PASSWORDSCREEN_MODE_CONNECTIONERR );
				netshell_Get()->GotoScreen( NetShell::SCREEN_CONNECTIONSELECT );
				break;
			case dp_SESSIONLOST_PACKET_ID:
				passwordscreen_displayMyWindow( PASSWORDSCREEN_MODE_CONNECTIONLOST );
				netfunc_Get()->Leave();

				if ( s_dbw )
				{
					DialogBoxWindow::PopDown( s_dbw );
					s_dbw = nullptr;
				}
				netshell_Get()->GotoScreen( NetShell::SCREEN_LOBBY );
				((LobbyWindow *)netshell_Get()->FindWindow( NetShell::WINDOW_LOBBY ))->Update();
				break;

			default:
				break;
			}

			if ( m->GetCode() == NETFunc::Message::ADDAIPLAYER )
			{
				nf_AIPlayer aiplayer;
				aiplayer.Set( m->GetBodySize(), m->GetBody() );

				BOOL isFemale = FALSE;
				sint32 tribeToGet = FindTribe( 0, TRUE, &isFemale );

				BOOL success = AssignTribe(
					tribeToGet,
					*(uint16 *)aiplayer.GetKey()->buf,
					TRUE,
					isFemale,
					FALSE );
				Assert( success );
			}
			else if ( m->GetCode() == NETFunc::Message::DELAIPLAYER )
			{

				nf_AIPlayer aiplayer;
				aiplayer.Set( m->GetBodySize(), m->GetBody() );

				BOOL success = AssignTribe(
					0,
					*(uint16 *)aiplayer.GetKey()->buf,
					TRUE,
					FALSE,
					TRUE );
				Assert( success );
			}
			else if ( m->GetCode() == (NETFunc::Message::Code)CUSTOMCODE_REQUESTTRIBE )
			{
				ns_HPlayerItem *hplayer = GetHPlayerFromId( m->GetSender() );
				Assert( hplayer != nullptr );
				if ( hplayer )
				{
					uint16 key = *(uint16 *)hplayer->GetPlayer()->GetKey()->buf;

					sint32 index = *(sint32 *)m->GetBody();

					if ( index == 0 )
					{

						AssignTribe(
							0,
							key,
							FALSE,
							FALSE,
							TRUE );

						index = FindTribe( 0, FALSE );
					}

					uint16 curKey;
					BOOL curIsAI;
					BOOL curIsFemale;
					BOOL success = WhoHasTribe( index, &curKey, &curIsAI, &curIsFemale );
					Assert( success );

					if ( curKey == 0 )
					{


						BOOL success = AssignTribe(
							index,
							key,
							FALSE,
							FALSE,
							FALSE );
						Assert( success );
					}
					else if ( curKey != key )
					{

						sint32 curTribe = FindTribe( key, FALSE );

						BOOL success = AssignTribe(
							curTribe,
							key,
							FALSE,
							FALSE,
							FALSE );
						Assert( success );

						NETFunc::Message msg = NETFunc::Message(
							(NETFunc::Message::Code)CUSTOMCODE_REQUESTDENIED );

						NETFunc::STATUS err = NETFunc::Send(
							netfunc_Get()->GetDP(),
							&msg,
							m->GetSender() );
						Assert( err == NETFunc::OK );
					}
					else
					{

						Assert( curIsAI == FALSE );
					}
				}
			}
			else if(m->GetCode() == NETFunc::Message::ENTERGAME)
			{

				memcpy(
					&gameKey,
					(NETFunc::KeyStruct *)(m->GetBody()),
					sizeof(NETFunc::KeyStruct));
				m_joinedGame = joinedgame = true;

				m_scenInfo.isScenario = FALSE;
			}
			else if ( m->GetCode() == NETFunc::Message::PLAYERENTER )
			{






				m_shouldAddAIPlayer = 0;
			}
			else if(m->GetCode() == NETFunc::Message::GAMELAUNCH)
				launchGame = true;
			 else if(m->GetCode() == NETFunc::Message::PLAYERPACKET)
			 {
				 ns_HPlayerListBox *l = (ns_HPlayerListBox *)
					 FindControl( AllinoneWindow::CONTROL_HPLAYERSLISTBOX );
				 ns_HPlayerItem *item = (ns_HPlayerItem *)(l->GetSelectedItem());
				 if(item && !item->IsAI())
				 {
					 NETFunc::Player *player = item->GetPlayer();
					 PlayerEditWindow *p = (PlayerEditWindow *)netshell_Get()->
						 FindWindow( NetShell::WINDOW_PLAYEREDIT );
					 *(NETFunc::PlayerSetup *)&rplayersetup_Get() =
						 NETFunc::PlayerSetup(player);
					 rplayersetup_Get().Packet::Set(m->GetBodySize(), m->GetBody());
					 p->SetPlayerSetup(&rplayersetup_Get());
					 p->SetMode(PlayerEditWindow::VIEW);
					 netshell_Get()->GetCurrentScreen()->AddWindow(p, TRUE);
				 }
			}
			std::unique_ptr<NETFunc::Message>( m ).reset();
		} else
			n = false;
	}

	if(joinedgame && NETFunc::GetStatus() == NETFunc::OK) {
		PlayerSelectWindow *w = (PlayerSelectWindow *)netshell_Get()->
			FindWindow(NetShell::WINDOW_PLAYERSELECT);
		playersetup_Get() = *(w->GetPlayerSetup(netfunc_Get()->GetPlayer()));
		netfunc_Get()->SetPlayerSetup(&playersetup_Get());

		netfunc_Get()->PushChatMessage(m_messageGameCreate->GetString());

		NETFunc::EnumPlayers(false, &gameKey);

		((ns_GPlayerListBox *)FindControl( CONTROL_GPLAYERSLISTBOX ))->
			SetKey(&gameKey);

		((ns_ChatBox *)FindControl( CONTROL_CHATBOX ))->SetKey(&gameKey);

		NETFunc::EnumPlayers(true, &gameKey);

		((aui_TextField *)(FindControl(CONTROL_GAMENAMETEXTFIELD)))->
			SetFieldText(netfunc_Get()->GetSession()->GetName());

        std::unique_ptr<ns_Units>(nsunits_Get()).reset();              nsunits_Set(std::make_unique<ns_Units>().release());
        std::unique_ptr<ns_Improvements>(nsimprovements_Get()).reset(); nsimprovements_Set(std::make_unique<ns_Improvements>().release());
        std::unique_ptr<ns_Wonders>(nswonders_Get()).reset();          nswonders_Set(std::make_unique<ns_Wonders>().release());

		CreateExclusions();







		playersetup_Get().SetKey( netfunc_Get()->GetPlayer()->GetKey() );

		BOOL success = FALSE;
		if ( m_mode == CONTINUE_CREATE ||
			 (IsScenarioGame()))
		{
			SaveInfo *info = nullptr;
			BOOL deleteIt = FALSE;
			if(IsScenarioGame()) {
				ScenarioPack *pack;
				Scenario *scen;
				CivScenarios *cs = civscenarios_Get();
				if(cs->FindScenario(scenario_name_buf(),
												&pack, &scen)) {
					info = cs->LoadSaveInfo(scen);
					if(info)
						deleteIt = TRUE;
				}
				if(!info)
					SetupNewScenario();
			} else {
				info = loadsavewindow_Get()->GetSaveInfo();
			}

			if ( info )
			{
				LoadGUIDs(info);


				m_receivedGuids = true;
				memcpy(
					&m_civGuids,
					info->networkGUID,
					sizeof(CivGuid) * k_MAX_PLAYERS);

				sint32 i;
				for(i = 0; i < k_MAX_PLAYERS; i++) {
					if(m_civGuids[i].guid == *network_Get().GetGuid()) {
						success = AssignTribe(
							m_civGuids[i].civIndex + 1,
							*(uint16 *)playersetup_Get().GetKey()->buf,
							FALSE,
							FALSE,
							FALSE );
						Assert( success );
						break;
					}
				}

				for( i = 0; i < k_NS_MAX_PLAYERS; i++ )
				{
					if ( gamesetup_Get().GetSavedTribeSlots()[ i ].isAI )
						AddAIPlayer();
				}

				BOOL haveScenario = SetScenarioInfo(info);
				if(deleteIt)
					std::unique_ptr<SaveInfo>( info ).reset();
				if(!haveScenario) {
					if(netfunc_Get()->GetTransport()) {

						if ( s_dbw ) {
							DialogBoxWindow::PopDown( s_dbw );
							s_dbw = nullptr;
						}

						passwordscreen_displayMyWindow( PASSWORDSCREEN_MODE_SCENARIO_NOT_FOUND );

						aui_Button *cancel = (aui_Button *)
							m_controls[ CONTROL_CANCELBUTTON ];
						cancel->GetAction()->Execute(
													 cancel,
													 AUI_BUTTON_ACTION_EXECUTE,
													 0 );
					} else {
#ifdef WIN32
						PostMessage( aui_ui_Get()->TheHWND(), WM_CLOSE, 0, 0 );
#endif
				    }
				}
			}
		}

		if ( !success )
		{
			success = AssignTribe(
				FindTribe( 0, FALSE ),
				*(uint16 *)playersetup_Get().GetKey()->buf,
				FALSE,
				FALSE,
				FALSE );
			Assert( success );
		}





		UpdateDisplay();

		UpdatePlayerButtons();


		if ( s_dbw )
		{
			DialogBoxWindow::PopDown( s_dbw );
			s_dbw = nullptr;
		}
	}

	if(launchGame) {
		netfunc_Get()->PushChatMessage(m_messageLaunched->GetString());
		*(NETFunc::Session *)&gamesetup_Get() = *(NETFunc::Session *)&NETFunc::gameSetup;
		AllinoneWindow_SetupGameForLaunch();

		for ( sint32 child = aui_ui_Get()->ChildList()->L(); child; child-- )
			aui_ui_Get()->RemoveChild( aui_ui_Get()->ChildList()->GetHead()->Id() );
		NetShell::Leave(k_NS_FLAGS_LAUNCH | k_NS_FLAGS_DESTROYNETSHELL, TRUE);
	}

	if ( m_shouldUpdateGame )
	{
		uint32 tickNow = GetTickCount();
		if ( tickNow - m_tickGame > k_PACKET_DELAY )
		{
			ReallyUpdateGameSetup();

			m_tickGame = tickNow;
		}
	}
	if ( m_shouldAddAIPlayer )
	{

		if ( !OKToAddPlayers() )
			m_shouldAddAIPlayer = 0;
		else
			AddAIPlayer( m_shouldAddAIPlayer );
	}
	if ( m_shouldUpdateAIPlayer )
	{
		uint32 tickNow = GetTickCount();
		if ( tickNow - m_tickAIPlayer > k_PACKET_DELAY )
		{
			ReallyUpdateAIPlayerSetup();

			m_tickAIPlayer = tickNow;
		}
	}
	if ( m_joinedGame )
	{

		uint32 tickNow = GetTickCount();
		if ( tickNow - m_tickPlayer > k_PACKET_DELAY )
		{




			if ( (!gamesetup_Get().GetDynamicJoin() && CurNumHumanPlayers() < 2) ||
				 (gamesetup_Get().GetDynamicJoin() && ((ns_HPlayerListBox *)
			m_controls[ CONTROL_HPLAYERSLISTBOX ])->NumItems() < 2 )
				 || playersetup_Get().IsReadyToLaunch() )
			{
				if ( !m_controls[ CONTROL_OKBUTTON ]->IsDisabled() )
					m_controls[ CONTROL_OKBUTTON ]->Enable( false );
			}
			else
			{
				if ( m_controls[ CONTROL_OKBUTTON ]->IsDisabled() )
					m_controls[ CONTROL_OKBUTTON ]->Enable( true );
			}


			MBCHAR temp[ dp_SNAMELEN + 1 ];
			((aui_TextField *)m_controls[ CONTROL_GAMENAMETEXTFIELD ])->
				GetFieldText( temp, dp_SNAMELEN );
			if ( strncmp( gamesetup_Get().GetName(), temp, dp_SNAMELEN ) )
			{
				gamesetup_Get().SetName( temp );
				UpdateGameSetup();
			}

			m_tickPlayer = tickNow;
		}
	}
	}

	else if ( m_mode == JOIN ||
			  m_mode == CONTINUE_JOIN )
	{
	bool n = true;
	NETFunc::KeyStruct gameKey;
	bool becameHost = false;
	bool launchGame = false;
	bool playerKick = false;
	bool joinedgame = false;
	bool dontHaveScenario = false;
	static bool gotpacket = false;

	while (n)
    {
		if (NETFunc::Message * m = netfunc_Get()->GetMessage())
        {
			netfunc_Get()->HandleMessage(m);
			gamesetup_Get().Handle(m);

			switch ( m->GetCode() )
			{
			case NETFunc::Message::NETWORKERR:
				passwordscreen_displayMyWindow( PASSWORDSCREEN_MODE_CONNECTIONERR );
				netshell_Get()->GotoScreen( NetShell::SCREEN_CONNECTIONSELECT );
				break;
			case dp_SESSIONLOST_PACKET_ID:
				passwordscreen_displayMyWindow( PASSWORDSCREEN_MODE_CONNECTIONLOST );
				netfunc_Get()->Leave();

				if ( s_dbw )
				{
					DialogBoxWindow::PopDown( s_dbw );
					s_dbw = nullptr;
				}
				netshell_Get()->GotoScreen( NetShell::SCREEN_LOBBY );
				((LobbyWindow *)netshell_Get()->FindWindow( NetShell::WINDOW_LOBBY ))->Update();
				break;
			case NETFunc::Message::UNLAUNCH:
				if(playersetup_Get().IsReadyToLaunch())
					netfunc_Get()->PushChatMessage(m_messageGameSetup->GetString());
				EnableButtonsForUnlaunch();
				playersetup_Get().SetReadyToLaunch(false);
				break;
			default:
				break;
			}

			if ( m->GetCode() ==
				 (NETFunc::Message::Code)CUSTOMCODE_REQUESTDENIED )
			{
				netfunc_Get()->PushChatMessage(m_messageRequestDenied->GetString());

				if ( playersetup_Get().GetTribe() == 0 )
					RequestTribe( 0 );
			}
			else if ( m->GetCode() == dp_USER_PLAYERDATA_PACKET_ID )
			{
				dp_user_playerData_packet_t playerData =
					*(dp_user_playerData_packet_t *)m->GetBody();































				if(playerData.key == k_GUIDDATA_ID) {
					m_receivedGuids = true;
					memcpy(
						&m_civGuids,
						playerData.data,
						sizeof(CivGuid) * k_MAX_PLAYERS);

					if ( m_mode == CONTINUE_JOIN && m_joinedGame && gotpacket )
					{

						sint32 i;
						for(i = 0; i < k_MAX_PLAYERS; i++) {
							if(m_civGuids[i].guid == *network_Get().GetGuid()) {
								RequestTribe( m_civGuids[ i ].civIndex + 1 );
								break;
							}
						}

						if ( i == k_MAX_PLAYERS )
							RequestTribe( 0 );
					}
				} else if(playerData.key == k_SCENARIO_INFO_ID) {
					memcpy(&m_scenInfo, playerData.data, sizeof(ns_ScenarioInfo));

					SetScenarioGame(m_scenInfo.isScenario);

					if(m_scenInfo.isScenario) {







						ScenarioPack *pack;
						Scenario *scen;
						// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
						strlcpy(scenario_name_buf(), m_scenInfo.m_gameName, k_SCENARIO_NAME_MAX);
						if(civscenarios_Get()->FindScenario(scenario_name_buf(),
														&pack, &scen)) {

							civpaths_Get()->SetCurScenarioPath(scen->m_path);
							civpaths_Get()->SetCurScenarioPackPath(pack->m_path);
						} else {
							dontHaveScenario = true;
						}

						ns_GPlayerListBox *listbox = (ns_GPlayerListBox *)
							m_controls[ CONTROL_GPLAYERSLISTBOX ];
						listbox->EnableTribeButton(netfunc_Get()->GetPlayer());




						UpdateTribeSwitches();

					} else if(m_scenInfo.m_scenarioName[0] != 0) {
						// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
						strlcpy(scenario_name_buf(), m_scenInfo.m_scenarioName, k_SCENARIO_NAME_MAX);
						ScenarioPack *pack;
						Scenario *scen;
						if(civscenarios_Get()->FindScenario(scenario_name_buf(),
														&pack, &scen)) {
							civpaths_Get()->SetCurScenarioPath(scen->m_path);
							civpaths_Get()->SetCurScenarioPackPath(pack->m_path);
						} else {
							dontHaveScenario = true;
						}
					} else {
						scenario_name_buf()[0] = 0;
					}




					dpSetPlayerData(
									netfunc_Get()->GetDP(),
									netfunc_Get()->GetPlayer()->GetId(),
									k_SCENARIO_INFO_ID,
									&m_scenInfo,
									sizeof(m_scenInfo),
									0);
				}
			}
			else if(m->GetCode() == NETFunc::Message::ENTERGAME)
			{

				memcpy(
					&gameKey,
					(NETFunc::KeyStruct *)(m->GetBody()),
					sizeof(NETFunc::KeyStruct));
				m_joinedGame = joinedgame = true;

				gotpacket = false;
			}
			else if ( m->GetCode() == NETFunc::Message::ENTERLOBBY )
			{

				if ( s_dbw )
				{
					DialogBoxWindow::PopDown( s_dbw );
					s_dbw = nullptr;
				}

				passwordscreen_displayMyWindow( PASSWORDSCREEN_MODE_FULL );

				aui_Button *cancel = (aui_Button *)
					m_controls[ CONTROL_CANCELBUTTON ];
				cancel->GetAction()->Execute(
					cancel,
					AUI_BUTTON_ACTION_EXECUTE,
					0 );
			}
			else if(m->GetCode() == NETFunc::Message::GAMEHOST) {
				becameHost = true;
			} else if(m->GetCode() == NETFunc::Message::GAMELAUNCH) {
				launchGame = true;
			} else if(m->GetCode() == NETFunc::Message::PLAYERKICK) {
				playerKick = true;
				n = false;
			} else if(m->GetCode() == NETFunc::Message::GAMESESSION ) {
				((aui_TextField *)(FindControl(CONTROL_GAMENAMETEXTFIELD)))->
					SetFieldText(gamesetup_Get().GetName());
			} else if(m->GetCode() == NETFunc::Message::GAMEPACKET) {
				UpdateDisplay();
				if ( !gotpacket )
				{
					gotpacket = true;
				}
			} else if(m->GetCode() == NETFunc::Message::PLAYERPACKET)
			{
				ns_HPlayerListBox *l = (ns_HPlayerListBox *)
					FindControl( AllinoneWindow::CONTROL_HPLAYERSLISTBOX );
				ns_HPlayerItem *item = (ns_HPlayerItem *)(l->GetSelectedItem());
				if(item && !item->IsAI())
				{
					NETFunc::Player *player = item->GetPlayer();
					PlayerEditWindow *p = (PlayerEditWindow *)netshell_Get()->
						FindWindow( NetShell::WINDOW_PLAYEREDIT );
					*(NETFunc::PlayerSetup *)&rplayersetup_Get() =
						NETFunc::PlayerSetup(player);
					rplayersetup_Get().Packet::Set(m->GetBodySize(), m->GetBody());
					p->SetPlayerSetup(&rplayersetup_Get());
					p->SetMode(PlayerEditWindow::VIEW);
					netshell_Get()->GetCurrentScreen()->AddWindow(p, TRUE);
				}
			}
			std::unique_ptr<NETFunc::Message>( m ).reset();
		} else
			n = false;
		if(becameHost)
			break;
	}

	if(playerKick) {
		if(netfunc_Get()->GetTransport()) {

			netfunc_Get()->PushChatMessage(m_messageKicked->GetString());
			LobbyWindow *w = (LobbyWindow *)netshell_Get()->
				FindWindow( NetShell::WINDOW_LOBBY );
			netshell_Get()->GotoScreen( NetShell::SCREEN_LOBBY );
			w->Update();
		} else {
#ifdef WIN32
			PostMessage( aui_ui_Get()->TheHWND(), WM_CLOSE, 0, 0 );
#endif
		}

	}

	if(dontHaveScenario) {
		if(netfunc_Get()->GetTransport()) {

			if ( s_dbw ) {
				DialogBoxWindow::PopDown( s_dbw );
				s_dbw = nullptr;
			}

			passwordscreen_displayMyWindow( PASSWORDSCREEN_MODE_SCENARIO_NOT_FOUND );

			aui_Button *cancel = (aui_Button *)
				m_controls[ CONTROL_CANCELBUTTON ];
			cancel->GetAction()->Execute(
				cancel,
				AUI_BUTTON_ACTION_EXECUTE,
				0 );
		} else {
#ifdef WIN32
			PostMessage( aui_ui_Get()->TheHWND(), WM_CLOSE, 0, 0 );
#endif
	}
	}

	if(joinedgame && NETFunc::GetStatus() == NETFunc::OK) {

		PlayerSelectWindow *w = (PlayerSelectWindow *)netshell_Get()->
			FindWindow(NetShell::WINDOW_PLAYERSELECT);
		playersetup_Get() = *(w->GetPlayerSetup(netfunc_Get()->GetPlayer()));
		netfunc_Get()->SetPlayerSetup(&playersetup_Get());

		netfunc_Get()->PushChatMessage(m_messageGameEnter->GetString());

		NETFunc::EnumPlayers(false, &gameKey);

		((ns_GPlayerListBox *)FindControl( CONTROL_GPLAYERSLISTBOX ))->
			SetKey(&gameKey);

		((ns_ChatBox *)FindControl( CONTROL_CHATBOX ))->SetKey(&gameKey);

		NETFunc::EnumPlayers(true, &gameKey);

		((aui_TextField *)(FindControl(CONTROL_GAMENAMETEXTFIELD)))->
			SetFieldText(netfunc_Get()->GetSession()->GetName());
	}

	static bool didntdoyet = true;
	if ( !m_joinedGame ) didntdoyet = true;

	if ( didntdoyet && m_joinedGame && gotpacket )
	{

		didntdoyet = false;

        std::unique_ptr<ns_Units>(nsunits_Get()).reset();              nsunits_Set(std::make_unique<ns_Units>().release());
        std::unique_ptr<ns_Improvements>(nsimprovements_Get()).reset(); nsimprovements_Set(std::make_unique<ns_Improvements>().release());
        std::unique_ptr<ns_Wonders>(nswonders_Get()).reset();          nswonders_Set(std::make_unique<ns_Wonders>().release());

		CreateExclusions();




		playersetup_Get().SetKey( netfunc_Get()->GetPlayer()->GetKey() );
		if ( m_mode == CONTINUE_JOIN )
		{

			if ( m_receivedGuids )
			{
				sint32 i;

				for (i = 0; i < k_MAX_PLAYERS; i++) {
					if(m_civGuids[i].guid == *network_Get().GetGuid()) {
						RequestTribe( m_civGuids[ i ].civIndex + 1 );
						break;
					}
				}

				if ( i == k_MAX_PLAYERS )
					RequestTribe( 0 );
			}
		}
		else
		{
			RequestTribe( 0 );
		}







		UpdatePlayerButtons();


		if ( s_dbw )
		{
			DialogBoxWindow::PopDown( s_dbw );
			s_dbw = nullptr;
		}
	}

	if(becameHost) {





		nf_GameSetup *newGS = std::make_unique<nf_GameSetup>( gamesetup_Get() ).release();
		if ( newGS )
		{
			ns_GameSetupListBox *gslb = (ns_GameSetupListBox *)
				((ns_Window *)netshell_Get()->
				 FindWindow( NetShell::WINDOW_GAMESELECT ))->
				FindControl( GameSelectWindow::CONTROL_GAMENAMELISTBOX );
			gslb->InsertItem( newGS );
			gslb->SelectItem( gslb->FindItem( newGS ) );

			gamesetup_Get() = *gslb->FindItem( newGS )->
				GetNetShellObject()->GetNETFuncObject();

			nf_GameSetup temp = gamesetup_Get();
			memset(
				temp.GetTribeSlots(),
				0,
				k_NS_MAX_PLAYERS * sizeof( TribeSlot ) );

			gslb->ChangeItem( &temp );
		}

		netfunc_Get()->PushChatMessage(m_messageGameHost->GetString());





		NETFunc::EnumPlayers(false, netfunc_Get()->GetSession()->GetKey());

		((ns_GPlayerListBox *)FindControl( CONTROL_GPLAYERSLISTBOX ))->
			SetKey(netfunc_Get()->GetSession()->GetKey());

		((ns_ChatBox *)FindControl( CONTROL_CHATBOX ))->
			SetKey(netfunc_Get()->GetSession()->GetKey());

		NETFunc::EnumPlayers(true, netfunc_Get()->GetSession()->GetKey());
		switch ( m_mode )
		{
		case JOIN: SetMode( CREATE ); break;
		case CONTINUE_JOIN: SetMode( CONTINUE_CREATE ); break;
		default: Assert( FALSE ); break;
		}

		if ( !FindTribe( *(uint16 *)playersetup_Get().GetKey()->buf, FALSE ) )
		{
			BOOL success = AssignTribe(
				FindTribe( 0, FALSE ),
				*(uint16 *)playersetup_Get().GetKey()->buf,
				FALSE,
				FALSE,
				FALSE );
			Assert( success );
		}

		UpdateDisplay();
	}

	if(!becameHost && launchGame) {
		netfunc_Get()->PushChatMessage(m_messageLaunched->GetString());
		*(NETFunc::Session *)&gamesetup_Get() = *(NETFunc::Session *)&NETFunc::gameSetup;
		AllinoneWindow_SetupGameForLaunch();

		for ( sint32 child = aui_ui_Get()->ChildList()->L(); child; child-- )
			aui_ui_Get()->RemoveChild( aui_ui_Get()->ChildList()->GetHead()->Id() );
		NetShell::Leave(k_NS_FLAGS_LAUNCH | k_NS_FLAGS_DESTROYNETSHELL, TRUE);
	}
	}

	return AUI_ERRCODE_OK;
}


void AllinoneWindow::UpdateDisplay( )
{
	((aui_TextField *)FindControl(CONTROL_GAMENAMETEXTFIELD))->
		SetFieldText(gamesetup_Get().GetName());

	((aui_DropDown *)m_controls[ CONTROL_PLAYSTYLEDROPDOWN ])->
		SetSelectedItem( gamesetup_Get().GetPlayStyle() );
	((ctp2_Spinner *)m_controls[ CONTROL_PLAYSTYLEVALUESPINNER ])->
		SetValue( gamesetup_Get().GetPlayStyleValue(), 10 );

	((aui_Switch *)m_controls[ CONTROL_DYNAMICJOINSWITCH ])->SetState(
		gamesetup_Get().GetDynamicJoin() );




	((aui_Switch *)m_controls[ CONTROL_HANDICAPPINGSWITCH ])->SetState(
		gamesetup_Get().GetHandicapping() );

	((aui_Switch *)m_controls[ CONTROL_BLOODLUSTSWITCH ])->SetState(
		gamesetup_Get().GetBloodlust() );






	((aui_Switch *)m_controls[ CONTROL_POLLUTIONSWITCH ])->SetState(
		gamesetup_Get().GetPollution() );
	((c3_EditButton *)m_controls[ CONTROL_CIVPOINTSBUTTON ])->SetValue(
		gamesetup_Get().GetCivPoints() );
	((c3_EditButton *)m_controls[ CONTROL_PWPOINTSBUTTON ])->SetValue(
		gamesetup_Get().GetPwPoints() );

	((aui_Switch *)m_controls[ CONTROL_LOCKSWITCH ])->SetState(
		gamesetup_Get().IsClosed() );






	agesscreen_setStartAge( gamesetup_Get().GetStartAge() );
	agesscreen_setEndAge( gamesetup_Get().GetEndAge() );
	spnewgamemapsizescreen_setMapSizeIndex( gamesetup_Get().GetMapSize() );
	spnewgamemapshapescreen_setMapShapeIndex( gamesetup_Get().GetWorldShape() );

	UpdateTribeSwitches();

	custommapscreen_setValues(
		gamesetup_Get().GetWorldType1(),
		gamesetup_Get().GetWorldType2(),
		gamesetup_Get().GetWorldType3(),
		gamesetup_Get().GetWorldType4(),
		gamesetup_Get().GetWorldType5(),
		gamesetup_Get().GetWorldType6() );
	spnewgamediffscreen_setDifficulty1( gamesetup_Get().GetDifficulty1() );
	spnewgamediffscreen_setDifficulty2( gamesetup_Get().GetDifficulty2() );

	if ( !m_createdExclusions ) CreateExclusions();

	m_numAvailUnits = gamesetup_Get().GetNumAvailUnits();
	sint32 i;
	for ( i = 0; i < m_numAvailUnits; i++ )
		if ( m_units[ i ] ) m_units[ i ]->SetState( gamesetup_Get().GetUnit( i ) );

	m_numAvailImprovements = gamesetup_Get().GetNumAvailImprovements();
	for ( i = 0; i < m_numAvailImprovements; i++ )
		m_improvements[ i ]->SetState( gamesetup_Get().GetImprovement( i ) );

	m_numAvailWonders = gamesetup_Get().GetNumAvailWonders();
	for ( i = 0; i < m_numAvailWonders; i++ )
		m_wonders[ i ]->SetState( gamesetup_Get().GetWonder( i ) );

	if ( !gamesetup_Get().GetHandicapping() )
	{
		ns_HPlayerListBox *hplayerslistbox = (ns_HPlayerListBox *)
			m_controls[ CONTROL_HPLAYERSLISTBOX ];

		for ( sint32 i = 0; i < hplayerslistbox->NumItems(); i++ )
		{
			ns_HPlayerItem *item = (ns_HPlayerItem *)hplayerslistbox->
				GetItemByIndex( i );
			if ( item->IsAI() )
			{
				if ( NETFunc::IsHost() )
				{
					item->GetCivpointsButton()->Enable( false );
					item->GetPwpointsButton()->Enable( false );
				}
			}
			else
			{
				if ( IsMine( item->GetPlayer() ) )
				{
					item->GetCivpointsButton()->Enable( false );
					item->GetPwpointsButton()->Enable( false );
				}
			}
		}
	}

	if ( !playersetup_Get().IsReadyToLaunch() )
		m_controls[ CONTROL_OKBUTTON ]->Enable( TRUE );




}


void AllinoneWindow::UpdateTribeSwitches( )
{
	TribeSlot *tribeSlots = gamesetup_Get().GetTribeSlots();

	if ( !NETFunc::IsHost() )
	{
		for ( sint32 i = 0; i < k_NS_MAX_PLAYERS; i++ )
		{
			if ( !tribeSlots[ i ].isAI )
			if ( tribeSlots[ i ].key == *(uint16 *)playersetup_Get().GetKey()->buf )
			{
				if ( playersetup_Get().GetTribe() != tribeSlots[ i ].tribe )
				{
					playersetup_Get().SetTribe( tribeSlots[ i ].tribe );
					UpdatePlayerSetup();
				}

				break;
			}
		}
	}

	spnewgametribescreen_clearTribes();
	if(IsScenarioGame())
	{
		if(m_scenInfo.m_haveSavedGame)
		{
			for(int m_legalCiv : m_scenInfo.m_legalCivs)
			{
				if(m_legalCiv != 0)
				{
					spnewgametribescreen_addTribeNoDuplicate(m_legalCiv);
				}
			}
			return;
		}
	}

	spnewgametribescreen_addAllTribes();
	sint32 i;
	for ( i = 0; i < k_NS_MAX_PLAYERS; i++ )
	{
		if ( tribeSlots[ i ].tribe != playersetup_Get().GetTribe() )
			spnewgametribescreen_removeTribe( tribeSlots[ i ].tribe - 1 );
	}


	if ( m_mode == CONTINUE_CREATE || m_mode == CONTINUE_JOIN)
	{
		if(m_scenInfo.isScenario &&

		   ((m_scenInfo.m_startInfoType == uint8(STARTINFOTYPE_NOLOCS)) ||
			(m_scenInfo.m_startInfoType == uint8(STARTINFOTYPE_POSITIONSFIXED))))

			return;

		for ( i = 2; i < nstribes_Get()->GetNumTribes(); i++ )
		{
			bool found = false;
			sint32 j;
			if(m_scenInfo.isScenario)
			{
				for(j = 0; j < m_scenInfo.m_numStartPositions; j++)
				{
					if(i - 1 == m_scenInfo.m_civs[j])
					{
						found = true;
						break;
					}
				}
			} else {
				for ( j = 0; j < k_NS_MAX_PLAYERS; j++ )
				{
					if ( i == gamesetup_Get().GetSavedTribeSlots()[ j ].tribe )
					{
						found = true;
						break;
					}
				}
			}

			if ( !found )
				spnewgametribescreen_removeTribe( i - 1 );
		}
	}
}


void AllinoneWindow::UpdateConfig( )
{


	ns_HPlayerListBox *hplayerslistbox = (ns_HPlayerListBox *)
		m_controls[ CONTROL_HPLAYERSLISTBOX ];

	aui_Switch *hand = (aui_Switch *)m_controls[ CONTROL_HANDICAPPINGSWITCH ];
	aui_Control *cpspinner = m_controls[ CONTROL_CIVPOINTSBUTTON ];
	aui_Control *pwspinner = m_controls[ CONTROL_PWPOINTSBUTTON ];
	aui_Static *sheet = (aui_Static *)m_controls[ CONTROL_RULESSHEET ];

#ifdef LOCKSETTINGSONLAUNCH
	if ( hand->IsOn() && !playersetup_Get().IsReadyToLaunch() )
#else
	if ( hand->IsOn() )
#endif
	{
		sheet->RemoveChild( m_controls[ CONTROL_GOLDSTATICTEXT ]->Id() );
		sheet->RemoveChild( cpspinner->Id() );
		sheet->RemoveChild( m_controls[ CONTROL_PWSTATICTEXT ]->Id() );
		sheet->RemoveChild( pwspinner->Id() );

		for ( sint32 i = 0; i < hplayerslistbox->NumItems(); i++ )
		{
			ns_HPlayerItem *item = (ns_HPlayerItem *)hplayerslistbox->
				GetItemByIndex( i );
			if ( item->IsAI() )
			{
				if ( NETFunc::IsHost() )
				{
					item->GetCivpointsButton()->Enable( true );
					item->GetPwpointsButton()->Enable( true );
				}
			}
			else
			{
				if ( IsMine( item->GetPlayer() ) )
				{
					item->GetCivpointsButton()->Enable( true );
					item->GetPwpointsButton()->Enable( true );
				}
			}
		}
	}
	else
	{
		sheet->AddChild( m_controls[ CONTROL_GOLDSTATICTEXT ] );
		sheet->AddChild( cpspinner );
		sheet->AddChild( m_controls[ CONTROL_PWSTATICTEXT ] );
		sheet->AddChild( pwspinner );

		if ( !sheet->IsHidden() )
		{
			m_controls[ CONTROL_GOLDSTATICTEXT ]->Show();
			cpspinner->Show();
			m_controls[ CONTROL_PWSTATICTEXT ]->Show();
			pwspinner->Show();
		}

		for ( sint32 i = 0; i < hplayerslistbox->NumItems(); i++ )
		{
			ns_HPlayerItem *item = (ns_HPlayerItem *)hplayerslistbox->
				GetItemByIndex( i );
			if ( item->IsAI() )
			{
				if ( NETFunc::IsHost() )
				{
					item->GetCivpointsButton()->Enable( false );
					item->GetPwpointsButton()->Enable( false );
				}
			}
			else
			{
				if ( IsMine( item->GetPlayer() ) )
				{
					item->GetCivpointsButton()->Enable( false );
					item->GetPwpointsButton()->Enable( false );
				}
			}
		}
	}

	aui_DropDown *playstyle = (aui_DropDown *)
		m_controls[ CONTROL_PLAYSTYLEDROPDOWN ];
	aui_Control *psspinner = m_controls[ CONTROL_PLAYSTYLEVALUESPINNER ];
	aui_Control *psstatic = m_controls[ CONTROL_PLAYSTYLEVALUESTATICTEXT ];

	sint32 index = playstyle->GetSelectedItem();
	Assert( index >= 0 && index < m_playStyleValueStrings->GetNumStrings());
	if ( index < 0 || index >= m_playStyleValueStrings->GetNumStrings() )
		return;

	MBCHAR *string = m_playStyleValueStrings->GetString( index );


	if ( strlen( string ) )
	{
		m_controls[ CONTROL_PLAYERSSHEET ]->AddChild( psspinner );
		m_controls[ CONTROL_PLAYERSSHEET ]->AddChild( psstatic );
		psstatic->SetText( string );
	}
	else
	{
		m_controls[ CONTROL_PLAYERSSHEET ]->RemoveChild( psspinner->Id() );
		m_controls[ CONTROL_PLAYERSSHEET ]->RemoveChild( psstatic->Id() );
	}

	ShouldDraw();
	if ( g_rulesWindow ) g_rulesWindow->ShouldDraw();
}


void AllinoneWindow::UpdateGameSetup(bool b)
{

	if ( NETFunc::IsHost() )
	{
		if(b)
			NETFunc::UnLaunchAll();

		m_shouldUpdateGame = true;

		uint32 tickNow = GetTickCount();

		if ( tickNow - m_tickGame > k_PACKET_DELAY )
		{
			ReallyUpdateGameSetup();
		}

		m_tickGame = tickNow;
	}
}


void AllinoneWindow::ReallyUpdateGameSetup()
{

	gamesetup_Get().Update();

	GameSelectWindow *gsw = (GameSelectWindow *)netshell_Get()->
		FindWindow( NetShell::WINDOW_GAMESELECT );
	ns_GameSetupListBox *gl = (ns_GameSetupListBox *)gsw->
		FindControl( GameSelectWindow::CONTROL_GAMENAMELISTBOX );

	nf_GameSetup temp = gamesetup_Get();
	memset( temp.GetTribeSlots(), 0, k_NS_MAX_PLAYERS * sizeof( TribeSlot ) );
	memset( temp.GetSavedTribeSlots(), 0, k_NS_MAX_PLAYERS * sizeof( TribeSlot ) );

	gl->ChangeItem( &temp );

	m_shouldUpdateGame = false;
}


void AllinoneWindow::UpdatePlayerSetup()
{
	m_shouldUpdatePlayer = true;

	playersetup_Get().Update();

	ns_PlayerSetupListBox *l = (ns_PlayerSetupListBox *)
		((ns_Window *)netshell_Get()->
		 FindWindow( NetShell::WINDOW_PLAYERSELECT ))->
		FindControl( PlayerSelectWindow::CONTROL_PLAYERNAMELISTBOX );

	if( l->FindItem( &playersetup_Get() ) )
		l->ChangeItem( &playersetup_Get() );

	m_shouldUpdatePlayer = false;
}


void AllinoneWindow::UpdateAIPlayerSetup( nf_AIPlayer *aiplayer )
{

	if ( NETFunc::IsHost() )
	{

		if ( !m_aiplayerList->Find( aiplayer ) )
			m_aiplayerList->AddTail( aiplayer );

		m_shouldUpdateAIPlayer = true;

		uint32 tickNow = GetTickCount();
		if ( tickNow - m_tickAIPlayer > k_PACKET_DELAY )
		{
			ReallyUpdateAIPlayerSetup();
		}

		m_tickAIPlayer = tickNow;
	}
}


void AllinoneWindow::ReallyUpdateAIPlayerSetup( )
{
	for ( sint32 i = m_aiplayerList->L(); i; i-- )
	{
		nf_AIPlayer *aiplayer = m_aiplayerList->RemoveHead();
		aiplayer->Pack();
		netfunc_Get()->ChangeAIPlayer( aiplayer );
	}

	m_shouldUpdateAIPlayer = false;
}


void AllinoneWindow::DeleteAIPlayer( nf_AIPlayer *player )
{
	ListPos pos = m_aiplayerList->Find( player );
	if ( pos )
	{
		m_aiplayerList->DeleteAt( pos );
		if ( !m_aiplayerList->L() ) m_shouldUpdateAIPlayer = false;
	}
}


sint32 AllinoneWindow::CurNumHumanPlayers( )
{

	return netfunc_Get()->players.size();
}


sint32 AllinoneWindow::CurNumAiPlayers( )
{
	ns_AIPlayerListBox *aiplistbox = (ns_AIPlayerListBox *)
		m_controls[ CONTROL_AIPLAYERSLISTBOX ];

	return aiplistbox->NumItems();
}


sint32 AllinoneWindow::CurNumPlayers( )
{
	return CurNumHumanPlayers() + CurNumAiPlayers();
}


sint32 AllinoneWindow::OKToAddPlayers( )
{
	sint32 curNumPlayers = CurNumHumanPlayers();


	if ( curNumPlayers == 0 ) return 1;





	sint32 numPlayers = curNumPlayers + CurNumAiPlayers();

	if ( numPlayers < 0 || numPlayers > k_NS_MAX_PLAYERS ) return 0;

	if ( numPlayers == k_NS_MAX_PLAYERS ) return 0;


	if(m_scenInfo.isScenario &&
		((m_scenInfo.m_startInfoType != (uint8)STARTINFOTYPE_NONE &&
		  m_scenInfo.m_startInfoType != (uint8)STARTINFOTYPE_NOLOCS))) {
		if(numPlayers >= m_scenInfo.m_numStartPositions)
			return 0;
	}

	ns_HPlayerListBox *hplistbox = (ns_HPlayerListBox *)
		m_controls[ CONTROL_HPLAYERSLISTBOX ];
	if ( hplistbox->NumItems() >= k_NS_MAX_PLAYERS ) return 0;

	return numPlayers;
}


void AllinoneWindow::AddAIPlayer( sint32 curCount )
{


	if ( !curCount )
	{


		m_shouldAddAIPlayer++;
	}

	if ( !OKToAddPlayers() ) return;

	sint16 newMaxNumHumans = gamesetup_Get().GetMaxPlayers();
	sint32 maxPlayersTotal = k_NS_MAX_PLAYERS;
	if(m_scenInfo.isScenario &&
	   (m_scenInfo.m_startInfoType != (uint8)STARTINFOTYPE_NONE &&
	    m_scenInfo.m_startInfoType != (uint8)STARTINFOTYPE_NOLOCS)) {
		if(m_scenInfo.m_numStartPositions < k_NS_MAX_PLAYERS)
			maxPlayersTotal = m_scenInfo.m_numStartPositions;
	}

	if ( maxPlayersTotal - CurNumAiPlayers() <= k_NS_MAX_HUMANS )
		--newMaxNumHumans;
	Assert( newMaxNumHumans > 0 );
	if ( !newMaxNumHumans ) newMaxNumHumans = 1;
	gamesetup_Get().SetSize(newMaxNumHumans);
	UpdateGameSetup();

	static NETFunc::KeyStruct key;
	static bool firstTime = true;
	if ( firstTime )
	{
		firstTime = false;
		memset( &key, 0, sizeof( NETFunc::KeyStruct ) );
		key.len = 1;
	}

	if( key.buf[ key.len - 1 ] == (char) 255u )
		key.len++;
	key.buf[ key.len - 1 ]++;

	nf_AIPlayer *aiplayer = std::make_unique<nf_AIPlayer>().release();
	aiplayer->SetKey( &key );
	aiplayer->SetName( "--" );

	if ( !gamesetup_Get().GetHandicapping() )
	{
		aiplayer->SetCivPoints( gamesetup_Get().GetCivPoints() );
		aiplayer->SetPwPoints( gamesetup_Get().GetPwPoints() );
	}

	aiplayer->Pack();

	netfunc_Get()->InsertAIPlayer( aiplayer );

	m_shouldAddAIPlayer--;
}


void AllinoneWindow::Update()
{
	ns_GPlayerListBox *list = (ns_GPlayerListBox *)
		FindControl( AllinoneWindow::CONTROL_GPLAYERSLISTBOX );
	list->Destroy();

	((aui_TextField *)FindControl(CONTROL_GAMENAMETEXTFIELD))->SetFieldText("");


	if ( !s_dbw )
	{
		aui_Action *actions[] = { m_dbActionArray[0].get() };
		switch ( m_mode )
		{
		case CREATE:
		case CONTINUE_CREATE:
			s_dbw = DialogBoxWindow::PopUp(
				"createdialogboxwindow",
				actions );
			break;

		case JOIN:
		case CONTINUE_JOIN:
			s_dbw = DialogBoxWindow::PopUp(
				"joindialogboxwindow",
				actions );
			break;

		default:

			Assert( FALSE );
			break;
		}
	}
}


AUI_ERRCODE AllinoneWindow::SetParent( aui_Region *region )
{

	if ( region ) {
		ns_ChatBox *chatbox = (ns_ChatBox *)FindControl( CONTROL_CHATBOX );
		chatbox->SetText( "" );
		chatbox->GetInputField()->SetKeyboardFocus();


		RemoveChild( m_controls[ CONTROL_RULESSHEET ]->Id() );
		RemoveChild( m_controls[ CONTROL_EXCLUSIONSSHEET ]->Id() );
		AddChild( m_controls[ CONTROL_PLAYERSSHEET ] );

		m_joinedGame = false;
		m_receivedGuids = false;
		playersetup_Get().SetTribe( 0 );

		memset( m_lname, 0, sizeof( m_lname ) );




	}

	return ns_Window::SetParent(region);
}


void AllinoneWindow::PlayersListBoxAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != AUI_LISTBOX_ACTION_SELECT ) return;

	AllinoneWindow *w = g_allinoneWindow;
	if ( !w ) return;

	ns_HPlayerListBox *listbox = (ns_HPlayerListBox *)control;
	tech_WLList<sint32>	justSelectedList;
	tech_WLList<sint32>	justDeselectedList;
	listbox->WhatsChanged(justSelectedList,justDeselectedList);

	w->UpdatePlayerButtons();

	ns_ChatBox *chatbox = (ns_ChatBox *)w->
		FindControl(AllinoneWindow::CONTROL_CHATBOX);

	ListPos position = justDeselectedList.GetHeadPosition();
	sint32 i;
	for ( i = justDeselectedList.L(); i; i-- )
    {
		sint32 index = justDeselectedList.GetNext( position );
		ns_HPlayerItem *item = (ns_HPlayerItem *)listbox->GetItemByIndex(index);
		if ( !item->IsAI() )
		{
			NETFunc::Player *player = item->GetPlayer();
			if(chatbox->GetPlayer() && player->Equals(chatbox->GetPlayer()))
			{
				chatbox->SetPlayer(nullptr);
			}
		}
	}

	position = justSelectedList.GetHeadPosition();
	for ( i = justSelectedList.L(); i; i-- )
    {
		sint32 index = justSelectedList.GetNext( position );
		ns_HPlayerItem *item = (ns_HPlayerItem *)listbox->GetItemByIndex(index);
		if (item->IsAI())
		{
			chatbox->SetPlayer(nullptr);
		}
		else
		{
			NETFunc::Player *player = item->GetPlayer();
			chatbox->SetPlayer(player);
		}
	}

	if (!chatbox->GetPlayer() || chatbox->GetPlayer()->IsMe())
    {
		aui_Switch * s = (aui_Switch *) w->FindControl( AllinoneWindow::CONTROL_PPTSWITCH );
		s->SetState(k_PPT_PUBLIC);
	}

	justSelectedList.DeleteAll();
	justDeselectedList.DeleteAll();
}


void AllinoneWindow::UpdatePlayerButtons( )
{
	ns_HPlayerListBox *listbox = (ns_HPlayerListBox *)
		m_controls[ CONTROL_HPLAYERSLISTBOX ];
	ns_HPlayerItem *item = (ns_HPlayerItem *)listbox->GetSelectedItem();
	if ( item )
	{
		if ( item->IsAI() )
		{
			m_controls[ CONTROL_INFOBUTTON ]->Enable( false );
			m_controls[ CONTROL_PPTSWITCH ]->Enable( false );
		}
		else
		{
			m_controls[ CONTROL_INFOBUTTON ]->Enable( true );
			m_controls[ CONTROL_PPTSWITCH ]->Enable(!item->GetPlayer()->IsMe());
		}

		if ( NETFunc::IsHost() )
		{
			if ( item->IsAI() )
			{
				m_controls[ CONTROL_KICKBUTTON ]->Enable( true );
			}
			else
			{
		        m_controls[ CONTROL_KICKBUTTON ]->Enable(!item->GetPlayer()->IsMe());
			}
		}
	}
	else
	{
		m_controls[ CONTROL_INFOBUTTON ]->Enable( false );
		m_controls[ CONTROL_PPTSWITCH ]->Enable( false );

		if ( NETFunc::IsHost() )
		{
			m_controls[ CONTROL_KICKBUTTON ]->Enable( false );
		}
	}
}


void AllinoneWindow::GameNameTextFieldAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_TEXTFIELD_ACTION_EXECUTE ) return;

	AllinoneWindow *w = g_allinoneWindow;
	if ( !w ) return;

	char name[ dp_SNAMELEN + 1 ];
	((aui_TextField *)g_allinoneWindow->FindControl(
		AllinoneWindow::CONTROL_GAMENAMETEXTFIELD ))->
			GetFieldText(name, dp_SNAMELEN);

	gamesetup_Get().SetName(name);
	w->UpdateGameSetup();

	control->ReleaseKeyboardFocus();
}


void AllinoneWindow::PPTSwitchAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_SWITCH_ACTION_ON
	&&   action != (uint32)AUI_SWITCH_ACTION_OFF ) return;

	AllinoneWindow *w = g_allinoneWindow;
	if ( !w ) return;
	ns_ChatBox *chatbox = (ns_ChatBox *)(w->FindControl(AllinoneWindow::CONTROL_CHATBOX));

	aui_Switch *s = (aui_Switch *)control;

    switch (s->GetState())
	{
	case k_PPT_PUBLIC:
		chatbox->SetWhisper(false);
		chatbox->SetGroup(false);
		break;

	case k_PPT_PRIVATE:
		if(!chatbox->GetPlayer() || chatbox->GetPlayer()->IsMe())
		{
			s->SetState(k_PPT_PUBLIC);
		}
		else
		{
			chatbox->SetWhisper(true);
		}
		chatbox->SetGroup(false);
		break;






	default:

		Assert( FALSE );
		break;
	}

	s->SetText( w->m_PPTStrings->GetString( s->GetState() ) );
}

void AllinoneWindow::KickButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	Assert( netfunc_Get()->IsHost() );
	if ( !NETFunc::IsHost() ) return;

	AllinoneWindow *w = (AllinoneWindow *)control->GetParentWindow();
	ns_HPlayerListBox *listbox = (ns_HPlayerListBox *)w->
		FindControl( AllinoneWindow::CONTROL_HPLAYERSLISTBOX );
	ns_HPlayerItem *item = (ns_HPlayerItem *)listbox->GetSelectedItem();

	if( item )
	{
		if ( item->IsAI() )
		{

			sint16 newMaxNumHumans = gamesetup_Get().GetMaxPlayers();
			sint32 maxPlayersTotal = k_NS_MAX_PLAYERS;
			if(w->GetScenarioInfo()->isScenario &&
			   (w->GetScenarioInfo()->m_startInfoType != (uint8)STARTINFOTYPE_NONE &&
			    w->GetScenarioInfo()->m_startInfoType != (uint8)STARTINFOTYPE_NOLOCS)) {
				if(w->GetScenarioInfo()->m_numStartPositions < k_NS_MAX_PLAYERS)
					maxPlayersTotal = w->GetScenarioInfo()->m_numStartPositions;
			}

			if ( maxPlayersTotal - w->CurNumAiPlayers() < k_NS_MAX_HUMANS )
				++newMaxNumHumans;
			Assert( newMaxNumHumans <= k_NS_MAX_HUMANS );
			if ( newMaxNumHumans > k_NS_MAX_HUMANS ) newMaxNumHumans = k_NS_MAX_HUMANS;
			gamesetup_Get().SetSize(newMaxNumHumans);
			w->UpdateGameSetup();

			w->UpdatePlayerButtons();

			nf_AIPlayer *player = item->GetAIPlayer();

			w->DeleteAIPlayer( player );
			netfunc_Get()->DeleteAIPlayer( player );
		}
		else
		{
			NETFunc::Player *player = item->GetPlayer();
			if ( !w->IsMine( player ) )
			{
				NETFunc::Kick(player);

				w->UpdatePlayerButtons();
			}
		}
	}
}


void AllinoneWindow::InfoButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;
	AllinoneWindow *w = g_allinoneWindow;
	ns_HPlayerListBox *l = (ns_HPlayerListBox *)w->
		FindControl( AllinoneWindow::CONTROL_HPLAYERSLISTBOX );

	ns_HPlayerItem *item = (ns_HPlayerItem *)(l->GetSelectedItem());
	if(item && !item->IsAI() )
	{
		NETFunc::Player *player = item->GetPlayer();

		if ( w->IsMine( player ) )
		{
			PlayerEditWindow *p = (PlayerEditWindow *)netshell_Get()->
				FindWindow( NetShell::WINDOW_PLAYEREDIT );
			p->SetPlayerSetup(&playersetup_Get());
			p->SetMode(PlayerEditWindow::EDIT_GAMESETUP);
			netshell_Get()->GetCurrentScreen()->AddWindow(p, TRUE);
		}
		else
		{
			netfunc_Get()->GetPlayerSetupPacket(player);
		}
	}
}


void AllinoneWindow::OKButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	if ( playersetup_Get().IsReadyToLaunch() ) return;

	if (NETFunc::IsHost() && !c3files_HasLegalCD() )
	{
		static ns_String donthavecd( "strings.system.donthavecd" );
		netfunc_Get()->PushChatMessage( donthavecd.GetString() );
		return;
	}

	AllinoneWindow *w = g_allinoneWindow;

	if ( NETFunc::IsHost() && !gamesetup_Get().GetDynamicJoin() )
	{

		if ( w->CurNumHumanPlayers() <= 1 ||
			 ((aui_ListBox *)w->FindControl( CONTROL_GPLAYERSLISTBOX ))->
			 NumItems() <= 1 )
		{
			if ( !w->FindControl( CONTROL_GPLAYERSLISTBOX )->IsDisabled() )
				w->FindControl( CONTROL_GPLAYERSLISTBOX )->Enable( false );

			return;
		}
	}


	char name[ dp_SNAMELEN + 1 ];
	((aui_TextField *)g_allinoneWindow->FindControl(
		AllinoneWindow::CONTROL_GAMENAMETEXTFIELD ))->
			GetFieldText(name, dp_SNAMELEN);
	profiledb_Get()->SetGameName( name );















	profiledb_Get()->SetCivName( playersetup_Get().GetName() );


	if ( NETFunc::IsHost() )
	{
		sint32 playStyleValue = gamesetup_Get().GetPlayStyleValue();

		switch ( gamesetup_Get().GetPlayStyle() )
		{
		default:
			Assert( FALSE );

		case 0:
			network_Get().SetClassicStyle();
			break;
		case 1:
			network_Get().SetSpeedStyle( TRUE, playStyleValue, FALSE, 0 );

			profiledb_Get()->SetZoomedCombatAlways( FALSE );
			profiledb_Get()->SetThroneRoom( FALSE );

			break;
		case 2:
			network_Get().SetSpeedStyle(TRUE, playStyleValue, FALSE, 0);
			network_Get().SetCarryoverStyle( TRUE );
			break;
		case 3:
			network_Get().SetSpeedStyle(TRUE, playStyleValue, FALSE, playStyleValue);
			break;








		}
	}




	if ( NETFunc::IsHost() )
	{
		aui_Switch *ls = (aui_Switch *)w->FindControl( AllinoneWindow::CONTROL_LOCKSWITCH );

		if ( gamesetup_Get().GetDynamicJoin() )
		{

			ls->GetAction()->Execute( ls, AUI_SWITCH_ACTION_OFF, 0 );
		}
		else
		{

			ls->GetAction()->Execute( ls, AUI_SWITCH_ACTION_ON, 0 );
		}
	}

































	agesscreen_setStartAge( gamesetup_Get().GetStartAge() );
	agesscreen_setEndAge( gamesetup_Get().GetEndAge() );
	spnewgamemapsizescreen_setMapSizeIndex( gamesetup_Get().GetMapSize() );

	spnewgamemapshapescreen_setMapShapeIndex( gamesetup_Get().GetWorldShape() );


	spnewgametribescreen_setTribeIndex(
		playersetup_Get().GetTribe() - 1,
		strlen( w->m_lname ) ? w->m_lname : nullptr );

	custommapscreen_setValues(
		gamesetup_Get().GetWorldType1(),
		gamesetup_Get().GetWorldType2(),
		gamesetup_Get().GetWorldType3(),
		gamesetup_Get().GetWorldType4(),
		gamesetup_Get().GetWorldType5(),
		gamesetup_Get().GetWorldType6() );
	spnewgamediffscreen_setDifficulty1( gamesetup_Get().GetDifficulty1() );
	spnewgamediffscreen_setDifficulty2( gamesetup_Get().GetDifficulty2() );




	profiledb_Get()->SetGenocideRule(
		((aui_Switch *)g_allinoneWindow->FindControl(
			AllinoneWindow::CONTROL_BLOODLUSTSWITCH ))->GetState() );

























	profiledb_Get()->SetPollutionRule(
		((aui_Switch *)g_allinoneWindow->FindControl(
			AllinoneWindow::CONTROL_POLLUTIONSWITCH ))->GetState() );







	profiledb_Get()->SetPowerPoints(
		((c3_EditButton *)g_allinoneWindow->FindControl(
			AllinoneWindow::CONTROL_CIVPOINTSBUTTON ))->GetValue() );













	NETFunc::STATUS status = netfunc_Get()->Launch();
    Assert(NETFunc::OK == status);


	playersetup_Get().SetReadyToLaunch( true );
	control->Enable( FALSE );

#ifdef LOCKSETTINGSONLAUNCH

	if ( NETFunc::IsHost() )
	{

		uint8 isScenario = g_allinoneWindow->GetScenarioInfo()->isScenario;

		g_allinoneWindow->SetMode( g_allinoneWindow->GetMode() );
		g_allinoneWindow->GetScenarioInfo()->isScenario = isScenario;

		g_allinoneWindow->FindControl( AllinoneWindow::CONTROL_LOCKSWITCH )->
			Enable( TRUE );
		g_allinoneWindow->UpdatePlayerButtons();
	}

	ns_HPlayerListBox *hplistbox = (ns_HPlayerListBox *)
		g_allinoneWindow->FindControl( CONTROL_HPLAYERSLISTBOX );
	for ( sint32 i = 0; i < hplistbox->NumItems(); i++ )
	{
		ns_HPlayerItem *item = (ns_HPlayerItem *)hplistbox->GetItemByIndex( i );
		item->GetTribeButton()->Enable( FALSE );

		item->GetCivpointsButton()->Enable( FALSE );
		item->GetPwpointsButton()->Enable( FALSE );
	}
#endif

	static ns_String readytolaunch( "strings.system.readytolaunch" );
	netfunc_Get()->PushChatMessage( readytolaunch.GetString() );
}

void AllinoneWindow_SetupGameForLaunch( )
{

	sint32 numPlayers = ((aui_ListBox *)g_allinoneWindow->FindControl(
		AllinoneWindow::CONTROL_HPLAYERSLISTBOX))->NumItems();

	profiledb_Get()->SetNPlayers(numPlayers + 1);

	aui_ListBox *HPlayersList = (aui_ListBox *)g_allinoneWindow->FindControl(AllinoneWindow::CONTROL_HPLAYERSLISTBOX);
	sint32 i;
	for(i = 0; i < numPlayers; i++) {
		ns_HPlayerItem *item = (ns_HPlayerItem *)HPlayersList->GetItemByIndex(i);
		NETFunc::Player *nfPlayer;
		if(item->IsAI()) {
			nf_AIPlayer *aiPlayer = item->GetAIPlayer();
			Assert(aiPlayer);
			if(aiPlayer) {
				int tribe = aiPlayer->GetTribe();
				if(tribe == 0)
					tribe = CIV_INDEX_RANDOM;
				else
					tribe--;

				if(gamesetup_Get().GetHandicapping()) {
					network_Get().SetNSAIPlayerInfo(tribe,
												aiPlayer->GetGroup(),
												aiPlayer->GetCivPoints(),
												aiPlayer->GetPwPoints());
				} else {
					network_Get().SetNSAIPlayerInfo(tribe,
												aiPlayer->GetGroup(),
												gamesetup_Get().GetCivPoints(),
												gamesetup_Get().GetPwPoints());
				}

			}
		} else {
			nfPlayer = item->GetPlayer();
			nf_PlayerSetup playersetup(nfPlayer);

			int tribe = playersetup.GetTribe();
			if(tribe == 0)
				tribe = CIV_INDEX_RANDOM;
			else
				tribe--;

			if(gamesetup_Get().GetHandicapping()) {
				network_Get().SetNSPlayerInfo(nfPlayer->GetId(),
										  nfPlayer->GetName(),
										  tribe,
										  playersetup.GetGroup(),
										  playersetup.GetCivPoints(),
										  playersetup.GetPwPoints());
			} else {
				network_Get().SetNSPlayerInfo(nfPlayer->GetId(),
										  nfPlayer->GetName(),
										  tribe,
										  playersetup.GetGroup(),
										  gamesetup_Get().GetCivPoints(),
										  gamesetup_Get().GetPwPoints());
			}
		}
	}

	profiledb_Get()->SetAI(NETFunc::IsHost());

	network_Get().SetStartingAge(agesscreen_getStartAge());
	network_Get().SetEndingAge(agesscreen_getEndAge());
	network_Get().SetDynamicJoin(gamesetup_Get().GetDynamicJoin());
	ns_ScenarioInfo *scenInfo = g_allinoneWindow->GetScenarioInfo();

	if(scenInfo->isScenario) {
		scenario_civs_Set(numPlayers);
	}

	if(scenInfo->isScenario) {
		profiledb_Get()->SetIsScenario(TRUE);





		ScenarioPack *pack;
		Scenario *scen;
		// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
		strlcpy(scenario_name_buf(), scenInfo->m_gameName, k_SCENARIO_NAME_MAX);
		if(civscenarios_Get()->FindScenario(scenario_name_buf(),
										&pack, &scen)) {

			civpaths_Get()->SetCurScenarioPath(scen->m_path);
			civpaths_Get()->SetCurScenarioPackPath(pack->m_path);
		}
	} else {
		if(scenInfo->m_scenarioName[0] != 0) {
			// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
			strlcpy(scenario_name_buf(), scenInfo->m_scenarioName, k_SCENARIO_NAME_MAX);
			ScenarioPack *pack;
			Scenario *scen;
			if(civscenarios_Get()->FindScenario(scenario_name_buf(),
											&pack, &scen)) {

				profiledb_Get()->SetIsScenario(TRUE);
				civpaths_Get()->SetCurScenarioPath(scen->m_path);
				civpaths_Get()->SetCurScenarioPackPath(pack->m_path);
			}
		} else {
			profiledb_Get()->SetIsScenario(FALSE);
			civpaths_Get()->ClearCurScenarioPath();
		}
	}
}


























void AllinoneWindow::RulesButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	aui_ui_Get()->AddWindow( g_rulesWindow );
}


void AllinoneWindow::ExclusionsButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	aui_ui_Get()->AddWindow( g_exclusionsWindow );
}






























void AllinoneWindow::RulesOKButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	aui_ui_Get()->RemoveWindow( g_rulesWindow->Id() );
}


void AllinoneWindow::ExclusionsOKButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	aui_ui_Get()->RemoveWindow( g_exclusionsWindow->Id() );
}


void AllinoneWindow::CancelButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;
	if(netfunc_Get()->GetTransport()) {

		if ( NETFunc::IsHost() )
		{
			AllinoneWindow *w = g_allinoneWindow;

			w->AssignTribe(
				0,
				*(uint16 *)netfunc_Get()->GetPlayer()->GetKey()->buf,
				FALSE,
				FALSE,
				TRUE );

			ns_HPlayerListBox *hplistbox = (ns_HPlayerListBox *)
				w->FindControl( AllinoneWindow::CONTROL_HPLAYERSLISTBOX );
			for ( sint32 i = 0; i < hplistbox->NumItems(); i++ )
			{
				ns_HPlayerItem *item = (ns_HPlayerItem *)hplistbox->
					GetItemByIndex( i );
				nf_AIPlayer *aiplayer = item->GetAIPlayer();
				if ( aiplayer )
				{
					w->AssignTribe(
						0,
						*(uint16 *)aiplayer->GetKey()->buf,
						TRUE,
						FALSE,
						TRUE );

					w->DeleteAIPlayer( aiplayer );
					netfunc_Get()->DeleteAIPlayer( aiplayer );
				}
			}

			gamesetup_Get().SetClosed( FALSE );
			gamesetup_Get().SetSize( k_NS_MAX_HUMANS );

			std::this_thread::sleep_for(std::chrono::milliseconds(k_PACKET_DELAY));
			w->UpdateGameSetup();
		}
		netfunc_Get()->Leave();
		LobbyWindow *w = (LobbyWindow *)(netshell_Get()->FindWindow( NetShell::WINDOW_LOBBY ));
		netshell_Get()->GotoScreen( NetShell::SCREEN_LOBBY );
		w->Update();
	} else {
#ifdef WIN32
		PostMessage( aui_ui_Get()->TheHWND(), WM_CLOSE, 0, 0 );
#endif
}
}

void AllinoneWindow::ReviewButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	g_allinoneWindow->SpitOutGameSetup();
}




void AllinoneWindow::SpitOutGameSetup( )
{
	bool displayedSomething = false;

	const sint32 biglen = 2 << 15;
	static MBCHAR info[ biglen + 1 ];
	static MBCHAR moreinfo[ biglen + 1 ];
	static MBCHAR temp[ biglen + 1 ];


	memset( info, 0, sizeof( info ) );

	static ns_String customRules( "strings.customrules" );
	strncat( info, customRules.GetString(), biglen );
	strncat( info, "\n", biglen );



















	memset( moreinfo, 0, sizeof( moreinfo ) );
	sint32 i;
	for ( i = 0; i < m_numAvailUnits; i++ )
	{
		if ( m_units[ i ] && m_units[ i ]->GetState() )
		{
			strlcat( moreinfo, m_units[ i ]->GetText(), sizeof(moreinfo) );
			strlcat( moreinfo, ", ", sizeof(moreinfo) );
		}
	}
	sint32 len = strlen( moreinfo );
	if ( len > 2 )
	{
		static ns_String excludedUnitInfo( "strings.excludedunitinfo" );

		strlcpy( temp, excludedUnitInfo.GetString(), sizeof(temp) );

		moreinfo[ len - 2 ] = '\0';
		strlcat( temp, moreinfo, sizeof(temp) );

		strncat( info, temp, biglen );
		strncat( info, "\n", biglen );
		displayedSomething = true;
	}

	memset( moreinfo, 0, sizeof( moreinfo ) );
	for ( i = 0; i < m_numAvailImprovements; i++ )
	{
		if ( m_improvements[ i ]->GetState() )
		{
			strlcat( moreinfo, m_improvements[ i ]->GetText(), sizeof(moreinfo) );
			strlcat( moreinfo, ", ", sizeof(moreinfo) );
		}
	}
	len = strlen( moreinfo );
	if ( len > 2 )
	{
		static ns_String excludedImprovementInfo(
			"strings.excludedimprovementinfo" );

		strlcpy( temp, excludedImprovementInfo.GetString(), sizeof(temp) );

		moreinfo[ len - 2 ] = '\0';
		strlcat( temp, moreinfo, sizeof(temp) );

		strncat( info, temp, biglen );
		strncat( info, "\n", biglen );
		displayedSomething = true;
	}

	memset( moreinfo, 0, sizeof( moreinfo ) );
	for ( i = 0; i < m_numAvailWonders; i++ )
	{
		if ( m_wonders[ i ]->GetState() )
		{
			strlcat( moreinfo, m_wonders[ i ]->GetText(), sizeof(moreinfo) );
			strlcat( moreinfo, ", ", sizeof(moreinfo) );
		}
	}
	len = strlen( moreinfo );
	if ( len > 2 )
	{
		static ns_String excludedWonderInfo( "strings.excludedwonderinfo" );

		strlcpy( temp, excludedWonderInfo.GetString(), sizeof(temp) );

		moreinfo[ len - 2 ] = '\0';
		strlcat( temp, moreinfo, sizeof(temp) );

		strncat( info, temp, biglen );
		strncat( info, "\n", biglen );
		displayedSomething = true;
	}




	aui_Switch *sw = (aui_Switch *)m_controls[ CONTROL_DYNAMICJOINSWITCH ];
	if ( sw->GetState() )
	{
		static ns_String dynamicJoin( "strings.dynamicjoin" );
		strncat( info, dynamicJoin.GetString(), biglen );
		strncat( info, "\n", biglen );
		displayedSomething = true;
	}



















	sw = (aui_Switch *)m_controls[ CONTROL_HANDICAPPINGSWITCH ];
	if ( sw->GetState() )
	{
		static ns_String handicapping( "strings.handicapping" );
		strncat( info, handicapping.GetString(), biglen );
		strncat( info, "\n", biglen );
		displayedSomething = true;
	}
	else
	{
		c3_EditButton *r = (c3_EditButton *)m_controls[ CONTROL_CIVPOINTSBUTTON ];
		if ( r->GetValue() > r->GetMinimum() )
		{

			{
				static ns_String goldInfo( "strings.goldinfo" );
				snprintf(temp, sizeof(temp),
					"%s%d",
					goldInfo.GetString(),
					r->GetValue() );
				strncat( info, temp, biglen );
				strncat( info, "\n", biglen );
			}












			displayedSomething = true;
		}
		r = (c3_EditButton *)m_controls[ CONTROL_PWPOINTSBUTTON ];
		if ( r->GetValue() > r->GetMinimum() )
		{

			{
				static ns_String pwInfo( "strings.pwinfo" );
				snprintf(temp, sizeof(temp),
					"%s%d",
					pwInfo.GetString(),
					r->GetValue() );
				strncat( info, temp, biglen );
				strncat( info, "\n", biglen );
			}












			displayedSomething = true;
		}
	}










	sw = (aui_Switch *)m_controls[ CONTROL_BLOODLUSTSWITCH ];
	if ( sw->GetState() )
	{
		static ns_String bloodlust( "strings.bloodlust" );
		strncat( info, bloodlust.GetString(), biglen );
		strncat( info, "\n", biglen );
		displayedSomething = true;
	}































	sw = (aui_Switch *)m_controls[ CONTROL_POLLUTIONSWITCH ];
	if ( !sw->GetState() )
	{
		static ns_String pollution( "strings.pollution" );
		strncat( info, pollution.GetString(), biglen );
		strncat( info, "\n", biglen );
		displayedSomething = true;
	}




	{
		displayedSomething = true;
		AUI_ERRCODE errcode = AUI_ERRCODE_OK;

		static ns_String mapsize( "strings.mapsize" );
		static aui_StringTable mapsizestrings( &errcode, "strings.mapsizestrings" );
		snprintf(temp, sizeof(temp),
			"%s: %s\n",
			mapsize.GetString(),
			mapsizestrings.GetString( gamesetup_Get().GetMapSize() ) );
		strncat( info, temp, biglen );

		static ns_String worldtype1( "strings.worldtype1" );
		snprintf(temp, sizeof(temp),
			"%s%d\n",
			worldtype1.GetString(),
			gamesetup_Get().GetWorldType1() );
		strncat( info, temp, biglen );

		static ns_String worldtype2( "strings.worldtype2" );
		snprintf(temp, sizeof(temp),
			"%s%d\n",
			worldtype2.GetString(),
			gamesetup_Get().GetWorldType2() );
		strncat( info, temp, biglen );

		static ns_String worldtype3( "strings.worldtype3" );
		snprintf(temp, sizeof(temp),
			"%s%d\n",
			worldtype3.GetString(),
			gamesetup_Get().GetWorldType3() );
		strncat( info, temp, biglen );

		static ns_String worldtype4( "strings.worldtype4" );
		snprintf(temp, sizeof(temp),
			"%s%d\n",
			worldtype4.GetString(),
			gamesetup_Get().GetWorldType4() );
		strncat( info, temp, biglen );

		static ns_String worldtype5( "strings.worldtype5" );
		snprintf(temp, sizeof(temp),
			"%s%d\n",
			worldtype5.GetString(),
			gamesetup_Get().GetWorldType5() );
		strncat( info, temp, biglen );

		static ns_String worldtype6( "strings.worldtype6" );
		snprintf(temp, sizeof(temp),
			"%s%d\n",
			worldtype6.GetString(),
			gamesetup_Get().GetWorldType6() );
		strncat( info, temp, biglen );

		static ns_String worldshape( "strings.worldshape" );
		static aui_StringTable worldshapestrings( &errcode, "strings.worldshapestrings" );
		snprintf(temp, sizeof(temp),
			"%s: %s\n",
			worldshape.GetString(),
			worldshapestrings.GetString( gamesetup_Get().GetWorldShape() ) );
		strncat( info, temp, biglen );

		static ns_String difficulty1( "strings.difficulty1" );
		static aui_StringTable difficulty1strings( &errcode, "strings.difficulty1strings" );
		snprintf(temp, sizeof(temp),
			"%s: %s\n",
			difficulty1.GetString(),
			difficulty1strings.GetString( gamesetup_Get().GetDifficulty1() ) );
		strncat( info, temp, biglen );

		static ns_String difficulty2( "strings.difficulty2" );
		static aui_StringTable difficulty2strings( &errcode, "strings.difficulty2strings" );
		snprintf(temp, sizeof(temp),
			"%s: %s\n",
			difficulty2.GetString(),
			difficulty2strings.GetString( gamesetup_Get().GetDifficulty2() ) );
		strncat( info, temp, biglen );

		static ns_String startage( "strings.startage" );
		snprintf(temp, sizeof(temp),
			"%s %s\n",
			startage.GetString(),
			g_theAgeDB->Get(gamesetup_Get().GetStartAge())->GetNameText());
		strncat( info, temp, biglen );

		static ns_String endage( "strings.endage" );

		snprintf(temp, sizeof(temp),
			"%s %s\n",
			endage.GetString(),
			g_theAgeDB->Get(gamesetup_Get().GetEndAge())->GetNameText());
		strncat( info, temp, biglen );
	}

	if ( !displayedSomething )
	{
		static ns_String defaultGameSetup( "strings.defaultgamesetup" );
		strncat( info, defaultGameSetup.GetString(), biglen );
		strncat( info, "\n", biglen );
	}


	if ( len > 2 )
	{
		moreinfo[ len - 2 ] = '\0';
	}




	static aui_TextBase textStyle( "styles.system", (MBCHAR *)nullptr );
	((ns_ChatBox *)m_controls[ CONTROL_CHATBOX ])->
		AppendText( info, textStyle.GetTextColor(), FALSE, FALSE );
}


void AllinoneWindow::DialogBoxPopDownAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	netfunc_Get()->Leave();

	if ( s_dbw )
	{
		DialogBoxWindow::PopDown( s_dbw );
		s_dbw = nullptr;
	}
	LobbyWindow *w = (LobbyWindow *)(netshell_Get()->FindWindow( NetShell::WINDOW_LOBBY ));
	netshell_Get()->GotoScreen( NetShell::SCREEN_LOBBY );
	w->Update();
}


















































void AllinoneWindow::PlayStyleDropDownAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_DROPDOWN_ACTION_SELECT ) return;

	AllinoneWindow *w = g_allinoneWindow;
	if ( !w ) return;

	aui_DropDown *dropdown = (aui_DropDown *)w->
		FindControl( AllinoneWindow::CONTROL_PLAYSTYLEDROPDOWN );
	sint32 index = dropdown->GetListBox()->GetSelectedItemIndex();
	if ( index >= 0 )
	{

		w->UpdateConfig();


		if ( gamesetup_Get().GetPlayStyle() != index )
			((ctp2_Spinner *)w->FindControl(
				AllinoneWindow::CONTROL_PLAYSTYLEVALUESPINNER ))->
					SetValue( 0, 0 );

		{
			gamesetup_Get().SetPlayStyle( index );
			w->UpdateGameSetup(true);
		}
	}
}



























void AllinoneWindow::PlayStyleValueSpinnerCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if(action != AUI_RANGER_ACTION_VALUECHANGE)
		return;

	static bool inCallback = false;
	if(inCallback)
		return;
	inCallback = true;
	AllinoneWindow *w = g_allinoneWindow;
	if ( !w ) return;   /// @todo Check inCallback (this function is blocked forever)

	sint32 value = ((aui_Ranger *)control)->GetValueX();

	{
		gamesetup_Get().SetPlayStyleValue( value );
		w->UpdateGameSetup(true);
	}

	inCallback = false;
}


void AllinoneWindow::DynamicJoinSwitchAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_SWITCH_ACTION_ON
	&&   action != (uint32)AUI_SWITCH_ACTION_OFF ) return;


	AllinoneWindow *w = g_allinoneWindow;
	if ( !w ) return;

	BOOL join = ((aui_Switch *)control)->GetState();

	{
		gamesetup_Get().SetDynamicJoin( (char)join );
		w->UpdateGameSetup();
	}















}


















































void AllinoneWindow::HandicappingSwitchAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_SWITCH_ACTION_ON
	&&   action != (uint32)AUI_SWITCH_ACTION_OFF ) return;


	AllinoneWindow *w = g_allinoneWindow;
	if ( !w ) return;

	sint32 hand = ((aui_Switch *)control)->GetState();

	w->UpdateConfig();

		gamesetup_Get().SetHandicapping(static_cast<char>(hand));

	if ( !gamesetup_Get().GetHandicapping() )
	{
		playersetup_Get().SetCivPoints( gamesetup_Get().GetCivPoints() );
		playersetup_Get().SetPwPoints( gamesetup_Get().GetPwPoints() );
		w->UpdatePlayerSetup();

		if ( NETFunc::IsHost() )
		{
			ns_HPlayerListBox *hplistbox = (ns_HPlayerListBox *)w->
				FindControl( AllinoneWindow::CONTROL_HPLAYERSLISTBOX );

			for ( sint32 i = 0; i < hplistbox->NumItems(); i++ )
			{
				ns_HPlayerItem *item = (ns_HPlayerItem *)hplistbox->
					GetItemByIndex( i );

				if ( item->IsAI() )
				{
					nf_AIPlayer *aiplayer = item->GetAIPlayer();
					aiplayer->SetCivPoints( gamesetup_Get().GetCivPoints() );
					aiplayer->SetPwPoints( gamesetup_Get().GetPwPoints() );
					w->UpdateAIPlayerSetup( aiplayer );
				}
			}
		}
	}

		w->UpdateGameSetup(true);
}

























































































void AllinoneWindow::BloodlustSwitchAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_SWITCH_ACTION_ON
	&&   action != (uint32)AUI_SWITCH_ACTION_OFF ) return;


	AllinoneWindow *w = g_allinoneWindow;
	if ( !w ) return;

	sint32 bloodlust = ((aui_Switch *)control)->GetState();

	{
		gamesetup_Get().SetBloodlust(static_cast<char>(bloodlust));
		w->UpdateGameSetup(true);
	}
}

























































































void AllinoneWindow::PollutionSwitchAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_SWITCH_ACTION_ON
	&&   action != (uint32)AUI_SWITCH_ACTION_OFF ) return;


	AllinoneWindow *w = g_allinoneWindow;
	if ( !w ) return;

	sint32 poll = ((aui_Switch *)control)->GetState();

	{
		gamesetup_Get().SetPollution(static_cast<char>(poll));
		w->UpdateGameSetup(true);
	}
}


void AllinoneWindow::CivPointsButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;


	AllinoneWindow *w = g_allinoneWindow;
	if ( !w ) return;

	sint32 value = ((c3_EditButton *)control)->GetValue();

		gamesetup_Get().SetCivPoints( value );

	if ( !gamesetup_Get().GetHandicapping() )
	{
		playersetup_Get().SetCivPoints( gamesetup_Get().GetCivPoints() );
		w->UpdatePlayerSetup();

		if ( NETFunc::IsHost() )
		{
			ns_HPlayerListBox *hplistbox = (ns_HPlayerListBox *)w->
				FindControl( AllinoneWindow::CONTROL_HPLAYERSLISTBOX );

			for ( sint32 i = 0; i < hplistbox->NumItems(); i++ )
			{
				ns_HPlayerItem *item = (ns_HPlayerItem *)hplistbox->
					GetItemByIndex( i );

				if ( item->IsAI() )
				{
					nf_AIPlayer *aiplayer = item->GetAIPlayer();
					aiplayer->SetCivPoints( gamesetup_Get().GetCivPoints() );
					w->UpdateAIPlayerSetup( aiplayer );
				}
			}
		}
	}

		w->UpdateGameSetup(true);
}

void AllinoneWindow::PwPointsButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;


	AllinoneWindow *w = g_allinoneWindow;
	if ( !w ) return;

	sint32 value = ((c3_EditButton *)control)->GetValue();

		gamesetup_Get().SetPwPoints( value );

	if ( !gamesetup_Get().GetHandicapping() )
	{
		playersetup_Get().SetPwPoints( gamesetup_Get().GetPwPoints() );
		w->UpdatePlayerSetup();

		if ( NETFunc::IsHost() )
		{
			ns_HPlayerListBox *hplistbox = (ns_HPlayerListBox *)w->
				FindControl( AllinoneWindow::CONTROL_HPLAYERSLISTBOX );

			for ( sint32 i = 0; i < hplistbox->NumItems(); i++ )
			{
				ns_HPlayerItem *item = (ns_HPlayerItem *)hplistbox->
					GetItemByIndex( i );

				if ( item->IsAI() )
				{
					nf_AIPlayer *aiplayer = item->GetAIPlayer();
					aiplayer->SetPwPoints( gamesetup_Get().GetPwPoints() );
					w->UpdateAIPlayerSetup( aiplayer );
				}
			}
		}
	}

		w->UpdateGameSetup(true);
}





































































void AllinoneWindow::AgesButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	AllinoneWindow *w = g_allinoneWindow;

	agesscreen_displayMyWindow(
		w->GetMode() == JOIN ||
		w->GetMode() == CONTINUE_CREATE ||
		w->GetMode() == CONTINUE_JOIN ||
		( NETFunc::IsHost() && playersetup_Get().IsReadyToLaunch() ) );
}


void AllinoneAgesCallback(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void* cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	AllinoneWindow *w = g_allinoneWindow;
	if ( !w ) return;

	agesscreen_removeMyWindow( action );


	if ( w->GetMode() == AllinoneWindow::CREATE )
	{
		gamesetup_Get().SetStartAge(static_cast<char>(agesscreen_getStartAge()));
		gamesetup_Get().SetEndAge(static_cast<char>(agesscreen_getEndAge()));
		w->UpdateGameSetup(true);
	}
}


void AllinoneWindow::MapSizeButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	AllinoneWindow *w = g_allinoneWindow;

	spnewgamemapsizescreen_displayMyWindow(
		w->GetMode() == JOIN ||
		w->GetMode() == CONTINUE_CREATE ||
		w->GetMode() == CONTINUE_JOIN ||
		( NETFunc::IsHost() && playersetup_Get().IsReadyToLaunch() ),
		0 );
}


void AllinoneWindow::WorldShapeButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	AllinoneWindow *w = g_allinoneWindow;


	spnewgamemapshapescreen_displayMyWindow(
		w->GetMode() == JOIN ||
		w->GetMode() == CONTINUE_CREATE ||
		w->GetMode() == CONTINUE_JOIN ||
		( NETFunc::IsHost() && playersetup_Get().IsReadyToLaunch() ) );

}


void AllinoneMapSizeCallback(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void* cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	AllinoneWindow *w = g_allinoneWindow;
	if ( !w ) return;

	spnewgamemapsizescreen_removeMyWindow( action );


	if ( w->GetMode() == AllinoneWindow::CREATE )
	{
		gamesetup_Get().SetMapSize(static_cast<char>(spnewgamemapsizescreen_getMapSizeIndex()));
		w->UpdateGameSetup(true);
	}
}


void AllinoneWorldShapeCallback(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void* cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	AllinoneWindow *w = g_allinoneWindow;
	if ( !w ) return;

	spnewgamemapshapescreen_removeMyWindow( action );


	if ( w->GetMode() == AllinoneWindow::CREATE )
	{
		gamesetup_Get().SetWorldShape
            (static_cast<char>(spnewgamemapshapescreen_getMapShapeIndex()));
		w->UpdateGameSetup(true);
	}
}


void AllinoneTribeCallback(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void* cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	AllinoneWindow *w = g_allinoneWindow;
	if ( !w ) return;

	ns_HPlayerItem *item = (ns_HPlayerItem *)cookie;

	spnewgametribescreen_removeMyWindow(
		action,
		item->IsAI() ? nullptr : w->m_lname );




	sint32 index = spnewgametribescreen_getTribeIndex() + 1;
	if ( index < 0 ) index = 0;

	if ( !item->GetPlayer() && !item->GetAIPlayer() ) return;

	uint16 key = item->IsAI() ?
		key = *(uint16 *)item->GetAIPlayer()->GetKey()->buf :
		key = *(uint16 *)item->GetPlayer()->GetKey()->buf;


	if ( NETFunc::IsHost() && index == 0 )
	{
		w->AssignTribe(
			0,
			key,
			item->IsAI(),
			FALSE,
			TRUE );

		index = w->FindTribe( 0, item->IsAI() );
	}

	if ( item->IsAI() )
	{
		if ( NETFunc::IsHost() )
		{
			uint16 curKey;
			BOOL curIsAI;
			BOOL curIsFemale;
			BOOL success = w->WhoHasTribe( index, &curKey, &curIsAI, &curIsFemale );
			Assert( success );

			if ( curKey == 0 || curKey == key )
			{

				BOOL success = w->AssignTribe(
					index,
					key,
					TRUE,
					s_maleRadio ? !s_maleRadio->GetState() : FALSE,
					FALSE );
				Assert( success );
			}
			else
			{

			}








			spnewgametribescreen_setTribeIndex(
				playersetup_Get().GetTribe() - 1,
				strlen( w->m_lname ) ? w->m_lname : nullptr );
		}
	}
	else
	{
		ns_GPlayerListBox *gplistbox = (ns_GPlayerListBox *)
			w->FindControl( AllinoneWindow::CONTROL_GPLAYERSLISTBOX );

		if ( gplistbox->NumItems() && w->IsMine( item->GetPlayer() ) )
		{
			if ( !NETFunc::IsHost() )
			{

				w->RequestTribe( index );
			}
			else
			{
				uint16 curKey;
				BOOL curIsAI;
				BOOL curIsFemale;
				BOOL success = w->WhoHasTribe( index, &curKey, &curIsAI, &curIsFemale );
				Assert( success );

				if ( curKey == 0 )
				{

					BOOL success = w->AssignTribe(
						index,
						key,
						FALSE,
						FALSE,
						FALSE );
					Assert( success );
				}
				else if ( curKey != key )
				{

				}
				else
				{

					Assert( curIsAI == FALSE );
				}
			}
		}
	}
}


void AllinoneWindow::WorldTypeButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	AllinoneWindow *w = g_allinoneWindow;

	custommapscreen_displayMyWindow(
		w->GetMode() == JOIN ||
		w->GetMode() == CONTINUE_CREATE ||
		w->GetMode() == CONTINUE_JOIN ||
		( NETFunc::IsHost() && playersetup_Get().IsReadyToLaunch() ) );
}


void AllinoneWorldTypeCallback(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void* cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	AllinoneWindow *w = g_allinoneWindow;
	if ( !w ) return;

	custommapscreen_removeMyWindow( action );


	if ( w->GetMode() == AllinoneWindow::CREATE )
	{
		sint32 val1;
		sint32 val2;
		sint32 val3;
		sint32 val4;
		sint32 val5;
		sint32 val6;
		custommapscreen_getValues( val1, val2, val3, val4, val5, val6 );
		gamesetup_Get().SetWorldType1(static_cast<char>(val1));
		gamesetup_Get().SetWorldType2(static_cast<char>(val2));
		gamesetup_Get().SetWorldType3(static_cast<char>(val3));
		gamesetup_Get().SetWorldType4(static_cast<char>(val4));
		gamesetup_Get().SetWorldType5(static_cast<char>(val5));
		gamesetup_Get().SetWorldType6(static_cast<char>(val6));
		w->UpdateGameSetup(true);
	}
}


void AllinoneWindow::DifficultyButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	AllinoneWindow *w = g_allinoneWindow;

	spnewgamediffscreen_displayMyWindow(
		w->GetMode() == JOIN ||
		w->GetMode() == CONTINUE_CREATE ||
		w->GetMode() == CONTINUE_JOIN ||
		( NETFunc::IsHost() && playersetup_Get().IsReadyToLaunch() ) );
}


void AllinoneDifficultyCallback(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void* cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	AllinoneWindow *w = g_allinoneWindow;
	if ( !w ) return;

	spnewgamediffscreen_removeMyWindow( action );


	if ( w->GetMode() == AllinoneWindow::CREATE )
	{
		gamesetup_Get().SetDifficulty1(static_cast<char>(spnewgamediffscreen_getDifficulty1()));
		gamesetup_Get().SetDifficulty2(static_cast<char>(spnewgamediffscreen_getDifficulty2()));
		w->UpdateGameSetup(true);
	}
}


void AllinoneWindow::UnitExclusionAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_SWITCH_ACTION_ON
	&&   action != (uint32)AUI_SWITCH_ACTION_OFF ) return;


	if(!netfunc_Get())
		return;

	if ( NETFunc::IsHost() )
	{
		AllinoneWindow *w = g_allinoneWindow;
		if ( !w ) return;

		aui_Switch *sw = (aui_Switch *)control;

		if(!sw) return;


		if(exclusions_Get())
			exclusions_Get()->ExcludeUnit( m_index, sw->GetState() );

		{
			gamesetup_Get().SetUnit( (char)sw->GetState(), m_index );
			w->UpdateGameSetup(true);
		}
	}
}


void AllinoneWindow::ImprovementExclusionAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_SWITCH_ACTION_ON
	&&   action != (uint32)AUI_SWITCH_ACTION_OFF ) return;


	if ( NETFunc::IsHost() )
	{
		AllinoneWindow *w = g_allinoneWindow;
		if ( !w ) return;

		aui_Switch *sw = (aui_Switch *)control;

		exclusions_Get()->ExcludeBuilding( m_index, sw->GetState() );

		{
			gamesetup_Get().SetImprovement( (char)sw->GetState(), m_index );
			w->UpdateGameSetup(true);
		}
	}
}


void AllinoneWindow::WonderExclusionAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_SWITCH_ACTION_ON
	&&   action != (uint32)AUI_SWITCH_ACTION_OFF ) return;


	if ( NETFunc::IsHost() )
	{
		AllinoneWindow *w = g_allinoneWindow;
		if ( !w ) return;

		aui_Switch *sw = (aui_Switch *)control;

		exclusions_Get()->ExcludeWonder( m_index, sw->GetState() );

		{
			gamesetup_Get().SetWonder( (char)sw->GetState(), m_index );
			w->UpdateGameSetup(true);
		}
	}
}


void AllinoneWindow::LockSwitchAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_SWITCH_ACTION_ON
	&&   action != (uint32)AUI_SWITCH_ACTION_OFF ) return;


	AllinoneWindow *w = g_allinoneWindow;
	if ( !w ) return;

	bool lock = ( action == (uint32)AUI_SWITCH_ACTION_ON ) ? true : false;

	if ( lock )
	{
		if ( !gamesetup_Get().IsClosed() )
		{

			gamesetup_Get().SetSize(static_cast<char>(w->CurNumHumanPlayers()));
			dpEnableNewPlayers(netfunc_Get()->GetDP(), 0);
			gamesetup_Get().SetClosed( true );
			w->UpdateGameSetup();
		}
	}
	else
	{
		if ( gamesetup_Get().IsClosed() )
		{

			sint32 newMaxNumHumans = k_NS_MAX_PLAYERS - w->CurNumAiPlayers();
			if ( newMaxNumHumans > k_NS_MAX_HUMANS )
				newMaxNumHumans = k_NS_MAX_HUMANS;
			gamesetup_Get().SetSize(static_cast<char>(newMaxNumHumans));
			dpEnableNewPlayers(netfunc_Get()->GetDP(), 1);
			gamesetup_Get().SetClosed( false );
			w->UpdateGameSetup();
		}
	}
}


void AllinoneWindow::AddAIButtonAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	AllinoneWindow *w = g_allinoneWindow;

	w->AddAIPlayer();
}

void TribesButtonCallback(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	AllinoneWindow *w = g_allinoneWindow;

	ns_HPlayerItem *item = (ns_HPlayerItem *)cookie;

	if ( item->IsAI() )
	{
		spnewgametribescreen_removeTribe( playersetup_Get().GetTribe() - 1 );
		spnewgametribescreen_addTribeNoDuplicate( item->GetAIPlayer()->GetTribe() - 1 );
		spnewgametribescreen_setTribeIndex(item->GetAIPlayer()->GetTribe() - 1);
	}
	else
	{
		g_allinoneWindow->UpdateTribeSwitches();
		spnewgametribescreen_setTribeIndex(
			playersetup_Get().GetTribe() - 1,
			strlen( w->m_lname ) ? w->m_lname : nullptr );
	}

	spnewgametribescreen_displayMyWindow(
		cookie,
		!item->IsAI() );
}








































































































void CivPointsButtonCallback(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void *cookie )
{


	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;


	c3_EditButton *button = (c3_EditButton *)control;
	sint32 value = button->GetValue();

	AllinoneWindow *w = g_allinoneWindow;
	if ( !w ) return;

	ns_HPlayerItem *item = (ns_HPlayerItem *)cookie;

	if ( item->IsAI() )
	{
		if ( NETFunc::IsHost() )
		{
			nf_AIPlayer *aiplayer = item->GetAIPlayer();
			if ( value != aiplayer->GetCivPoints() )
			{
				aiplayer->SetCivPoints( value );
				w->UpdateAIPlayerSetup( aiplayer );
			}
		}
	}
	else
	{
		ns_GPlayerListBox *gplistbox = (ns_GPlayerListBox *)
			w->FindControl( AllinoneWindow::CONTROL_GPLAYERSLISTBOX );

		if ( gplistbox->NumItems() && w->IsMine( item->GetPlayer() ) )
		{
			if ( value != playersetup_Get().GetCivPoints() )
			{
				playersetup_Get().SetCivPoints( value );
				w->UpdatePlayerSetup();
			}
		}
	}
}


void PwPointsButtonCallback(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	AllinoneWindow *w = g_allinoneWindow;
	if ( !w ) return;

	c3_EditButton *button = (c3_EditButton *)control;
	sint32 value = button->GetValue();

	ns_HPlayerItem *item = (ns_HPlayerItem *)cookie;

	if ( item->IsAI() )
	{
		if ( NETFunc::IsHost() )
		{
			nf_AIPlayer *aiplayer = item->GetAIPlayer();
			if ( value != aiplayer->GetPwPoints() )
			{
				aiplayer->SetPwPoints( value );
				w->UpdateAIPlayerSetup( aiplayer );
			}
		}
	}
	else
	{
		ns_GPlayerListBox *gplistbox = (ns_GPlayerListBox *)
			w->FindControl( AllinoneWindow::CONTROL_GPLAYERSLISTBOX );

		if ( gplistbox->NumItems() && w->IsMine( item->GetPlayer() ) )
		{
			if ( value != playersetup_Get().GetPwPoints() )
			{
				playersetup_Get().SetPwPoints( value );
				w->UpdatePlayerSetup();
			}
		}
	}
}

void AllinoneWindow::EnableButtonsForUnlaunch()
{
	ns_HPlayerListBox *lb = ((ns_HPlayerListBox *)m_controls[CONTROL_HPLAYERSLISTBOX]);
	sint32 i;
	for(i = 0; i < lb->NumItems(); i++) {
		ns_HPlayerItem *item = (ns_HPlayerItem *)lb->GetItemByIndex(i);
		if((item->IsAI() && NETFunc::IsHost()) || (item->GetPlayer() && IsMine(item->GetPlayer()))) {
			if(NETFunc::IsHost()) {
				item->GetTribeButton()->Enable(TRUE);
				if(gamesetup_Get().GetHandicapping()) {
					item->GetCivpointsButton()->Enable(true);
					item->GetPwpointsButton()->Enable(true);
				}
			}
		}
	}
}
