// test/cpp/test_player_view.cpp
// Ratcheted guard test against ui/ includes in gs/ and ai/.
//
// The clean-architecture migration is moving game-state (gs/) and AI (ai/)
// code off UI header dependencies. We can't yet assert "zero ui/ includes"
// because many call sites are still in flight. Instead we ratchet: the count
// must never grow. When a migration commit reduces the count, the matching
// baseline below should be lowered in the same commit so the ratchet locks
// in the progress.
//
// We track .cpp and .h files separately because they have different migration
// shapes: .cpp files are leaves (touch one file), while .h files are
// transitively included by many callers (touch one file, possibly break N
// callers).
//
// Procedure for migrators:
//   1. Migrate a file (replace ui/<header>.h with the appropriate game_observer
//      / player_view interface, or push the include down into consumers that
//      actually use it).
//   2. Re-run `make test`. The test will MESSAGE that the count dropped below
//      baseline — that's the cue to update the baseline constants below.
//   3. Commit the source change AND the baseline tightening together.

#include "doctest.h"
#include "gs/core/player_view.h"

#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <fstream>
#include <map>
#include <regex>
#include <string>
#include <utility>
#include <vector>

namespace {

// Ratchet baselines — total `#include "ui/..."` lines, summed across files.
// Tighten whenever the migration reduces the actual count.
constexpr std::size_t GS_UI_CPP_BASELINE    = 0;
constexpr std::size_t GS_UI_HEADER_BASELINE = 0;
constexpr std::size_t AI_UI_CPP_BASELINE    = 0;
constexpr std::size_t AI_UI_HEADER_BASELINE = 0;

// Ratchet baselines — total `#include "gfx/..."` lines, summed across files.
// Phase 2 migrated all director_Get() call sites onto render_observer, but many
// gs/ files still include gfx/ headers for type references (UnitActor*,
// SpriteState*, etc.).  These baselines lock the current count so they can
// only shrink as those type refs get pushed behind interfaces or pImpl'd.
// Remaining gs/.cpp gfx leak: 1 file (TileInfo.cpp's GoodActor.h).
// TileInfo legitimately owns a GoodActor with 8 distinct operations
// (construction × 3, copy, archive-load, FullLoad, Serialize, delete).
// Migrating it would need a 6+ function factory for modest benefit —
// documented exception.
//
// Remaining gs/.h gfx leak: 1 file (UnitData.h's UnitActor.h).
// UnitData genuinely owns a shared_ptr<UnitActor> and exposes it via
// Get/SetSpriteState accessors.  The real architectural debt is that
// UnitActor itself mixes game-state (position, ownership, fortified,
// walls, vision) with graphics-state (sprite, animation).  The proper
// fix is splitting UnitActor into a gs-side UnitState + gfx-side
// UnitRenderer — multi-wave work not done yet.
constexpr std::size_t GS_GFX_CPP_BASELINE    = 1;
constexpr std::size_t GS_GFX_HEADER_BASELINE = 1;
constexpr std::size_t AI_GFX_CPP_BASELINE    = 0;
constexpr std::size_t AI_GFX_HEADER_BASELINE = 0;

// Ratchet baselines — total `#include "sound/..."` lines, summed across files.
// Phase 3 migrated all g_soundManager call sites onto audio_observer.
// Remaining includes are for enum types (SOUNDTYPE, MUSICSTYLE, GAMESOUNDS)
// passed as sint32 through audio_observer's signature.  Shrinks as those
// enums move to a neutral header.
constexpr std::size_t GS_SOUND_CPP_BASELINE    = 1;
constexpr std::size_t GS_SOUND_HEADER_BASELINE = 0;
constexpr std::size_t AI_SOUND_CPP_BASELINE    = 0;
constexpr std::size_t AI_SOUND_HEADER_BASELINE = 0;

// robot/ subsystem — pathing + ai backdoor; gs/ historically depends on
// these (civarchive, dynarr) for save/load support, but the surface is
// big and bleeds in transitive includes.  Ratcheting locks the current
// counts so future work can only reduce them.
// Wave 11 moved TileInfo.{h,cpp} from gfx/tilesys/ to gs/world/ — a net
// architectural win (TileInfo is pure data, belongs with the world model).
// TileInfo.cpp's existing dependency on robot/aibackdoor/civarchive.h
// (for save-load Serialize) now counts toward gs/.cpp robot/-include
// count.  Baseline raised by 1 to reflect this.
// Phase F-5: json_save.cpp adds #include "robot/pathing/Path.h" for
// TradeRouteData's embedded Path bridge (+1).
// Phase 0.C-5: removed ~80 dead `#include "robot/aibackdoor/civarchive.h"`
// lines + dead Load/Save(CivArchive&) decls in ai/ headers.  Counts
// dropped: gs .cpp 118->39, gs .h 25->11, ai .cpp 13->10, ai .h 7->3.
constexpr std::size_t GS_ROBOT_CPP_BASELINE    = 37;
constexpr std::size_t GS_ROBOT_HEADER_BASELINE = 10;

// ai/ files include robot/ headers for pathing and backdoor access.
// Ratcheting locks the current counts so future work can only reduce them.
constexpr std::size_t AI_ROBOT_CPP_BASELINE    = 10;
constexpr std::size_t AI_ROBOT_HEADER_BASELINE = 3;

// Ratchet baselines — total `#include "net/..."` lines in gs/ .cpp files.
// gs/ historically depends on net/ for multiplayer and network file I/O.
// Ratcheting locks the current count so future decoupling work can only
// reduce it.
constexpr std::size_t NET_INCLUDES_GS_CPP_BASELINE = 133;

// Ratchet baselines — total `#include "net/..."` lines in ai/ .cpp files.
// ai/ includes net/ headers for multiplayer backdoor and message types.
// Ratcheting locks the current count so future work can only reduce it.
constexpr std::size_t AI_NET_CPP_BASELINE = 9;

// Ratchet baselines — total `#include "ai/..."` lines in gs/ .cpp files.
// gs/ includes ai/ headers for AI backdoor interfaces and strategy types.
// Ratcheting locks the current count so future work can only reduce it.
constexpr std::size_t GS_AI_CPP_BASELINE = 38;

// slic/ is a subset of gs/.  Wave 7 cleared all gfx/ includes from
// slic/; this ratchet locks that at 0 so the SLIC interpreter never
// re-couples to graphics.
constexpr std::size_t SLIC_GFX_CPP_BASELINE    = 0;
constexpr std::size_t SLIC_GFX_HEADER_BASELINE = 0;

// Ratchet baselines — total `#include "gs/database/..."` lines in ai/ .cpp files.
// ai/ includes gs/database/ headers for DB access and schema types.
// Ratcheting locks the current count so future decoupling work can only reduce it.
constexpr std::size_t AI_GS_DATABASE_CPP_BASELINE = 17;

// Ratchet baselines — total `#include "gs/outcom/..."` lines in gs/ .cpp files.
// gs/outcom/ is a deprecated COM wrapper layer; remaining includes are
// technical debt.  Ratcheting locks the current count so it can only shrink.
constexpr std::size_t GS_OUTCOM_CPP_BASELINE = 31;

// Ratchet baselines — total `#include "gs/gameobj/..."` lines in ai/ .h files.
// ai/ headers include gs/gameobj/ for game object type definitions.
// Ratcheting locks the current count so future work can only reduce it.
constexpr std::size_t AI_GS_GAMEOBJ_H_BASELINE = 16;

// gfx/ .cpp historically includes gs/database/ for record types used in
// rendering.  Lock the count.
constexpr std::size_t GFX_GS_DATABASE_CPP_BASELINE = 17;

// net/ .cpp includes gs/gameobj/ for serializing game objects across the
// network.  Lock the count.
constexpr std::size_t NET_GS_GAMEOBJ_CPP_BASELINE = 208;

// ui/ .cpp depends on gs/ broadly — that's architecturally fine.  Lock
// the count anyway to detect new direct couplings.
constexpr std::size_t UI_GS_CPP_BASELINE = 809;  // +5 from ui/aui_ctp2/trade_pool_draw.cpp (relocated TradePool::Draw from gs/gameobj/TradePool.cpp)

struct Violation {
    std::string file;
    std::size_t line;
    std::string text;
};

// POSIX recursive walk for a given extension (".cpp" or ".h").  Avoids C++17
// <filesystem> because the project builds with cpp_std=none and other tests
// do not assume C++17.
void walk_source_files(const std::string& root, const char* extension,
                       std::vector<std::string>& out)
{
    DIR* dir = opendir(root.c_str());
    if (!dir) return;

    const std::size_t ext_len = std::strlen(extension);

    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        if (std::strcmp(ent->d_name, ".") == 0 || std::strcmp(ent->d_name, "..") == 0)
            continue;

        std::string full = root + "/" + ent->d_name;
        struct stat st;
        if (lstat(full.c_str(), &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            walk_source_files(full, extension, out);
        } else if (S_ISREG(st.st_mode)) {
            const std::size_t n = std::strlen(ent->d_name);
            if (n > ext_len &&
                std::strcmp(ent->d_name + n - ext_len, extension) == 0)
            {
                out.push_back(full);
            }
        }
    }
    closedir(dir);
}

// Cached file contents per (root, extension) pair.  Walking
// ctp2_code/gs (~490 files) or ctp2_code/ai (~280 files) and reading
// every line back from disk takes ~3s per pass; with 8+ ratchet test
// cases each previously triggering its own walk+read, the fast suite
// spent ~25s on the gs/ tree alone.  Caching the line contents
// once-per-key collapses that to a single walk+read + N cheap regex
// passes over in-memory strings.
//
// std::map (not unordered_map) because the key is a small pair-of-string
// and we only have ~6 distinct entries; std::map's deterministic
// iteration is convenient for any future debugging dump.
struct FileLines {
    std::string path;
    std::vector<std::string> lines;
};

const std::vector<FileLines>& cached_read(const std::string& root,
                                          const char* extension)
{
    using Key = std::pair<std::string, std::string>;
    static std::map<Key, std::vector<FileLines>> cache;

    Key key{root, std::string(extension)};
    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second;
    }

