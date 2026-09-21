//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Single player new game rules screen
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
// THIS FILE MAPS TO SPNEWGAMEPOPUPS.LDL
// Modifications from the original Activision code:
//
// - 7 options total needed to implement a new rule
//
// - Memory leaks repaired.
// - Initialized local variables. (Sep 9th 2005 Martin Gühmann)
// - Removed new rules attempt - E 12.27.2006
// - Added citycapture as an option and mapped it to gameplayoptions
//   but appears to have no affect
// - Changed whole file to look more like gameplayoptions 3.21.2007
// - Added revoltinsurgents option 3.22.2007
// - Added NoRandomCivs option
// - Added aifreeupgrades
// - added unit gold support
// - added city gold support
// - Added no gold deficit for cities
// - Added no production deficit for cities
// - Added no gold hunger for ai
// - Added no shield hunger for ai
// - Added an upgrade option (13-Sep-2008 Martin Gühmann)
// - Added a new combat option (28-Feb-2009 Maq)
// - Added a no goody huts option (20-Mar-2009 Maq)
// - Added custom start/end "ages" button. (11-Apr-2009 Maq)
// - Removed AI specific rules from window, and added mouse-over description
//   for each existing rule. (25-Jul-2009 Maq)
// - Removed sectarian happiness rule as it isn't doing anything.(25-Jul-2009 Maq)
//
//----------------------------------------------------------------------------

#include <memory>

#include "ctp/c3.h"
#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_ctp2/c3_popupwindow.h"
#include "ui/aui_ctp2/c3_button.h"
#include "ui/aui_ctp2/c3_listitem.h"
#include "ui/aui_ctp2/c3_dropdown.h"
#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/c3slider.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_common/aui_stringtable.h"
#include "ui/aui_common/aui_switch.h"
#include "ui/aui_common/aui_uniqueid.h"

#include "gs/database/profileDB.h"

#include "ui/interface/spnewgamewindow.h"
#include "ui/interface/spnewgamerulesscreen.h"
#include "ui/interface/agesscreen.h"

#include "ui/aui_ctp2/keypress.h"
//missing?
#include "gs/gameobj/GameSettings.h"
#include "ui/interface/screenutils.h"
#include "net/general/network.h"


static std::unique_ptr<c3_PopupWindow>	s_spNewGameRulesScreen;
static std::unique_ptr<aui_Switch>		s_genocide,
						s_pollution,
						s_citycapture,
						s_onecity,
						s_revoltinsurgent,
						s_revoltcasualty,
						s_barbspawn,
						s_NonRandomCivs,
						s_Upgrade,
						s_NewCombat,
						s_NoGoodyHuts,
						s_UNITGOLD,
						s_CITYGOLD,
						s_NOCITYLIMIT;
std::unique_ptr<ctp2_Button>			s_ages;
static std::unique_ptr<c3_Static>		m_ruleDetails;
static std::unique_ptr<aui_StringTable>	m_ruleDetailsStrings;

enum
{
	R_GENOCIDE,
	R_POLLUTION,
	GP_CITYCAPTURE,
	R_ONECITY,
	R_INSURGENT,
	R_CASUALTY,
	R_BARBSPAWN,
	R_NonRandomCivs,
	R_UPGRADE,
	R_NEWCOMBAT,
	R_NOGOODYHUTS,
	R_UNITGOLD,
	R_CITYGOLD,
	R_NOCITYLIMIT,
	GP_TOTAL
};

static uint32 check[] =
{
	R_GENOCIDE,
	R_POLLUTION,
	GP_CITYCAPTURE,
	R_ONECITY,
	R_INSURGENT,
	R_CASUALTY,
	R_BARBSPAWN,
	R_NonRandomCivs,
	R_UPGRADE,
	R_NEWCOMBAT,
	R_NOGOODYHUTS,
	R_UNITGOLD,
	R_CITYGOLD,
	R_NOCITYLIMIT,

	GP_TOTAL
};

