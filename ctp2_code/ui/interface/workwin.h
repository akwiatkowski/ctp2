#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __WORKWIN_H__
#define __WORKWIN_H__

#include "ui/aui_common/aui_action.h"

AUI_ACTION_BASIC(WorkWinCleanupAction);
AUI_ACTION_BASIC(WorkWinUpdateAction);

sint32 workwin_Initialize( );
sint32 workwin_Cleanup( );

sint32 workwin_Update( );

class WorkWindow;
WorkWindow * workwindow_Get();
void         workwindow_Set(WorkWindow *p);

#endif
