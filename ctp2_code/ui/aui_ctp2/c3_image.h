#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __C3_IMAGE_H__
#define __C3_IMAGE_H__

#include "ui/aui_common/aui_image.h"

class c3_Image : public aui_Image
{
public:

	c3_Image(
		AUI_ERRCODE *retval,
		MBCHAR *filename = nullptr );
	~c3_Image() override = default;

	BOOL PtOnImage( POINT *p );

protected:
	c3_Image() : aui_Image() {}
	AUI_ERRCODE InitCommon( );
};

#endif
