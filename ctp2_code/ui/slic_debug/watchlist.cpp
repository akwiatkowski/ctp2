//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Slic watch list
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
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"


#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_static.h"
#include "ui/aui_common/aui_hypertextbox.h"

#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/thermometer.h"

#include "ui/aui_ctp2/textbutton.h"
#include "ui/aui_ctp2/c3_button.h"

#include "ui/aui_ctp2/c3textfield.h"

#include "ui/aui_ctp2/c3listbox.h"
#include "ui/aui_ctp2/c3_listbox.h"
#include "ui/aui_common/aui_listbox.h"

#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_ctp2/c3windows.h"
#include "ui/aui_ctp2/c3_popupwindow.h"
#include "ui/aui_ctp2/c3_utilitydialogbox.h"

#include "ui/aui_ctp2/keypress.h"

#include "ui/slic_debug/watchlist.h"

#include "gs/slic/SlicEngine.h"
#include "gfx/gfx_utils/pixelutils.h"
#include "gfx/gfx_utils/colorset.h"

#include "gs/slic/sliccmd.h"
#include "sliccmd.tab.h"

#include "ctp/ctp2_utils/pointerlist.h"


static WatchList *g_watchList = nullptr;

void watchlist_Callback(sint32 arg)
{
}

void watchlist_Display()
{
	if(!g_watchList) {
		g_watchList = new WatchList(watchlist_Callback);
	}
	g_watchList->DisplayWindow();
}

void watchlist_Remove()
{
	if(g_watchList) {
		g_watchList->RemoveWindow();
	}
}

void watchlist_Refresh()
{
	watchlist_Display();
}

void watchlist_AddExpression(char *exp)
{
	watchlist_Display();
	if(!g_watchList)
		return;

	g_watchList->AddExpression(exp);
}

WatchList::WatchList(WatchListCallback callback, MBCHAR *ldlBlock)
:   m_window                (nullptr),
    m_list                  (nullptr),
    m_newButton             (nullptr),
    m_clearButton           (nullptr),
    m_exitButton            (nullptr),
	m_callback              (callback)
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
    strlcpy(windowBlock, ldlBlock ? ldlBlock : "WatchListPopup", sizeof(windowBlock));

		m_window = new c3_PopupWindow( &errcode, aui_UniqueId(), windowBlock, 16, AUI_WINDOW_TYPE_FLOATING, false);
		Assert( AUI_NEWOK(m_window, errcode) );
	if (AUI_NEWOK(m_window, errcode))
    {
		m_window->Resize(m_window->Width(),m_window->Height());
		m_window->GrabRegion()->Resize(m_window->Width(),m_window->Height());
		m_window->SetStronglyModal(FALSE);
		m_window->SetDraggable(TRUE);

    	Initialize(windowBlock);
	}
}

WatchList::~WatchList()
{
    if (c3ui_Get() && m_window)
    {
	    c3ui_Get()->RemoveWindow(m_window->Id());
    }

    if (m_list)
    {
	    m_list->Clear();
    }

	delete m_list;
	delete m_newButton;
    delete m_clearButton;
    delete m_exitButton;
	delete m_window;
}

void WatchListActionCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if((action != (uint32)AUI_LISTBOX_ACTION_SELECT) &&
	   (action != (uint32)AUI_LISTBOX_ACTION_RMOUSESELECT) &&
	   (action != (uint32)AUI_LISTBOX_ACTION_DOUBLECLICKSELECT))
		return;

	WatchList *list = (WatchList *)cookie;

	WatchListItem *item = (WatchListItem *)list->GetList()->GetSelectedItem();

	if(action == AUI_LISTBOX_ACTION_DOUBLECLICKSELECT) {

	}

	if(!item)
		return;

	if(action == AUI_LISTBOX_ACTION_RMOUSESELECT) {
		return;
	}

	}

void WatchListButtonCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if(action != AUI_BUTTON_ACTION_EXECUTE) {
		return;
	}

	if(!g_watchList)
		return;

	if(control == g_watchList->GetNewButton()) {
		g_watchList->AddExpression("");
	}

	if(control == g_watchList->GetClearButton()) {
		g_watchList->Clear();
	}

	if(control == g_watchList->GetExitButton()) {
		watchlist_Remove();
	}
}

sint32 WatchList::Initialize(MBCHAR *windowBlock)
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];




	snprintf( controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "WatchList" );
	m_list = new c3_ListBox(&errcode, aui_UniqueId(), controlBlock, WatchListActionCallback, this);
	m_list->SetAbsorbancy(FALSE);
	m_list->Clear();

	Assert( AUI_NEWOK(m_list, errcode) );
	if ( !AUI_NEWOK(m_list, errcode) )
		return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "NewButton");
	m_newButton = new c3_Button(&errcode, aui_UniqueId(), controlBlock, WatchListButtonCallback, this);

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "ClearButton");
	m_clearButton = new c3_Button(&errcode, aui_UniqueId(), controlBlock, WatchListButtonCallback, this);

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "ExitButton");
	m_exitButton = new c3_Button(&errcode, aui_UniqueId(), controlBlock, WatchListButtonCallback, this);

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );

	return 0;
}

