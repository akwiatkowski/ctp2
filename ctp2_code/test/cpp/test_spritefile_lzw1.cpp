// Engine-side LZW1 decode parity (P11 modern-sprite track). The only v2/LZW1
// .SPR in the shipping data is a single GOOD sprite (GG023); the Python
// converter's decoder is covered by its own self-test + a real --verify pass on
// GG023, but the engine's SpriteFile::DeCompressData_LZW1 had no test at all.
// These crafted streams mirror the Python self-test vectors, proving both
// decoders implement the identical algorithm (SpriteFile.h:57-61).
#include "doctest.h"

#include "ctp/c3.h"
#include "gfx/spritesys/SpriteFile.h"

#include <cstring>
#include <memory>
#include <vector>

namespace {

// DeCompressData_LZW1 is protected; expose it. The ctor only stores the
// filename (opens nothing), so a bare instance is cheap and file-free.
struct LZW1Access : SpriteFile
{
	LZW1Access() : SpriteFile("lzw1-doctest") {}
	using SpriteFile::DeCompressData_LZW1;
};

} // namespace

TEST_CASE("SpriteFile::DeCompressData_LZW1 copy mode returns the raw payload")
{
	LZW1Access sf;
	// [flag=COPY, 0,0,0] + "raw-frame"; copy mode emits the payload verbatim.
	std::vector<uint8> stream = {
		LZW1_FLAG_COPY, 0, 0, 0,
		'r','a','w','-','f','r','a','m','e'
	};
	size_t const actual = 9;   // strlen("raw-frame")
	std::unique_ptr<uint8[]> out(
		sf.DeCompressData_LZW1(stream.data(), stream.size(), actual));
	REQUIRE(out != nullptr);
	CHECK(std::memcmp(out.get(), "raw-frame", actual) == 0);
}

TEST_CASE("SpriteFile::DeCompressData_LZW1 decodes a back-reference to ABCABC")
{
	LZW1Access sf;
	// Three literals 'A','B','C' then a back-reference (offset=3, len=3) -> the
	// three prior bytes repeat, yielding "ABCABC".
	//   control word, LSB-first ops = lit,lit,lit,backref -> 0b1000 = 0x0008.
	//   back-ref byte1 = ((offset & 0xF00) >> 4) | (len - 1) = 0x00 | 0x02 = 0x02
	//   back-ref byte2 = offset & 0xFF                                      = 0x03
	std::vector<uint8> stream = {
		LZW1_FLAG_COMPRESS, 0, 0, 0,
		0x08, 0x00,          // control word (little-endian)
		'A', 'B', 'C',
		0x02, 0x03           // back-reference: offset 3, length 3
	};
	size_t const actual = 6;   // "ABCABC"
	std::unique_ptr<uint8[]> out(
		sf.DeCompressData_LZW1(stream.data(), stream.size(), actual));
	REQUIRE(out != nullptr);
	CHECK(std::memcmp(out.get(), "ABCABC", actual) == 0);
}
