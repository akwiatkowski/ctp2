#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __PIXELUTILS_H__
#define __PIXELUTILS_H__

#define BLEND_LEVELS		16
#define BLEND_MAX_VALUE		64
#define RGB_VALUES			32768
#define k_MAX_PERCENT		128
#define k_PERCENT_SHIFT		7

#include "gfx/gfx_utils/pixeltypes.h"






typedef union
{
  Pixel8  *b_ptr;
  Pixel16 *w_ptr;
  Pixel32 *l_ptr;

} PixelAddress;

void pixelutils_Initialize();

Pixel16 *RGB32ToRGB16(char *buf, uint16 width, uint16 height);

void RGB32Components(Pixel32 pixel, Pixel16 *r, Pixel16 *g, Pixel16 *b, Pixel16 *a);
Pixel32 ComponentsToRGB32(Pixel16 r, Pixel16 g, Pixel16 b, Pixel16 a) ;
void RGB32Info(Pixel32 pixel, Pixel16 *outPixel, unsigned char *alpha);
void pixelutils_ComputeBlendTable();

// Shared inline-pixel-blending state.  Hoisted to file scope so the
// inline functions below don't each carry their own function-scoped
// extern declarations (9 + 3 of them, all referenced by the globals
// ratchet — see test_player_view.cpp).
// Display pixel format is 5-6-5 (vs 5-5-5).  Definition + accessors
// live in pixelutils.cpp; civ3_main probes the surface format at
// startup and calls is_565_Set().  All readers (~60 sites across gfx/
// and ui/) use is_565_Get().  Inline funcs in this header below
// likewise dispatch through the accessor.
bool is_565_Get();
void is_565_Set(bool v);
extern short  gPixelTable[BLEND_LEVELS][BLEND_MAX_VALUE][BLEND_MAX_VALUE];


inline Pixel16 pixelutils_Blend(Pixel16 pixel1, Pixel16 pixel2, short blend)
{
	short			 r1;
	short			 g1;
	short			 b1;
	short			 r2;
	short			 g2;
	short			 b2;
	short			 r0;
	short			 g0;
	short			 b0;

	if (is_565_Get())
	{
		r1 = (short)((pixel1 & 0xF800) >> 10) ;
		g1 = (short)((pixel1 & 0x07E0) >> 5);
		b1 = (short)((pixel1 & 0x001F) << 1) ;

		r2 = (short)((pixel2 & 0xF800) >> 10);
		g2 = (short)((pixel2 & 0x07E0) >> 5);
		b2 = (short)((pixel2 & 0x001F) << 1);

		r0 = (short)((gPixelTable[blend][r1][r2] & 0xFFFE) << 10) ;
		g0 = (short)(gPixelTable[blend][g1][g2] << 5) ;
		b0 = (short)(gPixelTable[blend][b1][b2] >> 1) ;
	}
	else
	{
		r1 = (short)((pixel1 & 0x7C00) >> 9) ;
		g1 = (short)((pixel1 & 0x03E0) >> 4) ;
		b1 = (short)((pixel1 & 0x001F) << 1) ;

		r2 = (short)((pixel2 & 0x7C00) >> 9) ;
		g2 = (short)((pixel2 & 0x03E0) >> 4) ;
		b2 = (short)((pixel2 & 0x001F) << 1) ;

		r0 = (short)((gPixelTable[blend][r1][r2] & 0xFFFE) << 9) ;
		g0 = (short)((gPixelTable[blend][g1][g2] & 0xFFFE) << 4) ;
		b0 = (short)(gPixelTable[blend][b1][b2] >> 1) ;
	}

	return static_cast<Pixel16>(r0 | g0 | b0);
}

