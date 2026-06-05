//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Single Player New game Start Screen
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
// - Fixed crash when the game tries to display invalid text strings,
//   by Martin G�hmann.
// - Tribe index check updated.
// - Allowed for a number of players less than 3 to be displayed
//   - JJB 2005/06/28
// - Replaced old civilisation database by new one. (Aug 21st 2005 Martin G�hmann)
// - Added setting up of single-player start and end age values. (11-Apr-2009 Maq)
// - Ensure agesscreen::s_numAges is set when selecting a scenario directly.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/interface/spnewgamewindow.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_surface.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_imagebase.h"
#include "ui/aui_common/aui_textbase.h"
#include "ui/aui_common/aui_textfield.h"
#include "ui/aui_common/aui_stringtable.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_ctp2/ctp2_button.h"
#include "ui/aui_ctp2/c3_switch.h"
#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/c3_checkbox.h"
#include "ui/aui_ctp2/c3_listitem.h"
#include "ui/aui_ctp2/c3_dropdown.h"
#include "gs/fileio/civscenarios.h"
#include "gs/utility/Globals.h"
#include "gs/database/profileDB.h"                  // profiledb_Get()
#include "CivilisationRecord.h"
#include "gs/gameobj/CivilisationPool.h"
#include "ui/aui_ctp2/c3textfield.h"
#include "ui/interface/loadsavemapwindow.h"
#include "ui/interface/spnewgametribescreen.h"
#include "ui/interface/spnewgamemapsizescreen.h"
#include "gs/database/StrDB.h"                      // stringdb_Get()
#include "AgeRecord.h"					// g_theAgeDB
#include "ui/interface/agesscreen.h"

extern LoadSaveMapWindow			*g_loadSaveMapWindow;








































SPNewGameWindow::SPNewGameWindow(AUI_ERRCODE *retval, uint32 id,
		MBCHAR *ldlBlock, sint32 bpp, AUI_WINDOW_TYPE type, bool bevel)
