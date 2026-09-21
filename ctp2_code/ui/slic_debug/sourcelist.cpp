//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Slic source list
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

#ifdef CTP2_ENABLE_SLICDEBUG

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

#include "ui/slic_debug/sourcelist.h"

#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicSegment.h"
#include "gs/slic/SlicConditional.h"

#include "gfx/gfx_utils/pixelutils.h"
#include "gfx/gfx_utils/colorset.h"               // colorset_Get()
#include <memory>


static SourceList *g_sourceList = nullptr;

class SourceListItemContinueAction : public aui_Action
{
public:
	SourceListItemContinueAction(SourceListItem *item)
    :   aui_Action  (),
        m_item      (item)
    { ; };
    ~SourceListItemContinueAction() override { ; };

	void Execute
    (
        aui_Control *   control,
	    uint32          action,
	    uint32          data
    ) override
	{
        if (m_item)
        {
		    m_item->Continue();
        }
	}

private:
	SourceListItem *    m_item;
};

void sourcelist_Callback(sint32 arg)
{
}

void sourcelist_Display(SlicSegment *segment)
{
	if(!g_sourceList) {
		g_sourceList = std::make_unique<SourceList>(sourcelist_Callback).release();
	}
	g_sourceList->DisplayWindow(segment);
}

void sourcelist_Remove()
{
	if(g_sourceList) {
		g_sourceList->RemoveWindow();
	}
}

void sourcelist_RegisterBreak(SlicSegment *segment, sint32 offset)
{
	sourcelist_Display(segment);
	if(!g_sourceList)
		return;

	g_sourceList->ShowBreak(offset);
}

SourceList::SourceList(SourceListCallback *callback, MBCHAR *ldlBlock)
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	m_continue = nullptr;
	m_list = nullptr;
	m_exit = nullptr;
	m_step = nullptr;
	m_stepInto = nullptr;
	m_status = nullptr;

	if (ldlBlock) strlcpy(windowBlock,ldlBlock, sizeof(windowBlock));
	else strlcpy(windowBlock,"SourceListPopup", sizeof(windowBlock));

	{
		m_window = std::make_unique<c3_PopupWindow>( &errcode, aui_UniqueId(), windowBlock, 16, AUI_WINDOW_TYPE_FLOATING, false);
		Assert( AUI_NEWOK(m_window, errcode) );
		if ( !AUI_NEWOK(m_window, errcode) ) return;

		m_window->Resize(m_window->Width(),m_window->Height());
		m_window->GrabRegion()->Resize(m_window->Width(),m_window->Height());
		m_window->SetStronglyModal(FALSE);
		m_window->SetDraggable(TRUE);
	}

	m_callback = callback;

	Initialize( windowBlock );
}

SourceList::~SourceList()
{
    if (c3ui_Get() && m_window)
    {
	    c3ui_Get()->RemoveWindow(m_window->Id());
    }
    // Controls are released in reverse declaration order, so the window --
    // declared first -- outlives every control it hosts.

	// m_callback : reference only
	// m_segment  : reference only
}

void SourceListActionCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if((action != (uint32)AUI_LISTBOX_ACTION_SELECT) &&
	   (action != (uint32)AUI_LISTBOX_ACTION_RMOUSESELECT) &&
	   (action != (uint32)AUI_LISTBOX_ACTION_DOUBLECLICKSELECT))
		return;

	SourceList *list = (SourceList *)cookie;

	SourceListItem *item = (SourceListItem *)list->GetList()->GetSelectedItem();

	if(action == AUI_LISTBOX_ACTION_DOUBLECLICKSELECT) {

	}

	if(!item)
		return;

	if(action == AUI_LISTBOX_ACTION_RMOUSESELECT) {
		return;
	}

	}

void SourceListButtonCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if(action == AUI_BUTTON_ACTION_EXECUTE) {
		if(!g_sourceList)
			return;

		if(control == g_sourceList->m_continue.get()) {
			g_sourceList->Continue();
		}

		if(control == g_sourceList->m_exit.get()) {
			sourcelist_Remove();
		}

		if(control == g_sourceList->m_step.get()) {

			g_sourceList->StepInto();
		}

		if(control == g_sourceList->m_stepInto.get()) {
			g_sourceList->StepInto();
		}
	}
}

sint32 SourceList::Initialize(MBCHAR *windowBlock)
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];




	snprintf( controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "SourceList" );
	m_list = std::make_unique<c3_ListBox>(&errcode, aui_UniqueId(), controlBlock, SourceListActionCallback, this);
	m_list->SetAbsorbancy(FALSE);
	Assert( AUI_NEWOK(m_list, errcode) );
	if ( !AUI_NEWOK(m_list, errcode) )
		return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "ContinueButton");
	m_continue = std::make_unique<c3_Button>(&errcode, aui_UniqueId(), controlBlock, SourceListButtonCallback, this);
	Assert(AUI_NEWOK(m_continue, errcode));
	if( !AUI_NEWOK(m_continue, errcode))
		return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "ExitButton");
	m_exit = std::make_unique<c3_Button>(&errcode, aui_UniqueId(), controlBlock, SourceListButtonCallback, this);
	Assert(AUI_NEWOK(m_exit, errcode));
	if( !AUI_NEWOK(m_exit, errcode))
		return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "StepButton");
	m_step = std::make_unique<c3_Button>(&errcode, aui_UniqueId(), controlBlock, SourceListButtonCallback, this);
	Assert(AUI_NEWOK(m_step, errcode));
	if( !AUI_NEWOK(m_step, errcode))
		return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "StepIntoButton");
	m_stepInto = std::make_unique<c3_Button>(&errcode, aui_UniqueId(), controlBlock, SourceListButtonCallback, this);
	Assert(AUI_NEWOK(m_stepInto, errcode));
	if( !AUI_NEWOK(m_stepInto, errcode))
		return -1;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "Status");
	m_status = std::make_unique<c3_Static>(&errcode, aui_UniqueId(), controlBlock);
	Assert(AUI_NEWOK(m_status, errcode));
	if(!AUI_NEWOK(m_status, errcode))
		return -1;

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );

	return 0;
}

void SourceList::Cleanup()
{
    if (c3ui_Get() && m_window)
    {
        c3ui_Get()->RemoveWindow(m_window->Id());
    }

    // Released here so the list can be rebuilt; the window goes last, after
    // the controls it hosts. (This replaced a local `delete p; p = NULL;`
    // macro that hid the release behind a name.)
    m_continue.reset();
    m_list.reset();
    m_window.reset();

    m_callback = nullptr;
}

void SourceList::DisplayWindow(SlicSegment *segment)
{
	AUI_ERRCODE auiErr;

	m_segment = segment;
	UpdateData();

	auiErr = c3ui_Get()->AddWindow(m_window.get());
	Assert(auiErr == AUI_ERRCODE_OK);

	keypress_RegisterHandler(this);
}

void SourceList::RemoveWindow()
{
	AUI_ERRCODE auiErr;

	auiErr = c3ui_Get()->RemoveWindow(m_window->Id());
	Assert(auiErr == AUI_ERRCODE_OK);

	keypress_RemoveHandler(this);
}

void SourceList::kh_Close()
{
	RemoveWindow();
}

