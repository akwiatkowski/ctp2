#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef CTP2_TABGROUP_H__
#define CTP2_TABGROUP_H__

#include "ui/aui_common/aui_control.h"

class ctp2_Tab;

class ctp2_TabGroup : public aui_Control {
public:

	ctp2_TabGroup(AUI_ERRCODE *retval, uint32 id, MBCHAR *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr, void *cookie = nullptr);

	~ctp2_TabGroup() override;

	AUI_ERRCODE DoneInstantiatingThis(const MBCHAR *ldlBlock) override;

	AUI_ERRCODE	Show() override;


	AUI_ERRCODE Draw(aui_Surface *surface, sint32 x, sint32 y) override;

	void SelectTab(ctp2_Tab *tab);

	ctp2_Tab *GetCurrentTab() {return m_currentTab;}

private:

	ctp2_Tab *m_currentTab;
};

#endif
