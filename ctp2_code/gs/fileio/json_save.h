//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : JSON savegame entry points (Phase A scaffold)
//
//----------------------------------------------------------------------------
//
// Replaces the CTP2 binary save format (CivArchive + 18-magic-table +
// USE_FORMAT_67 ifdefs) with a human-readable, gzip-compressed JSON
// format.  See ~/projects/claude/plans/ctp2-json-savegame.md.
//
// This file is the Phase A skeleton — round-trips the 3 leaf types
// (MapPoint, ID, sint32) and a minimal top-level header
// {"magic": "CTP2-JSON", "schema_version": 1}.  Each subsequent phase
// adds to_json/from_json for more classes and extends the top-level
// object.
//
// nlohmann/json free-function ADL pattern: to_json / from_json live
// in the namespace of the type they serialise.  Both MapPoint and ID
// are in the global namespace, so the bridges below are too.
//
//----------------------------------------------------------------------------

#pragma once

#include "ctp2_inttypes.h"
#include <nlohmann/json.hpp>

#include "gs/world/MapPoint.h"
#include "gs/gameobj/ID.h"
#include "gs/gameobj/GameSettings.h"
#include "gs/utility/TurnCnt.h"
#include "gs/utility/RandGen.h"

// Phase B selection block.  SelectedItem (in ui/) is too coupled to
// Army/Unit objects to round-trip until those types are JSON-
// serialisable in Phase E.  Phase B records only the scalar current
// player; the per-player select state, position, and selected
// army/unit arrays land later.
struct SelectionState
{
    sint32 current_player = -1;
};

// Save-file string codec: game memory is Latin-1 (StringDB, fixed name
// buffers, the UI font path); the JSON save stores UTF-8. utf8_safe
// encodes on write (lossless for Latin-1, never invalid UTF-8);
// latin1_safe decodes on load (outside-Latin-1 becomes '?').
std::string utf8_safe(MBCHAR const *src);
std::string latin1_safe(std::string const &utf8);

inline void to_json(nlohmann::json &j, SelectionState const &s)
{
    j = nlohmann::json{{"current_player", s.current_player}};
}

inline void from_json(nlohmann::json const &j, SelectionState &s)
{
    j.at("current_player").get_to(s.current_player);
}

// --- Leaf bridges (global namespace, picked up by nlohmann via ADL) ---

inline void to_json(nlohmann::json &j, MapPointData const &p)
{
    j = nlohmann::json{{"x", p.x}, {"y", p.y}, {"z", p.z}};
}

inline void from_json(nlohmann::json const &j, MapPointData &p)
{
    j.at("x").get_to(p.x);
    j.at("y").get_to(p.y);
    j.at("z").get_to(p.z);
    p.pad = 0;
}

// MapPoint derives from MapPointData and adds no fields — share the
// bridge.  Implemented as a thin forward so nlohmann's ADL finds it
// for both the base and derived type.
inline void to_json(nlohmann::json &j, MapPoint const &p)
{
    to_json(j, static_cast<MapPointData const &>(p));
}

inline void from_json(nlohmann::json const &j, MapPoint &p)
{
    from_json(j, static_cast<MapPointData &>(p));
}

inline void to_json(nlohmann::json &j, ID const &id)
{
    j = id.m_id;
}

inline void from_json(nlohmann::json const &j, ID &id)
{
    id.m_id = j.get<uint32>();
}

// --- Top-level save / load entry points ---

namespace json_save {

// Constants advertising the file format.
constexpr char const *MAGIC          = "CTP2-JSON";
constexpr int         SCHEMA_VERSION = 1;

// Write the current game state to `path` as JSON.  Phase A only
// emits the file header — later phases extend it with world, players,
// pools, trackers, etc.  Returns true on success.
bool SaveJson(char const *path);

// Read JSON from `path`, verify magic + schema_version, populate
// game state.  Phase A only verifies the header.  Returns true on
// success.  Hard-breaks on magic / schema mismatch (Decision #1 in
// the plan: no migrators during migration phases).
bool LoadJson(char const *path);

}  // namespace json_save