    std::vector<std::string> file_paths;
    walk_source_files(root, extension, file_paths);

    std::vector<FileLines> result;
    result.reserve(file_paths.size());
    for (auto& path : file_paths) {
        std::ifstream file(path);
        if (!file) continue;

        FileLines fl;
        fl.path = std::move(path);
        std::string line;
        while (std::getline(file, line)) {
            fl.lines.push_back(std::move(line));
        }
        result.push_back(std::move(fl));
    }

    auto inserted = cache.emplace(std::move(key), std::move(result));
    return inserted.first->second;
}

// Cheap heuristic: every ratchet regex in this file is anchored to an
// `#include` line, so we can skip the (expensive) regex_search for any
// line that doesn't contain `#include` at all.  This cuts work by ~95%
// because the vast majority of lines in source files aren't includes.
// std::regex on a typical line takes ~10us; string::find takes ~50ns.
bool line_might_be_include(const std::string& line)
{
    return line.find("#include") != std::string::npos;
}

std::vector<Violation> scan_directory_with_regex(const std::string& root,
                                                  const char* extension,
                                                  const std::regex& include_re)
{
    std::vector<Violation> violations;
    const auto& files = cached_read(root, extension);

    for (const auto& fl : files) {
        std::size_t line_num = 0;
        for (const auto& line : fl.lines) {
            ++line_num;
            if (!line_might_be_include(line)) continue;
            if (std::regex_search(line, include_re)) {
                violations.push_back({fl.path, line_num, line});
            }
        }
    }
    return violations;
}

std::vector<Violation> scan_directory(const std::string& root,
                                      const char* extension)
{
    // ui/netshell/* is the multiplayer network shell, not UI rendering;
    // consumed by gs/fileio/gamefile.h and gs/network code as a legitimate dependency.
    static const std::regex include_ui_re(R"(^\s*#include\s+[<\"]ui/(?!netshell/))");
    return scan_directory_with_regex(root, extension, include_ui_re);
}

