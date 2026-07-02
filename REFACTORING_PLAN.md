# CTP2 Refactoring Plan

Purpose: give any assistant or developer a quick, shared answer to: "how much more refactoring work remains?"

This file is deliberately plain Markdown so it is easy for different LLMs (OpenAI, Claude Opus, local models) to read and update. Keep it short. Update the board after each committed batch.

## Overall Progress

**43 / 45 checkboxes done (~96%).** M7 and M8 are complete. Remaining bounded work: **M9 close-out (~1 session)** reaches the original Definition of Done. After that the priority (per Olek, 2026-07-02) is **M11 mechanical memory-safety refactoring** (raw `new`/`delete` → RAII); the modern-asset converter (M10) is **parked** now that the M8 spike is done.

Recount any time with:

```sh
grep -c '^- \[x\]' REFACTORING_PLAN.md   # done
grep -c '^- \[ \]' REFACTORING_PLAN.md   # remaining
# M11 memory phase is tracked in its own file (per-file checkboxes):
grep -c '^- \[x\]' docs/memory-refactor-checklist.md   # files fixed
grep -c '^- \[ \]' docs/memory-refactor-checklist.md   # files remaining
```

## Current Definition Of Done

The refactoring effort is "done enough" when CTP2 has:

- Fast pre-commit tests that stay green. ✅ (M1, M6)
- A working sanitizer smoke tier on macOS/Linux. ✅ (`make ubsan-smoke`; ASan documented as platform-blocked)
- Modernization ratchets preventing regression in unsafe patterns. ✅ (M3; enforced in `make test`)
- Warning noise reduced enough that new warnings are visible. ✅ (M2; leftovers documented)
- Timelapse/play-session tooling stable enough for repeated use. ✅ (M4)
- A first modern asset-conversion path that preserves original game data as canonical input. ⬜ (M8)
- A documented list of remaining legacy-risk areas instead of open-ended cleanup. ⬜ (M9)

This is not a full rewrite. A fully modernized CTP2 engine is open-ended and likely much larger than this project needs.

## Short Answer Formula

When asked "how much more work remains?", answer from this scale:

- Finish obsolete subsystem removal (M7): **1-3 sessions**.
- Modern asset pipeline spike (M8): **3-6 sessions**.
- Close-out documentation pass (M9): **~1 session**.
- Fully refactored engine: **open-ended / not a bounded goal**.

Recommended default answer: **about 5-9 focused sessions** to reach the Definition of Done.

## Milestone Board

Use this as the real burn-down list. Move an item to `[x]` only after the code is committed and the listed verification has passed.

| ID | Milestone | Status | Estimate | Verification |
| --- | --- | --- | --- | --- |
| M1 | Baseline safety loop | 4/4 done | complete | `make test`, `make ubsan-smoke` |
| M2 | Warning noise reduction | 8/8 done | complete | `make test`, warning category visibly reduced |
| M3 | Modernization ratchet burn-down | 6/6 done | complete | ratchet baseline lowered without regressions |
| M4 | Timelapse/play tooling polish | 5/5 done | complete | short timelapse smoke, docs updated |
| M5 | UI/frame polish | 3/3 done | complete | visible/manual or smoke verification |
| M6 | Final stabilization pass | 4/4 done | complete | all standard checks, plan updated |
| M7 | Obsolete subsystem removal | 8/8 done | complete | obsolete code removed without network/movie regressions |
| M8 | Modern asset pipeline spike | 5/5 done | complete | legacy sprite exports reproducibly; visual parity checked |
| M9 | Close-out | 0/2 done | ~1 session | plan reflects reality; DoD declared |
| M11 | Memory-safety refactoring (raw new/delete → RAII) | 9/574 files (see M11 breakdown) | multi-session (**priority**) | ratchet raw_new/raw_delete/c_allocation counts fall without behavior change |
| M10 | Modern-asset converter + first-run (follow-on) | **parked** | deferred | `~/.ctp2` atlases generated from owned data; engine modern-first loader |

### M1: Baseline Safety Loop

- [x] Add modernization ratchets to `make test`.
- [x] Add `sanitized-smoke` target for ASan/UBSan attempt.
- [x] Add working `ubsan-smoke` target.
- [x] Document ASan macOS blocker in this plan with sample-stack summary.

