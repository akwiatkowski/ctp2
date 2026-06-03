#ifndef __C3_SWITCH_H__
#define __C3_SWITCH_H__

#include "ui/aui_ctp2/patternbase.h"
#include "ui/aui_common/aui_switch.h"

#define k_C3_SWITCH_DEFAULTNUMSTATES		2
#define k_C3_SWITCH_DEFAULT_BEVELWIDTH		2
#define k_C3_SWITCH_LDL_BEVELWIDTH			"bevelwidth"

class aui_Surface;

class c3_Switch : public aui_Switch, public PatternBase
{
public:

	c3_Switch(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	c3_Switch(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		MBCHAR *pattern,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr,
		sint32 state = 0,
		sint32 numStates = k_C3_SWITCH_DEFAULTNUMSTATES );
	~c3_Switch() override = default;

	void SetBevelWidth(uint32 w) { m_bevelWidth = w; };
	sint32 BevelWidth( ) const { return m_bevelWidth; }
protected:
	c3_Switch() : aui_Switch() {}
	AUI_ERRCODE InitCommonLdl( MBCHAR *ldlBlock );
	AUI_ERRCODE InitCommon( sint32 bevelWidth  );

public:
	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

private:
	sint32	m_bevelWidth;
};

#endif
