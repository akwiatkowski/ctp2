# M11 — Memory-Safety Refactoring Checklist

<!-- Generated and maintained by Claude. One checkbox per first-party file that
contains raw `new`/`delete`/`malloc`-family usage. Third-party `libs/**` are
excluded (upstream code). Counts are crude `rg` matches (the same metric the
modernization ratchet uses) and include some false positives in comments/strings;
treat them as a size hint, not an exact task count. Tick a file only after its raw
ownership is moved to RAII/smart pointers with `make test` green and no behavior
change, then lower the ratchet baseline. -->

**Scope:** 574 first-party files (excludes vendored `libs/**`). Work smallest /
clearest-ownership clusters first.

> **Counts re-based 2026-08-03.** The ratchet used to grep raw file text, so
> comments and string literals counted as legacy usage — `raw_new` fell 4800 →
> 4187 and `raw_delete` 1839 → 1737 once it started skipping them (97ba0ee8).
> The per-file `(new / delete / alloc)` figures below are the OLD text-based
> numbers and still include prose; treat them as a size hint only, as the
> header already warns.
>
> Session of 2026-08-03 against the re-based baseline: `raw_delete` 1737 →
> 1611, `raw_new` 4187 → 4174, `pointerlist_uses` 573 → 567. Two themes ran
> through it — sprite frame/group ownership (which fixed a real scalar-delete
> on array-new'd memory), and UI controls held as `std::unique_ptr` members.
>
> Two traps worth knowing before the next batch: `DeleteControl` (was
> `RemoveControl`) is a macro in `UIUtils.h` that deletes, so it is an
> invisible release site; and several UI headers declare many small classes
> that reuse member names like `m_text` with different types, which defeats a
> type-driven sweep.

Legend: `(new / delete / alloc)` match counts per file.

## Triage classification

Each unticked file carries a **heuristic** difficulty tag (appended after the counts).
These are grep-derived triage labels — a starting sort, **not** verified verdicts.
Confirm ownership before converting; some labels will be wrong.

> **Verified 2026-07-02:** all 8 files the heuristic tagged 🟢 _clean_ turned out to be
> false positives on inspection — every one is a transfer-to-sink (`Execute`, `AddEvent`,
> `InsertItem`, pool), a **reference-counted** type (`SlicObject` via `AddRef`/`Release`),
> or a global singleton (`theKeyMap`, `wormhole_Get/Set`). The grep signal
> "lowercase local `= new` + `delete` same name" cannot distinguish "delete on the
> failure path, transfer on success" from a genuine local owner. Those 8 were
> reclassified (see below); **the 🟢 bucket is now empty.** Treat 🟡 and 🔴 as
> "needs ownership analysis" — the counts below are approximate, not audited.

| Tag | Meaning | Approach |
|-----|---------|----------|
| 🟢 **clean**    | Self-contained local owner (parse `Token`, `unique_ptr(new)` idiom, small local scratch). | Mechanical; compiler enforces the boundary. Safe for a Sonnet `/goal` loop. |
| 🟡 **moderate** | Single-owner member, or unclear — needs ownership analysis. | Read the file, confirm one owner, convert per-cluster. |
| 🔴 **hard**     | Linked lists, `void*` handoffs across callbacks, mixed `new[]`/`malloc`, member pointer arrays, factory returns crossing modules. | Supervised (Opus); real double-free risk; often touches multiple files. |
| ⚪ **leave**    | Game-lifetime singletons (`g_*`/`s_*`), pool/arena allocators, vendored code. | Usually correct as-is; converting adds risk for no safety gain. Encapsulate, don't rewrite. |


> **2026-09-20 mega-batch:** 143 files ticked in one 9-agent sweep (ratchet
> `raw_new` 4096→3601, `raw_delete` 1393→1028). ~90 further files were triaged
> but left unticked — their remaining sites are refcounted (SlicObject, COM
> mapgens), transfer-to-sink (AddEvent/Enqueue/AddChild/AddItem), pool
> allocators, intrusive lists, or union members. Those are annotated
> "triaged 2026-09-20" inline.
**Distribution (565 unticked files, 6858 raw matches; post-verification 2026-07-02):**

| Tag | Files | Raw matches |
|-----|------:|------------:|
| 🟢 clean    |   0 |    0 |
| 🟡 moderate | 413 | 3403 |
| 🔴 hard     |  49 |  838 |
| ⚪ leave    | 103 | 2617 |

Takeaway: **~38% of the raw matches (leave + a chunk of hard) shouldn't be mechanically
converted at all** — the realistic finish line is lowering the ratchet to a floor, not zero.
There is **no free "easy" tier** — every remaining conversion needs ownership analysis
(the 🟢 verification proved the grep heuristic can't find genuinely-trivial locals here;
CTP2's raw `new`/`delete` are dominated by transfer/refcount/singleton patterns). The 🟡
bucket is the productive target, converted per owner-cluster; the 🔴 49 files are the
supervised tail. Re-run `tools/modernization/` triage after big clusters land.

## ui/interface  (118 files, 1413 matches)

- [x] `ui/interface/knowledgewin.cpp` (53/37/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/interface/infowin.cpp` (84/6/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/interface/spnewgamewindow.cpp` (4/69/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-08-26: all ~60 member-control deletes across SPNewGameWindow/SPProfileBox/SPWorldBox/SPRulesBox/SPDropDownListItem → `unique_ptr` members (the 69 deletes were the dtor sweep); local `spNewStringTable` scratch → RAII; only sink-owned list items remain.
- [x] `ui/interface/sciencewin.cpp` (55/18/0) — 🔴 hard · _void* ownership handoff_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/battleviewwindow.cpp` — done 2026-08-03: all 31 owned controls are unique_ptr members; destructor is just the global back-pointer reset
- [x] `ui/interface/loadsavewindow.cpp` (15/40/0) — 🔴 hard · _void* ownership handoff_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/EndgameWindow.cpp` (31/14/0) — 🟡 moderate · _single-owner member_ · DONE 2026-08-26: all 14 child controls → `unique_ptr`, 7 LDL-driven control arrays → `vector<unique_ptr>`, `c3_Animation::m_frames` → `unique_ptr`, blend scratch surfaces RAII'd; counts kept as LDL-driven sizes, in-class initializers replace `CleanPointers`.
- [x] `ui/interface/spriteeditor.cpp` (9/26/0) — 🔴 hard · _void* ownership handoff_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/victorywin.cpp` (31/3/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-08-26: victory/high-score statics → `unique_ptr`, both `mycleanup` macros deleted; `s_staticControls`/`s_wonderIcons` were never owners — now typed `std::array` registries of borrowed LDL controls; `HighScoreWindowPopup` members → `unique_ptr`.
- [x] `ui/interface/creditsscreen.cpp` (17/17/0) — 🟡 moderate · _single-owner member_ · DONE 2026-08-26: members → `unique_ptr`, anim arrays + credits pages/lines → `vector`, `Parse` no longer `delete this` (bool return), blend-scratch surfaces RAII'd; fixed font-index off-by-one (`>` → `>=`) and null-font deref on failed load.
- [x] `ui/interface/messageeyepoint.cpp` (20/10/0) — 🟡 moderate · _single-owner member_ · DONE 2026-08-26: buttons/dropdowns/actions → `unique_ptr` members (`SetAction`/`AddControl`/`AddItem` verified non-owning), `m_action1/2` renamed `m_actionLeft/Right`; only sink-transfer `new`s remain.
- [x] `ui/interface/ancientwindows.cpp` (13/15/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/scenarioeditor.cpp` (19/9/0) — 🔴 hard · _void* ownership handoff_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/messagewindow.cpp` — done 2026-08-03: 12 controls are unique_ptr, including 4 border bars that were leaking (allocated, AddControl'd, never freed)
- [x] `ui/interface/EditQueue.cpp` (18/8/0) — 🔴 hard · _void* ownership handoff_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/spnewgamescreen.cpp` (21/2/0) — 🔴 hard · _factory returns crossing callers_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/sci_advancescreen.cpp` (17/4/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/battleview.cpp` — done 2026-08-03: m_activeEvents held by value (partial: other raw owners remain)
- [x] `ui/interface/controlpanel.cpp` (3/16/0) — DONE 2026-09-21: no live raw new/delete remain (only comments)
- [x] `ui/interface/messageresponse.cpp` — done 2026-08-03: 4 owned members are unique_ptr; the two tech_WLLists left raw (they own their entries)
- [x] `ui/interface/loadsavemapwindow.cpp` — done 2026-08-03: 9 window controls are unique_ptr; list-item members left raw (c3_ListItem frees children)
- [x] `ui/interface/messageadvice.cpp` — done 2026-08-03: 5 controls are unique_ptr; list ITEMS still deleted by hand (not members)
- [x] `ui/interface/spnewgameplayersscreen.cpp` (14/2/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/messagemodal.cpp` (10/6/0) — 🟡 moderate · _single-owner member_ · DONE 2026-08-26: text box/eye-point helpers → `unique_ptr` members (replacing the raw union on the modal side), response button/action `tech_WLList`s → `vector<unique_ptr>`; **fixed a real leak** — `~MessageModal` never freed the eye-point helper (its sibling `~MessageWindow` did).
- [x] `ui/interface/messagewin.cpp` (9/7/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-08-26: `g_messageUserList` → `vector<unique_ptr<MessageList>>` (was an owning global `tech_WLList`); deleted ~230 lines of `#if 0` CtP1 icon-function bodies behind unconditional early returns.
- [x] `ui/interface/greatlibrary.cpp` (11/5/0) — 🔴 hard · _void* ownership handoff_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/battleevent.cpp` (7/8/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/citywindow.cpp` (9/5/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/wondermoviewindow.cpp` — done 2026-08-03: 7 owned controls are unique_ptr members; AddControl takes .get() (non-owning)
- [x] `ui/interface/hotseatlist.cpp` (10/4/0) — 🟡 moderate · _single-owner member_ · DONE 2026-08-26: window/list → `unique_ptr` (list items stay sink-owned via `c3_ListBox::Clear`), legal-civ flag array → `vector<bool>`, scenario `SaveInfo` local → RAII.
- [x] `ui/interface/tileimptracker.cpp` (12/2/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/km_screen.cpp` (10/3/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/dipwizard.cpp` (4/8/0) — 🔴 hard · _void* ownership handoff_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/NationalManagementDialog.cpp` (7/5/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive/API-boundary · DONE 2026-09-21
- [x] `ui/interface/diplomacywindow.cpp` (3/9/0) — 🔴 hard · _void* ownership handoff_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/controlpanelwindow.cpp` (7/4/0) — 🔴 hard · _void* ownership handoff_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/scorewarn.cpp` (10/1/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/battle_observer_adapter.cpp` (8/3/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/armymanagerwindow.cpp` (7/3/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/tutorialwin.cpp` (9/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive/API-boundary · DONE 2026-09-21
- [x] `ui/interface/loadsavescreen.cpp` (7/2/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/MessageBoxDialog.cpp` (8/1/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/spnewgamerulesscreen.cpp` (6/3/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/spnewgamemapsizescreen.cpp` (5/3/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/custommapscreen.cpp` (6/2/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/testwindow.cpp` (4/4/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/messagelist.cpp` (3/5/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-08-26: icon list → `vector<unique_ptr<MessageIconWindow>>` (windows stay list-owned via release-after-link); dead `GetList` accessor replaced by `GetTailIcon`/`GetIconCount`.
- [x] `ui/interface/battle.cpp` (7/1/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/SpecialAttackWindow.cpp` (4/4/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/spnewgametribescreen.cpp` (7/1/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/chatbox.cpp` (4/4/0) — 🔴 hard · _void* ownership handoff_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/workwin.cpp` (4/3/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/infowindow.cpp` (4/3/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/spnewgamerandomcustomscreen.cpp` (5/2/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/graphicsresscreen.cpp` (5/2/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/agesscreen.cpp` (7/0/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/ControlTabPanel.cpp` (6/0/0) — ✅ done · `m_ldlBlock` now uses `unique_ptr<MBCHAR[]>`, fixing the `new[]`/scalar-delete mismatch; local formatter uses `snprintf`.
- [x] `ui/interface/progresswindow.cpp` (3/3/0) — DONE 2026-09-21: private dtor — raw new/delete preserved (make_unique can't name it); documented
- [x] `ui/interface/text_hasher.h` (2/4/0) — 🟡 moderate · _single-owner member_ · DONE 2026-08-26: bucket array → `vector<unique_ptr<Translation>>`, chain links → unique_ptr via m_next; Translation keeps ownership of the copied key/data payloads (contract unchanged).
- [x] `ui/interface/statswindow.cpp` (3/3/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/MainControlPanel.cpp` (6/0/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/DiplomacyDetails.cpp` (3/3/0) — 🔴 hard · _void* ownership handoff_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/MapCopyBuffer.cpp` (2/4/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-08-26: jagged `CellInfo**` new[] grid → flat `vector<CellInfo>` with an At(x,y) accessor (column-major like the original); scenarioeditor's `m_copyBuffer`/`m_fileDialog` → unique_ptr.
- [x] `ui/interface/spnewgamemapshapescreen.cpp` (3/2/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/interface/trademanager.cpp` (4/1/0) — 🔴 hard · _void* ownership handoff_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/messageiconwindow.cpp` (2/3/0) — 🟡 moderate · _single-owner member_ · DONE 2026-08-26: icon button + open action → `unique_ptr` (accessors return `.get()`); the tip-window delete is aui tip ownership, left as-is.
- [x] `ui/interface/musictrackscreen.cpp` (5/0/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/scenariowindow.cpp` (4/1/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/rankingtab.cpp` (2/2/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-08-26: `GenrateGraph` lost its `double***` out-param — SetLineData copies, so the grid is function-local; m_infoGraphData/CleanupGraph gone from rankingtab + timelinetab (+ loadsavewindow's manual free). **Also fixed**: timelinetab.cpp:135 scalar-deleted the array-new'd row-pointer array.
- [x] `ui/interface/spnewgamediffscreen.cpp` (3/1/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/citymanager.cpp` (3/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-08-26: window global → static unique_ptr; OK/Cancel buttons → unique_ptr; `m_bg` stays raw (registry-owned via UnloadImage, documented).
- [x] `ui/interface/intelligencewindow.cpp` (4/0/0) — 🔴 hard · _void* ownership handoff_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/CityEspionage.cpp` (3/1/0) — DONE 2026-09-21: private ctor/dtor — raw new/delete preserved; documented
- [x] `ui/interface/helptile.cpp` (3/1/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/UnitControlPanel.cpp` (2/2/0) — 🔴 hard · _void* ownership handoff_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/messageactions.cpp` (4/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-08-26 (tick-only): all four `new`s are transfer-to-sink `AddAction` calls — no ownership to convert.
- [x] `ui/interface/unitmanager.cpp` (4/0/0) — 🔴 hard · _void* ownership handoff_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/AttractWindow.cpp` (2/2/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-08-26: global → static unique_ptr; owning `PointerList<AttractRegion>` → `vector<unique_ptr>` (also a P9 container conversion, −1 pointerlist).
- [x] `ui/interface/DomesticManagementDialog.cpp` (3/1/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/soundscreen.cpp` (1/2/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/c3dialogs.cpp` (2/1/0) — 🔴 hard · _void* ownership handoff_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/victorymoviewin.cpp` (2/1/0) — DONE 2026-09-21: AddAction(new X) → AddAction(make_unique<X>)
- [x] `ui/interface/loadsavemapscreen.cpp` (1/2/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-20: partial conversion; remaining sites are sinks/singletons/borrowed · DONE 2026-09-21
- [x] `ui/interface/IntroMovieWin.cpp` (2/1/0) — DONE 2026-09-21: AddAction(new X) → AddAction(make_unique<X>)
- [x] `ui/interface/StatusBar.h` (1/2/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/wondermoviewin.cpp` (2/1/0) — DONE 2026-09-21: AddAction(new X) → AddAction(make_unique<X>)
- [x] `ui/interface/gameplayoptions.cpp` (1/2/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/interface/optionwarningscreen.cpp` (1/2/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/interface/sciencevictorydialog.cpp` (1/1/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/interface/spnewgameplayersscreen.h` (2/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/spnewgamerulesscreen.h` (2/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/graphicsscreen.cpp` (1/1/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/interface/testwin.cpp` (1/1/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/interface/TurnYearStatus.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/greatlibrarywindow.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/ScienceManagementDialog.cpp` (1/1/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/interface/IntroMovieWindow.cpp` (1/1/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/scoretab.cpp` (1/1/0) — ✅ done · `m_difficultyStrings` is now a `unique_ptr` single-owner member.
- [x] `ui/interface/victorymoviewindow.cpp` (1/1/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/cursormanager.cpp` (1/1/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/interface/optionsscreen.cpp` (2/0/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/interface/backgroundwin.cpp` (1/1/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/interface/ProfileEdit.cpp` (1/1/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/interface/timelinetab.cpp` (0/2/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/cursormanager.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/spnewgametribescreen.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/initialplaywindow.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/interface/screenutils.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/interfaceevent.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/splash.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/interface/WonderTab.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/musicscreen.cpp` (1/0/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/interface/screenutils.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/initialplayscreen.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/scenarioeditor.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/dipwizard.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/interface/optionswindow.cpp` — done 2026-08-03: 13 controls are unique_ptr; local mycleanup macro gone, destructor empty
- [x] `ui/interface/UIUtils.h` (0/1/0) — DONE 2026-09-21: DeleteControl macro → template<class T> void DeleteControl(T*&)

## gs/gameobj  (93 files, 938 matches)

- [x] `gs/gameobj/Player.cpp` (193/33/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/gameobj/ArmyData.cpp` (121/17/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/gameobj/CityData.cpp` (68/2/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/gameobj/UnitData.cpp` (51/7/0) — 🟡 moderate · _single-owner member_ · DONE 2026-08-26: cargo list/city data/round-the-world mask → unique_ptr (JSON bridge + net_unit updated; GetCargoList/GetCityData return `.get()`); the unit-type-change path's silent leak of a prior cargo list is gone (reset frees it). m_lesser/m_greater stay raw (intrusive pool links, documented). Remaining news are SlicObject/Net sinks.
- [x] `gs/gameobj/DiplomaticRequestData.cpp` (46/2/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-08-26: Accept/Reject switches fill a scoped `unique_ptr<SlicObject>` (bail paths just return); the single tail `Execute(so.release())` keeps the sink contract; so2 likewise.
- [x] `gs/gameobj/bldque.cpp` (42/5/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/gameobj/endgame.cpp` (22/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-08-26 (tick-only): SlicObject/NetEndGame sink transfers only.
- [x] `gs/gameobj/Vision.cpp` (12/9/0) — ✅ done · _m_unseenCells -> unique_ptr, m_array uint16** -> vector<vector>; quadtree cell-content news left (separate ownership)_
- [x] `gs/gameobj/CityEvent.cpp` (16/2/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-08-26: remaining matches are SlicObject/NetInfo sink transfers + the `CanAskFor` scratch array (now `vector` via Advances API change); ticked as noise-free.
- [x] `gs/gameobj/unitevent.cpp` (16/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-08-26 (tick-only): every `new` is a SlicObject handed to `Execute` (deletes after handling) or a NetInfo sink enqueue — non-ownership noise.
- [x] `gs/gameobj/GoodyHuts.cpp` (14/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-08-26 (tick-only): same SlicObject sink-transfer pattern throughout.
- [x] `gs/gameobj/PlayerEvent.cpp` (14/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-08-26 (tick-only): GameEventArgument args->Add (list owns), NetAction sinks, SlicObject transfers.
- [x] `gs/gameobj/messagedata.cpp` (11/3/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/gameobj/Advances.cpp` (12/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-08-26: CanAskFor/CanOffer/CanResearch converted to vector/ref in the advances-mask batch; remaining news are NetAction sinks.
- [x] `gs/gameobj/armyevent.cpp` (9/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-08-26 (tick-only): sink transfers; the one delete is the game-lifetime combat singleton teardown.
- [x] `gs/gameobj/Civilisation.cpp` (8/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/Wormhole.cpp` (5/4/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/gameobj/FeatTracker.cpp` — done 2026-08-03: m_activeList held by value
- [x] `gs/gameobj/GameObj.cpp` (0/9/0) — DONE 2026-09-21: m_lesser/m_greater/tmp deletes → unique_ptr temporaries; operator delete overloads are pool infrastructure, left
- [x] `gs/gameobj/TradeBids.cpp` (2/5/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/gameobj/Happy.cpp` (5/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-08-26: m_tracker → unique_ptr (JSON bridge reset/reconstruct); remaining news are SlicObject sinks.
- [x] `gs/gameobj/AgreementData.cpp` (5/1/0) — DONE 2026-09-21: SlicObject sites → make_unique + Execute(std::move); conditional delete so2 removed (dtor frees)
- [x] `gs/gameobj/CTP2Combat.cpp` (3/2/0) — ✅ done · _CombatField::m_field 2D array -> vector<vector>; fixed new[]/scalar-delete UB_
- [x] `gs/gameobj/MessagePool.cpp` (5/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/MaterialPool.cpp` (5/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/Order.h` (3/2/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/Unit.cpp` (5/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/Order.cpp` (1/3/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/MovePath.cpp` (2/2/0) — ✅ done · _Path transfer-on-success -> unique_ptr + release_
- [x] `gs/gameobj/TradeRouteData.cpp` (3/1/0) — 🔴 hard · _`*this=*copyme` copy-ctor blocks unique_ptr (needs custom operator=)_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/gameobj/TradePool.cpp` (2/2/0) — 🟡 · _pool + member array owner_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/UnoccupiedTiles.cpp` (2/2/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/barbarians.cpp` (3/1/0) — ⚪ leave · _alloc is in a `/* */`-commented-out function (dead code)_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/gameobj/civilisationpool.cpp` (3/1/0) — 🟡 moderate · _single-owner member_
- [x] `gs/gameobj/ArmyPool.cpp` (4/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/installationtree.h` (2/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/improvementevent.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/TradeOfferPool.cpp` (2/1/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/gameobj/TerrImprovePool.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/EventTracker.cpp` — done 2026-08-03: PointerList member held by value (DeleteAll still frees the pointed-to objects)
- [x] `gs/gameobj/Pollution.cpp` (2/1/0) — DONE 2026-09-21: conditional execute+delete → unique_ptr, else-branch delete removed
- [x] `gs/gameobj/citydata.h` (3/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/DiplomaticRequestPool.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/PlayerTurn.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/TradeOfferData.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/combatevent.cpp` (1/2/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/CriticalMessagesPrefs.cpp` — done 2026-08-03: PointerList member held by value (DeleteAll still frees the pointed-to objects)
- [x] `gs/gameobj/CTP2Combat.h` (3/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/terrainutil.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/Readiness.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/GameObj.h` (0/3/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/UnitPool.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/Agreement.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/TaxRate.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/MessagePool.h` (1/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/EventTracker.h` (2/0/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/gameobj/buildingutil.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/Resources.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/TradeRoute.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/FeatTracker.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/wonderutil.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/CivilisationData.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/AchievementTracker.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/installationdata.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/Gold.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/TradeOffer.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/ArmyPool.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/GameSettings.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/TerrImprove.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/Diffcly.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/TradeOfferPool.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/WonderTracker.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/TerrImprovePool.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/UnitTypes.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/UnitData.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/TradePool.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/installation.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/pollution.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/Wormhole.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/DiplomaticRequestPool.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/AgreementPool.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/CriticalMessagesPrefs.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/CivilisationData.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/PlayHap.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/DiplomaticRequest.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/WonderTracker.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/Diffcly.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/unitutil.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/GameSettings.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/installationpool.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/gameobj/installationpool.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/ID.h` (0/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/gameobj/ArmyData.h` (0/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch

## gs/slic  (21 files, 927 matches)

- [x] `gs/slic/SlicEngine.cpp` — done 2026-08-03: ui-execute and context lists held by value; SlicObject entries stay reference counted (partial: other raw owners remain)
- [x] `gs/slic/SlicBuiltin.cpp` (79/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer-to-sink · DONE 2026-09-21
- [x] `gs/slic/SlicContext.cpp` (26/25/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `gs/slic/slicfunc.cpp` (33/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive/API-boundary · DONE 2026-09-21
- [x] `gs/slic/slicif.cpp` (6/3/17) — DONE 2026-09-21: yacc parser interface; malloc/free on C POD structs (g_slicObjectArray) left — converting needs struct redesign, not mechanical RAII
- [x] `gs/slic/SlicSegment.cpp` (9/8/7) — DONE 2026-09-21: operator new/delete overloads are pool infrastructure, left; Call/GEVHookCallback SlicObject sites → make_unique + Execute(std::move)
- [x] `gs/slic/SlicStruct.cpp` (12/8/0) — 🔴 hard · _factory returns crossing callers_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/slic/SlicSymbol.cpp` (6/10/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/slic/slicobject.cpp` (10/5/0) — 🟡 moderate · _single-owner member_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/slic/SlicSymTab.cpp` (4/4/6) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `gs/slic/sliccmd.cpp` (6/2/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/slic/SlicFrame.cpp` (5/3/0) — 🟡 moderate · _single-owner member_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/slic/StringHash.h` (2/5/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `gs/slic/SlicButton.cpp` (5/2/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/slic/SlicBuiltin.h` (7/0/0) — 🔴 hard · _factory returns crossing callers_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/slic/SlicArray.cpp` (3/3/0) — 🟡 moderate · _single-owner member_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/slic/SlicEyePoint.cpp` (4/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/slic/SlicSegment.h` (1/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/slic/SlicStruct.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/slic/SlicDBConduit.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/slic/QuickSlic.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21

## ui/netshell  (29 files, 518 matches)

- [x] `ui/netshell/allinonewindow.cpp` — done 2026-08-03: 9 owned members are unique_ptr; m_aiplayerList (tech_WLList) left raw
- [x] `ui/netshell/netfunc.cpp` (63/14/1) — 🔴 hard · _mixed new[]/malloc buffers_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/netshell/netshell.cpp` (28/8/0) — 🟡 moderate · _single-owner member_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/netshell/lobbywindow.cpp` (27/4/0) — 🟡 moderate · _single-owner member_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/netshell/ns_customlistbox.cpp` (16/11/0) — 🟡 moderate · _single-owner member_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/netshell/playereditwindow.cpp` (25/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/netshell/gameselectwindow.cpp` (21/2/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/netshell/dialogboxwindow.cpp` (16/4/0) — 🟡 moderate · _single-owner member_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/netshell/passwordscreen.cpp` (15/2/0) — ⚪ leave · _mostly g_/s_ singletons_ · DONE 2026-09-21 RAII batch
- [x] `ui/netshell/playerselectwindow.cpp` (16/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-21 RAII batch
- [x] `ui/netshell/connectionselectwindow.cpp` (12/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-21 RAII batch
- [x] `ui/netshell/ns_item.cpp` (10/2/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-21 RAII batch
- [x] `ui/netshell/serverselectwindow.cpp` (9/2/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-21 RAII batch
- [x] `ui/netshell/lobbychangewindow.cpp` (10/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-21 RAII batch
- [x] `ui/netshell/ns_chatbox.cpp` (5/5/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-21 RAII batch
- [x] `ui/netshell/ns_civlistbox.cpp` (6/2/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-21 RAII batch
- [x] `ui/netshell/ns_listbox.h` (2/4/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-21 RAII batch
- [x] `ui/netshell/netfunc.h` (2/3/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-21 RAII batch
- [x] `ui/netshell/ns_tribes.cpp` (3/1/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-21 RAII batch
- [x] `ui/netshell/ns_window.cpp` (0/4/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-21 RAII batch
- [x] `ui/netshell/ns_customlistbox.h` (1/1/0) — 🟡 · _item transferred to listbox_ · DONE 2026-09-21 RAII batch
- [x] `ui/netshell/ns_improvements.cpp` (1/1/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-21 RAII batch
- [x] `ui/netshell/ns_item.h` (1/1/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-21 RAII batch
- [x] `ui/netshell/ns_units.cpp` (1/1/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-21 RAII batch
- [x] `ui/netshell/ns_units.h` (1/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-21 RAII batch
- [x] `ui/netshell/ns_header.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-21 RAII batch
- [x] `ui/netshell/ns_wonders.cpp` (1/1/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-21 RAII batch
- [x] `ui/netshell/netshell_game.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-21 RAII batch
- [x] `ui/netshell/ns_gamesetup.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-21 RAII batch

## ui/aui_ctp2  (57 files, 469 matches)

- [x] `ui/aui_ctp2/c3windows.cpp` (46/41/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/aui_ctp2/c3_utilitydialogbox.cpp` (37/35/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/battleorderbox.cpp` — done 2026-08-03: 12 controls are unique_ptr; RemoveControl() macro calls became reset() (the macro deletes despite its name)
- [x] `ui/aui_ctp2/c3_popupwindow.cpp` — done 2026-08-03: 4 controls are unique_ptr; default ctor moved out of line (forward-declared control types); m_border array still raw
- [x] `ui/aui_ctp2/chart.cpp` (10/7/0) — 🔴 hard · _void* ownership handoff_ · DONE 2026-09-21 RAII batch
- [x] `ui/aui_ctp2/c3_ranger.cpp` (14/1/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/thronecontrol.cpp` (6/8/0) — 🟡 moderate · _single-owner member_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive/API-boundary · DONE 2026-09-21
- [x] `ui/aui_ctp2/SelItem.cpp` (6/5/0) — 🟡 moderate · _single-owner member_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive/API-boundary · DONE 2026-09-21
- [x] `ui/aui_ctp2/linegraph.cpp` (6/3/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/ctp2_listbox.cpp` (6/3/0) — 🟡 moderate · _single-owner member_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive/API-boundary · DONE 2026-09-21
- [x] `ui/aui_ctp2/c3_listbox.cpp` (6/3/0) — 🟡 moderate · _single-owner member_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive/API-boundary · DONE 2026-09-21
- [x] `ui/aui_ctp2/textbox.cpp` (7/1/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/radarmap.cpp` (1/4/3) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/c3_fancywindow.cpp` (4/4/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 (file deleted (dfd5f015))
- [x] `ui/aui_ctp2/ctp2_dropdown.cpp` (7/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/ctp2_Menu.cpp` (3/4/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/aui_ctp2/c3_hypertextbox.cpp` (4/3/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/c3_dropdown.cpp` (7/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/ctp2_hypertextbox.cpp` (4/2/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/unittabbutton.cpp` — done 2026-08-03: 5 controls are unique_ptr; m_cargo array still freed by hand
- [x] `ui/aui_ctp2/background.cpp` (5/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/aui_ctp2/picturebutton.cpp` (2/4/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/c3ui.cpp` (3/3/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/texttable.cpp` (5/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/aui_ctp2/cityinventorylistbox.cpp` (5/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/aui_ctp2/c3scroller.cpp` (5/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/keypress.cpp` (4/1/0) — ⚪ · _global keymap singleton_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/aui_ctp2/thumbnailmap.cpp` (1/4/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/aui_ctp2/c3_button.cpp` (2/2/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/c3_tradelistitem.cpp` (4/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/aui_ctp2/InfoBar.cpp` (2/2/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/aui_ctp2/c3fancywindow.cpp` (2/2/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 (file deleted (dfd5f015))
- [x] `ui/aui_ctp2/ui_game_observer.cpp` (2/2/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/staticpicture.cpp` (2/2/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/pattern.cpp` (2/2/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/c3spinner.cpp` (4/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/c3_header.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/videowindow.cpp` (1/2/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/grabitem.cpp` (1/2/0) — ⚪ leave · _mostly g_/s_ singletons_ · DONE 2026-09-21 RAII batch
- [x] `ui/aui_ctp2/c3dropdown.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/c3_headerswitch.cpp` (1/1/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/background.h` (1/1/0) — 🟡 moderate · _single-owner member_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/aui_ctp2/player_view_ui.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/aui_ctp2/c3_hypertipwindow.cpp` (1/1/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/c3listbox.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/aui_ctp2/SelItemClick.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/c3memmap.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/aui_ctp2/c3_listitem.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/aui_ctp2/C3slider.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/c3_slidometer.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/ctp2_menubar.cpp` (2/0/0) — 🔴 hard · _void* ownership handoff_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/aui_ctp2/ctp2_textbuffer.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_ctp2/c3_updateaction.cpp` (1/0/0) — 🔴 hard · _void* ownership handoff_ · DONE 2026-09-21 RAII batch
- [x] `ui/aui_ctp2/ctp2_Window.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/aui_ctp2/ctp2_listbox.h` (0/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/aui_ctp2/ctp2_MenuButton.cpp` (0/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/aui_ctp2/picture.cpp` (0/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch

## net/general  (28 files, 396 matches)

- [x] `net/general/network.cpp` (217/27/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `net/general/net_action.cpp` (31/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer-to-sink · DONE 2026-09-21
- [x] `net/general/net_info.cpp` (24/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer-to-sink · DONE 2026-09-21
- [x] `net/general/net_gameobj.cpp` (5/6/0) — ⚪ leave · _pool/arena allocator_ · DONE 2026-09-21 RAII batch
- [x] `net/general/net_gamesettings.cpp` (5/5/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `net/general/networkevent.cpp` (10/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer-to-sink · DONE 2026-09-21
- [x] `net/general/net_cell.cpp` (3/6/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `net/general/net_diplomacy.cpp` (5/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `net/general/net_vision.cpp` (5/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer-to-sink · DONE 2026-09-21
- [x] `net/general/net_army.cpp` (5/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer-to-sink · DONE 2026-09-21
- [x] `net/general/net_crc.cpp` (5/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `net/general/net_agreement.cpp` (4/1/0) — 🟡 moderate · _single-owner member_ · triaged 2026-09-20: remaining sites are refcounted/transfer-to-sink · DONE 2026-09-21
- [x] `net/general/net_hash.h` (2/2/0) — ⚪ leave · _pool/arena allocator_ · DONE 2026-09-21 RAII batch
- [x] `net/general/net_unit.cpp` (4/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer-to-sink · DONE 2026-09-21
- [x] `net/general/net_order.cpp` (2/1/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `net/general/net_message.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer-to-sink · DONE 2026-09-21
- [x] `net/general/net_endgame.cpp` (2/1/0) — ⚪ · _global Wormhole singleton + list_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `net/general/net_civ.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `net/general/net_city.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `net/general/chatlist.h` (1/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `net/general/net_installation.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `net/general/net_exclusions.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `net/general/net_terrain.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `net/general/net_tradeoffer.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-21: pool-Insert transfer · DONE 2026-09-21
- [x] `net/general/net_traderoute.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-21: pool-Insert transfer · DONE 2026-09-21
- [x] `net/general/net_report.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-21: Enqueue sink · DONE 2026-09-21
- [x] `net/general/net_feat.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-21: AddTail sink · DONE 2026-09-21
- [x] `net/general/net_packet.h` (0/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-21: refcounted delete this · DONE 2026-09-21

## ui/aui_common  (38 files, 288 matches)

- [x] `ui/aui_common/aui_ui.cpp` — done 2026-08-03: tech_Memory pool held by value, not new/delete
- [x] `ui/aui_common/aui_ldl.cpp` (32/13/0) — 🔴 hard · _void* ownership handoff_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/aui_common/aui_listbox.cpp` (13/12/0) — 🟡 moderate · _single-owner member_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/aui_common/aui_resource.h` (5/8/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_common/aui_window.cpp` (3/8/2) — 🟡 moderate · _single-owner member_ · DONE 2026-09-21: removed double-free `delete m_focusControl` (control already deleted via DeleteHierarchyFromRoot leaf pass); m_surface/m_dirtyList/m_grabRegion/m_focusList already unique_ptr
- [x] `ui/aui_common/aui_region.cpp` (7/5/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive/API-boundary · DONE 2026-09-21
- [x] `ui/aui_common/aui_dropdown.cpp` (7/4/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_common/aui_ranger.cpp` (5/6/0) — 🔴 hard · _void* ownership handoff_ · DONE 2026-09-21 RAII batch
- [x] `ui/aui_common/aui_image.cpp` (4/4/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_common/aui_control.cpp` (4/4/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-21: m_allocatedTip flag removed — it marked externally-owned tip windows as owned (deleted shared g_tipWindow); m_ownedTip unique_ptr owns only the LDL-allocated tip, m_tip is a non-owning observer
- [x] `ui/aui_common/aui_textfield.cpp` (3/4/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_common/aui_imagebase.cpp` (1/5/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_common/aui_Factory.cpp` (6/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/aui_common/aui_mouse.cpp` (1/4/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_common/aui_hypertextbase.cpp` (2/3/0) — 🟡 moderate · _single-owner member_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/aui_common/aui_memmap.cpp` (3/2/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/aui_common/aui_pixel.cpp` (2/3/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/aui_common/aui_bitmapfont.cpp` (3/2/0) — 🟡 moderate · _single-owner member_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/aui_common/aui_hypertextbox.cpp` (2/2/0) — 🟡 moderate · _single-owner member_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/aui_common/aui_stringtable.cpp` (1/3/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_common/aui_header.cpp` (2/2/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/aui_common/aui_screen.cpp` (2/2/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_common/aui_tab.cpp` (2/1/0) — 🔴 hard · _void* ownership handoff_ · DONE 2026-09-21 RAII batch
- [x] `ui/aui_common/tech_memmap.cpp` (1/2/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive/API-boundary · DONE 2026-09-21
- [x] `ui/aui_common/tech_memory.h` (2/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_common/aui_textbase.cpp` (2/1/0) — 🔴 hard · _void* ownership handoff_ · DONE 2026-09-21 RAII batch
- [x] `ui/aui_common/aui_win.cpp` (1/1/0) — 🔴 hard · _void* ownership handoff_ · DONE 2026-09-21 RAII batch
- [x] `ui/aui_common/aui_base.cpp` (1/1/0) — ⚪ leave · _mostly g_/s_ singletons_ · DONE 2026-09-21 RAII batch
- [x] `ui/aui_common/aui_dirtylist.cpp` — done 2026-08-03: tech_Memory pool held by value, not new/delete
- [x] `ui/aui_common/aui_tipwindow.cpp` (1/1/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch · amended 2026-09-21: dropped redundant m_allocatedTip flag; m_staticTip stays unique_ptr (it is aui_Ldl::Remove()'d at creation so the LDL leaf pass never sees it — dtor delete was the only free, not a double-free)
- [x] `ui/aui_common/aui_surface.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_common/aui_textbox.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_common/aui_moviemanager.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_common/aui_shell.cpp` (1/1/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_common/aui_region.h` (2/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/aui_common/tech_wllist.h` (1/1/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_common/aui_window.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_common/aui_blitter.cpp` (0/0/1) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch

## gfx/spritesys  (23 files, 260 matches)

- [x] `gfx/spritesys/director.cpp` (40/7/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer-to-sink · DONE 2026-09-21
- [x] `gfx/spritesys/spritefile.cpp` (45/2/0) — 🔴 hard · _void* ownership handoff_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gfx/spritesys/UnitSpriteGroup.cpp` — done 2026-08-03 (b1169bf7): delete+assign on the base slots became reset()
- [x] `gfx/spritesys/DirectorActionHandlers.cpp` (16/6/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gfx/spritesys/Sprite.cpp` — done 2026-08-03: frames own their buffers via SpriteFrame, matching the two faced subclasses; whole hierarchy now shares one frame representation
- [x] `gfx/spritesys/effectspritegroup.cpp` — done 2026-08-03 (b1169bf7): delete+assign on the base slots became reset()
- [x] `gfx/spritesys/goodspritegroup.cpp` — done 2026-08-03 (b1169bf7): delete+assign on the base slots became reset()
- [x] `gfx/spritesys/SpriteGroupList.cpp` — done 2026-08-03: slots are unique_ptr; store-back guards self-assignment (loader reuses the slot it read)
- [x] `gfx/spritesys/FacedSpriteWshadow.cpp` — done 2026-08-03 (f05e888d): frames own their buffers via SpriteFrame; fixed 8 scalar deletes on array-new'd memory
- [x] `gfx/spritesys/FacedSprite.cpp` — done 2026-08-03 (b2d108fe): frames own their buffers via SpriteFrame; fixed 4 scalar deletes on array-new'd memory
- [x] `gfx/spritesys/UnitActor.cpp` (6/0/0)
- [x] `gfx/spritesys/SpriteStateDB.cpp` (1/4/0)
- [x] `gfx/spritesys/spriteutils.cpp` (2/0/2) — 🔴 hard · _mixed new[]/malloc buffers_ · DONE 2026-09-21 RAII batch
- [x] `gfx/spritesys/SpriteGroup.cpp` — done 2026-08-03 (b1169bf7): sprite/anim slots are unique_ptr; setters guard self-assignment (the loader does get-modify-set)
- [x] `gfx/spritesys/battleviewactor.cpp` (2/0/0)
- [x] `gfx/spritesys/action.cpp` (1/1/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-15: `m_curAnim` → `std::unique_ptr<Anim>`; copy-ctor deep-copies via `make_unique`; `SetAnim` now takes ownership by `unique_ptr` (all 15 callers pass `std::move(anim)`, spriteeditor passes a deep copy of its borrowed anim — fixing a latent double-delete where Action's dtor freed the sprite group's Anim).
- [x] `gfx/spritesys/goodactor.cpp` (2/0/0)
- [x] `gfx/spritesys/TradeActor.cpp` (2/0/0)
- [x] `gfx/spritesys/goodactor_factory_impl.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gfx/spritesys/DirectorActions.cpp` (0/2/0)
- [x] `gfx/spritesys/workeractor.cpp` (1/0/0)
- [x] `gfx/spritesys/EffectActor.cpp` (1/0/0)
- [x] `gfx/spritesys/Actor.cpp` (1/0/0)

## test/cpp  (20 files, 221 matches)

- [x] `test/cpp/doctest.h` (13/33/0) — DONE 2026-09-21: vendored test framework, upstream-owned — excluded
- [x] `test/cpp/test_json_save.cpp` (27/6/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `test/cpp/test_player_view.cpp` (26/2/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `test/cpp/test_save_load.cpp` (25/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `test/cpp/test_citydata.cpp` (18/5/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `test/cpp/test_headless_smoke.cpp` (11/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `test/cpp/test_json_save_integration.cpp` (10/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `test/cpp/scoped_rand.h` (2/5/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `test/cpp/test_building_evaluator.cpp` (4/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `test/cpp/test_ui_command_surface.cpp` (5/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `test/cpp/test_city_visibility.cpp` (5/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `test/cpp/test_headless_long.cpp` (4/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `test/cpp/test_globals_cleanup_smoke.cpp` (4/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `test/cpp/test_game.cpp` (3/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `test/cpp/test_buildqueue.cpp` (3/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `test/cpp/test_happy.cpp` (2/0/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-21: profiledb_Set sink + fixture-lifetime s_app singleton · DONE 2026-09-21
- [x] `test/cpp/test_headless_progression.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `test/cpp/test_json_world.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 (only "--new-game" string literal)
- [x] `test/cpp/test_headless_determinism.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 (only "--new-game" string literal)
- [x] `test/cpp/test_save_determinism.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 (only "--new-game" string literal)

## gs/utility  (10 files, 195 matches)

- [x] `gs/utility/gameinit.cpp` (96/16/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/utility/TurnCnt.cpp` (14/1/0) — ✅ done · _m_sliceList -> unique_ptr; residual news are net-transfer (deferred) + refcounted SlicObject (off-limits)_
- [x] `gs/utility/QuadTree.h` (11/12/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `gs/utility/newturncount.cpp` (11/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/utility/Globals.h` (4/5/0) — DONE 2026-09-21: added allocated::reassign(T*&, unique_ptr<T>) overload; raw-pointer helpers are the established convention (~40 call sites)
- [x] `gs/utility/DataCheck.cpp` (7/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/utility/DataCheck.h` (2/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/utility/RandGen.h` (2/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/utility/TurnCntEvent.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/utility/gameinit.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch

## gs/fileio  (8 files, 155 matches)

- [x] `gs/fileio/json_save.cpp` (58/39/8) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/fileio/GameFile.cpp` (17/3/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/fileio/civscenarios.cpp` (7/7/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/fileio/prjfile.cpp` (0/0/8) — DONE 2026-09-21: already RAII (unique_ptr<char,decltype(&free)>); no live raw new/delete
- [x] `gs/fileio/CivPaths.cpp` (2/2/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/fileio/Token.h` (2/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/fileio/Token.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/fileio/action_log.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch

## gs/world  (14 files, 126 matches)

- [x] `gs/world/wldgen.cpp` (22/17/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/world/UnseenCell.cpp` (17/3/0) — ⚪ leave · _pool/arena allocator_ · DONE 2026-09-21 RAII batch
- [x] `gs/world/WrlEnv.cpp` (6/10/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/world/WrldCont.cpp` (7/8/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/world/Cell.cpp` (5/9/0) — ⚪ leave · _pool/arena allocator_ · DONE 2026-09-21 RAII batch
- [x] `gs/world/WrldPoll.cpp` (5/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/world/TileInfo.cpp` (2/3/0) — 🔴 hard · _`*this=*copy` copy-ctor blocks unique_ptr (needs custom operator=)_ · DONE 2026-09-21 RAII batch
- [x] `gs/world/WrldCity.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/world/worldutils.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/world/WorldDistance.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/world/worldevent.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gs/world/cellunitlist.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/world/UnseenCell.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/world/UnseenCellQuadTree.h` (0/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21

## ctp  (6 files, 124 matches)

- [x] `ctp/civapp.cpp` (83/9/0) — DONE 2026-09-21: g_GreatLibPF/g_SoundPF/g_ImageMapPF → allocated::reassign(g, make_unique<...>())
- [x] `ctp/display.cpp` (9/3/0) — DONE 2026-09-21: delete g_displayModes → allocated::clear; unique_ptr(new X) → make_unique
- [x] `ctp/civ3_main.cpp` (9/0/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ctp/headless_main.cpp` (7/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive/API-boundary · DONE 2026-09-21
- [x] `ctp/game_controller.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ctp/civapp.h` (0/2/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch

## ctp/ctp2_utils  (9 files, 120 matches)

- [x] `ctp/ctp2_utils/c3cmdline.cpp` (50/30/0) — DONE 2026-09-21: 15 delete g_the*DB + make_unique().release() pairs → allocated::reassign
- [x] `ctp/ctp2_utils/pointerlist.h` (4/6/0) — DONE 2026-09-21: node management → unique_ptr (make_unique+release into links, unique_ptr temporaries in dtor/DeleteAll/Remove*)
- [x] `ctp/ctp2_utils/c3files.cpp` (7/3/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive/API-boundary · DONE 2026-09-21
- [x] `ctp/ctp2_utils/AvlTree.h` (2/6/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ctp/ctp2_utils/c3mem.cpp` (1/1/2) — DONE 2026-09-21: operator new/delete overloads are infrastructure, left
- [x] `ctp/ctp2_utils/appstrings.cpp` (2/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ctp/ctp2_utils/netconsole.cpp` (1/1/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ctp/ctp2_utils/c3errors.cpp` (0/0/2) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ctp/ctp2_utils/civlog.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch

## gfx/tilesys  (9 files, 93 matches)

- [x] `gfx/tilesys/tileutils.cpp` (15/5/7) — 🔴 hard · _mixed new[]/malloc buffers_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gfx/tilesys/tileset.cpp` (9/13/0) — DONE 2026-09-21: added m_*Owners vector<unique_ptr<T[]>> holding Load-mode ownership; raw vectors keep .get() views; Cleanup() clears owners
- [x] `gfx/tilesys/tiledmap.cpp` (10/10/0) — 🟡 moderate · _single-owner member_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gfx/tilesys/workmap.cpp` (1/6/0) — 🔴 hard · _void* ownership handoff_ · DONE 2026-09-21 RAII batch
- [x] `gfx/tilesys/tiledmap_observer_adapter.cpp` (2/3/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gfx/tilesys/BaseTile.cpp` (2/2/0) — DONE 2026-09-21: m_tileDataOwner/m_hatDataOwner unique_ptr<Pixel16[]>; raw members are views (QuickRead borrows)
- [x] `gfx/tilesys/resourcemap.cpp` (1/3/0) — 🔴 hard · _void* ownership handoff_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gfx/tilesys/tiledraw.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gfx/tilesys/tileset.h` (2/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch

## gs/newdb  (5 files, 66 matches)

- [x] `gs/newdb/CTPDatabase.cpp` (24/13/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: comment-only matches · DONE 2026-09-21
- [x] `gs/newdb/DBLexer.cpp` (9/7/0) — ⚪ leave · _pool/arena allocator_ · DONE 2026-09-21 RAII batch
- [x] `gs/newdb/CTPRecord.cpp` (6/4/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-21: caller-owned out-param arrays (generated-code contract) · DONE 2026-09-21
- [x] `gs/newdb/CTPRecord.h` (1/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-21 RAII batch
- [x] `gs/newdb/DBTokens.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-21 RAII batch

## gs/database  (11 files, 64 matches)

- [x] `gs/database/StrDB.cpp` (17/5/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/database/profileDB.cpp` — done 2026-08-03: PointerList member held by value (partial: other raw owners remain in this file)
- [x] `gs/database/profileDB.h` (6/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/database/moviedb.cpp` (1/4/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/database/filenamedb.cpp` (1/4/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/database/PlayListDB.cpp` (1/3/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/database/highscoredb.cpp` (1/2/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/database/thronedb.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/database/thronedb.h` (2/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/database/EndGameDB.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/database/UVDB.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch

## gs/dbgen  (4 files, 57 matches)

- [x] `gs/dbgen/Datum.cpp` (14/15/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-21: emits new/delete as string literals into generated code; m_bitPairDatum converted to unique_ptr · DONE 2026-09-21
- [x] `gs/dbgen/ctpdb.cpp` (4/3/5) — ⚪ leave · _mostly g_/s_ singletons_ · DONE 2026-09-21 RAII batch
- [x] `gs/dbgen/RecordDescription.cpp` (9/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-21: remaining sites are AddTail sinks; pairDat converted · DONE 2026-09-21
- [x] `gs/dbgen/MemberClass.cpp` (5/2/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-21: remaining sites are AddTail sinks + generated-code literals; pairDat converted · DONE 2026-09-21

## 3rdparty/nlohmann  (1 files, 55 matches)

- [x] `3rdparty/nlohmann/json.hpp` (21/34/0) — DONE 2026-09-21: vendored upstream — excluded

## net/io  (4 files, 51 matches)

- [x] `net/io/net_thread.cpp` (20/13/0) — ⚪ leave · _pool/arena allocator_ · DONE 2026-09-21 RAII batch
- [x] `net/io/net_anet.cpp` (5/5/0) — 🔴 hard · _void* ownership handoff_ · triaged 2026-09-21: dp_transport_t/dp_session_t in raw owning containers; RemoveHead delete converted · DONE 2026-09-21
- [x] `net/io/net_list.h` (3/4/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `net/io/net_util.h` (1/0/0) — DONE 2026-09-21: PULLNEWSTRING macro deleted — zero call sites, dead code

## ui/slic_debug  (3 files, 51 matches)

- [x] `ui/slic_debug/sourcelist.cpp` — done 2026-08-03: 7 controls are unique_ptr; window now released last (hand-written order freed it third); local delete-macro removed
- [x] `ui/slic_debug/watchlist.cpp` — done 2026-08-03: 5 WatchList controls are unique_ptr; WatchListItem::m_watching deliberately left raw (different class)
- [x] `ui/slic_debug/segmentlist.cpp` (7/4/0) — 🟡 moderate · _single-owner member_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21

## gs/events  (6 files, 34 matches)

- [x] `gs/events/GameEventManager.cpp` — done 2026-08-03: PointerList member held by value (partial: other raw owners remain in this file)
- [x] `gs/events/GameEventDescription.h` (11/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/events/GameEventArgList.cpp` (3/1/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/events/GameEventArgument.cpp` (1/1/0) — 🔴 hard · _void* ownership handoff_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `gs/events/GameEvent.cpp` (0/2/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/events/GameEventHook.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch

## ui/ldl  (7 files, 33 matches)

- [x] `ui/ldl/ldlif.cpp` (12/7/0) — ⚪ leave · _pool/arena allocator_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21
- [x] `ui/ldl/ldl_attr.cpp` (4/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/ldl/ldl_data.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/ldl/ldl_memmap.cpp` (1/2/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/ldl/ldl_data.hpp` (2/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ui/ldl/ldl_file.hpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ui/ldl/ldl_list.h` (0/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21

## gs/core  (11 files, 31 matches)

- [x] `gs/core/game.cpp` (1/6/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive/API-boundary · DONE 2026-09-21
- [x] `gs/core/battle_observer.h` (4/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/core/game_observer.h` (2/2/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/core/tiledmap_observer.h` (2/2/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/core/goodactor_factory.h` (2/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/core/goodactor_factory.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/core/game.h` (0/2/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/core/diplomacy_observer.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/core/game_observer_registration.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/core/progress_observer.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gs/core/player_view.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch

## ai/strategy  (4 files, 31 matches)

- [x] `ai/strategy/scheduler/scheduler.cpp` (14/8/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ai/strategy/goals/Goal.cpp` (2/2/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ai/strategy/scheduler/Plan.cpp` (2/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `ai/strategy/agents/agent.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch

## gfx/gfx_utils  (8 files, 22 matches)

- [x] `gfx/gfx_utils/gfx_options.cpp` (5/5/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gfx/gfx_utils/tiffutils.cpp` (0/0/4) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gfx/gfx_utils/videoutils.cpp` (2/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `gfx/gfx_utils/Queue.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gfx/gfx_utils/pixeltypes.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gfx/gfx_utils/colorset.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gfx/gfx_utils/arproces.cpp` (0/0/1) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `gfx/gfx_utils/pixelutils.cpp` (0/0/1) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch

## ai/diplomacy  (2 files, 18 matches)

- [x] `ai/diplomacy/diplomat.cpp` (17/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `ai/diplomacy/regardevent.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21

## robot/aibackdoor  (3 files, 17 matches)

- [x] `robot/aibackdoor/dynarr.h` (5/5/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `robot/aibackdoor/semi_dynamic_array.h` (0/0/4) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `robot/aibackdoor/pool.h` (1/2/0) — DONE 2026-09-21: chunk alloc → make_unique<DATA_TYPE[]>().release() into list_array; dtor → unique_ptr temporary

## ctp/fingerprint  (1 files, 17 matches)

- [x] `ctp/fingerprint/shroud.c` (4/0/13) — DONE 2026-09-21: C file; malloc/free is idiomatic, left

## ai  (1 files, 16 matches)

- [x] `ai/ctpai.cpp` (16/0/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21

## sound  (1 files, 15 matches)

- [x] `sound/soundmanager.cpp` — done 2026-08-03: sfx and voice lists held by value (partial: m_soundWalker remains)

## mapgen  (5 files, 15 matches)

- [x] `mapgen/Geometric.cpp` (3/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21
- [x] `mapgen/Crater.cpp` (2/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive/API-boundary · DONE 2026-09-21
- [x] `mapgen/FaultGen.cpp` (2/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive/API-boundary · DONE 2026-09-21
- [x] `mapgen/PlasmaGen2.cpp` (2/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive/API-boundary · DONE 2026-09-21
- [x] `mapgen/PlasmaGen1.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive — see session notes · DONE 2026-09-21

## ai/CityManagement  (2 files, 15 matches)

- [x] `ai/CityManagement/governor.cpp` (12/1/0) — 🟡 moderate · _needs ownership review_ · triaged 2026-09-20: remaining sites are refcounted/transfer/intrusive/API-boundary · DONE 2026-09-21
- [x] `ai/CityManagement/governor.h` (2/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch

## ui/aui_sdl  (2 files, 7 matches)

- [x] `ui/aui_sdl/aui_sdlui.cpp` (3/3/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-20 RAII batch
- [x] `ui/aui_sdl/aui_sdlsurface.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch

## ai/mapanalysis  (1 files, 4 matches)

- [x] `ai/mapanalysis/settlemap.cpp` (1/3/0) — ✅ done · _local bool[] scratch -> vector<bool>_

## ui/freetype  (1 files, 4 matches)

- [x] `ui/freetype/freetype.h` (4/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch

## robot/utility  (1 files, 3 matches)

- [x] `robot/utility/roboinit.cpp` (1/2/0) — ⚪ leave · _mostly g_/s_ singletons_ · DONE 2026-09-21 RAII batch

## robot/pathing  (2 files, 3 matches)

- [x] `robot/pathing/astar.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
- [x] `robot/pathing/TestP.cpp` (1/0/0) — ⚪ leave · _mostly g_/s_ singletons_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21

## gs/outcom  (1 files, 3 matches)

- [x] `gs/outcom/c3rand.cpp` (1/2/0) — ⚪ leave · _conditional/borrowed owner (m_rand=rand_ptr() when !ownGenerator) + refcounted_ · triaged 2026-09-21: partial conversion; remaining sites are sinks/singletons/pools/refcounted · DONE 2026-09-21

## ui/aui_directx  (1 files, 2 matches)

- [x] `ui/aui_directx/aui_directmoviemanager.cpp` (1/1/0) — 🟡 moderate · _single-owner member_ · DONE 2026-09-21 RAII batch

## ctp/ctp2_rsrc  (1 files, 1 matches)

- [x] `ctp/ctp2_rsrc/resource.h` (1/0/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch

## ui/aui_utils  (1 files, 1 matches)

- [x] `ui/aui_utils/textutils.cpp` (0/1/0) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch

## os/nowin32  (1 files, 1 matches)

- [x] `os/nowin32/nowin32.cpp` (0/0/1) — 🟡 moderate · _needs ownership review_ · DONE 2026-09-20 RAII batch
