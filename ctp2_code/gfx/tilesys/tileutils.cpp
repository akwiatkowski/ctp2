//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Tile utilities
// Id           : $Id$
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
// WIN32
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include <memory>
#include <vector>

#include "gs/utility/Globals.h"

#include "gfx/gfx_utils/pixelutils.h"
#include "gfx/spritesys/spriteutils.h"
#include "gfx/tilesys/tileutils.h"
#include "gfx/gfx_utils/tiffutils.h"
#include "gfx/gfx_utils/targautils.h"

#include "gfx/tilesys/BaseTile.h"
#include "gfx/tilesys/tileset.h"

#include "gs/fileio/Token.h"
#include "gs/fileio/CivPaths.h"

#include "gfx/gfx_utils/arproces.h"
#include "gs/gameobj/terrainutil.h"
#include "TerrainRecord.h"



BOOL		g_isForSpace;

BOOL		g_useLowPass = FALSE;
uint32		g_lowPassScale = 1;

BOOL		g_adjustBrightness = FALSE;
sint16		g_brightness = 0;

BOOL		g_adjustContrast = FALSE;
uint32		g_threshold = 2;

BOOL		g_adjustSV = FALSE;
double		g_deltaS = 0;
double		g_deltaV = 0;

BOOL		g_blendWithGray = TRUE;
Pixel16		g_grayValueR = 0x0025;
Pixel16		g_grayValueG = 0x0025;
Pixel16		g_grayValueB = 0x0045;

Pixel16	*   g_transitions[TERRAIN_MAX][TERRAIN_MAX][k_TRANSITIONS_PER_TILE];

Pixel32 *tileutils_LowPassFilter(Pixel32 *image, uint32 width, uint32 height)
{
	double LP1[]=
				{ 0.11111111, 0.11111111, 0.11111111,
				  0.11111111, 0.11111111, 0.11111111,
				  0.11111111, 0.11111111, 0.11111111 };

	Pixel32	*filteredImage;

	if (!RealConvolution(image, width, height, 0, 0, LP1, 3, 3, g_lowPassScale, FALSE, &filteredImage)) {
		free(image);
		return filteredImage;
	} else {
		return nullptr;
	}
}

char *tileutils_PostProcessImage(char *image, uint16 width, uint16 height)
{
	if (!g_isForSpace) return image;

	if (g_useLowPass) {
		image = (char *)tileutils_LowPassFilter((Pixel32 *)image, (uint32)width, (uint32)height);
		if (image == nullptr) return nullptr;
	}

	if (g_adjustBrightness) {
		AdjImageBrightness((Pixel32 *)image, g_brightness, (uint32)width, (uint32)height, 0, 0);
	}

	if (g_adjustContrast) {
		StretchImageContrast((Pixel32 *)image, g_threshold, (uint32)width, (uint32)height, 0, 0);
	}

	if (g_adjustSV) {
		AdjustSV((Pixel32 *)image, (uint32)width,  (uint32)height, g_deltaS, g_deltaV);
	}

	if (g_blendWithGray) {
		BlendWithGray((Pixel32 *)image, (uint32)width, (uint32)height, g_grayValueR, g_grayValueG, g_grayValueB);
	}

	return image;
}

char *tileutils_TIF2mem(char *filename, uint16 *width, uint16 *height)
{
	char *image = TIF2mem(filename, width, height);

	return tileutils_PostProcessImage(image, *width, *height);
}

char *tileutils_StripTIF2Mem(char *filename, uint16 *width, uint16 *height)
{
	char *image = StripTIF2Mem(filename, width, height);

	return tileutils_PostProcessImage(image, *width, *height);
}

Pixel16 *tileutils_TGA2mem(char *filename, uint16 *width, uint16 *height)
{

	int		bpp;
	int		 w;
	int		 h;

	if (!Get_TGA_Dimension(filename, w, h, bpp)) {
		Assert(FALSE);
		*width = 0;
		*height = 0;
		return nullptr;
	}

	Pixel16 *   buffer = new Pixel16[w * h]; // TODO(phase-2): ownership transfer out of function
	Load_TGA_File(filename, (uint8 *)buffer, (int)w*sizeof(Pixel16), w, h, nullptr, FALSE);

	*width = (uint16)w;
	*height = (uint16)h;

	return buffer;
}

/**
 * Encode a run of identical 32-bit pixels into a tile copy-run record.
 *
 * @param inBuf            Pointer to the current input pixel (updated as we consume pixels).
 * @param currentPixelPos  Pointer to the current X position within the scanline (0-based, updated).
 * @param scanlineWidth    Total width of the scanline in pixels.
 * @param outBufPtr        Pointer to the output buffer cursor (updated).
 *
 * @note Guard added to prevent reading past the end of the scanline buffer.
 */
void tileutils_EncodeCopyRun(Pixel32 **inBuf,
							 int *currentPixelPos,
							 int scanlineWidth,
							 Pixel16 **outBufPtr)
{
	Pixel16			blendedPixel16;
	unsigned char	pixelAlpha;
	int				runLength      = 0;
	Pixel16	*       headerPtr      = *outBufPtr;
	(*outBufPtr)++;

	RGB32Info(**inBuf, &blendedPixel16, &pixelAlpha);
	Pixel32         currentPixel32 = (**inBuf) & k_32_BIT_RGB_MASK;

	while (currentPixel32 != k_32_BIT_SKIP_PIXEL
				&& currentPixel32 != k_32_BIT_SHADOW_PIXEL
				&& currentPixel32 != k_32_BIT_COLORIZE_PIXEL
				&& (*currentPixelPos < scanlineWidth)) {

		**outBufPtr = blendedPixel16;

		(*outBufPtr)++;

		(*inBuf)++;

		(*currentPixelPos)++;

		runLength++;

		if (*currentPixelPos < scanlineWidth) {
			RGB32Info(**inBuf, &blendedPixel16, &pixelAlpha);
			currentPixel32 = (**inBuf) & k_32_BIT_RGB_MASK;
		}
	}

	Pixel16			footer      = static_cast<Pixel16>(k_TILE_COPY_RUN_ID << 8 | runLength);

	if (*currentPixelPos >= scanlineWidth) footer |= k_TILE_EOLN_ID << 8;

	*headerPtr = footer;
}

/**
 * Encode a run of colorize pixels (32-bit source) into a tile colorize-run record.
 *
 * @param inBuf            Pointer to the current input pixel (updated).
 * @param currentPixelPos  Pointer to the current X position within the scanline (updated).
 * @param scanlineWidth    Total width of the scanline in pixels.
 * @param outBufPtr        Pointer to the output buffer cursor (updated).
 */
void tileutils_EncodeColorizeRun(Pixel32 **inBuf,
								 int *currentPixelPos,
								 int scanlineWidth,
								 Pixel16 **outBufPtr)
{
	Pixel16			blendedPixel16;
	unsigned char	pixelAlpha;
	int				runLength      = 0;
	Pixel16	*       headerPtr      = *outBufPtr;
	(*outBufPtr)++;

	RGB32Info(**inBuf, &blendedPixel16, &pixelAlpha);
	Pixel32			currentPixel32 = (**inBuf) & k_32_BIT_RGB_MASK;
	while (currentPixel32 == k_32_BIT_COLORIZE_PIXEL && (*currentPixelPos < scanlineWidth)) {

		(*inBuf)++;

		(*currentPixelPos)++;

		runLength++;

		if (*currentPixelPos < scanlineWidth) {
			RGB32Info(**inBuf, &blendedPixel16, &pixelAlpha);
			currentPixel32 = (**inBuf) & k_32_BIT_RGB_MASK;
		}
	}

	Pixel16			footer = static_cast<Pixel16>(k_TILE_COLORIZE_RUN_ID << 8 | runLength);

	if (*currentPixelPos >= scanlineWidth) footer |= k_TILE_EOLN_ID << 8;

	*headerPtr = footer;
}

/**
 * Encode a run of shadow pixels (32-bit source) into a tile shadow-run record.
 *
 * @param inBuf            Pointer to the current input pixel (updated).
 * @param currentPixelPos  Pointer to the current X position within the scanline (updated).
 * @param scanlineWidth    Total width of the scanline in pixels.
 * @param outBufPtr        Pointer to the output buffer cursor (updated).
 */
