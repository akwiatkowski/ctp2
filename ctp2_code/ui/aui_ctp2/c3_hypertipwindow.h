#ifndef __C3_HYPERTIPWINDOW_H__
#define __C3_HYPERTIPWINDOW_H__

#include "ui/aui_common/aui_tipwindow.h"
#include "ui/aui_ctp2/patternbase.h"

class aui_HyperTextBox;


#define k_C3_HYPERTIPWINDOW_LDL_TIP		"hypertip"


class c3_HyperTipWindow : public aui_TipWindow, public PatternBase
{
public:

	c3_HyperTipWindow(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR const *ldlBlock );
	c3_HyperTipWindow(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		MBCHAR const *pattern );
	~c3_HyperTipWindow() override;

protected:
	c3_HyperTipWindow() : aui_TipWindow() {}
	AUI_ERRCODE InitCommonLdl( MBCHAR const *ldlBlock );
	AUI_ERRCODE InitCommon( );

public:

	aui_HyperTextBox *GetHyperTip( ) const { return m_hyperTip; }
	AUI_ERRCODE SetHyperTipText(MBCHAR const *text);

	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

protected:
	BOOL m_allocatedHyperTip;
	aui_HyperTextBox	*m_hyperTip;
};

#endif