std::vector<Violation> scan_directory_gfx(const std::string& root,
                                          const char* extension)
{
    static const std::regex include_gfx_re(R"(^\s*#include\s+[<\"]gfx/)");
    return scan_directory_with_regex(root, extension, include_gfx_re);
}

std::vector<Violation> scan_directory_sound(const std::string& root,
                                            const char* extension)
{
    static const std::regex include_sound_re(R"(^\s*#include\s+[<\"]sound/)");
    return scan_directory_with_regex(root, extension, include_sound_re);
}

std::vector<Violation> scan_directory_robot(const std::string& root,
                                            const char* extension)
{
    static const std::regex include_robot_re(R"(^\s*#include\s+[<\"]robot/)");
    return scan_directory_with_regex(root, extension, include_robot_re);
}

std::vector<Violation> scan_directory_net(const std::string& root,
                                          const char* extension)
{
    static const std::regex include_net_re(R"(^\s*#include\s+[<\"]net/)");
    return scan_directory_with_regex(root, extension, include_net_re);
}

std::vector<Violation> scan_directory_ai(const std::string& root,
                                         const char* extension)
{
    static const std::regex include_ai_re(R"(^\s*#include\s+[<\"]ai/)");
    return scan_directory_with_regex(root, extension, include_ai_re);
}

std::vector<Violation> scan_directory_gs_database(const std::string& root,
                                                   const char* extension)
{
    static const std::regex re(R"(^\s*#include\s+[<\"]gs/database/)");
    return scan_directory_with_regex(root, extension, re);
}

std::vector<Violation> scan_directory_gs_outcom(const std::string& root,
                                                const char* extension)
{
    static const std::regex re(R"(^\s*#include\s+[<\"]gs/outcom/)");
    return scan_directory_with_regex(root, extension, re);
}

std::vector<Violation> scan_directory_gs_gameobj(const std::string& root,
                                                  const char* extension)
{
    static const std::regex re(R"(^\s*#include\s+[<\"]gs/gameobj/)");
    return scan_directory_with_regex(root, extension, re);
}

std::vector<Violation> scan_directory_gs(const std::string& root,
                                          const char* extension)
{
    static const std::regex re(R"(^\s*#include\s+[<\"]gs/)");
    return scan_directory_with_regex(root, extension, re);
}

}  // namespace

// ---------------------------------------------------------------------------
// Ratchet tests — .cpp files
// ---------------------------------------------------------------------------
TEST_CASE("gs/ .cpp ratchet: UI includes must not grow above baseline")
{
    const auto violations = scan_directory("ctp2_code/gs", ".cpp");

    if (violations.size() > GS_UI_CPP_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("gs/ .cpp UI-include count " << violations.size()
             << " exceeds baseline " << GS_UI_CPP_BASELINE
             << ". A new include of ui/<header> has been introduced in a "
                "gs/ source file — revert it or migrate to game_observer / "
                "player_view.");
    } else if (violations.size() < GS_UI_CPP_BASELINE) {
        MESSAGE("gs/ .cpp UI-include count " << violations.size()
                << " < baseline " << GS_UI_CPP_BASELINE
                << " — lower GS_UI_CPP_BASELINE to lock in progress.");
    }
}

TEST_CASE("ai/ .cpp ratchet: UI includes must not grow above baseline")
{
    const auto violations = scan_directory("ctp2_code/ai", ".cpp");

    if (violations.size() > AI_UI_CPP_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("ai/ .cpp UI-include count " << violations.size()
             << " exceeds baseline " << AI_UI_CPP_BASELINE
             << ". A new include of ui/<header> has been introduced in an "
                "ai/ source file — revert it or migrate.");
    } else if (violations.size() < AI_UI_CPP_BASELINE) {
        MESSAGE("ai/ .cpp UI-include count " << violations.size()
                << " < baseline " << AI_UI_CPP_BASELINE
                << " — lower AI_UI_CPP_BASELINE to lock in progress.");
    }
}

// ---------------------------------------------------------------------------
// Ratchet tests — .h files
// ---------------------------------------------------------------------------
TEST_CASE("gs/ .h ratchet: UI includes must not grow above baseline")
{
    const auto violations = scan_directory("ctp2_code/gs", ".h");

    if (violations.size() > GS_UI_HEADER_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("gs/ .h UI-include count " << violations.size()
             << " exceeds baseline " << GS_UI_HEADER_BASELINE
             << ". A new include of ui/<header> has been introduced in a "
                "gs/ header file — revert it or push the include down into "
                "the .cpp files that actually use the symbol.");
    } else if (violations.size() < GS_UI_HEADER_BASELINE) {
        MESSAGE("gs/ .h UI-include count " << violations.size()
                << " < baseline " << GS_UI_HEADER_BASELINE
                << " — lower GS_UI_HEADER_BASELINE to lock in progress.");
    }
}

TEST_CASE("ai/ .h ratchet: UI includes must not grow above baseline")
{
    const auto violations = scan_directory("ctp2_code/ai", ".h");

    if (violations.size() > AI_UI_HEADER_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("ai/ .h UI-include count " << violations.size()
             << " exceeds baseline " << AI_UI_HEADER_BASELINE
             << ". A new include of ui/<header> has been introduced in an "
                "ai/ header file — revert it or push the include down.");
    } else if (violations.size() < AI_UI_HEADER_BASELINE) {
        MESSAGE("ai/ .h UI-include count " << violations.size()
                << " < baseline " << AI_UI_HEADER_BASELINE
                << " — lower AI_UI_HEADER_BASELINE to lock in progress.");
    }
}

