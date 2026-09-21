//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Science window
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
// - Use the same science percentage everywhere.
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include <memory>

#include "ctp/c3.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_static.h"
#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_common/tech_wllist.h"
#include "ui/aui_common/aui_stringtable.h"

#include "ui/aui_ctp2/c3dropdown.h"
#include "ui/aui_common/aui_listbox.h"
#include "ui/aui_ctp2/c3_listbox.h"
#include "ui/aui_ctp2/c3dropdown.h"

#include "ui/aui_ctp2/controlsheet.h"
#include "ui/interface/statictextbox.h"
#include "ui/aui_ctp2/c3_hypertextbox.h"

#include "gfx/gfx_utils/pixelutils.h"
#include "ui/aui_ctp2/c3_switch.h"
#include "gfx/gfx_utils/colorset.h"               // colorset_Get()
#include "gfx/tilesys/tileset.h"
#include "ui/aui_ctp2/c3_icon.h"

#include "ui/aui_ctp2/c3slider.h"
#include "ui/aui_ctp2/thermometer.h"

#include "ui/aui_ctp2/textbutton.h"
#include "ui/aui_ctp2/textswitch.h"
#include "ui/aui_common/aui_switchgroup.h"
#include "ui/aui_ctp2/picturebutton.h"
#include "ui/aui_common/aui_button.h"
#include "ui/aui_ctp2/c3_button.h"
#include "ui/aui_ctp2/ctp2_button.h"

#include "ui/aui_common/aui_listbox.h"
#include "ui/aui_ctp2/c3_listbox.h"

#include "gs/database/StrDB.h"                  // stringdb_Get()
#include "BuildingRecord.h"
#include "WonderRecord.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/UnitData.h"
#include "gs/gameobj/citydata.h"
#include "gs/gameobj/Advances.h"
#include "gs/utility/Globals.h"
#include "AdvanceRecord.h"

#include "gs/gameobj/player.h"
#include "gs/gameobj/Readiness.h"
#include "ui/aui_ctp2/SelItem.h"
#include "gs/gameobj/ObjPool.h"

#include "ui/interface/debugwindow.h"

#include "gs/gameobj/UnitData.h"
#include "gs/gameobj/player.h"                 // player_Get()
#include "gs/gameobj/PlayHap.h"
#include "ui/aui_ctp2/SelItem.h"                // selitem_Get()
#include "gs/gameobj/Sci.h"

#include "ui/aui_ctp2/chart.h"

#include "gfx/gfx_utils/pixelutils.h"

#include "ui/interface/sci_advancescreen.h"

#include "ui/aui_ctp2/c3windows.h"
#include "ui/interface/GreatLibraryTypes.h"  // DATABASE definition for sciencewindow.h
#include "ui/interface/sciencewindow.h"

#include "ui/aui_ctp2/c3window.h"

#include "ui/interface/UIUtils.h"
#include "ui/interface/screenutils.h"

#include "ui/interface/sciencewin.h"
#include "ui/aui_ctp2/keypress.h"

#include "AdvanceBranchRecord.h"
#include "ctp/ctp2_utils/c3math.h"		            // AsPercentage

extern sint32			    g_modalWindow;
extern DebugWindow			*g_debugWindow;
extern aui_Surface			*g_sharedSurface;

std::unique_ptr<ScienceWin>	g_scienceWin;

static std::unique_ptr<c3_Button>	s_returnButton;

static sint32			s_scienceTax;

static std::unique_ptr<c3_Static>	s_lt;
static std::unique_ptr<c3_Static>	s_ct;
static std::unique_ptr<c3_Static>	s_rt;
static std::unique_ptr<c3_Static>	s_left;
static std::unique_ptr<c3_Static>	s_right;
static std::unique_ptr<c3_Static>	s_bottom;

static std::unique_ptr<c3_Static>	s_listtop;
static std::unique_ptr<c3_Static>	s_listbl;
static std::unique_ptr<c3_Static>	s_listbc;
static std::unique_ptr<c3_Static>	s_listbr;

static std::unique_ptr<c3_Static>	s_titleText;


static MBCHAR			s_ldlBlocks[k_EXTRA_PLAYERS][50] = {
	"PlayerOne",
	"PlayerTwo",
	"PlayerThree",
	"PlayerFour",
	"PlayerFive",
	"PlayerSix",
	"PlayerSeven",
	"PlayerEight"
};

static MBCHAR			s_flagBlocks[k_EXTRA_PLAYERS][50] = {
	"FlagOne",
	"FlagTwo",
	"FlagThree",
	"FlagFour",
	"FlagFive",
	"FlagSix",
	"FlagSeven",
	"FlagEight"
};

