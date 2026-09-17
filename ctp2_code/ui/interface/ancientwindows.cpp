#include "ctp/c3.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_ctp2/c3ui.h"

#include "ui/aui_common/aui_static.h"
#include "ui/aui_ctp2/c3_static.h"

#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_ctp2/c3windows.h"

#include "ui/aui_ctp2/bevellesswindow.h"
#include "ui/interface/radarwindow.h"
#include "ui/interface/statswindow.h"
#include "ui/interface/controlpanelwindow.h"

#include "ui/interface/ancientwindows.h"

extern sint32 g_ScreenWidth;
extern sint32 g_ScreenHeight;


extern StatsWindow				*g_statsWindow;
















int AncientWindows_PreInitialize()
{
return 0;
}

sint32 ancientwindows_GetControlPieceY( )
{

	return controlpanel_Get()->Y();
}

sint32 ancientwindows_GetControlPieceHeight( )
{

	return controlpanel_Get()->Height();
}


BevelLessWindow		*s_controlPanelLeftHat=nullptr;
BevelLessWindow		*s_controlPanelRightHat=nullptr;

int AncientWindows_Initialize( )
{
return 0;

}

int AncientWindows_Cleanup( )
{

	if (s_controlPanelLeftHat) {
		c3ui_Get()->RemoveWindow(s_controlPanelLeftHat->Id());
		delete s_controlPanelLeftHat;
	}
	if (s_controlPanelRightHat) {
		c3ui_Get()->RemoveWindow(s_controlPanelRightHat->Id());
		delete s_controlPanelRightHat;
	}

return 0;

}
