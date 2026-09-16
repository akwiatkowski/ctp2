//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Handling of user preferences.
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
// - Addion by Martin Gühmann: Two more world shape options,
//   flat world and Uranus world
// - Improved import structure and compatibility.
//
//----------------------------------------------------------------------------

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef SPNEWGAMEWINDOW_FLAG
#define SPNEWGAMEWINDOW_FLAG

//----------------------------------------------------------------------------
// Library dependencies
//----------------------------------------------------------------------------

// #include <>

//----------------------------------------------------------------------------
// Export overview
//----------------------------------------------------------------------------

class SPDropDownListItem;
#include <memory>

class SPNewGameWindow;
class SPProfileBox;
class SPRulesBox;
class SPWorldBox;
class TwoChoiceButton;

enum SP_NEWGAME_STR
{
	SP_NEWGAME_STR_CHIEFTAIN,
	SP_NEWGAME_STR_WARLORD,
	SP_NEWGAME_STR_PRINCE,
	SP_NEWGAME_STR_KING,
	SP_NEWGAME_STR_EMPEROR,
	SP_NEWGAME_STR_DEITY,

	SP_NEWGAME_STR_SMALL,
	SP_NEWGAME_STR_MEDIUM,
	SP_NEWGAME_STR_LARGE,
	SP_NEWGAME_STR_VERYLARGE,

	SP_NEWGAME_STR_RANDOMMAP,
	SP_NEWGAME_STR_CUSTOMMAP,

	SP_NEWGAME_STR_EARTH,
	SP_NEWGAME_STR_DOUGHNUT,
	SP_NEWGAME_STR_FLAT,
	SP_NEWGAME_STR_URANUS,

	SP_NEWGAME_STR_DEFAULT,
	SP_NEWGAME_STR_CUSTOM,

	SP_NEWGAME_STR_MAX
};

//----------------------------------------------------------------------------
// Project dependencies
//----------------------------------------------------------------------------

#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_ctp2/c3_dropdown.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_ctp2/c3_button.h"
#include "ui/aui_ctp2/c3_listitem.h"
#include "ui/aui_ctp2/ctp2_button.h"

class aui_Control;
class c3_Static;
class c3_ListBox;
class C3Window;
class C3Slider;
class aui_SwitchGroup;
class aui_StringTable;
class c3_CheckBox;
class c3_Switch;

class aui_TextField;
class C3TextField;
class aui_StringTable;

#include "ui/aui_ctp2/ctp2_button.h"

//----------------------------------------------------------------------------
// Declarations
//----------------------------------------------------------------------------

sint32      spnewgamescreen_displayMyWindow();
sint32      spnewgamescreen_removeMyWindow(uint32 action);
sint32      spnewgamescreen_setPlayerName( const MBCHAR *name );
AUI_ERRCODE spnewgamescreen_Initialize( );
void        spnewgamescreen_Cleanup();
sint32      spnewgamescreen_update( );




void spnewgamescreen_instaPress(aui_Control *control, uint32 action, uint32 data, void *cookie );
void spnewgamescreen_startPress(aui_Control *control, uint32 action, uint32 data, void *cookie );
void spnewgamescreen_returnPress(aui_Control *control, uint32 action, uint32 data, void *cookie );
void spnewgamescreen_quitPress(aui_Control *control, uint32 action, uint32 data, void *cookie );






void spnewgamescreen_tribePress( aui_Control *control, uint32 action, uint32 data, void *cookie );
void spnewgamescreen_malePress( aui_Control *control, uint32 action, uint32 data, void *cookie );
void spnewgamescreen_femalePress( aui_Control *control, uint32 action, uint32 data, void *cookie );
void spnewgamescreen_difficultyPress( aui_Control *control, uint32 action, uint32 data, void *cookie );

void spnewgamescreen_mapSizePress( aui_Control *control, uint32 action, uint32 data, void *cookie );
void spnewgamescreen_playersPress( aui_Control *control, uint32 action, uint32 data, void *cookie );

void spnewgamescreen_mapPress( aui_Control *control, uint32 action, uint32 data, void *cookie );
void spnewgamescreen_rulesPress( aui_Control *control, uint32 action, uint32 data, void *cookie );