inline Pixel16 pixelutils_Additive(Pixel16 pixel1, Pixel16 pixel2)
{

	Pixel16				 r;
	Pixel16				 g;
	Pixel16				 b;
	Pixel16				 sum = (short)(pixel2 & 0x1F) ;

	if (is_565_Get())
	{
		r = ((pixel1 & 0xF800) >> 11) + sum;
		g = ((pixel1 & 0x07E0) >> 5)  + (sum << 1);
		b = ((pixel1 & 0x001F))       + sum;

		if (r > 0x001F) r = 0x001F;
		if (g > 0x003F) g = 0x003F;
		if (b > 0x001F) b = 0x001F;

		return static_cast<Pixel16>((r<<11) | (g<<5) | b);
	}
	else
	{
		r = ((pixel1 & 0x7C00) >> 10) + sum;
		g = ((pixel1 & 0x03E0) >> 5)  + sum;
		b = ((pixel1 & 0x001F))       + sum;

		if (r > 0x001F) r = 0x001F;
		if (g > 0x001F) g = 0x001F;
		if (b > 0x001F) b = 0x001F;

		return static_cast<Pixel16>((r<<10) | (g<<5) | b);
	}
}

inline Pixel16 pixelutils_BlendFast(sint32 pixel1, sint32 pixel2, sint32 blend)
{
	sint32 rb2;
	sint32 g2;
	sint32 rb0;
	sint32 g0;

	if (is_565_Get())
	{
		rb2 = (pixel2 & 0xF81F);







		rb0 = ( ((rb2<<5) + (blend * ((pixel1 & 0xF81F)-rb2) )) >> 5)  & 0xF81F;

		g2 = (pixel2 & 0x07E0);
		g0 = (((g2<<5)+blend*((pixel1 & 0x07E0)-g2))>>5) & 0x07E0;
	}
	else
	{
		rb2 = (pixel2 & 0x7C1F);
		rb0 = (((rb2<<5)+blend*((pixel1 & 0x7C1F)-rb2))>>5) & 0x7C1F;

		g2 = (pixel2 & 0x03E0);
		g0 = (((g2<<5)+blend*((pixel1 & 0x03E0)-g2))>>5) & 0x07E0;
	}

	Pixel16 pixel = (Pixel16)(rb0|g0);
	return ((!pixel) ? static_cast<Pixel16>(0x0001) : pixel);
}

inline Pixel16 pixelutils_Shadow(Pixel16 pixel)
{

	if (is_565_Get())
      return static_cast<Pixel16>((pixel&0xF7DF)>>1);
	else
	{
	    short				 r;
	    short				 g;
	    short				 b;

		r = (pixel & 0x7C00) >> 11;
		g = (pixel & 0x03E0) >> 6;
		b = (pixel & 0x001F) >> 1;

		Pixel16 p = static_cast<Pixel16>((r<<10) | (g<<5) | b);
		if (!p) p = 0x0001;
		return p;
	}
}

inline Pixel16 pixelutils_Lightening(Pixel16 pixel)
{
	short		 r;
	short		 g;
	short		 b;

	if (is_565_Get())
	{
		r = (pixel & 0xF800) >> 10;
		if (r > 0x001F)
			r = 0x001F;
		g = (pixel & 0x07E0) >> 4;
		if (g > 0x003F)
			g = 0x003F;
		b = (pixel & 0x001F) << 1;
		if (b > 0x001F)
			b = 0x001F;

		return static_cast<Pixel16>((r<<11) | (g<<5) | b);
	}
	else
	{
		r = (pixel & 0x7C00) >> 9;
		if (r > 0x001F)
			r = 0x001F;
		g = (pixel & 0x03E0) >> 4;
		if (g > 0x001F)
			g = 0x001F;
		b = (pixel & 0x001F) << 1;
		if (b > 0x001F)
			b = 0x001F;

		return static_cast<Pixel16>((r<<10) | (g<<5) | b);
	}
}

inline Pixel16 pixelutils_PercentDarken(Pixel16 pixel, sint32 percent)
{
	sint32 r32;
	sint32 g32;
	sint32 b32;
	sint16 r;
	sint16 g;
	sint16 b;
	sint32 newPercent = k_MAX_PERCENT - percent;

	if (is_565_Get())
	{
		r32 = (pixel & 0xF800) >> 11;
		g32 = (pixel & 0x07E0) >> 5;
		b32 = (pixel & 0x001F);

		r32 = (r32 * newPercent) >> k_PERCENT_SHIFT;
		r = (sint16)r32;
		g32 = (g32 * newPercent) >> k_PERCENT_SHIFT;
		g = (sint16)g32;
		b32 = (b32 * newPercent) >> k_PERCENT_SHIFT;
		b = (sint16)b32;

		return static_cast<Pixel16>((r<<11) | (g<<5) | b);
	}
	else
	{
		r32 = (pixel & 0x7C00) >> 10;
		g32 = (pixel & 0x03E0) >> 5;
		b32 = (pixel & 0x001F);

		r32 = (r32 * newPercent) >> k_PERCENT_SHIFT;
		r = (sint16)r32;
		g32 = (g32 * newPercent) >> k_PERCENT_SHIFT;
		g = (sint16)g32;
		b32 = (b32 * newPercent) >> k_PERCENT_SHIFT;
		b = (sint16)b32;

		return static_cast<Pixel16>((r<<10) | (g<<5) | b);
	}
}