:
    C3Window                (retval, id, ldlBlock, bpp, type, bevel),
	m_spStart               (nullptr),
    m_spOk                  (nullptr),
    m_spReturn              (nullptr),
	m_spTribe               (nullptr),
    m_spDifficulty          (nullptr),
	m_spMapSize             (nullptr),
    m_spPlayers             (nullptr),
	m_spMap                 (nullptr),
    m_spRules               (nullptr),
	m_spEditor              (nullptr),
	m_spScenario            (nullptr),
	m_spGeneral             (nullptr),
    m_spWorld               (nullptr),
    m_spCustom              (nullptr),
    m_spName                (nullptr),
	m_mapTypeButton         (nullptr),
	m_mapTypeLabel          (nullptr),
	m_worldShapeButton      (nullptr),
	m_worldShapeLabel       (nullptr),
    m_useCustomMap          (false),
    m_civilizationLabel     (nullptr),
	m_leaderNameLabel       (nullptr),
	m_difficultyLabel       (nullptr),
	m_worldSizeLabel        (nullptr),
	m_rulesLabel            (nullptr),
	m_playersLabel          (nullptr),
	m_worldTypeLabel        (nullptr),
	m_quitButton            (nullptr),
	m_spTitle               (nullptr),
	m_spBackground          (nullptr),
	m_string                (nullptr),
	m_scenarioName          (nullptr),
	m_scenarioStaticText    (nullptr)
{
	Assert(AUI_SUCCESS(*retval));

	m_spStart = spNew_ctp2_Button(retval,ldlBlock,"StartButton",spnewgamescreen_startPress);
	m_spReturn = spNew_ctp2_Button(retval,ldlBlock,"ReturnButton",spnewgamescreen_returnPress);
	m_scenarioName = spNew_c3_Static(retval, ldlBlock, "ScenarioName");
	m_scenarioStaticText = spNew_c3_Static(retval, ldlBlock, "ScenarioStaticText");
	m_spTribe		= spNew_ctp2_Button( retval, ldlBlock, "TribeButton", spnewgamescreen_tribePress );
	m_spDifficulty	= spNew_ctp2_Button( retval, ldlBlock, "DifficultyButton", spnewgamescreen_difficultyPress );
	m_spMapSize		= spNew_ctp2_Button( retval, ldlBlock, "MapSizeButton", spnewgamescreen_mapSizePress );
	m_spPlayers		= spNew_ctp2_Button( retval, ldlBlock, "PlayersButton", spnewgamescreen_playersPress );
	m_spMap			= spNew_ctp2_Button( retval, ldlBlock, "MapButton", spnewgamescreen_mapPress );
	m_spRules		= spNew_ctp2_Button( retval, ldlBlock, "RulesButton", spnewgamescreen_rulesPress );
	m_spEditor = spNew_ctp2_Button(retval, ldlBlock, "EditorButton", spnewgamescreen_editorPress);
	m_spScenario = spNew_ctp2_Button(retval, ldlBlock, "ScenarioButton", spnewgamescreen_scenarioPress);
	m_spName = spNewTextEntry(retval,ldlBlock,"Name");
	m_worldShapeLabel = spNew_c3_Static( retval, ldlBlock, "WorldShapeLabel" );
	m_worldShapeButton = spNew_ctp2_Button( retval, ldlBlock, "WorldShapeButton", spnewgamescreen_worldShapePress );
	m_civilizationLabel = spNew_c3_Static( retval, ldlBlock, "CivilizationLabel" );
	m_leaderNameLabel = spNew_c3_Static( retval, ldlBlock, "LeaderNameLabel" );
	m_difficultyLabel = spNew_c3_Static( retval, ldlBlock, "DifficultyLabel" );
	m_worldSizeLabel = spNew_c3_Static( retval, ldlBlock, "MapSizeLabel" );
	m_rulesLabel = spNew_c3_Static( retval, ldlBlock, "RulesLabel" );
	m_playersLabel = spNew_c3_Static( retval, ldlBlock, "PlayersLabel" );
	m_worldTypeLabel = spNew_c3_Static( retval, ldlBlock, "MapButtonLabel" );
	m_quitButton = spNew_ctp2_Button( retval, ldlBlock, "QuitButton", spnewgamescreen_quitPress );
	m_spTitle			= spNew_c3_Static(retval,ldlBlock,"Title");
	m_spBackground		= spNew_c3_Static(retval,ldlBlock,"Background");
	m_string			= spNewStringTable(retval,"SPNewGameStrings");

	// Reset failsafe start/end ages here, so they're correct before agesscreen is initialized.
	profiledb_Get()->SetSPStartingAge(0);
	profiledb_Get()->SetSPEndingAge(g_theAgeDB->NumRecords() - 1);
	profiledb_Get()->Save();

    Update();
}

SPNewGameWindow::~SPNewGameWindow()
{
    delete m_spStart;
    delete m_spOk;
	delete m_spReturn;
	delete m_spTribe;
	delete m_spDifficulty;
	delete m_spMapSize;
	delete m_spPlayers;
	delete m_spMap;
	delete m_spRules;
	delete m_spEditor;
	delete m_spScenario;
	delete m_spGeneral;
    delete m_spWorld;
    delete m_spCustom;
    delete m_spName;
	delete m_mapTypeButton;
	delete m_mapTypeLabel;
	delete m_worldShapeButton;
	delete m_worldShapeLabel;
	delete m_civilizationLabel;
	delete m_leaderNameLabel;
	delete m_difficultyLabel;
	delete m_worldSizeLabel;
	delete m_rulesLabel;
	delete m_playersLabel;
	delete m_worldTypeLabel;
	delete m_quitButton;
	delete m_spTitle;
	delete m_spBackground;
	delete m_string;
	delete m_scenarioName;
	delete m_scenarioStaticText;
}

