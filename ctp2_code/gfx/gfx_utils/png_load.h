//----------------------------------------------------------------------------
// Minimal PNG loader for modern-asset atlases (P11 Stage 2 B1).
//
// Decodes ONLY the deliberately-simple PNG the asset converter emits
// (tools/assets/spr_export.py): colour type 6 (RGBA), bit depth 8, no
// interlace, filter type 0 on every scanline, a single IDAT chunk. That lets
// the decode be inflate-then-strip-one-filter-byte-per-row with no
// un-filtering, using the already-linked vendored zlib. It is NOT a
// general-purpose PNG reader.
//----------------------------------------------------------------------------
#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __PNG_LOAD_H__
#define __PNG_LOAD_H__

#include <cstdint>
#include <vector>

/// Decode a converter-emitted RGBA8 PNG file into tightly-packed RGBA bytes
/// (row-major, top-down, 4 bytes/pixel, R,G,B,A order).
/// Returns true on success and fills width/height/rgba; false on any I/O,
/// signature, header-shape, or inflate error (rgba left unspecified).
bool png_load_rgba(char const * path,
                   int & width, int & height,
                   std::vector<uint8_t> & rgba);

/// Decode from an in-memory PNG blob (same constraints). Used by tests and by
/// callers that already hold the bytes.
bool png_decode_rgba(uint8_t const * data, size_t size,
                     int & width, int & height,
                     std::vector<uint8_t> & rgba);

#endif // __PNG_LOAD_H__
