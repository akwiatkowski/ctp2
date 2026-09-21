#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __PICTURE_H__
#define __PICTURE_H__

#include "ui/aui_common/aui_image.h"

#include <memory>

class aui_Surface;




class Picture : public aui_Image
{
public:

	Picture(
		AUI_ERRCODE *retval,
		MBCHAR const *szFileName = nullptr );
	~Picture() override;

	aui_Surface *TheMipmap( ) const { return m_mipmap.get(); }

	AUI_ERRCODE Draw( aui_Surface *pDestSurf, RECT *pDestRect );

	AUI_ERRCODE MakeMipmap( );

protected:
	uint16 AveragePixels( uint16 *pBuffer, sint32 width );

	std::unique_ptr<aui_Surface> m_mipmap;
};

#endif