//----------------------------------------------------------------------------
//
// Name       : Update
//
// Description: Updates the settings information in the SPNewGameWindow.
//
// Parameters : -
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : All sprintf function calls have now three arguments to
//              prevent a crash if the last argument is invalid.
//
//----------------------------------------------------------------------------
void SPNewGameWindow::Update( )
{
	MBCHAR s[_MAX_PATH];
	sint32 index;

	m_spTribe->SetText( profiledb_Get()->GetCivName() );

	if ( m_useCustomMap && g_loadSaveMapWindow && g_loadSaveMapWindow->GetSaveMapInfo() )
	{
		MBCHAR mname[ 100 ];
		g_loadSaveMapWindow->GetSaveMapName( mname );

	}
	else
	{

		m_useCustomMap = false;

	}

	index = profiledb_Get()->GetDifficulty();
//Added by Martin G�hmann
//Makes sure that the game doesn't crash if the according map size string is invalid.
	snprintf(s, sizeof(s), "%s", m_string->GetString(SP_NEWGAME_STR_CHIEFTAIN + index) );
	m_spDifficulty->SetText( s );


	MAPSIZE size;
	size = profiledb_Get()->GetMapSize();

	switch (size) {
	case MAPSIZE_SMALL:
		index = 0;
		break;
	case MAPSIZE_MEDIUM:
		index = 1;
		break;
	case MAPSIZE_LARGE:
		index = 2;
		break;
	case MAPSIZE_GIGANTIC:
		index = 3;
		break;
	}













//Added by Martin G�hmann
//Makes sure that the game doesn't crash if the according map size string is invalid.
	snprintf(s, sizeof(s), "%s", m_string->GetString(SP_NEWGAME_STR_SMALL + index) );
	m_spMapSize->SetText( s );

	sint32 shape = profiledb_Get()->GetWorldShape();
//Added by Martin G�hmann
//Makes sure that the game doesn't crash if the according world shape string is invalid.
	snprintf(s, sizeof(s), "%s", m_string->GetString(SP_NEWGAME_STR_EARTH + shape) );
	m_worldShapeButton->SetText( s );


	sint32 numPlayers = profiledb_Get()->GetNPlayers() - 1;

	// Removed the alteration to the value when it was below 3 - JJB
	snprintf(s, sizeof(s), "%d", numPlayers);
	m_spPlayers->SetText( s );

	// Make sure start and end ages are still within range.
	// A scenario was loaded.
	if (civpaths_Get()->GetCurScenarioPath() != nullptr) {

		if (strlen(scenario_name_buf()) > 0) {
			m_scenarioName->SetText(scenario_name_buf());
			m_scenarioName->ShouldDraw(TRUE);
		}
		m_spScenario->SetText(stringdb_Get()->GetNameStr("str_ldl_SP_STANDARD_GAME"));
		m_spScenario->ShouldDraw(TRUE);
		m_scenarioName->Show();
		m_scenarioStaticText->Show();

		sint32 ages		= g_theAgeDB->NumRecords();

		// Reset failsafe start/end age.
		profiledb_Get()->SetSPStartingAge(0);
		profiledb_Get()->SetSPEndingAge(ages - 1);
		agesscreen_Initialize();
		agesscreen_setStartAge(0);
		agesscreen_setEndAge(ages - 1);

	// No scenario loaded.
	} else {

		m_scenarioName->SetText(stringdb_Get()->GetNameStr("str_ldl_SP_STANDARD_GAME"));
		m_scenarioName->Hide();
		m_scenarioStaticText->Hide();
		m_scenarioName->ShouldDraw(TRUE);

		m_spScenario->SetText(stringdb_Get()->GetNameStr("str_ldl_SP_SCENARIO_PICKER"));
		m_spScenario->ShouldDraw(TRUE);

		sint32 ages		= g_theAgeDB->NumRecords();
		sint32 startAge	= profiledb_Get()->GetSPStartingAge();
		sint32 endAge	= profiledb_Get()->GetSPEndingAge();

		// Check ages are still within range.
		// Do not reset failsafe ages here, or they can never be set.
		if (startAge != 0) {
			if (startAge > endAge
			 || startAge < 0) {
				profiledb_Get()->SetSPStartingAge(0);
				agesscreen_setStartAge(0);
			}
		}

		if (endAge != (ages - 1)) {
			if (endAge < startAge
			 || endAge > (ages - 1)) {
				profiledb_Get()->SetSPEndingAge(ages - 1);
				agesscreen_setEndAge(ages - 1);
			}
		}
	}
	// Make sure changes are saved for a new game.
	profiledb_Get()->Save();
}