inline Pixel16 pixelutils_PercentLighten(Pixel16 pixel, sint32 percent)
{
	sint32 r32;
	sint32 g32;
	sint32 b32;
	sint16 r;
	sint16 g;
	sint16 b;
	sint32 newPercent = k_MAX_PERCENT + percent;

	if (is_565_Get())
	{
		r32 = (pixel & 0xF800) >> 11;
		g32 = (pixel & 0x07E0) >> 5;
		b32 = (pixel & 0x001F);

		r32 = (r32 * newPercent) >> k_PERCENT_SHIFT;
		if (r32 > 0x001F)
			r32 = 0x001F;
		r = (sint16)r32;
		g32 = (g32 * newPercent) >> k_PERCENT_SHIFT;
		if (g32 > 0x003F)
			g32 = 0x003F;
		g = (sint16)g32;
		b32 = (b32 * newPercent) >> k_PERCENT_SHIFT;
		if (b32 > 0x001F)
			b32 = 0x001F;
		b = (sint16)b32;

		return static_cast<Pixel16>((r<<11) | (g<<5) | b);
	}
	else
	{
		r32 = (pixel & 0x7C00) >> 10;
		g32 = (pixel & 0x03E0) >> 5;
		b32 = (pixel & 0x001F);

		r32 = (r32 * newPercent) >> k_PERCENT_SHIFT;
		if (r32 > 0x001F)
			r32 = 0x001F;
		r = (sint16)r32;
		g32 = (g32 * newPercent) >> k_PERCENT_SHIFT;
		if (g32 > 0x001F)
			g32 = 0x001F;
		g = (sint16)g32;
		b32 = (b32 * newPercent) >> k_PERCENT_SHIFT;
		if (b32 > 0x001F)
			b32 = 0x001F;
		b = (sint16)b32;

		return static_cast<Pixel16>((r<<10) | (g<<5) | b);
	}
}




inline Pixel16 pixelutils_Convert565to555(Pixel16 pixel)
{

	if (is_565_Get()) return pixel;

	return static_cast<Pixel16>(((pixel & 0xFFC0) >> 1) | (pixel & 0x001F));
}

inline Pixel16 pixelutils_Convert555to565(Pixel16 pixel)
{

	if (!is_565_Get()) return pixel;

	return static_cast<Pixel16>(((pixel & 0x7FE0) << 1) | (pixel & 0x001F));
}










inline Pixel16 pixelutils_Blend_565(Pixel16 pixel1, Pixel16 pixel2, short blend)
{
	Pixel16			 r1;
	Pixel16			 g1;
	Pixel16			 b1;
	Pixel16			 r2;
	Pixel16			 g2;
	Pixel16			 b2;
	Pixel16			 r0;
	Pixel16			 g0;
	Pixel16			 b0;

	r1 = ((pixel1 & 0xF800) >> 10) ;
	g1 = ((pixel1 & 0x07E0) >> 5);
	b1 = ((pixel1 & 0x001F) << 1) ;

	r2 = ((pixel2 & 0xF800) >> 10);
	g2 = ((pixel2 & 0x07E0) >> 5);
	b2 = ((pixel2 & 0x001F) << 1);

	r0 = ((gPixelTable[blend][r1][r2] & 0xFFFE) << 10) ;
	g0 = ( gPixelTable[blend][g1][g2] << 5) ;
	b0 = ( gPixelTable[blend][b1][b2] >> 1) ;

	return static_cast<Pixel16>(r0 | g0 | b0);

}