sint32 SourceList::UpdateData()
{
	MBCHAR ldlBlock[k_AUI_LDL_MAXBLOCK + 1];

	AUI_ERRCODE retval = AUI_ERRCODE_OK;

	strlcpy(ldlBlock, "SourceListItem", sizeof(ldlBlock));

	m_list->Clear();

	sint32 i;
	sint32 firstLineNum;
	sint32 firstLineOffset;
	sint32 lastLineNum;

	bool res = m_segment->GetSourceLines(firstLineNum, firstLineOffset, lastLineNum);
	Assert(res);
	if(!res)
		return 1;

	FILE *f = fopen(m_segment->GetFilename(), "r");
	Assert(f);
	if(!f)
		return 1;

	fseek(f, firstLineOffset, SEEK_SET);
	MBCHAR line[k_MAX_SOURCE_LINE];

	SourceListItem *item;

	for(i = firstLineNum; i <= lastLineNum; i++) {
		fgets(line, 1024, f);
		item = std::make_unique<SourceListItem>(&retval, i - firstLineNum,
								   m_segment, line, i, ldlBlock).release();
		m_list->AddItem((c3_ListItem *)item);

	}

	fclose(f);

	return 0;
}

void SourceList::ShowBreak(sint32 offset)
{
	sint32 lineNumber;
	lineNumber = m_segment->FindLineNumber(offset);
	sint32 i;

	char statusBuf[1024];
	snprintf(statusBuf, sizeof(statusBuf), "Break at %s:%d", m_segment->GetName(), lineNumber);

	for(i = 0; i < m_list->NumItems(); i++) {
		SourceListItem *item = (SourceListItem *)m_list->GetItemByIndex(i);
		if(item->m_lineNumber == lineNumber) {
			item->ShowBreak();
			SlicConditional *cond = item->GetSegment()->GetConditional(lineNumber);
			if(cond) {
				strncat(statusBuf, " when ", sizeof(statusBuf) - strlen(statusBuf) - 1);
				strncat(statusBuf, cond->GetExpression(), sizeof(statusBuf) - strlen(statusBuf) - 1);
			}
			break;
		}
	}

	m_status->SetText(statusBuf);
}

void SourceList::Continue()
{
	sint32 i;

	m_status->SetText("--");

	for(i = 0; i < m_list->NumItems(); i++) {
		SourceListItem *item = (SourceListItem *)m_list->GetItemByIndex(i);
		if(item->m_activeBreak) {
			c3ui_Get()->AddAction(std::make_unique<SourceListItemContinueAction>(item).release());
			return;
		}
	}

	if(slicengine_Get()->AtBreak()) {
		slicengine_Get()->Continue();
	}
}

void SourceList::StepInto()
{

	slicengine_Get()->RequestBreak();
	Continue();
}




SourceListItem::SourceListItem(AUI_ERRCODE *retval, sint32 index,
							   SlicSegment *segment, MBCHAR *line,
							   sint32 lineNumber, MBCHAR *ldlBlock) :
	aui_ImageBase(ldlBlock),
	aui_TextBase(ldlBlock, (MBCHAR *)nullptr),
	c3_ListItem(retval, ldlBlock)
{
	m_index = index;
	m_segment = segment;
	strlcpy(m_line, line, sizeof(m_line));
	m_lineNumber = lineNumber;

	m_break = m_segment->LineHasBreak(m_lineNumber, m_conditional);
	m_activeBreak = false;

	Assert(AUI_SUCCESS(*retval));
	if(!AUI_SUCCESS(*retval)) return;

	*retval = InitCommonLdl(segment, ldlBlock);
	Assert(AUI_SUCCESS(*retval));
	if(!AUI_SUCCESS(*retval)) return;
}

void SourceBreakItemCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if(action == k_C3_STATIC_ACTION_LMOUSE) {
		SourceListItem *item = (SourceListItem *)cookie;
		item->ToggleBreak();
	} else if(action == k_C3_STATIC_ACTION_RMOUSE) {
		SourceListItem *item = (SourceListItem *)cookie;
		item->EditConditional();
	}
}

AUI_ERRCODE SourceListItem::InitCommonLdl(SlicSegment *segment,
										   MBCHAR *ldlBlock)
{
	MBCHAR			block[ k_AUI_LDL_MAXBLOCK + 1 ];
	AUI_ERRCODE		retval;

	c3_Static *breakItem;
	c3_Static *textItem;

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "Break");
	breakItem = std::make_unique<c3_Static>(&retval, aui_UniqueId(), block).release();
	breakItem->SetActionFuncAndCookie(SourceBreakItemCallback, this);
	AddChild(breakItem);

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "Line");
	textItem = std::make_unique<c3_Static>(&retval, aui_UniqueId(), block).release();
	AddChild(textItem);

	Update();
	return AUI_ERRCODE_OK;
}

