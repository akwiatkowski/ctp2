#include "ctp/c3.h"
#include "ui/interface/UIUtils.h"
#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_control.h"
#include "ui/aui_common/aui_bitmapfont.h"
#include "ui/aui_ctp2/c3ui.h"


void BlockPush(MBCHAR *path, MBCHAR *addition)
	{

	Assert(addition != nullptr) ;

	Assert(addition[0] != '\0') ;

	Assert(path != nullptr) ;

	Assert((strlen(path)+strlen(addition)+2)<k_AUI_LDL_MAXBLOCK) ;

	Assert(addition[0] != '.') ;
	Assert(addition[strlen(addition)] != '.') ;

	if (path[0]!='\0')
		strlcat(path, ".", k_AUI_LDL_MAXBLOCK) ;

	strlcat(path, addition, k_AUI_LDL_MAXBLOCK) ;
	}

void BlockPop(MBCHAR *path)
	{
	MBCHAR	*p ;

	Assert(path != nullptr) ;

	Assert(path[0] != '\0') ;
	p = strrchr(path, '.') ;

	Assert(p!=nullptr) ;
	if (p==nullptr)
		return ;

	*p = '\0' ;
	}

void ui_TruncateString( aui_Control *control, MBCHAR *str )
{

	MBCHAR name[ _MAX_PATH + 1 ];
	strlcpy( name, str, sizeof(name) );

	if ( !control->GetTextFont() )
		control->TextReloadFont();

	control->GetTextFont()->TruncateString(
		name,
		control->Width() );

	control->SetText(name);
}

MBCHAR *uiutils_ChooseLdl(MBCHAR *firstChoice, MBCHAR *fallback)
{
    if (aui_Ldl::IsValid(firstChoice))
		return firstChoice;

    if (aui_Ldl::IsValid(fallback))
		return fallback;

	return nullptr;
}