static std::unique_ptr<c3_HyperTextBox>	s_givesBox;

static std::unique_ptr<c3_Static>		s_civBox;
static std::unique_ptr<c3_Static>		s_civText;




void sciencewin_ExitCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	close_ScienceStatus();
}

void ScienceWin::kh_Close()
{
	close_ScienceStatus();
}

void sciencewin_LibraryButtonCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;


	Chart *tree = (Chart *)cookie;

	open_GreatLibrary( tree->GetCenterIndex(), TRUE );
	close_ScienceStatus();
}

void sciencewin_ChangeButtonCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	sci_advancescreen_displayMyWindow(nullptr, k_SCI_INCLUDE_CANCEL);
	close_ScienceStatus();
}

void sciencewin_SciButtonCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	Player *p = player_Get(selitem_Get()->GetVisiblePlayer());

	if ( (c3_Button *)control == g_scienceWin->PlusButton() ) {

		s_scienceTax += 10;

		if ( s_scienceTax > 100 )
			s_scienceTax = 100;
	}
	else if ( (c3_Button *)control == g_scienceWin->MinusButton() ) {

		s_scienceTax -= 10;

		if ( s_scienceTax < 0 )
			s_scienceTax = 0;
	}

	double taxRate = (double)s_scienceTax / 100;
	p->SetTaxes( taxRate );

	g_scienceWin->UpdateData( SCI_UPDATE_NOCHART );




}

void sciencewin_PrereqActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	Chart *chart = (Chart *)cookie;
	sint32 numPreReq = chart->GetNumPreReq();

	for ( sint32 i = 0;i < numPreReq;i++ ) {
		if ( control->Id() == chart->GetPreReqButton(i)->Id() ) {

			chart->Update( chart->GetPreReqIndex(i) );
		}
	}
}

void sciencewin_LeadsToActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	Chart *chart = (Chart *)cookie;
	sint32 numLeadsTo = chart->GetNumLeadsTo();

	for ( sint32 i = 0;i < numLeadsTo;i++ ) {
		if ( control->Id() == chart->GetLeadsToButton(i)->Id() ) {

			chart->Update( chart->GetLeadsToIndex(i) );
		}
	}

}

void sciencewin_AdvanceListCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_LISTBOX_ACTION_SELECT &&
		 action != (uint32)AUI_LISTBOX_ACTION_RMOUSESELECT) return;
	if ( !g_scienceWin->AdvanceList() ) return;

	AdvanceListItem *item;
	item = (AdvanceListItem *)g_scienceWin->AdvanceList()->GetSelectedItem();

	if ( action == (uint32) AUI_LISTBOX_ACTION_RMOUSESELECT) {
		HandleGameSpecificRightClick((void *)item);
		return;
	}

	if ( item ) {


		g_scienceWin->Tree()->Update( item->GetIndex() );
	}
}

sint32 knowledgewin_Initialize( )
{

	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;
	MBCHAR			windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	MBCHAR			buttonBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	MBCHAR			controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];











































































































































































	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", windowBlock, "GivesBox" );
	s_givesBox = std::make_unique<c3_HyperTextBox>( &errcode, aui_UniqueId(), buttonBlock );
	Assert( AUI_NEWOK(s_givesBox, errcode) );
	if ( !AUI_NEWOK(s_givesBox, errcode) ) return -4;

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", windowBlock, "CivBox" );
	s_civBox = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), buttonBlock );
	Assert( AUI_NEWOK(s_civBox, errcode) );
	if ( !AUI_NEWOK(s_civBox, errcode) ) return -5;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", buttonBlock, "CivText" );
	s_civText = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), controlBlock );
	Assert( AUI_NEWOK(s_civText, errcode) );
	if ( !AUI_NEWOK(s_civText, errcode) ) return -6;





	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "TitleText" );
	s_titleText = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), controlBlock );
	Assert( AUI_NEWOK(s_titleText, errcode ) );
	if ( !AUI_NEWOK(s_titleText, errcode) ) return -23;

	knowledgewin_InitGraphicTrim( windowBlock );

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );
	if ( !AUI_SUCCESS(errcode) ) return -1;




	g_modalWindow++;




	return 0;
}

