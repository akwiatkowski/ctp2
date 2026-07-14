#include "doctest.h"

#include "ctp/c3.h"
#include "ui/aui_common/aui_blitter.h"
#include "ui/aui_sdl/aui_sdlcompat.h"
#include "ui/aui_sdl/aui_sdlsurface.h"

#include <cstring>

namespace {

uint32 read_pixel(aui_SDLSurface &surface, sint32 x, sint32 y)
{
	RECT rect{x, y, x + 1, y + 1};
	LPVOID pixel = nullptr;
	CHECK(surface.Lock(&rect, &pixel, 0) == AUI_ERRCODE_OK);
	uint32 value = 0;
	std::memcpy(&value, pixel, sizeof(value));
	CHECK(surface.Unlock(pixel) == AUI_ERRCODE_OK);
	return value;
}

void write_pixel(aui_SDLSurface &surface, sint32 x, sint32 y, uint32 value)
{
	RECT rect{x, y, x + 1, y + 1};
	LPVOID pixel = nullptr;
	CHECK(surface.Lock(&rect, &pixel, 0) == AUI_ERRCODE_OK);
	std::memcpy(pixel, &value, sizeof(value));
	CHECK(surface.Unlock(pixel) == AUI_ERRCODE_OK);
}

}

TEST_CASE("aui_Blitter ColorBlt fills 32bpp SDL surfaces")
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	aui_SDLSurface surface(&errcode, 4, 3, 32, nullptr, FALSE);
	REQUIRE(AUI_SUCCESS(errcode));
	REQUIRE(surface.BitsPerPixel() == 32);

	RECT rect{1, 1, 3, 3};
	aui_Blitter blitter;
	CHECK(blitter.ColorBlt(&surface, &rect, RGB(0x11, 0x22, 0x33), 0) == AUI_ERRCODE_OK);

	CHECK(read_pixel(surface, 0, 0) == 0);
	CHECK(read_pixel(surface, 1, 1) == 0xFF112233u);
	CHECK(read_pixel(surface, 2, 1) == 0xFF112233u);
	CHECK(read_pixel(surface, 1, 2) == 0xFF112233u);
	CHECK(read_pixel(surface, 2, 2) == 0xFF112233u);
}

TEST_CASE("aui_Surface SetChromaKey maps RGB to 32bpp ARGB")
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	aui_SDLSurface surface(&errcode, 1, 1, 32, nullptr, FALSE);
	REQUIRE(AUI_SUCCESS(errcode));

	CHECK(surface.aui_Surface::SetChromaKey(0x11, 0x22, 0x33) == 0);
	CHECK(surface.GetChromaKey() == 0xFF112233u);
}

TEST_CASE("aui_Blitter Blt copies 32bpp SDL surfaces")
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	aui_SDLSurface src(&errcode, 3, 2, 32, nullptr, FALSE);
	REQUIRE(AUI_SUCCESS(errcode));
	aui_SDLSurface dest(&errcode, 4, 3, 32, nullptr, FALSE);
	REQUIRE(AUI_SUCCESS(errcode));

	write_pixel(src, 1, 0, 0xFF102030u);
	write_pixel(src, 2, 0, 0xFF405060u);
	write_pixel(src, 1, 1, 0xFF708090u);
	write_pixel(src, 2, 1, 0xFFA0B0C0u);

	RECT srcRect{1, 0, 3, 2};
	aui_Blitter blitter;
	CHECK(blitter.Blt(&dest, 1, 1, &src, &srcRect, k_AUI_BLITTER_FLAG_COPY) == AUI_ERRCODE_OK);

	CHECK(read_pixel(dest, 0, 0) == 0);
	CHECK(read_pixel(dest, 1, 1) == 0xFF102030u);
	CHECK(read_pixel(dest, 2, 1) == 0xFF405060u);
	CHECK(read_pixel(dest, 1, 2) == 0xFF708090u);
	CHECK(read_pixel(dest, 2, 2) == 0xFFA0B0C0u);
}

