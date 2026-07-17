#include "doctest.h"

#include "ctp/c3.h"
#include "gfx/spritesys/ModernSpriteAtlas.h"
#include "gfx/spritesys/Sprite.h"           // k_BIT_DRAWFLAGS_*
#include "gfx/gfx_utils/pixelutils.h"        // reference effect helpers
#include "ui/aui_sdl/aui_sdlsurface.h"

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

namespace {
uint32_t read_px(aui_SDLSurface & s, int x, int y)
{
	RECT rect{x, y, x + 1, y + 1};
	LPVOID p = nullptr;
	REQUIRE(s.Lock(&rect, &p, 0) == AUI_ERRCODE_OK);
	uint32_t v = 0;
	std::memcpy(&v, p, sizeof(v));
	REQUIRE(s.Unlock(p) == AUI_ERRCODE_OK);
	return v;
}
} // namespace

TEST_CASE("ModernSpriteAtlas::Blit composites a frame with binary alpha")
{
	std::string const png  = "/tmp/ctp2_atlas_blit.png";
	std::string const json = "/tmp/ctp2_atlas_blit.json";

	// 2x2: red(opaque), transparent / green(opaque), blue(opaque).
	write_png(png, 2, 2, {0xFF,0,0,0xFF,  0,0,0,0,
	                      0,0xFF,0,0xFF,  0,0,0xFF,0xFF});
	write_file(json, R"json({
		"source": "GU.SPR",
		"atlas": {"png": "ctp2_atlas_blit.png", "width": 2, "height": 2},
		"actions": [{"name": "IDLE", "width": 2, "height": 2, "num_frames": 1, "facings": 1,
			"frames": [{"facing": 0, "frame": 0, "rect": {"x": 0, "y": 0, "w": 2, "h": 2}}]}]
	})json");

	std::string error;
	std::unique_ptr<ModernSpriteAtlas> atlas(ModernSpriteAtlas::Load(json.c_str(), error));
	REQUIRE(atlas != nullptr);

	AUI_ERRCODE ec = AUI_ERRCODE_OK;
	aui_SDLSurface dest(&ec, 4, 4, 32, nullptr, FALSE);
	REQUIRE(AUI_SUCCESS(ec));
	// Pre-fill with a sentinel so we can see what the blit did (and didn't) touch.
	for (int y = 0; y < 4; ++y)
		for (int x = 0; x < 4; ++x) {
			RECT r{x, y, x + 1, y + 1};
			LPVOID p = nullptr;
			REQUIRE(dest.Lock(&r, &p, 0) == AUI_ERRCODE_OK);
			uint32_t s = 0xFF010203u; std::memcpy(p, &s, sizeof(s));
			REQUIRE(dest.Unlock(p) == AUI_ERRCODE_OK);
		}

	CHECK(atlas->Blit(&dest, "IDLE", 0, 0, 1, 1));

	CHECK(read_px(dest, 0, 0) == 0xFF010203u);   // outside the blit: untouched
	CHECK(read_px(dest, 1, 1) == 0xFFFF0000u);   // red
	CHECK(read_px(dest, 2, 1) == 0xFF010203u);   // transparent atlas pixel: skipped
	CHECK(read_px(dest, 1, 2) == 0xFF00FF00u);   // green
	CHECK(read_px(dest, 2, 2) == 0xFF0000FFu);   // blue

	CHECK_FALSE(atlas->Blit(&dest, "MOVE", 0, 0, 0, 0));  // unknown frame

	std::remove(png.c_str());
	std::remove(json.c_str());
}

TEST_CASE("ModernSpriteAtlas::Blit mirror flips columns (reversed facings 5-8)")
{
	std::string const png  = "/tmp/ctp2_atlas_mirror.png";
	std::string const json = "/tmp/ctp2_atlas_mirror.json";

	// A 2x1 asymmetric frame: red(opaque) | blue(opaque). A horizontal flip
	// must swap the two columns.
	write_png(png, 2, 1, {0xFF,0,0,0xFF,  0,0,0xFF,0xFF});
	write_file(json, R"json({
		"source": "GU.SPR",
		"atlas": {"png": "ctp2_atlas_mirror.png", "width": 2, "height": 1},
		"actions": [{"name": "IDLE", "width": 2, "height": 1, "num_frames": 1, "facings": 1,
			"frames": [{"facing": 0, "frame": 0, "rect": {"x": 0, "y": 0, "w": 2, "h": 1}}]}]
	})json");

	std::string error;
	std::unique_ptr<ModernSpriteAtlas> atlas(ModernSpriteAtlas::Load(json.c_str(), error));
	REQUIRE(atlas != nullptr);

	AUI_ERRCODE ec = AUI_ERRCODE_OK;
	aui_SDLSurface dest(&ec, 2, 1, 32, nullptr, FALSE);
	REQUIRE(AUI_SUCCESS(ec));

	// Not mirrored: red then blue, left to right.
	CHECK(atlas->Blit(&dest, "IDLE", 0, 0, 0, 0, 0, 0, /*mirror*/false));
	CHECK(read_px(dest, 0, 0) == 0xFFFF0000u);   // red
	CHECK(read_px(dest, 1, 0) == 0xFF0000FFu);   // blue

	// Mirrored: columns swap -> blue then red.
	CHECK(atlas->Blit(&dest, "IDLE", 0, 0, 0, 0, 0, 0, /*mirror*/true));
	CHECK(read_px(dest, 0, 0) == 0xFF0000FFu);   // blue
	CHECK(read_px(dest, 1, 0) == 0xFFFF0000u);   // red

	std::remove(png.c_str());
	std::remove(json.c_str());
}