void spnewgamescreen_mapTypePress( aui_Control *control, uint32 action, uint32 data, void *cookie );
void spnewgamescreen_worldShapePress( aui_Control *control, uint32 action, uint32 data, void *cookie );

void spnewgamescreen_editorPress( aui_Control *control, uint32 action, uint32 data, void *cookie );

void spnewgamescreen_scenarioPress(aui_Control *control, uint32 action, uint32 data, void *cookie );





void spnewgamescreen_clanSelect(aui_Control *control, uint32 action, uint32 data, void *cookie );
void spnewgamescreen_genderSelect(aui_Control *control, uint32 action, uint32 data, void *cookie );
void spnewgamescreen_preferencePress(aui_Control *control, uint32 action, uint32 data, void *cookie );
void spnewgamescreen_pCustomPress(aui_Control *control, uint32 action, uint32 data, void *cookie );

void spnewgamescreen_mapSizeSelect(aui_Control *control, uint32 action, uint32 data, void *cookie );
void spnewgamescreen_worldTypeSelect(aui_Control *control, uint32 action, uint32 data, void *cookie );
void spnewgamescreen_worldShapeSelect(aui_Control *control, uint32 action, uint32 data, void *cookie );
void spnewgamescreen_difficultySelect(aui_Control *control, uint32 action, uint32 data, void *cookie );
void spnewgamescreen_riskLevelSelect(aui_Control *control, uint32 action, uint32 data, void *cookie );
void spnewgamescreen_opponentSelect(aui_Control *control, uint32 action, uint32 data, void *cookie );
void spnewgamescreen_wCustomPress(aui_Control *control, uint32 action, uint32 data, void *cookie );

void spnewgamescreen_genocidePress(aui_Control *control, uint32 action, uint32 data, void *cookie );
void spnewgamescreen_tradePress(aui_Control *control, uint32 action, uint32 data, void *cookie );
void spnewgamescreen_combatPress(aui_Control *control, uint32 action, uint32 data, void *cookie );
void spnewgamescreen_pollutionPress(aui_Control *control, uint32 action, uint32 data, void *cookie );

void spnewgamescreen_scenarioExitCallback(aui_Control *control, uint32 action, uint32 data, void *cookie );


class TwoChoiceButton : public ctp2_Button
{
public:
	TwoChoiceButton(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		MBCHAR *choiceOff, MBCHAR *choiceOn, uint32 onoff = 0,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	~TwoChoiceButton() override = default;

	uint32	GetChoice() { return m_choice; };
	uint32 Switch();
private:
	MBCHAR m_choices[2][ k_AUI_LDL_MAXBLOCK + 1 ];
	uint32	m_choice;
};


class SPProfileBox
{
public:
	SPProfileBox (
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock );
	virtual ~SPProfileBox();
	void	SetLeader(uint32 index);
private:

	std::unique_ptr<c3_DropDown>	m_spClan;
	std::unique_ptr<c3_DropDown>	m_spGender;
	std::unique_ptr<C3TextField>	m_spName;
	std::unique_ptr<ctp2_Button>	m_spPreferences;
	std::unique_ptr<ctp2_Button>	m_spCustom;

	std::unique_ptr<c3_Static>	m_PTOP;
	std::unique_ptr<c3_Static>	m_PHEADER;
	std::unique_ptr<c3_Static>	m_PBOT;
	std::unique_ptr<c3_Static>	m_PLEFT;
	std::unique_ptr<c3_Static>	m_PRIGHT;
};


class SPWorldBox
{
public:
	SPWorldBox (
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock );
	virtual ~SPWorldBox();
	AUI_ERRCODE DrawThis(
		aui_Surface *surface,
		sint32 x,
		sint32 y );
	uint32 GetMapSize() { return m_mapSize->GetListBox()->GetSelectedItemIndex(); };
	uint32 GetWorldType() { return m_worldType->GetListBox()->GetSelectedItemIndex(); };
	uint32 GetWorldShape() { return m_worldShape->GetListBox()->GetSelectedItemIndex(); };
	uint32 GetDifficulty() { return m_difficulty->GetListBox()->GetSelectedItemIndex(); };
	uint32 GetRiskLevel() { return m_riskLevel->GetListBox()->GetSelectedItemIndex(); };
	uint32 GetNumOpponents() { return m_opponent->GetListBox()->GetSelectedItemIndex()+3; };

private:
	std::unique_ptr<c3_DropDown>	m_mapSize;
	std::unique_ptr<c3_DropDown>	m_worldType;
	std::unique_ptr<c3_DropDown>	m_worldShape;
	std::unique_ptr<c3_DropDown>	m_difficulty;
	std::unique_ptr<c3_DropDown>	m_riskLevel;
	std::unique_ptr<c3_DropDown>	m_opponent;
	std::unique_ptr<ctp2_Button>	m_spCustom;

