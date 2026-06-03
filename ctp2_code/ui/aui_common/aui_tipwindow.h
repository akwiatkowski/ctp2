#ifndef __AUI_TIPWINDOW_H__
#define __AUI_TIPWINDOW_H__

#include "ui/aui_common/aui_window.h"

class aui_Static;


#define k_AUI_TIPWINDOW_LDL_TIP		"tip"


class aui_TipWindow : public aui_Window
{
public:

	aui_TipWindow(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock );
	aui_TipWindow(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height );
	~aui_TipWindow() override;

protected:
	aui_TipWindow() : aui_Window() {}
	AUI_ERRCODE InitCommonLdl( MBCHAR *ldlBlock );
	AUI_ERRCODE InitCommon( );

public:

	aui_Static *GetStatic( ) const { return m_staticTip; }
	AUI_ERRCODE SetTipText(MBCHAR *text);

	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

protected:
	BOOL		m_allocatedTip;
	aui_Static	*m_staticTip;
};

#endif