ASan blocker summary: on the current macOS/Apple clang setup, `build-sanitized/ctp2_headless` hangs before `main` in the dynamic loader/ASan runtime. A `sample` of the process showed `dyld4::APIs::runAllInitializersForMain -> libSystem_initializer -> __malloc_init -> wrap_malloc_default_zone -> __asan::AsanInitFromRtl -> __asan::InitializeShadowMemory -> __sanitizer::MemoryRangeIsAvailable`, then spinning in `__sanitizer::StaticSpinMutex::LockSlow`. This means the ASan issue is currently platform/runtime startup, not CTP2 game initialization. Use `make ubsan-smoke` as the working sanitizer tier until ASan is fixed or tested on another platform/toolchain.

### M2: Warning Noise Reduction

- [x] Clean scalar `NULL` defaults/args in isolated files.
- [x] Clean first include-case and fixed-array-null warning batches.
- [x] Mechanical `Player.h` -> `player.h` include-case sweep for headers/sources that compile in fast/unit/headless paths.
- [x] Mechanical `CityData.h` -> `citydata.h` sweep where warnings remain.
- [x] Fix low-risk AI `&&`/`||` precedence warnings in `Goal.cpp` and `settlemap.cpp`.
- [x] Fix safe dead-local / unused-variable warnings where the variable has no side effect.
- [x] Fix simple constructor initializer-order warnings where member order is obvious.
- [x] Re-run `make ubsan-smoke` and record remaining high-signal warning categories here.

Remaining high-signal warning categories after the M2 pass (post-DoD, opportunistic only):

- Include-case stragglers in files outside the targeted fast/unit/headless sweep, mostly remaining `Player.h` includes pulled in by broader rebuilds.
- Legacy writable string conversions (`char *` / `MBCHAR *` from string literals), especially diplomacy, UI popup/window, and logging paths.
- Broader unused-variable / set-but-unused warnings in large legacy files such as `governor.cpp`, `ArmyData.cpp`, and `diplomat.cpp`; many need local behavior review before removal because initializers can have side effects.
- Additional logical precedence warnings in broad game/pathing files such as `ArmyData.cpp` and `robotastar2.cpp`; defer to smaller behavior-preserving batches.
- Switch exhaustiveness warnings for legacy enums and overloaded-virtual hiding warnings in UI/sprite/network classes.

### M3: Modernization Ratchet Burn-Down

- [x] Establish ratchet baseline and enforce it in `make test`.
- [x] Reduce one small unsafe string API cluster, then update baseline.
- [x] Reduce one small raw `new`/`delete` ownership cluster with clear ownership.
- [x] Reduce one C allocation cluster where lifetime is local/simple.
- [x] Reduce one type-erased cast cluster only if behavior is obvious.
- [x] Add a short note here listing ratchet counts after the last reduction.

Current baseline (`tools/modernization/ratchet_baseline.json`, verified 2026-07-02): `c_allocation=516`, `raw_delete=2065`, `raw_new=5528`, `type_erased_casting=3739`, `unsafe_string_api=1416`. Further reductions are post-DoD, opportunistic only.

### M4: Timelapse / Play Tooling Polish

- [x] Run long fogged hero timelapse successfully.
- [x] Add `query_names` metadata and richer Chronicle captions.
- [x] Add short usage docs for common timelapse commands.
- [x] Add a tiny caption-render smoke fixture or helper test if practical.
- [x] Pick one visible caption/frame polish improvement and verify with a short run.

### M5: UI / Frame Polish

- [x] Pick one concrete frame pacing/window persistence/UI scaling issue.
- [x] Implement the smallest visible fix.
- [x] Verify on desktop and avoid broad UI constructor const-correctness unless explicitly scoped.

M5 fix: the SDL main-loop idle cap now uses a 17 ms frame budget, matching `1000 / 60` rounded up instead of an undershooting 16 ms magic number. Verified with `mise exec -- make test`, `git diff --check`, and `mise exec -- make ubsan-smoke`; committed as `a3e59cf6`.

### M6: Final Stabilization Pass

- [x] Run `make test` and `make ubsan-smoke` cleanly after final batch.
- [x] Update this plan with final counts and remaining caveats.
- [x] Ensure `README.md` does not contradict current Makefile commands.
- [x] Stop with clean worktree and a concise handoff.

M6 final verification: `mise exec -- make test`, `git diff --check`, and `mise exec -- make ubsan-smoke` pass. `README.md` build instructions now point at the current root-level `mise exec -- make setup`, `mise exec -- make build`, `mise exec -- make test`, and UBSan smoke workflow instead of the stale `build-headless` Meson commands.

### M7: Obsolete Subsystem Removal

