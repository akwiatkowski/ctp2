#include "doctest.h"

#include "gfx/spritesys/ModernSpriteManifest.h"

#include <cstdio>
#include <fstream>

TEST_CASE("modern sprite manifest parser accepts atlas rects")
{
	nlohmann::json doc = {
		{"source", "GU04.SPR"},
		{"atlas", {{"png", "GU04.png"}, {"width", 64}, {"height", 32}}},
		{"actions", nlohmann::json::array({{
			{"name", "MOVE"},
			{"width", 16},
			{"height", 16},
			{"num_frames", 2},
			{"facings", 1},
			{"frames", nlohmann::json::array({
				{{"facing", 0}, {"frame", 0}, {"rect", {{"x", 0}, {"y", 0}, {"w", 16}, {"h", 16}}}},
				{{"facing", 0}, {"frame", 1}, {"rect", {{"x", 16}, {"y", 0}, {"w", 16}, {"h", 16}}}},
			})},
		}})},
	};

	ModernSpriteManifest manifest;
	std::string error;
	CHECK(ModernSpriteManifestParse(doc, manifest, error));
	CHECK(error.empty());
	CHECK(manifest.source == "GU04.SPR");
	CHECK(manifest.atlasPng == "GU04.png");
	REQUIRE(manifest.actions.size() == 1);
	CHECK(manifest.actions[0].frames.size() == 2);
}

TEST_CASE("modern sprite manifest parser rejects out-of-bounds rects")
{
	nlohmann::json doc = {
		{"source", "GU04.SPR"},
		{"atlas", {{"png", "GU04.png"}, {"width", 16}, {"height", 16}}},
		{"actions", nlohmann::json::array({{
			{"name", "MOVE"},
			{"width", 16},
			{"height", 16},
			{"num_frames", 1},
			{"facings", 1},
			{"frames", nlohmann::json::array({
				{{"facing", 0}, {"frame", 0}, {"rect", {{"x", 8}, {"y", 0}, {"w", 16}, {"h", 16}}}},
			})},
		}})},
	};

	ModernSpriteManifest manifest;
	std::string error;
	CHECK_FALSE(ModernSpriteManifestParse(doc, manifest, error));
	CHECK(error == "frame rect exceeds atlas bounds");
}

TEST_CASE("modern sprite manifest parser rejects unsafe atlas paths")
{
	nlohmann::json doc = {
		{"source", "GU04.SPR"},
		{"atlas", {{"png", "../GU04.png"}, {"width", 16}, {"height", 16}}},
		{"actions", nlohmann::json::array()},
	};

	ModernSpriteManifest manifest;
	std::string error;
	CHECK_FALSE(ModernSpriteManifestParse(doc, manifest, error));
	CHECK(error == "atlas png must be a file name");
}

TEST_CASE("modern sprite manifest parser rejects oversized atlases")
{
	nlohmann::json doc = {
		{"source", "GU04.SPR"},
		{"atlas", {{"png", "GU04.png"}, {"width", 16385}, {"height", 16}}},
		{"actions", nlohmann::json::array()},
	};

	ModernSpriteManifest manifest;
	std::string error;
	CHECK_FALSE(ModernSpriteManifestParse(doc, manifest, error));
	CHECK(error == "atlas dimensions exceed supported limit");
}

TEST_CASE("modern sprite manifest loader reads validated json files")
{
	char const *path = "/tmp/ctp2_modern_sprite_manifest_test.json";
	{
		std::ofstream out(path);
		out << R"({
			"source": "GU04.SPR",
			"atlas": {"png": "GU04.png", "width": 16, "height": 16},
			"actions": [{
				"name": "MOVE",
				"width": 16,
				"height": 16,
				"num_frames": 1,
				"facings": 1,
				"frames": [{"facing": 0, "frame": 0, "rect": {"x": 0, "y": 0, "w": 16, "h": 16}}]
			}]
		})";
	}

	ModernSpriteManifest manifest;
	std::string error;
	CHECK(ModernSpriteManifestLoad(path, manifest, error));
	CHECK(error.empty());
	CHECK(manifest.atlasWidth == 16);
	std::remove(path);
}

TEST_CASE("modern sprite manifest loader rejects oversized files")
{
	char const *path = "/tmp/ctp2_modern_sprite_manifest_oversized.json";
	{
		std::ofstream out(path, std::ios::binary);
		out.seekp(4 * 1024 * 1024);
		out.put('\n');
	}

	ModernSpriteManifest manifest;
	std::string error;
	CHECK_FALSE(ModernSpriteManifestLoad(path, manifest, error));
    CHECK(error.find("exceeds size limit") != std::string::npos);
	std::remove(path);
}

// Verbatim output of `spr_export.py --atlas` on the real GG012.SPR good
// sprite. It carries fields the synthetic cases above omit (version,
// source_fingerprint, type, sprite_type, hot_points); the parser must accept
// real converter output and ignore the extra keys. Regenerate with:
//   tools/assets/spr_export.py --atlas -o <dir> ctp2_data/.../sprites/GG012.SPR
TEST_CASE("modern sprite manifest loader accepts real spr_export output")
{
	char const *path = "/tmp/ctp2_modern_sprite_manifest_real.json";
	{
		std::ofstream out(path);
		out << R"json({
  "source": "GG012.SPR",
  "version": "v0 (0x00010003, v13 layout)",
  "source_fingerprint": "59e9f0bf2832e619",
  "type": "GOOD",
  "actions": [
    {
      "name": "IDLE",
      "sprite_type": "NORMAL",
      "width": 96,
      "height": 72,
      "num_frames": 1,
      "facings": 1,
      "hot_points": [[49, 47]],
      "frames": [
        {"facing": 0, "frame": 0, "rect": {"x": 0, "y": 0, "w": 96, "h": 72}}
      ]
    }
  ],
  "atlas": {"png": "GG012.png", "width": 96, "height": 72}
})json";
	}

	ModernSpriteManifest manifest;
	std::string error;
	CHECK(ModernSpriteManifestLoad(path, manifest, error));
	CHECK(error.empty());
	CHECK(manifest.source == "GG012.SPR");
	CHECK(manifest.atlasPng == "GG012.png");
	CHECK(manifest.atlasWidth == 96);
	CHECK(manifest.atlasHeight == 72);
	REQUIRE(manifest.actions.size() == 1);
	CHECK(manifest.actions[0].name == "IDLE");
	CHECK(manifest.actions[0].numFrames == 1);
	CHECK(manifest.actions[0].facings == 1);
	REQUIRE(manifest.actions[0].frames.size() == 1);
	CHECK(manifest.actions[0].frames[0].rect.w == 96);
	std::remove(path);
}
