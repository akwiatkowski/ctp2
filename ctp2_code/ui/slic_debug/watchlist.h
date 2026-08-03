#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __WATCH_LIST_H__
#define __WATCH_LIST_H__

#ifdef CTP2_ENABLE_SLICDEBUG

#include <memory>
#include "ui/aui_ctp2/c3_listitem.h"
#include "ui/aui_common/aui_action.h"
#include "ui/aui_ctp2/keyboardhandler.h"
#include "gs/slic/SlicSymbol.h"

class c3_Button;
class c3_PopupWindow;
class c3_ListBox;
template <class T> class PointerList;

typedef void (*WatchListCallback)(sint32 arg);

#define k_MAX_WATCH_LINE 1024

class WatchListItem : public c3_ListItem, public SlicSymbolWatchCallback
{
public:
	WatchListItem(AUI_ERRCODE *retval, sint32 index, MBCHAR *line,
				  MBCHAR *ldlBlock);
	~WatchListItem() override;

	void Update() override;

	void WatchCallback(SlicSymbolData *symbol, bool isAddCallback) override;
	void WatchVariableDeleted(SlicSymbolData *symbol) override;

	MBCHAR *GetLine();
	sint32 Compare(c3_ListItem *item2, uint32 column) override;
	void ToggleBreak() { m_break = !m_break; }

protected:
	WatchListItem() : c3_ListItem() {}

	AUI_ERRCODE InitCommonLdl(MBCHAR *ldlBlock);

private:
	friend class WatchList;

	sint32 m_index;
	MBCHAR m_line[k_MAX_WATCH_LINE];
	PointerList<SlicSymbolData> *m_watching;
	bool m_break;
};

class WatchList : public KeyboardHandler
{
public:
	WatchList(WatchListCallback callback = nullptr, MBCHAR *ldlBlock = nullptr);
	~WatchList() override;

//public:
	sint32 Initialize(MBCHAR *ldlBlock);
	sint32 Cleanup();
	sint32 UpdateData();

	void RemoveWindow();
	void DisplayWindow();

	void kh_Close() override;

	c3_ListBox *GetList() { return m_list.get(); }
	void ShowBreak(sint32 offset);

	void AddExpression(char *exp);
	void Clear();

	c3_Button *GetNewButton() { return m_newButton.get(); }
	c3_Button *GetClearButton() { return m_clearButton.get(); }
	c3_Button *GetExitButton() { return m_exitButton.get(); }

public:
	std::unique_ptr<c3_PopupWindow>	m_window;
	std::unique_ptr<c3_ListBox>	m_list;
	std::unique_ptr<c3_Button>	m_newButton;
	std::unique_ptr<c3_Button>	m_clearButton;
	std::unique_ptr<c3_Button>	m_exitButton;

	WatchListCallback m_callback;
};


void watchlist_Display();
void watchlist_Remove();
void watchlist_Refresh();
void watchlist_AddExpression(char *exp);

#endif//CTP2_ENABLE_SLICDEBUG

#endif
