#include "ctp/c3.h"
#include "ui/interface/battleviewwindow.h"

#include "gfx/gfx_utils/pixelutils.h"
#include "gfx/spritesys/battleviewactor.h"
#include "gfx/spritesys/director.h"  // director_Get()
#include "gfx/tilesys/tiledmap.h"
#include "gfx/tilesys/tileset.h"
#include "gs/events/GameEventManager.h"
#include "gs/gameobj/CTP2Combat.h"
#include "TerrainRecord.h"
#include "gs/world/World.h"  // world_Get()
#include "net/general/network.h"
#include "sound/soundmanager.h"
#include "ui/aui_common/aui_blitter.h"
#include "ui/aui_common/aui_control.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_stringtable.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_ctp2/SelItem.h"
#include "ui/aui_ctp2/c3_icon.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/ctp2_Static.h"
#include "ui/aui_ctp2/ctp2_button.h"
#include "ui/aui_utils/primitives.h"
#include "ui/interface/battle.h"
#include "gs/core/battle_observer.h"
#include "ui/interface/battleview.h"
#include "ui/ldl/ldl_data.hpp"
#include "ui/ldl/ldl_file.hpp"

extern sint32	g_modalWindow;

static BattleViewWindow		*g_battleViewWindow = NULL;

BattleViewWindow * battleviewwindow_Get()
{
	return g_battleViewWindow;
}


void battleview_ExitButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	AUI_ERRCODE auiErr = c3ui_Get()->RemoveWindow(g_battleViewWindow->Id());
	Assert( auiErr == AUI_ERRCODE_OK );
	if ( auiErr != AUI_ERRCODE_OK ) return;

	// Wave G: was `combat_Get()->GetBattle()` — gs/ no longer holds
	// a Battle*.  The active Battle is owned by the UI adapter; ask it
	// directly.
	extern Battle *BattleObserverAdapter_GetCurrentBattle();
	Battle *currentBattle = BattleObserverAdapter_GetCurrentBattle();
	RemoveBattleViewAction	*actionObj = new RemoveBattleViewAction(combat_Get() && g_battleViewWindow && g_battleViewWindow->GetBattleView() && currentBattle &&
																	g_battleViewWindow->GetBattleView()->IsCurrentBattle(currentBattle));
	c3ui_Get()->AddAction(actionObj);

}

void battleview_RetreatButtonActionCallback(aui_Control *control, uint32 action,
											uint32 data, void *cookie)
{

	if(action != (uint32)AUI_BUTTON_ACTION_EXECUTE)
		return;

	control->Enable(false);

	if(combat_Get())
		combat_Get()->Retreat();
}


void RemoveBattleViewAction::Execute(aui_Control *control, uint32 action, uint32 data)
{

	if(combat_Get() && m_killBattle) {
		// Wave G: was `combat_Get()->KillBattle()` which did
		// `delete m_battle; m_battle = NULL;` on the gs-held Battle*.
		// Now the adapter owns the pointer; tell it to end + clear gs's
		// active flag.
		battle_observer::EndBattle();
		combat_Get()->DeactivateBattle();
	}


	gevmanager_Get()->GotUserInput();




	BattleViewWindow::Cleanup();
}


void BattleViewWindow::Initialize(SequenceWeakPtr seq)
{
	AUI_ERRCODE		errcode;

	if (g_battleViewWindow)
		Cleanup();

	g_battleViewWindow = new BattleViewWindow( &errcode, aui_UniqueId(), "BattleViewWindow", 16,
											AUI_WINDOW_TYPE_POPUP);

	g_battleViewWindow->SetSequence(seq);

	g_modalWindow++;

	Assert(g_battleViewWindow != NULL);
}


void BattleViewWindow::Cleanup()
{
	SequenceWeakPtr	seq;

	if (g_battleViewWindow) {
		seq = g_battleViewWindow->GetSequence();

		c3ui_Get()->RemoveWindow(g_battleViewWindow->Id());

		delete g_battleViewWindow;
		g_battleViewWindow = NULL;
	}

	g_modalWindow--;

	director_Get()->ActionFinished(seq);
}


BattleViewWindow::BattleViewWindow
(
    AUI_ERRCODE *   retval,
	uint32          id,
    MBCHAR *        ldlBlock,
    sint32          bpp,
    AUI_WINDOW_TYPE type
)
:
	C3Window                (retval, id, ldlBlock, bpp, type),
	m_battleView            (NULL),
