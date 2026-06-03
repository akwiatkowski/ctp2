#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __PICTUREBUTTON_H__
#define __PICTUREBUTTON_H__

#include "ui/aui_common/aui_button.h"

class Picture;


class PictureButton : public aui_Button
{
public:

	PictureButton(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		MBCHAR *upPicture,
		MBCHAR *downPicture,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );

	PictureButton(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );

	~PictureButton() override;

	AUI_ERRCODE InitCommon(MBCHAR *upPicture, MBCHAR *downPicture, BOOL isLDL = FALSE);

	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;


	Picture *&UpPicture( ) { return m_upPicture; }
	Picture *&DownPicture( ) { return m_downPicture; }

protected:
	Picture *m_upPicture;
	Picture *m_downPicture;
};

#endif