	std::unique_ptr<c3_Static>	m_WTOP;
	std::unique_ptr<c3_Static>	m_WHEADER;
	std::unique_ptr<c3_Static>	m_WBOT;
	std::unique_ptr<c3_Static>	m_WLEFT;
	std::unique_ptr<c3_Static>	m_WRIGHT;

};


class SPRulesBox
{
public:
	SPRulesBox (
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock );
	virtual ~SPRulesBox();

	uint32 GetGenocideRules();
	uint32 GetTradeRules();
	uint32 GetCombatRules();
	uint32 GetPollutionRules();
private:
	std::unique_ptr<c3_CheckBox>	m_spGenocide;
	std::unique_ptr<c3_CheckBox>	m_spTrade;
	std::unique_ptr<c3_CheckBox>	m_spCombat;
	std::unique_ptr<c3_CheckBox>	m_spPollution;

	std::unique_ptr<c3_Static>	m_RTOP;
	std::unique_ptr<c3_Static>	m_RHEADER;
	std::unique_ptr<c3_Static>	m_RBOT;
	std::unique_ptr<c3_Static>	m_RLEFT;
	std::unique_ptr<c3_Static>	m_RRIGHT;
};


class SPNewGameWindow : public C3Window
{
public:
	SPNewGameWindow(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		sint32 bpp,
		AUI_WINDOW_TYPE type = AUI_WINDOW_TYPE_STANDARD,
		bool bevel = true);
	~SPNewGameWindow() override;

	void Update( );


	std::unique_ptr<ctp2_Button>	m_spStart;
	std::unique_ptr<c3_Static>	m_spOk;
	std::unique_ptr<ctp2_Button>	m_spReturn;


	std::unique_ptr<ctp2_Button>	m_spTribe;
	std::unique_ptr<ctp2_Button>	m_spDifficulty;
	std::unique_ptr<ctp2_Button>	m_spMapSize;
	std::unique_ptr<ctp2_Button>	m_spPlayers;
	std::unique_ptr<ctp2_Button>	m_spMap;
	std::unique_ptr<ctp2_Button>	m_spRules;

	std::unique_ptr<ctp2_Button>	m_spEditor;

	std::unique_ptr<ctp2_Button>	m_spScenario;

	std::unique_ptr<c3_Static>	m_spGeneral;
	std::unique_ptr<c3_Static>	m_spWorld;
	std::unique_ptr<c3_Static>	m_spCustom;

	std::unique_ptr<C3TextField>	m_spName;






	std::unique_ptr<ctp2_Button>	m_mapTypeButton;
	std::unique_ptr<c3_Static>	m_mapTypeLabel;
	std::unique_ptr<ctp2_Button>	m_worldShapeButton;
	std::unique_ptr<c3_Static>	m_worldShapeLabel;

	bool			m_useCustomMap;

	std::unique_ptr<c3_Static>	m_civilizationLabel;
	std::unique_ptr<c3_Static>	m_leaderNameLabel;
	std::unique_ptr<c3_Static>	m_difficultyLabel;
	std::unique_ptr<c3_Static>	m_worldSizeLabel;
	std::unique_ptr<c3_Static>	m_rulesLabel;
	std::unique_ptr<c3_Static>	m_playersLabel;
	std::unique_ptr<c3_Static>	m_worldTypeLabel;

	std::unique_ptr<ctp2_Button>	m_quitButton;

	std::unique_ptr<c3_Static>	m_spTitle;
	std::unique_ptr<c3_Static>	m_spBackground;

	std::unique_ptr<aui_StringTable>	m_string;

