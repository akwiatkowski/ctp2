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
    m_useCustomMap          (false)
{
	Assert(AUI_SUCCESS(*retval));

	m_spStart.reset(spNew_ctp2_Button(retval,ldlBlock,"StartButton",spnewgamescreen_startPress));
	m_spReturn.reset(spNew_ctp2_Button(retval,ldlBlock,"ReturnButton",spnewgamescreen_returnPress));
	m_scenarioName.reset(spNew_c3_Static(retval, ldlBlock, "ScenarioName"));
	m_scenarioStaticText.reset(spNew_c3_Static(retval, ldlBlock, "ScenarioStaticText"));
	m_spTribe.reset(spNew_ctp2_Button( retval, ldlBlock, "TribeButton", spnewgamescreen_tribePress ));
	m_spDifficulty.reset(spNew_ctp2_Button( retval, ldlBlock, "DifficultyButton", spnewgamescreen_difficultyPress ));
	m_spMapSize.reset(spNew_ctp2_Button( retval, ldlBlock, "MapSizeButton", spnewgamescreen_mapSizePress ));
	m_spPlayers.reset(spNew_ctp2_Button( retval, ldlBlock, "PlayersButton", spnewgamescreen_playersPress ));
	m_spMap.reset(spNew_ctp2_Button( retval, ldlBlock, "MapButton", spnewgamescreen_mapPress ));
	m_spRules.reset(spNew_ctp2_Button( retval, ldlBlock, "RulesButton", spnewgamescreen_rulesPress ));
	m_spEditor.reset(spNew_ctp2_Button(retval, ldlBlock, "EditorButton", spnewgamescreen_editorPress));
	m_spScenario.reset(spNew_ctp2_Button(retval, ldlBlock, "ScenarioButton", spnewgamescreen_scenarioPress));
	m_spName.reset(spNewTextEntry(retval,ldlBlock,"Name"));
	m_worldShapeLabel.reset(spNew_c3_Static( retval, ldlBlock, "WorldShapeLabel" ));
	m_worldShapeButton.reset(spNew_ctp2_Button( retval, ldlBlock, "WorldShapeButton", spnewgamescreen_worldShapePress ));
	m_civilizationLabel.reset(spNew_c3_Static( retval, ldlBlock, "CivilizationLabel" ));
	m_leaderNameLabel.reset(spNew_c3_Static( retval, ldlBlock, "LeaderNameLabel" ));
	m_difficultyLabel.reset(spNew_c3_Static( retval, ldlBlock, "DifficultyLabel" ));
	m_worldSizeLabel.reset(spNew_c3_Static( retval, ldlBlock, "MapSizeLabel" ));
	m_rulesLabel.reset(spNew_c3_Static( retval, ldlBlock, "RulesLabel" ));
	m_playersLabel.reset(spNew_c3_Static( retval, ldlBlock, "PlayersLabel" ));
	m_worldTypeLabel.reset(spNew_c3_Static( retval, ldlBlock, "MapButtonLabel" ));
	m_quitButton.reset(spNew_ctp2_Button( retval, ldlBlock, "QuitButton", spnewgamescreen_quitPress ));
	m_spTitle.reset(spNew_c3_Static(retval,ldlBlock,"Title"));
	m_spBackground.reset(spNew_c3_Static(retval,ldlBlock,"Background"));
	m_string.reset(spNewStringTable(retval,"SPNewGameStrings"));

	// Reset failsafe start/end ages here, so they're correct before agesscreen is initialized.
	profiledb_Get()->SetSPStartingAge(0);
	profiledb_Get()->SetSPEndingAge(g_theAgeDB->NumRecords() - 1);
	profiledb_Get()->Save();

    Update();
}

