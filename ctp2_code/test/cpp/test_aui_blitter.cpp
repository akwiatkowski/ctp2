#include "doctest.h"

#include "ctp/c3.h"
#include "ui/aui_common/aui_blitter.h"
#include "ui/aui_common/aui_dirtylist.h"
#include "ui/aui_sdl/aui_sdlcompat.h"
#include "ui/aui_sdl/aui_sdlsurface.h"
#include "gfx/gfx_utils/pixelutils.h"

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

void write_pixel16(aui_SDLSurface &surface, sint32 x, sint32 y, uint16 value)
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

TEST_CASE("aui_Blitter BevelBlt shades 32bpp SDL surfaces")
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	aui_SDLSurface surface(&errcode, 4, 4, 32, nullptr, FALSE);
	REQUIRE(AUI_SUCCESS(errcode));

	for (sint32 y = 0; y < 4; ++y)
		for (sint32 x = 0; x < 4; ++x)
			write_pixel(surface, x, y, 0x80404040u);

	RECT rect{0, 0, 4, 4};
	aui_Blitter blitter;
	CHECK(blitter.BevelBlt(&surface, &rect, &rect, 1, 0, 0, k_AUI_BLITTER_FLAG_OUT) == AUI_ERRCODE_OK);

	CHECK(read_pixel(surface, 0, 0) == 0x80606060u);
	CHECK(read_pixel(surface, 2, 0) == 0x80585858u);
	CHECK(read_pixel(surface, 3, 1) == 0x80202020u);
	CHECK(read_pixel(surface, 2, 3) == 0x80282828u);
	CHECK(read_pixel(surface, 1, 1) == 0x80404040u);
}

TEST_CASE("aui_Blitter ColorStencilBlt fills 32bpp SDL surfaces through 16bpp stencils")
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	aui_SDLSurface dest(&errcode, 3, 1, 32, nullptr, FALSE);
	REQUIRE(AUI_SUCCESS(errcode));
	aui_SDLSurface stencil(&errcode, 3, 1, 16, nullptr, FALSE);
	REQUIRE(AUI_SUCCESS(errcode));

	write_pixel(dest, 0, 0, 0xFF010203u);
	write_pixel(dest, 1, 0, 0xFF040506u);
	write_pixel(dest, 2, 0, 0xFF070809u);
	write_pixel16(stencil, 0, 0, 0x0000u);
	write_pixel16(stencil, 1, 0, 0x1234u);
	write_pixel16(stencil, 2, 0, 0x0000u);

	RECT rect{0, 0, 3, 1};
	aui_Blitter blitter;
	CHECK(blitter.ColorStencilBlt(&dest, &rect, &stencil, &rect, RGB(0x11, 0x22, 0x33), 0) == AUI_ERRCODE_OK);

	CHECK(read_pixel(dest, 0, 0) == 0xFF112233u);
	CHECK(read_pixel(dest, 1, 0) == 0xFF040506u);
	CHECK(read_pixel(dest, 2, 0) == 0xFF112233u);
}

TEST_CASE("aui_Blitter StretchBlt scales 32bpp SDL surfaces")
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	aui_SDLSurface src(&errcode, 2, 2, 32, nullptr, FALSE);
	REQUIRE(AUI_SUCCESS(errcode));
	aui_SDLSurface dest(&errcode, 4, 4, 32, nullptr, FALSE);
	REQUIRE(AUI_SUCCESS(errcode));

	write_pixel(src, 0, 0, 0xFF100000u);
	write_pixel(src, 1, 0, 0xFF200000u);
	write_pixel(src, 0, 1, 0xFF300000u);
	write_pixel(src, 1, 1, 0xFF400000u);

	RECT srcRect{0, 0, 2, 2};
	RECT destRect{0, 0, 4, 4};
	aui_Blitter blitter;
	CHECK(blitter.StretchBlt(&dest, &destRect, &src, &srcRect, k_AUI_BLITTER_FLAG_COPY) == AUI_ERRCODE_OK);

	CHECK(read_pixel(dest, 0, 0) == 0xFF100000u);
	CHECK(read_pixel(dest, 1, 1) == 0xFF100000u);
	CHECK(read_pixel(dest, 2, 0) == 0xFF200000u);
	CHECK(read_pixel(dest, 0, 2) == 0xFF300000u);
	CHECK(read_pixel(dest, 3, 3) == 0xFF400000u);
}