void tileutils_EncodeShadowRun(Pixel32 **inBuf,
							   int *currentPixelPos,
							   int scanlineWidth,
							   Pixel16 **outBufPtr)
{
	Pixel16			blendedPixel16;
	unsigned char	pixelAlpha;
	int				runLength      = 0;
	Pixel16	*       headerPtr      = *outBufPtr;
	(*outBufPtr)++;

	RGB32Info(**inBuf, &blendedPixel16, &pixelAlpha);
	Pixel32			currentPixel32 = (**inBuf) & k_32_BIT_RGB_MASK;
	while (currentPixel32 == k_32_BIT_SHADOW_PIXEL && (*currentPixelPos < scanlineWidth)) {

		(*inBuf)++;

		(*currentPixelPos)++;

		runLength++;

		if (*currentPixelPos < scanlineWidth) {
			RGB32Info(**inBuf, &blendedPixel16, &pixelAlpha);
			currentPixel32 = (**inBuf) & k_32_BIT_RGB_MASK;
		}
	}

	Pixel16			footer = static_cast<Pixel16>(k_TILE_SHADOW_RUN_ID << 8 | runLength);

	if (*currentPixelPos >= scanlineWidth) footer |= k_TILE_EOLN_ID << 8;

	*headerPtr = footer;
}

/**
 * Encode a run of skip pixels (32-bit source) into a tile skip-run record.
 *
 * @param inBuf            Pointer to the current input pixel (updated).
 * @param currentPixelPos  Pointer to the current X position within the scanline (updated).
 * @param scanlineWidth    Total width of the scanline in pixels.
 * @param outBufPtr        Pointer to the output buffer cursor (updated).
 *
 * @return 0 if the skip run consumed the entire scanline, 1 otherwise.
 */
char tileutils_EncodeSkipRun(Pixel32 **inBuf,
							 int *currentPixelPos,
							 int scanlineWidth,
							 Pixel16 **outBufPtr)
{
	Pixel16			blendedPixel16;
	unsigned char	pixelAlpha;
	int				runLength = 0;

	RGB32Info(**inBuf, &blendedPixel16, &pixelAlpha);
	Pixel32			currentPixel32 = (**inBuf) & k_32_BIT_RGB_MASK;

	while (currentPixel32 == k_32_BIT_SKIP_PIXEL && (*currentPixelPos < scanlineWidth)) {

		(*inBuf)++;

		(*currentPixelPos)++;

		runLength++;

		if (*currentPixelPos < scanlineWidth) {
			RGB32Info(**inBuf, &blendedPixel16, &pixelAlpha);
			currentPixel32 = (**inBuf) & k_32_BIT_RGB_MASK;
		}
	}

	if (runLength < scanlineWidth) {

		Pixel16			footer = static_cast<Pixel16>(k_TILE_SKIP_RUN_ID << 8 | runLength);

		if (*currentPixelPos >= scanlineWidth) footer |= k_EOLN_ID << 8;

		**outBufPtr = footer;
		(*outBufPtr)++;
	    return FALSE;
	} else {

		return TRUE;
	}

}

char tileutils_EncodeScanline(Pixel32 *scanline, int width, Pixel16 **outBufPtr)
{
	Pixel16			pix16;
	Pixel32			pix32;
	Pixel32	*       scanPtr    = scanline;
	unsigned char	alpha;
	int				pos         = 0;
	char			empty = FALSE;

	while (scanPtr < (scanline + width)) {
		pix32 = *scanPtr;

		RGB32Info(pix32, &pix16, &alpha);
		pix32 = pix32 & 0x00FFFFFF;

		switch (pix32) {
		case k_32_BIT_SKIP_PIXEL :
			empty = tileutils_EncodeSkipRun(&scanPtr, &pos, width, outBufPtr);
			break;
		case k_32_BIT_COLORIZE_PIXEL :
			tileutils_EncodeColorizeRun(&scanPtr, &pos, width, outBufPtr);
			break;
		case k_32_BIT_SHADOW_PIXEL :
			tileutils_EncodeShadowRun(&scanPtr, &pos, width, outBufPtr);
			break;
		default:
			tileutils_EncodeCopyRun(&scanPtr, &pos, width, outBufPtr);
		}
	}

	return empty;
}

Pixel16 *tileutils_EncodeTile(Pixel32 *buf, uint16 width, uint16 height, uint32 *size)
{
	Pixel32	*               srcPixel            = buf;
	std::vector<Pixel16>    outBuf_vec          (width * height * 2);
	Pixel16	*               outBuf              = outBuf_vec.data();
	std::vector<Pixel16>    table_vec           (height + 2);
	Pixel16	*               table               = table_vec.data();
	Pixel16 *               startDataPtr;
	char				    empty               = FALSE;
	int					    lastNonEmpty        = -1;

    Pixel16 *               tableStart          = table;
	unsigned short int *    firstNonEmptyPtr    = table;
	table++;
	unsigned short int *    endLinePtr          = table;
	table++;

	Pixel16	*               startOfData         = outBuf;
	Pixel16 *               dataPtr             = startOfData;
	int                     firstNonEmpty       = -1;

	for (uint16 y = 0; y < height; y++)
    {

		srcPixel = buf + width * y;

		startDataPtr = dataPtr;

		empty = tileutils_EncodeScanline(srcPixel, width, &dataPtr);
		if (!empty) {
			if (firstNonEmpty == -1) {

				*firstNonEmptyPtr = y;
				firstNonEmpty = 0;
			}

			*table++ = startDataPtr - startOfData;
			lastNonEmpty = y;
		} else {


			if (firstNonEmpty != -1)
				*table++ = (Pixel16) -1;

		}
	}

	if (lastNonEmpty != -1) {
		*endLinePtr = lastNonEmpty;
	} else {
		Assert(lastNonEmpty != -1);
	}

	int tableSize = ((*endLinePtr-*firstNonEmptyPtr + 1 + 2)) * 2;

	int dataSize = (dataPtr - outBuf) * 2;

	char *returnBuf = new char[dataSize+tableSize]; // TODO(phase-2): ownership transfer out of function

	memcpy(returnBuf, tableStart, tableSize);
	memcpy(returnBuf+tableSize, outBuf, dataSize);

	*size = (dataSize + tableSize);

	return (Pixel16 *)returnBuf;
}

/**
 * Encode a run of identical 16-bit pixels into a tile copy-run record.
 *
 * @param inBuf            Pointer to the current input pixel (updated as we consume pixels).
 * @param currentPixelPos  Pointer to the current X position within the scanline (0-based, updated).
 * @param scanlineWidth    Total width of the scanline in pixels.
 * @param outBufPtr        Pointer to the output buffer cursor (updated).
 * @param sourceDataIs565  If true, convert 565-format pixels to 555 format on the fly.
 *
 * @note The original code read the next input pixel AFTER incrementing the cursor,
 *       which caused a heap-buffer-overflow when the last pixel of a row was consumed.
 *       The guard `if (*currentPixelPos < scanlineWidth)` prevents the read-past-end.
 */
void tileutils_EncodeCopyRun16(Pixel16 **inBuf,
							   int *currentPixelPos,
							   int scanlineWidth,
							   Pixel16 **outBufPtr,
							   BOOL sourceDataIs565)
{
	int			runLength      = 0;
	Pixel16	*	headerPtr      = *outBufPtr;
	(*outBufPtr)++;

	Pixel16		currentPixel   = (**inBuf);
	if (sourceDataIs565)
		currentPixel = ((currentPixel & 0xFFC0) >> 1) | (currentPixel & 0x001F);

	while (currentPixel != k_16_BIT_SKIP_PIXEL
				&& currentPixel != k_16_BIT_SHADOW_PIXEL
				&& currentPixel != k_16_BIT_COLORIZE_PIXEL
				&& (*currentPixelPos < scanlineWidth)) {

		**outBufPtr = currentPixel;

		(*outBufPtr)++;

		(*inBuf)++;

		(*currentPixelPos)++;

		runLength++;

		/* Only read the next pixel if we have not reached the end of the row.
		   Otherwise we would read past the end of the scanline buffer. */
		if (*currentPixelPos < scanlineWidth) {
			currentPixel = (**inBuf);
			if (sourceDataIs565)
				currentPixel = ((currentPixel & 0xFFC0) >> 1) | (currentPixel & 0x001F);
		}
	}

	Pixel16			footer = static_cast<Pixel16>(k_TILE_COPY_RUN_ID << 8 | runLength);

	if (*currentPixelPos >= scanlineWidth) footer |= k_TILE_EOLN_ID << 8;

	*headerPtr = footer;
}

/**
 * Encode a run of colorize pixels (16-bit source) into a tile colorize-run record.
 *
 * @param inBuf            Pointer to the current input pixel (updated).
 * @param currentPixelPos  Pointer to the current X position within the scanline (updated).
 * @param scanlineWidth    Total width of the scanline in pixels.
 * @param outBufPtr        Pointer to the output buffer cursor (updated).
 * @param sourceDataIs565  If true, convert 565-format pixels to 555 format on the fly.
 */