void WatchList::DisplayWindow()
{
	AUI_ERRCODE auiErr;

	UpdateData();

	auiErr = c3ui_Get()->AddWindow(m_window);
	Assert(auiErr == AUI_ERRCODE_OK);

	keypress_RegisterHandler(this);
}

void WatchList::RemoveWindow()
{
	AUI_ERRCODE auiErr;

	auiErr = c3ui_Get()->RemoveWindow(m_window->Id());
	Assert(auiErr == AUI_ERRCODE_OK);

	keypress_RemoveHandler(this);
}

void WatchList::kh_Close()
{
	RemoveWindow();
}

sint32 WatchList::UpdateData()
{
	MBCHAR ldlBlock[k_AUI_LDL_MAXBLOCK + 1];

	strlcpy(ldlBlock, "WatchListItem", sizeof(ldlBlock));

	return 0;
}

void WatchList::AddExpression(char *exp)
{
	AUI_ERRCODE retval = AUI_ERRCODE_OK;
	WatchListItem *item = new WatchListItem(&retval, 0, exp, "WatchListItem");
	m_list->AddItem(item);
}

void WatchList::Clear()
{
	m_list->Clear();
}




WatchListItem::WatchListItem(AUI_ERRCODE *retval, sint32 index,
							 MBCHAR *line, MBCHAR *ldlBlock) :
	aui_ImageBase(ldlBlock),
	aui_TextBase(ldlBlock, (MBCHAR *)nullptr),
	c3_ListItem(retval, ldlBlock)
{
	m_index = index;
	m_break = false;

	strlcpy(m_line, line, sizeof(m_line));
	m_watching = new PointerList<SlicSymbolData>;

	Assert(AUI_SUCCESS(*retval));
	if(!AUI_SUCCESS(*retval)) return;

	*retval = InitCommonLdl(ldlBlock);
	Assert(AUI_SUCCESS(*retval));
	if(!AUI_SUCCESS(*retval)) return;
}

WatchListItem::~WatchListItem()
{
	if(m_watching) {
		PointerList<SlicSymbolData>::Walker walk(m_watching);
		while(walk.IsValid()) {
			walk.GetObj()->RemoveWatch(this);
			walk.Next();
		}

		delete m_watching;
		m_watching = nullptr;
	}
}

void WatchExpressionItemCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if(action == (uint32)AUI_TEXTFIELD_ACTION_EXECUTE) {
		WatchListItem *item = (WatchListItem *)cookie;
		item->Update();
	}
}

void WatchBreakItemCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if(action == (uint32)AUI_TEXTFIELD_ACTION_EXECUTE) {
		WatchListItem *item = (WatchListItem *)cookie;
		item->ToggleBreak();
		item->Update();
	}
}

AUI_ERRCODE WatchListItem::InitCommonLdl(MBCHAR *ldlBlock)
{
	MBCHAR			block[ k_AUI_LDL_MAXBLOCK + 1 ];
	AUI_ERRCODE		retval;

	C3TextField *expressionItem;
	c3_Static *valueItem;
	c3_Static *breakItem;

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "Break");
	breakItem = new c3_Static(&retval, aui_UniqueId(), block);
	breakItem->SetActionFuncAndCookie(WatchBreakItemCallback, this);
	AddChild(breakItem);

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "Expression");
	expressionItem = new C3TextField(&retval, aui_UniqueId(), block, WatchExpressionItemCallback, this);
	expressionItem->SetFieldText(m_line);
	AddChild(expressionItem);

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "Value");
	valueItem = new c3_Static(&retval, aui_UniqueId(), block);
	AddChild(valueItem);

	Update();
	return AUI_ERRCODE_OK;
}

void WatchListItem::Update()
{
	C3TextField *expressionItem;
	expressionItem = (C3TextField *)GetChildByIndex(1);
	expressionItem->GetFieldText(m_line, k_MAX_WATCH_LINE);

	sint32 res;
	char valbuf[1024];
	char catString = 0;
	res = sliccmd_parse(SLICCMD_WATCH, m_line, valbuf, 1024, 0, &catString);

	while(m_watching->GetHead()) {
		m_watching->RemoveHead();
	}

	sliccmd_add_watch(this);

	if(res != 0) {
		strlcpy(valbuf, "--", sizeof(valbuf));
	}

	c3_Static *valueItem = (c3_Static *)GetChildByIndex(2);
	valueItem->SetText(valbuf);

	c3_Static *breakItem = (c3_Static *)GetChildByIndex(0);
	if(m_break) {
		breakItem->SetText("X");
	} else {
		breakItem->SetText("-");
	}
}

sint32 WatchListItem::Compare(c3_ListItem *item2, uint32 column)
{
	return 0;
}

void WatchListItem::WatchCallback(SlicSymbolData *symbol, bool isAddCallback)
{
	if(!isAddCallback) {
		Update();
		if(m_break) {
			slicengine_Get()->RequestBreak();
		}
	} else {
		m_watching->AddTail(symbol);
	}
}

void WatchListItem::WatchVariableDeleted(SlicSymbolData *symbol)
{
	PointerList<SlicSymbolData>::Walker walk(m_watching);
	while(walk.IsValid()) {
		if(walk.GetObj() == symbol) {
			walk.Remove();
		} else {
			walk.Next();
		}
	}
}
