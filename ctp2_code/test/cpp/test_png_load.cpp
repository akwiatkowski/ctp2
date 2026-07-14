#include "doctest.h"

#include "ctp/c3.h"
#include "gfx/gfx_utils/png_load.h"

#include <cstring>
#include <vector>
#include <zlib.h>

namespace {

// Emit one PNG chunk: length(BE) + type + data + CRC32(type+data)(BE).
void append_chunk(std::vector<uint8_t> & out, char const * type,
                  std::vector<uint8_t> const & body)
{
	auto be32 = [&](uint32_t v) {
		out.push_back((v >> 24) & 0xFF); out.push_back((v >> 16) & 0xFF);
		out.push_back((v >> 8) & 0xFF);  out.push_back(v & 0xFF);
	};
	be32(static_cast<uint32_t>(body.size()));
	size_t const typePos = out.size();
	out.insert(out.end(), type, type + 4);
	out.insert(out.end(), body.begin(), body.end());
	uLong crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc, out.data() + typePos, static_cast<uInt>(4 + body.size()));
	be32(static_cast<uint32_t>(crc));
}

// Build a converter-shaped RGBA8 / filter-0 / single-IDAT PNG from raw RGBA.
std::vector<uint8_t> make_png(int w, int h, std::vector<uint8_t> const & rgba)
{
	std::vector<uint8_t> png = {0x89,'P','N','G','\r','\n',0x1A,'\n'};

	std::vector<uint8_t> ihdr;
	auto push_be32 = [&](std::vector<uint8_t> & v, uint32_t x) {
		v.push_back((x >> 24) & 0xFF); v.push_back((x >> 16) & 0xFF);
		v.push_back((x >> 8) & 0xFF);  v.push_back(x & 0xFF);
	};
	push_be32(ihdr, static_cast<uint32_t>(w));
	push_be32(ihdr, static_cast<uint32_t>(h));
	ihdr.push_back(8);   // bit depth
	ihdr.push_back(6);   // colour type: RGBA
	ihdr.push_back(0);   // compression
	ihdr.push_back(0);   // filter method
	ihdr.push_back(0);   // interlace: none
	append_chunk(png, "IHDR", ihdr);

	// Raw = per row: filter byte 0 + row RGBA.
	std::vector<uint8_t> raw;
	size_t const stride = static_cast<size_t>(w) * 4;
	for (int y = 0; y < h; ++y) {
		raw.push_back(0);
		raw.insert(raw.end(), rgba.begin() + y * stride, rgba.begin() + (y + 1) * stride);
	}
	uLongf zlen = compressBound(static_cast<uLong>(raw.size()));
	std::vector<uint8_t> idat(zlen);
	REQUIRE(compress2(idat.data(), &zlen, raw.data(),
	                  static_cast<uLong>(raw.size()), 9) == Z_OK);
	idat.resize(zlen);
	append_chunk(png, "IDAT", idat);

	append_chunk(png, "IEND", {});
	return png;
}

} // namespace

TEST_CASE("png_decode_rgba round-trips a converter-shaped RGBA8 PNG")
{
	// 2x2 image: red, green / blue, opaque-black.
	std::vector<uint8_t> src = {
		0xFF,0x00,0x00,0xFF,  0x00,0xFF,0x00,0xFF,
		0x00,0x00,0xFF,0xFF,  0x00,0x00,0x00,0xFF,
	};
	std::vector<uint8_t> png = make_png(2, 2, src);

	int w = 0, h = 0;
	std::vector<uint8_t> out;
	REQUIRE(png_decode_rgba(png.data(), png.size(), w, h, out));
	CHECK(w == 2);
	CHECK(h == 2);
	REQUIRE(out.size() == src.size());
	CHECK(std::memcmp(out.data(), src.data(), src.size()) == 0);
}

TEST_CASE("png_decode_rgba rejects a bad signature and truncation")
{
	int w = 0, h = 0;
	std::vector<uint8_t> out;
	std::vector<uint8_t> notPng = {'n','o','t','a','p','n','g','!'};
	CHECK_FALSE(png_decode_rgba(notPng.data(), notPng.size(), w, h, out));

	std::vector<uint8_t> src(4 * 4, 0x7F);
	std::vector<uint8_t> png = make_png(2, 2, src);
	CHECK_FALSE(png_decode_rgba(png.data(), png.size() / 2, w, h, out));  // truncated
}