void tileutils_EncodeColorizeRun16(Pixel16 **inBuf,
								   int *currentPixelPos,
								   int scanlineWidth,
								   Pixel16 **outBufPtr,
								   BOOL sourceDataIs565)
{
	int			runLength      = 0;
	Pixel16 *   headerPtr      = *outBufPtr;
	(*outBufPtr)++;

	Pixel16		currentPixel   = (**inBuf);
	if (sourceDataIs565)
		currentPixel = ((currentPixel & 0xFFC0) >> 1) | (currentPixel & 0x001F);

	while (currentPixel == k_16_BIT_COLORIZE_PIXEL && (*currentPixelPos < scanlineWidth)) {

		(*inBuf)++;

		(*currentPixelPos)++;

		runLength++;

		if (*currentPixelPos < scanlineWidth) {
			currentPixel = (**inBuf);
			if (sourceDataIs565)
				currentPixel = ((currentPixel & 0xFFC0) >> 1) | (currentPixel & 0x001F);
		}
	}

	Pixel16			footer = static_cast<Pixel16>(k_TILE_COLORIZE_RUN_ID << 8 | runLength);

	if (*currentPixelPos >= scanlineWidth) footer |= k_TILE_EOLN_ID << 8;

	*headerPtr = footer;
}

/**
 * Encode a run of shadow pixels (16-bit source) into a tile shadow-run record.
 *
 * @param inBuf            Pointer to the current input pixel (updated).
 * @param currentPixelPos  Pointer to the current X position within the scanline (updated).
 * @param scanlineWidth    Total width of the scanline in pixels.
 * @param outBufPtr        Pointer to the output buffer cursor (updated).
 * @param sourceDataIs565  If true, convert 565-format pixels to 555 format on the fly.
 */
void tileutils_EncodeShadowRun16(Pixel16 **inBuf,
								 int *currentPixelPos,
								 int scanlineWidth,
								 Pixel16 **outBufPtr,
								 BOOL sourceDataIs565)
{
	int			runLength      = 0;
	Pixel16 *   headerPtr      = *outBufPtr;
	(*outBufPtr)++;

	Pixel16		currentPixel   = (**inBuf);
	if (sourceDataIs565)
		currentPixel = ((currentPixel & 0xFFC0) >> 1) | (currentPixel & 0x001F);

	while (currentPixel == k_16_BIT_SHADOW_PIXEL && (*currentPixelPos < scanlineWidth)) {

		(*inBuf)++;

		(*currentPixelPos)++;

		runLength++;

		if (*currentPixelPos < scanlineWidth) {
			currentPixel = (**inBuf);
			if (sourceDataIs565)
				currentPixel = ((currentPixel & 0xFFC0) >> 1) | (currentPixel & 0x001F);
		}
	}

	Pixel16			footer = static_cast<Pixel16>(k_TILE_SHADOW_RUN_ID << 8 | runLength);

	if (*currentPixelPos >= scanlineWidth) footer |= k_TILE_EOLN_ID << 8;

	*headerPtr = footer;
}

/**
 * Encode a run of skip pixels (16-bit source) into a tile skip-run record.
 *
 * @param inBuf            Pointer to the current input pixel (updated).
 * @param currentPixelPos  Pointer to the current X position within the scanline (updated).
 * @param scanlineWidth    Total width of the scanline in pixels.
 * @param outBufPtr        Pointer to the output buffer cursor (updated).
 * @param sourceDataIs565  If true, convert 565-format pixels to 555 format on the fly.
 *
 * @return 0 if the skip run consumed the entire scanline, 1 otherwise.
 */
char tileutils_EncodeSkipRun16(Pixel16 **inBuf,
							   int *currentPixelPos,
							   int scanlineWidth,
							   Pixel16 **outBufPtr,
							   BOOL sourceDataIs565)
{
	int			runLength = 0;

	Pixel16		currentPixel   = (**inBuf);
	if (sourceDataIs565)
		currentPixel = ((currentPixel & 0xFFC0) >> 1) | (currentPixel & 0x001F);

	while (currentPixel == k_16_BIT_SKIP_PIXEL && (*currentPixelPos < scanlineWidth)) {

		(*inBuf)++;

		(*currentPixelPos)++;

		runLength++;

		if (*currentPixelPos < scanlineWidth) {
			currentPixel = (**inBuf);
			if (sourceDataIs565)
				currentPixel = ((currentPixel & 0xFFC0) >> 1) | (currentPixel & 0x001F);
		}
	}

	if (runLength < scanlineWidth) {

		Pixel16			footer = static_cast<Pixel16>(k_TILE_SKIP_RUN_ID << 8 | runLength);

		if (*currentPixelPos >= scanlineWidth) footer |= k_EOLN_ID << 8;

		**outBufPtr = footer;
		(*outBufPtr)++;
		return FALSE;
	} else {

		return TRUE;
	}
}

char tileutils_EncodeScanline16(Pixel16 *scanline, int width, Pixel16 **outBufPtr,
								BOOL sourceDataIs565)
{
	Pixel16		pix16;
	Pixel16	*   scanPtr     = scanline;
	int			pos         = 0;
	char		empty       = FALSE;

	while (scanPtr < (scanline + width)) {
		pix16 = *scanPtr;
		if (sourceDataIs565)
			pix16 = ((pix16 & 0xFFC0) >> 1) | (pix16 & 0x001F);
		switch (pix16) {
		case k_16_BIT_SKIP_PIXEL :
			empty = tileutils_EncodeSkipRun16(&scanPtr, &pos, width, outBufPtr, sourceDataIs565);
			break;
		case k_16_BIT_COLORIZE_PIXEL :
			tileutils_EncodeColorizeRun16(&scanPtr, &pos, width, outBufPtr, sourceDataIs565);
			break;
		case k_16_BIT_SHADOW_PIXEL :
			tileutils_EncodeShadowRun16(&scanPtr, &pos, width, outBufPtr, sourceDataIs565);
			break;
		default:
			tileutils_EncodeCopyRun16(&scanPtr, &pos, width, outBufPtr, sourceDataIs565);
		}
	}

	return empty;
}

Pixel16 *tileutils_EncodeTile16(Pixel16 *buf, uint16 width, uint16 height, uint32 *size,
								sint32 pitch)
{
	Pixel16				*srcPixel = buf;
	std::vector<Pixel16>    outBuf_vec(width * height * 2);
	Pixel16				*outBuf = outBuf_vec.data();
	std::vector<Pixel16>    table_vec(height + 2);
	Pixel16				*table = table_vec.data();
	Pixel16				*startDataPtr;
	char				empty = FALSE;
	int					lastNonEmpty = -1;

	BOOL				sourceDataIs565 = FALSE;

	if (pitch == 0) {
		pitch = width*2;
	} else {

		if (is_565_Get())
			sourceDataIs565 = TRUE;
	}

	Pixel16 *               tableStart          = table;
	unsigned short int *    firstNonEmptyPtr    = table;
	table++;
	unsigned short int *    endLinePtr          = table;
	table++;

	Pixel16	*               startOfData         = outBuf;
	Pixel16 *               dataPtr             = outBuf;
	int                     firstNonEmpty       = -1;

	for(int y=0; y<height; y++) {

		srcPixel = buf + (pitch/2) * y;

		startDataPtr = dataPtr;

		empty = tileutils_EncodeScanline16(srcPixel, width, &dataPtr, sourceDataIs565);
	    if (!empty) {
			if (firstNonEmpty == -1) {

				*firstNonEmptyPtr = y;
				firstNonEmpty = 0;
			}

			*table++ = startDataPtr - startOfData;
			lastNonEmpty = y;
		} else {


			if (firstNonEmpty != -1)
				*table++ = (Pixel16) -1;

		}
	}

	if (lastNonEmpty != -1) {
		*endLinePtr = lastNonEmpty;
	} else {
		Assert(lastNonEmpty != -1);
	}

	int tableSize = ((*endLinePtr-*firstNonEmptyPtr + 1 + 2)) * 2;

	int dataSize = (dataPtr - outBuf) * 2;

	char *returnBuf = new char[dataSize+tableSize]; // TODO(phase-2): ownership transfer out of function

	memcpy(returnBuf, tableStart, tableSize);
	memcpy(returnBuf+tableSize, outBuf, dataSize);

	*size = (dataSize + tableSize);


	return (Pixel16 *)returnBuf;
}