SPProfileBox::SPProfileBox ( AUI_ERRCODE *retval, uint32 id, MBCHAR *ldlBlock ) :
	m_spClan(nullptr),m_spGender(nullptr),m_spName(nullptr),
	m_spPreferences(nullptr),m_spCustom(nullptr),
	m_PTOP(nullptr),
	m_PHEADER(nullptr),
	m_PBOT(nullptr),
	m_PLEFT(nullptr),
	m_PRIGHT(nullptr)
{
	{
		int i=0;
		int numClans;

		numClans = g_theCivilisationDB->NumRecords();

		m_spClan		= spNew_c3_DropDown(retval,ldlBlock,"Clan",spnewgamescreen_clanSelect);

		while(i<numClans) {
			aui_Item	*item = nullptr;
			const MBCHAR *cName = stringdb_Get()->GetNameStr(g_theCivilisationDB->Get(i)->GetPluralCivName());
			item = (aui_Item*)new SPDropDownListItem(retval, "SPDropDownListItem", "Clan", cName);
			if (item)
				m_spClan->AddItem(item );
			i++;
		}
	}
	{
		m_spGender		= spNew_c3_DropDown(retval,ldlBlock,"Gender",spnewgamescreen_genderSelect);
		aui_StringTable * gender = spNewStringTable(retval,"SPGenderChoicesStringTable");
		spFillDropDown(retval,m_spGender,gender,"SPDropDownListItem","Gender");
		delete gender;
	}
	m_spName		= spNewTextEntry(retval,ldlBlock,"Name");
	m_spPreferences = spNew_ctp2_Button(retval,ldlBlock,"Preferences",spnewgamescreen_preferencePress);
	m_spCustom		= spNew_ctp2_Button(retval,ldlBlock,"PCustom", spnewgamescreen_pCustomPress);

	m_PTOP			= spNew_c3_Static(retval,ldlBlock, "PTOP");
	m_PHEADER		= spNew_c3_Static(retval,ldlBlock, "PHEADER");
	m_PBOT			= spNew_c3_Static(retval,ldlBlock, "PBOT");
	m_PLEFT			= spNew_c3_Static(retval,ldlBlock, "PLEFT");
	m_PRIGHT		= spNew_c3_Static(retval,ldlBlock, "PRIGHT");
}

SPProfileBox::~SPProfileBox()
{
	delete m_spClan;
	delete m_spGender;
	delete m_spName;
	delete m_spPreferences;
	delete m_spCustom;

    delete m_PTOP;
	delete m_PHEADER;
	delete m_PBOT;
	delete m_PLEFT;
	delete m_PRIGHT;
}

void SPProfileBox::SetLeader(uint32 index)
{
	sint32 const    tribeIndex = spnewgametribescreen_getTribeIndex();
	if ((tribeIndex < 0) || (tribeIndex >= INDEX_TRIBE_INVALID))
	{
		const MBCHAR *name =
			stringdb_Get()->GetNameStr(
				g_theCivilisationDB->Get(index)->GetLeaderNameMale());

		m_spName->SetFieldText(name);
	}
	else
	{
		const sint32 size = 100;
		MBCHAR lname[ size + 1 ];
		spnewgametribescreen_getLeaderName( lname );
		m_spName->SetFieldText( lname );
	}
}










