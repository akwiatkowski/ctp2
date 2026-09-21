#ifndef __SERVERSELECTWINDOW_H__
#define __SERVERSELECTWINDOW_H__

#include "ui/netshell/ns_window.h"
#include <memory>
#include "ui/aui_common/aui_action.h"


class ServerSelectWindow : public ns_Window
{
public:

	ServerSelectWindow( AUI_ERRCODE *retval );
	~ServerSelectWindow() override;

protected:
	ServerSelectWindow() : ns_Window() {}
	AUI_ERRCODE	InitCommon( ) override;
	AUI_ERRCODE CreateControls( );

public:
	void	Update( bool wait = false );
	AUI_ERRCODE Idle( ) override;


	enum CONTROL
	{
		CONTROL_FIRST = 0,

		CONTROL_TITLESTATICTEXT = CONTROL_FIRST,

		CONTROL_SELECTSERVERLISTBOX,
		CONTROL_OKBUTTON,
		CONTROL_CANCELBUTTON,
		CONTROL_LAST,
		CONTROL_MAX = CONTROL_LAST - CONTROL_FIRST
	};

protected:

	std::unique_ptr<aui_Action>	m_dbActionArray[ 1 ];

	AUI_ACTION_BASIC(OKButtonAction);
    AUI_ACTION_BASIC(CancelButtonAction);
    AUI_ACTION_BASIC(ServerListBoxAction);
    AUI_ACTION_BASIC(DialogBoxPopDownAction);
};

#endif