TEST_CASE("aui_Blitter Blt applies 32bpp chroma keys")
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	aui_SDLSurface src(&errcode, 2, 1, 32, nullptr, FALSE);
	REQUIRE(AUI_SUCCESS(errcode));
	aui_SDLSurface dest(&errcode, 2, 1, 32, nullptr, FALSE);
	REQUIRE(AUI_SUCCESS(errcode));

	write_pixel(src, 0, 0, 0xFFFF00FFu);
	write_pixel(src, 1, 0, 0xFF123456u);
	write_pixel(dest, 0, 0, 0xFF010203u);
	write_pixel(dest, 1, 0, 0xFF040506u);
	CHECK(src.aui_Surface::SetChromaKey(0xFF, 0x00, 0xFF) == 0);

	RECT srcRect{0, 0, 2, 1};
	aui_Blitter blitter;
	CHECK(blitter.Blt(&dest, 0, 0, &src, &srcRect, k_AUI_BLITTER_FLAG_CHROMAKEY) == AUI_ERRCODE_OK);

	CHECK(read_pixel(dest, 0, 0) == 0xFF010203u);
	CHECK(read_pixel(dest, 1, 0) == 0xFF123456u);
}

TEST_CASE("aui_Blitter TileBlt repeats 32bpp SDL surfaces")
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	aui_SDLSurface src(&errcode, 2, 2, 32, nullptr, FALSE);
	REQUIRE(AUI_SUCCESS(errcode));
	aui_SDLSurface dest(&errcode, 5, 3, 32, nullptr, FALSE);
	REQUIRE(AUI_SUCCESS(errcode));

	write_pixel(src, 0, 0, 0xFF100000u);
	write_pixel(src, 1, 0, 0xFF200000u);
	write_pixel(src, 0, 1, 0xFF300000u);
	write_pixel(src, 1, 1, 0xFF400000u);

	RECT srcRect{0, 0, 2, 2};
	RECT destRect{1, 0, 5, 3};
	aui_Blitter blitter;
	CHECK(blitter.TileBlt(&dest, &destRect, &src, &srcRect, 0, 0, k_AUI_BLITTER_FLAG_COPY) == AUI_ERRCODE_OK);

	CHECK(read_pixel(dest, 0, 0) == 0);
	CHECK(read_pixel(dest, 1, 0) == 0xFF100000u);
	CHECK(read_pixel(dest, 2, 0) == 0xFF200000u);
	CHECK(read_pixel(dest, 3, 0) == 0xFF100000u);
	CHECK(read_pixel(dest, 4, 0) == 0xFF200000u);
	CHECK(read_pixel(dest, 1, 1) == 0xFF300000u);
	CHECK(read_pixel(dest, 2, 1) == 0xFF400000u);
	CHECK(read_pixel(dest, 1, 2) == 0xFF100000u);
	CHECK(read_pixel(dest, 4, 2) == 0xFF200000u);
}

TEST_CASE("aui_Blitter TileBlt applies 32bpp chroma keys")
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	aui_SDLSurface src(&errcode, 2, 1, 32, nullptr, FALSE);
	REQUIRE(AUI_SUCCESS(errcode));
	aui_SDLSurface dest(&errcode, 4, 1, 32, nullptr, FALSE);
	REQUIRE(AUI_SUCCESS(errcode));

	write_pixel(src, 0, 0, 0xFFFF00FFu);
	write_pixel(src, 1, 0, 0xFFABCDEFu);
	for (sint32 x = 0; x < 4; ++x)
		write_pixel(dest, x, 0, 0xFF010203u + x);
	CHECK(src.aui_Surface::SetChromaKey(0xFF, 0x00, 0xFF) == 0);

	RECT srcRect{0, 0, 2, 1};
	RECT destRect{0, 0, 4, 1};
	aui_Blitter blitter;
	CHECK(blitter.TileBlt(&dest, &destRect, &src, &srcRect, 0, 0, k_AUI_BLITTER_FLAG_CHROMAKEY) == AUI_ERRCODE_OK);

	CHECK(read_pixel(dest, 0, 0) == 0xFF010203u);
	CHECK(read_pixel(dest, 1, 0) == 0xFFABCDEFu);
	CHECK(read_pixel(dest, 2, 0) == 0xFF010205u);
	CHECK(read_pixel(dest, 3, 0) == 0xFFABCDEFu);
}
