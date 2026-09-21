#include "ctp/c3.h"

#include <memory>

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_window.h"
#include "ui/aui_common/aui_surface.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_button.h"
#include "ui/aui_common/aui_uniqueid.h"

#include "ui/interface/test.h"
#include "ui/interface/testwin.h"
#include "ui/interface/testwindow.h"

#include "gs/gameobj/message.h"
#include "ui/interface/messageiconwindow.h"
#include "ui/interface/messagewin.h"

#include "ui/aui_ctp2/c3ui.h"


static std::unique_ptr<UITestWindow> g_testWindow;

int uitest_Initialize( void )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;

	g_testWindow = std::make_unique<UITestWindow>( &errcode, aui_UniqueId(),
									100, 100, 200, 100, 16, "upba0104.tif" );
	Assert( AUI_NEWOK( g_testWindow, errcode ));
	if ( !AUI_NEWOK( g_testWindow, errcode )) return -1;

	errcode = c3ui_Get()->AddWindow( g_testWindow.get() );
	Assert(errcode == AUI_ERRCODE_OK);
	if ( errcode != AUI_ERRCODE_OK ) return 11;

	messagewin_InitializeMessages();

	return 1;
}

int uitest_Cleanup( void )
{
	if ( g_testWindow ) {
		g_testWindow.reset();
	}

	return 1;
}

int AddTestWindowToUI( void )
{
	return uitest_Initialize();
}

int RemoveTestWindowToUI( void )
{
	return uitest_Cleanup();
}
