#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __aui_sdl__aui_sdljoystick_h__
#define __aui_sdl__aui_sdljoystick_h__ 1

#include "os/include/ctp2_config.h"

#if defined(__AUI_USE_SDL__)

#include "ui/aui_common/aui_joystick.h"
#include "ui/aui_sdl/aui_sdlinput.h"

class aui_SDLJoystick : public aui_Joystick, public aui_SDLInput {
public:
	aui_SDLJoystick(
		AUI_ERRCODE *retval,
		uint32 lMin = -1000,
		uint32 lMax = 1000);
	~aui_SDLJoystick() override;


	void SetDeviceName( MBCHAR *name ) { strlcpy( m_deviceName, name, sizeof( m_deviceName ) ); };
	MBCHAR *DeviceName( ) { return m_deviceName; };

	uint32 GetLowerMin ( ) { return m_lMin; };
	uint32 GetUpperMax ( ) { return m_lMax; };

protected:
	aui_SDLJoystick() {}

public:
	AUI_ERRCODE Acquire( ) override { return aui_SDLInput::Acquire(); }
	AUI_ERRCODE Unacquire( ) override { return aui_SDLInput::Unacquire(); }

protected:
	MBCHAR	m_deviceName[40];

	uint32	m_lMax;
	uint32	m_lMin;

};

#endif // defined(__AUI_USE_SDL__)

#endif
