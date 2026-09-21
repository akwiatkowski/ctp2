//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : tiff image format utilities
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
// - None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Removed unused local variables. (Sep 9th 2005 Martin Gähmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "gfx/gfx_utils/tiffutils.h"
#include <tiffio.h>
#include <vector>

namespace {
// Owning handle for _TIFFmalloc'd scratch rasters — the libtiff allocator
// counterpart to TifBuffer (which frees malloc'd return buffers).
struct TiffRasterDeleter
{
	// _TIFFfree takes void*; keep the deleter generic so the smart
	// pointer can hold the uint32_t element type TIFFReadRGBAImage wants.
	void operator()(uint32_t *pixels) const { _TIFFfree(pixels); }
};
using TiffRaster = std::unique_ptr<uint32_t, TiffRasterDeleter>;
}

char *tiffutils_LoadTIF(const char *filename, uint16_t *width, uint16_t *height, size_t *size)
{
	TIFF * tif = TIFFOpen(filename, "r");
	if (tif)
	{
		uint32_t w;
		uint32_t h;
		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);

		size_t   npixels = w * h;
		TiffRaster raster(static_cast<uint32_t *>(_TIFFmalloc(npixels * sizeof(uint32_t))));
		if (raster)
		{
			if (TIFFReadRGBAImage(tif, w, h, raster.get(), 0))
			{
				TifBuffer destImage(static_cast<char *>(malloc(npixels * sizeof(uint32_t))));
				if (!destImage) {
					TIFFClose(tif);
					return nullptr;
				}
				if (size)
					*size = npixels * sizeof(uint32_t);
				memcpy(destImage.get(), raster.get(), npixels * sizeof(uint32_t));

				*width = (uint16_t)w;
				*height = (uint16_t)h;

				return destImage.release();
			}
		}
		TIFFClose(tif);
	}

	return nullptr;
}

char *TIF2mem(const char *filename, uint16_t *width, uint16_t *height, size_t *size)
{
	TifBuffer image;
	uint32_t  w=0;
	uint32_t  h=0;
	TIFF    *tif = TIFFOpen(filename, "r");

	if (tif) {
		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);

		size_t npixels     = w * h;

		image.reset(static_cast<char *>(malloc(npixels * sizeof(uint32_t))));
		if (!image) {
			TIFFClose(tif);
			return nullptr;
		}

		if (size)
			*size = npixels * sizeof(uint32_t);

		TiffRaster raster(static_cast<uint32_t *>(_TIFFmalloc(npixels * sizeof(uint32_t))));
		if (raster) {
			sint32 bytesPerRow = w * 4;
			if (TIFFReadRGBAImage(tif, w, h, raster.get(), 0)) {
				char * imagePtr  = image.get();
				// Walk the raster in uint32_t elements (w per row) so no
				// cast is needed; memcpy takes the pointer as void*.
				uint32_t * rasterPtr = raster.get() + (w * (h-1));
				for (uint32_t row = 0; row < h; row++) {
					memcpy(imagePtr, rasterPtr, bytesPerRow);
					imagePtr += bytesPerRow;
					rasterPtr -= w;
				}
			}
		}

		TIFFClose(tif);
	}

	*width = (uint16_t)w;
	*height = (uint16_t)h;

	return image.release();
}

int TIFGetMetrics(const char *filename, uint16_t *width, uint16_t *height)
{
	TIFF *  tif = TIFFOpen(filename, "r");

	if (tif)
    {
	    uint32_t  w   = 0;
        uint32_t  h   = 0;

		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);

		TIFFClose(tif);

		*width = (uint16_t)w;
		*height = (uint16_t)h;
	}

	return 0;
}

