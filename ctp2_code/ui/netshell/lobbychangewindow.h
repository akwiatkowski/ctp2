#ifndef __LOBBYCHANGEWINDOW_H__
#define __LOBBYCHANGEWINDOW_H__

#include "ui/netshell/ns_window.h"
#include "ui/aui_common/aui_action.h"


class LobbyChangeWindow : public ns_Window
{
public:

	LobbyChangeWindow( AUI_ERRCODE *retval );
	~LobbyChangeWindow() override = default;

protected:
	LobbyChangeWindow() : ns_Window() {}
	AUI_ERRCODE	InitCommon( ) override;
	AUI_ERRCODE CreateControls( );

public:
	void	Update( );
	AUI_ERRCODE Idle( ) override;
	AUI_ERRCODE SetParent( aui_Region *region ) override;


	enum CONTROL
	{
		CONTROL_FIRST = 0,
		CONTROL_TITLESTATICTEXT = CONTROL_FIRST,
		CONTROL_CURRENTLOBBYSTATICTEXT,
		CONTROL_CURRENTLOBBYTEXTFIELD,

		CONTROL_LOBBIESLISTBOX,
		CONTROL_OKBUTTON,
		CONTROL_CANCELBUTTON,
		CONTROL_LAST,
		CONTROL_MAX = CONTROL_LAST - CONTROL_FIRST
	};

protected:
    AUI_ACTION_BASIC(OKButtonAction);
	AUI_ACTION_BASIC(CancelButtonAction);
    AUI_ACTION_BASIC(LobbyListBoxAction);
};

#endif
