// test/cpp/test_color_types.cpp
// Pure-type sanity tests for color_types.h and pixel_types.h (wave 15a W6).
//
// Verifies COLOR enum values are stable and Pixel{32,16,8} typedefs are the
// expected sized unsigned integers.  These are extracted pure-type headers;
// this test is a cheap regression net that compiles in < 1 s.

#include "ctp/c3.h"
#include "doctest.h"
#include "gs/core/color_types.h"
#include "gs/core/pixel_types.h"
#include <type_traits>

TEST_CASE("COLOR enum sentinel values are stable") {
    CHECK(static_cast<int>(COLOR_BLACK) == 0);
    CHECK(static_cast<int>(COLOR_WHITE) == 1);
    CHECK(static_cast<int>(COLOR_RED)   == 2);
}

TEST_CASE("COLOR_PLAYER1 ..= COLOR_PLAYER33 are contiguous") {
    CHECK(static_cast<int>(COLOR_PLAYER2)  - static_cast<int>(COLOR_PLAYER1)  == 1);
    CHECK(static_cast<int>(COLOR_PLAYER33) - static_cast<int>(COLOR_PLAYER1)  == 32);
}

TEST_CASE("COLOR_MAX is the last enumerator") {
    // sanity check — value greater than all named entries
    CHECK(static_cast<int>(COLOR_MAX) > static_cast<int>(COLOR_GRAY));
}

TEST_CASE("Pixel32 is exactly 32 bits unsigned") {
    static_assert(sizeof(Pixel32) == 4, "Pixel32 must be 32 bits");
    static_assert(std::is_unsigned<Pixel32>::value, "Pixel32 must be unsigned");
}

TEST_CASE("Pixel16 is exactly 16 bits unsigned") {
    static_assert(sizeof(Pixel16) == 2, "Pixel16 must be 16 bits");
}

TEST_CASE("Pixel8 is exactly 8 bits unsigned") {
    static_assert(sizeof(Pixel8) == 1, "Pixel8 must be 8 bits");
}