//	RECT					m_battleViewRect;
	m_topBorder             (NULL),
    m_leftBorder            (NULL),
    m_rightBorder           (NULL),
    m_bottomBorder          (NULL),
    m_exitButton            (NULL),
    m_retreatButton         (NULL),
    m_titleText             (NULL),
    m_attackersText         (NULL),
    m_attackersName         (NULL),
    m_attackersFlag         (NULL),
    m_defendersText         (NULL),
    m_defendersName         (NULL),
    m_defendersFlag         (NULL),
    m_terrainBonusText      (NULL),
    m_terrainBonusValue     (NULL),
    m_cityBonusText         (NULL),
    m_cityBonusValue        (NULL),
    m_citylandattackBonusText (NULL),
    m_citylandattackBonusValue (NULL),
    m_cityairattackBonusText (NULL),
    m_cityairattackBonusValue (NULL),
    m_cityseaattackBonusText (NULL),
    m_cityseaattackBonusValue (NULL),
    m_cityName              (NULL),
    m_fortBonusText         (NULL),
    m_fortBonusValue        (NULL),
    m_fortBonusImage        (NULL),
    m_fortifiedBonusText    (NULL),
    m_fortifiedBonusValue   (NULL)
{
	InitCommonLdl(ldlBlock);
}


BattleViewWindow::~BattleViewWindow()
{
	delete m_battleView;
	delete m_topBorder;
	delete m_leftBorder;
	delete m_rightBorder;
	delete m_bottomBorder;
	delete m_exitButton;
	delete m_retreatButton;
	delete m_titleText;
	delete m_attackersText;
	delete m_attackersName;
	delete m_attackersFlag;
	delete m_defendersText;
	delete m_defendersName;
	delete m_defendersFlag;
	delete m_terrainBonusText;
	delete m_terrainBonusValue;
	delete m_cityBonusText;
	delete m_cityBonusValue;
	delete m_citylandattackBonusText;
	delete m_citylandattackBonusValue;
	delete m_cityairattackBonusText;
	delete m_cityairattackBonusValue;
	delete m_cityseaattackBonusText;
	delete m_cityseaattackBonusValue;
	delete m_cityName;
	delete m_fortBonusText;
	delete m_fortBonusValue;
	delete m_fortBonusImage;
	delete m_fortifiedBonusText;
	delete m_fortifiedBonusValue;

	Assert(this == g_battleViewWindow);
	if (this == g_battleViewWindow)
    {
		g_battleViewWindow = NULL;
    }
}


AUI_ERRCODE BattleViewWindow::InitCommonLdl(MBCHAR *ldlBlock)
{
	MBCHAR			buttonBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	AUI_ERRCODE		errcode;

	m_battleView = new BattleView();
	Assert(m_battleView != NULL);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "TitleText");
	m_titleText = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_titleText);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "AttackersText");
	m_attackersText = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_attackersText);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "AttackersName");
	m_attackersName = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_attackersName);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "AttackersFlag");
	m_attackersFlag = new c3_Icon(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_attackersFlag);
	m_attackersFlag->SetMapIcon( MAPICON_FLAG );

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "DefendersText");
	m_defendersText = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_defendersText);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "DefendersName");
	m_defendersName = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_defendersName);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "DefendersFlag");
	m_defendersFlag = new c3_Icon(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_defendersFlag);
	m_defendersFlag->SetMapIcon( MAPICON_FLAG );

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "TerrainBonusText");
	m_terrainBonusText = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_terrainBonusText);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "TerrainBonusValue");
	m_terrainBonusValue = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_terrainBonusValue);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "CityBonusText");
	m_cityBonusText = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_cityBonusText);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "CityBonusValue");
	m_cityBonusValue = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_cityBonusValue);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "CityLandAttackBonusText");
	m_citylandattackBonusText = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_citylandattackBonusText);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "CityLandAttackBonusValue");
	m_citylandattackBonusValue = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_citylandattackBonusValue);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "CityAirAttackBonusText");
	m_cityairattackBonusText = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_cityairattackBonusText);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "CityAirAttackBonusValue");
	m_cityairattackBonusValue = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_cityairattackBonusValue);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "CitySeaAttackBonusText");
	m_cityseaattackBonusText = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_cityseaattackBonusText);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "CitySeaAttackBonusValue");
	m_cityseaattackBonusValue = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_cityseaattackBonusValue);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "CityName");
	m_cityName = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_cityName);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "FortBonusText");
	m_fortBonusText = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_fortBonusText);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "FortBonusValue");
	m_fortBonusValue = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_fortBonusValue);

	m_fortBonusImage = NULL;

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "FortifiedBonusText");
	m_fortifiedBonusText = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_fortifiedBonusText);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "FortifiedBonusValue");
	m_fortifiedBonusValue = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_fortifiedBonusValue);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "ExitButton");
	m_exitButton = new ctp2_Button(&errcode, aui_UniqueId(), buttonBlock,
		battleview_ExitButtonActionCallback);
	Assert(m_exitButton != NULL);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "RetreatButton");
	m_retreatButton = new ctp2_Button(&errcode, aui_UniqueId(), buttonBlock,
		battleview_RetreatButtonActionCallback);
	Assert(m_retreatButton != NULL);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "TopBorder");
	m_topBorder = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_topBorder);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "LeftBorder");
	m_leftBorder = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_leftBorder);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "RightBorder");
	m_rightBorder = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_rightBorder);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "BottomBorder");
	m_bottomBorder = new ctp2_Static(&errcode, aui_UniqueId(), buttonBlock);
	Assert(m_bottomBorder);

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "BattleViewArea");

    ldl_datablock *block = aui_Ldl::GetLdl()->FindDataBlock( buttonBlock );
	Assert(block);
	if (!block) return AUI_ERRCODE_OK;

	if ( block->GetAttributeType( k_AUI_LDL_HABSPOSITION ) == ATTRIBUTE_TYPE_INT )	{
		m_battleViewRect.left = block->GetInt( k_AUI_LDL_HABSPOSITION );
	}

	if ( block->GetAttributeType( k_AUI_LDL_VABSPOSITION ) == ATTRIBUTE_TYPE_INT )	{
		m_battleViewRect.top = block->GetInt( k_AUI_LDL_VABSPOSITION );
	}

	if ( block->GetAttributeType( k_AUI_LDL_HABSSIZE ) == ATTRIBUTE_TYPE_INT )	{
		m_battleViewRect.right = m_battleViewRect.left + block->GetInt( k_AUI_LDL_HABSSIZE );
	}

	if ( block->GetAttributeType( k_AUI_LDL_VABSSIZE ) == ATTRIBUTE_TYPE_INT )	{
		m_battleViewRect.bottom = m_battleViewRect.top + block->GetInt( k_AUI_LDL_VABSSIZE );
	}

	errcode = aui_Ldl::SetupHeirarchyFromRoot( ldlBlock );

	return AUI_ERRCODE_OK;
}