sint32 tileutils_ConvertPixelFormatFrom565(Pixel16 *data)
{
	uint16		start = (uint16)*data++;
	uint16		end = (uint16)*data++;
	Pixel16		*table = data;
	Pixel16		*dataStart = table + (end - start + 1);

	for(sint32 j=start; j<end; j++) {

		if ((sint16)table[j-start] == -1)
			continue;

		Pixel16	*   rowData = dataStart + table[j-start];
		Pixel16		tag;

		do {
			tag = *rowData++;

			switch ((tag & 0x0F00) >> 8) {
				case k_TILE_SKIP_RUN_ID	:
					break;
				case k_TILE_COPY_RUN_ID			: {
						short len = (tag & 0x00FF);

						for (short i=0; i<len; i++) {
							*rowData = pixelutils_Convert565to555(*rowData);
							rowData++;
						}

					}
					break;
				case k_TILE_SHADOW_RUN_ID:
					break;
				case k_TILE_COLORIZE_RUN_ID:
					break;
			}

		} while ((tag & 0xF000) == 0);
	}

	return 0;
}

sint32 tileutils_ConvertPixelFormatFrom555(Pixel16 *data)
{
	uint16		start = (uint16)*data++;
	uint16		end = (uint16)*data++;
	Pixel16		*table = data;
	Pixel16		*dataStart = table + (end - start + 1);

	for(sint32 j=start; j<end; j++) {

		if ((sint16)table[j-start] == -1)
			continue;

		Pixel16	*   rowData = dataStart + table[j-start];
		Pixel16		tag;

		do {
			tag = *rowData++;

			switch ((tag & 0x0F00) >> 8) {
				case k_TILE_SKIP_RUN_ID	:
					break;
				case k_TILE_COPY_RUN_ID			: {
						short len = (tag & 0x00FF);

						for (short i=0; i<len; i++) {
							*rowData = pixelutils_Convert555to565(*rowData);
							rowData++;
						}

					}
					break;
				case k_TILE_SHADOW_RUN_ID:
					break;
				case k_TILE_COLORIZE_RUN_ID:
					break;
			}

		} while ((tag & 0xF000) == 0);
	}

	return 0;
}

void tileutils_DecodeToBuffer(Pixel16 *data, int width, int height)
{
	std::vector<Pixel16>    outBuf_vec(width * height);
	Pixel16		*outBuf = outBuf_vec.data();
	Pixel16		*destPixel = outBuf;

	Pixel16		*table = data + 1;
	Pixel16		start = *data++;
	Pixel16		*dataStart = table + (72 - start);

	for(int j=start; j<72; j++) {
		if (table[j-start] != k_EMPTY_TABLE_ENTRY) {
			Pixel16 *   rowData = dataStart + table[j-start];
			Pixel16		tag     = *rowData++ & 0x0FFF;

			while ((tag & 0xF000) == 0) {
				switch ((tag & 0x0F00) >> 8) {
					case k_TILE_SKIP_RUN_ID	:
							destPixel += (tag & 0x00FF);
						break;
					case k_TILE_COPY_RUN_ID			: {
							short len = (tag & 0x00FF);

							for (short i=0; i<len; i++) {
								*destPixel++ = *rowData++;
							}

						}
						break;
				}
				tag = *rowData++;
			}
		}
	}
}

Pixel16 g_offsets[k_TILE_PIXEL_HEIGHT*32*sizeof(Pixel16)];
uint32 g_bitsTable[k_TILE_PIXEL_HEIGHT];
uint16 g_stencilSize = 0;

sint32 tileutils_EncodeStencil(MBCHAR *filename)
{
	uint16	 width=0;
	uint16	 height=0;
	TifBuffer tifBuf(tileutils_TIF2mem(filename, &width, &height));
	char	*tif    = tifBuf.get();   // was leaked before
	Assert(tif != nullptr);
	if (tif == nullptr) exit(-1);

	if (height != k_TILE_PIXEL_HEIGHT) {
		printf("\nImage height is %d and should be %d.\n", height, k_TILE_PIXEL_HEIGHT);
		exit(-1);
	}
	if (width != k_TILE_PIXEL_WIDTH) {
		printf("\nImage width is %d and should be %d.\n", width, k_TILE_PIXEL_WIDTH);
		exit(-1);
	}

	Pixel32 *   ptr     = (Pixel32 *)tif;
	sint32      top     = 9999999;
	sint32      bottom  = -1;
	sint32      left    = 9999999;
	sint32      right   = -1;

	sint32	 i;
	sint32	 j;

	for (i=0; i<height; i++) {
		for (j=0; j<width; j++) {
			if ((ptr[i*width+j] & 0xFFFFFF) != 0) {
				if (j < left)
					left = j;
				if (i < top)
					top = i;
				if (i > bottom)
					bottom = i;
				if (j > right)
					right = j;
			}
		}
	}

//	RECT stencilRect = {left, top, right, bottom};

	printf("Stencil: left %d top %d right %d bottom %d\n", left, top, right, bottom);

	uint32 accum = 0;
//	sint32 counter = 0;

    std::fill(g_bitsTable, g_bitsTable + k_TILE_PIXEL_HEIGHT, 0);
    std::fill(g_offsets, g_offsets + k_TILE_PIXEL_HEIGHT * 32, (Pixel16) 0xFFFF);
	uint32	bit;

	uint16	index=0;

	for (i=top; i<=bottom; i++) {
		if (i<=23) left = (23-i)*2;
		else left = (i-24)*2;

		uint16 rowIndex = 0;
		for (j=left; j<left+32; j++) {
			if ((ptr[i*width+j] & 0xFFFFFF) != 0) {
				bit = 0x80000000;
				g_offsets[i*32 + rowIndex] = index;
				index++;
				rowIndex++;
			} else {
				bit = 0;
			}
			accum >>= 1;
			accum = (accum | bit);
		}
		g_bitsTable[i] = accum;
		accum = 0;
	}




	g_stencilSize = index;

	uint16 tableHeight = k_TILE_PIXEL_HEIGHT;

	FILE *  file = fopen("source" FILE_SEP "stencil" FILE_SEP "stencil.bin", "wb");
	if (file != nullptr) {
		fwrite(&tableHeight, 1, sizeof(uint16), file);
		fwrite(g_bitsTable, 1, sizeof(sint32) * k_TILE_PIXEL_HEIGHT, file);
		fwrite(&index, 1, sizeof(index), file);
		fwrite(g_offsets, 1, 32 * k_TILE_PIXEL_HEIGHT * sizeof(Pixel16), file);
		fclose(file);
	}

	return 0;
}

void tileutils_DumpStencil(MBCHAR *filename)
{
	uint32		accum;
	sint32		 i;
	sint32		 j;
	sint32		nudge;
	FILE *  file = fopen(filename, "w");

	for (i=0; i<48; i++) {
		accum = g_bitsTable[i];
		if (i<=23) nudge = (23-i)*2;
		else nudge = (i-24)*2;

		for (j=0; j<nudge; j++)
			fprintf(file," ");

		for (j=0; j<32; j++) {
			if (accum & 1) fprintf(file,"1");
			else fprintf(file,"0");
			accum>>=1;
		}
		fprintf(file,"\n");
	}

	for (i=0; i<k_TILE_PIXEL_HEIGHT; i++) {
		for (j=0; j<32; j++) {
			fprintf(file, "\t%d", g_offsets[i*32+j]);
		}
		fprintf(file, "\n");
	}

	fclose(file);
}

void tileutils_LoadStencil()
{
	FILE *  file    = fopen("source" FILE_SEP "stencil" FILE_SEP "stencil.bin", "rb");

	if (file != nullptr) {
	    uint16		tableHeight = 0;
		fread(&tableHeight, 1, sizeof(uint16), file);

		fread(g_bitsTable, 1, sizeof(sint32) * k_TILE_PIXEL_HEIGHT, file);

	    uint16		stencilLen = 0;
		fread(&stencilLen, 1, sizeof(stencilLen), file);
		g_stencilSize = stencilLen;

		fread(g_offsets, 1, 32 * k_TILE_PIXEL_HEIGHT * sizeof(Pixel16), file);

		fclose(file);
	}

}

Pixel16 *tileutils_CreateBorkBork()
{
	Pixel16		*bork = new Pixel16[48*94]; // TODO(phase-2): ownership transfer out of function
	sint32		 i;
	sint32		 j;
	sint32		pixelX;
	uint32		accum;

	memset(bork, 0xFF, sizeof(Pixel16)*48*94);

	for (i=0; i<k_TILE_PIXEL_HEIGHT; i++) {
		if (i<=23) {
			pixelX = (23-i)*2;
		} else {
			pixelX = (i-24)*2;
		}

		accum = g_bitsTable[i];
		for (j=0; j<32; j++) {
			if (accum & 0x00000001) {

				bork[i * k_TILE_PIXEL_WIDTH + pixelX] = 0;

				bork[(k_TILE_PIXEL_HEIGHT - 1 - i) * k_TILE_PIXEL_WIDTH + pixelX] = 1;

				bork[(k_TILE_PIXEL_HEIGHT - 1 - i) * k_TILE_PIXEL_WIDTH + (k_TILE_PIXEL_WIDTH - 1 - pixelX)] = 2;

				bork[i * k_TILE_PIXEL_WIDTH + (k_TILE_PIXEL_WIDTH - 1 - pixelX)] = 3;
			}
			accum >>= 1;
			pixelX++;
		}
	}
















	return bork;
}