Scope: remove legacy systems that are no longer product goals. Windows support should later use SDL/Linux-like paths, not old DirectX/Win32 runtime plumbing. Networking is out of scope for this milestone. Movie playback and movie DB/schema/data are product goals and should be preserved for future repair, not removed.

- [x] Remove remaining CD-ROM / Redbook audio / copy-protection code.
- [x] Confirm GameWatch provenance; if it is original Activision telemetry/recording/plugin code, remove it.
- [x] Remove Windows registry / file-association / DirectX startup checks.
- [x] Guard or drop the unconditional `aui_directx` includes in active sources: `ui/aui_common/aui_Factory.cpp` (3 headers), `ui/aui_ctp2/c3blitter.cpp` (`aui_directsurface.h`), `ctp/civ3_main.cpp` (`aui_directmoviemanager.h`). Use branches (`c3ui.h` `#else` pattern) or `__AUI_USE_DIRECTX__` guards so SDL builds no longer need the headers on disk.
- [x] Decide movie-path handling before deletion: `aui_directmovie.*` / `aui_directmoviemanager.*` and the self-guarded `directvideo.*` are movie code — keep, relocate, or stub them (movies are a product goal for later repair).
- [x] Delete the remaining `ui/aui_directx/` sources that are not needed for movie repair.
- [x] Purge `aui_directx` references from Windows/legacy project files: `ui/ui.dsp`, `ui/ui.vcxproj(.filters)`, `ctp2_code/Makefile.am`, `gs/newdb/Makefile.am`, plus stragglers in `net/net.dsp`, `gs/gs.dsp`, `robot*/…dsp`, `mapgen/plasma1.dsp`.
- [x] Re-run `make test`, `git diff --check`, and `make ubsan-smoke`; record any retained compatibility caveats here.

GameWatch was confirmed as obsolete plugin-based recording/delivery code (`gwciv`, `gwfile`, `gwarchive`) that wrote unit build/kill records and `recordN.dat` payloads. Runtime hooks, profile setting, active Meson include path, legacy autotools include paths, source tree, DLL, and static libraries were removed in `b4f79eb0`. Stale Visual Studio GameWatch include paths, import defines, and library dependencies were removed with the Windows registry / DirectX startup cleanup batch.

Windows registry / DirectX startup cleanup removed the `.c2g` file-association registry writes, the fatal `dxver.dll` startup gate, and the duplicate AUI `dxver` probe. Movie/DirectShow COM code and networking/anet registry helpers are intentionally retained: movies should be repaired later, and networking is outside this milestone.

DirectX AUI backend status (verified 2026-07-02): the active Meson build no longer references `ui/aui_directx` include paths or source files (`8eb6fe01`). The remaining unconditional `aui_directx` includes in active sources are now guarded (2026-07-02): `aui_Factory.cpp` and `c3blitter.cpp` wrap their DirectX headers in `#if defined(__AUI_USE_DIRECTX__)` (the symbols are only used in the matching `#elif defined(__AUI_USE_DIRECTX__)` branches), and `civ3_main.cpp` wraps `aui_directmoviemanager.h` in `#if !defined(__GNUC__)` to match the exact guard on the only `aui_DirectMovieManager` use. SDL builds (`make test`, `make ubsan-smoke`) no longer need the `ui/aui_directx` headers on disk. The `ui/aui_directx/` source tree still exists; deleting it (after settling movie-path handling) remains a later M7 checkbox. `directvideo.*` stays in the Meson build but is fully self-guarded by `__AUI_USE_DIRECTX__`, so it compiles to nothing on SDL.

Movie-path handling decision (2026-07-02): **keep movie code in place, guarded; do not relocate or stub in this refactor.** Rationale and consequences for the deletion checkbox:

- Build config: `auicfg.h` defines `__AUI_USE_DIRECTX__` + `__AUI_USE_DIRECTMEDIA__` only under `WIN32`; SDL builds get `__AUI_USE_SDL__` only. So all DirectX/DirectMedia movie code is inert on the current macOS/Linux build.
- `ui/aui_directx/aui_directmovie.*` and `aui_directmoviemanager.*` are the DirectShow reference implementation, guarded by `__AUI_USE_DIRECTMEDIA__` and absent from the Meson build. Keep them as the thing to repair later. The only active-source touch is `civ3_main.cpp`, whose include and `new aui_DirectMovieManager()` use are both guarded (`!__GNUC__`).
- The cross-platform movie surface already compiles in the SDL build and stays: `ui/aui_common/aui_movie.*` / `aui_moviemanager.*` / `aui_moviebutton.*`, `ui/aui_ctp2/directvideo.*` (self-guarded by `__AUI_USE_DIRECTX__`), `gs/database/moviedb.*`, and the `ui/interface/*moviewin*` windows.
- Consequence for the next checkbox: `aui_directmovie.cpp` includes sibling headers `aui_directui.h` and `aui_directsurface.h`, so those two headers count as "needed for movie repair" and must be retained (or their loss explicitly accepted as dangling includes) when the rest of `ui/aui_directx/` is deleted.