sint32 knowledgewin_InitGraphicTrim( MBCHAR *windowBlock )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		imageBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	snprintf(imageBlock, sizeof(imageBlock), "%s.%s", windowBlock, "Lt" );
	s_lt = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), imageBlock);
	Assert( AUI_NEWOK(s_lt, errcode) );
	if ( !AUI_NEWOK(s_lt, errcode) ) return -10;
	snprintf(imageBlock, sizeof(imageBlock), "%s.%s", windowBlock, "Ct" );
	s_ct = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), imageBlock);
	Assert( AUI_NEWOK(s_ct, errcode) );
	if ( !AUI_NEWOK(s_ct, errcode) ) return -10;
	snprintf(imageBlock, sizeof(imageBlock), "%s.%s", windowBlock, "Rt" );
	s_rt = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), imageBlock);
	Assert( AUI_NEWOK(s_rt, errcode) );
	if ( !AUI_NEWOK(s_rt, errcode) ) return -10;
	snprintf(imageBlock, sizeof(imageBlock), "%s.%s", windowBlock, "LeftImage" );
	s_left = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), imageBlock);
	Assert( AUI_NEWOK(s_left, errcode) );
	if ( !AUI_NEWOK(s_left, errcode) ) return -10;
	snprintf(imageBlock, sizeof(imageBlock), "%s.%s", windowBlock, "RightImage" );
	s_right = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), imageBlock);
	Assert( AUI_NEWOK(s_right, errcode) );
	if ( !AUI_NEWOK(s_right, errcode) ) return -10;
	snprintf(imageBlock, sizeof(imageBlock), "%s.%s", windowBlock, "BottomImage" );
	s_bottom = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), imageBlock);
	Assert( AUI_NEWOK(s_bottom, errcode) );
	if ( !AUI_NEWOK(s_bottom, errcode) ) return -10;

	snprintf(imageBlock, sizeof(imageBlock), "%s.%s", windowBlock, "ListTop" );
	s_listtop = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), imageBlock);
	Assert( AUI_NEWOK(s_listtop, errcode) );
	if ( !AUI_NEWOK(s_listtop, errcode) ) return -10;
	snprintf(imageBlock, sizeof(imageBlock), "%s.%s", windowBlock, "ListBl" );
	s_listbl = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), imageBlock);
	Assert( AUI_NEWOK(s_listbl, errcode) );
	if ( !AUI_NEWOK(s_listbl, errcode) ) return -10;
	snprintf(imageBlock, sizeof(imageBlock), "%s.%s", windowBlock, "ListBc" );
	s_listbc = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), imageBlock);
	Assert( AUI_NEWOK(s_listbc, errcode) );
	if ( !AUI_NEWOK(s_listbc, errcode) ) return -10;
	snprintf(imageBlock, sizeof(imageBlock), "%s.%s", windowBlock, "ListBr" );
	s_listbr = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), imageBlock);
	Assert( AUI_NEWOK(s_listbr, errcode) );
	if ( !AUI_NEWOK(s_listbr, errcode) ) return -10;

















































	return 0;
}

sint32 knowledgewin_Cleanup( )
{




	s_returnButton.reset();










	s_givesBox.reset();
	s_civBox.reset();

	s_civText.reset();

	s_titleText.reset();

	s_lt.reset();
	s_ct.reset();
	s_rt.reset();
	s_left.reset();
	s_right.reset();
	s_bottom.reset();

	s_listtop.reset();
	s_listbl.reset();
	s_listbc.reset();
	s_listbr.reset();






	return 0;
}

KnowledgeListItem::KnowledgeListItem(AUI_ERRCODE *retval, sint32 index, MBCHAR *ldlBlock)
	:
	aui_ImageBase(ldlBlock),
	aui_TextBase(ldlBlock, (MBCHAR *)nullptr),
	c3_ListItem( retval, ldlBlock)
{
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommonLdl(index, ldlBlock);
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}

AUI_ERRCODE KnowledgeListItem::InitCommonLdl(sint32 index, MBCHAR *ldlBlock)
{
	MBCHAR			block[ k_AUI_LDL_MAXBLOCK + 1 ];
	AUI_ERRCODE		retval;

	m_index = index;

	c3_Static		*subItem;

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "Name");
	subItem = std::make_unique<c3_Static>(&retval, aui_UniqueId(), block).release();
	AddChild(subItem);

	Update();

	return AUI_ERRCODE_OK;
}

void KnowledgeListItem::Update()
{

	c3_Static *subItem;

	subItem = (c3_Static *)GetChildByIndex(0);
	subItem->SetText( g_theAdvanceDB->GetNameStr(m_index) );
}

