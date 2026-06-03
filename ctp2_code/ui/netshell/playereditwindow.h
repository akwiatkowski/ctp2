#ifndef __PLAYEREDITWINDOW_H__
#define __PLAYEREDITWINDOW_H__

#include "ui/netshell/ns_window.h"
#include "ui/aui_common/aui_action.h"
#include "ui/netshell/ns_playersetup.h"

class PlayerEditWindow : public ns_Window
{
public:

	PlayerEditWindow( AUI_ERRCODE *retval );
	~PlayerEditWindow() override;

protected:
	PlayerEditWindow() : ns_Window() {}
	AUI_ERRCODE	InitCommon( ) override;
	AUI_ERRCODE CreateControls( );
	nf_PlayerSetup *m_playersetup;

public:
	enum Mode {
		EDIT,
		EDIT_GAMESETUP,
		VIEW
	};
	void SetPlayerSetup(nf_PlayerSetup *p);
	nf_PlayerSetup *GetPlayerSetup( ) const { return m_playersetup; }
	void SetMode(Mode m);
	Mode GetMode();

	AUI_ERRCODE Idle( ) override;
	AUI_ERRCODE SetParent( aui_Region *region ) override;


	enum CONTROL
	{
		CONTROL_FIRST = 0,
		CONTROL_TITLESTATICTEXT = CONTROL_FIRST,
		CONTROL_PLAYERNAMESTATICTEXT,
		CONTROL_PLAYERNAMETEXTFIELD,
		CONTROL_ICQSTATICTEXT,
		CONTROL_ICQTEXTFIELD,
		CONTROL_EMAILSTATICTEXT,
		CONTROL_EMAILTEXTFIELD,
		CONTROL_LOCATIONSTATICTEXT,
		CONTROL_LOCATIONTEXTFIELD,
		CONTROL_EXPERIENCESTATICTEXT,
		CONTROL_EXPERIENCESWITCHGROUP,
		CONTROL_EXPERIENCE0CHECKBOX,
		CONTROL_EXPERIENCE1CHECKBOX,
		CONTROL_EXPERIENCE2CHECKBOX,
		CONTROL_EXPERIENCE3CHECKBOX,
		CONTROL_EXPERIENCE4CHECKBOX,
		CONTROL_EXPERIENCE5CHECKBOX,
		CONTROL_PLAYERINFOSTATICTEXT,
		CONTROL_PLAYERINFOTEXTFIELD,
		CONTROL_OKBUTTON,
		CONTROL_CANCELBUTTON,
		CONTROL_LAST,
		CONTROL_MAX = CONTROL_LAST - CONTROL_FIRST
	};

protected:
	Mode    mode;

    AUI_ACTION_BASIC(OKButtonAction);
    AUI_ACTION_BASIC(CancelButtonAction);
};

#endif