TEST_CASE("aui_Blitter StretchBlt applies 32bpp chroma keys")
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	aui_SDLSurface src(&errcode, 2, 1, 32, nullptr, FALSE);
	REQUIRE(AUI_SUCCESS(errcode));
	aui_SDLSurface dest(&errcode, 4, 1, 32, nullptr, FALSE);
	REQUIRE(AUI_SUCCESS(errcode));

	write_pixel(src, 0, 0, 0xFFFF00FFu);
	write_pixel(src, 1, 0, 0xFF123456u);
	for (sint32 x = 0; x < 4; ++x)
		write_pixel(dest, x, 0, 0xFF010203u + x);
	CHECK(src.aui_Surface::SetChromaKey(0xFF, 0x00, 0xFF) == 0);

	RECT srcRect{0, 0, 2, 1};
	RECT destRect{0, 0, 4, 1};
	aui_Blitter blitter;
	CHECK(blitter.StretchBlt(&dest, &destRect, &src, &srcRect, k_AUI_BLITTER_FLAG_CHROMAKEY) == AUI_ERRCODE_OK);

	CHECK(read_pixel(dest, 0, 0) == 0xFF010203u);
	CHECK(read_pixel(dest, 1, 0) == 0xFF010204u);
	CHECK(read_pixel(dest, 2, 0) == 0xFF123456u);
	CHECK(read_pixel(dest, 3, 0) == 0xFF123456u);
}

TEST_CASE("aui_Blitter SpanBlt copies 32bpp dirty spans")
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	aui_SDLSurface src(&errcode, 4, 4, 32, nullptr, FALSE);
	REQUIRE(AUI_SUCCESS(errcode));
	aui_SDLSurface dest(&errcode, 4, 4, 32, nullptr, FALSE);
	REQUIRE(AUI_SUCCESS(errcode));

	for (sint32 y = 0; y < 4; ++y)
		for (sint32 x = 0; x < 4; ++x)
		{
			write_pixel(src, x, y, 0xFF100000u + static_cast<uint32>(y * 4 + x));
			write_pixel(dest, x, y, 0xFF000001u);
		}

	aui_DirtyList dirty(TRUE, 4, 4);
	CHECK(dirty.AddRect(1, 1, 3, 3) == AUI_ERRCODE_OK);

	aui_Blitter blitter;
	CHECK(blitter.SpanBlt(&dest, 0, 0, &src, &dirty, k_AUI_BLITTER_FLAG_COPY) == AUI_ERRCODE_OK);

	CHECK(read_pixel(dest, 0, 0) == 0xFF000001u);
	CHECK(read_pixel(dest, 1, 1) == 0xFF100005u);
	CHECK(read_pixel(dest, 2, 1) == 0xFF100006u);
	CHECK(read_pixel(dest, 1, 2) == 0xFF100009u);
	CHECK(read_pixel(dest, 2, 2) == 0xFF10000Au);
	CHECK(read_pixel(dest, 3, 2) == 0xFF000001u);
}

// P11 Stage 2 B: the 565/555 -> ARGB8888 expanders must match the surface's
// byte order (0xAARRGGBB) exactly, since world writers store raw pixels into
// the 32-bit surface. Pin known colours so a byte-order slip fails loudly.
TEST_CASE("pixelutils 565->8888 expander uses ARGB byte order")
{
	CHECK(pixelutils_565to8888(0x0000) == 0xFF000000u);  // black, opaque
	CHECK(pixelutils_565to8888(0xFFFF) == 0xFFFFFFFFu);  // white
	CHECK(pixelutils_565to8888(0xF800) == 0xFFFF0000u);  // pure red  -> R in 0x00FF0000
	CHECK(pixelutils_565to8888(0x07E0) == 0xFF00FF00u);  // pure green
	CHECK(pixelutils_565to8888(0x001F) == 0xFF0000FFu);  // pure blue -> B in low byte
}

TEST_CASE("pixelutils 555->8888 expander uses ARGB byte order")
{
	CHECK(pixelutils_555to8888(0x0000) == 0xFF000000u);
	CHECK(pixelutils_555to8888(0x7FFF) == 0xFFFFFFFFu);  // white (15-bit all set)
	CHECK(pixelutils_555to8888(0x7C00) == 0xFFFF0000u);  // pure red
	CHECK(pixelutils_555to8888(0x03E0) == 0xFF00FF00u);  // pure green
	CHECK(pixelutils_555to8888(0x001F) == 0xFF0000FFu);  // pure blue
}
