//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Information window
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
// - Added Update function so that the info window doesn't need to be closed
//   for update during the turns. - Aug 7th 2005 Martin
// - Added cleanup method. (Sep 13th 2005 Martin Gühmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/interface/infowindow.h"

#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/ctp2_button.h"
#include "ui/aui_ctp2/ctp2_Tab.h"
#include "ui/aui_ctp2/ctp2_TabGroup.h"
#include "ui/aui_ctp2/ctp2_Window.h"
#include "ui/interface/rankingtab.h"
#include "ui/interface/scoretab.h"
#include "ui/interface/WonderTab.h"


static std::unique_ptr<InfoWindow> s_InfoWindow;

InfoWindow::InfoWindow()
:
	m_window        (static_cast<ctp2_Window*>
                        (aui_Ldl::BuildHierarchyFromRoot("InfoDialog"))
                    ),
	m_closeButton   (nullptr),
	m_ranking_tab   (nullptr),
    m_score_tab     (std::make_unique<ScoreTab>()),
    m_wonder_tab    (nullptr)
{
	Assert(m_window);

    m_ranking_tab   = std::make_unique<RankingTab>(m_window);
	m_wonder_tab    = std::make_unique<WonderTab>(m_window);

    m_closeButton   = static_cast<ctp2_Button*>
                        (aui_Ldl::GetObject("InfoDialog.CloseButton"));
    Assert(m_closeButton);
    if (m_closeButton)
    {
	    m_closeButton->SetActionFuncAndCookie(&CloseButtonActionCallback, this);
    }
}

InfoWindow::~InfoWindow()
{
    // unique_ptr members; reset in the original explicit order.
    m_ranking_tab.reset();
    m_score_tab.reset();
    m_wonder_tab.reset();

    if (m_window)
    {
        aui_Ldl::DeleteHierarchyFromRoot("InfoDialog");
    }
}

void InfoWindow::SelectRankingTab()
{
	Open();
	ctp2_TabGroup *tabGroup = (ctp2_TabGroup *)aui_Ldl::GetObject("InfoDialog.TabGroup");
	tabGroup->SelectTab((ctp2_Tab *)aui_Ldl::GetObject("InfoDialog.TabGroup.Tab3"));
}

void InfoWindow::SelectWonderTab()
{
	Open();
	ctp2_TabGroup *tabGroup = (ctp2_TabGroup *)aui_Ldl::GetObject("InfoDialog.TabGroup");
	tabGroup->SelectTab((ctp2_Tab *)aui_Ldl::GetObject("InfoDialog.TabGroup.Tab2"));
}

void InfoWindow::SelectScoreTab()
{
	Open();
	ctp2_TabGroup *tabGroup = (ctp2_TabGroup *)aui_Ldl::GetObject("InfoDialog.TabGroup");
	tabGroup->SelectTab((ctp2_Tab *)aui_Ldl::GetObject("InfoDialog.TabGroup.Tab1"));
}

void InfoWindow::Open()
{
	if (!s_InfoWindow)
		s_InfoWindow = std::make_unique<InfoWindow>();

	c3ui_Get()->AddWindow(s_InfoWindow->m_window);
	s_InfoWindow->Show();
}

/// Update the data, without modifying the current window status
void InfoWindow::Update()
{
    if (s_InfoWindow)
    {
        s_InfoWindow->UpdateData();
    }
}

void InfoWindow::Close()
{
    if (s_InfoWindow)
    {
        s_InfoWindow->Hide();
    }
}

/// Update the data at all tabs
void InfoWindow::UpdateData()
{
    m_score_tab->Update();
    m_ranking_tab->LoadData();
    m_wonder_tab->UpdateList();
}

/// Show the window, after updating the data
void InfoWindow::Show()
{
    UpdateData();
    m_window->Show();
}

void InfoWindow::Hide()
{
    m_window->Hide();
	c3ui_Get()->RemoveWindow(m_window->Id());
}

void InfoWindow::CloseButtonActionCallback
(
    aui_Control *   control,
	uint32          action,
    uint32          data,
    void *          cookie
)
{
	if (action != static_cast<uint32>(AUI_BUTTON_ACTION_EXECUTE))
		return;

	static_cast<InfoWindow*>(cookie)->Hide();
}

//----------------------------------------------------------------------------
//
// Name       : InfoWindow::Cleanup
//
// Description: Deletes the static InfoWindow.
//
// Parameters : -
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
void InfoWindow::Cleanup()
{
	Close();
    s_InfoWindow.reset();
}
