#include "ctp/c3.h"
#include "ui/aui_common/aui_ui.h"
#include "ui/aui_common/aui_ldl.h"

#include "gs/slic/SlicEngine.h"
#include "ctp/civapp.h"

extern BOOL			g_helpMode;


BOOL HandleGameSpecificLeftClick( void *control )
{
	BOOL handled = FALSE;

	aui_Ldl *ldl = aui_ui_Get()->GetLdl();
	if ( ldl )
	{





			slicengine_Get()->RunUITriggers( aui_Ldl::GetBlock( control ) );

	}

	return handled;
}

BOOL HandleGameSpecificRightClick( void *control )
{
	BOOL handled = FALSE;

	aui_Ldl *ldl = aui_ui_Get()->GetLdl();
	if ( ldl )
	{
		if (g_helpMode) {

				slicengine_Get()->RunHelpTriggers(aui_Ldl::GetBlock(control));
			handled = TRUE;
		}

	}

	return handled;
}