inline Pixel16 pixelutils_Additive_565(Pixel16 pixel1, Pixel16 pixel2)
{
	Pixel16				 r;
	Pixel16				 g;
	Pixel16				 b;
	Pixel16				 sum = (short)(pixel2 & 0x1F) ;

	r = ((pixel1 & 0xF800) >> 11) + sum;
	g = ((pixel1 & 0x07E0) >> 5)  + (sum << 1);
	b = ((pixel1 & 0x001F))       + sum;

	if (r > 0x001F) r = 0x001F;
	if (g > 0x003F) g = 0x003F;
	if (b > 0x001F) b = 0x001F;

	return static_cast<Pixel16>((r<<11) | (g<<5) | b);
}

inline Pixel16 pixelutils_BlendFast_565(sint32 pixel1, sint32 pixel2, sint32 blend)
{

	sint32 rb2;
	sint32 g2;
	sint32 rb0;
	sint32 g0;

	rb2 = (pixel2 & 0xF81F);





	rb0 = ( ((rb2<<5) + (blend * ((pixel1 & 0xF81F)-rb2) )) >> 5)  & 0xF81F;

	g2 = (pixel2 & 0x07E0);
	g0 = (((g2<<5)+blend*((pixel1 & 0x07E0)-g2))>>5) & 0x07E0;

	return static_cast<Pixel16>(rb0 | g0);
}

inline Pixel16 pixelutils_Shadow_565(Pixel16 pixel)
{
    return static_cast<Pixel16>((pixel&0xF7DF)>>1);
}

inline Pixel16 pixelutils_Lightening_565(Pixel16 pixel)
{
	short r;
	short g;
	short b;

	r = (pixel & 0xF800) >> 10;
	if (r > 0x001F)
   		r = 0x001F;

	g = (pixel & 0x07E0) >> 4;
	if (g > 0x003F)
   		g = 0x003F;

	b = (pixel & 0x001F) << 1;
	if (b > 0x001F)
   		b = 0x001F;

    return static_cast<Pixel16>((r<<11) | (g<<5) | b);
}

inline Pixel16 pixelutils_PercentDarken_565(Pixel16 pixel, sint32 percent)
{
	sint32 r32;
	sint32 g32;
	sint32 b32;
	sint16 r;
	sint16 g;
	sint16 b;
	sint32 newPercent = k_MAX_PERCENT - percent;

	r32 = (pixel & 0xF800) >> 11;
	g32 = (pixel & 0x07E0) >> 5;
	b32 = (pixel & 0x001F);

	r32 = (r32 * newPercent) >> k_PERCENT_SHIFT;
	r = (sint16)r32;
	g32 = (g32 * newPercent) >> k_PERCENT_SHIFT;
	g = (sint16)g32;
	b32 = (b32 * newPercent) >> k_PERCENT_SHIFT;
	b = (sint16)b32;

	return static_cast<Pixel16>((r<<11) | (g<<5) | b);
}

inline Pixel16 pixelutils_PercentLighten_565(Pixel16 pixel, sint32 percent)
{
	sint32 r32;
	sint32 g32;
	sint32 b32;
	sint16 r;
	sint16 g;
	sint16 b;
	sint32 newPercent = k_MAX_PERCENT + percent;

	r32 = (pixel & 0xF800) >> 11;
	g32 = (pixel & 0x07E0) >> 5;
	b32 = (pixel & 0x001F);

	r32 = (r32 * newPercent) >> k_PERCENT_SHIFT;
	if (r32 > 0x001F)
		r32 = 0x001F;
	r = (sint16)r32;
	g32 = (g32 * newPercent) >> k_PERCENT_SHIFT;
	if (g32 > 0x003F)
		g32 = 0x003F;
	g = (sint16)g32;
	b32 = (b32 * newPercent) >> k_PERCENT_SHIFT;
	if (b32 > 0x001F)
		b32 = 0x001F;
	b = (sint16)b32;

	return static_cast<Pixel16>((r<<11) | (g<<5) | b);
}

inline Pixel16 pixelutils_Desaturate_565(Pixel16 pixel)
{
	sint32 ave = (((pixel & 0xF800) >> 11) + ((pixel & 0x07E0) >> 6) + (pixel & 0x001F)+128)>>2;

	return ((Pixel16)(((ave & 0x1F) << 11) | ((ave & 0x3F) << 6) | (ave & 0x1F)));
}

