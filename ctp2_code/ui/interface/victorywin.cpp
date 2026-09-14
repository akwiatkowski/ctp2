//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : The victory window plays the win/lose video
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
// - Removed references to the old civilisation database. (Aug 20th 2005 Martin G�hmann)
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
// - Cleaned up static data.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/interface/victorywin.h"

#include <array>
#include <memory>

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_uniqueid.h"

#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/ctp2_Static.h"

#include "ui/aui_ctp2/textbutton.h"
#include "ui/aui_ctp2/c3_button.h"

#include "ui/aui_ctp2/ctp2_button.h"
#include "ui/aui_ctp2/ctp2_Window.h"
#include "ui/aui_ctp2/ctp2_listbox.h"
#include "ui/aui_ctp2/ctp2_listitem.h"

#include "ui/aui_ctp2/c3listbox.h"
#include "ui/aui_ctp2/c3_listbox.h"
#include "ui/aui_common/aui_listbox.h"

#include "ui/aui_ctp2/c3_dropdown.h"

#include "ui/aui_ctp2/texttab.h"
#include "ui/aui_common/aui_tabgroup.h"

#include "ui/aui_ctp2/ctp2_TabGroup.h"
#include "ui/aui_ctp2/ctp2_Tab.h"

#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_ctp2/c3windows.h"
#include "ui/interface/victorywindow.h"
#include "ui/interface/infowin.h"
#include "ui/aui_common/aui_stringtable.h"
#include "ui/aui_ctp2/c3_popupwindow.h"
#include "ui/interface/screenutils.h"

#include "gs/fileio/CivPaths.h"           // civpaths_Get()
#include "gs/gameobj/ObjPool.h"
#include "gs/world/Cell.h"
#include "gs/world/MapPoint.h"
#include "gfx/tilesys/tiledmap.h"
#include "ui/aui_ctp2/SelItem.h"
#include "ctp/ctp2_utils/c3files.h"
#include "ctp/ctp2_utils/pointerlist.h"
#include "ui/aui_ctp2/linegraph.h"
#include "gs/utility/TurnCnt.h"
#include "gs/gameobj/Strengths.h"
#include "gs/gameobj/UnitPool.h"
#include "gs/gameobj/Score.h"

#include "gfx/gfx_utils/pixelutils.h"
#include "ui/aui_ctp2/radarmap.h"




#include "gs/gameobj/player.h"
#include "gs/newdb/UnitRec.h"
#include "gs/gameobj/XY_Coordinates.h"
#include "gs/world/World.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/UnitData.h"
#include "gs/utility/UnitDynArr.h"
#include "gs/gameobj/citydata.h"
#include "gs/database/StrDB.h"
#include "BuildingRecord.h"
#include "WonderRecord.h"
#include "TerrainRecord.h"
#include "gs/gameobj/TopTen.h"
#include "AgeRecord.h"
#include "gs/database/highscoredb.h"
#include "gs/gameobj/GameSettings.h"

#include "ui/aui_ctp2/c3_listitem.h"
#include "ctp/civapp.h"
#include "gs/gameobj/Civilisation.h"
#include "gs/gameobj/CivilisationData.h"
#include "gfx/gfx_utils/colorset.h"           // colorset_Get()
#include "IconRecord.h"
#include "ctp/ctp2_rsrc/resource.h"
#include "ui/aui_ctp2/thumbnailmap.h"
#include "net/general/network.h"
#include "gs/utility/TurnCnt.h"
#include "gs/gameobj/wonderutil.h"
#include "gs/gameobj/CivilisationPool.h"   // civilisationpool_Get()

extern sint32                   g_ScreenWidth;
extern sint32                   g_ScreenHeight;
extern PointerList<Player>      *g_deadPlayer;
extern sint32                   g_modalWindow;


static std::unique_ptr<VictoryWindow> g_victoryWindow;

static ctp2_Button              *s_okButton;