// ---------------------------------------------------------------------------
// Ratchet tests — gfx/ includes (Phase 2 lock).
// gs/ files include gfx/ headers for type references (UnitActor*, etc.).
// Phase 2 ended with these baselines; future work pushes them toward 0 via
// pImpl or interface relocation.
// ---------------------------------------------------------------------------
TEST_CASE("gs/ .cpp ratchet: gfx includes must not grow above baseline")
{
    const auto violations = scan_directory_gfx("ctp2_code/gs", ".cpp");

    if (violations.size() > GS_GFX_CPP_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("gs/ .cpp gfx-include count " << violations.size()
             << " exceeds baseline " << GS_GFX_CPP_BASELINE
             << ". A new include of gfx/<header> has been introduced in a "
                "gs/ source file — revert it or use render_observer / "
                "forward declaration.");
    } else if (violations.size() < GS_GFX_CPP_BASELINE) {
        MESSAGE("gs/ .cpp gfx-include count " << violations.size()
                << " < baseline " << GS_GFX_CPP_BASELINE
                << " — lower GS_GFX_CPP_BASELINE to lock in progress.");
    }
}

TEST_CASE("gs/ .h ratchet: gfx includes must not grow above baseline")
{
    const auto violations = scan_directory_gfx("ctp2_code/gs", ".h");

    if (violations.size() > GS_GFX_HEADER_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("gs/ .h gfx-include count " << violations.size()
             << " exceeds baseline " << GS_GFX_HEADER_BASELINE
             << ". A new include of gfx/<header> has been introduced in a "
                "gs/ header file — revert it or forward-declare.");
    } else if (violations.size() < GS_GFX_HEADER_BASELINE) {
        MESSAGE("gs/ .h gfx-include count " << violations.size()
                << " < baseline " << GS_GFX_HEADER_BASELINE
                << " — lower GS_GFX_HEADER_BASELINE to lock in progress.");
    }
}

TEST_CASE("ai/ .cpp ratchet: gfx includes must not grow above baseline")
{
    const auto violations = scan_directory_gfx("ctp2_code/ai", ".cpp");

    if (violations.size() > AI_GFX_CPP_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("ai/ .cpp gfx-include count " << violations.size()
             << " exceeds baseline " << AI_GFX_CPP_BASELINE
             << ". A new include of gfx/<header> has been introduced in an "
                "ai/ source file — revert it or migrate.");
    } else if (violations.size() < AI_GFX_CPP_BASELINE) {
        MESSAGE("ai/ .cpp gfx-include count " << violations.size()
                << " < baseline " << AI_GFX_CPP_BASELINE
                << " — lower AI_GFX_CPP_BASELINE to lock in progress.");
    }
}

TEST_CASE("ai/ .h ratchet: gfx includes must not grow above baseline")
{
    const auto violations = scan_directory_gfx("ctp2_code/ai", ".h");

    if (violations.size() > AI_GFX_HEADER_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("ai/ .h gfx-include count " << violations.size()
             << " exceeds baseline " << AI_GFX_HEADER_BASELINE
             << ". A new include of gfx/<header> has been introduced in an "
                "ai/ header file — revert it or push the include down.");
    } else if (violations.size() < AI_GFX_HEADER_BASELINE) {
        MESSAGE("ai/ .h gfx-include count " << violations.size()
                << " < baseline " << AI_GFX_HEADER_BASELINE
                << " — lower AI_GFX_HEADER_BASELINE to lock in progress.");
    }
}

// ---------------------------------------------------------------------------
// Ratchet tests — sound/ includes (Phase 3 lock).
// gs/ files still include sound/ headers for enum types (SOUNDTYPE,
// MUSICSTYLE, GAMESOUNDS) passed through audio_observer as sint32.
// Shrinks as those enums move to a neutral header.
// ---------------------------------------------------------------------------
TEST_CASE("gs/ .cpp ratchet: sound includes must not grow above baseline")
{
    const auto violations = scan_directory_sound("ctp2_code/gs", ".cpp");

    if (violations.size() > GS_SOUND_CPP_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("gs/ .cpp sound-include count " << violations.size()
             << " exceeds baseline " << GS_SOUND_CPP_BASELINE
             << ". A new include of sound/<header> has been introduced in a "
                "gs/ source file — revert it or use audio_observer.");
    } else if (violations.size() < GS_SOUND_CPP_BASELINE) {
        MESSAGE("gs/ .cpp sound-include count " << violations.size()
                << " < baseline " << GS_SOUND_CPP_BASELINE
                << " — lower GS_SOUND_CPP_BASELINE to lock in progress.");
    }
}

TEST_CASE("gs/ .h ratchet: sound includes must not grow above baseline")
{
    const auto violations = scan_directory_sound("ctp2_code/gs", ".h");

    if (violations.size() > GS_SOUND_HEADER_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("gs/ .h sound-include count " << violations.size()
             << " exceeds baseline " << GS_SOUND_HEADER_BASELINE);
    } else if (violations.size() < GS_SOUND_HEADER_BASELINE) {
        MESSAGE("gs/ .h sound-include count " << violations.size()
                << " < baseline " << GS_SOUND_HEADER_BASELINE
                << " — lower GS_SOUND_HEADER_BASELINE to lock in progress.");
    }
}

TEST_CASE("ai/ .cpp ratchet: sound includes must not grow above baseline")
{
    const auto violations = scan_directory_sound("ctp2_code/ai", ".cpp");

    if (violations.size() > AI_SOUND_CPP_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("ai/ .cpp sound-include count " << violations.size()
             << " exceeds baseline " << AI_SOUND_CPP_BASELINE);
    } else if (violations.size() < AI_SOUND_CPP_BASELINE) {
        MESSAGE("ai/ .cpp sound-include count " << violations.size()
                << " < baseline " << AI_SOUND_CPP_BASELINE
                << " — lower AI_SOUND_CPP_BASELINE to lock in progress.");
    }
}

TEST_CASE("ai/ .h ratchet: sound includes must not grow above baseline")
{
    const auto violations = scan_directory_sound("ctp2_code/ai", ".h");

    if (violations.size() > AI_SOUND_HEADER_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("ai/ .h sound-include count " << violations.size()
             << " exceeds baseline " << AI_SOUND_HEADER_BASELINE);
    } else if (violations.size() < AI_SOUND_HEADER_BASELINE) {
        MESSAGE("ai/ .h sound-include count " << violations.size()
                << " < baseline " << AI_SOUND_HEADER_BASELINE
                << " — lower AI_SOUND_HEADER_BASELINE to lock in progress.");
    }
}