Pixel32 pixelutils_Blend32_565(Pixel32 pixel1, Pixel32 pixel2, short blend);
Pixel32 pixelutils_Additive32_565(Pixel32 pixel1, Pixel32 pixel2);
Pixel32 pixelutils_BlendFast32_565(sint32 pixel1, sint32 pixel2, sint32 blend);

inline Pixel32 pixelutils_Shadow32_565(Pixel32 pixel)
{
  return ((pixel&0xF7DEF7DE)>>1);
}

// --- True ARGB8888 expanders (P11 Stage 2 B) --------------------------------
// The 32-bit screen/world surfaces are ARGB8888 = 0xAARRGGBB (blue in the low
// byte, per the SDL surface masks R=0x00FF0000 G=0x0000FF00 B=0x000000FF).
// These expand a stored 16-bit pixel to that exact layout, fully opaque, using
// bit-replication — the same math SDL's own 565->8888 blit uses — so an opaque
// world pixel written through the expander is byte-identical to the historic
// "compose 565 then let SDL convert" path. (Note: this differs from
// ComponentsToRGB32(), which packs red in the low byte; do NOT use that helper
// for raw surface writes.)
inline Pixel32 pixelutils_565to8888(Pixel16 p)
{
	uint32 const r5 = (p >> 11) & 0x1F;
	uint32 const g6 = (p >> 5)  & 0x3F;
	uint32 const b5 =  p        & 0x1F;
	uint32 const r8 = (r5 << 3) | (r5 >> 2);
	uint32 const g8 = (g6 << 2) | (g6 >> 4);
	uint32 const b8 = (b5 << 3) | (b5 >> 2);
	return 0xFF000000u | (r8 << 16) | (g8 << 8) | b8;
}

inline Pixel32 pixelutils_555to8888(Pixel16 p)
{
	uint32 const r5 = (p >> 10) & 0x1F;
	uint32 const g5 = (p >> 5)  & 0x1F;
	uint32 const b5 =  p        & 0x1F;
	uint32 const r8 = (r5 << 3) | (r5 >> 2);
	uint32 const g8 = (g5 << 3) | (g5 >> 2);
	uint32 const b8 = (b5 << 3) | (b5 >> 2);
	return 0xFF000000u | (r8 << 16) | (g8 << 8) | b8;
}

// Expand using whichever 16-bit layout the display is in (565 vs 555).
inline Pixel32 pixelutils_16to8888(Pixel16 p)
{
	return is_565_Get() ? pixelutils_565to8888(p) : pixelutils_555to8888(p);
}

// Store one 565/555 pixel at a raw byte pointer, expanding to ARGB8888 when the
// destination world surface is 32-bit. Lets a tile writer keep a single loop
// body (bpp32 is loop-invariant → predicts well at -O2); in 16-bit mode it is
// exactly the historic *(Pixel16*)p = v, so conversions are behaviour-
// preserving until the world surface is flipped to 32-bit.
inline void pixelutils_StorePixel(uint8 * p, Pixel16 v, bool bpp32)
{
	if (bpp32) *reinterpret_cast<Pixel32 *>(p) = pixelutils_16to8888(v);
	else       *reinterpret_cast<Pixel16 *>(p) = v;
}

// --- True ARGB8888 per-pixel ops (for writers that read the destination) -----
// Used by tile overlay shadow runs and the legacy sprite fallback (transparency
// / additive / shadow / desaturate). These operate directly on 0xAARRGGBB and
// are NOT byte-identical to "do it in 565 then expand" — that is expected and
// acceptable (the atlas path is the primary sprite route; overlay shadows are a
// minor visual detail). Alpha is preserved / forced opaque as noted per op.

// Halve each RGB channel, keep alpha (mirrors pixelutils_Shadow's darken-to-50%).
inline Pixel32 pixelutils_Shadow8888(Pixel32 p)
{
	return ((p >> 1) & 0x007F7F7Fu) | (p & 0xFF000000u);
}

// Grey out: replace RGB with their average, keep alpha (mirrors Desaturate_565).
inline Pixel32 pixelutils_Desaturate8888(Pixel32 p)
{
	uint32 const r = (p >> 16) & 0xFF;
	uint32 const g = (p >> 8)  & 0xFF;
	uint32 const b =  p        & 0xFF;
	uint32 const ave = (r + g + b) / 3;
	return (p & 0xFF000000u) | (ave << 16) | (ave << 8) | ave;
}