sint32 KnowledgeListItem::Compare(c3_ListItem *item2, uint32 column)
{
	sint32	 val1;
	sint32	 val2;

	if (column < 0) return 0;

	switch (column) {
	case 0:
		val1 = m_index;
		val2 = ((KnowledgeListItem *)item2)->GetIndex();
		if (val1 < val2) return -1;
		else if (val1 > val2) return 1;
		else return 0;

		break;
	}

	return 0;
}

EmbassyListItem::EmbassyListItem(AUI_ERRCODE *retval, sint32 index, MBCHAR *ldlBlock)
	:
	aui_ImageBase(ldlBlock),
	aui_TextBase(ldlBlock, (MBCHAR *)nullptr),
	c3_ListItem( retval, ldlBlock)
{
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommonLdl(index, ldlBlock);
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}

AUI_ERRCODE EmbassyListItem::InitCommonLdl(sint32 index, MBCHAR *ldlBlock)
{
	MBCHAR			block[ k_AUI_LDL_MAXBLOCK + 1 ];
	AUI_ERRCODE		retval;

	m_index = index;

	c3_Static		*subItem;

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "Name");
	subItem = std::make_unique<c3_Static>(&retval, aui_UniqueId(), block).release();
	AddChild(subItem);

	Update();

	return AUI_ERRCODE_OK;
}

void EmbassyListItem::Update()
{
	MBCHAR			name[_MAX_PATH];

	c3_Static *subItem;

	player_Get(m_index)->GetPluralCivName( name );

	subItem = (c3_Static *)GetChildByIndex(0);
	subItem->SetText( name );
}

sint32 EmbassyListItem::Compare(c3_ListItem *item2, uint32 column)
{
	sint32	 val1;
	sint32	 val2;

	if (column < 0) return 0;

	switch (column) {
	case 0:
		val1 = m_index;
		val2 = ((EmbassyListItem *)item2)->GetIndex();
		if (val1 < val2) return -1;
		else if (val1 > val2) return 1;
		else return 0;

		break;
	}

	return 0;
}

AdvanceListItem::AdvanceListItem(AUI_ERRCODE *retval, sint32 index, MBCHAR *ldlBlock)
	:
	aui_ImageBase(ldlBlock),
	aui_TextBase(ldlBlock, (MBCHAR *)nullptr),
	c3_ListItem( retval, ldlBlock)
{
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommonLdl(index, ldlBlock);
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}

AdvanceListItem::~AdvanceListItem()
{
	ListPos position = m_childList->GetHeadPosition();

	for ( sint32 i = m_childList->L(); i; i-- ) {
		aui_Region		*subControl;

		subControl = m_childList->GetNext( position );
		if (subControl) {
			ListPos	subPos = subControl->ChildList()->GetHeadPosition();

			for (sint32 j = subControl->ChildList()->L(); j; j--) {
				std::unique_ptr<aui_Region>{subControl->ChildList()->GetNext(subPos)};
			}
		}
		std::unique_ptr<aui_Region>{subControl};
	}

	m_childList->DeleteAll();
}

AUI_ERRCODE AdvanceListItem::InitCommonLdl(sint32 index, MBCHAR *ldlBlock)
{
	MBCHAR			block[ k_AUI_LDL_MAXBLOCK + 1 ];
	MBCHAR			subBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	AUI_ERRCODE		retval;

	m_index = index;
	m_branchVal = g_theAdvanceBranchDB->Get( g_theAdvanceDB->Get(m_index)->GetBranchIndex() )->GetValue();

	c3_Static		*subItem;
	c3_Static		*branchItem;

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "PlayerFlag");
	subItem = std::make_unique<c3_Icon>(&retval, aui_UniqueId(), block).release();
	AddChild(subItem);

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "SymTwo");
	subItem = std::make_unique<c3_Static>(&retval, aui_UniqueId(), block).release();
	AddChild(subItem);

		snprintf(subBlock, sizeof(subBlock), "%s.%s", block, "Branch" );
		branchItem = std::make_unique<c3_Static>( &retval, aui_UniqueId(), subBlock ).release();
		subItem->AddChild( branchItem );

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "Name");
	subItem = std::make_unique<c3_Static>(&retval, aui_UniqueId(), block).release();
	AddChild(subItem);

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "FlagOne");
	subItem = std::make_unique<c3_Icon>(&retval, aui_UniqueId(), block).release();
	AddChild(subItem);

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "FlagTwo");
	subItem = std::make_unique<c3_Icon>(&retval, aui_UniqueId(), block).release();
	AddChild(subItem);

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "FlagThree");
	subItem = std::make_unique<c3_Icon>(&retval, aui_UniqueId(), block).release();
	AddChild(subItem);

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "FlagFour");
	subItem = std::make_unique<c3_Icon>(&retval, aui_UniqueId(), block).release();
	AddChild(subItem);

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "FlagFive");
	subItem = std::make_unique<c3_Icon>(&retval, aui_UniqueId(), block).release();
	AddChild(subItem);

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "FlagSix");
	subItem = std::make_unique<c3_Icon>(&retval, aui_UniqueId(), block).release();
	AddChild(subItem);

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "FlagSeven");
	subItem = std::make_unique<c3_Icon>(&retval, aui_UniqueId(), block).release();
	AddChild(subItem);





	aui_Ldl		*ldl = c3ui_Get()->GetLdl();
	if (ldl) {
		aui_Ldl::Remove((void *)this);

		MBCHAR name[k_MAX_NAME_LEN];

		snprintf(name, sizeof(name), "%s", stringdb_Get()->GetIdStr(g_theAdvanceDB->Get(index)->m_name));

		aui_Ldl::Associate((aui_Control *)this, name);
	}

	Update();

	return AUI_ERRCODE_OK;
}