Project-file purge done (2026-07-02): removed every `aui_directx` reference from the 27 legacy VS6/autotools/VS project files (`ui/ui.dsp`, `ui/ui.vcxproj(.filters)`, `ctp/civctp.{dsp,vcxproj,vcxproj.filters}`, `ctp/civctp_j.dsp`, `ctp2_code/Makefile.am`, `ctp/Makefile.am`, `gs/newdb/Makefile.am`, `os/linux/civctp2.prj`, `net/net.dsp`, `gs/gs.dsp`, `gfx/gfx.dsp`, `robot*/*.dsp`, `robotcom/robotcom.mak`, `mapgen/*.{dsp,vcxproj}`) — both `/I`/`AdditionalIncludeDirectories` include paths and source-file/group membership, including the retained movie sources' entries. Removal was verified with a repo-wide grep (zero `aui_directx` refs remain in project files) and an XML well-formedness parse of every touched `.vcxproj`/`.filters`. None of these files are part of the active Meson build, so `make test` and `make ubsan-smoke` are unaffected (both pass). CRLF line endings on the `.dsp`/`.mak` files were preserved; `git diff --check` reports "trailing whitespace" on their changed lines, but that is only the pre-existing CR (`0x0d`) of the DOS line endings, not added whitespace (verified byte-for-byte against `HEAD`) — do not "fix" it, as stripping the CR would corrupt the VS6 line-ending style.

M7 verification (2026-07-02): `make test` (fast+unit) and `make ubsan-smoke` pass; `git diff --check` is clean apart from the documented CRLF-CR artifacts on `.dsp`/`.mak` files. Retained compatibility caveats: (1) the DirectShow movie reference (`aui_directmovie.*`, `aui_directmoviemanager.*`) plus its `aui_directui.h`/`aui_directsurface.h`/`aui_directx.h` header closure stay on disk for future SDL-based repair and never compile on SDL; (2) the retained `aui_directui.h`/`aui_directsurface.h` declare classes whose `.cpp` bodies were deleted; (3) the legacy VS6/autotools project files remain in the tree but no longer reference the DirectX backend — they are not the canonical build (Meson is) and are tracked as a deferred legacy-risk area for M9.

Do not remove yet:

- Network / multiplayer code; it will be resolved later.
- Movie playback code and wonder/victory movie DB/schema/data fields; the desired direction is to make movies work later.
- `aui_directmovie.*`, `aui_directmoviemanager.*`, and their `aui_directui.h` / `aui_directsurface.h` dependencies within `ui/aui_directx/` (needed for movie repair per the decision above).

Backend deletion done (2026-07-02): removed 17 pure-DirectX-backend files from `ui/aui_directx/` (`aui_directaudiomanager.*`, `aui_directblitter.*`, `aui_directinput.*`, `aui_directjoystick.*`, `aui_directkeyboard.*`, `aui_directmouse.*`, `aui_directsound.*`, plus the `.cpp` bodies of `aui_directsurface`, `aui_directui`, `aui_directx`). This was build-safe because the whole directory is already out of the Meson build and every active-source include of these headers sits behind a `__AUI_USE_DIRECTX__` / `!__GNUC__` guard. Retained the closed header/source set the DirectShow movie reference needs: `aui_directmovie.{h,cpp}`, `aui_directmoviemanager.{h,cpp}`, `aui_directui.h`, `aui_directsurface.h`, `aui_directx.h` (the last three are the transitive `#include` closure of `aui_directmovie.cpp`). The retained `aui_directui.h` / `aui_directsurface.h` now declare classes whose `.cpp` implementations are gone; this is intentional — they remain only as reference headers for a future SDL-based movie repair and never compile on SDL. Verified with `make test`, `git diff --check`, and `make ubsan-smoke`.

### M8: Modern Asset Pipeline Spike

Goal: make original game data compatible with a future modern renderer without breaking mod/original-data compatibility. Original assets remain canonical; generated modern assets are cache/build artifacts.