// Blend src toward dst by blend/32 per channel: dst + blend*(src-dst)/32.
// Matches pixelutils_BlendFast's weighting; result is opaque.
inline Pixel32 pixelutils_BlendFast8888(Pixel32 src, Pixel32 dst, sint32 blend)
{
	sint32 const sr = (src >> 16) & 0xFF, sg = (src >> 8) & 0xFF, sb = src & 0xFF;
	sint32 const dr = (dst >> 16) & 0xFF, dg = (dst >> 8) & 0xFF, db = dst & 0xFF;
	uint32 const r = static_cast<uint32>(dr + ((blend * (sr - dr)) >> 5));
	uint32 const g = static_cast<uint32>(dg + ((blend * (sg - dg)) >> 5));
	uint32 const b = static_cast<uint32>(db + ((blend * (sb - db)) >> 5));
	return 0xFF000000u | (r << 16) | (g << 8) | b;
}

// Saturating additive of src onto dst (for the selection flash), opaque.
inline Pixel32 pixelutils_Additive8888(Pixel32 dst, Pixel32 src)
{
	uint32 r = ((dst >> 16) & 0xFF) + ((src >> 16) & 0xFF); if (r > 0xFF) r = 0xFF;
	uint32 g = ((dst >> 8)  & 0xFF) + ((src >> 8)  & 0xFF); if (g > 0xFF) g = 0xFF;
	uint32 b = ( dst        & 0xFF) + ( src        & 0xFF); if (b > 0xFF) b = 0xFF;
	return 0xFF000000u | (r << 16) | (g << 8) | b;
}


Pixel32 pixelutils_Lightening32_565(Pixel16 pixel);
Pixel32 pixelutils_PercentDarken32_565(Pixel32 pixel, sint32 percent);
Pixel32 pixelutils_PercentLighten32_565(Pixel32 pixel, sint32 percent);






inline Pixel16 pixelutils_Blend_555(Pixel16 pixel1, Pixel16 pixel2, short blend)
{
	Pixel16			 r1;
	Pixel16			 g1;
	Pixel16			 b1;
	Pixel16			 r2;
	Pixel16			 g2;
	Pixel16			 b2;
	Pixel16			 r0;
	Pixel16			 g0;
	Pixel16			 b0;

	r1 = ((pixel1 & 0x7C00) >> 9) ;
	g1 = ((pixel1 & 0x03E0) >> 4) ;
	b1 = ((pixel1 & 0x001F) << 1) ;

	r2 = ((pixel2 & 0x7C00) >> 9) ;
	g2 = ((pixel2 & 0x03E0) >> 4) ;
	b2 = ((pixel2 & 0x001F) << 1) ;

	r0 = ((gPixelTable[blend][r1][r2] & 0xFFFE) << 9) ;
	g0 = ((gPixelTable[blend][g1][g2] & 0xFFFE) << 4) ;
	b0 = (gPixelTable[blend][b1][b2] >> 1) ;

	return static_cast<Pixel16>(r0 | g0 | b0);
}

inline Pixel16 pixelutils_Additive_555(Pixel16 pixel1, Pixel16 pixel2)
{
	Pixel16				 r;
	Pixel16				 g;
	Pixel16				 b;
	Pixel16				 sum = (short)(pixel2 & 0x1F) ;

	r = ((pixel1 & 0x7C00) >> 10) + sum;
	g = ((pixel1 & 0x03E0) >> 5)  + sum;
	b = ((pixel1 & 0x001F))       + sum;

	if (r > 0x001F) r = 0x001F;
	if (g > 0x001F) g = 0x001F;
	if (b > 0x001F) b = 0x001F;

	return static_cast<Pixel16>((r<<10) | (g<<5) | b);
}

inline Pixel16 pixelutils_BlendFast_555(sint32 pixel1, sint32 pixel2, sint32 blend)
{
	sint32 rb2;
	sint32 g2;
	sint32 rb0;
	sint32 g0;

	rb2 = (pixel2 & 0x7C1F);
	rb0 = (((rb2<<5)+blend*((pixel1 & 0x7C1F)-rb2))>>5) & 0x7C1F;

	g2 = (pixel2 & 0x03E0);
	g0 = (((g2<<5)+blend*((pixel1 & 0x03E0)-g2))>>5) & 0x07E0;

	return static_cast<Pixel16>(rb0 | g0);
}

