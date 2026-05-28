//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Pixel typedefs (extracted from gfx/gfx_utils/pixeltypes.h)
//
// These typedefs are pure-data primitives — just sized integer aliases
// for color formats.  Moved here so gs/ headers (e.g. gs/fileio/gamefile.h)
// can store Pixel16 arrays without depending on gfx/.
//
// gfx/gfx_utils/pixeltypes.h re-includes this header to preserve the
// existing API for gfx/ consumers.
//
//----------------------------------------------------------------------------

#pragma once

#include <stdint.h>

typedef uint32_t Pixel32;
typedef uint16_t Pixel16;
typedef uint8_t  Pixel8;