- [x] Write a small `.SPR` inspector/exporter for one representative unit sprite (`GU###.SPR`).
- [x] Export frames with action/facing/frame metadata, hot points, dimensions, and draw flags needed by the current renderer.

Inspector done (2026-07-02): `tools/assets/spr_inspect.py` is a standalone read-only inspector (no engine dependency, original assets stay canonical). It parses the `.SPR` container header for all three versions and decodes the unit action table plus each present action's sprite header — sprite type, `width`x`height`, frame count, first frame, and per-facing hot points — without decoding pixels. Format was reverse-engineered from `gfx/spritesys/spritefile.cpp` (`Open`, `ReadBasic_v13`, `ReadBasic_v20`, `ReadFacedSpriteDataBasic`) and cross-checked byte-for-byte against `GU04.SPR`/`GU065.SPR`. Key facts for the next slices: tag `0x53505246` ("FRPS" on disk); versions `0x00010003` (v13) / `0x00020000` (v20) / `0x00020001` (v20+compression field); type `4` = UNIT. v13 unit body = `int32 offsets[5]` (MOVE, ATTACK, IDLE, VICTORY, WORK); v20 unit body = `int32 offsets[17]` where the last entry is the special-data (shield/fire points) offset, not an action. Per-action header: `uint16 sprite_type` (0 NORMAL / 1 FACED / 2 FACEDWSHADOW), `uint16 width`, `uint16 height`, hot points (one `POINT{int32 x,y}` for NORMAL, five for FACED), `uint16 first_frame`, `uint16 num_frames`. Sample `GU04.SPR` = 96x72, MOVE(11 frames)/ATTACK(8)/IDLE(4). Frame pixel payloads are LZW1-or-raw compressed and still undecoded — that is the next checkbox.
- [x] Generate a debug-friendly PNG frame dump first; atlas/KTX-style packing can follow after parity is proven.
Frame export done (2026-07-02): `tools/assets/spr_export.py` decodes unit `.SPR` frames to debug PNGs plus a JSON manifest (action/facing/frame mapping, hot points, dimensions, sprite type), read-only. The frame RLE format was reverse-engineered from `Sprite::DrawLow565` (`spritelow.cpp`): per frame, `Pixel16 frame[0]` skipped, `Pixel16 table[height]` of per-row offsets (`0xFFFF` = empty row), then forward-read RLE runs (`0x0A` chromakey/transparent, `0x0C` copy, `0x0E` shadow, `0x0F` feathered; row ends when the tag high nibble is set). Stored pixels are RGB565 (confirmed by `spriteutils_ConvertPixelFormat`, which converts 565->555 at load). Verified on `GU04.SPR`: 99 PNGs (MOVE 5x11, ATTACK 5x8, IDLE 1x4), all 96x72, ~14% opaque, and an ASCII silhouette of MOVE frame 0 renders a clearly coherent humanoid unit — decode confirmed correct. v0/v1 payloads are raw; **v2 (LZW1) pixel decode is not yet implemented** (the tool reports header/metadata and exits with a note for v2). Draw flags (transparency/fog/desaturate) are runtime render options, not stored per frame, so they are documented rather than exported.

- [x] Add a visual-parity check against the current CPU sprite path for one sprite/action/facing/frame set.
Parity check done (2026-07-02): `spr_export.py --verify` applies the invariant the engine's non-clipped `DrawLow565` relies on — a decoded row must never advance past the scanline width (trailing transparent pixels are implicit/unencoded, so rows legitimately end at `x <= width`; a row that pushes `x > width` would corrupt the next scanline). Ran across all 151 v0/v1 `GU*.SPR` unit sprites: **151 OK, 0 overflow**. Combined with the coherent humanoid silhouette rendered from `GU04.SPR` MOVE frame 0, this is strong format-level parity evidence. (This is a structural/format parity check plus visual confirmation, not a live pixel-diff against a running engine render.)

- [x] Document the intended layout of the generated modern assets and the fallback rule: load generated assets when valid, otherwise use legacy loaders.

Design doc done (2026-07-02): `docs/modern-assets.md` captures the agreed direction — original data stays canonical and user-supplied; a persistent (not disposable) modern asset set is generated **locally** from the user's own data into `~/.ctp2/assets/<source-fingerprint>/`; target format is a **packed texture atlas** (PNG then optionally KTX2) plus a per-unit JSON manifest with frame rects and hot points; conversion is an **offline Python tool** extending the existing decoders; the engine loads modern assets when a valid fingerprinted set exists, else falls back to legacy `.SPR` loaders. It also records the licensing finding the owner asked for: game data is **not** part of the Activision/Apolyton source release and must be user-supplied, so converting is a personal, local format shift — generated (and original) assets must never be redistributed or committed. Full converter + engine first-run integration is scoped as a separate later milestone.