// Borrowed LDL controls indexed by the k_VICWIN_* enum; the LDL hierarchy
// owns them (deleted with VictoryWindow's DeleteHierarchyFromRoot).
static std::array<ctp2_Static *, k_VICWIN_STATIC_MAX> s_staticControls;
static std::unique_ptr<aui_StringTable> s_stringTable;

static std::unique_ptr<HighScoreWindowPopup>     s_highScoreWin;






static LineGraph                *s_graph;
static ctp2_ListBox             *s_graphList;
static ctp2_Button              *s_lineOrZeroSumButton;
static BOOL                     s_lineGraph;
static ctp2_ListBox             *s_scoreList;

static ctp2_ListBox             *s_wonderList;

static ctp2_Static              *s_wonderBlock;


// Borrowed wonder-icon controls; owned by the LDL hierarchy like the
// statics above (see the cleanup comment in victorywin_Cleanup).
static std::array<ctp2_Static *, k_VICWIN_WONDERICON_MAX> s_wonderIcons;


void VictoryWindowButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	AUI_ERRCODE auiErr;

	if ((ctp2_Button*)control == s_okButton)
	{

		auiErr = c3ui_Get()->RemoveWindow( g_victoryWindow->m_window->Id() );
		Assert( auiErr == AUI_ERRCODE_OK );
		if ( auiErr != AUI_ERRCODE_OK ) return;

		g_modalWindow--;

		s_highScoreWin->DisplayWindow();
	}
}

void LineOrZeroSumButtonActionCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{

	if( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	victorywin_SetLineGraph( s_lineGraph ? false : true);

	s_graph->RenderGraph();
	g_victoryWindow->m_window->Draw();

}

void HighScoreWinButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	HighScoreWindowPopup *popup = (HighScoreWindowPopup *)cookie;
	if (!popup) return;

	if ((ctp2_Button*)control == popup->m_continueButton.get())
	{

		popup->RemoveWindow();

		if(turn_Get()->IsHotSeat() || turn_Get()->IsEmail())
		{
			Player* player = player_Get(selitem_Get()->GetVisiblePlayer());
			if(!player
			||  player->m_isDead
			){
				if(player->IsRobot())
				{
					turn_Get()->EndThisTurnBeginNewTurn(FALSE);
				}
				else
				{
					selitem_Get()->SetPlayerOnScreen(selitem_Get()->GetCurPlayer());
				}
			}
		}
	}
        else if ((ctp2_Button*)control == popup->m_creditsButton.get())
	{
		open_CreditsScreen();
	}
	else if ((ctp2_Button*)control == popup->m_quitButton.get())
	{
		popup->RemoveWindow();
		if(network_Get().IsActive()
		|| network_Get().IsNetworkLaunch()
		){
			civapp_Get()->PostQuitToLobbyAction();
		}
		else
		{
			civapp_Get()->PostEndGameAction();
		}
	}
}

sint32 victorywin_SetLineGraph( BOOL lineGraph)
{
	s_lineGraph = lineGraph;

	if (s_lineGraph)
	{
		s_graph->SetGraphType(GRAPH_TYPE_LINE);
		s_lineOrZeroSumButton->SetText(stringdb_Get()->GetNameStr("str_ldl_ZEROSUM_BUTTON"));
	}
	else
	{
		s_graph->SetGraphType(GRAPH_TYPE_ZEROSUM);
		s_lineOrZeroSumButton->SetText(stringdb_Get()->GetNameStr("str_ldl_LINE_BUTTON"));
	}

	return 0;
}