SPNewGameWindow::~SPNewGameWindow()
{
    m_spStart.reset();
    m_spOk.reset();
	m_spReturn.reset();
	m_spTribe.reset();
	m_spDifficulty.reset();
	m_spMapSize.reset();
	m_spPlayers.reset();
	m_spMap.reset();
	m_spRules.reset();
	m_spEditor.reset();
	m_spScenario.reset();
	m_spGeneral.reset();
    m_spWorld.reset();
    m_spCustom.reset();
    m_spName.reset();
	m_mapTypeButton.reset();
	m_mapTypeLabel.reset();
	m_worldShapeButton.reset();
	m_worldShapeLabel.reset();
	m_civilizationLabel.reset();
	m_leaderNameLabel.reset();
	m_difficultyLabel.reset();
	m_worldSizeLabel.reset();
	m_rulesLabel.reset();
	m_playersLabel.reset();
	m_worldTypeLabel.reset();
	m_quitButton.reset();
	m_spTitle.reset();
	m_spBackground.reset();
	m_string.reset();
	m_scenarioName.reset();
	m_scenarioStaticText.reset();
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










SPProfileBox::SPProfileBox ( AUI_ERRCODE *retval, uint32 id, MBCHAR *ldlBlock )
{
	{
		int i=0;
		int numClans;

		numClans = g_theCivilisationDB->NumRecords();

		m_spClan.reset(spNew_c3_DropDown(retval,ldlBlock,"Clan",spnewgamescreen_clanSelect));

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
		m_spGender.reset(spNew_c3_DropDown(retval,ldlBlock,"Gender",spnewgamescreen_genderSelect));
		std::unique_ptr<aui_StringTable> gender(spNewStringTable(retval,"SPGenderChoicesStringTable"));
		spFillDropDown(retval,m_spGender.get(),gender.get(),"SPDropDownListItem","Gender");
	}
	m_spName.reset(spNewTextEntry(retval,ldlBlock,"Name"));
	m_spPreferences.reset(spNew_ctp2_Button(retval,ldlBlock,"Preferences",spnewgamescreen_preferencePress));
	m_spCustom.reset(spNew_ctp2_Button(retval,ldlBlock,"PCustom", spnewgamescreen_pCustomPress));

	m_PTOP.reset(spNew_c3_Static(retval,ldlBlock, "PTOP"));
	m_PHEADER.reset(spNew_c3_Static(retval,ldlBlock, "PHEADER"));
	m_PBOT.reset(spNew_c3_Static(retval,ldlBlock, "PBOT"));
	m_PLEFT.reset(spNew_c3_Static(retval,ldlBlock, "PLEFT"));
	m_PRIGHT.reset(spNew_c3_Static(retval,ldlBlock, "PRIGHT"));
}

SPProfileBox::~SPProfileBox()
{
	m_spClan.reset();
	m_spGender.reset();
	m_spName.reset();
	m_spPreferences.reset();
	m_spCustom.reset();

    m_PTOP.reset();
	m_PHEADER.reset();
	m_PBOT.reset();
	m_PLEFT.reset();
	m_PRIGHT.reset();
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










SPWorldBox::SPWorldBox ( AUI_ERRCODE *retval, uint32 id, MBCHAR *ldlBlock )
{
	m_mapSize.reset(spNew_c3_DropDown(retval,ldlBlock,"MapSize",spnewgamescreen_mapSizeSelect));
	{
		std::unique_ptr<aui_StringTable> mysizes(spNewStringTable(retval,"SPMapSizeStringTable"));
		spFillDropDown(retval,m_mapSize.get(),mysizes.get(),"SPDropDownListItem","MapSize");
	}
	m_worldType.reset(spNew_c3_DropDown(retval,ldlBlock,"WorldType",spnewgamescreen_worldTypeSelect));
	{
		std::unique_ptr<aui_StringTable> mytypes(spNewStringTable(retval,"SPWorldTypeStringTable"));
		spFillDropDown(retval,m_worldType.get(),mytypes.get(),"SPDropDownListItem","WorldType");
	}
	m_worldShape.reset(spNew_c3_DropDown(retval,ldlBlock,"WorldShape",spnewgamescreen_worldShapeSelect));
	{
		std::unique_ptr<aui_StringTable> myshapes(spNewStringTable(retval,"SPWorldShapeStringTable"));
		spFillDropDown(retval,m_worldShape.get(),myshapes.get(),"SPDropDownListItem","WorldShape");
	}
	m_difficulty.reset(spNew_c3_DropDown(retval,ldlBlock,"Difficulty",spnewgamescreen_difficultySelect));
	{
		std::unique_ptr<aui_StringTable> mydiffs(spNewStringTable(retval,"SPDifficultyStringTable"));
		spFillDropDown(retval,m_difficulty.get(),mydiffs.get(),"SPDropDownListItem","Difficulty");

		m_difficulty->SetSelectedItem(profiledb_Get()->GetDifficulty());
	}
	m_riskLevel.reset(spNew_c3_DropDown(retval,ldlBlock,"RiskLevel",spnewgamescreen_riskLevelSelect));
	{
		std::unique_ptr<aui_StringTable> myrisks(spNewStringTable(retval,"SPRiskLevelStringTable"));
		spFillDropDown(retval,m_riskLevel.get(),myrisks.get(),"SPDropDownListItem","RiskLevel");

		m_riskLevel->SetSelectedItem(profiledb_Get()->GetRiskLevel());
	}
	m_opponent.reset(spNew_c3_DropDown(retval,ldlBlock,"Opponent",spnewgamescreen_opponentSelect));
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
	m_spCustom.reset(spNew_ctp2_Button(retval,ldlBlock,"WCustom", spnewgamescreen_wCustomPress));

	m_WTOP.reset(spNew_c3_Static(retval,ldlBlock, "WTOP"));
	m_WHEADER.reset(spNew_c3_Static(retval,ldlBlock, "WHEADER"));
	m_WBOT.reset(spNew_c3_Static(retval,ldlBlock, "WBOT"));
	m_WLEFT.reset(spNew_c3_Static(retval,ldlBlock, "WLEFT"));
	m_WRIGHT.reset(spNew_c3_Static(retval,ldlBlock, "WRIGHT"));
}

SPWorldBox::~SPWorldBox()
{
	m_mapSize.reset();
	m_worldType.reset();
	m_worldShape.reset();
	m_difficulty.reset();
	m_riskLevel.reset();
	m_opponent.reset();
	m_spCustom.reset();

	m_WTOP.reset();
	m_WHEADER.reset();
	m_WBOT.reset();
	m_WLEFT.reset();
	m_WRIGHT.reset();
}









SPRulesBox::SPRulesBox ( AUI_ERRCODE *retval, uint32 id, MBCHAR *ldlBlock )
{

	m_spGenocide.reset(spNew_c3_CheckBox(retval,ldlBlock,"GenocideButton",0,spnewgamescreen_genocidePress));
	m_spTrade.reset(spNew_c3_CheckBox(retval,ldlBlock,"TradeButton",0,spnewgamescreen_tradePress));
	m_spCombat.reset(spNew_c3_CheckBox(retval,ldlBlock,"CombatButton",0,spnewgamescreen_combatPress));
	m_spPollution.reset(spNew_c3_CheckBox(retval,ldlBlock,"PollutionButton",0,spnewgamescreen_pollutionPress));

	m_spGenocide->SetState(profiledb_Get()->IsGenocideRule());
	m_spTrade->SetState(profiledb_Get()->IsTradeRule());
	m_spCombat->SetState(profiledb_Get()->IsSimpleCombatRule());
	m_spPollution->SetState(profiledb_Get()->IsPollutionRule());

	m_RTOP.reset(spNew_c3_Static(retval,ldlBlock, "RTOP"));
	m_RHEADER.reset(spNew_c3_Static(retval,ldlBlock, "RHEADER"));
	m_RBOT.reset(spNew_c3_Static(retval,ldlBlock, "RBOT"));
	m_RLEFT.reset(spNew_c3_Static(retval,ldlBlock, "RLEFT"));
	m_RRIGHT.reset(spNew_c3_Static(retval,ldlBlock, "RRIGHT"));

}

SPRulesBox::~SPRulesBox()
{
	m_spGenocide.reset();
	m_spTrade.reset();
	m_spCombat.reset();
	m_spPollution.reset();

	m_RTOP.reset();
	m_RHEADER.reset();
	m_RBOT.reset();
	m_RLEFT.reset();
	m_RRIGHT.reset();
}

uint32 SPRulesBox::GetGenocideRules() { return m_spGenocide->IsOn(); }
uint32 SPRulesBox::GetTradeRules() { return m_spGenocide->IsOn(); }
uint32 SPRulesBox::GetCombatRules() { return m_spGenocide->IsOn(); }
uint32 SPRulesBox::GetPollutionRules() { return m_spGenocide->IsOn(); }











SPDropDownListItem::SPDropDownListItem(AUI_ERRCODE *retval, MBCHAR *ldlBlock,MBCHAR *type,const MBCHAR *name)
:
	aui_ImageBase(ldlBlock),
	aui_TextBase(ldlBlock, (MBCHAR *)nullptr),
	c3_ListItem( retval, ldlBlock)
{
	m_myItem.reset(spNew_c3_Static(retval, ldlBlock, type));
	if(m_myItem) {
		m_myItem->SetText(name);
		AddChild(m_myItem.get());
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
