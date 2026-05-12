# CTP2 Repo Collaboration: What Each Side Could Gain

## tl;dr

Both repos share the same Activision/Apolyton heritage but diverged on build system, platform support, and testing philosophy. A two-way exchange of the macOS port + smoke test from one side, and CI/packaging/FFmpeg from the other, would produce the most robust CTP2 build to date.

---

## What civctp2/civctp2 Could Take From This Repo

### 1. macOS / Apple Silicon Port (Unique)
**Status:** This is the only known native macOS (ARM64) SDL2 build of CTP2.

**What's included:**
- Meson build system with macOS toolchain detection
- SDL2 input on main thread (no separate mouse thread)
- Apple Silicon compatible sprite rendering
- Bounds-checked sprite parsing to prevent RLE buffer overruns

**Why it matters:** Issue [#350](https://github.com/civctp2/civctp2/issues/350) ("Building on OSX") has been open since 2020 with no resolution. This repo proves it's possible.

### 2. Headless Smoke Test Harness
**Status:** Unix socket command server + Python test runner. No GUI required.

**Commands implemented:**
- `new_game`, `start_game`, `end_turn`, `build_city`
- `set_production`, `enable_governor`
- `turn_counter`, `advance_turns`
- `save_game`, `load_game`
- `screenshot`, `diplomacy_status`
- `move_unit`, `list_visible_units`

**Why it's better than Sikuli:**
- Runs without X11 (works in CI containers)
- ~5 seconds per scenario vs. minutes of GUI clicking
- Can be wrapped in `lldb` for automatic crash backtraces
- Tests game engine directly, not UI simulation

**What civctp2 would need:** Integration into their GitLab CI pipeline (a single Docker stage running the Python harness).

### 3. Crash Fixes With Root Cause Analysis

| Crash | Root Cause | Fix in this repo |
|-------|-----------|------------------|
| Next Turn crash | `UNITACTION` negative enum values used as array index → underflow | Bounds-checked getters |
| Sprite loading crash | Corrupt RLE data causes buffer overruns | Bounds checking in `ConvertPixelFormat` |
| Scroll infinite loop | `KEYUP` events not pumped in scroll loop | SDL keyboard event pump in scroll loop |
| ID trivially-copyable | Custom copy ctor prevented C varargs passing | `= default` copy ctor |
| Governor crash | `UnitData::GetDBRec(this=NULL)` during AI turn | Discovered via smoke test |

### 4. Mouse Thread Elimination
**Problem:** civctp2 still has a separate `MouseThread` calling SDL event functions concurrently with the main thread (crash-prone on all platforms, fatal on macOS).

**Fix here:** Move all SDL input processing to the main thread. This is a prerequisite for any macOS port and improves stability on Linux/Windows too.

---

## What This Repo Could Take From civctp2/civctp2

### 1. Cross-Platform Save Format
**Status:** civctp2 unified the Linux/Windows save format (commit `22dd180`). Old Linux saves need conversion.

**Why it matters:** Our `save_game` / `load_game` smoke test commands work, but we haven't tested cross-platform save compatibility. If we want saves to work between macOS and Linux builds, we need the unified format.

### 2. CI/CD Infrastructure
**Status:** GitLab CI with Docker images for multiple Ubuntu versions, Windows builds via Visual Studio.

**What we'd gain:**
- Automated build verification on every commit
- Reproducible Docker-based builds (pin dependency versions)
- Multi-compiler testing (GCC, Clang, MSVC)

### 3. FFmpeg Video Playback
**Status:** `USE_SDL_FFMPEG` support for in-game movies. Disabled by default due to FFmpeg API churn.

**Why it matters:** CTP2 has intro videos, wonder movies, and advisor animations. The macOS build currently has no video support.

**Integration effort:** Medium — needs meson build rules for FFmpeg detection and the `aui_sdlmovie.cpp` video renderer.

### 4. Windows 64-bit Build
**Status:** civctp2 builds on Win32, x64, ARM, and ARM64 (though ARM/ARM64 don't link due to missing SDL2/FFmpeg libs).

**Why it matters:** If we ever want to distribute this macOS port to Windows users, having the upstream 64-bit Windows build ensures parity.

### 5. Modding & Data Tools
**Status:** Advance-Graph generator, scenario editor improvements, mod tools in `tools/` directory.

### 6. GDB Auto-Backtrace Setup
**Status:** PR #442 includes a GDB command file that automatically captures backtraces on assertion failure and SIGSEGV.

**Why it matters:** We already wrap the game in `lldb`, but civctp2's GDB setup is more sophisticated (batch mode, SIGINT catching for assertions).

---

## Proposed Collaboration Path

### Phase 1: Share Smoke Test (Low Risk)
1. **This repo** opens a PR against civctp2 with the smoke test server + Python harness
2. civctp2 adds a GitLab CI stage that runs the smoke test in a headless container
3. Both repos benefit from shared test scenarios

### Phase 2: Upstream Crash Fixes (Medium Risk)
1. Extract the UNITACTION bounds checking, sprite bounds checking, and mouse thread elimination as focused PRs
2. Each PR includes a smoke test scenario that fails before the fix and passes after
3. civctp2 reviewers can verify on Linux/Windows

### Phase 3: macOS Port (Higher Risk)
1. Port the meson build system changes to civctp2 (or add macOS support to their autotools)
2. Merge the SDL2 main-thread input changes
3. Add macOS to their CI matrix (GitHub Actions macOS runner)

### Phase 4: Shared Test Scenarios (Ongoing)
1. Create a shared `tests/scenarios/` directory with JSON scenario files
2. Both repos run the same scenarios via their respective harnesses
3. Scenarios act as regression tests for crashes

---

## Open Questions for civctp2 Maintainers

1. **Build system:** Would you accept meson as an alternative build system, or should we port macOS support to autotools?
2. **SDL2 input:** Are you open to eliminating the `MouseThread` on all platforms, or should it be macOS-only?
3. **Compiler optimizations:** Have you tried AddressSanitizer instead of disabling `-O2`? ASan catches the exact bugs we're finding.
4. **Smoke test:** Would you replace Sikuli with the socket-based harness, or run both in parallel?

---

## Contact Context

When reaching out to MartinGuehmann / LynxAbraxas, mention:
- You have a working **Apple Silicon port** (something they've wanted since 2020)
- You found **root causes for spurious crashes** they reported in #441
- Your smoke test is **faster and CI-friendly** compared to their Sikuli setup
- You're offering **focused, tested PRs** rather than a big-bang rewrite
