#include "ctp/c3.h"
#include "ui/aui_common/aui_ui.h"
#include "ui/aui_common/aui_ldl.h"

#include "gs/slic/SlicEngine.h"
#include "ctp/civapp.h"

extern BOOL			g_helpMode;
extern aui_UI		*g_ui;
extern CivApp		*g_civApp;

BOOL HandleGameSpecificLeftClick( void *control )
{
	BOOL handled = FALSE;

	aui_Ldl *ldl = g_ui->GetLdl();
	if ( ldl )
	{





			slicengine_Get()->RunUITriggers( ldl->GetBlock( control ) );

	}

	return handled;
}

BOOL HandleGameSpecificRightClick( void *control )
{
	BOOL handled = FALSE;

	aui_Ldl *ldl = g_ui->GetLdl();
	if ( ldl )
	{
		if (g_helpMode) {

				slicengine_Get()->RunHelpTriggers(ldl->GetBlock(control));
			handled = TRUE;
		}

	}

	return handled;
}