	std::unique_ptr<c3_Static>	m_scenarioName;
	std::unique_ptr<c3_Static>	m_scenarioStaticText;

};


class SPDropDownListItem : public c3_ListItem
{
public:
	SPDropDownListItem(AUI_ERRCODE *retval, MBCHAR const *ldlBlock,MBCHAR const *type,const MBCHAR *name);
	~SPDropDownListItem() override;
	sint32 Compare(c3_ListItem *item2, uint32 column) override{return 0; };
private:
	std::unique_ptr<c3_Static>	m_myItem;
};


sint32				callbackSetSelected(aui_Control *control, void *cookie);

aui_StringTable		*spNewStringTable(AUI_ERRCODE *errcode, MBCHAR const *ldlme);
c3_Button			*spNew_c3_Button(AUI_ERRCODE *errcode, MBCHAR const *ldlParent,MBCHAR const *ldlMe,
									void (*callback)(aui_Control*,uint32,uint32,void*));
ctp2_Button			*spNew_ctp2_Button(AUI_ERRCODE *errcode, MBCHAR const *ldlParent,MBCHAR const *ldlMe,
									   void (*callback)(aui_Control*,uint32,uint32,void*));
c3_Switch			*spNew_c3_Switch(AUI_ERRCODE *errcode, MBCHAR const *ldlParent,MBCHAR const *ldlMe,
									void (*callback)(aui_Control*,uint32,uint32,void*)=nullptr,
									void *cookie=nullptr );
aui_Switch			*spNew_aui_Switch(
									AUI_ERRCODE *errcode,
									MBCHAR const *ldlParent,MBCHAR const *ldlMe,
									void (*callback)(aui_Control*,uint32,uint32,void*)=nullptr,
									void *cookie=nullptr );
c3_Static			*spNew_c3_Static(AUI_ERRCODE *errcode, MBCHAR const *ldlParent,MBCHAR const *ldlMe);
C3TextField			*spNewTextEntry(AUI_ERRCODE *errcode, MBCHAR const *ldlParent,MBCHAR const *ldlMe,
									void (*callback)(aui_Control*,uint32,uint32,void*)=nullptr ,void *cookie=nullptr );
c3_ListBox			*spNew_c3_ListBox(AUI_ERRCODE *errcode, MBCHAR const *ldlParent,MBCHAR const *ldlMe,
									void (*callback)(aui_Control*,uint32,uint32,void*) = nullptr,
									void *cookie = nullptr);
c3_DropDown			*spNew_c3_DropDown(AUI_ERRCODE *errcode, MBCHAR const *ldlParent,MBCHAR const *ldlMe,void (*callback)(aui_Control*,uint32,uint32,void*));
void				spFillDropDown(AUI_ERRCODE *retval, c3_DropDown *mydrop, aui_StringTable *mytable, MBCHAR const *listitemparent, MBCHAR const *listitemme);
void				spFillListBox(AUI_ERRCODE *retval, c3_ListBox *mylist, aui_StringTable *mytable, MBCHAR *listitemparent, MBCHAR *listitemme);
TwoChoiceButton		*spNewTwoChoiceButton(AUI_ERRCODE *errcode, MBCHAR* ldlParent, MBCHAR const *ldlMe,MBCHAR *ldlstringtable,
									uint32 state,void (*callback)(aui_Control*,uint32,uint32,void*) = nullptr);
C3Slider			*spNew_C3Slider(AUI_ERRCODE *errcode, MBCHAR const *ldlParent, MBCHAR const *ldlMe,
	 								void (*callback)(aui_Control*,uint32,uint32,void*));
c3_CheckBox			*spNew_c3_CheckBox(AUI_ERRCODE *errcode, MBCHAR* ldlParent, MBCHAR const *ldlMe,
									uint32 state,void (*callback)(aui_Control*,uint32,uint32,void*) = nullptr, void *cookie = nullptr);
aui_SwitchGroup		*spNew_aui_SwitchGroup( AUI_ERRCODE *errcode, MBCHAR const *ldlParent, MBCHAR const *ldlMe );




ctp2_Button*
spNew_ctp2_Button(AUI_ERRCODE *errcode,
				  MBCHAR const *ldlParent,
				  MBCHAR const *ldlMe,
				  MBCHAR *default_text,
				  void (*callback)(aui_Control*,uint32,uint32,void*),
				  MBCHAR *buttonFlavor);

#endif
