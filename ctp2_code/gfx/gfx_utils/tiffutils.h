#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __TIFFUTILS_H__
#define __TIFFUTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

	char *tiffutils_LoadTIF(char const * filename, uint16_t *width, uint16_t *height, size_t *size = nullptr);
	char *TIF2mem(char const * filename, uint16_t *width, uint16_t *height, size_t *size = nullptr);

	int TIFGetMetrics(char const * filename, uint16_t *width, uint16_t *height);
	int TIFLoadIntoBuffer16(char const * filename, uint16_t *width, uint16_t *height, uint16_t imageRowBytes, uint16_t *buffer, bool is565);

	char *StripTIF2Mem(char const * filename, uint16_t *width, uint16_t *height, size_t *size = nullptr);
#ifdef __cplusplus
}

#include <memory>

// Owning handle for the C-allocated pixel buffers the loaders above (and
// spriteutils_CreateQuarterSize) return. Wrap the raw return right at the
// call site; the buffer is released automatically on every exit path.
struct tiffutils_BufferDeleter
{
	void operator()(char *pixels) const;
};
using TifBuffer = std::unique_ptr<char, tiffutils_BufferDeleter>;
#endif

#endif