### M9: Close-Out

- [ ] Write a "Remaining Legacy-Risk Areas" section in this file consolidating: leftover warning categories (M2 note), ratchet counts (M3 note), deferred systems (network, movies, Windows project files), and any M7/M8 caveats.
- [ ] Final verification pass (`make test`, `make ubsan-smoke`, clean worktree); update Overall Progress and declare the Definition of Done met.

### M11: Memory-Safety Refactoring (priority phase)

Scope requested by Olek (2026-07-02): systematically remove legacy manual memory management from the first-party engine — raw `new`/`delete`, `malloc`/`calloc`/`realloc`/`free`, and the raw-owning-pointer patterns around them — moving ownership to RAII / smart pointers (`std::unique_ptr`, `std::vector`, `std::string`, containers) without changing behavior.

The per-file worklist lives in **`docs/memory-refactor-checklist.md`** — one checkbox per affected file (574 first-party files, ~6880 raw `new`/`delete`/alloc matches), grouped by module, each tagged with a difficulty label. This keeps the burn-down out of the short board here.

#### Progress snapshot (2026-07-02)

Authoritative metric is the modernization ratchet (enforced by `make test`); the per-file `rg` counts are original-baseline size hints (noisy, include comments/strings).

| Ratchet counter | M11 start | Now | Removed |
| --- | --: | --: | --: |
| `raw_new` | 5528 | **5479** | 49 |
| `raw_delete` | 2065 | **2019** | 46 |
| `c_allocation` | 516 | **515** | 1 |

Files ticked in the checklist: **9 / 574**. (Ratchet reductions run ahead of ticked files because most touched files still have a harder residual cluster — e.g. `Sprite.cpp`, the sprite-group family — so their box stays open even though their easy locals are already RAII.)

#### Difficulty breakdown (heuristic triage of the 565 remaining files)

Labels are grep-derived (see the legend in `docs/memory-refactor-checklist.md`) — a triage sort, not verified verdicts. Difficulty maps: **easy** = self-contained local owner (compiler-enforced); **medium** = single-owner member / needs analysis; **hard** = factory returns crossing callers; **very hard** = linked lists, `void*` handoffs, member pointer arrays, mixed `new[]`/`malloc` (real double-free risk, multi-file blast radius); **skip** = game-lifetime singletons, pools/arenas, vendored — usually correct as-is, converting adds risk for no gain.

| Difficulty | Files | Unsafe lines | Notes |
| --- | --: | --: | --- |
| 🟢 easy | 8 | 33 | mechanical; safe for a Sonnet `/goal` loop |
| 🟡 medium | 409 | 3387 | the bulk; one owner-cluster per commit |
| 🔴 hard | 4 | 96 | change return type, let the compiler guide callers |
| 🔴🔴 very hard | 43 | 733 | supervised (Opus); reason-tagged in checklist |
| ⚪ skip | 101 | 2609 | don't mechanically convert |
| ✅ done | 9 | 22 | committed this phase |
| **total** | **574** | **6880** | |

**Realistic finish line:** the ~38% in `skip` (+ much of `very hard`) should be *encapsulated behind clear owners*, not rewritten. The productive automatable target is 🟢 + 🟡 ≈ **3420 lines / 417 files**; the 🔴 47 files are the supervised tail.

#### Where the work is (per-module difficulty × unsafe-line total)

| Module | easy | med | hard | v.hard | skip | lines |
|---|--:|--:|--:|--:|--:|--:|
| `ui/interface` | 1 | 51 | 1 | 17 | 48 | 1413 |
| `gs/gameobj` | 4 | 77 | 0 | 0 | 12 | 938 |
| `gs/slic` | 0 | 14 | 2 | 1 | 4 | 927 |
| `ui/netshell` | 1 | 26 | 0 | 1 | 1 | 518 |
| `ui/aui_ctp2` | 1 | 49 | 0 | 3 | 4 | 469 |
| `net/general` | 1 | 22 | 0 | 0 | 5 | 396 |
| `ui/aui_common` | 0 | 32 | 0 | 5 | 1 | 288 |
| `gfx/spritesys` | 0 | 5 | 0 | 9 | 0 | 238 |
| `test/cpp` | 0 | 18 | 1 | 0 | 1 | 221 |
| `gs/utility` | 0 | 9 | 0 | 0 | 1 | 195 |
| `gs/fileio` | 0 | 4 | 0 | 0 | 4 | 155 |
| `gs/world` | 0 | 11 | 0 | 0 | 3 | 126 |
| `ctp` | 0 | 3 | 0 | 0 | 3 | 124 |
| `ctp/ctp2_utils` | 0 | 8 | 0 | 0 | 1 | 120 |
| `gfx/tilesys` | 0 | 4 | 0 | 5 | 0 | 93 |
| `gs/database` | 0 | 10 | 0 | 0 | 1 | 64 |
| _(other 27 modules)_ | 0 | 84 | 0 | 2 | 12 | ~495 |