TEST_CASE("ModernSpriteAtlas::Blit applies the per-pixel draw flags in 8888")
{
	std::string const png  = "/tmp/ctp2_atlas_flags.png";
	std::string const json = "/tmp/ctp2_atlas_flags.json";

	// A single opaque, non-grey source pixel so shadow/desaturate/blend each
	// produce a distinct, checkable result.
	uint8_t const R = 0x40, G = 0x80, B = 0xC0;
	write_png(png, 1, 1, {R, G, B, 0xFF});
	write_file(json, R"json({
		"source": "GU.SPR",
		"atlas": {"png": "ctp2_atlas_flags.png", "width": 1, "height": 1},
		"actions": [{"name": "IDLE", "width": 1, "height": 1, "num_frames": 1, "facings": 1,
			"frames": [{"facing": 0, "frame": 0, "rect": {"x": 0, "y": 0, "w": 1, "h": 1}}]}]
	})json");

	std::string error;
	std::unique_ptr<ModernSpriteAtlas> atlas(ModernSpriteAtlas::Load(json.c_str(), error));
	REQUIRE(atlas != nullptr);

	Pixel32 const src = 0xFF000000u | (uint32_t(R) << 16) | (uint32_t(G) << 8) | B;

	// Each case re-fills the 1x1 dest with a known background, blits with one
	// flag, and asserts the pixel equals the reference pixelutils_*8888 result —
	// proving the effect runs in full 8888 straight from the atlas RGB (no 565
	// round-trip) and matches the legacy weighting.
	auto fill = [](aui_SDLSurface & s, uint32_t v) {
		RECT r{0, 0, 1, 1};
		LPVOID p = nullptr;
		REQUIRE(s.Lock(&r, &p, 0) == AUI_ERRCODE_OK);
		std::memcpy(p, &v, sizeof(v));
		REQUIRE(s.Unlock(p) == AUI_ERRCODE_OK);
	};

	AUI_ERRCODE ec = AUI_ERRCODE_OK;
	aui_SDLSurface dest(&ec, 1, 1, 32, nullptr, FALSE);
	REQUIRE(AUI_SUCCESS(ec));

	SUBCASE("transparency blends over the destination")
	{
		uint32_t const bg = 0xFF102030u;
		uint16_t const t  = 16;                 // half, in the /32 weighting
		fill(dest, bg);
		CHECK(atlas->Blit(&dest, "IDLE", 0, 0, 0, 0, t, k_BIT_DRAWFLAGS_TRANSPARENCY));
		CHECK(read_px(dest, 0, 0) == pixelutils_BlendFast8888(src, bg, t));
	}
	SUBCASE("fog shadows the sprite")
	{
		fill(dest, 0xFF102030u);
		CHECK(atlas->Blit(&dest, "IDLE", 0, 0, 0, 0, 0, k_BIT_DRAWFLAGS_FOGGED));
		CHECK(read_px(dest, 0, 0) == pixelutils_Shadow8888(src));
	}
	SUBCASE("desaturate greys the sprite")
	{
		fill(dest, 0xFF102030u);
		CHECK(atlas->Blit(&dest, "IDLE", 0, 0, 0, 0, 0, k_BIT_DRAWFLAGS_DESATURATED));
		CHECK(read_px(dest, 0, 0) == pixelutils_Desaturate8888(src));
	}
	SUBCASE("transparency wins when combined with fog (legacy precedence)")
	{
		uint32_t const bg = 0xFF102030u;
		uint16_t const t  = 8;
		fill(dest, bg);
		CHECK(atlas->Blit(&dest, "IDLE", 0, 0, 0, 0, t,
		                  k_BIT_DRAWFLAGS_TRANSPARENCY | k_BIT_DRAWFLAGS_FOGGED));
		CHECK(read_px(dest, 0, 0) == pixelutils_BlendFast8888(src, bg, t));
	}

	std::remove(png.c_str());
	std::remove(json.c_str());
}