Pixel16 *tileutils_ExtractUpperRight(char *tif, uint16 width, uint16 height, sint32 x, sint32 y)
{
	sint32		 i;
	sint32		 j;
	sint32		 pixelX;
	sint32		 pixelY;
	uint32		accum;
	uint32		*image = (uint32 *)tif;

	Pixel16		*data = new Pixel16[g_stencilSize]; // TODO(phase-2): ownership transfer out of function
	Pixel16		*dataPtr = data;
	Pixel32		pix32;
	Pixel16		pix16;
	uint8		alpha;
	sint32		row;

	for (i=0; i<k_TILE_PIXEL_HEIGHT; i++) {
		row = y+(k_TILE_PIXEL_HEIGHT-1)-i;
		if (i<=23) {
			pixelX = (x + k_TILE_PIXEL_WIDTH) - (23-i)*2-1;
			pixelY = row;
		} else {
			pixelX = (x + k_TILE_PIXEL_WIDTH) - (i-24)*2-1;
			pixelY = row;
		}

		accum = g_bitsTable[i];
		for (j=0; j<32; j++) {
			if (accum & 0x00000001) {
				pix32 = image[ row * width + pixelX];

				RGB32Info(pix32, &pix16, &alpha);

				*dataPtr = pix16;
				dataPtr++;
			}
			accum >>= 1;
			pixelX--;
		}
	}

	return data;
}

Pixel16 *tileutils_ExtractLowerLeft(char *tif, uint16 width, uint16 height, sint32 x, sint32 y)
{
	sint32		 i;
	sint32		 j;
	sint32		 pixelX;
	sint32		 pixelY;
	uint32		accum;
	uint32		*image = (uint32 *)tif;

	Pixel16		*data = new Pixel16[g_stencilSize]; // TODO(phase-2): ownership transfer out of function
	Pixel16		*dataPtr = data;
	Pixel32		pix32;
	Pixel16		pix16;
	uint8		alpha;
	sint32		row;

	for (i=0; i<k_TILE_PIXEL_HEIGHT; i++) {
		row = (y + i);
		if (i<=23) {
			pixelX = (x + (23-i)*2);
			pixelY = row;
		} else {
			pixelX = (x + (i-24)*2);
			pixelY = row;
		}

		accum = g_bitsTable[i];
		for (j=0; j<32; j++) {
			if (accum & 0x00000001) {
				pix32 = image[ row * width + pixelX];

				RGB32Info(pix32, &pix16, &alpha);

				*dataPtr = pix16;
				dataPtr++;
			}
			accum >>= 1;
			pixelX++;
		}
	}

	return data;
}

Pixel16 *tileutils_LoadStencilImage(uint16 from, uint16 to)
{
	Pixel16		*data = new Pixel16[g_stencilSize]; // TODO(phase-2): ownership transfer out of function
	MBCHAR		fname[_MAX_PATH];
	snprintf(fname, sizeof(fname), "output" FILE_SEP "xitions" FILE_SEP "gtft%.2d%.2d.bin", from, to);

	FILE *      file = fopen(fname, "rb");
	if (file != nullptr) {
		fread(data, 1, g_stencilSize * sizeof(Pixel16), file);
		fclose(file);
	}

	return data;
}

Pixel16 *tileutils_MakeTransition1(Pixel16 *sourceStencil)
{
	sint32		 i;
	sint32		 j;
	Pixel16		pix;
	uint32		accum;
	uint16		off;
	sint32		rowIndex;

	Pixel16 *   data    = new Pixel16[g_stencilSize]; // TODO(phase-2): ownership transfer out of function
	Pixel16 *   dataPtr = data;
	sint32      bottom  = k_TILE_PIXEL_HEIGHT-1;

	for (i=0; i<k_TILE_PIXEL_HEIGHT; i++) {
		accum = g_bitsTable[bottom-i];
		rowIndex = 0;
		for(j=0; j<32; j++) {
			if (accum & 0x00000001) {
				off = g_offsets[(bottom-i)*32 + rowIndex];

				pix = sourceStencil[off];

				*dataPtr = pix;
				dataPtr++;
				rowIndex++;
			}
			accum >>= 1;
		}
	}

	return data;
}

Pixel16 *tileutils_MakeTransition2(Pixel16 *sourceStencil)
{
	sint32		 i;
	sint32		 j;
	sint32		 k;
	Pixel16		pix;
	uint32		accum;
	uint16		off;
	sint32		rowIndex;

	Pixel16	*   data    = new Pixel16[g_stencilSize]; // TODO(phase-2): ownership transfer out of function
	Pixel16 *   dataPtr = data;
	sint32      bottom  = k_TILE_PIXEL_HEIGHT-1;

	for (i=0; i<k_TILE_PIXEL_HEIGHT; i++) {
		accum = g_bitsTable[bottom-i];

		for(k=0; k<32; k++) {
			if (g_offsets[(bottom-i)*32+k] == 0xFFFF) {
				k--;
				break;
			}
		}
		rowIndex = k;

		for(j=0; j<32; j++) {
			if (accum & 0x00000001) {
				off = g_offsets[(bottom-i)*32 + rowIndex];

				pix = sourceStencil[off];

				*dataPtr = pix;
				dataPtr++;
				rowIndex--;
			}
			accum >>= 1;
		}
	}

	return data;
}

Pixel16 *tileutils_MakeTransition3(Pixel16 *sourceStencil)
{
	sint32		 i;
	sint32		 j;
	sint32		 k;
	Pixel16		pix;
	uint32		accum;
	uint16		off;
	sint32		rowIndex;

	Pixel16 *   data    = new Pixel16[g_stencilSize]; // TODO(phase-2): ownership transfer out of function
	Pixel16 *   dataPtr = data;

	for (i=0; i<k_TILE_PIXEL_HEIGHT; i++) {
		accum = g_bitsTable[i];

		for(k=0; k<32; k++) {
			if (g_offsets[i*32+k] == 0xFFFF) {
				k--;
				break;
			}
		}
		rowIndex = k;

		for(j=0; j<32; j++) {
			if (accum & 0x00000001) {
				off = g_offsets[i*32 + rowIndex];

				pix = sourceStencil[off];

				*dataPtr = pix;
				dataPtr++;
				rowIndex--;
			}
			accum >>= 1;
		}
	}

	return data;
}

void tileutils_DumpAllTransitions(MBCHAR *filename, Pixel16 *t0, Pixel16 *t1, Pixel16 *t2, Pixel16 *t3)
{
	uint32		accum;
	sint32		 i;
	sint32		 j;
	sint32		index;
	FILE *      file = fopen(filename, "w");

	fprintf(file, "\n Transition 0\n\n");
	for (i=0; i<48; i++) {
		accum = g_bitsTable[i];

		index = 0;
		for (j=0; j<32; j++) {
			if (accum & 1) {
				fprintf(file, "\t%#.4x", t0[g_offsets[i*32+index]]);
				index++;
			}
			accum>>=1;
		}
		fprintf(file,"\n");
	}

	fprintf(file, "\n Transition 1\n\n");
	for (i=0; i<48; i++) {
		accum = g_bitsTable[47-i];

		index = 0;
		for (j=0; j<32; j++) {
			if (accum & 1) {
				fprintf(file, "\t%#.4x", t1[g_offsets[(47-i)*32+index]]);
				index++;
			}
			accum>>=1;
		}
		fprintf(file,"\n");
	}

	fprintf(file, "\n Transition 2\n\n");
	for (i=0; i<48; i++) {
		accum = g_bitsTable[47-i];

		index = 0;
		for (j=0; j<32; j++) {
			if (accum & 1) {
				fprintf(file, "\t%#.4x", t2[g_offsets[(47-i)*32+index]]);
				index++;
			}
			accum>>=1;
		}
		fprintf(file,"\n");
	}

	fprintf(file, "\n Transition 3\n\n");
	for (i=0; i<48; i++) {
		accum = g_bitsTable[i];

		index = 0;
		for (j=0; j<32; j++) {
			if (accum & 1) {
				fprintf(file, "\t%#.4x", t3[g_offsets[i*32+index]]);
				index++;
			}
			accum>>=1;
		}
		fprintf(file,"\n");
	}

	fclose(file);
}

