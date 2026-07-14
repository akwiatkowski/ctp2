//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Activision User Interface - image handling
//
//----------------------------------------------------------------------------
//
// Disclaimer
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
// This material has been developed at apolyton.net by the Apolyton CtP2
// Source Code Project. Contact the authors at ctp2source@apolyton.net.
//
//----------------------------------------------------------------------------
//
// Compiler flags
//
// - None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Crash prevented.
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include <vector>

#include "ui/aui_common/aui_blitter.h"
#include "ui/aui_common/aui_Factory.h"
#include "ui/aui_common/aui_image.h"
#include "ui/aui_common/aui_memmap.h"
#include "ui/aui_common/aui_pixel.h"
#include "ui/aui_common/aui_ui.h"

aui_Image::aui_Image(
	AUI_ERRCODE *retval,
	MBCHAR const * filename )
	:
	aui_Base()
{

	if (filename==nullptr)
	{
		*retval = AUI_ERRCODE_OK;
		return;
	}

	*retval = InitCommon( filename );
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}

AUI_ERRCODE aui_Image::InitCommon( MBCHAR const *filename )
{
	m_surface = nullptr,
	m_format = nullptr;

	AUI_ERRCODE errcode = SetFilename( filename );
	Assert( AUI_SUCCESS(errcode) );
	if ( !AUI_SUCCESS(errcode) ) return errcode;

	return AUI_ERRCODE_OK;
}

aui_Image::~aui_Image()
{
	Unload();
}

AUI_ERRCODE aui_Image::SetFilename( MBCHAR const *filename )
{
	Unload();	// deletes and NULLs m_format and m_surface

	memset( m_filename, '\0', sizeof( m_filename ) );

	if ( !filename ) return AUI_ERRCODE_INVALIDPARAM;

	strlcpy( m_filename, filename, sizeof( m_filename ) );

	if (aui_ui_Get() && aui_ui_Get()->TheMemMap())
	{
		m_format = static_cast<aui_ImageFormat *>
						(aui_ui_Get()->TheMemMap()->GetFileFormat(m_filename));
	}

	Assert(m_format);
	return m_format ? AUI_ERRCODE_OK : AUI_ERRCODE_MEMALLOCFAILED;
}

AUI_ERRCODE aui_Image::Load( )
{
	Assert(m_format);
	if ( !m_format ) return AUI_ERRCODE_INVALIDPARAM;

	if ( m_surface ) return AUI_ERRCODE_OK;

	return m_format->Load(m_filename, this);
}

AUI_ERRCODE aui_Image::Unload( )
{
	if (aui_ui_Get() && aui_ui_Get()->TheMemMap())
	{
		aui_ui_Get()->TheMemMap()->ReleaseFileFormat(m_format);
		m_format = nullptr;
	}

	delete  m_surface;
	m_surface = nullptr;

	return AUI_ERRCODE_OK;
}

AUI_ERRCODE aui_Image::LoadEmpty( sint32 width, sint32 height, sint32 bpp )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;

	m_surface = aui_Factory::new_Surface(errcode, width, height);

	Assert( AUI_NEWOK(m_surface, errcode) );
	return errcode;
}

AUI_ERRCODE aui_Image::LoadFileMapped( sint32 width, sint32 height,
                                       sint32 bpp, sint32 pitch,
                                       uint8 *buffer )
{
	AUI_ERRCODE retcode = AUI_ERRCODE_OK;

	m_surface = new aui_Surface(
	    &retcode,
	    width,
	    height,
	    bpp,
	    pitch,
	    buffer );

	Assert( AUI_NEWOK(m_surface,retcode) );

	return retcode;
}