`gs/slic` and networking (`ui/netshell`, `net/general`) carry big line counts but are **deferred** per the phase rules (generated/wire code); `gs/newdb`/`gs/dbgen` should be fixed at the generator, not the output.

#### 🟢 Easy files — do these first (8)

`gs/gameobj/AgreementData.cpp` (5/1/0) · `gs/gameobj/MovePath.cpp` (2/2/0) · `gs/gameobj/Pollution.cpp` (2/1/0) · `gs/gameobj/TradePool.cpp` (2/2/0) · `net/general/net_endgame.cpp` (2/1/0) · `ui/aui_ctp2/keypress.cpp` (4/1/0) · `ui/interface/progresswindow.cpp` (3/3/0) · `ui/netshell/ns_customlistbox.h` (1/1/0)

#### 🔴 Hard + very-hard tail — supervised (47, biggest first)

`ui/netshell/netfunc.cpp` (63/14/1) · `ui/interface/sciencewin.cpp` (55/18/0) · `gfx/spritesys/spritefile.cpp` (45/2/0) · `ui/interface/loadsavewindow.cpp` (15/40/0) · `ui/aui_common/aui_ldl.cpp` (32/13/0) · `ui/interface/spriteeditor.cpp` (9/26/0) · `gs/slic/slicif.cpp` (6/3/17) · `ui/interface/spnewgamescreen.cpp` (21/2/0) · `gfx/tilesys/tileutils.cpp` (15/5/7) · `ui/interface/scenarioeditor.cpp` (19/9/0) · `gs/slic/SlicStruct.cpp` (12/8/0) · `gfx/spritesys/UnitSpriteGroup.cpp` (15/13/0) · `ui/interface/EditQueue.cpp` (18/8/0) · `gfx/tilesys/tileset.cpp` (9/13/0) · `test/cpp/doctest.h` (13/33/0) · `ui/aui_ctp2/chart.cpp` (10/7/0) · `gfx/spritesys/Sprite.cpp` (3/10/4) · `ui/interface/greatlibrary.cpp` (11/5/0) · `ui/interface/loadsavemapwindow.cpp` (10/6/0) · `gfx/spritesys/effectspritegroup.cpp` (9/7/0) · `gfx/spritesys/goodspritegroup.cpp` (8/8/0) · `gs/slic/SlicBuiltin.h` (7/0/0) · `ui/interface/dipwizard.cpp` (4/8/0) · `ui/interface/diplomacywindow.cpp` (3/9/0) · `ui/interface/controlpanelwindow.cpp` (7/4/0) · `ui/aui_common/aui_ranger.cpp` (5/6/0) · `gfx/spritesys/FacedSpriteWshadow.cpp` (0/8/3) · `net/io/net_anet.cpp` (5/5/0) · `ui/interface/chatbox.cpp` (4/4/0) · `gfx/spritesys/FacedSprite.cpp` (0/8/0) · `gfx/tilesys/workmap.cpp` (1/6/0) · `ui/interface/DiplomacyDetails.cpp` (3/3/0) · `ui/interface/trademanager.cpp` (4/1/0) · `ui/interface/intelligencewindow.cpp` (4/0/0) · `ui/interface/UnitControlPanel.cpp` (2/2/0) · `ui/interface/unitmanager.cpp` (4/0/0) · `gfx/spritesys/spriteutils.cpp` (2/0/2) · `gfx/spritesys/SpriteGroup.cpp` (0/4/0) · `gfx/tilesys/BaseTile.cpp` (2/2/0) · `gfx/tilesys/resourcemap.cpp` (1/3/0) · `ui/interface/c3dialogs.cpp` (2/1/0) · `ui/aui_common/aui_tab.cpp` (2/1/0) · `ui/aui_common/aui_textbase.cpp` (2/1/0) · `ui/aui_ctp2/ctp2_menubar.cpp` (2/0/0) · `ui/aui_common/aui_win.cpp` (1/1/0) · `gs/events/GameEventArgument.cpp` (1/1/0) · `ui/aui_ctp2/c3_updateaction.cpp` (1/0/0)

