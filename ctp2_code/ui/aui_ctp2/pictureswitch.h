#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __PICTURESWITCH_H__
#define __PICTURESWITCH_H__

#include "ui/aui_common/aui_switch.h"

class Pattern;
class Picture;

class PictureSwitch : public aui_Switch
{
public:

	PictureSwitch(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		Picture *pictureOn = nullptr,
		Picture *pictureOff = nullptr,
		Picture *pictureActiveOn = nullptr,
		Picture *pictureActiveOff = nullptr,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr,
		BOOL selected = FALSE );
	virtual ~PictureSwitch() {}

	Picture *&ThePictureOn( ) { return m_pictureOn; }
	Picture *&ThePictureOff( ) { return m_pictureOff; }
	Picture *&ThePictureActiveOn( ) { return m_pictureActiveOn; }
	Picture *&ThePictureActiveOff( ) { return m_pictureActiveOff; }

	virtual AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 );

protected:
	Picture *m_pictureOn;
	Picture *m_pictureOff;
	Picture *m_pictureActiveOn;
	Picture *m_pictureActiveOff;
};

#endif
