#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __C3BLITTER_H__
#define __C3BLITTER_H__

#ifdef __AUI_USE_DIRECTX__
#include "ui/aui_directx/aui_directblitter.h"
#elif defined(__AUI_USE_SDL__)
#include "ui/aui_sdl/aui_sdlblitter.h"
#endif // __AUI_USE_SDL__

class aui_Surface;

#define k_ChromaKeyValue 0x0000

class C3Blitter;

typedef  AUI_ERRCODE (C3Blitter::*_C3BLIT_PROTO)(	aui_Surface *destSurf,
													RECT *destRect,
													aui_Surface *srcSurf,
													RECT *srcRect,
													uint32 flags );

#ifdef __AUI_USE_DIRECTX__
class C3Blitter : public aui_DirectBlitter
#elif defined(__AUI_USE_SDL__)
class C3Blitter : public aui_SDLBlitter
#endif
{
public:
	C3Blitter();
	~C3Blitter() override = default;

	NakedBltFunc Blt16To16 override;
	NakedColorBltFunc ColorBlt16 override;
	NakedStretchBltFunc StretchBlt16To16 override;
	NakedColorStencilBltFunc ColorStencilBlt16 override;

protected:
	AUI_ERRCODE Blt16To16Fast(
		aui_Surface *destSurf,
		RECT *destRect,
		aui_Surface *srcSurf,
		RECT *srcRect,
		uint32 flags );

	AUI_ERRCODE Blt16To16FastMMX(
		aui_Surface *destSurf,
		RECT *destRect,
		aui_Surface *srcSurf,
		RECT *srcRect,
		uint32 flags );

	AUI_ERRCODE Blt16To16FastFPU(
		aui_Surface *destSurf,
		RECT *destRect,
		aui_Surface *srcSurf,
		RECT *srcRect,
		uint32 flags );

	_C3BLIT_PROTO  _Blt16To16Fast;

	bool CheckMMXTechnology();
};

#endif
