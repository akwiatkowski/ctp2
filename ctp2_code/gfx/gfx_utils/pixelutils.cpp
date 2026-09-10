#include "ctp/c3.h"

#include "gfx/gfx_utils/pixelutils.h"
#ifdef WIN32
#include <windows.h>
#else
#include "os/nowin32/windows.h"
#endif

short		gPixelTable[BLEND_LEVELS][BLEND_MAX_VALUE][BLEND_MAX_VALUE];
Pixel16		gRGBTable[RGB_VALUES];

// Owned here.  civ3_main probes the SDL surface PixelFormat() once at
// startup and calls is_565_Set() before any rendering touches this flag.
static sint32 s_is565Format = TRUE;

bool is_565_Get()   { return s_is565Format != 0; }
void is_565_Set(bool v) { s_is565Format = v ? TRUE : FALSE; }

void pixelutils_Initialize()
{
	pixelutils_ComputeBlendTable();
	pixelutils_ComputeRGBTable();
}




std::vector<Pixel16> RGB32ToRGB16(char *buf, uint16 width, uint16 height)
{
	std::vector<Pixel16> outBuf(static_cast<size_t>(width) * height);
	unsigned long	*srcPixel = (unsigned long *)buf;

	for(size_t i = 0; i < outBuf.size(); i++) {
		unsigned long int pix = *srcPixel;
		unsigned short int r;
		unsigned short int g;
		unsigned short int b;

		r = (unsigned short int) ((pix & 0x000000FF) >> 0);
		g = (unsigned short int) ((pix & 0x0000FF00) >> 8);
		b = (unsigned short int) ((pix & 0x00FF0000) >> 16);

		if (is_565_Get())
			outBuf[i] = (Pixel16)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3));
		else
			outBuf[i] = (Pixel16)(((r & 0xF8) << 7) | ((g & 0xF8) << 2) | ((b & 0xF8) >> 3));

		srcPixel++;
	}

	return outBuf;
}

void pixelutils_ComputeBlendTable()
{
	for (short i=0; i<BLEND_LEVELS; i++) {
		for (short c1=0; c1<BLEND_MAX_VALUE; c1++) {
			for (short c2=0; c2<BLEND_MAX_VALUE; c2++) {
				gPixelTable[i][c1][c2] = (short) (( ((long)i * (long)c1) + ((long)(BLEND_LEVELS-i-1) * (long)c2)) / (long)BLEND_LEVELS);
			}
		}
	}
}

void RGB32Components(Pixel32 pixel, Pixel16 *r, Pixel16 *g, Pixel16 *b, Pixel16 *a)
{
	*r = (Pixel16) ((pixel & 0x000000FF) >> 0);
	*g = (Pixel16) ((pixel & 0x0000FF00) >> 8);
	*b = (Pixel16) ((pixel & 0x00FF0000) >> 16);
	*a = (Pixel16) ((pixel & 0xFF000000) >> 24);
}

void RGB32Info(Pixel32 pixel, Pixel16 *outPixel, unsigned char *alpha)
{
	unsigned short int r;
	unsigned short int g;
	unsigned short int b;
	unsigned short int a;

	RGB32Components(pixel, &r, &g, &b, &a);

	*alpha = (unsigned char) a;

	if (is_565_Get())
		*outPixel = (Pixel16) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3));
	else
		*outPixel = (Pixel16) (((r & 0xF8) << 7) | ((g & 0xF8) << 2) | ((b & 0xF8) >> 3));
}

void pixelutils_ComputeRGBTable()
{
	for (int r = 0; r < 32; r++)
    {
		for (int g = 0; g < 32; g++)
        {
			for (int b = 0; b < 32; b++)
            {
                Pixel16 rgb555 = (r << 10) | (g << 5) | b;
				gRGBTable[rgb555] = rgb555;
            }
        }
    }
}

Pixel16 pixelutils_RGB(int r,int g,int b)
{
	if (is_565_Get())
	{
		Pixel16 temp = gRGBTable[(r<<10) | (g<<5) | b];
		short rg = (temp & 0x7FE0) << 1;
		short b = (temp & 0x001F);
		return (rg | b);
	}
	else
		return gRGBTable[(r<<10) | (g<<5) | b];
}

Pixel32 ComponentsToRGB32(Pixel16 r, Pixel16 g, Pixel16 b, Pixel16 a)
{
	Pixel32		pix32;

	pix32 = (r & 0xFF) | ((g & 0xFF) << 8) | ((b & 0xFF) << 16) | ((a & 0xFF) << 24);

	return pix32;
}

Pixel16 pixelutils_Desaturate(Pixel16 pixel)
{
		Pixel16		tempPix;
	sint32		ave;

	if (is_565_Get())
	{
		ave = (((pixel & 0xF800)>>11) + ((pixel & 0x07E0) >> 6) + (pixel & 0x001F)+128)>>2;

		tempPix = (Pixel16)(((ave & 0x1F) << 11) | ((ave & 0x3F) << 6) | (ave & 0x1F));
	} else {
		ave = (((pixel & 0x7C00) >> 10) + ((pixel & 0x03E0) >> 5) + (pixel & 0x001F)+128)>>2;

		tempPix = (Pixel16)(((ave & 0x1F) << 10) | ((ave & 0x1F) << 5) | (ave & 0x1f));
	}

	return tempPix;
}



















