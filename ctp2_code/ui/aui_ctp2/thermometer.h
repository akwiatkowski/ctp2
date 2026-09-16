#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __THERMOMETER_H__
#define __THERMOMETER_H__

#include "ui/aui_common/aui_control.h"
#include "ui/aui_ctp2/patternbase.h"

#define k_THERMOMETER_PERCENT_FILLED "percent"
#define k_THERMOMETER_COLOR			 "color"

class Thermometer : public aui_Control, public PatternBase
{
public:

	Thermometer(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR const *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	Thermometer(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		MBCHAR const *pattern,
		sint32 percentFilled = 0,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	~Thermometer() override = default;

	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

	AUI_ERRCODE InitCommonLdl( MBCHAR const *ldlBlock );

	sint32 GetPercentFilled() { return m_percentFilled; }
	void SetPercentFilled( sint32 percentFilled );

protected:
	sint32	m_percentFilled;

};

#endif