void AdvanceListItem::Update()
{
	c3_Static *subItem;
	c3_Icon	*subIcon;
	c3_Static *branchItem;

	sint32 curPlayer = selitem_Get()->GetVisiblePlayer();

	subIcon = (c3_Icon*)GetChildByIndex(0);
	if ( player_Get(curPlayer) && player_Get(curPlayer)->HasAdvance(m_index) ) {
		subIcon->SetColor( colorset_Get()->ComputePlayerColor(curPlayer) );
		subIcon->SetMapIcon( MAPICON_FLAG );
	}
	else {
		subIcon->SetMapIcon( MAPICON_MAX );
	}

	subItem = (c3_Static *)GetChildByIndex(1);
	branchItem = (c3_Static *)subItem->GetChildByIndex(0);
	if ( g_scienceWin ) {
		branchItem->SetImage( g_scienceWin->GetString(g_theAdvanceDB->Get(m_index)->GetBranchIndex()) );
	}

	subItem = (c3_Static *)GetChildByIndex(2);
	subItem->SetText( g_theAdvanceDB->GetNameStr(m_index) );

	sint32 i;
	sint32 x = 0;

	for ( i = 0;i < k_MAX_PLAYERS;i++ ) {
		if ( i != curPlayer ) {

			if ( player_Get(i) && player_Get(curPlayer)->HasEmbassyWith(i) ) {
				subIcon = (c3_Icon *)GetChildByIndex(x+++3);

				if (subIcon) {

					if ( player_Get(i)->HasAdvance(m_index) ) {
						subIcon->SetColor( colorset_Get()->ComputePlayerColor(i) );
						subIcon->SetMapIcon( MAPICON_FLAG );
					}

					else {
						subIcon->SetMapIcon( MAPICON_MAX );
					}
				}
			}
		}
	}

	while ( x < k_PLAYERS ) {
		subIcon = (c3_Icon *)GetChildByIndex(x+++3);

		if (subIcon) {
			subIcon->SetMapIcon( MAPICON_MAX );
		}
	}
}

sint32 AdvanceListItem::Compare(c3_ListItem *item2, uint32 column)
{
	c3_Static		 *i1;
	c3_Static		 *i2;
	sint32	 val1;
	sint32	 val2;

	if (column < 0) return 0;

	switch (column) {
	case 0:
		val1 = m_index;
		val2 = ((AdvanceListItem *)item2)->GetIndex();
		if (val1 < val2) return -1;
		else if (val1 > val2) return 1;
		else return 0;

		break;
	case 1:
		val1 = m_branchVal;
		val2 = ((AdvanceListItem *)item2)->GetBranchVal();
		if (val1 < val2) return -1;
		else if (val1 > val2) return 1;
		else return 0;

		break;
	case 2:
		i1 = (c3_Static *)this->GetChildByIndex(column);
		i2 = (c3_Static *)item2->GetChildByIndex(column);

		return strcmp(i1->GetText(), i2->GetText());

		break;
	case 3:
		val1 = m_index;
		val2 = ((AdvanceListItem *)item2)->GetIndex();
		if (val1 < val2) return -1;
		else if (val1 > val2) return 1;
		else return 0;

		break;
	case 4:
		val1 = m_index;
		val2 = ((AdvanceListItem *)item2)->GetIndex();
		if (val1 < val2) return -1;
		else if (val1 > val2) return 1;
		else return 0;

		break;
	case 5:
		val1 = m_index;
		val2 = ((AdvanceListItem *)item2)->GetIndex();
		if (val1 < val2) return -1;
		else if (val1 > val2) return 1;
		else return 0;

		break;
	case 6:
		val1 = m_index;
		val2 = ((AdvanceListItem *)item2)->GetIndex();
		if (val1 < val2) return -1;
		else if (val1 > val2) return 1;
		else return 0;

		break;
	case 7:
		val1 = m_index;
		val2 = ((AdvanceListItem *)item2)->GetIndex();
		if (val1 < val2) return -1;
		else if (val1 > val2) return 1;
		else return 0;

		break;
	case 8:
		val1 = m_index;
		val2 = ((AdvanceListItem *)item2)->GetIndex();
		if (val1 < val2) return -1;
		else if (val1 > val2) return 1;
		else return 0;

		break;
	case 9:
		val1 = m_index;
		val2 = ((AdvanceListItem *)item2)->GetIndex();
		if (val1 < val2) return -1;
		else if (val1 > val2) return 1;
		else return 0;

		break;
	case 10:
		val1 = m_index;
		val2 = ((AdvanceListItem *)item2)->GetIndex();
		if (val1 < val2) return -1;
		else if (val1 > val2) return 1;
		else return 0;

		break;
	}

	return 0;
}

