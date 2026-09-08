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
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <stdexcept>
#include <unistd.h>
#include "gfx/spritesys/Sprite.h"
#include "gfx/spritesys/Anim.h"
#include "gfx/spritesys/UnitSpriteGroup.h"
#include "gfx/spritesys/GoodSpriteGroup.h"
#include "gfx/spritesys/EffectSpriteGroup.h"
#include <memory>
#include <vector>

namespace {

// DeCompressData_LZW1 is protected; expose it. The ctor only stores the
// filename (opens nothing), so a bare instance is cheap and file-free.
struct LZW1Access : SpriteFile
{
	LZW1Access() : SpriteFile("lzw1-doctest") {}
	using SpriteFile::DeCompressData_LZW1;
    using SpriteFile::DeCompressData_Default;
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

TEST_CASE("SpriteFile rejects truncated or out-of-bounds compressed streams")
{
    LZW1Access sf;
    for (auto stream : std::vector<std::vector<uint8>>{
        {}, {0}, {0,0,0,0,1}, // short header/control
        {1,0,0,0,'a','b'}, // copy payload larger than declared output
        {0,0,0,0,1,0,0,0}, // zero-distance reference
        {0,0,0,0,1,0,0,3}, // reference before output begins
        {0,0,0,0,0,0,'a','b'}, // literal exceeds output
        {0,0,0,0,2,0,'a',0x0f,1}, // oversized back-reference
        {2,0,0,0,'a'}, // invalid mode
    }) {
        CHECK_THROWS_AS(sf.DeCompressData_LZW1(stream.data(), stream.size(), 1), std::runtime_error);
    }
    uint8 raw[] = {'a','b'};
    CHECK_THROWS_AS(sf.DeCompressData_Default(raw, 1, 2), std::runtime_error);
    CHECK_THROWS_AS(sf.DeCompressData_LZW1(raw, 2, size_t(-1)), std::runtime_error);
}

namespace {
void u16(std::vector<uint8> &v, uint16 n) { v.push_back(n & 255); v.push_back(n >> 8); }
void u32(std::vector<uint8> &v, uint32 n) { u16(v, n & 65535); u16(v, n >> 16); }
std::vector<uint8> plainSprite()
{
    std::vector<uint8> v;
    for (uint32 n : {uint32(k_SPRITEFILE_TAG), uint32(k_SPRITEFILE_VERSION1),
                     uint32(SPRITEFILETYPE_PLAIN), 0u, 0u}) u32(v, n);
    u16(v, 2); u16(v, 2); // width, height
    u32(v, 0); u32(v, 0); // hot point
    u16(v, 0); u16(v, 1); // first frame, frame count
    u32(v, 6); u32(v, 4); // full and mini frame sizes
    u16(v, 0); u16(v, 0xffff); u16(v, 0xffff); // empty rows
    u16(v, 0); u16(v, 0xffff);
    u16(v, 0); u16(v, 1); u16(v, 10); u16(v, 0); // animation
    u16(v, 0); u32(v, 0); u32(v, 0); u16(v, 0);
    return v;
}
struct SpriteInput {
    char path[64] = "/tmp/ctp2-sprite-input-XXXXXX";
    explicit SpriteInput(std::vector<uint8> const &bytes) {
        int fd = mkstemp(path);
        if (fd < 0) throw std::runtime_error("cannot create sprite fixture");
        close(fd);
        std::ofstream output(path, std::ios::binary);
        output << std::string(bytes.begin(), bytes.end());
        if (!output) throw std::runtime_error("cannot write sprite fixture");
    }
    ~SpriteInput() { std::remove(path); }
};
bool readPlain(std::vector<uint8> const &bytes)
{
    SpriteInput input(bytes);
    SpriteFile file(input.path);
    SPRITEFILETYPE type;
    if (file.Open(&type) != SPRITEFILEERR_OK) return false;
    Sprite *sprite = nullptr;
    Anim *animation = nullptr;
    auto result = file.Read(&sprite, &animation);
    std::unique_ptr<Sprite> ownedSprite(sprite);
    std::unique_ptr<Anim> ownedAnimation(animation);
    return result == SPRITEFILEERR_OK;
}
}

TEST_CASE("SpriteFile rejects truncated files, frame counts and row offsets")
{
    auto valid = plainSprite();
    REQUIRE(readPlain(valid));
    for (size_t length = 0; length < valid.size(); ++length) {
        INFO("truncation at byte ", length);
        CHECK_FALSE(readPlain(std::vector<uint8>(valid.begin(), valid.begin() + length)));
    }
    for (auto count : {0u, 801u, 65535u}) {
        auto bad = valid;
        bad[34] = count & 255; bad[35] = count >> 8;
        CHECK_FALSE(readPlain(bad));
    }
    auto bad = valid;
    bad[46] = 0xfe; // full-frame row points beyond its six-byte payload
    CHECK_FALSE(readPlain(bad));
    bad = valid;
    bad[36] = bad[37] = bad[38] = bad[39] = 255; // declared 4 GiB frame
    CHECK_FALSE(readPlain(bad));
}

TEST_CASE("SpriteFile loads installed legacy corpus" * doctest::skip())
{
    auto directory = std::getenv("CTP2_SPR_CORPUS");
    REQUIRE(directory != nullptr);
    size_t count = 0;
    for (auto const &entry : std::filesystem::directory_iterator(directory)) {
        if (entry.path().extension() != ".SPR" && entry.path().extension() != ".spr") continue;
        INFO(entry.path().string());
        for (int mode : {0, 1, 2}) {
            SpriteFile file(std::filesystem::absolute(entry.path()).c_str());
            SPRITEFILETYPE type;
            REQUIRE(file.Open(&type) == SPRITEFILEERR_OK);
            if (type == SPRITEFILETYPE_UNIT) {
                UnitSpriteGroup group(GROUPTYPE_UNIT);
                REQUIRE((mode == 2 ? file.ReadIndexed(&group, static_cast<GAME_ACTION>(UNITACTION_MOVE)) :
                         mode == 1 ? file.ReadBasic(&group) : file.ReadFull(&group)) == SPRITEFILEERR_OK);
            } else if (type == SPRITEFILETYPE_GOOD) {
                GoodSpriteGroup group(GROUPTYPE_GOOD);
                REQUIRE((mode == 2 ? file.ReadIndexed(&group, static_cast<GAME_ACTION>(GOODACTION_IDLE)) :
                         mode == 1 ? file.ReadBasic(&group) : file.ReadFull(&group)) == SPRITEFILEERR_OK);
            } else if (type == SPRITEFILETYPE_EFFECT) {
                EffectSpriteGroup group(GROUPTYPE_EFFECT);
                REQUIRE(file.Read(&group) == SPRITEFILEERR_OK);
            } else {
                FAIL("unexpected corpus sprite type");
            }
        }
        ++count;
    }
    REQUIRE(count > 0);
    std::printf("[sprite corpus] loaded %zu files in full, basic and indexed modes\n", count);
}

TEST_CASE("SpriteFile clears partially loaded groups after truncated animation")
{
    auto plain = plainSprite();
    std::vector<uint8> bytes;
    for (uint32 n : {uint32(k_SPRITEFILE_TAG), uint32(k_SPRITEFILE_VERSION1),
                     uint32(SPRITEFILETYPE_GOOD), 0u, 1u}) u32(bytes, n);
    u16(bytes, SPRITETYPE_NORMAL);
    bytes.insert(bytes.end(), plain.begin() + 20, plain.end());
    GoodSpriteGroup group(GROUPTYPE_GOOD);
    auto read = [&]() {
        SpriteInput input(bytes);
        SpriteFile file(input.path);
        SPRITEFILETYPE type;
        REQUIRE(file.Open(&type) == SPRITEFILEERR_OK);
        return file.ReadFull(&group);
    };
    REQUIRE(read() == SPRITEFILEERR_OK);
    auto action = static_cast<GAME_ACTION>(GOODACTION_IDLE);
    REQUIRE(group.GetGroupSprite(action) != nullptr);
    REQUIRE(group.GetAnim(action) != nullptr);
    bytes.pop_back();
    CHECK(read() == SPRITEFILEERR_READERR);
    CHECK(group.GetGroupSprite(action) == nullptr);
    CHECK(group.GetAnim(action) == nullptr);
}
