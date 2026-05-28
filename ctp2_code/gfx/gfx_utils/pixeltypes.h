#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef PIXEL_TYPES_H__
#define PIXEL_TYPES_H__

// Pixel32/Pixel16/Pixel8 typedefs live in gs/core/pixel_types.h so gs/
// headers can store them without depending on gfx/.  This header
// re-includes the new canonical location for back-compat with gfx/ /
// ui/ / net/ consumers.
#include "gs/core/pixel_types.h"

#endif