void BattleViewWindow::SetupBattle(Battle *battle)
{
	if(!m_battleView)
		return;
	if(!battle)
		return;
	if(!combat_Get())
		return;

	if (combat_Get()->GetAttacker() ==
		    selitem_Get()->GetVisiblePlayer()
	    && !g_network.IsActive()
       )
    {
		m_retreatButton->Show();
		m_retreatButton->Enable(true);
	}
    else
    {
		m_retreatButton->Hide();
	}

	RECT battleRect = m_battleViewRect;
	OffsetRect(&battleRect, -battleRect.left, -battleRect.top);

	m_battleView->Initialize(battleRect);
	m_battleView->SetBattle(battle);

	sint32	terrainType = battle->GetTerrainType();
	sint32  attackerTerrain = battle->GetAttackersTerrainType();

	AUI_ERRCODE	errcode = AUI_ERRCODE_OK;
	aui_StringTable	*table = new aui_StringTable(&errcode, "BattleViewTerrainTable");
	Assert(errcode == AUI_ERRCODE_OK);
	MBCHAR *imageName = NULL;

	const TerrainRecord *defTerrRec = g_theTerrainDB->Get(terrainType);
	const TerrainRecord *attackTerrRec = g_theTerrainDB->Get(attackerTerrain);

	bool useSplit = false;

	if(!defTerrRec->GetMovementTypeSea() && attackTerrRec->GetMovementTypeSea()) {
		useSplit = true;
		imageName = "UPBG006.TGA";
	} else {
		imageName = table->GetString(terrainType);
	}

	aui_Image *image = c3ui_Get()->LoadImage(imageName);
	Assert(image);
	if(image)
		m_battleView->SetBackgroundImage(image);

	delete table;

	double bonus = battle->GetTerrainBonus();
	MBCHAR s[k_MAX_NAME_LEN];
	snprintf(s, sizeof(s), "+%d%%", (sint32)(bonus * 100.0));
	m_terrainBonusValue->SetText(s);

	if(battle->GetCityImage() != -1) {

		aui_StringTable	*cityTable = new aui_StringTable(&errcode, "BattleViewCityTable");
		Assert(errcode == AUI_ERRCODE_OK);
		Assert(cityTable);





		aui_Image *cityImage = c3ui_Get()->LoadImage(useSplit ? "UPBO006.tga" : cityTable->GetString(terrainType));
		m_battleView->SetCityImage(cityImage);
		delete cityTable;

		m_cityName->SetText(battle->GetCityName());
		m_cityName->Show();

	} else {

		m_cityName->Hide();

	}

	bonus = battle->GetCityBonus();
	snprintf(s, sizeof(s), "+%d", (sint32)bonus);
	m_cityBonusValue->SetText(s);

	bonus = battle->GetCityLandAttackBonus();
	snprintf(s, sizeof(s), "+%d", (sint32)bonus);
	m_citylandattackBonusValue->SetText(s);

	bonus = battle->GetCityAirAttackBonus();
	snprintf(s, sizeof(s), "+%d", (sint32)bonus);
	m_cityairattackBonusValue->SetText(s);

	bonus = battle->GetCitySeaAttackBonus();
	snprintf(s, sizeof(s), "+%d", (sint32)bonus);
	m_cityseaattackBonusValue->SetText(s);

	bonus = battle->GetFortBonus();
	snprintf(s, sizeof(s), "+%d%%", (sint32)(bonus * 100.0));
	m_fortBonusValue->SetText(s);

	bonus = battle->GetFortifiedBonus();
	snprintf(s, sizeof(s), "+%d%%", (sint32)(bonus * 100.0));
	m_fortifiedBonusValue->SetText(s);

	m_attackersFlag->SetColor( battle->GetAttackersColor() );
	m_defendersFlag->SetColor( battle->GetDefendersColor() );

	MBCHAR name[_MAX_PATH];
	battle->GetAttackersName(name);
	m_attackersName->SetText(name);
	battle->GetDefendersName(name);
	m_defendersName->SetText(name);

	if(soundmgr_Get()) {
		soundmgr_Get()->TerminateAllLoopingSounds(SOUNDTYPE_SFX);
		soundmgr_Get()->TerminateAllLoopingSounds(SOUNDTYPE_VOICE);
	}
}