sint32 sciencewin_Initialize( )
{
	if ( g_scienceWin ) {
		g_scienceWin->UpdateData( SCI_UPDATE_ALL );
		g_scienceWin->m_window->MoveOG();
		return 0;
	}

	g_scienceWin = std::make_unique<ScienceWin>();
	g_scienceWin->UpdateData( SCI_UPDATE_ALL );

	return 0;
}

sint32 sciencewin_Cleanup( )
{
	g_scienceWin.reset();

	return 0;
}

ScienceWin::ScienceWin( )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	strlcpy(windowBlock,"ScienceWin", sizeof(windowBlock));

	m_window = std::make_unique<C3Window>( &errcode, aui_UniqueId(), windowBlock, 16, AUI_WINDOW_TYPE_FLOATING, false );
	Assert( AUI_NEWOK(m_window, errcode) );
	if ( !AUI_NEWOK(m_window, errcode) ) return;

	m_window->SetSurface( g_sharedSurface );

	m_window->Resize(m_window->Width(),m_window->Height());
	m_window->SetStronglyModal(TRUE);

	m_window->GrabRegion()->Resize( m_window->Width(), 20 );
	m_window->SetDraggable( TRUE );

	Initialize( windowBlock );
}

sint32 ScienceWin::Initialize( MBCHAR *windowBlock )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	MBCHAR		buttonBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "CloseButton" );
	m_closeButton = std::make_unique<c3_Button>( &errcode, aui_UniqueId(), controlBlock, sciencewin_ExitCallback );
	TestControl( m_closeButton );

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "Title" );
	m_title = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), controlBlock );
	TestControl( m_title );
	m_title->SetBlindness( TRUE );

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "AdvanceList" );
	m_advanceList = std::make_unique<c3_ListBox>( &errcode, aui_UniqueId(), controlBlock, sciencewin_AdvanceListCallback );
	TestControl( m_advanceList );

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", windowBlock, "ChangeButton" );
	m_changeButton = std::make_unique<c3_Button>( &errcode, aui_UniqueId(), buttonBlock, sciencewin_ChangeButtonCallback );
	TestControl( m_changeButton );

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "ResearchBox" );
	m_researchBox = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), controlBlock );
	TestControl( m_researchBox );

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "ResearchMeter" );
	m_researchMeter = std::make_unique<Thermometer>( &errcode, aui_UniqueId(), controlBlock );
	TestControl( m_researchMeter );

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "ResearchClock" );
	m_researchClock = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), controlBlock );
	TestControl( m_researchClock );

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "TurnsBox" );
	m_turnsBox = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), controlBlock );
	TestControl( m_turnsBox );

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "CostLabel" );
	m_costLabel = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), controlBlock );
	TestControl( m_costLabel );

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "CostBox" );
	m_costBox = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), controlBlock );
	TestControl( m_costBox );

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", windowBlock, "PlusButton" );
	m_plusButton = std::make_unique<c3_Button>( &errcode, aui_UniqueId(), buttonBlock, sciencewin_SciButtonCallback );
	TestControl( m_plusButton );

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", windowBlock, "MinusButton" );
	m_minusButton = std::make_unique<c3_Button>( &errcode, aui_UniqueId(), buttonBlock, sciencewin_SciButtonCallback );
	TestControl( m_minusButton );

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "PercentBox" );
	m_percentBox = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), controlBlock );
	TestControl( m_percentBox );

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "SciLabel" );
	m_sciLabel = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), controlBlock );
	TestControl( m_sciLabel );

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "SciBeaker" );
	m_sciBeaker = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), controlBlock );
	TestControl( m_sciBeaker );

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "SciBox" );
	m_sciBox = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), controlBlock );
	TestControl( m_sciBox );

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "Chart" );
	m_tree = std::make_unique<Chart>( &errcode, aui_UniqueId(), controlBlock );
	TestControl( m_tree );
	m_tree->Update(0);

	sint32 i;
	for ( i = 0;i < 4 ;i++ ) {
		m_tree->GetPreReqButton(i)->SetActionFuncAndCookie( sciencewin_PrereqActionCallback, m_tree.get() );
	}

	for ( i = 0;i < 4 ;i++ ) {
		m_tree->GetLeadsToButton(i)->SetActionFuncAndCookie( sciencewin_LeadsToActionCallback, m_tree.get() );
	}

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", windowBlock, "LibraryButton" );
	m_libraryButton = std::make_unique<c3_Button>( &errcode, aui_UniqueId(), buttonBlock, sciencewin_LibraryButtonCallback, m_tree.get() );
	TestControl( m_libraryButton );

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "KnownToLabel" );
	m_knownToLabel = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), controlBlock );
	TestControl( m_knownToLabel );

	for ( i = 0;i < k_EXTRA_PLAYERS;i++ ) {
		snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, s_ldlBlocks[i] );
		m_playerLabel[i] = std::make_unique<c3_Static>( &errcode, aui_UniqueId(), controlBlock );
		TestControl( m_playerLabel[i] );

		snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, s_flagBlocks[i] );
		m_playerFlag[i] = std::make_unique<c3_Icon>( &errcode, aui_UniqueId(), controlBlock );
		TestControl( m_playerFlag[i] );
		m_playerFlag[i]->SetMapIcon( MAPICON_MAX );
	}

	m_string = std::make_unique<aui_StringTable>( &errcode, "ScienceStrings" );
	TestControl( m_string );

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );

	return 0;
}

