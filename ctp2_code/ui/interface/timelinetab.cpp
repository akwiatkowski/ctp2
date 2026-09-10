// Looks like this is dead file, should be removed







#include "ctp/c3.h"

#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_ctp2/ctp2_Window.h"
#include "ui/aui_ctp2/ctp2_button.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/ctp2_Static.h"

#include "gs/database/StrDB.h"

#include "ui/interface/timelinetab.h"

#include "ui/aui_ctp2/linegraph.h"

#include "ui/aui_common/aui_uniqueid.h"

#include "ui/aui_common/aui_stringtable.h"

#include "gfx/gfx_utils/colorset.h"

#include "ui/interface/rankingtab.h"

#include "gs/gameobj/EventTracker.h"


#include "ui/aui_ctp2/c3_button.h"



static sint32			s_currentWonderDisplay;
static c3_Button		*s_eventsInfoButton[17];

TimelineTab::TimelineTab(ctp2_Window *parent) :
	m_rightButton(static_cast<ctp2_Button*>(
		aui_Ldl::GetObject("InfoDialog.TabGroup.Tab2.TabPanel.RightButton"))),
	m_leftButton(static_cast<ctp2_Button*>(
		aui_Ldl::GetObject("InfoDialog.TabGroup.Tab2.TabPanel.LeftButton"))),
	m_infoGraph(static_cast<LineGraph *>(
		aui_Ldl::GetObject("InfoDialog", "TabGroup.Tab2.TabPanel.InfoGraph")))
{

	m_info_window = parent;

	m_infoGraph->SetEventTracker(eventtracker_Get());

	m_infoGraph->EnableYNumber(FALSE);
	m_infoGraph->EnablePrecision(FALSE);


	m_leftButton->SetActionFuncAndCookie(EventsInfoButtonActionCallback, this);
	m_rightButton->SetActionFuncAndCookie(EventsInfoButtonActionCallback, this);




	Assert( m_infoGraph );

	m_infoGraph->Show();




	LoadData();

	m_currentWonderDisplay=0;
}

void TimelineTab::EventsInfoButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	TimelineTab *tab = (TimelineTab *)cookie;
	tab->doButtonCallback((ctp2_Button *)control);
}

void TimelineTab::doButtonCallback(ctp2_Button *button)
{
	if (button == m_leftButton)
	{
		m_currentWonderDisplay--;
		if(m_currentWonderDisplay<0)
			m_currentWonderDisplay=0;
		m_infoGraph->RenderGraph(m_currentWonderDisplay);
		m_infoGraph->ShouldDraw(TRUE);
	}
	else if (button == m_rightButton)
	{
		m_currentWonderDisplay++;
		EventTracker *et = eventtracker_Get();
		if(m_currentWonderDisplay>=et->GetEventCount())
			m_currentWonderDisplay=et->GetEventCount()-1;
		m_infoGraph->RenderGraph(m_currentWonderDisplay);
		m_infoGraph->ShouldDraw(TRUE);
	}
}


void TimelineTab::Show()
{
}

void TimelineTab::Hide()
{
}

void TimelineTab::LoadData()
{

	UpdateGraph();
}

void TimelineTab::UpdateGraph()
{
	sint32 xCount = 0;
	sint32 yCount = 0;
	m_infoGraph->GenrateGraph(xCount, yCount, kRankingOverall);
	m_info_window->Draw();
}

TimelineTab::~TimelineTab()
{
}