sint32 victorywin_Initialize( sint32 type )
{
	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;
	MBCHAR			windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	if ( g_victoryWindow )
	{

		victorywin_UpdateData(type);

		return 0;
	}

	strlcpy(windowBlock, "VictoryWindow", sizeof(windowBlock));

	g_victoryWindow.reset(new VictoryWindow(&errcode));
	Assert( AUI_NEWOK(g_victoryWindow, errcode) );
	if ( !AUI_NEWOK(g_victoryWindow, errcode) ) return -1;







	g_victoryWindow->m_window->SetStronglyModal(TRUE);

	victorywin_Init_Controls(windowBlock);


	s_highScoreWin.reset(new HighScoreWindowPopup(type));

	s_stringTable.reset(new aui_StringTable( &errcode, "VictoryStrings" ));
	Assert( AUI_NEWOK(s_stringTable, errcode) );
	if ( !AUI_NEWOK(s_stringTable, errcode) ) return -2;

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );
	if ( !AUI_SUCCESS(errcode) ) return -1;

	return 0;
}

sint32 victorywin_DisplayWindow(sint32 type)
{
	AUI_ERRCODE		errcode;

	errcode = c3ui_Get()->AddWindow( g_victoryWindow->m_window );
	Assert( AUI_SUCCESS(errcode) );
	if ( !AUI_SUCCESS(errcode) ) return -1;

	g_modalWindow++;

	victorywin_UpdateData(type);

	return 0;
}

sint32 victorywin_RemoveWindow( )
{
	if ( c3ui_Get()->GetWindow(g_victoryWindow->m_window->Id()) ) {
		c3ui_Get()->RemoveWindow( g_victoryWindow->m_window->Id() );
		g_modalWindow--;
	}

	return 1;
}

void victorywin_Cleanup( )
{
    // The individual "s_wonderIcons[i]" items will be deleted through
    // DeleteHierarchyFromRoot(s_VictoryWindowBlock) in the destructor
    // of g_victoryWindow; the arrays above only index them.

    if (s_graphList)
    {
        s_graphList->Clear();
    }
    if (s_wonderList)
    {
        s_wonderList->Clear();
    }
    if (s_scoreList)
    {
        s_scoreList->Clear();
    }

    s_highScoreWin.reset();
    s_stringTable.reset();
    g_victoryWindow.reset();
}

