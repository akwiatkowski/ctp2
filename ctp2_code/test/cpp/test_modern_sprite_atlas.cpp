#include "doctest.h"

#include "ctp/c3.h"
#include "gfx/spritesys/ModernSpriteAtlas.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include <zlib.h>

namespace {

void be32(std::vector<uint8_t> & v, uint32_t x)
{
	v.push_back((x >> 24) & 0xFF); v.push_back((x >> 16) & 0xFF);
	v.push_back((x >> 8) & 0xFF);  v.push_back(x & 0xFF);
}

void chunk(std::vector<uint8_t> & out, char const * type, std::vector<uint8_t> const & body)
{
	be32(out, static_cast<uint32_t>(body.size()));
	size_t const typePos = out.size();
	out.insert(out.end(), type, type + 4);
	out.insert(out.end(), body.begin(), body.end());
	uLong crc = crc32(crc32(0L, Z_NULL, 0), out.data() + typePos, static_cast<uInt>(4 + body.size()));
	be32(out, static_cast<uint32_t>(crc));
}

// A converter-shaped RGBA8 / filter-0 / single-IDAT PNG (see png_load).
void write_png(std::string const & path, int w, int h, std::vector<uint8_t> const & rgba)
{
	std::vector<uint8_t> png = {0x89,'P','N','G','\r','\n',0x1A,'\n'};
	std::vector<uint8_t> ihdr;
	be32(ihdr, static_cast<uint32_t>(w)); be32(ihdr, static_cast<uint32_t>(h));
	ihdr.push_back(8); ihdr.push_back(6); ihdr.push_back(0); ihdr.push_back(0); ihdr.push_back(0);
	chunk(png, "IHDR", ihdr);
	std::vector<uint8_t> raw;
	size_t const stride = static_cast<size_t>(w) * 4;
	for (int y = 0; y < h; ++y) { raw.push_back(0); raw.insert(raw.end(), rgba.begin() + y*stride, rgba.begin() + (y+1)*stride); }
	uLongf zlen = compressBound(static_cast<uLong>(raw.size()));
	std::vector<uint8_t> idat(zlen);
	REQUIRE(compress2(idat.data(), &zlen, raw.data(), static_cast<uLong>(raw.size()), 9) == Z_OK);
	idat.resize(zlen);
	chunk(png, "IDAT", idat);
	chunk(png, "IEND", {});
	FILE * fp = fopen(path.c_str(), "wb");
	REQUIRE(fp != nullptr);
	fwrite(png.data(), 1, png.size(), fp);
	fclose(fp);
}

void write_file(std::string const & path, std::string const & text)
{
	FILE * fp = fopen(path.c_str(), "wb");
	REQUIRE(fp != nullptr);
	fwrite(text.data(), 1, text.size(), fp);
	fclose(fp);
}

} // namespace

TEST_CASE("ModernSpriteAtlas loads manifest + PNG and resolves frame rects")
{
	std::string const png  = "/tmp/ctp2_atlas_test.png";
	std::string const json = "/tmp/ctp2_atlas_test.json";

	// 2x2 atlas: red, green / blue, white.
	write_png(png, 2, 2, {0xFF,0,0,0xFF, 0,0xFF,0,0xFF, 0,0,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF});
	write_file(json, R"json({
		"source": "GU04.SPR",
		"atlas": {"png": "ctp2_atlas_test.png", "width": 2, "height": 2},
		"actions": [{
			"name": "IDLE", "width": 1, "height": 1, "num_frames": 1, "facings": 2,
			"frames": [
				{"facing": 0, "frame": 0, "rect": {"x": 0, "y": 0, "w": 1, "h": 1}},
				{"facing": 1, "frame": 0, "rect": {"x": 1, "y": 1, "w": 1, "h": 1}}
			]
		}]
	})json");

	std::string error;
	std::unique_ptr<ModernSpriteAtlas> atlas(ModernSpriteAtlas::Load(json.c_str(), error));
	REQUIRE(atlas != nullptr);
	CHECK(error.empty());
	CHECK(atlas->Width() == 2);
	CHECK(atlas->Height() == 2);
	CHECK(atlas->Rgba().size() == 2u * 2u * 4u);

	ModernSpriteRect const * r0 = atlas->FindRect("IDLE", 0, 0);
	REQUIRE(r0 != nullptr);
	CHECK(r0->x == 0);
	CHECK(r0->w == 1);
	ModernSpriteRect const * r1 = atlas->FindRect("IDLE", 1, 0);
	REQUIRE(r1 != nullptr);
	CHECK(r1->x == 1);
	CHECK(r1->y == 1);
	CHECK(atlas->FindRect("MOVE", 0, 0) == nullptr);   // absent action

	std::remove(png.c_str());
	std::remove(json.c_str());
}

TEST_CASE("ModernSpriteAtlas fails cleanly when the atlas image is missing")
{
	std::string const json = "/tmp/ctp2_atlas_missing.json";
	write_file(json, R"json({
		"source": "X.SPR",
		"atlas": {"png": "does_not_exist.png", "width": 2, "height": 2},
		"actions": [{"name": "IDLE", "width": 1, "height": 1, "num_frames": 1, "facings": 1,
			"frames": [{"facing": 0, "frame": 0, "rect": {"x": 0, "y": 0, "w": 1, "h": 1}}]}]
	})json");

	std::string error;
	ModernSpriteAtlas * atlas = ModernSpriteAtlas::Load(json.c_str(), error);
	CHECK(atlas == nullptr);
	CHECK_FALSE(error.empty());
	std::remove(json.c_str());
}
