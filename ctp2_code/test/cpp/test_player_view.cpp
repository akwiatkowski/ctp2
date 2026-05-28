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
#include <regex>
#include <string>
#include <vector>

namespace {

// Ratchet baselines — total `#include "ui/..."` lines, summed across files.
// Tighten whenever the migration reduces the actual count.
constexpr std::size_t GS_UI_CPP_BASELINE    = 0;
constexpr std::size_t GS_UI_HEADER_BASELINE = 0;
constexpr std::size_t AI_UI_CPP_BASELINE    = 0;
constexpr std::size_t AI_UI_HEADER_BASELINE = 0;

// Ratchet baselines — total `#include "gfx/..."` lines, summed across files.
// Phase 2 migrated all g_director call sites onto render_observer, but many
// gs/ files still include gfx/ headers for type references (UnitActor*,
// SpriteState*, etc.).  These baselines lock the current count so they can
// only shrink as those type refs get pushed behind interfaces or pImpl'd.
constexpr std::size_t GS_GFX_CPP_BASELINE    = 3;
constexpr std::size_t GS_GFX_HEADER_BASELINE = 3;
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
constexpr std::size_t GS_ROBOT_CPP_BASELINE    = 120;
constexpr std::size_t GS_ROBOT_HEADER_BASELINE = 27;

// ai/ files include robot/ headers for pathing and backdoor access.
// Ratcheting locks the current counts so future work can only reduce them.
constexpr std::size_t AI_ROBOT_CPP_BASELINE    = 13;
constexpr std::size_t AI_ROBOT_HEADER_BASELINE = 7;

// Ratchet baselines — total `#include "net/..."` lines in gs/ .cpp files.
// gs/ historically depends on net/ for multiplayer and network file I/O.
// Ratcheting locks the current count so future decoupling work can only
// reduce it.
constexpr std::size_t NET_INCLUDES_GS_CPP_BASELINE = 134;

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
constexpr std::size_t UI_GS_CPP_BASELINE = 785;

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

std::vector<Violation> scan_directory_with_regex(const std::string& root,
                                                  const char* extension,
                                                  const std::regex& include_re)
{
    std::vector<Violation> violations;
    std::vector<std::string> files;
    walk_source_files(root, extension, files);

    for (const auto& path : files) {
        std::ifstream file(path);
        if (!file) continue;

        std::string line;
        std::size_t line_num = 0;
        while (std::getline(file, line)) {
            ++line_num;
            if (std::regex_search(line, include_re)) {
                violations.push_back({path, line_num, line});
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