sint32 spnewgamerulesscreen_updateData()
{
	if(!profiledb_Get()) return -1;

	s_genocide       ->SetState( profiledb_Get()->IsGenocideRule            () );
	s_pollution      ->SetState( profiledb_Get()->IsPollutionRule           () );
	s_citycapture    ->SetState( profiledb_Get()->IsCityCaptureOptions      () );
	s_onecity        ->SetState( profiledb_Get()->IsOneCityChallenge        () );
	s_revoltinsurgent->SetState( profiledb_Get()->IsRevoltInsurgents        () );
	s_revoltcasualty ->SetState( profiledb_Get()->IsRevoltCasualties        () );
	s_barbspawn      ->SetState( profiledb_Get()->IsBarbarianSpawnsBarbarian() );
	s_NonRandomCivs  ->SetState( profiledb_Get()->IsNonRandomCivs           () );
	s_Upgrade        ->SetState( profiledb_Get()->IsUpgrade                 () );
	s_NewCombat      ->SetState( profiledb_Get()->IsNewCombat               () );
	s_NoGoodyHuts    ->SetState( profiledb_Get()->IsNoGoodyHuts             () );
	s_UNITGOLD       ->SetState( profiledb_Get()->IsGoldPerUnitSupport      () );
	s_CITYGOLD       ->SetState( profiledb_Get()->IsGoldPerCity             () );
	s_NOCITYLIMIT    ->SetState( profiledb_Get()->IsNoCityLimit             () );

	return 1;
}

sint32	spnewgamerulesscreen_displayMyWindow()
{
	sint32 retval=0;
	if(!s_spNewGameRulesScreen) { retval = spnewgamerulesscreen_Initialize(); }

	spnewgamerulesscreen_updateData();

	AUI_ERRCODE auiErr;

	auiErr = c3ui_Get()->AddWindow(s_spNewGameRulesScreen.get());
	keypress_RegisterHandler(s_spNewGameRulesScreen.get());

	Assert( auiErr == AUI_ERRCODE_OK );

	return retval;
}
sint32 spnewgamerulesscreen_removeMyWindow(uint32 action)
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return 0;

	AUI_ERRCODE auiErr;

	auiErr = c3ui_Get()->RemoveWindow( s_spNewGameRulesScreen->Id() );
	keypress_RemoveHandler(s_spNewGameRulesScreen.get());

	Assert( auiErr == AUI_ERRCODE_OK );

	spnewgamescreen_update();

	return 1;
}


