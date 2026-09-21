#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __SEGMENT_LIST_H__
#define __SEGMENT_LIST_H__

#ifdef CTP2_ENABLE_SLICDEBUG

class SegmentListItem;
class SegmentList;

#include "ui/aui_ctp2/c3_listitem.h"
#include "ui/aui_common/aui_action.h"
#include "ui/aui_ctp2/keyboardhandler.h"

#include <memory>

class SlicSegment;

typedef void (SegmentListCallback)(sint32 arg);

class SegmentListItem : public c3_ListItem
{
public:
	SegmentListItem(AUI_ERRCODE *retval, sint32 index, SlicSegment *segment, MBCHAR *ldlBlock);

	void Update() override;

	SlicSegment *GetSegment() { return m_segment; }
	sint32 Compare(c3_ListItem *item2, uint32 column) override;

	void Open();

protected:
	SegmentListItem() : c3_ListItem() {}

	AUI_ERRCODE InitCommonLdl(SlicSegment *segment, MBCHAR *ldlBlock);

private:
	sint32 m_index;
	SlicSegment *m_segment;
};

class SegmentList : public KeyboardHandler
{
public:
	SegmentList(SegmentListCallback* callback = nullptr, MBCHAR *ldlBlock = nullptr);
	~SegmentList() override;

	void DisplayWindow();
	void RemoveWindow();
	c3_ListBox *GetList() const { return m_list.get(); }

	void kh_Close() override;

private:
	sint32 Initialize(MBCHAR *ldlBlock);
	sint32 UpdateData();

	// Owned controls: ~aui_Region does not delete children, so the
	// destructor releases these explicitly (in declaration order below).
	std::unique_ptr<c3_PopupWindow> m_window;
	std::unique_ptr<c3_ListBox>     m_list;
	std::unique_ptr<c3_Button>      m_watchButton;
	std::unique_ptr<c3_Button>      m_exitButton;


    friend void SegmentListButtonCallback(aui_Control *control, uint32 action, uint32 data, void *cookie);
};


void segmentlist_Display();
void segmentlist_Remove();

#endif //CTP2_ENABLE_SLICDEBUG

#endif
