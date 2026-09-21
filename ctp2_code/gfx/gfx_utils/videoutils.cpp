#include "ctp/c3.h"
#include <memory>
#include "ui/aui_common/aui.h"
#include "gs/fileio/CivPaths.h"

#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/pattern.h"
#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_ctp2/videowindow.h"
#include "ui/aui_ctp2/textbutton.h"

#include "gfx/gfx_utils/videoutils.h"

#define k_VIDEO_WINDOW_ID			20000
#define k_ID_VIDEOWINDOW_CLOSE_BOX	20001

std::unique_ptr<C3Window>	g_videoWindow;


void videoutils_VideoWindowCloseBox(aui_Control *control, uint32 action, uint32 data, void *cookie);

void videoutils_Initialize()
{
	g_videoWindow = nullptr;
}

sint32 videoutils_PlayVideoInWindow(MBCHAR *name, MBCHAR *pattern)
{
	AUI_ERRCODE		errcode;

	auto vidWin = std::make_unique<VideoWindow>(&errcode, k_VIDEO_WINDOW_ID, 30, 30, 32, 32, 16, pattern, name, TRUE);

	sint32 controlWidth = 16;
	sint32 controlHeight = 16;
	sint32 controlX = vidWin->Width() - controlWidth - 2;
	sint32 controlY = 2;

	auto button = std::make_unique<TextButton>(
		&errcode,
		k_ID_VIDEOWINDOW_CLOSE_BOX,
		controlX, controlY, controlWidth, controlHeight,
		pattern,
		"",
		videoutils_VideoWindowCloseBox );
	if ( !button ) return -3;

	errcode = vidWin->AddControl( button.release() );
	Assert( errcode == AUI_ERRCODE_OK );
	if ( errcode != AUI_ERRCODE_OK ) return -4;

	errcode = c3ui_Get()->AddWindow(vidWin.release());

	return 0;
}

void videoutils_Cleanup()
{
	if (g_videoWindow != nullptr) {
		c3ui_Get()->RemoveWindow(k_VIDEO_WINDOW_ID);
		g_videoWindow.reset();
	}
	g_videoWindow = nullptr;
}

void videoutils_VideoWindowCloseBox(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	aui_Window *window = control->GetParentWindow();
	if (window != nullptr) {
		c3ui_Get()->RemoveWindow(window->Id());

	}
}
