//----------------------------------------------------------------------------
// Minimal PNG loader for modern-asset atlases (P11 Stage 2 B1). See png_load.h
// for the exact (narrow) format contract. Uses the vendored zlib.
//----------------------------------------------------------------------------
#include "ctp/c3.h"
#include "gfx/gfx_utils/png_load.h"

#include <cstdio>
#include <cstring>
#include <zlib.h>

namespace
{
    // PNG multi-byte integers are big-endian.
    uint32_t be32(uint8_t const * p)
    {
        return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16)
             | (static_cast<uint32_t>(p[2]) << 8)  |  static_cast<uint32_t>(p[3]);
    }

    uint8_t const k_PNG_SIG[8] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };

    // Guard against absurd allocations from a corrupt/hostile header.
    int const k_MAX_DIM = 16384;
}

bool png_decode_rgba(uint8_t const * data, size_t size,
                     int & width, int & height,
                     std::vector<uint8_t> & rgba)
{
    if (!data || size < 8 || std::memcmp(data, k_PNG_SIG, 8) != 0)
        return false;

    int              w = 0;
    int              h = 0;
    bool             haveIhdr = false;
    std::vector<uint8_t> idat;

    size_t pos = 8;
    while (pos + 12 <= size)                 // 4 length + 4 type + >=0 data + 4 crc
    {
        uint32_t const len = be32(data + pos);
        uint8_t const * type = data + pos + 4;
        size_t const bodyPos = pos + 8;
        if (len > size || bodyPos + len + 4 > size)     // truncated / overflow
            return false;

        if (std::memcmp(type, "IHDR", 4) == 0)
        {
            if (len != 13) return false;
            w = static_cast<int>(be32(data + bodyPos));
            h = static_cast<int>(be32(data + bodyPos + 4));
            uint8_t const bitDepth  = data[bodyPos + 8];
            uint8_t const colorType = data[bodyPos + 9];
            uint8_t const interlace = data[bodyPos + 12];
            // Only the converter's exact shape: RGBA8, no interlace.
            if (bitDepth != 8 || colorType != 6 || interlace != 0) return false;
            if (w <= 0 || h <= 0 || w > k_MAX_DIM || h > k_MAX_DIM) return false;
            haveIhdr = true;
        }
        else if (std::memcmp(type, "IDAT", 4) == 0)
        {
            if (!haveIhdr) return false;
            idat.insert(idat.end(), data + bodyPos, data + bodyPos + len);
        }
        else if (std::memcmp(type, "IEND", 4) == 0)
        {
            break;
        }

        pos = bodyPos + len + 4;             // skip data + CRC
    }

    if (!haveIhdr || idat.empty())
        return false;

    // Inflate: our PNGs prefix each scanline with one filter byte (must be 0),
    // so the raw stream is exactly height * (1 + width*4) bytes.
    size_t const stride   = static_cast<size_t>(w) * 4;
    uLongf       rawLen   = static_cast<uLongf>(h) * (1 + stride);
    std::vector<uint8_t> raw(rawLen);
    if (uncompress(raw.data(), &rawLen, idat.data(),
                   static_cast<uLong>(idat.size())) != Z_OK
        || rawLen != static_cast<uLongf>(h) * (1 + stride))
    {
        return false;
    }

    rgba.resize(static_cast<size_t>(h) * stride);
    for (int y = 0; y < h; ++y)
    {
        uint8_t const * row = raw.data() + static_cast<size_t>(y) * (1 + stride);
        if (row[0] != 0) return false;       // only filter type 0 (None) supported
        std::memcpy(rgba.data() + static_cast<size_t>(y) * stride, row + 1, stride);
    }

    width  = w;
    height = h;
    return true;
}

bool png_load_rgba(char const * path,
                   int & width, int & height,
                   std::vector<uint8_t> & rgba)
{
    FILE * fp = fopen(path, "rb");
    if (!fp)
        return false;

    fseek(fp, 0, SEEK_END);
    long const fileLen = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (fileLen <= 8)
    {
        fclose(fp);
        return false;
    }

    std::vector<uint8_t> buf(static_cast<size_t>(fileLen));
    size_t const got = fread(buf.data(), 1, buf.size(), fp);
    fclose(fp);
    if (got != buf.size())
        return false;

    return png_decode_rgba(buf.data(), buf.size(), width, height, rgba);
}