ScienceWin::~ScienceWin( )
{
	sint32 i;

	AUI_ERRCODE errcode;
	errcode = c3ui_Get()->RemoveWindow( m_window->Id() );
	Assert( errcode == AUI_ERRCODE_OK );

	m_closeButton.reset();

	m_title.reset();

	m_advanceList.reset();

	m_changeButton.reset();
	m_researchBox.reset();
	m_researchMeter.reset();
	m_researchClock.reset();
	m_turnsBox.reset();
	m_costLabel.reset();
	m_costBox.reset();
	m_plusButton.reset();
	m_minusButton.reset();
	m_percentBox.reset();
	m_sciLabel.reset();
	m_sciBeaker.reset();
	m_sciBox.reset();
	m_libraryButton.reset();
	m_tree.reset();
	m_knownToLabel.reset();

	for ( i= 0;i < k_EXTRA_PLAYERS;i++ ) {
		m_playerLabel[i].reset();
	}

	for ( i = 0;i < k_EXTRA_PLAYERS;i++ ) {
		m_playerFlag[i].reset();
	}

	m_string.reset();


	if (m_window)
		m_window->SetSurface(nullptr);

	m_window.reset();
}

void ScienceWin::Display( )
{
	AUI_ERRCODE errcode;

	errcode = c3ui_Get()->AddWindow( m_window.get() );
	Assert( errcode == AUI_ERRCODE_OK );

	keypress_RegisterHandler(this);

	g_modalWindow++;
}

void ScienceWin::Remove( )
{
	AUI_ERRCODE errcode;

	errcode = c3ui_Get()->RemoveWindow( m_window->Id() );
	Assert( errcode == AUI_ERRCODE_OK );

	keypress_RemoveHandler(this);

	g_modalWindow--;
}