Recompute this breakdown after big clusters land: re-run the triage classifier over the checklist (heuristic) and refresh the ratchet snapshot from `tools/modernization/ratchet_baseline.json`.

Ground rules for this phase:

- **First-party only.** Vendored `libs/**` (anet, freetype, tiff, zlib, miles, etc.) are upstream code and are excluded from the checklist — do not refactor them.
- **Deferred sub-areas stay low priority within the phase:** networking (`net/**`, `ui/netshell/**`) is deferred per M7, and generated DB code (`gs/newdb`, `gs/dbgen`) should be fixed at the generator, not the output. Tests (`test/cpp`) are optional cleanup.
- **One file (or one clear ownership cluster) per commit.** Preserve behavior exactly; no drive-by logic changes.
- **Verify every batch** with `mise exec -- make test` (and `make ubsan-smoke` when touching headless/game-loop code), then **lower the ratchet baseline** (`tools/modernization/ratchet_baseline.json`) so the reduction is locked in and can't regress.
- Prefer the smallest, clearest-ownership clusters first (a `new` with an obvious single `delete` in the same scope → `unique_ptr`/stack object) before tackling shared-ownership or hand-rolled containers.
- The crude `rg` counts include false positives (the word "new"/"delete" in comments/strings, placement new, `operator delete` overrides); confirm real ownership before editing, and it is fine to tick a file whose remaining matches are all non-ownership noise.

Progress is tracked as files ticked in the checklist, not as a single board checkbox. This phase is **beyond the original Definition of Done** (which M1–M9 cover) but is the current top priority; the modern-asset converter (M10) stays parked until it is well underway.

## Progress Rules

- A normal `continue` should complete one checkbox or one coherent part of a checkbox.
- Update the Overall Progress counts and the Milestone Board `Status` column in the same commit as the checkbox change.
- If a task discovers a blocker, add or update a blocker/caveat instead of pretending progress happened.
- Prefer many small commits over one broad refactor commit.

## Next Best Batches

Priority updated 2026-07-02 (Olek): the modern-asset converter (M10) is **parked** now that the M8 spike proved the format is decodable; the mechanical memory refactoring is more important. Do these in order:

1. M9: close-out documentation pass (consolidate legacy-risk areas, declare DoD) — reaches the original Definition of Done. Small.
2. M11 (**new priority**): mechanical memory-safety refactoring — reduce the large raw `new`/`delete`/C-allocation clusters (ratchet now `raw_new=5479`, `raw_delete=2019`, `c_allocation=515`; see the M11 progress snapshot + difficulty breakdown above) toward RAII/smart-pointer ownership, in small behavior-preserving batches, lowering the ratchet baseline as each cluster clears. Start with the 8 🟢 easy files, then work 🟡 medium clusters by module; leave the 🔴 47-file tail for supervised sessions. This reopens M3's burn-down as a focused effort rather than opportunistic side work.
3. M10 (parked, follow-on): offline modern-asset converter + engine modern-first loader; resume only after the memory work. See `docs/modern-assets.md`.

## Assistant Protocol

For any LLM continuing this work:

- Read this file first, then `git log --oneline -10`, then `git status --short`.
- Prefer one small batch per commit.
- Verify with `mise exec -- make test` and `git diff --check`.
- Use `mise exec -- make ubsan-smoke` after touching headless/game-loop code.
- Do not treat "fully refactored" as the goal unless Olek explicitly redefines scope.
- Do not make converted assets canonical; preserve original data/mod compatibility and treat generated modern assets as rebuildable cache/output.
- Warning cleanup and ratchet reductions are post-DoD opportunistic work now (M2/M3 complete); do them only as small, obvious side batches.

## Last Known Verification Commands

```sh
mise exec -- make test
mise exec -- make ubsan-smoke
```

## Known Blockers / Caveats

- `make sanitized-smoke` exists, but ASan currently hangs before `main` on this macOS/Apple clang setup; see M1 stack summary.
- `make ubsan-smoke` is the working sanitizer smoke path.
- Windows is best-effort and should not drive refactoring decisions unless Olek asks.
- Avoid `MBCHAR *` UI string-literal cleanup unless intentionally doing a broader UI const-correctness batch.