// ---------------------------------------------------------------------------
// Ratchet tests — robot/ includes (Wave 8a lock).
// gs/ files still include robot/ headers for save/load support (civarchive,
// dynarr).  Future work reduces these via pImpl or interface relocation.
// ---------------------------------------------------------------------------
TEST_CASE("gs/ .cpp ratchet: robot/ includes must not grow above baseline")
{
    const auto violations = scan_directory_robot("ctp2_code/gs", ".cpp");

    if (violations.size() > GS_ROBOT_CPP_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("gs/ .cpp robot-include count " << violations.size()
             << " exceeds baseline " << GS_ROBOT_CPP_BASELINE
             << ". A new include of robot/<header> has been introduced in a "
                "gs/ source file — revert it or use an abstraction layer.");
    } else if (violations.size() < GS_ROBOT_CPP_BASELINE) {
        MESSAGE("gs/ .cpp robot-include count " << violations.size()
                << " < baseline " << GS_ROBOT_CPP_BASELINE
                << " — lower GS_ROBOT_CPP_BASELINE to lock in progress.");
    }
}

TEST_CASE("gs/ .h ratchet: robot/ includes must not grow above baseline")
{
    const auto violations = scan_directory_robot("ctp2_code/gs", ".h");

    if (violations.size() > GS_ROBOT_HEADER_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("gs/ .h robot-include count " << violations.size()
             << " exceeds baseline " << GS_ROBOT_HEADER_BASELINE
             << ". A new include of robot/<header> has been introduced in a "
                "gs/ header file — revert it or forward-declare.");
    } else if (violations.size() < GS_ROBOT_HEADER_BASELINE) {
        MESSAGE("gs/ .h robot-include count " << violations.size()
                << " < baseline " << GS_ROBOT_HEADER_BASELINE
                << " — lower GS_ROBOT_HEADER_BASELINE to lock in progress.");
    }
}

TEST_CASE("ai/ .cpp ratchet: robot/ includes must not grow above baseline")
{
    const auto violations = scan_directory_robot("ctp2_code/ai", ".cpp");

    if (violations.size() > AI_ROBOT_CPP_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("ai/ .cpp robot-include count " << violations.size()
             << " exceeds baseline " << AI_ROBOT_CPP_BASELINE
             << ". A new include of robot/<header> has been introduced in an "
                "ai/ source file — revert it or use an abstraction layer.");
    } else if (violations.size() < AI_ROBOT_CPP_BASELINE) {
        MESSAGE("ai/ .cpp robot-include count " << violations.size()
                << " < baseline " << AI_ROBOT_CPP_BASELINE
                << " — lower AI_ROBOT_CPP_BASELINE to lock in progress.");
    }
}

TEST_CASE("ai/ .h ratchet: robot/ includes must not grow above baseline")
{
    const auto violations = scan_directory_robot("ctp2_code/ai", ".h");

    if (violations.size() > AI_ROBOT_HEADER_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("ai/ .h robot-include count " << violations.size()
             << " exceeds baseline " << AI_ROBOT_HEADER_BASELINE
             << ". A new include of robot/<header> has been introduced in an "
                "ai/ header file — revert it or forward-declare.");
    } else if (violations.size() < AI_ROBOT_HEADER_BASELINE) {
        MESSAGE("ai/ .h robot-include count " << violations.size()
                << " < baseline " << AI_ROBOT_HEADER_BASELINE
                << " — lower AI_ROBOT_HEADER_BASELINE to lock in progress.");
    }
}

// ---------------------------------------------------------------------------
// Ratchet tests — slic/ gfx/ includes (Wave 7 zero-lock).
// Wave 7 cleared all gfx/ includes from slic/; this ratchet locks that
// at 0 so the SLIC interpreter never re-couples to graphics.
// ---------------------------------------------------------------------------
TEST_CASE("gs/slic/ .cpp ratchet: gfx includes must stay at zero")
{
    const auto violations = scan_directory_gfx("ctp2_code/gs/slic", ".cpp");

    if (violations.size() > SLIC_GFX_CPP_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("gs/slic/ .cpp gfx-include count " << violations.size()
             << " exceeds baseline " << SLIC_GFX_CPP_BASELINE
             << ". SLIC must not depend on gfx/; use tiledmap_observer or "
                "render_observer instead.");
    } else if (violations.size() < SLIC_GFX_CPP_BASELINE) {
        MESSAGE("gs/slic/ .cpp gfx-include count " << violations.size()
                << " < baseline " << SLIC_GFX_CPP_BASELINE
                << " — lower SLIC_GFX_CPP_BASELINE to lock in progress.");
    }
}

TEST_CASE("gs/slic/ .h ratchet: gfx includes must stay at zero")
{
    const auto violations = scan_directory_gfx("ctp2_code/gs/slic", ".h");

    if (violations.size() > SLIC_GFX_HEADER_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("gs/slic/ .h gfx-include count " << violations.size()
             << " exceeds baseline " << SLIC_GFX_HEADER_BASELINE
             << ". SLIC must not depend on gfx/; use tiledmap_observer or "
                "render_observer instead.");
    } else if (violations.size() < SLIC_GFX_HEADER_BASELINE) {
        MESSAGE("gs/slic/ .h gfx-include count " << violations.size()
                << " < baseline " << SLIC_GFX_HEADER_BASELINE
                << " — lower SLIC_GFX_HEADER_BASELINE to lock in progress.");
    }
}