void SourceListItem::Update()
{
	c3_Static *codeItem;
	codeItem = (c3_Static *)GetChildByIndex(1);
	codeItem->SetText(m_line);

	m_break = m_segment->LineHasBreak(m_lineNumber, m_conditional);

	c3_Static *breakItem = (c3_Static *)GetChildByIndex(0);
	char breakChars[3];
	if(!m_break) {
		breakChars[0] = '-';
	} else {
		if(m_conditional) {
			breakChars[0] = 'C';
		} else {
			breakChars[0] = '+';
		}
	}

	if(m_activeBreak) {
		breakChars[1] = '>';
		breakChars[2] = 0;
		breakItem->SetText(breakChars);
		codeItem->SetTextColor(colorset_Get()->GetColorRef(COLOR_RED));
	} else {
		breakChars[1] = 0;
		breakItem->SetText(breakChars);
		codeItem->SetTextColor(colorset_Get()->GetColorRef(COLOR_BLACK));
	}
}

sint32 SourceListItem::Compare(c3_ListItem *item2, uint32 column)
{
	return 0;
}

void SourceListItem::Open()
{
	}

void SourceListItem::Continue()
{
	m_activeBreak = false;
	Update();
		slicengine_Get()->Continue();
}

void SourceListItem::ToggleBreak()
{
	m_break = !m_break;

	if(m_break) {
		m_segment->SetBreak(m_lineNumber, true);
	} else {
		m_segment->SetBreak(m_lineNumber, false);
	}

	Update();
}

void SourceListItem::SetBreak()
{
	m_break = true;
	m_segment->SetBreak(m_lineNumber, true);
	Update();
}

void SourceListItem::ClearBreak()
{
	m_break = false;
	m_segment->SetBreak(m_lineNumber, false);
	Update();
}

void SourceListItem::ShowBreak()
{
	m_activeBreak = TRUE;
	Update();
}

static c3_UtilityTextFieldPopup *s_conditionalPopup = nullptr;

class KillConditionalPopupAction : public aui_Action
{
public:
	void Execute(aui_Control* control,
	                     uint32 action,
	                     uint32 data) override
	{
		std::unique_ptr<c3_UtilityTextFieldPopup>{s_conditionalPopup};
		s_conditionalPopup = nullptr;
	};
};

void SourceListItemConditionalCallback(MBCHAR const *text, sint32 val2, void *data)
{
	if(!val2)
		return;

	SourceListItem *item = (SourceListItem *)data;
	if(!text || strlen(text) < 1) {
		item->GetSegment()->RemoveConditional(item->GetLineNumber());
		item->ClearBreak();
		return;
	}

	SlicConditional *cond = item->GetSegment()->GetConditional(item->GetLineNumber());
	if (cond)
    {
		cond->SetExpression(text);
    }
    else
    {
		cond = item->GetSegment()->NewConditional(item->GetLineNumber(), text);
		Assert(cond);
	}
	item->SetBreak();

	c3ui_Get()->AddAction(std::make_unique<KillConditionalPopupAction>().release());
}

void SourceListItem::EditConditional()
{
	SlicConditional *cond = m_segment->GetConditional(m_lineNumber);
	if(!s_conditionalPopup)
	s_conditionalPopup = std::make_unique<c3_UtilityTextFieldPopup>(SourceListItemConditionalCallback,
													  nullptr,
													  cond ? cond->GetExpression() : "",
													  nullptr,
													  "SourceListConditionalPopup",
													  this,
													  true).release();
	s_conditionalPopup->DisplayWindow();

}

#endif // CTP2_ENABLE_SLICDEBUG