sint32 victorywin_AddWonders( MBCHAR *windowBlock )
{
	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;
	MBCHAR			controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	int i = 0;

	s_wonderIcons.fill(nullptr);

	snprintf(controlBlock, sizeof(controlBlock), "%s", "TabGroup.Tab1.TabPanel.WonderList" );
	s_wonderList = (ctp2_ListBox *)aui_Ldl::GetObject(windowBlock, controlBlock);





	s_wonderList->SetAbsorbancy( FALSE );
	sint32 height = s_wonderList->Height();

	tech_WLList<ctp2_Static *> wonderList;
	ctp2_Static *item = nullptr;

	for ( i = 0; i < k_VICWIN_WONDERICON_MAX; i++ )
	{
		item = new ctp2_Static(&errcode, aui_UniqueId(), "VictoryWindow_WonderIcon");
		Assert( AUI_NEWOK(item,errcode) );
		if ( !AUI_NEWOK(item,errcode) ) return - 1;

		s_wonderIcons[i] = item;

		if ( (height -= item->Height()) < 0 )
			break;

		wonderList.AddTail( item );
	}
	ListPos pos = wonderList.GetHeadPosition();

	for ( i++; i < k_VICWIN_WONDERICON_MAX; i++ )
	{
		wonderList.GetNext( pos )->AddChild( item );

		item = new ctp2_Static(&errcode, aui_UniqueId(), "VictoryWindow_WonderIcon");
		Assert( AUI_NEWOK(item,errcode) );
		if ( !AUI_NEWOK(item,errcode) ) return AUI_ERRCODE_MEMALLOCFAILED;

		s_wonderIcons[i] = item;

		if ( !pos )
			pos = wonderList.GetHeadPosition();
	}

	wonderList.GetAt( pos )->AddChild( item );
	pos = wonderList.GetHeadPosition();
	for ( i = wonderList.L(); i; i-- )
		s_wonderList->AddItem( (aui_Item *)wonderList.GetNext( pos ) );

	return 0;

}
sint32 victorywin_Init_Controls( MBCHAR *windowBlock )
{
	MBCHAR			controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	MBCHAR			tabBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	sint32 i = 0;

	sint32 staticNum = k_VICWIN_STATIC_MAX;
	s_staticControls.fill(nullptr);

	s_okButton = (ctp2_Button *)aui_Ldl::GetObject(windowBlock, "CloseButton");
	s_okButton->SetActionFuncAndCookie(VictoryWindowButtonActionCallback, nullptr);

	s_staticControls[k_VICWIN_MAIN_TITLE] = (ctp2_Static *)aui_Ldl::GetObject(windowBlock, "Title");

	snprintf(tabBlock, sizeof(tabBlock), "%s", "TabGroup.Tab2.TabPanel");

	for ( i = 0 ; i < k_VICWIN_STATIC_MAX - 1; i++ )
	{
		snprintf(controlBlock, sizeof(controlBlock), "%s.StaticText%d", tabBlock, i );
		s_staticControls[i] = (ctp2_Static *)aui_Ldl::GetObject(windowBlock, controlBlock);
	}

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", tabBlock, "ScoreList");
	s_scoreList = (ctp2_ListBox *)aui_Ldl::GetObject(windowBlock, controlBlock);

	snprintf(tabBlock, sizeof(tabBlock), "%s", "TabGroup.Tab3.TabPanel");
	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", tabBlock, "Graph");
	s_graph = (LineGraph *)aui_Ldl::GetObject(windowBlock, controlBlock);

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", tabBlock, "LineOrZeroSum");
	s_lineOrZeroSumButton = (ctp2_Button *)aui_Ldl::GetObject(windowBlock, controlBlock);
	s_lineOrZeroSumButton->SetActionFuncAndCookie(LineOrZeroSumButtonActionCallback, nullptr);

	victorywin_SetLineGraph( false );
	s_graph->EnableYNumber(FALSE);
	s_graph->EnablePrecision(FALSE);


	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", tabBlock, "GraphList");
	s_graphList = (ctp2_ListBox *)aui_Ldl::GetObject(windowBlock, controlBlock);

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "TabGroup");
	ctp2_TabGroup *tabGroup = (ctp2_TabGroup *)aui_Ldl::GetObject(controlBlock);
	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", controlBlock, "Tab2");
	tabGroup->SelectTab((ctp2_Tab *)aui_Ldl::GetObject(controlBlock));

	victorywin_AddWonders(windowBlock);

	return 0;
}





















































































































