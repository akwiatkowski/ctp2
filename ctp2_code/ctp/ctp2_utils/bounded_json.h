#pragma once

#include <istream>
#include <nlohmann/json.hpp>

// File-backed JSON boundaries share size/depth limits before building a DOM.
inline nlohmann::json ReadBoundedJson(std::istream &input, std::streamoff maxBytes)
{
    input.seekg(0, std::ios::end);
    auto size = input.tellg();
    if (size < 0 || size > maxBytes)
        throw nlohmann::json::other_error::create(532, "JSON file exceeds size limit", nullptr);
    input.seekg(0, std::ios::beg);
    return nlohmann::json::parse(input, [](int depth, auto, auto &) {
        if (depth > 64)
            throw nlohmann::json::other_error::create(532, "JSON nesting exceeds 64 levels", nullptr);
        return true;
    });
}