SPWorldBox::SPWorldBox ( AUI_ERRCODE *retval, uint32 id, MBCHAR *ldlBlock ) :
	m_mapSize(nullptr), m_worldType(nullptr), m_worldShape(nullptr),
	m_difficulty(nullptr), m_riskLevel(nullptr), m_opponent(nullptr), m_spCustom(nullptr),
	m_WTOP(nullptr),
	m_WHEADER(nullptr),
	m_WBOT(nullptr),
	m_WLEFT(nullptr),
	m_WRIGHT(nullptr)
{
	m_mapSize		= spNew_c3_DropDown(retval,ldlBlock,"MapSize",spnewgamescreen_mapSizeSelect);
	{
		aui_StringTable * mysizes = spNewStringTable(retval,"SPMapSizeStringTable");
		spFillDropDown(retval,m_mapSize,mysizes,"SPDropDownListItem","MapSize");
		delete mysizes;
	}
	m_worldType		= spNew_c3_DropDown(retval,ldlBlock,"WorldType",spnewgamescreen_worldTypeSelect);
	{
		aui_StringTable * mytypes = spNewStringTable(retval,"SPWorldTypeStringTable");
		spFillDropDown(retval,m_worldType,mytypes,"SPDropDownListItem","WorldType");
		delete mytypes;
	}
	m_worldShape	= spNew_c3_DropDown(retval,ldlBlock,"WorldShape",spnewgamescreen_worldShapeSelect);
	{
		aui_StringTable * myshapes	= spNewStringTable(retval,"SPWorldShapeStringTable");
		spFillDropDown(retval,m_worldShape,myshapes,"SPDropDownListItem","WorldShape");
		delete myshapes;
	}
	m_difficulty	= spNew_c3_DropDown(retval,ldlBlock,"Difficulty",spnewgamescreen_difficultySelect);
	{
		aui_StringTable * mydiffs = spNewStringTable(retval,"SPDifficultyStringTable");
		spFillDropDown(retval,m_difficulty,mydiffs,"SPDropDownListItem","Difficulty");
		delete mydiffs;

		m_difficulty->SetSelectedItem(profiledb_Get()->GetDifficulty());
	}
	m_riskLevel		= spNew_c3_DropDown(retval,ldlBlock,"RiskLevel",spnewgamescreen_riskLevelSelect);
	{
		aui_StringTable * myrisks = spNewStringTable(retval,"SPRiskLevelStringTable");
		spFillDropDown(retval,m_riskLevel,myrisks,"SPDropDownListItem","RiskLevel");
		delete myrisks;

		m_riskLevel->SetSelectedItem(profiledb_Get()->GetRiskLevel());
	}
	m_opponent		= spNew_c3_DropDown(retval,ldlBlock,"Opponent",spnewgamescreen_opponentSelect);
	{
		for(uint32 i=3; i<=16; i++) {
			MBCHAR			textBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
			snprintf(textBlock, sizeof(textBlock), "%d",i);
			c3_ListItem *myitem = new SPDropDownListItem(retval,"SPDropDownListItem","Opponent", textBlock);
			if(myitem) m_opponent->AddItem(myitem);
		}

		uint32 numPlayers = profiledb_Get()->GetNPlayers();
		Assert((numPlayers>2) && (numPlayers<17));
		m_opponent->SetSelectedItem(numPlayers-3);
	}
	m_spCustom		= spNew_ctp2_Button(retval,ldlBlock,"WCustom", spnewgamescreen_wCustomPress);

	m_WTOP			= spNew_c3_Static(retval,ldlBlock, "WTOP");
	m_WHEADER		= spNew_c3_Static(retval,ldlBlock, "WHEADER");
	m_WBOT			= spNew_c3_Static(retval,ldlBlock, "WBOT");
	m_WLEFT			= spNew_c3_Static(retval,ldlBlock, "WLEFT");
	m_WRIGHT		= spNew_c3_Static(retval,ldlBlock, "WRIGHT");
}

SPWorldBox::~SPWorldBox()
{
	delete m_mapSize;
	delete m_worldType;
	delete m_worldShape;
	delete m_difficulty;
	delete m_riskLevel;
	delete m_opponent;
	delete m_spCustom;

	delete m_WTOP;
	delete m_WHEADER;
	delete m_WBOT;
	delete m_WLEFT;
	delete m_WRIGHT;
}