sint32 victorywin_UpdateData( sint32 type )
{
	MBCHAR strbuf[256];

	sint32 curPlayer = selitem_Get()->GetVisiblePlayer();
	Player *pl = player_Get(curPlayer);
	if(!pl) {
		PointerList<Player>::Walker walk(g_deadPlayer);
		while(walk.IsValid()) {
			if(walk.GetObj()->m_owner == selitem_Get()->GetVisiblePlayer()) {
				pl = walk.GetObj();
				break;
			}
			walk.Next();
		}
	}
	if(!pl)
		return 0;

	bool disableContinue = (type == k_VICWIN_DEFEAT);
	if(disableContinue
	&&(turn_Get()->IsHotSeat()
	|| turn_Get()->IsEmail())
	){
		for(sint32 i = 1; i < k_MAX_PLAYERS; i++)
		{
			if(i != curPlayer
			&& player_Get(i)
			&& player_Get(i)->IsHuman()
			){
				disableContinue = false;
			}
		}
	}
	else if(disableContinue
	     && player_Get(curPlayer)
	     &&!player_Get(curPlayer)->m_isDead
	     ){
		disableContinue = false;
	}


	if(type == k_VICWIN_DEFEAT) {

		s_staticControls[k_VICWIN_MAIN_TITLE]->SetText(s_stringTable->GetString(1));
	} else {
		s_staticControls[k_VICWIN_MAIN_TITLE]->SetText(s_stringTable->GetString(0));
	}

	if (disableContinue)
	{
		s_highScoreWin->m_continueButton->Enable(FALSE);

	}
	else
	{
		if((player_Get(selitem_Get()->GetVisiblePlayer())
		&& !player_Get(selitem_Get()->GetVisiblePlayer())->m_isDead)
		||  turn_Get()->IsEmail()
		||  turn_Get()->IsHotSeat()
		){
			s_highScoreWin->m_continueButton->Enable(TRUE);
		}
		else
		{
			s_highScoreWin->m_continueButton->Enable(FALSE);
		}
	}


	snprintf(strbuf, sizeof(strbuf),"%d",pl->GetTotalPopulation());
	s_staticControls[k_VICWIN_POP_BOX]->SetText(strbuf);

	snprintf(strbuf, sizeof(strbuf),"%d",pl->GetNumCities());
	s_staticControls[k_VICWIN_CITY_BOX]->SetText(strbuf);

	sint32 curScore = infowin_GetCivScore(curPlayer);
	snprintf(strbuf, sizeof(strbuf),"%s %d", s_stringTable->GetString(8),curScore);
	s_staticControls[k_VICWIN_SCORE_LABEL]->SetText(strbuf);







	strlcpy(strbuf, pl->GetDescriptionString(), sizeof(strbuf));


	if (s_highScoreWin) {
		if(GameSettings *gs = gamesettings_Get(); gs && gs->GetKeeppScore()) {
			s_highScoreWin->m_highScoreDB->AddHighScore(strbuf,curScore);
		}
	}

	victorywin_LoadWonderData();

	victorywin_LoadGraphData();

	victorywin_LoadScoreData();





	return 0;
}

sint32 victorywin_DisplayHighScore( )
{
	s_highScoreWin->DisplayWindow();

	return 0;
}












HighScoreListItem::HighScoreListItem(AUI_ERRCODE *retval, MBCHAR *name, sint32 score, MBCHAR *ldlBlock)
	:
	aui_ImageBase(ldlBlock),
	aui_TextBase(ldlBlock, (MBCHAR *)nullptr),
	ctp2_ListItem( retval, ldlBlock)
{
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommonLdl(name, score, ldlBlock);
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}

AUI_ERRCODE HighScoreListItem::InitCommonLdl(MBCHAR *name, sint32 score, MBCHAR *ldlBlock)
{
	MBCHAR			block[ k_AUI_LDL_MAXBLOCK + 1 ];
	AUI_ERRCODE		retval;

	m_score = score;

	strlcpy(m_name, name, sizeof(m_name));

	c3_Static		*subItem;

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "Rank");
	subItem = new c3_Static(&retval, aui_UniqueId(), block);
	AddChild(subItem);

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "Score");
	subItem = new c3_Static(&retval, aui_UniqueId(), block);
	AddChild(subItem);

	Update();

	return AUI_ERRCODE_OK;
}

void HighScoreListItem::Update()
{

	c3_Static *subItem;
	MBCHAR	strbuf[256];

	subItem = (c3_Static *)GetChildByIndex(0);
	subItem->SetText(m_name);

	subItem = (c3_Static *)GetChildByIndex(1);
	snprintf(strbuf, sizeof(strbuf),"%d",m_score);
	subItem->SetText(strbuf);
}

sint32 HighScoreListItem::Compare(ctp2_ListItem *item2, uint32 column)
{
	c3_Static		 *i1;
	c3_Static		 *i2;

	if (column < 0) return 0;

	switch (column) {
	case 0:
	case 1:

		i1 = (c3_Static *)this->GetChildByIndex(column);
		i2 = (c3_Static *)item2->GetChildByIndex(column);

		return strcmp(i1->GetText(), i2->GetText());
		break;
	}
	return 0;
}