AUI_ERRCODE spnewgamerulesscreen_Initialize( )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	if ( s_spNewGameRulesScreen ) return AUI_ERRCODE_OK;

	strlcpy(windowBlock, "SPNewGameRulesScreen", sizeof(windowBlock));

	s_spNewGameRulesScreen = std::make_unique<c3_PopupWindow>( &errcode, aui_UniqueId(), windowBlock, 16, AUI_WINDOW_TYPE_FLOATING, false);
		Assert( AUI_NEWOK(s_spNewGameRulesScreen, errcode) );
		if ( !AUI_NEWOK(s_spNewGameRulesScreen, errcode) ) return errcode;

		s_spNewGameRulesScreen->Resize(s_spNewGameRulesScreen->Width(),s_spNewGameRulesScreen->Height());
		s_spNewGameRulesScreen->GrabRegion()->Resize(s_spNewGameRulesScreen->Width(),s_spNewGameRulesScreen->Height());
		s_spNewGameRulesScreen->SetStronglyModal(TRUE);

	s_genocide			.reset(spNew_aui_Switch(&errcode, windowBlock, "RuleOne",             spnewgamerulesscreen_checkPress, &check[R_GENOCIDE     ]));
	s_pollution			.reset(spNew_aui_Switch(&errcode, windowBlock, "RuleTwo",             spnewgamerulesscreen_checkPress, &check[R_POLLUTION    ]));
	s_citycapture		.reset(spNew_aui_Switch(&errcode, windowBlock, "CityCapture",         spnewgamerulesscreen_checkPress, &check[GP_CITYCAPTURE ])); //emod5
	s_onecity			.reset(spNew_aui_Switch(&errcode, windowBlock, "OneCity",             spnewgamerulesscreen_checkPress, &check[R_ONECITY      ])); //emod5
	s_revoltinsurgent	.reset(spNew_aui_Switch(&errcode, windowBlock, "RevoltInsurgents",    spnewgamerulesscreen_checkPress, &check[R_INSURGENT    ])); //emod5
	s_revoltcasualty	.reset(spNew_aui_Switch(&errcode, windowBlock, "RevoltCasualties",    spnewgamerulesscreen_checkPress, &check[R_CASUALTY     ])); //emod5
	s_barbspawn			.reset(spNew_aui_Switch(&errcode, windowBlock, "BarbSpawn",           spnewgamerulesscreen_checkPress, &check[R_BARBSPAWN    ])); //emod5
	s_NonRandomCivs		.reset(spNew_aui_Switch(&errcode, windowBlock, "NonRandomCivs",       spnewgamerulesscreen_checkPress, &check[R_NonRandomCivs])); //emod5
	s_Upgrade			.reset(spNew_aui_Switch(&errcode, windowBlock, "Upgrade",             spnewgamerulesscreen_checkPress, &check[R_UPGRADE      ])); //emod5
	s_NewCombat			.reset(spNew_aui_Switch(&errcode, windowBlock, "NewCombat",           spnewgamerulesscreen_checkPress, &check[R_NEWCOMBAT    ]));
	s_NoGoodyHuts		.reset(spNew_aui_Switch(&errcode, windowBlock, "NoGoodyHuts",         spnewgamerulesscreen_checkPress, &check[R_NOGOODYHUTS  ]));
	s_UNITGOLD			.reset(spNew_aui_Switch(&errcode, windowBlock, "UnitGold",            spnewgamerulesscreen_checkPress, &check[R_UNITGOLD     ])); //emod5
	s_CITYGOLD			.reset(spNew_aui_Switch(&errcode, windowBlock, "CityGold",            spnewgamerulesscreen_checkPress, &check[R_CITYGOLD     ])); //emod5
	s_NOCITYLIMIT		.reset(spNew_aui_Switch(&errcode, windowBlock, "NoCityLimit",         spnewgamerulesscreen_checkPress, &check[R_NOCITYLIMIT  ])); //emod5

	s_ages				.reset(spNew_ctp2_Button(&errcode, windowBlock, "AgesButton", spnewgamerulesscreen_agesPress));

	m_ruleDetails		.reset(spNew_c3_Static(&errcode,windowBlock, "RuleDetail"));

	//AUI_ERRCODE	errcode = AUI_ERRCODE_OK;
	m_ruleDetailsStrings = std::make_unique<aui_StringTable>(&errcode, "RuleDetailsStringTable");
	//Assert(errcode == AUI_ERRCODE_OK);

	spnewgamerulesscreen_updateData();

	MBCHAR block[ k_AUI_LDL_MAXBLOCK + 1 ];
	snprintf(block, sizeof(block), "%s.%s", windowBlock, "Name" );
	s_spNewGameRulesScreen->AddTitle( block );
	s_spNewGameRulesScreen->AddClose( spnewgamerulesscreen_exitPress );

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );

	return AUI_ERRCODE_OK;
}

AUI_ERRCODE spnewgamerulesscreen_Cleanup()
{
	if ( !s_spNewGameRulesScreen  ) return AUI_ERRCODE_OK;

	c3ui_Get()->RemoveWindow( s_spNewGameRulesScreen->Id() );
	keypress_RemoveHandler(s_spNewGameRulesScreen.get());

	// unique_ptr statics; reset in the original explicit order.
	s_genocide.reset();
	s_pollution.reset();
	s_citycapture.reset();
	s_onecity.reset();
	s_revoltinsurgent.reset();
	s_revoltcasualty.reset();
	s_barbspawn.reset();
	s_NonRandomCivs.reset();
	s_Upgrade.reset();
	s_NewCombat.reset();
	s_NoGoodyHuts.reset();
	s_UNITGOLD.reset();
	s_CITYGOLD.reset();
	s_NOCITYLIMIT.reset();
	s_ages.reset();
	m_ruleDetails.reset();

	m_ruleDetailsStrings.reset();
	s_spNewGameRulesScreen.reset();

	return AUI_ERRCODE_OK;
}

