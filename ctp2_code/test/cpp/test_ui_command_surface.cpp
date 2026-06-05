// test/cpp/test_ui_command_surface.cpp
// Ratcheted guard test against direct game-state field writes from ui/.
//
// Background
// ----------
// Phase 0 step 4 of the master plan ("Clean Architecture — Event/Logic/UI
// Separation") establishes that UI code mutates game state EXCLUSIVELY
// through approved command-shaped methods on Player / CityData / ArmyData
// / Diplomat / etc. (e.g. `player_Get(X)->SetResearching(tech)`,
// `cityData->AddBuyFront()`).  These wrappers exist because they handle
// the cross-cutting concerns commands must own: eligibility checks,
// network synchronization (currently dormant — see below), and the local
// mutation itself.
//
// What this test catches
// ----------------------
// Field-level writes that bypass the command surface — e.g. UI code
// directly setting `player_Get(X)->m_playerType = PLAYER_TYPE_ROBOT` instead
// of going through a method.  Such bypasses skip whatever guards the
// approved methods enforce.  The ratchet counts all such patterns across
// ui/aui_ctp2/ and ui/interface/, fails if the count exceeds the
// baseline, and MESSAGEs if it drops below (cue to tighten).
//
// Why this is NOT a typed ICommand wrapper
// ----------------------------------------
// Olek (2026-05-29) chose option "4.3b" from
// `~/projects/claude/docs/ctp2/icommand-design.md`: keep the existing
// command-shaped wrappers as the input surface, just enforce the contract
// with a ratchet.  No new class hierarchy.  If a future session needs a
// typed wrapper (replay infrastructure, AI eval harness, or a network
// rewrite that needs to serialize commands over the wire), the design doc
// has the trigger conditions and an upgrade path documented.
//
// Network reactivation notes
// --------------------------
// CTP2's network code is currently disabled but kept in the tree.
// Existing wrappers like `Player::SendTradeBid` and `CityData::AddBuyFront`
// already contain inactive multiplayer branches guarded by
// `network_Get().IsClient()` / `network_Get().IsHost()`.  When network is
// re-enabled:
//   - The ratchet here does NOT need to change — UI still goes through
//     the same wrapper methods regardless of multiplayer state.
//   - Backward compatibility with the old multiplayer protocol is not
//     required (Olek's call, 2026-05-29).  Old save files / multiplayer
//     wire formats can break.
//   - That's likely when a typed ICommand wrapper (option 4.3a in the
//     design doc) becomes worth doing, because the new network layer
//     will define what the wire format for commands looks like.
//
// Procedure for new ratchet violations
// ------------------------------------
//   - If you legitimately need to add a new direct-field write from UI
//     (e.g. scenario-editor admin tooling), raise the baseline below in
//     the same commit, with a comment explaining why the bypass is OK.
//   - Otherwise, refactor the UI call to go through an approved wrapper
//     method on the gs/ object.  If the appropriate wrapper does not yet
//     exist, add one to the gs/ class.
//
// Procedure for tightening
// ------------------------
//   - When a migration removes a direct-field write, re-run the test.
//     It will MESSAGE that the count dropped below baseline.  Lower the
//     baseline constants below in the same commit.

#include "doctest.h"

#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <fstream>
#include <regex>
#include <string>
#include <vector>