HighScoreWindowPopup::HighScoreWindowPopup( sint32 type )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	strlcpy(windowBlock, "HighScoreWindowPopup", sizeof(windowBlock));

	{
		m_window.reset(new c3_PopupWindow( &errcode, aui_UniqueId(), windowBlock, 16, AUI_WINDOW_TYPE_FLOATING, false));
		Assert( AUI_NEWOK(m_window, errcode) );
		if ( !AUI_NEWOK(m_window, errcode) ) return;

		m_window->Resize(m_window->Width(),m_window->Height());
		m_window->GrabRegion()->Resize(m_window->Width(),m_window->Height());
		m_window->SetStronglyModal(TRUE);
	}

	m_highScoreDB.reset(new HighScoreDB());

	Initialize( windowBlock );

	if (player_Get(selitem_Get()->GetVisiblePlayer()) == nullptr) {
		m_continueButton->Enable(FALSE);
	} else {
		m_continueButton->Enable(TRUE);
	}

}

sint32 HighScoreWindowPopup::Initialize( MBCHAR *windowBlock )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "ContinueButton" );
	m_continueButton.reset(new ctp2_Button(&errcode, aui_UniqueId(), controlBlock, HighScoreWinButtonActionCallback, this));

	Assert( AUI_NEWOK(m_continueButton, errcode) );
	if ( !AUI_NEWOK(m_continueButton, errcode) ) return -1;







	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "QuitButton" );
	m_quitButton.reset(new ctp2_Button(&errcode, aui_UniqueId(), controlBlock, HighScoreWinButtonActionCallback, this));

	Assert( AUI_NEWOK(m_quitButton, errcode) );
	if ( !AUI_NEWOK(m_quitButton, errcode) ) return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "HighScoreList" );
	m_list.reset(new ctp2_ListBox(&errcode, aui_UniqueId(), controlBlock, nullptr, nullptr));
	Assert( AUI_NEWOK(m_list, errcode) );
	if ( !AUI_NEWOK(m_list, errcode) ) return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "Title");
	m_window->AddTitle(controlBlock);







	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );

	return 0;
}

HighScoreWindowPopup::~HighScoreWindowPopup( )
{
	Cleanup();
}

void HighScoreWindowPopup::Cleanup( )
{
    if (m_window && c3ui_Get())
    {
	    c3ui_Get()->RemoveWindow(m_window->Id());
    }

	// Same release order the mycleanup macro used.
	m_continueButton.reset();
	m_creditsButton.reset();
	m_quitButton.reset();
	m_list.reset();
	m_highScoreDB.reset();
	m_window.reset();
}

void HighScoreWindowPopup::DisplayWindow( )
{
	AUI_ERRCODE auiErr;

	UpdateData();

	g_modalWindow++;
	auiErr = c3ui_Get()->AddWindow(m_window.get());
	Assert( auiErr == AUI_ERRCODE_OK );
}

void HighScoreWindowPopup::RemoveWindow( )
{
	AUI_ERRCODE auiErr;

	auiErr = c3ui_Get()->RemoveWindow( m_window->Id() );
	Assert( auiErr == AUI_ERRCODE_OK );

	g_modalWindow--;
	c3ui_Get()->AddAction(new CloseVictoryWindowAction);
}

sint32 HighScoreWindowPopup::UpdateData( )
{

	MBCHAR ldlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	AUI_ERRCODE		retval;

	m_list->Clear();
	strlcpy(ldlBlock, "HighScoreListItem", sizeof(ldlBlock));
	HighScoreListItem *item = nullptr;
	HighScoreInfo *info = nullptr;

	for ( sint32 i = 0; i < m_highScoreDB->m_nHighScores ; i++ )
	{
		info = m_highScoreDB->GetHighScoreInfo(i);

		item = new HighScoreListItem(&retval, info->m_name, info->m_score, ldlBlock);
		m_list->AddItem((ctp2_ListItem *)item);
	}

	return 0;
}









