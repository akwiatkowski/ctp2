#include "gfx/spritesys/ModernSpriteManifest.h"

#include <fstream>
#include <set>
#include <utility>

namespace
{
constexpr std::streamoff kMaxManifestBytes = 4 * 1024 * 1024;
constexpr int kMaxAtlasDimension = 16384;
constexpr size_t kMaxActions = 32;
constexpr size_t kMaxFrames = 65536;

bool require_string(nlohmann::json const &doc, char const *key, std::string &value, std::string &error)
{
	if (!doc.contains(key) || !doc[key].is_string())
	{
		error = std::string(key) + " must be a string";
		return false;
	}
	value = doc[key].get<std::string>();
	if (value.empty())
	{
		error = std::string(key) + " must not be empty";
		return false;
	}
	return true;
}

bool require_int(nlohmann::json const &doc, char const *key, int &value, std::string &error)
{
	if (!doc.contains(key) || !doc[key].is_number_integer())
	{
		error = std::string(key) + " must be an integer";
		return false;
	}
	value = doc[key].get<int>();
	return true;
}

} // namespace

bool ModernSpriteManifestParse(nlohmann::json const &doc, ModernSpriteManifest &out, std::string &error)
{
	ModernSpriteManifest parsed;
	if (!doc.is_object())
	{
		error = "manifest must be an object";
		return false;
	}
	if (!require_string(doc, "source", parsed.source, error))
	{
		return false;
	}
	if (!doc.contains("atlas") || !doc["atlas"].is_object())
	{
		error = "atlas must be an object";
		return false;
	}
	nlohmann::json const &atlas = doc["atlas"];
	if (!require_string(atlas, "png", parsed.atlasPng, error)
		|| !require_int(atlas, "width", parsed.atlasWidth, error)
		|| !require_int(atlas, "height", parsed.atlasHeight, error))
	{
		return false;
	}
	if (parsed.atlasWidth <= 0 || parsed.atlasHeight <= 0)
	{
		error = "atlas dimensions must be positive";
		return false;
	}
	if (parsed.atlasWidth > kMaxAtlasDimension || parsed.atlasHeight > kMaxAtlasDimension)
	{
		error = "atlas dimensions exceed supported limit";
		return false;
	}
	if (parsed.atlasPng == "." || parsed.atlasPng == ".."
		|| parsed.atlasPng.find('/') != std::string::npos
		|| parsed.atlasPng.find('\\') != std::string::npos)
	{
		error = "atlas png must be a file name";
		return false;
	}
	if (!doc.contains("actions") || !doc["actions"].is_array())
	{
		error = "actions must be an array";
		return false;
	}
	if (doc["actions"].size() > kMaxActions)
	{
		error = "too many actions";
		return false;
	}
	for (nlohmann::json const &action_doc : doc["actions"])
	{
		if (!action_doc.is_object())
		{
			error = "action must be an object";
			return false;
		}
		ModernSpriteAction action;
		if (!require_string(action_doc, "name", action.name, error)
			|| !require_int(action_doc, "width", action.width, error)
			|| !require_int(action_doc, "height", action.height, error)
			|| !require_int(action_doc, "num_frames", action.numFrames, error)
			|| !require_int(action_doc, "facings", action.facings, error))
		{
			return false;
		}
		if (action.width <= 0 || action.height <= 0 || action.numFrames <= 0 || action.facings <= 0)
		{
			error = "action dimensions, frame count, and facings must be positive";
			return false;
		}
		if (!action_doc.contains("frames") || !action_doc["frames"].is_array())
		{
			error = "frames must be an array";
			return false;
		}
		if (action_doc["frames"].size() > kMaxFrames)
		{
			error = "too many frames";
			return false;
		}
		std::set<int> distinct_frames;
		for (nlohmann::json const &frame_doc : action_doc["frames"])
		{
			if (!frame_doc.is_object())
			{
				error = "frame must be an object";
				return false;
			}
			ModernSpriteFrame frame;
			if (!require_int(frame_doc, "facing", frame.facing, error)
				|| !require_int(frame_doc, "frame", frame.frame, error))
			{
				return false;
			}
			if (!frame_doc.contains("rect") || !frame_doc["rect"].is_object())
			{
				error = "frame rect must be an object";
				return false;
			}
			nlohmann::json const &rect = frame_doc["rect"];
			if (!require_int(rect, "x", frame.rect.x, error)
				|| !require_int(rect, "y", frame.rect.y, error)
				|| !require_int(rect, "w", frame.rect.w, error)
				|| !require_int(rect, "h", frame.rect.h, error))
			{
				return false;
			}
			if (frame.facing < 0 || frame.facing >= action.facings || frame.frame < 0 || frame.frame >= action.numFrames)
			{
				error = "frame index out of action range";
				return false;
			}
			if (frame.rect.x < 0 || frame.rect.y < 0 || frame.rect.w <= 0 || frame.rect.h <= 0
				|| frame.rect.x > parsed.atlasWidth || frame.rect.y > parsed.atlasHeight
				|| frame.rect.w > parsed.atlasWidth - frame.rect.x
				|| frame.rect.h > parsed.atlasHeight - frame.rect.y)
			{
				error = "frame rect exceeds atlas bounds";
				return false;
			}
			distinct_frames.insert(frame.frame);
			action.frames.push_back(frame);
		}
		if (static_cast<int>(distinct_frames.size()) != action.numFrames)
		{
			error = "num_frames does not match distinct frame indices";
			return false;
		}
		parsed.actions.push_back(action);
	}
	out = std::move(parsed);
	return true;
}

bool ModernSpriteManifestLoad(char const *path, ModernSpriteManifest &out, std::string &error)
{
	std::ifstream input(path);
	if (!input)
	{
		error = "could not open manifest";
		return false;
	}
	input.seekg(0, std::ios::end);
	std::streamoff const fileSize = input.tellg();
	if (fileSize < 0 || fileSize > kMaxManifestBytes)
	{
		error = "manifest exceeds size limit";
		return false;
	}
	input.seekg(0, std::ios::beg);
	nlohmann::json doc;
	try
	{
		input >> doc;
	}
	catch (nlohmann::json::exception const &exc)
	{
		error = exc.what();
		return false;
	}
	return ModernSpriteManifestParse(doc, out, error);
}