void BattleViewWindow::UpdateBattle(Battle *battle)
{

	if(!m_battleView)
		return;
	if(!battle)
		return;

	m_battleView->UpdateBattle(battle);
}

void BattleViewWindow::EndBattle()
{

	m_retreatButton->Enable(false);
}

void BattleViewWindow::RemoveActor(BattleViewActor *actor)
{
	Assert(m_battleView);
	if (!m_battleView) return;
	m_battleView->RemoveActor(actor);
}


AUI_ERRCODE BattleViewWindow::DrawThis(aui_Surface *surface, sint32 x, sint32 y)
{

	if ( IsHidden() ) return AUI_ERRCODE_OK;

	C3Window::DrawThis(surface, x, y);

	if (!surface) surface = TheSurface();

	RECT rect = {0, 0, Width(), Height()};

	rect = m_battleViewRect;
	OffsetRect(&rect, -rect.left, -rect.top);

	if (m_battleView && m_battleView->GetBattleSurface())
		c3ui_Get()->TheBlitter()->Blt(surface, m_battleViewRect.left, m_battleViewRect.top,
			m_battleView->GetBattleSurface(), &rect, k_AUI_BLITTER_FLAG_COPY);

	m_dirtyList->AddRect( &rect );

	rect = m_battleViewRect;
	InflateRect(&rect, 5, 5);
	primitives_BevelRect16( surface, &rect, 5,1, 16, 16 );

	m_dirtyList->AddRect( &rect );

	return AUI_ERRCODE_OK;
}




AUI_ERRCODE BattleViewWindow::Idle()
{
	if(!tiledmap_Get()) {

		return AUI_ERRCODE_OK;
	}

	director_Get()->UpdateTimingClock();

	Refresh();

	return AUI_ERRCODE_OK;
}

#define k_BATTLE_VIEW_FRAME_TIME		50

void BattleViewWindow::Refresh()
{
	if(!m_battleView)
		return;

	static uint32 lastUpdate = 0;

	if (GetTickCount() > (lastUpdate + k_BATTLE_VIEW_FRAME_TIME)) {
		m_battleView->Process();

		lastUpdate = GetTickCount();

		m_battleView->UpdateDisplay();

		ShouldDraw(TRUE);
	}
}

void BattleViewWindow::GetAttackerPos(sint32 column, sint32 row, sint32 *x, sint32 *y)
{
	Assert(m_battleView);

	if (m_battleView)
		m_battleView->GetAttackerPos(column, row, x, y);
}

void BattleViewWindow::GetDefenderPos(sint32 column, sint32 row, sint32 *x, sint32 *y)
{
	Assert(m_battleView);

	if (m_battleView)
		m_battleView->GetDefenderPos(column, row, x, y);
}
