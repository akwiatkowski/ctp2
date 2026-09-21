#ifndef __C3_HEADERSWITCH_H__
#define __C3_HEADERSWITCH_H__

#include "ui/aui_common/aui_radio.h"
#include <memory>

#include "ui/aui_ctp2/patternbase.h"

#define k_C3_HEADERSWITCH_DEFAULTNUMSTATES	3

#define k_C3_HEADERSWITCH_IMAGE		"headerimage"

class c3_Static;

class c3_HeaderSwitch : public aui_Radio, public PatternBase
{
public:

	c3_HeaderSwitch(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR const *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	c3_HeaderSwitch(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		MBCHAR const *text,
		MBCHAR const *pattern,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr,
		sint32 state = 0,
		sint32 numStates = k_C3_HEADERSWITCH_DEFAULTNUMSTATES );
	~c3_HeaderSwitch() override;

	c3_Static *GetImage( ) const { return m_image.get(); }

protected:
	// Out-of-line in c3_headerswitch.cpp: unique_ptr<c3_Static> over a
	// forward-declared type needs the complete type at ctor/dtor instantiation.
	c3_HeaderSwitch();
	AUI_ERRCODE InitCommonLdl( MBCHAR const *ldlBlock );
	AUI_ERRCODE InitCommon( );

protected:
	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

	std::unique_ptr<c3_Static> m_image;
};

#endif
