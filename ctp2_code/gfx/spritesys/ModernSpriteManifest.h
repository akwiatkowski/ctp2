#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

struct ModernSpriteRect
{
	int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;
};

struct ModernSpriteFrame
{
	int facing = 0;
	int frame = 0;
	ModernSpriteRect rect;
};

struct ModernSpriteAction
{
	std::string name;
	int width = 0;
	int height = 0;
	int numFrames = 0;
	int facings = 0;
	std::vector<ModernSpriteFrame> frames;
};

struct ModernSpriteManifest
{
	std::string source;
	std::string atlasPng;
	int atlasWidth = 0;
	int atlasHeight = 0;
	std::vector<ModernSpriteAction> actions;
};

bool ModernSpriteManifestParse(nlohmann::json const &doc, ModernSpriteManifest &out, std::string &error);