int TIFLoadIntoBuffer16(const char *filename, uint16_t *width, uint16_t *height, uint16_t imageRowBytes, uint16_t *buffer, bool is565)
{
	uint32_t  w=0;
	uint32_t  h=0;
	TIFF    *tif = TIFFOpen(filename, "r");

	if (tif)
    {
		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);

		sint32  bytesPerRow = w * 4;
		size_t  npixels     = w * h;
		TiffRaster raster(static_cast<uint32_t *>(_TIFFmalloc(npixels * sizeof(uint32_t))));

        if (raster)
        {
			if (TIFFReadRGBAImage(tif, w, h, raster.get(), 0))
            {
				char * imagePtr     = (char *)buffer;
				char * rasterPtr    = reinterpret_cast<char *>(raster.get() + (w * (h-1)));

				uint32_t *rasterPtrCopy;
				uint16_t *imagePtrCopy;
				uint32_t pixel;
	                  sint32  i;

				if (is565) {
					for (uint32_t row = 0; row < h; row++) {

						imagePtrCopy = (uint16_t *)imagePtr;
						rasterPtrCopy = (uint32_t *)rasterPtr;

						for (i=0; i<(sint32)w; i++) {
							pixel = *rasterPtrCopy++;
							*imagePtrCopy++ = (uint16_t)(((pixel & 0x000000F8) << 8) | ((pixel & 0x0000FC00) >> 5) | ((pixel & 0x00F80000) >> 19));
						}

						imagePtr += imageRowBytes;
						rasterPtr -= bytesPerRow;
					}
				} else {
					for (uint32_t row = 0; row < h; row++) {

						imagePtrCopy = (uint16_t *)imagePtr;
						rasterPtrCopy = (uint32_t *)rasterPtr;

						for (i=0; i<(sint32)w; i++) {
							pixel = *rasterPtrCopy++;
							*imagePtrCopy++ = (uint16_t)(((pixel & 0x000000F8) << 7) | ((pixel & 0x0000F800) >> 6) | ((pixel & 0x00F80000) >> 19));
						}

						imagePtr += imageRowBytes;
						rasterPtr -= bytesPerRow;
					}
				}

			}

			// raster freed by TiffRaster
		}

		TIFFClose(tif);
	}

	*width = (uint16_t)w;
	*height = (uint16_t)h;

	return 0;
}




char *StripTIF2Mem(const char *filename, uint16_t *width, uint16_t *height, size_t *size)
{
	TIFF *  tif = TIFFOpen(filename, "r");
	if (!tif)
	   return nullptr;

	*width = static_cast<uint16_t>(-1);
	*height = static_cast<uint16_t>(-1);

	uint32_t      imageLength;
	uint32_t      imageWidth;
	uint32_t      RowsPerStrip;
	sint32      PhotometricInterpretation;

	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &imageWidth);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imageLength);
	TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &RowsPerStrip);
	TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &RowsPerStrip); /// @todo Check twice?
	TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &PhotometricInterpretation);

	tsize_t LineSize    = TIFFScanlineSize(tif);
	tsize_t stripSize   = TIFFStripSize(tif);
	std::vector<char> buf(stripSize);
	TifBuffer outBuf(static_cast<char *>(malloc(imageWidth * imageLength * 4)));
	if (size)
		*size = imageWidth * imageLength * 4;
	char *  outBufPtr   = outBuf.get();

	for (uint32_t row = 0; row < imageLength; row += RowsPerStrip)
	{
		tsize_t nrow = (row + RowsPerStrip > imageLength ? imageLength - row : RowsPerStrip);
		if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, row, 0), buf.data(), nrow*LineSize)==-1)
        {
            // outBuf freed by TifBuffer on this error path
			return nullptr;
		}
        else
        {
			for (tsize_t l = 0; l < nrow; l++)
            {
				memcpy(outBufPtr, &buf[l * LineSize], imageWidth * 4);
				outBufPtr += imageWidth * 4;
			}
		 }
	}

	TIFFClose(tif);

	*width  = (uint16_t) imageWidth;
	*height = (uint16_t) imageLength;

	return outBuf.release();
}

// Out-of-line so consumers of TifBuffer never spell the C deallocator —
// the buffers come from the C allocator inside the loaders above.
void tiffutils_BufferDeleter::operator()(char *pixels) const
{
	free(pixels);
}