sint32 ScienceWin::UpdateData( SCI_UPDATE update )
{
	MBCHAR str[_MAX_PATH];

	sint32 curPlayer = selitem_Get()->GetVisiblePlayer();
	Player *p = player_Get(selitem_Get()->GetVisiblePlayer());

	sint32 researching = player_Get(curPlayer)->m_advances->GetResearching();

	BOOL alreadyHas = p->HasAdvance(p->m_advances->GetResearching());

	if(alreadyHas)
	{
		str[0]=0;
	}
	else
	{
		snprintf(str, sizeof(str), "%s", g_theAdvanceDB->GetNameStr(researching) );
	}
	m_researchBox->SetText( str );

	sint32 sciLevel = p->GetCurrentScienceLevel();
	sint32 advanceCost = p->GetCurrentScienceCost();

	if(alreadyHas)
	{
		m_researchMeter->SetPercentFilled( 0 );
	}
	else
	{
		m_researchMeter->SetPercentFilled( (sciLevel * 100) / advanceCost );
	}

	if(alreadyHas)
	{
		str[0]=0;
	}
	else
	{
		snprintf(str, sizeof(str), "%d/%d", sciLevel, advanceCost );
	}
	m_costBox->SetText( str );

	sciLevel = p->m_advances->GetProjectedScience();
	if ( sciLevel < 0 ) {
		sciLevel = 0;
	}
	snprintf(str, sizeof(str), "%d", sciLevel );
	m_sciBox->SetText( str );

	double scienceTax;

	p->GetScienceTaxRate( scienceTax );

	s_scienceTax = AsPercentage(scienceTax);

	snprintf(str, sizeof(str),"%d%%",s_scienceTax);
	m_percentBox->SetText(str);

	sint32 advanceTurns = p->m_advances->TurnsToNextAdvance();

	if ( advanceTurns == -1 ) {
		snprintf(str, sizeof(str), "-" );
	}
	else if ( advanceTurns < 0 ) {
		snprintf(str, sizeof(str), "1" );
	}
	else {
		snprintf(str, sizeof(str), "%d", advanceTurns + 1 );
	}

	m_turnsBox->SetText( str );

	if ( update == SCI_UPDATE_NOCHART ) return 0;

	m_tree->Update(researching );

	if ( update == SCI_UPDATE_NOLIST ) return 0;

	UpdateList();

	player_Get(curPlayer)->GetPluralCivName( str );
	m_playerLabel[0]->SetText( str );
	m_playerLabel[0]->Show();
	m_playerFlag[0]->SetMapIcon( MAPICON_FLAG );
	m_playerFlag[0]->SetColor( colorset_Get()->ComputePlayerColor(curPlayer) );

	sint32 x = 1;
	for ( sint32 i = 0;i < k_MAX_PLAYERS;i++ ) {
		if ( i != curPlayer ) {
			if ( player_Get(i) && player_Get(curPlayer)->HasEmbassyWith(i) && x < k_EXTRA_PLAYERS) {
				player_Get(i)->GetPluralCivName( str );
				m_playerLabel[x]->SetText( str );
				m_playerLabel[x]->Show();
				m_playerFlag[x]->SetMapIcon( MAPICON_FLAG );
				m_playerFlag[x]->SetColor( colorset_Get()->ComputePlayerColor(i) );
				x++;
			}
		}
	}

	while ( x < k_PLAYERS ) {
		m_playerLabel[x]->Hide();
		m_playerLabel[x]->SetText( "" );
		m_playerFlag[x]->SetMapIcon( MAPICON_MAX );
		x++;
	}

	return 0;
}

void ScienceWin::UpdateList()
{
	AUI_ERRCODE errcode;
	MBCHAR		ldlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	sint32		curPlayer = selitem_Get()->GetVisiblePlayer();
	sint32		num = g_theAdvanceDB->NumRecords();

	m_advanceList->Clear();

	snprintf(ldlBlock, sizeof(ldlBlock), "AdvanceListItem" );

	for ( sint32 i = 0;i < num;i++ ) {

		if ( player_Get(curPlayer)->HasAdvance(i) ) {
			m_advanceList->AddItem( (c3_ListItem *)std::make_unique<AdvanceListItem>( &errcode, i, ldlBlock ).release() );
		}
		else {
			for ( sint32 j = 0;j < k_MAX_PLAYERS;j++ ) {
				if ( j != curPlayer ) {
					if ( player_Get(j) && player_Get(curPlayer)->HasEmbassyWith(j) && player_Get(j)->HasAdvance(i) ) {
						m_advanceList->AddItem( (c3_ListItem *)std::make_unique<AdvanceListItem>( &errcode, i, ldlBlock ).release() );
						break;
					}
				}
			}
		}
	}
}

void SW_UpdateAction::Execute(aui_Control *control, uint32 action, uint32 data)
{
	if ( g_scienceWin ) {
		if ( c3ui_Get()->GetWindow(g_scienceWin->m_window->Id()) ) {
			if ( m_all ) {
				g_scienceWin->UpdateData( SCI_UPDATE_ALL );
			}
			else {
				g_scienceWin->UpdateData( SCI_UPDATE_NOCHART );
			}
		}
	}
}