// ---------------------------------------------------------------------------
// Ratchet tests — net/ includes (Wave 9a lock).
// gs/ and ai/ files include net/ headers for multiplayer and network I/O.
// Ratcheting locks the current counts so future decoupling work can only
// reduce them.
// ---------------------------------------------------------------------------
TEST_CASE("gs/ .cpp ratchet: net/ includes must not grow above baseline")
{
    const auto violations = scan_directory_net("ctp2_code/gs", ".cpp");

    if (violations.size() > NET_INCLUDES_GS_CPP_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("gs/ .cpp net-include count " << violations.size()
             << " exceeds baseline " << NET_INCLUDES_GS_CPP_BASELINE
             << ". A new include of net/<header> has been introduced in a "
                "gs/ source file — revert it or use an abstraction layer.");
    } else if (violations.size() < NET_INCLUDES_GS_CPP_BASELINE) {
        MESSAGE("gs/ .cpp net-include count " << violations.size()
                << " < baseline " << NET_INCLUDES_GS_CPP_BASELINE
                << " — lower NET_INCLUDES_GS_CPP_BASELINE to lock in progress.");
    }
}

TEST_CASE("ai/ .cpp ratchet: net/ includes must not grow above baseline")
{
    const auto violations = scan_directory_net("ctp2_code/ai", ".cpp");

    if (violations.size() > AI_NET_CPP_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("ai/ .cpp net-include count " << violations.size()
             << " exceeds baseline " << AI_NET_CPP_BASELINE
             << ". A new include of net/<header> has been introduced in an "
                "ai/ source file — revert it or use an abstraction layer.");
    } else if (violations.size() < AI_NET_CPP_BASELINE) {
        MESSAGE("ai/ .cpp net-include count " << violations.size()
                << " < baseline " << AI_NET_CPP_BASELINE
                << " — lower AI_NET_CPP_BASELINE to lock in progress.");
    }
}

// ---------------------------------------------------------------------------
// Ratchet tests — ai/ includes in gs/ (Wave 9a lock).
// gs/ files include ai/ headers for AI backdoor interfaces and strategy types.
// Ratcheting locks the current count so future decoupling work can only
// reduce it.
// ---------------------------------------------------------------------------
TEST_CASE("gs/ .cpp ratchet: ai/ includes must not grow above baseline")
{
    const auto violations = scan_directory_ai("ctp2_code/gs", ".cpp");

    if (violations.size() > GS_AI_CPP_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("gs/ .cpp ai-include count " << violations.size()
             << " exceeds baseline " << GS_AI_CPP_BASELINE
             << ". A new include of ai/<header> has been introduced in a "
                "gs/ source file — revert it or use an abstraction layer.");
    } else if (violations.size() < GS_AI_CPP_BASELINE) {
        MESSAGE("gs/ .cpp ai-include count " << violations.size()
                << " < baseline " << GS_AI_CPP_BASELINE
                << " — lower GS_AI_CPP_BASELINE to lock in progress.");
    }
}

// ---------------------------------------------------------------------------
// Ratchet tests — gs/database/ includes in ai/ .cpp (Wave 10a lock).
// ai/ files include gs/database/ headers for DB access and schema types.
// Ratcheting locks the current count so future decoupling work can only
// reduce it.
// ---------------------------------------------------------------------------
TEST_CASE("ai/ .cpp ratchet: gs/database/ includes must not grow above baseline")
{
    const auto violations = scan_directory_gs_database("ctp2_code/ai", ".cpp");

    if (violations.size() > AI_GS_DATABASE_CPP_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("ai/ .cpp gs/database/ include count " << violations.size()
             << " exceeds baseline " << AI_GS_DATABASE_CPP_BASELINE
             << ". A new include of gs/database/<header> has been introduced in an "
                "ai/ source file — revert it or use an abstraction layer.");
    } else if (violations.size() < AI_GS_DATABASE_CPP_BASELINE) {
        MESSAGE("ai/ .cpp gs/database/ include count " << violations.size()
                << " < baseline " << AI_GS_DATABASE_CPP_BASELINE
                << " — lower AI_GS_DATABASE_CPP_BASELINE to lock in progress.");
    }
}

// ---------------------------------------------------------------------------
// Ratchet tests — gs/outcom/ includes in gs/ .cpp (Wave 10a lock).
// gs/outcom/ is a deprecated COM wrapper layer; remaining includes are
// technical debt.  Ratcheting locks the current count so it can only shrink.
// ---------------------------------------------------------------------------
TEST_CASE("gs/ .cpp ratchet: gs/outcom/ (deprecated) includes must not grow above baseline")
{
    const auto violations = scan_directory_gs_outcom("ctp2_code/gs", ".cpp");

    if (violations.size() > GS_OUTCOM_CPP_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("gs/ .cpp gs/outcom/ include count " << violations.size()
             << " exceeds baseline " << GS_OUTCOM_CPP_BASELINE
             << ". A new include of gs/outcom/<header> has been introduced in a "
                "gs/ source file — revert it or remove the dependency.");
    } else if (violations.size() < GS_OUTCOM_CPP_BASELINE) {
        MESSAGE("gs/ .cpp gs/outcom/ include count " << violations.size()
                << " < baseline " << GS_OUTCOM_CPP_BASELINE
                << " — lower GS_OUTCOM_CPP_BASELINE to lock in progress.");
    }
}

// ---------------------------------------------------------------------------
// Ratchet tests — gs/gameobj/ includes in ai/ .h (Wave 10a lock).
// ai/ headers include gs/gameobj/ for game object type definitions.
// Ratcheting locks the current count so future work can only reduce it.
// ---------------------------------------------------------------------------
TEST_CASE("ai/ .h ratchet: gs/gameobj/ includes must not grow above baseline")
{
    const auto violations = scan_directory_gs_gameobj("ctp2_code/ai", ".h");

    if (violations.size() > AI_GS_GAMEOBJ_H_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("ai/ .h gs/gameobj/ include count " << violations.size()
             << " exceeds baseline " << AI_GS_GAMEOBJ_H_BASELINE
             << ". A new include of gs/gameobj/<header> has been introduced in an "
                "ai/ header file — revert it or forward-declare.");
    } else if (violations.size() < AI_GS_GAMEOBJ_H_BASELINE) {
        MESSAGE("ai/ .h gs/gameobj/ include count " << violations.size()
                << " < baseline " << AI_GS_GAMEOBJ_H_BASELINE
                << " — lower AI_GS_GAMEOBJ_H_BASELINE to lock in progress.");
    }
}