namespace {

// Ratchet baselines.
//
// `player_Get(...)->m_...` field write — UI bypassing Player methods to
// poke fields directly.  Current 8 are in:
//   - scenarioeditor.cpp (4 sites — admin mode, sets m_playerType /
//     m_current_round directly).  Acceptable for now because scenario
//     editor is debug tooling, but should eventually move through a
//     `Player::SetPlayerType` method.
//   - chatbox.cpp (4 sites — IRC-style /human and /computer commands
//     that flip player type at runtime).  Same disposition.
constexpr std::size_t UI_GS_PLAYER_FIELD_WRITE_BASELINE = 0;

// `xxx.AccessData()->m_...` or `xxx->AccessData()->m_...` field write —
// UI reaching through the gs-handle accessor to bypass a method.  Should
// stay at zero.  AccessData() is a legitimate path to call methods (e.g.
// `unit.AccessData()->CreateOwnArmy()`); only field writes are flagged.
constexpr std::size_t UI_GS_ACCESSDATA_FIELD_WRITE_BASELINE = 0;

struct Violation {
    std::string file;
    std::size_t line;
    std::string text;
};

// POSIX recursive walk for a given extension.  Matches the convention in
// test_player_view.cpp (project builds with cpp_std=none; avoid C++17
// <filesystem>).
void walk_source_files(const std::string& root, const char* extension,
                       std::vector<std::string>& out)
{
    DIR* dir = opendir(root.c_str());
    if (!dir) return;

    const std::size_t ext_len = std::strlen(extension);

    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        if (std::strcmp(ent->d_name, ".") == 0 ||
            std::strcmp(ent->d_name, "..") == 0)
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

// Cached list of source files under ui/.  Walking the tree once and
// reusing the list keeps the fast-test suite under its meson budget;
// two separate walks would add ~5s.  Built lazily on first call.
const std::vector<std::string>& ui_source_files()
{
    static std::vector<std::string> files = []() {
        std::vector<std::string> v;
        const std::string ui_root = "../ctp2_code/ui";
        walk_source_files(ui_root, ".cpp", v);
        walk_source_files(ui_root, ".h", v);
        return v;
    }();
    return files;
}

std::vector<Violation> scan_for_pattern(const std::regex& pattern)
{
    std::vector<Violation> violations;
    for (const auto& path : ui_source_files()) {
        std::ifstream f(path);
        if (!f) continue;

        std::string line;
        std::size_t line_num = 0;
        while (std::getline(f, line)) {
            ++line_num;
            if (std::regex_search(line, pattern)) {
                violations.push_back({path, line_num, line});
            }
        }
    }
    return violations;
}

// Print up to 25 violations for diagnostic context.  Used when the
// caller has already reported the ratchet status itself.
void print_violations(const std::vector<Violation>& vs)
{
    for (std::size_t i = 0; i < vs.size() && i < 25; ++i) {
        MESSAGE(vs[i].file << ":" << vs[i].line
                << "  " << vs[i].text.c_str());
    }
    if (vs.size() > 25) {
        MESSAGE("... and " << (vs.size() - 25) << " more");
    }
}

}  // namespace

TEST_CASE("UI command-surface ratchet: player_Get()->m_ field writes")
{
    // `player_Get(<anything>)->m_<word>` followed by `=` (but NOT `==`).
    // Negative lookahead `(?!=)` excludes equality comparisons while
    // still allowing compound assignment (`+=`, `|=`, etc.) and bare
    // `=` at end-of-line (multi-line assignment continuation).
    std::regex pattern(R"(player_Get\([^)]+\)->m_\w+\s*[+\-*/|&^]?=(?!=))");
    auto violations = scan_for_pattern(pattern);

    if (violations.size() > UI_GS_PLAYER_FIELD_WRITE_BASELINE) {
        MESSAGE("player_Get()->m_ field-write ratchet BROKEN: found "
                << violations.size() << " violations, baseline "
                << UI_GS_PLAYER_FIELD_WRITE_BASELINE
                << ". A new direct field-write on player_Get(X)->m_<field> "
                   "has been introduced in UI code — refactor to go "
                   "through an approved Player::* method, or raise the "
                   "baseline (with comment) if the bypass is necessary.");
        print_violations(violations);
    } else if (violations.size() < UI_GS_PLAYER_FIELD_WRITE_BASELINE) {
        MESSAGE("player_Get()->m_ field-write count "
                << violations.size() << " < baseline "
                << UI_GS_PLAYER_FIELD_WRITE_BASELINE
                << " — lower UI_GS_PLAYER_FIELD_WRITE_BASELINE to lock "
                   "in progress.");
    }
    CHECK(violations.size() <= UI_GS_PLAYER_FIELD_WRITE_BASELINE);
}

TEST_CASE("UI command-surface ratchet: AccessData()->m_ field writes")
{
    // `something.AccessData()->m_<word> =` or `something->AccessData()->m_<word> =`.
    // The AccessData() pattern is `unit.AccessData()` (Unit value type)
    // or `someUnitPtr->AccessData()` (rare but exists in the codebase).
    // Negative lookahead `(?!=)` excludes `==` while allowing `+=`, `=$`, etc.
    std::regex pattern(R"(AccessData\(\)->m_\w+\s*[+\-*/|&^]?=(?!=))");
    auto violations = scan_for_pattern(pattern);

    if (violations.size() > UI_GS_ACCESSDATA_FIELD_WRITE_BASELINE) {
        MESSAGE("AccessData()->m_ field-write ratchet BROKEN: found "
                << violations.size() << " violations, baseline "
                << UI_GS_ACCESSDATA_FIELD_WRITE_BASELINE
                << ". A UI call now writes a gs/-side field via the "
                   "AccessData() backdoor.  Use a method on the gs/ "
                   "class instead.");
        print_violations(violations);
    } else if (violations.size() < UI_GS_ACCESSDATA_FIELD_WRITE_BASELINE) {
        MESSAGE("AccessData()->m_ field-write count "
                << violations.size() << " < baseline "
                << UI_GS_ACCESSDATA_FIELD_WRITE_BASELINE
                << " — lower UI_GS_ACCESSDATA_FIELD_WRITE_BASELINE to "
                   "lock in progress.");
    }
    CHECK(violations.size() <= UI_GS_ACCESSDATA_FIELD_WRITE_BASELINE);
}
