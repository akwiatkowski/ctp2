#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __PATTERN_H__
#define __PATTERN_H__

#include "ui/aui_common/aui_image.h"


class Pattern : public aui_Image
{
public:

	Pattern(
		AUI_ERRCODE *retval,
		MBCHAR const *filename,
		MBCHAR const *lightFilename,
		MBCHAR const *darkFilename );
	Pattern(
		AUI_ERRCODE *retval,
		MBCHAR const *filename = nullptr );
	~Pattern() override;

	aui_Image	*LightImage( ) const { return m_lightImage; }
	aui_Image	*DarkImage( ) const { return m_darkImage; }

	AUI_ERRCODE Draw( aui_Surface *pDestSurf, RECT *pDestRect );

	AUI_ERRCODE Draw( aui_Surface *pDestSurf, RECT *pDestRect, RECT *pSrcRect );

	AUI_ERRCODE DrawDither( aui_Surface *pDestSurf, RECT *pDestRect, BOOL flag );
	AUI_ERRCODE DrawDither( aui_Surface *pDestSurf, RECT *pDestRect, BOOL flag, sint32 lighten, sint32 darken );

protected:
	aui_Image *m_lightImage;
	aui_Image *m_darkImage;
};

#endif