sint32 tileutils_ExtractStencils(sint16 fromType, sint16 toType)
{

	MBCHAR		ageChar;
	MBCHAR		filename[_MAX_PATH];

	snprintf(filename, sizeof(filename), "gtft%.2d%.2d.tif", fromType, toType);
	ageChar = 'f';

	char	*tif;
	uint16	 width=0;
	uint16	 height=0;

	MBCHAR		fname[_MAX_PATH];
	snprintf(fname, sizeof(fname), "source" FILE_SEP "xitions" FILE_SEP "%s", filename);

	TifBuffer tifBuf(tileutils_TIF2mem(fname, &width, &height));
	tif = tifBuf.get();
	Assert(tif != nullptr);
	if (tif == nullptr) {
		printf("\n*** Could not find '%s'.\n", fname);
		exit(-1);
	}

	g_transitions[fromType][toType][0] = tileutils_ExtractUpperRight(tif, width, height, 0, 48);
	g_transitions[fromType][toType][1] = tileutils_MakeTransition1(g_transitions[fromType][toType][0]);
	g_transitions[fromType][toType][2] = tileutils_MakeTransition2(g_transitions[fromType][toType][0]);
	g_transitions[fromType][toType][3] = tileutils_MakeTransition3(g_transitions[fromType][toType][0]);


















	g_transitions[toType][fromType][0] = tileutils_ExtractLowerLeft(tif, width, height, 48, 24);
	g_transitions[toType][fromType][1] = tileutils_MakeTransition1(g_transitions[toType][fromType][0]);
	g_transitions[toType][fromType][2] = tileutils_MakeTransition2(g_transitions[toType][fromType][0]);
	g_transitions[toType][fromType][3] = tileutils_MakeTransition3(g_transitions[toType][fromType][0]);













	return 0;
}

uint16 *tileutils_GenerateAllWaterTable(uint16 width, uint16 height, uint16 x, uint16 y)
{
	uint16		*waterTable = new uint16[(height-y) * 2]; // TODO(phase-2): ownership transfer out of function
	uint16		i;
	uint16		 start;
	uint16		 end;

	for (i=y; i<height; i++) {
		if ((i-y)<=23) {
			start = (x + (23-(i-y))*2);
			end = width - start - 1;
		} else {
			start = (x + ((i-y)-24)*2);
			end = width - start - 1;
		}

		waterTable[(i-y)*2] = start;
		waterTable[(i-y)*2+1] = end;
	}

	return waterTable;
}

uint16 *tileutils_ExtractWaterTable(Pixel32 *image, uint16 width, uint16 height, uint16 x, uint16 y)
{
	uint16		*waterTable = new uint16[(height-y) * 2]; // TODO(phase-2): ownership transfer out of function
	BOOL		anyWater = FALSE;
	Pixel16		 r;
	Pixel16		 g;
	Pixel16		 b;
	Pixel16		 a;
	uint16		 start;
	uint16		 end;
	uint16		 i;
	uint16		 j;

	for (i=y; i<height; i++) {
		start = width;
		end = 0;
		for (j=0; j<width; j++) {
			RGB32Components(image[width * i + j], &r, &g, &b, &a);
			if (a > 0) {

				if (j < start)
					start = j;

				if (j > end)
					end = j;
			}
		}
		waterTable[(i-y)*2] = start;
		waterTable[(i-y)*2+1] = end;

		if (!anyWater && end != 0)
			anyWater = TRUE;
	}

	if (!anyWater) {
		delete[] waterTable;

		return nullptr;
	}

	return waterTable;
}

BaseTile	*g_baseTiles[k_MAX_BASE_TILES];




void tileutils_BorkifyTile(uint16 tileNum, MBCHAR ageChar, uint16 baseType, BOOL useT0, BOOL useT1, BOOL useT2, BOOL useT3)
{
	sint32		 x;
	sint32		 y;
	sint32		 startX;
	sint32		 endX;

	Pixel16		borkPix;
	sint32		yoffset = 24;
	uint32		accumList[k_TILE_PIXEL_HEIGHT][3];
	sint32		accumIndex = 0;
	sint32		accumCount;
	uint32		accum = 0;
	uint16		 width;
	uint16		 height;

	Pixel16 *   bork = tileutils_CreateBorkBork();

	MBCHAR		filename[_MAX_PATH];
	snprintf(filename, sizeof(filename), "source" FILE_SEP "basetiles" FILE_SEP "GT%cB%.4d.tif", ageChar, tileNum);

	TifBuffer tifBuf(
		(baseType == TERRAIN_WATER_BEACH)
			? tileutils_StripTIF2Mem(filename, &width, &height)
			: tileutils_TIF2mem(filename, &width, &height));
	char		*tif = tifBuf.get();

	Assert(tif != nullptr);
	if (tif == nullptr) {
		printf("\n*** Could not find '%s'.\n", filename);
		exit(-1);
	}

	std::vector<Pixel16> tileImage_vec = RGB32ToRGB16(tif, width, height);
	Pixel16 *   tileImage = tileImage_vec.data();

	for (auto & i : accumList) {
		i[0] = 0;
		i[1] = 0;
		i[2] = 0;
	}

	uint32      tileDataLen = (k_TILE_PIXEL_WIDTH * k_TILE_PIXEL_HEIGHT)/2 + k_TILE_PIXEL_HEIGHT;
	std::vector<Pixel16> tileData_vec(tileDataLen);
	Pixel16 *   tileData    = tileData_vec.data();
	Pixel16 *   tileDataPtr = tileData;

	sint32 len = 0;
	for (y=0; y<k_TILE_PIXEL_HEIGHT; y++) {
		if (y<=23) {
			startX = (23-y)*2;
			endX = k_TILE_PIXEL_WIDTH - startX;
		} else {
			startX = (y-24)*2;
			endX = k_TILE_PIXEL_WIDTH - startX;
		}

		accum = 0;
		accumIndex = 0;
		accumCount = 0;
		for (x = startX; x < endX; x++) {

			accum >>= 1;
			borkPix = bork[y*k_TILE_PIXEL_WIDTH+x];

			switch (borkPix) {
			case 0: if (!useT0)
						borkPix = 0xFFFF;
				break;
			case 1: if (!useT1)
						borkPix = 0xFFFF;
				break;
			case 2: if (!useT2)
						borkPix = 0xFFFF;
				break;
			case 3: if (!useT3)
						borkPix = 0xFFFF;
				break;
			}

			if (borkPix == 0xFFFF) {

				Pixel16 pix = tileImage[(y+yoffset)*k_TILE_PIXEL_WIDTH+x];


				if (pix == 0x0000 || pix == 0x0001 || pix == 0x0002 || pix == 0x0003) {
					pix = pix | 0x0820;
				}

				*tileDataPtr++ = pix;
				len++;
			} else {

				*tileDataPtr++ = borkPix;
				len++;
				accum |= 0x80000000;
			}

			accumCount++;

			if (accumCount >= 32) {
				accumList[y][accumIndex] = accum;
				accum = 0;
				accumIndex++;
				Assert(accumIndex<3);
				accumCount = 0;
			}
		}
		Assert(accumIndex<3);
		accum >>= 32-(endX-startX)-1;
		accumList[y][accumIndex] = accum;
	}

	BaseTile	*baseTile = new BaseTile;

	baseTile->SetBaseType((uint8)g_theTerrainDB->Get(baseType)->GetTilesetIndex());


	baseTile->SetTransitionFlag(0, useT0);
	baseTile->SetTransitionFlag(1, useT1);
	baseTile->SetTransitionFlag(2, useT2);
	baseTile->SetTransitionFlag(3, useT3);







	Assert(len >= 0);
	baseTile->SetTileDataLen(static_cast<uint16>(len*2));

	Pixel16		*dataCopy = new Pixel16[len]; // TODO(phase-2): ownership transfer out of function
	memcpy(dataCopy, tileData, len*2);
	baseTile->SetTileData(dataCopy);




	uint16		*waterTable;
	uint32		waterTableLen = k_TILE_PIXEL_HEIGHT * 2 * sizeof(uint16);

	if (baseType == TERRAIN_WATER_BEACH) {

		waterTable = tileutils_ExtractWaterTable((Pixel32 *)tif, width, height, 0, 24);
	} else {
		if (g_theTerrainDB->Get(baseType)->GetMovementTypeSea() ||
			g_theTerrainDB->Get(baseType)->GetMovementTypeShallowWater()) {

			waterTable = tileutils_GenerateAllWaterTable(width, height, 0, 24);
		} else {

			waterTable = nullptr;
			waterTableLen = 0;
		}
	}







	char		*hatTif=nullptr;
	Pixel16		*hatData=nullptr;
	uint32		hatDataLen=0;

	snprintf(filename, sizeof(filename), "source" FILE_SEP "hats" FILE_SEP "GTFh%.4d.tif", tileNum);
	TifBuffer hatTifBuf(tileutils_TIF2mem(filename, &width, &height));
	hatTif = hatTifBuf.get();
	if (hatTif) {
		hatData = (Pixel16 *)tileutils_EncodeTile((Pixel32 *)hatTif, width, height, &hatDataLen);
	}

	baseTile->SetHatDataLen((uint16)hatDataLen);
	baseTile->SetHatData(hatData);

	g_baseTiles[tileNum] = baseTile;

	delete[] bork;
}

