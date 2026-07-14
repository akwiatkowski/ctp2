#include "doctest.h"

#include "gfx/spritesys/ModernSpriteManifest.h"

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
