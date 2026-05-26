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

std::vector<Violation> scan_directory(const std::string& root,
                                      const char* extension)
{
    std::vector<Violation> violations;
    // ui/netshell/* is the multiplayer network shell, not UI rendering;
    // consumed by gs/fileio/gamefile.h and gs/network code as a legitimate dependency.
    static const std::regex include_ui_re(R"(^\s*#include\s+[<\"]ui/(?!netshell/))");

    std::vector<std::string> files;
    walk_source_files(root, extension, files);

    for (const auto& path : files) {
        std::ifstream file(path);
        if (!file) continue;

        std::string line;
        std::size_t line_num = 0;
        while (std::getline(file, line)) {
            ++line_num;
            if (std::regex_search(line, include_ui_re)) {
                violations.push_back({path, line_num, line});
            }
        }
    }
    return violations;
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