uint16 tileutils_CompileImprovements(FILE *file)
{
	sint32		i;
	MBCHAR		filename[_MAX_PATH];
	uint32		dataLen;
	char		*tif;
	uint16		 width;
	uint16		 height;
	uint16		 id;
	Pixel16		*data;
	uint16		count = 0;
#ifdef WIN32
	struct _stat tmpstat;
#else
	struct stat tmpstat;
#endif
	int			r;

	for (i=0; i<k_MAX_IMPROVEMENTS; i++) {
		snprintf(filename, sizeof(filename), "source%simprove%sGTFM%.3d.tif",
		        FILE_SEP, FILE_SEP, i);
#ifdef WIN32
		r = (sint32)_stat(filename, &tmpstat);
#else
		r = stat(filename, &tmpstat);
#endif
		if (r == 0) count++;
	}

	fwrite(&count, 1, sizeof(uint16), file);

	for (i=0; i<k_MAX_IMPROVEMENTS; i++) {
		snprintf(filename, sizeof(filename), "source%simprove%sGTFM%.3d.tif",
		        FILE_SEP, FILE_SEP, i);
		TifBuffer tifBuf(tileutils_TIF2mem(filename, &width, &height));
		tif = tifBuf.get();
		if (tif) {
			data = (Pixel16 *)tileutils_EncodeTile((Pixel32 *)tif, width, height, &dataLen);

			id = (uint16)i;

			fwrite(&id, 1, sizeof(uint16), file);

			fwrite(&dataLen, 1, sizeof(uint32), file);

			fwrite(data, 1, dataLen, file);
		}
	}

	return count;
}

void tileutils_EncodeTileset(MBCHAR *filename)
{
	sint32		 i;
	sint32		 j;
	sint32		 k;

	for (i=0; i<TERRAIN_MAX; i++) {
		for (j=0; j<TERRAIN_MAX; j++) {
			for(k=0; k<k_TRANSITIONS_PER_TILE; k++) {
				g_transitions[i][j][k] = nullptr;
			}
		}
	}

	for (i=0; i<k_MAX_BASE_TILES; i++) {
		g_baseTiles[i] = nullptr;
	}

	tileutils_ParseTileset(filename);
}