inline Pixel16 pixelutils_Shadow_555(Pixel16 pixel)
{
    return static_cast<Pixel16>((pixel&0x7BDF)>>1);
}

inline Pixel16 pixelutils_Lightening_555(Pixel16 pixel)
{
	short		 r;
	short		 g;
	short		 b;
	r = (pixel & 0x7C00) >> 9;
	if (r > 0x001F)
		r = 0x001F;
	g = (pixel & 0x03E0) >> 4;
	if (g > 0x001F)
		g = 0x001F;
	b = (pixel & 0x001F) << 1;
	if (b > 0x001F)
		b = 0x001F;

	return static_cast<Pixel16>((r<<10) | (g<<5) | b);
}

inline Pixel16 pixelutils_PercentDarken_555(Pixel16 pixel, sint32 percent)
{
	sint32 r32;
	sint32 g32;
	sint32 b32;
	sint16 r;
	sint16 g;
	sint16 b;
	sint32 newPercent = k_MAX_PERCENT - percent;

	r32 = (pixel & 0x7C00) >> 10;
	g32 = (pixel & 0x03E0) >> 5;
	b32 = (pixel & 0x001F);

	r32 = (r32 * newPercent) >> k_PERCENT_SHIFT;
	r = (sint16)r32;
	g32 = (g32 * newPercent) >> k_PERCENT_SHIFT;
	g = (sint16)g32;
	b32 = (b32 * newPercent) >> k_PERCENT_SHIFT;
	b = (sint16)b32;

	return static_cast<Pixel16>((r<<10) | (g<<5) | b);
}

inline Pixel16 pixelutils_PercentLighten_555(Pixel16 pixel, sint32 percent)
{
	sint32 r32;
	sint32 g32;
	sint32 b32;
	sint16 r;
	sint16 g;
	sint16 b;
	sint32 newPercent = k_MAX_PERCENT + percent;

	r32 = (pixel & 0x7C00) >> 10;
	g32 = (pixel & 0x03E0) >> 5;
	b32 = (pixel & 0x001F);

	r32 = (r32 * newPercent) >> k_PERCENT_SHIFT;
	if (r32 > 0x001F)
		r32 = 0x001F;
	r = (sint16)r32;
	g32 = (g32 * newPercent) >> k_PERCENT_SHIFT;
	if (g32 > 0x001F)
		g32 = 0x001F;
	g = (sint16)g32;
	b32 = (b32 * newPercent) >> k_PERCENT_SHIFT;
	if (b32 > 0x001F)
		b32 = 0x001F;
	b = (sint16)b32;

	return static_cast<Pixel16>((r<<10) | (g<<5) | b);
}


inline Pixel16 pixelutils_Desaturate_555(Pixel16 pixel)
{
	Pixel16 const ave =
        static_cast<Pixel16>((((pixel & 0x7C00) >> 10) + ((pixel & 0x03E0) >> 5) + (pixel & 0x001F)) / 3);

	return static_cast<Pixel16>(((ave & 0x1F) << 10) | ((ave & 0x1F) << 5) | (ave & 0x1f));
}

Pixel32 pixelutils_Blend32_555(Pixel32 pixel1, Pixel32 pixel2, short blend);
Pixel32 pixelutils_Additive32_555(Pixel32 pixel1, Pixel32 pixel2);
Pixel32 pixelutils_BlendFast32_555(Pixel32 pixel1,Pixel32 pixel2, sint32 blend);

inline Pixel32 pixelutils_Shadow32_555(Pixel32 pixel)
{
    return static_cast<Pixel32>((pixel&0x7BDE7BDE)>>1);
}

Pixel32 pixelutils_Lightening32_555(Pixel32 pixel);
Pixel32 pixelutils_PercentDarken32_555(Pixel32 pixel, sint32 percent);
Pixel32 pixelutils_PercentLighten32_555(Pixel32 pixel, sint32 percent);

Pixel16 pixelutils_Desaturate(Pixel16 pixel);
Pixel16 pixelutils_RGB(int r,int g,int b);
void pixelutils_ComputeRGBTable();

#endif