sint32 victorywin_GetWonderFilename( sint32 index, MBCHAR *name, size_t size )
{
	MBCHAR filename[80];
	MBCHAR strbuf[_MAX_PATH];

	if ( index < 0 ) return FALSE;

	strlcpy(filename, g_theWonderDB->Get(index)->GetDefaultIcon()->GetIcon(), sizeof(filename));

	if (civpaths_Get()->FindFile(C3DIR_PICTURES, filename, strbuf))
	{
		strlcpy(name, filename, size);
		return TRUE;
	}

	return FALSE;
}

sint32 victorywin_LoadGraphData( )
{
	AUI_ERRCODE	retval;
	MBCHAR ldlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	MBCHAR strbuf[256];

	double		**graphData;
	sint32		 xCount;
	sint32		 yCount;

	if (!s_graph) return -1;

	graphData = nullptr;
	infowin_UpdateGraph(s_graph, xCount, yCount, &graphData);


	s_graphList->Clear();
	strlcpy(ldlBlock, "VictoryPlayerListItem", sizeof(ldlBlock));
	InfoPlayerListItem *pItem = nullptr;


	sint32 color = 0;

	LineGraphData *myData = s_graph->GetData();

	sint32 lineIndex = 0;

	Player *p = nullptr;
	Civilisation *civ = nullptr;

	for ( sint32 i = 0 ; i < k_MAX_PLAYERS ; i++ )
	{

		if (player_Get(i) && (i != PLAYER_INDEX_VANDALS))
		{

			if (!myData)
				color = (sint32)colorset_Get()->ComputePlayerColor(i);
			else color = myData[lineIndex++].color;

			p = player_Get(i);
			civ = p->GetCivilisation();
			if(civilisationpool_Get()->IsValid(civ->m_id)) {
				civ->GetSingularCivName(strbuf);
			} else {
				strbuf[0] = 0;
			}

			pItem = new InfoPlayerListItem(&retval, strbuf, color, ldlBlock);
			s_graphList->AddItem((ctp2_ListItem *)pItem);
		}
	}

	PointerList<Player>::Walker walk(g_deadPlayer);

	while(walk.IsValid()) {

		if (!myData)
			color = (sint32)colorset_Get()->ComputePlayerColor(walk.GetObj()->GetOwner());
		else color = myData[lineIndex++].color;

		p = walk.GetObj();
		civ = p->GetCivilisation();
		if(civilisationpool_Get()->IsValid(civ->m_id)) {
			civ->GetSingularCivName(strbuf);
		} else {
			strbuf[0] = 0;
		}

		pItem = new InfoPlayerListItem(&retval, strbuf, color, ldlBlock);
		s_graphList->AddItem((ctp2_ListItem *)pItem);
		walk.Next();
	}

	return 0;
}





void CloseVictoryWindowAction::Execute(aui_Control *control, uint32 action, uint32 data)
{
	victorywin_Cleanup();
}