AUI_ERRCODE aui_BmpImageFormat::Load(MBCHAR const * filename, aui_Image *image )
{
	AUI_ERRCODE retcode = AUI_ERRCODE_OK;

#ifdef WIN32
	uint8 *filebits = aui_ui_Get()->TheMemMap()->GetFileBits( filename );
	Assert( filebits != NULL );
	if ( !filebits ) return AUI_ERRCODE_HACK;

	BITMAPFILEHEADER bfh;
	memcpy( &bfh, filebits, sizeof( bfh ) );
	uint32 foffset = sizeof( bfh );

	if ( LOBYTE(bfh.bfType) != 'B' || HIBYTE(bfh.bfType) != 'M' ) {
		aui_ui_Get()->TheMemMap()->ReleaseFileBits( filebits );
		return AUI_ERRCODE_LOADFAILED;
	}

	BITMAPINFOHEADER bih;
	memcpy( &bih, filebits + foffset, sizeof( bih ) );
	foffset += sizeof( bih );

	if ( bih.biCompression != BI_RGB ) {
		aui_ui_Get()->TheMemMap()->ReleaseFileBits( filebits );
		return AUI_ERRCODE_LOADFAILED;
	}

	RGBQUAD *rgbq = NULL;
	if ( bih.biBitCount == 8 &&
		(bfh.bfOffBits - (sizeof(bih) + sizeof(bfh)) == (256 * sizeof( RGBQUAD ))) ) {

		rgbq = new RGBQUAD[256];
		Assert( rgbq != NULL );
		if ( !rgbq ) {
			aui_ui_Get()->TheMemMap()->ReleaseFileBits( filebits );
			return AUI_ERRCODE_LOADFAILED;
		}

		memcpy( rgbq, filebits + foffset, ( 256 * sizeof( RGBQUAD ) ) );
		foffset += 256 * sizeof( RGBQUAD );
	}

	uint32 width = bih.biWidth;
	uint32 height = bih.biHeight >= 0 ? bih.biHeight : -bih.biHeight;

	sint32 temp = width * bih.biBitCount / 8;
	uint32 bmpPitch = temp + Mod(-temp,sizeof( LONG ));

	uint32 bpp = aui_ui_Get()->BitsPerPixel();

	AUI_ERRCODE errcode = image->LoadEmpty( width, height, bpp );
	Assert( AUI_SUCCESS(errcode) );
	if ( !AUI_SUCCESS(errcode) )
	{
		aui_ui_Get()->TheMemMap()->ReleaseFileBits( filebits );
		if ( rgbq ) delete [] rgbq;
		return AUI_ERRCODE_LOADFAILED;
	}

	aui_Surface *surface = image->TheSurface();

	switch ( bpp )
	{
	case 8:

		Assert( FALSE );
		retcode = AUI_ERRCODE_LOADFAILED;
		break;

	case 16:
		switch ( bih.biBitCount )
		{
		case 8:
			errcode = aui_Pixel::Convert8To16(
				surface,
				filebits + foffset,
				width,
				height,
				bmpPitch,
				rgbq );
			Assert( AUI_SUCCESS(errcode) );
			if ( !AUI_SUCCESS(errcode) )
				retcode = AUI_ERRCODE_LOADFAILED;
			break;

		case 16:

			Assert( FALSE );
			retcode = AUI_ERRCODE_LOADFAILED;
			break;

		case 24:
			errcode = aui_Pixel::Convert24To16(
				surface,
				filebits + foffset,
				width,
				height,
				bmpPitch );
			Assert( AUI_SUCCESS(errcode) );
			if ( !AUI_SUCCESS(errcode) )
				retcode = AUI_ERRCODE_LOADFAILED;
			break;

		default:

			Assert( FALSE );
			retcode = AUI_ERRCODE_LOADFAILED;
		}
		break;

	case 24:

		Assert( FALSE );
		retcode = AUI_ERRCODE_LOADFAILED;
		break;

	default:

		Assert( FALSE );
		retcode = AUI_ERRCODE_LOADFAILED;
		break;
	}

	if ( rgbq ) delete [] rgbq;
	aui_ui_Get()->TheMemMap()->ReleaseFileBits( filebits );

	if ( bih.biHeight > 0 )
	{
		LPVOID bits = NULL;
		errcode = surface->Lock( NULL, &bits, 0 );
		Assert( AUI_SUCCESS(errcode) );
		if ( !AUI_SUCCESS(errcode) )
			retcode = AUI_ERRCODE_SURFACELOCKFAILED;
		else
		{
			const sint32 pitch = surface->Pitch();

			std::vector<uint8> temp( pitch );
			uint8 *top = (uint8 *)bits;
			uint8 *bot = top + pitch * ( surface->Height() - 1 );
			for ( sint32 i = surface->Height() / 2; i; i-- )
			{
				memcpy( temp.data(), top, pitch );
				memcpy( top, bot, pitch );
				memcpy( bot, temp.data(), pitch );

				top += pitch;
				bot -= pitch;
			}

			errcode = surface->Unlock( bits );
			Assert( AUI_SUCCESS(errcode) );
			if ( !AUI_SUCCESS(errcode) )
				retcode = AUI_ERRCODE_SURFACEUNLOCKFAILED;
		}
	}

	return retcode;
#elif defined(__AUI_USE_SDL__)
        printf("%s L%d: image %s!\n", __FILE__, __LINE__, filename); //is this ever called?
	assert(0);
	SDL_Surface *bmp = SDL_LoadBMP(filename);
	SDL_Surface *surf = nullptr;
	SDL_PixelFormat fmt = { 0 };
//#if 0
//	if (aui_image_SDLPixelFormat(image, &fmt)) {
//		surf = SDL_ConvertSurface(bmp, &fmt, 0);
//	}
//#endif

        printf("%s L%d: image %s!\n", __FILE__, __LINE__, filename);
        if (aui_ui_Get()->Primary()->BitsPerPixel() != 16)
            printf("%s L%d: bpp %d", __FILE__, __LINE__,  aui_ui_Get()->Primary()->BitsPerPixel());
        if (bmp->format->Gmask >> bmp->format->Gshift == 0x3F)
            printf("%s L%d: 565 image!\n", __FILE__, __LINE__);
        if (bmp->format->Gmask >> bmp->format->Gshift == 0x1F)
            printf("%s L%d: 555 image!\n", __FILE__, __LINE__);
	if (nullptr == surf) {
		// SDL2: SDL_DisplayFormat removed; convert to a reasonable default format
		surf = SDL_ConvertSurfaceFormat(bmp, SDL_PIXELFORMAT_RGB565, 0);
	}
	CTP2_SDL_DestroySurface(bmp);
	if (nullptr == surf)
		return AUI_ERRCODE_LOADFAILED;
	//surface = image->TheSurface();
	//image->AttachSurface(surf);
        //The new surface should be assinged to the image-surfec but how???
        //conversion needed from SDL_Surface to aui_Surface
        //also using SDL_BlitSurface might be good in case surf 565 and img 555
        /* //this part doesn't create a visible effect:
        AUI_ERRCODE *retval;

        if (m_surface) {
            delete m_surface;
            }

        m_surface = new aui_Surface(retval, s->w, s->h, s->format->BitsPerPixel, s->pitch, (uint8 *) s->pixels);
        Assert( AUI_SUCCESS(*retval) );
        if ( !AUI_SUCCESS(*retval) ) return;
        */

        if (image->TheSurface()) {
	} else {
	}
	return retcode;
#else
	return AUI_ERRCODE_LOADFAILED;
#endif
}

void aui_Image::SetChromakey(sint32 r, sint32 g, sint32 b)
{
	aui_Surface		*surf = TheSurface();

	Assert(surf != nullptr);
	if (surf == nullptr)
		return;

	Assert(r >= 0);
	Assert(r <= 255);
	Assert(g >= 0);
	Assert(g <= 255);
	Assert(b >= 0);
	Assert(b <= 255);

	surf->SetChromaKey((uint8) r, (uint8) g,(uint8) b);
}
