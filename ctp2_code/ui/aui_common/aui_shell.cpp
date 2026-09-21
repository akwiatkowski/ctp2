#include "ctp/c3.h"
#include "ui/aui_common/aui_screen.h"

#include "ui/aui_common/aui_shell.h"


aui_Shell::aui_Shell(
	AUI_ERRCODE *retval )
{
	*retval = InitCommon();
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}


AUI_ERRCODE aui_Shell::InitCommon( )
{
	m_curScreen = nullptr;

	m_screenHistory = std::make_unique<tech_WLList<uint32>>();
	Assert( m_screenHistory != nullptr );
	if ( !m_screenHistory ) return AUI_ERRCODE_MEMALLOCFAILED;

	return AUI_ERRCODE_OK;
}


aui_Shell::~aui_Shell() = default;


aui_Screen *aui_Shell::LeaveCurrentScreen( )
{
	aui_Screen *prevCurScreen = m_curScreen;

	if ( m_curScreen )
	{
		m_curScreen->Hide();
		m_curScreen = nullptr;
	}

	return prevCurScreen;
}


AUI_ERRCODE aui_Shell::GotoScreen( uint32 id )
{
	aui_Screen *screen = FindScreen( id );
	Assert( screen != nullptr );
	if ( !screen ) return AUI_ERRCODE_HACK;

	if ( m_curScreen ) m_curScreen->Hide();

	m_curScreen = screen;

	m_screenHistory->AddTail( id );

	return m_curScreen->Show();
}


aui_Screen *aui_Shell::GoBackScreen( )
{
	aui_Screen *prevScreen = m_curScreen;

	if ( m_screenHistory->L() >= 2 )
	{
		m_screenHistory->RemoveTail();
		GotoScreen( m_screenHistory->RemoveTail() );
	}

	return prevScreen;
}