sint32 victorywin_LoadScoreData( )
{
	AUI_ERRCODE	retval;
	MBCHAR ldlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	MBCHAR strbuf[256];

	sint32 curPlayer =  selitem_Get()->GetVisiblePlayer();

	s_scoreList->Clear();
	strlcpy(ldlBlock, "VictoryScoreListItem", sizeof(ldlBlock));
	InfoScoreListItem *item = nullptr;
	InfoScoreLabelListItem *label = nullptr;

	sint32 posValue = 0;
	sint32 negValue = 0;

	label = new InfoScoreLabelListItem(&retval, s_stringTable->GetString(2), nullptr, ldlBlock);
	s_scoreList->AddItem((ctp2_ListItem *)label);





	item = new InfoScoreListItem(&retval, curPlayer, SCORE_CAT_ADVANCES, ldlBlock);
	s_scoreList->AddItem((ctp2_ListItem *)item);
	posValue += item->GetValue();

	item = new InfoScoreListItem(&retval, curPlayer, SCORE_CAT_WONDERS, ldlBlock);
	s_scoreList->AddItem((ctp2_ListItem *)item);
	posValue += item->GetValue();

	item = new InfoScoreListItem(&retval, curPlayer, SCORE_CAT_POPULATION, ldlBlock);
	s_scoreList->AddItem((ctp2_ListItem *)item);
	posValue += item->GetValue();

	item = new InfoScoreListItem(&retval, curPlayer, SCORE_CAT_FEATS, ldlBlock);
	s_scoreList->AddItem((ctp2_ListItem *)item);
	posValue += item->GetValue();



























	item = new InfoScoreListItem(&retval, curPlayer, SCORE_CAT_TYPE_OF_VICTORY, ldlBlock);
	s_scoreList->AddItem((c3_ListItem *)item);
	posValue += item->GetValue();

	snprintf(strbuf, sizeof(strbuf),"%d",posValue);
	label = new InfoScoreLabelListItem(&retval, s_stringTable->GetString(3), strbuf, ldlBlock);
	s_scoreList->AddItem((ctp2_ListItem *)label);

	item = new InfoScoreListItem(&retval, -1, 0, ldlBlock);
	s_scoreList->AddItem((ctp2_ListItem *)item);

	label = new InfoScoreLabelListItem(&retval, s_stringTable->GetString(4), nullptr, ldlBlock);
	s_scoreList->AddItem((ctp2_ListItem *)label);


















	snprintf(strbuf, sizeof(strbuf),"%d",negValue);
	label = new InfoScoreLabelListItem(&retval, s_stringTable->GetString(5), strbuf, ldlBlock);
	s_scoreList->AddItem((ctp2_ListItem *)label);

	item = new InfoScoreListItem(&retval, -1, 0, ldlBlock);
	s_scoreList->AddItem((ctp2_ListItem *)item);

	Player *pl = player_Get(curPlayer);
	if(!pl) {
		PointerList<Player>::Walker walk(g_deadPlayer);
		while(walk.IsValid()) {
			if(walk.GetObj()->m_owner == selitem_Get()->GetVisiblePlayer()) {
				pl = walk.GetObj();
				break;
			}
			walk.Next();
		}
	}
	if(!pl)
		return 0;

	Score *score = pl->m_score.get();
	sint32 totalValue = score->GetTotalScore();
	snprintf(strbuf, sizeof(strbuf),"%d",totalValue);
	label = new InfoScoreLabelListItem(&retval, s_stringTable->GetString(6), strbuf, ldlBlock);
	s_scoreList->AddItem((ctp2_ListItem *)label);

	sint32 civScore = infowin_GetCivScore(curPlayer);
	snprintf(strbuf, sizeof(strbuf),"%d",civScore);
	label = new InfoScoreLabelListItem(&retval, s_stringTable->GetString(7), strbuf, ldlBlock);
	s_scoreList->AddItem((ctp2_ListItem *)label);

	return 0;
}

sint32 victorywin_LoadWonderData( )
{
	MBCHAR strbuf[256];

	Assert(g_theWonderDB->NumRecords() <= (k_VICWIN_WONDER_COL_MAX * k_VICWIN_WONDER_ROW_MAX));

	sint32 curPlayer = selitem_Get()->GetVisiblePlayer();

	for ( sint32 i = 0; i < g_theWonderDB->NumRecords() ; i++ )
	{

		if (curPlayer == wonderutil_GetOwner(i))
		{

			victorywin_GetWonderFilename(i,strbuf,sizeof(strbuf));

			s_wonderIcons[i]->SetImage(strbuf);
		}
	}

	return 0;
}



bool victorywin_IsOnScreen()
{
    return  (g_victoryWindow && c3ui_Get()->GetWindow(g_victoryWindow->m_window->Id())) ||
            (s_highScoreWin  && c3ui_Get()->GetWindow(s_highScoreWin->GetWindow()->Id()));
}