SPRulesBox::SPRulesBox ( AUI_ERRCODE *retval, uint32 id, MBCHAR *ldlBlock ) :
	m_spGenocide(nullptr), m_spTrade(nullptr), m_spCombat(nullptr),
	m_spPollution(nullptr),
	m_RTOP(nullptr),
	m_RHEADER(nullptr),
	m_RBOT(nullptr),
	m_RLEFT(nullptr),
	m_RRIGHT(nullptr)
{

	m_spGenocide	= spNew_c3_CheckBox(retval,ldlBlock,"GenocideButton",0,spnewgamescreen_genocidePress);
	m_spTrade		= spNew_c3_CheckBox(retval,ldlBlock,"TradeButton",0,spnewgamescreen_tradePress);
	m_spCombat		= spNew_c3_CheckBox(retval,ldlBlock,"CombatButton",0,spnewgamescreen_combatPress);
	m_spPollution	= spNew_c3_CheckBox(retval,ldlBlock,"PollutionButton",0,spnewgamescreen_pollutionPress);

	m_spGenocide->SetState(profiledb_Get()->IsGenocideRule());
	m_spTrade->SetState(profiledb_Get()->IsTradeRule());
	m_spCombat->SetState(profiledb_Get()->IsSimpleCombatRule());
	m_spPollution->SetState(profiledb_Get()->IsPollutionRule());

	m_RTOP			= spNew_c3_Static(retval,ldlBlock, "RTOP");
	m_RHEADER		= spNew_c3_Static(retval,ldlBlock, "RHEADER");
	m_RBOT			= spNew_c3_Static(retval,ldlBlock, "RBOT");
	m_RLEFT			= spNew_c3_Static(retval,ldlBlock, "RLEFT");
	m_RRIGHT		= spNew_c3_Static(retval,ldlBlock, "RRIGHT");

}

SPRulesBox::~SPRulesBox()
{
	delete m_spGenocide;
	delete m_spTrade;
	delete m_spCombat;
	delete m_spPollution;

	delete m_RTOP;
	delete m_RHEADER;
	delete m_RBOT;
	delete m_RLEFT;
	delete m_RRIGHT;
}

uint32 SPRulesBox::GetGenocideRules() { return m_spGenocide->IsOn(); }
uint32 SPRulesBox::GetTradeRules() { return m_spGenocide->IsOn(); }
uint32 SPRulesBox::GetCombatRules() { return m_spGenocide->IsOn(); }
uint32 SPRulesBox::GetPollutionRules() { return m_spGenocide->IsOn(); }











SPDropDownListItem::SPDropDownListItem(AUI_ERRCODE *retval, MBCHAR *ldlBlock,MBCHAR *type,const MBCHAR *name)
:
	aui_ImageBase(ldlBlock),
	aui_TextBase(ldlBlock, (MBCHAR *)nullptr),
	c3_ListItem( retval, ldlBlock),
	m_myItem(nullptr)
{

	m_myItem = spNew_c3_Static(retval, ldlBlock, type);
	if(m_myItem) {
		m_myItem->SetText(name);
		AddChild(m_myItem);
	}
}

SPDropDownListItem::~SPDropDownListItem()
{

}
#if 0
sint32 SPDropDownListItem::Compare(c3_ListItem *item2, uint32 column)
{

}
#endif











TwoChoiceButton::TwoChoiceButton(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		MBCHAR *choiceOff, MBCHAR *choiceOn, uint32 onoff,
		ControlActionCallback *ActionFunc,
		void *cookie) :
	aui_ImageBase( ldlBlock ),
	aui_TextBase( ldlBlock, (MBCHAR *)nullptr ),
	ctp2_Button(retval,id,ldlBlock,ActionFunc,cookie),
	m_choice(0)
{
		Assert(onoff == 1 || onoff == 0 );
	m_choice = onoff;
	strlcpy(m_choices[0], choiceOff, sizeof(m_choices[0]));
	strlcpy(m_choices[1], choiceOn, sizeof(m_choices[1]));
	SetText(m_choices[m_choice]);
}

uint32 TwoChoiceButton::Switch()
{
	m_choice = (m_choice ? 0 : 1);
	SetText(m_choices[m_choice]);
	return m_choice;
}
