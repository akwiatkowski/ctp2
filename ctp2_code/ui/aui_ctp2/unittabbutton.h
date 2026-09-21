#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __UNITTABBUTTON_H__
#define __UNITTABBUTTON_H__

#include <array>
#include <memory>
#include "ui/aui_common/aui_control.h"
#include "ui/aui_ctp2/patternbase.h"

#define k_DEFAULT_HEALTHBAR_HEIGHT	4
#define k_CARGO_CAPACITY			5
#define k_CARGO_OFFSET				1
#define k_BIG_CARGO_OFFSET			4

class Thermometer;
class c3_ColorIconButton;
class c3_Static;
class c3_ColoredStatic;
class Unit;
class aui_StringTable;

class UnitTabButton : public aui_Control, public PatternBase
{
public:

	UnitTabButton(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR const *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	UnitTabButton(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		MBCHAR const *pattern,
		sint32 barHeight = k_DEFAULT_HEALTHBAR_HEIGHT,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	~UnitTabButton() override;

	AUI_ERRCODE DrawThis(aui_Surface *surface = nullptr,
								sint32 x = 0,
								sint32 y = 0) override;

	AUI_ERRCODE InitCommon( );

	c3_ColorIconButton	*IconButton( ) const { return m_button.get(); }

	sint32	UpdateData( Unit *unit );

private:
	std::unique_ptr<Thermometer>	m_healthBar;
	std::unique_ptr<c3_ColorIconButton>	m_button;
	std::unique_ptr<c3_Static>	m_fortify;
	std::unique_ptr<c3_Static>	m_veteran;
	std::unique_ptr<c3_Static>	m_arrow;
	std::array<std::unique_ptr<c3_ColoredStatic>, k_CARGO_CAPACITY> m_cargo;

	sint32	m_barHeight;

};

#endif