// ---------------------------------------------------------------------------
// Ratchet tests — gs/database/ includes in gfx/ .cpp (Wave 14a lock).
// gfx/ files include gs/database/ for record types used in rendering.
// Ratcheting locks the current count so future decoupling work can only
// reduce it.
// ---------------------------------------------------------------------------
TEST_CASE("gfx/ .cpp ratchet: gs/database/ includes must not grow above baseline")
{
    const auto violations = scan_directory_gs_database("ctp2_code/gfx", ".cpp");

    if (violations.size() > GFX_GS_DATABASE_CPP_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("gfx/ .cpp gs/database/ include count " << violations.size()
             << " exceeds baseline " << GFX_GS_DATABASE_CPP_BASELINE
             << ". A new include of gs/database/<header> has been introduced in a "
                "gfx/ source file — revert it or use an abstraction layer.");
    } else if (violations.size() < GFX_GS_DATABASE_CPP_BASELINE) {
        MESSAGE("gfx/ .cpp gs/database/ include count " << violations.size()
                << " < baseline " << GFX_GS_DATABASE_CPP_BASELINE
                << " — lower GFX_GS_DATABASE_CPP_BASELINE to lock in progress.");
    }
}

// ---------------------------------------------------------------------------
// Ratchet tests — gs/gameobj/ includes in net/ .cpp (Wave 14a lock).
// net/ files include gs/gameobj/ for serializing game objects across the
// network.  Ratcheting locks the current count so future decoupling work
// can only reduce it.
// ---------------------------------------------------------------------------
TEST_CASE("net/ .cpp ratchet: gs/gameobj/ includes must not grow above baseline")
{
    const auto violations = scan_directory_gs_gameobj("ctp2_code/net", ".cpp");

    if (violations.size() > NET_GS_GAMEOBJ_CPP_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("net/ .cpp gs/gameobj/ include count " << violations.size()
             << " exceeds baseline " << NET_GS_GAMEOBJ_CPP_BASELINE
             << ". A new include of gs/gameobj/<header> has been introduced in a "
                "net/ source file — revert it or use an abstraction layer.");
    } else if (violations.size() < NET_GS_GAMEOBJ_CPP_BASELINE) {
        MESSAGE("net/ .cpp gs/gameobj/ include count " << violations.size()
                << " < baseline " << NET_GS_GAMEOBJ_CPP_BASELINE
                << " — lower NET_GS_GAMEOBJ_CPP_BASELINE to lock in progress.");
    }
}

// ---------------------------------------------------------------------------
// Ratchet tests — gs/ includes in ui/ .cpp (Wave 14a lock).
// ui/ depends on gs/ broadly — that's architecturally fine.  We ratchet
// the count anyway to detect new direct couplings that might signal
// layering violations.
// ---------------------------------------------------------------------------
TEST_CASE("ui/ .cpp ratchet: gs/ includes must not grow above baseline")
{
    const auto violations = scan_directory_gs("ctp2_code/ui", ".cpp");

    if (violations.size() > UI_GS_CPP_BASELINE) {
        for (const auto& v : violations) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("ui/ .cpp gs/ include count " << violations.size()
             << " exceeds baseline " << UI_GS_CPP_BASELINE
             << ". A new include of gs/<header> has been introduced in a "
                "ui/ source file — review for layering violation.");
    } else if (violations.size() < UI_GS_CPP_BASELINE) {
        MESSAGE("ui/ .cpp gs/ include count " << violations.size()
                << " < baseline " << UI_GS_CPP_BASELINE
                << " — lower UI_GS_CPP_BASELINE to lock in progress.");
    }
}

