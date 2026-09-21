#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __SCIENCEWIN_H__
#define __SCIENCEWIN_H__

#include <array>
#include <memory>

#include "ui/aui_ctp2/c3_listitem.h"
#include "ui/aui_common/aui_stringtable.h"
#include "ui/aui_ctp2/c3_updateaction.h"
#include "ui/aui_ctp2/keyboardhandler.h"

class C3Window;
class c3_Static;
class c3_ListBox;
class c3_Button;
class Thermometer;
class Chart;
class c3_Icon;

#define	k_PLAYERS		7
#define k_EXTRA_PLAYERS	8

enum SCI_UPDATE {
	SCI_UPDATE_NOCHART,
	SCI_UPDATE_NOLIST,
	SCI_UPDATE_ALL
};

enum BRANCH {
	BRANCH_PHYSICAL_SCIENCE,
	BRANCH_FLIGHT,
	BRANCH_CONSTRUCTION,
	BRANCH_DEFENSIVE_WAR,
	BRANCH_SEA,
	BRANCH_AGGRESSIVE_WAR,
	BRANCH_CULTURAL_ADVANCEMENT,
	BRANCH_MECHANICAL_DISCOVERIES,
	BRANCH_ELECTRICITY,
	BRANCH_MEDICINE,
	BRANCH_ECONOMICS,

	BRANCH_MAX
};

sint32 sciencewin_Initialize( );
sint32 sciencewin_Cleanup( );

class ScienceWin : public KeyboardHandler {
public:
	std::unique_ptr<C3Window> m_window;

	ScienceWin( );
	~ScienceWin( ) override;

	sint32 Initialize( MBCHAR *windowBlock );
protected:
	std::unique_ptr<c3_Button>		m_closeButton;

	std::unique_ptr<c3_Static>		m_title;

	std::unique_ptr<c3_ListBox>		m_advanceList;

	std::unique_ptr<c3_Button>		m_changeButton;
	std::unique_ptr<c3_Static>		m_researchBox;
	std::unique_ptr<Thermometer>	m_researchMeter;
	std::unique_ptr<c3_Static>		m_researchClock;
	std::unique_ptr<c3_Static>		m_turnsBox;
	std::unique_ptr<c3_Static>		m_costLabel;
	std::unique_ptr<c3_Static>		m_costBox;
	std::unique_ptr<c3_Button>		m_plusButton;
	std::unique_ptr<c3_Button>		m_minusButton;
	std::unique_ptr<c3_Static>		m_percentBox;
	std::unique_ptr<c3_Static>		m_sciLabel;
	std::unique_ptr<c3_Static>		m_sciBeaker;
	std::unique_ptr<c3_Static>		m_sciBox;
	std::unique_ptr<c3_Button>		m_libraryButton;

	std::unique_ptr<Chart>			m_tree;

	std::unique_ptr<c3_Static>		m_knownToLabel;

	std::array<std::unique_ptr<c3_Static>, k_EXTRA_PLAYERS>	m_playerLabel;
	std::array<std::unique_ptr<c3_Icon>, k_EXTRA_PLAYERS>		m_playerFlag;

	std::unique_ptr<aui_StringTable>	m_string;

public:
	void Display( );
	void Remove( );

	void kh_Close() override;

	sint32 UpdateData( SCI_UPDATE update );
	void UpdateList();

	c3_ListBox *AdvanceList( ) { return m_advanceList.get(); }
	c3_Button *PlusButton( ) { return m_plusButton.get(); }
	c3_Button *MinusButton( ) { return m_minusButton.get(); }

	Chart *Tree( ) { return m_tree.get(); }

	MBCHAR *GetString( sint32 index ) { return m_string->GetString(index); }

};

class KnowledgeListItem: public c3_ListItem
{
public:

	KnowledgeListItem(AUI_ERRCODE *retval, sint32 index, MBCHAR *ldlBlock);


	void Update() override;

	sint32	GetIndex() { return m_index; }

protected:
	KnowledgeListItem() : c3_ListItem() {}


	AUI_ERRCODE InitCommonLdl(sint32 index, MBCHAR *ldlBlock);

public:

	sint32 Compare(c3_ListItem *item2, uint32 column) override;

private:
	sint32	m_index;
};

class EmbassyListItem: public c3_ListItem
{
public:

	EmbassyListItem(AUI_ERRCODE *retval, sint32 index, MBCHAR *ldlBlock);


	void Update() override;

	sint32	GetIndex() { return m_index; }

protected:
	EmbassyListItem() : c3_ListItem() {}


	AUI_ERRCODE InitCommonLdl(sint32 index, MBCHAR *ldlBlock);

public:

	sint32 Compare(c3_ListItem *item2, uint32 column) override;

private:
	sint32	m_index;
};

class AdvanceListItem: public c3_ListItem
{
public:

	AdvanceListItem(AUI_ERRCODE *retval, sint32 index, MBCHAR *ldlBlock);
	~AdvanceListItem() override;


	void Update() override;

	sint32	GetIndex() { return m_index; }
	sint32	GetBranchVal( ) { return m_branchVal; }

protected:
	AdvanceListItem() : c3_ListItem() {}


	AUI_ERRCODE InitCommonLdl(sint32 index, MBCHAR *ldlBlock);

public:

	sint32 Compare(c3_ListItem *item2, uint32 column) override;

private:
	sint32	m_index;
	sint32	m_branchVal;
};

sint32 knowledgewin_UpdateFromSwitch( );
sint32 knowledgewin_InitGraphicTrim( MBCHAR *windowBlock );
sint32 knowledgewin_Initialize( );
sint32 knowledgewin_Cleanup( );

class SW_UpdateAction : public c3_UpdateAction
{
public:
	SW_UpdateAction(bool all = false)
    :   c3_UpdateAction (),
        m_all           (all)
    { ; };

	~SW_UpdateAction() override { ; };

	void	Execute
	(
		aui_Control	*	control,
		uint32			action,
		uint32			data
	) override;

private:
	bool    m_all;
};

#endif