sint32 tileutils_ParseTileset(MBCHAR *filename)
{
	MBCHAR			scriptName[_MAX_PATH];
	BOOL			done = FALSE;
	std::vector<sint16> transforms[k_MAX_TRANSFORMS];
	std::vector<sint16> riverTransforms[k_MAX_RIVER_TRANSFORMS];
	Pixel16			*riverData[k_MAX_RIVER_TRANSFORMS];
	uint32			riverDataLen[k_MAX_RIVER_TRANSFORMS];
	uint16			megaTileLengths[k_MAX_MEGATILES];
	MegaTileStep	megaTileData[k_MAX_MEGATILES][k_MAX_MEGATILE_STEPS];

	sint32			 j;
	sint32			 k;
	MBCHAR			ageChar = 'f';
	uint16			numRiverTransforms = 0;
	uint16			numTransforms = 0;
	uint16			numTransitions = 0;
	uint16			numBaseTiles = 0;
	uint16			numMegaTiles = 0;
	uint16			numImprovements = 0;

	snprintf(scriptName, sizeof(scriptName), "%s", filename);

	printf("\nParsing Tileset Script: '%s'\n", scriptName);

	auto theToken = std::make_unique<Token>(scriptName, C3DIR_SPRITES);

	sint32 tmp;

	while (!done) {
		switch (theToken->Next()) {
		case TOKEN_TILESET_TILE :
			{

				theToken->Next();
				theToken->GetNumber(tmp);

				sint32	tileNum = tmp;
				sint32	hasTransition[4];

				if (!token_ParseAnOpenBraceNext(theToken.get())) return FALSE;

				if (!token_ParseValNext(theToken.get(), TOKEN_TILESET_TILE_BASE_TYPE, tmp)) return FALSE;
				sint32 baseType = tmp;

				if (!token_ParseValNext(theToken.get(), TOKEN_TILESET_TILE_TRANS_0, tmp)) return FALSE;
				hasTransition[0] = tmp;
				if (!token_ParseValNext(theToken.get(), TOKEN_TILESET_TILE_TRANS_1, tmp)) return FALSE;
				hasTransition[1] = tmp;
				if (!token_ParseValNext(theToken.get(), TOKEN_TILESET_TILE_TRANS_2, tmp)) return FALSE;
				hasTransition[2] = tmp;
				if (!token_ParseValNext(theToken.get(), TOKEN_TILESET_TILE_TRANS_3, tmp)) return FALSE;
				hasTransition[3] = tmp;

				printf(" * Basetile %.3d\n", tileNum);

				sint32 i;
				for(i = 0; i < g_theTerrainDB->NumRecords(); i++) {
					if(g_theTerrainDB->Get(i)->GetTilesetIndex() == baseType) {
						baseType = i;
						break;
					}
				}

				tileutils_BorkifyTile((uint16)tileNum, ageChar, (uint16)baseType, hasTransition[0], hasTransition[1], hasTransition[2], hasTransition[3]);

				if(tileNum < 100 &&
				   (hasTransition[0] || hasTransition[1] || hasTransition[2] || hasTransition[3])) {
					tileutils_BorkifyTile((uint16)(tileNum * 100) + 99, ageChar, (uint16)baseType, 0, 0, 0, 0);
				}

				if (!token_ParseAnCloseBraceNext(theToken.get())) return FALSE;

				numBaseTiles++;
			}
			break;
		case TOKEN_TILESET_MEGATILE:
			{
				MBCHAR		configStr[100];

				if (!token_ParseAnOpenBraceNext(theToken.get())) return FALSE;

				if (!token_ParseKeywordNext(theToken.get(), TOKEN_TILESET_MEGATILE_CONFIG)) return FALSE;

				theToken->Next();
				theToken->GetString(configStr);

                size_t len = strlen(configStr);

				megaTileLengths[numMegaTiles] = static_cast<uint16>(len);

				for (size_t i = 0; i < len; i++)
                {
					if (!token_ParseKeywordNext(theToken.get(), TOKEN_TILESET_MEGATILE_INFO)) return FALSE;

					uint8		dir = 0;

					switch (configStr[i]) {
					case k_MEGATILE_DIRECTION_CHAR_X : dir = k_MEGATILE_DIRECTION_X; break;
					case k_MEGATILE_DIRECTION_CHAR_N : dir = k_MEGATILE_DIRECTION_N; break;
					case k_MEGATILE_DIRECTION_CHAR_E : dir = k_MEGATILE_DIRECTION_E; break;
					case k_MEGATILE_DIRECTION_CHAR_S : dir = k_MEGATILE_DIRECTION_S; break;
					case k_MEGATILE_DIRECTION_CHAR_W : dir = k_MEGATILE_DIRECTION_W; break;
					default:
						Assert(FALSE);
					}

					sint32			terrainType;

					theToken->Next();
					theToken->GetNumber(terrainType);

					sint32			tileNum;

					theToken->Next();
					theToken->GetNumber(tileNum);

					MegaTileStep	step;

					step.direction = dir;
					step.terrainType = terrainType;
					step.tileNum = tileNum;

					megaTileData[numMegaTiles][i] = step;
				}

				printf(" * Supertile %s\n", configStr);

				if (!token_ParseAnCloseBraceNext(theToken.get())) return FALSE;

				numMegaTiles++;
			}
			break;
		case TOKEN_TILESET_TRANSFORM:
			{
				theToken->Next();
				theToken->GetNumber(tmp);
//				sint32 transformNum = tmp;

				if (!token_ParseAnOpenBraceNext(theToken.get())) return FALSE;

				transforms[numTransforms].assign(k_TRANSFORM_SIZE, k_TRANSFORM_TO_LIST_ID);

				size_t      i;
				for (i=0; i<k_TRANSFORM_TO_INDEX; i++) {
					theToken->Next();
					theToken->GetNumber(tmp);
					transforms[numTransforms][i] = (sint16)tmp;
				}

				switch (theToken->Next()) {

					case TOKEN_TILESET_TRANSFORM_TO :
							theToken->Next();
							theToken->GetNumber(tmp);

							transforms[numTransforms][k_TRANSFORM_TO_INDEX] = (sint16)tmp;
						break;

					case TOKEN_TILESET_TRANSFORM_TO_LIST :

							transforms[numTransforms][k_TRANSFORM_TO_INDEX] = k_TRANSFORM_TO_LIST_ID;

							for (i=0; i<k_MAX_TRANSFORM_TO_LIST; i++) {
								theToken->Next();
								theToken->GetNumber(tmp);
								transforms[numTransforms][k_TRANSFORM_TO_LIST_FIRST+i] = (sint16)tmp;
							}
						break;
				}

				if (!token_ParseAnCloseBraceNext(theToken.get())) return FALSE;

				printf(" * Transform %.3d\n", numTransforms);

				numTransforms++;
				Assert (numTransforms < k_MAX_TRANSFORMS);
			}
			break;

		case TOKEN_TILESET_TRANSITION:
			{
				sint16		 fromType;
				sint16		 toType;

				if (!token_ParseAnOpenBraceNext(theToken.get())) return FALSE;

				if (!token_ParseValNext(theToken.get(), TOKEN_TILESET_TRANSITION_FROM, tmp)) return FALSE;
				fromType = (sint16)tmp;

				if (!token_ParseValNext(theToken.get(), TOKEN_TILESET_TRANSITION_TO, tmp)) return FALSE;
				toType = (sint16)tmp;

				if (!token_ParseAnCloseBraceNext(theToken.get())) return FALSE;

				tileutils_ExtractStencils(fromType, toType);

				printf(" * Transition %.2d->%.2d\n", fromType, toType);

				numTransitions++;
			}
			break;

		case TOKEN_TILESET_RIVER_TRANSFORM:
			{
				if (!token_ParseAnOpenBraceNext(theToken.get())) return FALSE;

				riverTransforms[numRiverTransforms].resize(k_RIVER_TRANSFORM_SIZE);
				for (size_t i = 0; i<k_RIVER_TRANSFORM_SIZE-1; i++) {
					theToken->Next();
					theToken->GetNumber(tmp);
					riverTransforms[numRiverTransforms][i] = (sint16)tmp;
				}

				if (!token_ParseValNext(theToken.get(), TOKEN_TILESET_RIVER_PIECE, tmp)) return FALSE;
				riverTransforms[numRiverTransforms][k_RIVER_TRANSFORM_SIZE-1] = (sint16)tmp;

				if (!token_ParseAnCloseBraceNext(theToken.get())) return FALSE;

				MBCHAR		filename[_MAX_PATH];
				char		*tif;
				uint16		 width;
				uint16		 height;

				snprintf(filename, sizeof(filename), "source" FILE_SEP "rivers" FILE_SEP "GTFL%.2d.tif", tmp);
				TifBuffer tifBuf(tileutils_TIF2mem(filename, &width, &height));
				tif = tifBuf.get();

				uint32		dataLen=0;

				if (tif) riverData[numRiverTransforms] = (Pixel16 *)tileutils_EncodeTile((Pixel32 *)tif, width, height, &dataLen);
				else riverData[numRiverTransforms] = nullptr;

				riverDataLen[numRiverTransforms] = dataLen;


				numRiverTransforms++;
			}
			break;

		case TOKEN_TILESET_END :
			done = TRUE;
			break;
		}
	}

	MBCHAR	fname[_MAX_PATH];

	printf("\nWriting Tileset: ");

	if (is_565_Get()) {
		snprintf(fname, sizeof(fname), "output" FILE_SEP "gtset565.til");
	} else {
		snprintf(fname, sizeof(fname), "output" FILE_SEP "gtset555.til");
	}

	FILE *  tfile = fopen(fname, "wb");
	if (tfile) {

		fwrite(&numTransforms, 1, sizeof(uint16), tfile);

		if (numTransforms > 0) {

			for (size_t i = 0; i < numTransforms; i++)
            {
				fwrite(transforms[i].data(), 1, sizeof(sint16)*k_TRANSFORM_SIZE, tfile);

			}

			printf("...Transforms");
		}




		if (numTransitions > 0) {
			uint32		transitionCount = 0;

			size_t  i;
			for (i=0; i<TERRAIN_MAX; i++) {
				for (j=0; j<TERRAIN_MAX; j++) {
					if (g_transitions[i][j][0] != nullptr)
						transitionCount++;
				}
			}

			fwrite(&transitionCount, 1, sizeof(uint32), tfile);

			uint32  transitionSize = g_stencilSize * sizeof(Pixel16);
			fwrite(&transitionSize, 1, sizeof(uint32), tfile);

			for (i=0; i<TERRAIN_MAX; i++) {
				for (j=0; j<TERRAIN_MAX; j++) {
					sint16 from;
					sint16 to;

					if (g_transitions[i][j][0] != nullptr) {
						from = (sint16)i;
						to = (sint16)j;

						fwrite(&from, 1, sizeof(sint16), tfile);
						fwrite(&to, 1, sizeof(sint16), tfile);

						for(k=0; k<k_TRANSITIONS_PER_TILE; k++) {
							fwrite(g_transitions[i][j][k], 1, transitionSize, tfile);
						}
					}
				}
			}
			printf("...Transitions");
		}

		if (numBaseTiles > 0) {
			uint32		baseTileCount = 0;

			size_t i;
			for (i=0; i<k_MAX_BASE_TILES; i++) {
				if (g_baseTiles[i] != nullptr)
					baseTileCount++;
			}

			fwrite(&baseTileCount, 1, sizeof(uint32), tfile);

			for (i=0; i<k_MAX_BASE_TILES; i++) {
				if (g_baseTiles[i] != nullptr) {
					uint16		tNum = (uint16)i;
					fwrite(&tNum, 1, sizeof(uint16), tfile);

					uint8   baseType = (uint8)g_baseTiles[i]->GetBaseType();
					fwrite(&baseType, 1, sizeof(uint8), tfile);

					uint8 flag = 0;
					for (j = 0; j < k_TRANSITIONS_PER_TILE; j++)
                    {
						if (g_baseTiles[i]->GetTransitionFlag((uint16)j))
							flag |= (1 << j);
					}

					fwrite(&flag, 1, sizeof(uint8), tfile);








					uint16 len16 = g_baseTiles[i]->GetTileDataLen();
					fwrite(&len16, 1, sizeof(uint16), tfile);

					Pixel16 * tileData = g_baseTiles[i]->GetTileData();
					fwrite(tileData, 1, len16, tfile);
















					len16 = (uint16)g_baseTiles[i]->GetHatDataLen();
					fwrite(&len16, 1, sizeof(uint16), tfile);

					if (len16 > 0) {
						fwrite(g_baseTiles[i]->GetHatData(), 1, len16, tfile);
					}

					delete g_baseTiles[i];
					g_baseTiles[i] = nullptr;
				}
			}
			printf("...Base Tiles");
		}


		fwrite(&numRiverTransforms, 1, sizeof(uint16), tfile);

		if (numRiverTransforms > 0) {

			for (size_t i = 0; i < numRiverTransforms; i++) {
				fwrite(riverTransforms[i].data(), 1, sizeof(sint16)*k_RIVER_TRANSFORM_SIZE, tfile);

				fwrite(&riverDataLen[i], 1, sizeof(uint32), tfile);

				if (riverDataLen[i] > 0) {
					fwrite(riverData[i], 1, riverDataLen[i], tfile);
				}

				delete[] riverData[i];

				riverData[i] = nullptr;
			}

			printf("...River Transforms");
		}

		printf("...Improvements");
		numImprovements = tileutils_CompileImprovements(tfile);

		if (numMegaTiles > 0) {
			uint16		num = (uint16)numMegaTiles;

			fwrite(&num, 1, sizeof(uint16), tfile);
			for (size_t i = 0; i < numMegaTiles; i++) {
				uint16		len = megaTileLengths[i];

				fwrite(&len, 1, sizeof(uint16), tfile);

				fwrite(megaTileData[i], 1, sizeof(MegaTileStep)*len, tfile);
			}

			printf("...MegaTiles");
		}

		printf("\n\n");

		printf(" * Base Tiles:    %d\n", numBaseTiles);
		printf(" * Transitions:   %d\n", numTransitions);
		printf(" * Transforms:    %d\n", numTransforms);
		printf(" * River Xforms:  %d\n", numRiverTransforms);
		printf(" * MegaTiles:     %d\n", numMegaTiles);
		printf(" * Improvements:  %d\n", numImprovements);

		fclose(tfile);
	}
	return TRUE;

}