// ---------------------------------------------------------------------------
// Ratchet baseline — total `extern g_*` declarations in project headers.
//
// Global variables are a multi-threading hazard: every read or write from
// a non-owning thread becomes a data race unless explicitly synchronised.
// They're also a coupling hazard in single-threaded code: hidden
// dependencies, untestable code, order-of-init bugs.  This ratchet locks
// the current count of source-tree-declared globals so any new commit
// that grows the count fails CI.  Reductions (via encapsulation behind
// accessor methods, file-scope `static`-ification, or outright deletion)
// drop the baseline.
//
// Scope: project headers only (.h files under ai/ctp/gfx/gs/net/ui/sound/
// robot/robotcom/mapgen/GameWatch).  Excludes code-generated headers
// (build*/ subtrees) and vendored deps (libs/, 3rdparty/) — those are
// either constants or third-party concerns.
//
// Reference inventory + history:
//   ~/projects/claude/docs/ctp2/globals-audit-2026-05-31.md
//   ~/projects/claude/docs/ctp2/globals-audit-2026-05-31.tsv
// Each declaration counts as an access surface that should be locked.
// (Pre-cleanup baseline was 128 with 115 unique names — some globals had
// forward-decl + interface-decl duplication.)
//
// 2026-05-31: dropped 128 → 119 after the first globals-cleanup batch.
// 2026-05-31: dropped 119 → 117 after batch 2 (g_splash + g_numPlayers
// static-ified; 4 sibling targets in batch 2 found unexpected cross-TU
// consumers and were left as [WIP] for follow-up accessor refactors).
// 2026-05-31: dropped 117 → 113 after batch 3 (accessor-wrap pattern):
//   - g_slicWarnUndeclared deleted outright (dead)
//   - g_creditsWindow + g_lobbyWindow + g_nationalManagementDialog
//     demoted to file-scope `static`, exposed via *_Get() accessors.
// 2026-05-31: dropped 113 → 109 after batch 4 (more accessor-wraps):
//   - g_scienceManagementDialog (3 cross-TU sites consolidated)
//   - g_domesticManagementDialog (2 cross-TU sites consolidated)
//   - g_gameSelectWindow (1 cross-TU site)
//   - g_hotseatList: switched to operation API
//     (hotseatlist_DisplayWindow / hotseatlist_Cleanup) because external
//     consumers were writing the pointer, not just reading it.
// 2026-05-31: dropped 109 → 106 after batch 5 — first non-UI accessor
// wrap, the g_orderInfo[] / g_numOrderInfo / g_orderInfoMap[] triplet
// in gs/gameobj/Order.{h,cpp}.  Read-only after static-init; exposed
// via orderinfo_Get(idx) / orderinfo_Num() / orderinfo_MapAt(unitOrder)
// with bound-check Asserts.  3 cross-layer consumers (ArmyData,
// slicfunc, net_action) migrated.
// 2026-05-31: dropped 106 → 102 after batch 6 (4 more mid-fanout):
//   - g_uiUnitActorRegistry  (gfx+gs+ui, 13 sites)
//   - g_allinoneWindow       (4 ui/netshell consumers, 11 sites)
//   - g_loadsaveWindow       (3 ui consumers, 8 sites + 2 redundant
//                             local externs deleted)
//   - g_theCurrentBattle     (5 cross-layer consumers, 30 sites,
//                             get+set accessor pattern for the
//                             lifecycle-managed pointer)
// 2026-06-02: dropped 13 → 12 — g_player file-static in gameinit.cpp,
//   extern dropped from player.h.  Remaining real callers (safety.h,
//   inline Player::IsThisPlayerARobot) routed through player_Get().
//   Game now adopts the legacy Player** array.
// 2026-06-03: dropped 12 → 10 implicitly during Track A trampoline work —
//   g_turn extern dropped from TurnCnt.h (storage moved into
//   Ctp2::Game::m_turn); another extern eliminated through related
//   header cleanup during the trampoline migration.
// 2026-06-03: dropped 10 → 9 — g_graphicsOptions file-static in
//   gfx_options.cpp, extern dropped from gfx_options.h.  ~15 callers
//   across ui/, gfx/, gs/, ai/, ctp/ migrated to graphicsoptions_Get().
// 2026-06-03: dropped 9 → 7 — g_civPaths + g_gameObservers file-static
//   in their defining TUs, externs dropped from CivPaths.h and
//   game_observer.h.  ~50 cross-TU callers migrated to civpaths_Get()
//   and gameobservers_Get() respectively.
// 2026-06-03: dropped 7 → 4 — globals-finale Wave A.  g_soundManager,
//   g_netfunc, g_gamesetup all file-static in their defining TUs;
//   externs dropped from soundmanager.h and netshell.h.  ~80 consumer
//   files routed through soundmgr_Get(), netfunc_Get(), gamesetup_Get().
// 2026-06-03: dropped 4 → 2 — globals-finale Wave B.  g_colorSet and
//   g_ui file-static in their defining TUs; externs dropped from
//   colorset.h and aui_ui.h.  ~140 consumer files routed through
//   colorset_Get() and aui_ui_Get().
constexpr std::size_t PROJECT_GLOBALS_BASELINE = 2;

std::vector<Violation> scan_extern_globals(const std::string& root)
{
    // Match  `extern <type> [*] g_NAME;`  on a single line, after optional
    // leading whitespace.  The type can include pointer/reference/template
    // soup, so we accept anything non-greedy up to the identifier.  We
    // anchor on `g_` to keep the regex cheap on non-global lines.
    static const std::regex extern_g_re(
        R"(^\s*extern\s+[^;{}]+\bg_[A-Za-z_][A-Za-z0-9_]*\s*(?:\[[^\]]*\])?\s*;)");
    std::vector<Violation> hits;
    const auto& files = cached_read(root, ".h");
    for (const auto& fl : files) {
        std::size_t line_num = 0;
        for (const auto& line : fl.lines) {
            ++line_num;
            if (line.find("g_") == std::string::npos) continue;
            if (line.find("extern") == std::string::npos) continue;
            if (std::regex_search(line, extern_g_re)) {
                hits.push_back({fl.path, line_num, line});
            }
        }
    }
    return hits;
}

TEST_CASE("globals ratchet: extern g_* declarations must not grow above baseline")
{
    std::vector<Violation> all;
    for (const char* root : {
        "ctp2_code/ai", "ctp2_code/ctp", "ctp2_code/gfx", "ctp2_code/gs",
        "ctp2_code/net", "ctp2_code/ui", "ctp2_code/sound",
        "ctp2_code/robot", "ctp2_code/robotcom", "ctp2_code/mapgen",
        "ctp2_code/GameWatch",
    }) {
        auto hits = scan_extern_globals(root);
        all.insert(all.end(), hits.begin(), hits.end());
    }

    if (all.size() > PROJECT_GLOBALS_BASELINE) {
        for (const auto& v : all) {
            INFO("  " << v.file << ":" << v.line << " -> " << v.text);
        }
        FAIL("extern g_* count " << all.size()
             << " exceeds baseline " << PROJECT_GLOBALS_BASELINE
             << ".  A new global has been introduced.  Either encapsulate it "
                "behind an accessor class, demote it to file-scope `static`, "
                "or delete it.  See ~/projects/claude/docs/ctp2/"
                "globals-audit-2026-05-31.md for guidance.");
    } else if (all.size() < PROJECT_GLOBALS_BASELINE) {
        MESSAGE("extern g_* count " << all.size()
                << " < baseline " << PROJECT_GLOBALS_BASELINE
                << " — lower PROJECT_GLOBALS_BASELINE to lock in progress.");
    }
}

// ---------------------------------------------------------------------------
// player_view defaults in an uninitialised (headless) state.
// ---------------------------------------------------------------------------
TEST_CASE("player_view defaults in headless state")
{
    // In a headless build the UI never registers callbacks.
    // VisiblePlayer defaults to -1 (no human viewer).
    // CurPlayer defaults to 0 (first player slot).
    CHECK(player_view::VisiblePlayer() == -1);
    CHECK(player_view::CurPlayer() == 0);
}