void spnewgamerulesscreen_agesPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if (m_ruleDetails && m_ruleDetailsStrings)
	{
		m_ruleDetails->SetText("");
		m_ruleDetails->Hide();
		m_ruleDetails->Show();

		char buf[1024];
		snprintf(buf, sizeof(buf), "%s", m_ruleDetailsStrings->GetString(14));

		m_ruleDetails->SetText(buf);
	}

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	agesscreen_displayMyWindow(false);
}

void spnewgamerulesscreen_checkPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	uint32 checkbox = *((uint32*)cookie);

	sint32 rule = -1;

	if (m_ruleDetails && m_ruleDetailsStrings)
	{
		m_ruleDetails->SetText("");
		m_ruleDetails->Hide();

		switch(checkbox)
		{
			case R_GENOCIDE     : rule = 0; break;
			case R_POLLUTION    : rule = 1; break;
			case GP_CITYCAPTURE : rule = 2; break;
			case R_ONECITY      : rule = 3; break;
			case R_INSURGENT    : rule = 4; break;
			case R_CASUALTY     : rule = 5; break;
			case R_BARBSPAWN    : rule = 6; break;
			case R_NonRandomCivs: rule = 7; break;
			case R_UPGRADE      : rule = 8; break;
			case R_NEWCOMBAT    : rule = 9; break;
			case R_NOGOODYHUTS  : rule = 10; break;
			case R_UNITGOLD     : rule = 11; break;
			case R_CITYGOLD     : rule = 12; break;
			case R_NOCITYLIMIT  : rule = 13; break;
			default             : Assert(0); break;
		};

		if (rule >= 0)
		{
			m_ruleDetails->Show();
			m_ruleDetails->SetText("");

			char buf[1024];
			snprintf(buf, sizeof(buf), "%s", m_ruleDetailsStrings->GetString(rule));
			m_ruleDetails->SetText(buf);
		}
		else
		{
			m_ruleDetails->SetText("");
			m_ruleDetails->Hide();
		}
	}

	if ( action != (uint32)AUI_SWITCH_ACTION_PRESS ) return;

	void (ProfileDB::*func)(BOOL) = nullptr;
	uint32 state = data;

	switch(checkbox)
	{
		case R_GENOCIDE     : func = &ProfileDB::SetGenocideRule            ; break;
		case R_POLLUTION    : func = &ProfileDB::SetPollutionRule           ; break;
		case GP_CITYCAPTURE : func = &ProfileDB::SetCityCaptureOptions      ; break;
		case R_ONECITY      : func = &ProfileDB::SetOneCity                 ; break;
		case R_INSURGENT    : func = &ProfileDB::SetRevoltInsurgents        ; break;
		case R_CASUALTY     : func = &ProfileDB::SetRevoltCasualties        ; break;
		case R_BARBSPAWN    : func = &ProfileDB::SetBarbarianSpawnsBarbarian; break;
		case R_NonRandomCivs: func = &ProfileDB::SetNonRandomCivs           ; break;
		case R_UPGRADE      : func = &ProfileDB::SetUpgrade                 ; break;
		case R_NEWCOMBAT    : func = &ProfileDB::SetNewCombat               ; break;
		case R_NOGOODYHUTS  : func = &ProfileDB::SetNoGoodyHuts             ; break;
		case R_UNITGOLD     : func = &ProfileDB::SetGoldPerUnitSupport      ; break;
		case R_CITYGOLD     : func = &ProfileDB::SetGoldPerCity             ; break;
		case R_NOCITYLIMIT  : func = &ProfileDB::SetNoCityLimit             ; break;
		default             : Assert(0)                                     ; break;
	};

	if(func)
		(profiledb_Get()->*func)(state ? FALSE : TRUE);
}

void spnewgamerulesscreen_exitPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	profiledb_Get()->Save();

	spnewgamerulesscreen_removeMyWindow(action);
}
