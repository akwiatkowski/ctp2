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
