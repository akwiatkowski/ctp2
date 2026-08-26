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

- [ ] `ui/interface/knowledgewin.cpp` (53/37/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/infowin.cpp` (84/6/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/spnewgamewindow.cpp` (4/69/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/sciencewin.cpp` (55/18/0) — 🔴 hard · _void* ownership handoff_
- [x] `ui/interface/battleviewwindow.cpp` — done 2026-08-03: all 31 owned controls are unique_ptr members; destructor is just the global back-pointer reset
- [ ] `ui/interface/loadsavewindow.cpp` (15/40/0) — 🔴 hard · _void* ownership handoff_
- [x] `ui/interface/EndgameWindow.cpp` (31/14/0) — 🟡 moderate · _single-owner member_ · DONE 2026-08-26: all 14 child controls → `unique_ptr`, 7 LDL-driven control arrays → `vector<unique_ptr>`, `c3_Animation::m_frames` → `unique_ptr`, blend scratch surfaces RAII'd; counts kept as LDL-driven sizes, in-class initializers replace `CleanPointers`.
- [ ] `ui/interface/spriteeditor.cpp` (9/26/0) — 🔴 hard · _void* ownership handoff_
- [ ] `ui/interface/victorywin.cpp` (31/3/0) — 🟡 moderate · _needs ownership review_
- [x] `ui/interface/creditsscreen.cpp` (17/17/0) — 🟡 moderate · _single-owner member_ · DONE 2026-08-26: members → `unique_ptr`, anim arrays + credits pages/lines → `vector`, `Parse` no longer `delete this` (bool return), blend-scratch surfaces RAII'd; fixed font-index off-by-one (`>` → `>=`) and null-font deref on failed load.
- [x] `ui/interface/messageeyepoint.cpp` (20/10/0) — 🟡 moderate · _single-owner member_ · DONE 2026-08-26: buttons/dropdowns/actions → `unique_ptr` members (`SetAction`/`AddControl`/`AddItem` verified non-owning), `m_action1/2` renamed `m_actionLeft/Right`; only sink-transfer `new`s remain.
- [ ] `ui/interface/ancientwindows.cpp` (13/15/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/scenarioeditor.cpp` (19/9/0) — 🔴 hard · _void* ownership handoff_
- [x] `ui/interface/messagewindow.cpp` — done 2026-08-03: 12 controls are unique_ptr, including 4 border bars that were leaking (allocated, AddControl'd, never freed)
- [ ] `ui/interface/EditQueue.cpp` (18/8/0) — 🔴 hard · _void* ownership handoff_
- [ ] `ui/interface/spnewgamescreen.cpp` (21/2/0) — 🔴 hard · _factory returns crossing callers_
- [ ] `ui/interface/sci_advancescreen.cpp` (17/4/0) — ⚪ leave · _mostly g_/s_ singletons_
- [x] `ui/interface/battleview.cpp` — done 2026-08-03: m_activeEvents held by value (partial: other raw owners remain)
- [ ] `ui/interface/controlpanel.cpp` (3/16/0) — ⚪ leave · _mostly g_/s_ singletons_
- [x] `ui/interface/messageresponse.cpp` — done 2026-08-03: 4 owned members are unique_ptr; the two tech_WLLists left raw (they own their entries)
- [x] `ui/interface/loadsavemapwindow.cpp` — done 2026-08-03: 9 window controls are unique_ptr; list-item members left raw (c3_ListItem frees children)
- [x] `ui/interface/messageadvice.cpp` — done 2026-08-03: 5 controls are unique_ptr; list ITEMS still deleted by hand (not members)
- [ ] `ui/interface/spnewgameplayersscreen.cpp` (14/2/0) — ⚪ leave · _mostly g_/s_ singletons_
- [x] `ui/interface/messagemodal.cpp` (10/6/0) — 🟡 moderate · _single-owner member_ · DONE 2026-08-26: text box/eye-point helpers → `unique_ptr` members (replacing the raw union on the modal side), response button/action `tech_WLList`s → `vector<unique_ptr>`; **fixed a real leak** — `~MessageModal` never freed the eye-point helper (its sibling `~MessageWindow` did).
- [ ] `ui/interface/messagewin.cpp` (9/7/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/greatlibrary.cpp` (11/5/0) — 🔴 hard · _void* ownership handoff_
- [ ] `ui/interface/battleevent.cpp` (7/8/0) — ⚪ leave · _pool/arena allocator_
- [ ] `ui/interface/citywindow.cpp` (9/5/0) — ⚪ leave · _pool/arena allocator_
- [x] `ui/interface/wondermoviewindow.cpp` — done 2026-08-03: 7 owned controls are unique_ptr members; AddControl takes .get() (non-owning)
- [x] `ui/interface/hotseatlist.cpp` (10/4/0) — 🟡 moderate · _single-owner member_ · DONE 2026-08-26: window/list → `unique_ptr` (list items stay sink-owned via `c3_ListBox::Clear`), legal-civ flag array → `vector<bool>`, scenario `SaveInfo` local → RAII.
- [ ] `ui/interface/tileimptracker.cpp` (12/2/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/km_screen.cpp` (10/3/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/dipwizard.cpp` (4/8/0) — 🔴 hard · _void* ownership handoff_
- [ ] `ui/interface/NationalManagementDialog.cpp` (7/5/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/diplomacywindow.cpp` (3/9/0) — 🔴 hard · _void* ownership handoff_
- [ ] `ui/interface/controlpanelwindow.cpp` (7/4/0) — 🔴 hard · _void* ownership handoff_
- [ ] `ui/interface/scorewarn.cpp` (10/1/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/battle_observer_adapter.cpp` (8/3/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/interface/armymanagerwindow.cpp` (7/3/0) — ⚪ leave · _pool/arena allocator_
- [ ] `ui/interface/tutorialwin.cpp` (9/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/loadsavescreen.cpp` (7/2/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/MessageBoxDialog.cpp` (8/1/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/spnewgamerulesscreen.cpp` (6/3/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/interface/spnewgamemapsizescreen.cpp` (5/3/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/custommapscreen.cpp` (6/2/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/testwindow.cpp` (4/4/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/interface/messagelist.cpp` (3/5/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/battle.cpp` (7/1/0) — ⚪ leave · _pool/arena allocator_
- [ ] `ui/interface/SpecialAttackWindow.cpp` (4/4/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/spnewgametribescreen.cpp` (7/1/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/chatbox.cpp` (4/4/0) — 🔴 hard · _void* ownership handoff_
- [ ] `ui/interface/workwin.cpp` (4/3/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/infowindow.cpp` (4/3/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/interface/spnewgamerandomcustomscreen.cpp` (5/2/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/graphicsresscreen.cpp` (5/2/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/agesscreen.cpp` (7/0/0) — ⚪ leave · _mostly g_/s_ singletons_
- [x] `ui/interface/ControlTabPanel.cpp` (6/0/0) — ✅ done · `m_ldlBlock` now uses `unique_ptr<MBCHAR[]>`, fixing the `new[]`/scalar-delete mismatch; local formatter uses `snprintf`.
- [ ] `ui/interface/progresswindow.cpp` (3/3/0) — 🟡 · _window lifecycle (ref-param + c3ui)_
- [ ] `ui/interface/text_hasher.h` (2/4/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/interface/statswindow.cpp` (3/3/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/MainControlPanel.cpp` (6/0/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/DiplomacyDetails.cpp` (3/3/0) — 🔴 hard · _void* ownership handoff_
- [ ] `ui/interface/MapCopyBuffer.cpp` (2/4/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/spnewgamemapshapescreen.cpp` (3/2/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/trademanager.cpp` (4/1/0) — 🔴 hard · _void* ownership handoff_
- [ ] `ui/interface/messageiconwindow.cpp` (2/3/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/interface/musictrackscreen.cpp` (5/0/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/scenariowindow.cpp` (4/1/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/rankingtab.cpp` (2/2/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/spnewgamediffscreen.cpp` (3/1/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/citymanager.cpp` (3/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/intelligencewindow.cpp` (4/0/0) — 🔴 hard · _void* ownership handoff_
- [ ] `ui/interface/CityEspionage.cpp` (3/1/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/helptile.cpp` (3/1/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/UnitControlPanel.cpp` (2/2/0) — 🔴 hard · _void* ownership handoff_
- [ ] `ui/interface/messageactions.cpp` (4/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/unitmanager.cpp` (4/0/0) — 🔴 hard · _void* ownership handoff_
- [ ] `ui/interface/AttractWindow.cpp` (2/2/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/DomesticManagementDialog.cpp` (3/1/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/soundscreen.cpp` (1/2/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/c3dialogs.cpp` (2/1/0) — 🔴 hard · _void* ownership handoff_
- [ ] `ui/interface/victorymoviewin.cpp` (2/1/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/loadsavemapscreen.cpp` (1/2/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/IntroMovieWin.cpp` (2/1/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/StatusBar.h` (1/2/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/wondermoviewin.cpp` (2/1/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/gameplayoptions.cpp` (1/2/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/optionwarningscreen.cpp` (1/2/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/sciencevictorydialog.cpp` (1/1/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/spnewgameplayersscreen.h` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/spnewgamerulesscreen.h` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/graphicsscreen.cpp` (1/1/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/testwin.cpp` (1/1/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/TurnYearStatus.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/greatlibrarywindow.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/ScienceManagementDialog.cpp` (1/1/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/IntroMovieWindow.cpp` (1/1/0) — 🟡 moderate · _single-owner member_
- [x] `ui/interface/scoretab.cpp` (1/1/0) — ✅ done · `m_difficultyStrings` is now a `unique_ptr` single-owner member.
- [ ] `ui/interface/victorymoviewindow.cpp` (1/1/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/interface/cursormanager.cpp` (1/1/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/optionsscreen.cpp` (2/0/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/backgroundwin.cpp` (1/1/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/ProfileEdit.cpp` (1/1/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/timelinetab.cpp` (0/2/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/cursormanager.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/spnewgametribescreen.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/initialplaywindow.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/screenutils.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/interfaceevent.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/splash.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/WonderTab.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/musicscreen.cpp` (1/0/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/interface/screenutils.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/initialplayscreen.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/scenarioeditor.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/interface/dipwizard.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [x] `ui/interface/optionswindow.cpp` — done 2026-08-03: 13 controls are unique_ptr; local mycleanup macro gone, destructor empty
- [ ] `ui/interface/UIUtils.h` (0/1/0) — 🟡 moderate · _needs ownership review_

## gs/gameobj  (93 files, 938 matches)

- [ ] `gs/gameobj/Player.cpp` (193/33/0) — ⚪ leave · _pool/arena allocator_
- [ ] `gs/gameobj/ArmyData.cpp` (121/17/0) — ⚪ leave · _pool/arena allocator_
- [ ] `gs/gameobj/CityData.cpp` (68/2/0) — ⚪ leave · _pool/arena allocator_
- [ ] `gs/gameobj/UnitData.cpp` (51/7/0) — 🟡 moderate · _single-owner member_
- [ ] `gs/gameobj/DiplomaticRequestData.cpp` (46/2/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/bldque.cpp` (42/5/0) — ⚪ leave · _pool/arena allocator_
- [ ] `gs/gameobj/endgame.cpp` (22/0/0) — 🟡 moderate · _needs ownership review_
- [x] `gs/gameobj/Vision.cpp` (12/9/0) — ✅ done · _m_unseenCells -> unique_ptr, m_array uint16** -> vector<vector>; quadtree cell-content news left (separate ownership)_
- [ ] `gs/gameobj/CityEvent.cpp` (16/2/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/unitevent.cpp` (16/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/GoodyHuts.cpp` (14/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/PlayerEvent.cpp` (14/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/messagedata.cpp` (11/3/0) — ⚪ leave · _pool/arena allocator_
- [ ] `gs/gameobj/Advances.cpp` (12/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/armyevent.cpp` (9/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/Civilisation.cpp` (8/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/Wormhole.cpp` (5/4/0) — ⚪ leave · _pool/arena allocator_
- [x] `gs/gameobj/FeatTracker.cpp` — done 2026-08-03: m_activeList held by value
- [ ] `gs/gameobj/GameObj.cpp` (0/9/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/TradeBids.cpp` (2/5/0) — ⚪ leave · _pool/arena allocator_
- [ ] `gs/gameobj/Happy.cpp` (5/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/AgreementData.cpp` (5/1/0) — 🔴 · _refcounted SlicObject (AddRef/Release)_
- [x] `gs/gameobj/CTP2Combat.cpp` (3/2/0) — ✅ done · _CombatField::m_field 2D array -> vector<vector>; fixed new[]/scalar-delete UB_
- [ ] `gs/gameobj/MessagePool.cpp` (5/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/MaterialPool.cpp` (5/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/Order.h` (3/2/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/Unit.cpp` (5/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/Order.cpp` (1/3/0) — 🟡 moderate · _needs ownership review_
- [x] `gs/gameobj/MovePath.cpp` (2/2/0) — ✅ done · _Path transfer-on-success -> unique_ptr + release_
- [ ] `gs/gameobj/TradeRouteData.cpp` (3/1/0) — 🔴 hard · _`*this=*copyme` copy-ctor blocks unique_ptr (needs custom operator=)_
- [ ] `gs/gameobj/TradePool.cpp` (2/2/0) — 🟡 · _pool + member array owner_
- [ ] `gs/gameobj/UnoccupiedTiles.cpp` (2/2/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/barbarians.cpp` (3/1/0) — ⚪ leave · _alloc is in a `/* */`-commented-out function (dead code)_
- [x] `gs/gameobj/civilisationpool.cpp` (3/1/0) — 🟡 moderate · _single-owner member_
- [ ] `gs/gameobj/ArmyPool.cpp` (4/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/installationtree.h` (2/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/improvementevent.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/TradeOfferPool.cpp` (2/1/0) — ⚪ leave · _pool/arena allocator_
- [ ] `gs/gameobj/TerrImprovePool.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_
- [x] `gs/gameobj/EventTracker.cpp` — done 2026-08-03: PointerList member held by value (DeleteAll still frees the pointed-to objects)
- [ ] `gs/gameobj/Pollution.cpp` (2/1/0) — 🔴 · _refcounted SlicObject (AddRef/Release)_
- [ ] `gs/gameobj/citydata.h` (3/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/DiplomaticRequestPool.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/PlayerTurn.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/TradeOfferData.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/combatevent.cpp` (1/2/0) — 🟡 moderate · _needs ownership review_
- [x] `gs/gameobj/CriticalMessagesPrefs.cpp` — done 2026-08-03: PointerList member held by value (DeleteAll still frees the pointed-to objects)
- [ ] `gs/gameobj/CTP2Combat.h` (3/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/terrainutil.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/Readiness.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/GameObj.h` (0/3/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/UnitPool.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/Agreement.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/TaxRate.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/MessagePool.h` (1/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/EventTracker.h` (2/0/0) — ⚪ leave · _pool/arena allocator_
- [ ] `gs/gameobj/buildingutil.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/Resources.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/TradeRoute.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/FeatTracker.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/wonderutil.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/CivilisationData.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/AchievementTracker.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/installationdata.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/Gold.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/TradeOffer.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/ArmyPool.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/GameSettings.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/TerrImprove.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/Diffcly.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/TradeOfferPool.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/WonderTracker.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/TerrImprovePool.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/UnitTypes.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/UnitData.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/TradePool.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/installation.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/pollution.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/Wormhole.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/DiplomaticRequestPool.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/AgreementPool.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/CriticalMessagesPrefs.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/CivilisationData.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/PlayHap.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/DiplomaticRequest.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/WonderTracker.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/Diffcly.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/unitutil.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/GameSettings.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/installationpool.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/installationpool.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/ID.h` (0/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/gameobj/ArmyData.h` (0/1/0) — 🟡 moderate · _needs ownership review_

## gs/slic  (21 files, 927 matches)

- [x] `gs/slic/SlicEngine.cpp` — done 2026-08-03: ui-execute and context lists held by value; SlicObject entries stay reference counted (partial: other raw owners remain)
- [ ] `gs/slic/SlicBuiltin.cpp` (79/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/slic/SlicContext.cpp` (26/25/0) — 🟡 moderate · _single-owner member_
- [ ] `gs/slic/slicfunc.cpp` (33/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/slic/slicif.cpp` (6/3/17) — 🔴 hard · _void* ownership handoff_
- [ ] `gs/slic/SlicSegment.cpp` (9/8/7) — ⚪ leave · _pool/arena allocator_
- [ ] `gs/slic/SlicStruct.cpp` (12/8/0) — 🔴 hard · _factory returns crossing callers_
- [ ] `gs/slic/SlicSymbol.cpp` (6/10/0) — ⚪ leave · _pool/arena allocator_
- [ ] `gs/slic/slicobject.cpp` (10/5/0) — 🟡 moderate · _single-owner member_
- [ ] `gs/slic/SlicSymTab.cpp` (4/4/6) — 🟡 moderate · _single-owner member_
- [ ] `gs/slic/sliccmd.cpp` (6/2/0) — ⚪ leave · _pool/arena allocator_
- [ ] `gs/slic/SlicFrame.cpp` (5/3/0) — 🟡 moderate · _single-owner member_
- [ ] `gs/slic/StringHash.h` (2/5/0) — 🟡 moderate · _single-owner member_
- [ ] `gs/slic/SlicButton.cpp` (5/2/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/slic/SlicBuiltin.h` (7/0/0) — 🔴 hard · _factory returns crossing callers_
- [ ] `gs/slic/SlicArray.cpp` (3/3/0) — 🟡 moderate · _single-owner member_
- [ ] `gs/slic/SlicEyePoint.cpp` (4/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/slic/SlicSegment.h` (1/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/slic/SlicStruct.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/slic/SlicDBConduit.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/slic/QuickSlic.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_

## ui/netshell  (29 files, 518 matches)

- [x] `ui/netshell/allinonewindow.cpp` — done 2026-08-03: 9 owned members are unique_ptr; m_aiplayerList (tech_WLList) left raw
- [ ] `ui/netshell/netfunc.cpp` (63/14/1) — 🔴 hard · _mixed new[]/malloc buffers_
- [ ] `ui/netshell/netshell.cpp` (28/8/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/netshell/lobbywindow.cpp` (27/4/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/netshell/ns_customlistbox.cpp` (16/11/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/netshell/playereditwindow.cpp` (25/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/netshell/gameselectwindow.cpp` (21/2/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/netshell/dialogboxwindow.cpp` (16/4/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/netshell/passwordscreen.cpp` (15/2/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/netshell/playerselectwindow.cpp` (16/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/netshell/connectionselectwindow.cpp` (12/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/netshell/ns_item.cpp` (10/2/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/netshell/serverselectwindow.cpp` (9/2/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/netshell/lobbychangewindow.cpp` (10/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/netshell/ns_chatbox.cpp` (5/5/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/netshell/ns_civlistbox.cpp` (6/2/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/netshell/ns_listbox.h` (2/4/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/netshell/netfunc.h` (2/3/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/netshell/ns_tribes.cpp` (3/1/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/netshell/ns_window.cpp` (0/4/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/netshell/ns_customlistbox.h` (1/1/0) — 🟡 · _item transferred to listbox_
- [ ] `ui/netshell/ns_improvements.cpp` (1/1/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/netshell/ns_item.h` (1/1/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/netshell/ns_units.cpp` (1/1/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/netshell/ns_units.h` (1/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/netshell/ns_header.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/netshell/ns_wonders.cpp` (1/1/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/netshell/netshell_game.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/netshell/ns_gamesetup.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_

## ui/aui_ctp2  (57 files, 469 matches)

- [ ] `ui/aui_ctp2/c3windows.cpp` (46/41/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/c3_utilitydialogbox.cpp` (37/35/0) — 🟡 moderate · _single-owner member_
- [x] `ui/aui_ctp2/battleorderbox.cpp` — done 2026-08-03: 12 controls are unique_ptr; RemoveControl() macro calls became reset() (the macro deletes despite its name)
- [x] `ui/aui_ctp2/c3_popupwindow.cpp` — done 2026-08-03: 4 controls are unique_ptr; default ctor moved out of line (forward-declared control types); m_border array still raw
- [ ] `ui/aui_ctp2/chart.cpp` (10/7/0) — 🔴 hard · _void* ownership handoff_
- [ ] `ui/aui_ctp2/c3_ranger.cpp` (14/1/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_ctp2/thronecontrol.cpp` (6/8/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_ctp2/SelItem.cpp` (6/5/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_ctp2/linegraph.cpp` (6/3/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/ctp2_listbox.cpp` (6/3/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_ctp2/c3_listbox.cpp` (6/3/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_ctp2/textbox.cpp` (7/1/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_ctp2/radarmap.cpp` (1/4/3) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_ctp2/c3_fancywindow.cpp` (4/4/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_ctp2/ctp2_dropdown.cpp` (7/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/ctp2_Menu.cpp` (3/4/0) — ⚪ leave · _pool/arena allocator_
- [ ] `ui/aui_ctp2/c3_hypertextbox.cpp` (4/3/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_ctp2/c3_dropdown.cpp` (7/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/ctp2_hypertextbox.cpp` (4/2/0) — 🟡 moderate · _single-owner member_
- [x] `ui/aui_ctp2/unittabbutton.cpp` — done 2026-08-03: 5 controls are unique_ptr; m_cargo array still freed by hand
- [ ] `ui/aui_ctp2/background.cpp` (5/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/picturebutton.cpp` (2/4/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_ctp2/c3ui.cpp` (3/3/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_ctp2/texttable.cpp` (5/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/cityinventorylistbox.cpp` (5/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/c3scroller.cpp` (5/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/keypress.cpp` (4/1/0) — ⚪ · _global keymap singleton_
- [ ] `ui/aui_ctp2/thumbnailmap.cpp` (1/4/0) — ⚪ leave · _pool/arena allocator_
- [ ] `ui/aui_ctp2/c3_button.cpp` (2/2/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_ctp2/c3_tradelistitem.cpp` (4/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/InfoBar.cpp` (2/2/0) — ⚪ leave · _pool/arena allocator_
- [ ] `ui/aui_ctp2/c3fancywindow.cpp` (2/2/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_ctp2/ui_game_observer.cpp` (2/2/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/staticpicture.cpp` (2/2/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_ctp2/pattern.cpp` (2/2/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_ctp2/c3spinner.cpp` (4/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/c3_header.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/videowindow.cpp` (1/2/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_ctp2/grabitem.cpp` (1/2/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ui/aui_ctp2/c3dropdown.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/c3_headerswitch.cpp` (1/1/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_ctp2/background.h` (1/1/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_ctp2/player_view_ui.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/c3_hypertipwindow.cpp` (1/1/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_ctp2/c3listbox.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/SelItemClick.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/c3memmap.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/c3_listitem.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/C3slider.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/c3_slidometer.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/ctp2_menubar.cpp` (2/0/0) — 🔴 hard · _void* ownership handoff_
- [ ] `ui/aui_ctp2/ctp2_textbuffer.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/c3_updateaction.cpp` (1/0/0) — 🔴 hard · _void* ownership handoff_
- [ ] `ui/aui_ctp2/ctp2_Window.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/ctp2_listbox.h` (0/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/ctp2_MenuButton.cpp` (0/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_ctp2/picture.cpp` (0/1/0) — 🟡 moderate · _needs ownership review_

## net/general  (28 files, 396 matches)

- [ ] `net/general/network.cpp` (217/27/0) — ⚪ leave · _pool/arena allocator_
- [ ] `net/general/net_action.cpp` (31/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `net/general/net_info.cpp` (24/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `net/general/net_gameobj.cpp` (5/6/0) — ⚪ leave · _pool/arena allocator_
- [ ] `net/general/net_gamesettings.cpp` (5/5/0) — ⚪ leave · _pool/arena allocator_
- [ ] `net/general/networkevent.cpp` (10/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `net/general/net_cell.cpp` (3/6/0) — ⚪ leave · _pool/arena allocator_
- [ ] `net/general/net_diplomacy.cpp` (5/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `net/general/net_vision.cpp` (5/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `net/general/net_army.cpp` (5/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `net/general/net_crc.cpp` (5/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `net/general/net_agreement.cpp` (4/1/0) — 🟡 moderate · _single-owner member_
- [ ] `net/general/net_hash.h` (2/2/0) — ⚪ leave · _pool/arena allocator_
- [ ] `net/general/net_unit.cpp` (4/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `net/general/net_order.cpp` (2/1/0) — 🟡 moderate · _single-owner member_
- [ ] `net/general/net_message.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `net/general/net_endgame.cpp` (2/1/0) — ⚪ · _global Wormhole singleton + list_
- [ ] `net/general/net_civ.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `net/general/net_city.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `net/general/chatlist.h` (1/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `net/general/net_installation.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `net/general/net_exclusions.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `net/general/net_terrain.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `net/general/net_tradeoffer.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `net/general/net_traderoute.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `net/general/net_report.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `net/general/net_feat.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `net/general/net_packet.h` (0/1/0) — 🟡 moderate · _needs ownership review_

## ui/aui_common  (38 files, 288 matches)

- [x] `ui/aui_common/aui_ui.cpp` — done 2026-08-03: tech_Memory pool held by value, not new/delete
- [ ] `ui/aui_common/aui_ldl.cpp` (32/13/0) — 🔴 hard · _void* ownership handoff_
- [ ] `ui/aui_common/aui_listbox.cpp` (13/12/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_common/aui_resource.h` (5/8/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_common/aui_window.cpp` (3/8/2) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_common/aui_region.cpp` (7/5/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_common/aui_dropdown.cpp` (7/4/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_common/aui_ranger.cpp` (5/6/0) — 🔴 hard · _void* ownership handoff_
- [ ] `ui/aui_common/aui_image.cpp` (4/4/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_common/aui_control.cpp` (4/4/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_common/aui_textfield.cpp` (3/4/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_common/aui_imagebase.cpp` (1/5/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_common/aui_Factory.cpp` (6/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_common/aui_mouse.cpp` (1/4/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_common/aui_hypertextbase.cpp` (2/3/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_common/aui_memmap.cpp` (3/2/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_common/aui_pixel.cpp` (2/3/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_common/aui_bitmapfont.cpp` (3/2/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_common/aui_hypertextbox.cpp` (2/2/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_common/aui_stringtable.cpp` (1/3/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_common/aui_header.cpp` (2/2/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_common/aui_screen.cpp` (2/2/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_common/aui_tab.cpp` (2/1/0) — 🔴 hard · _void* ownership handoff_
- [ ] `ui/aui_common/tech_memmap.cpp` (1/2/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_common/tech_memory.h` (2/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_common/aui_textbase.cpp` (2/1/0) — 🔴 hard · _void* ownership handoff_
- [ ] `ui/aui_common/aui_win.cpp` (1/1/0) — 🔴 hard · _void* ownership handoff_
- [ ] `ui/aui_common/aui_base.cpp` (1/1/0) — ⚪ leave · _mostly g_/s_ singletons_
- [x] `ui/aui_common/aui_dirtylist.cpp` — done 2026-08-03: tech_Memory pool held by value, not new/delete
- [ ] `ui/aui_common/aui_tipwindow.cpp` (1/1/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_common/aui_surface.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_common/aui_textbox.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_common/aui_moviemanager.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_common/aui_shell.cpp` (1/1/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_common/aui_region.h` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_common/tech_wllist.h` (1/1/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_common/aui_window.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/aui_common/aui_blitter.cpp` (0/0/1) — 🟡 moderate · _needs ownership review_

## gfx/spritesys  (23 files, 260 matches)

- [ ] `gfx/spritesys/director.cpp` (40/7/0) — 🟡 moderate · _needs ownership review_
- [ ] `gfx/spritesys/spritefile.cpp` (45/2/0) — 🔴 hard · _void* ownership handoff_
- [x] `gfx/spritesys/UnitSpriteGroup.cpp` — done 2026-08-03 (b1169bf7): delete+assign on the base slots became reset()
- [ ] `gfx/spritesys/DirectorActionHandlers.cpp` (16/6/0) — 🟡 moderate · _needs ownership review_
- [x] `gfx/spritesys/Sprite.cpp` — done 2026-08-03: frames own their buffers via SpriteFrame, matching the two faced subclasses; whole hierarchy now shares one frame representation
- [x] `gfx/spritesys/effectspritegroup.cpp` — done 2026-08-03 (b1169bf7): delete+assign on the base slots became reset()
- [x] `gfx/spritesys/goodspritegroup.cpp` — done 2026-08-03 (b1169bf7): delete+assign on the base slots became reset()
- [x] `gfx/spritesys/SpriteGroupList.cpp` — done 2026-08-03: slots are unique_ptr; store-back guards self-assignment (loader reuses the slot it read)
- [x] `gfx/spritesys/FacedSpriteWshadow.cpp` — done 2026-08-03 (f05e888d): frames own their buffers via SpriteFrame; fixed 8 scalar deletes on array-new'd memory
- [x] `gfx/spritesys/FacedSprite.cpp` — done 2026-08-03 (b2d108fe): frames own their buffers via SpriteFrame; fixed 4 scalar deletes on array-new'd memory
- [x] `gfx/spritesys/UnitActor.cpp` (6/0/0)
- [x] `gfx/spritesys/SpriteStateDB.cpp` (1/4/0)
- [ ] `gfx/spritesys/spriteutils.cpp` (2/0/2) — 🔴 hard · _mixed new[]/malloc buffers_
- [x] `gfx/spritesys/SpriteGroup.cpp` — done 2026-08-03 (b1169bf7): sprite/anim slots are unique_ptr; setters guard self-assignment (the loader does get-modify-set)
- [x] `gfx/spritesys/battleviewactor.cpp` (2/0/0)
- [ ] `gfx/spritesys/action.cpp` (1/1/0) — 🟡 moderate · _single-owner member_
- [x] `gfx/spritesys/goodactor.cpp` (2/0/0)
- [x] `gfx/spritesys/TradeActor.cpp` (2/0/0)
- [ ] `gfx/spritesys/goodactor_factory_impl.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_
- [x] `gfx/spritesys/DirectorActions.cpp` (0/2/0)
- [x] `gfx/spritesys/workeractor.cpp` (1/0/0)
- [x] `gfx/spritesys/EffectActor.cpp` (1/0/0)
- [x] `gfx/spritesys/Actor.cpp` (1/0/0)

## test/cpp  (20 files, 221 matches)

- [ ] `test/cpp/doctest.h` (13/33/0) — 🔴 hard · _factory returns crossing callers_
- [ ] `test/cpp/test_json_save.cpp` (27/6/0) — 🟡 moderate · _needs ownership review_
- [ ] `test/cpp/test_player_view.cpp` (26/2/0) — 🟡 moderate · _needs ownership review_
- [ ] `test/cpp/test_save_load.cpp` (25/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `test/cpp/test_citydata.cpp` (18/5/0) — 🟡 moderate · _needs ownership review_
- [ ] `test/cpp/test_headless_smoke.cpp` (11/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `test/cpp/test_json_save_integration.cpp` (10/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `test/cpp/scoped_rand.h` (2/5/0) — 🟡 moderate · _needs ownership review_
- [ ] `test/cpp/test_building_evaluator.cpp` (4/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `test/cpp/test_ui_command_surface.cpp` (5/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `test/cpp/test_city_visibility.cpp` (5/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `test/cpp/test_headless_long.cpp` (4/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `test/cpp/test_globals_cleanup_smoke.cpp` (4/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `test/cpp/test_game.cpp` (3/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `test/cpp/test_buildqueue.cpp` (3/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `test/cpp/test_happy.cpp` (2/0/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `test/cpp/test_headless_progression.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `test/cpp/test_json_world.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `test/cpp/test_headless_determinism.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `test/cpp/test_save_determinism.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_

## gs/utility  (10 files, 195 matches)

- [ ] `gs/utility/gameinit.cpp` (96/16/0) — ⚪ leave · _pool/arena allocator_
- [x] `gs/utility/TurnCnt.cpp` (14/1/0) — ✅ done · _m_sliceList -> unique_ptr; residual news are net-transfer (deferred) + refcounted SlicObject (off-limits)_
- [ ] `gs/utility/QuadTree.h` (11/12/0) — 🟡 moderate · _single-owner member_
- [ ] `gs/utility/newturncount.cpp` (11/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/utility/Globals.h` (4/5/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/utility/DataCheck.cpp` (7/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/utility/DataCheck.h` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/utility/RandGen.h` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/utility/TurnCntEvent.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/utility/gameinit.h` (1/0/0) — 🟡 moderate · _needs ownership review_

## gs/fileio  (8 files, 155 matches)

- [ ] `gs/fileio/json_save.cpp` (58/39/8) — ⚪ leave · _pool/arena allocator_
- [ ] `gs/fileio/GameFile.cpp` (17/3/0) — ⚪ leave · _pool/arena allocator_
- [ ] `gs/fileio/civscenarios.cpp` (7/7/0) — ⚪ leave · _pool/arena allocator_
- [ ] `gs/fileio/prjfile.cpp` (0/0/8) — 🟡 moderate · _needs ownership review_
- [ ] `gs/fileio/CivPaths.cpp` (2/2/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `gs/fileio/Token.h` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/fileio/Token.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/fileio/action_log.h` (1/0/0) — 🟡 moderate · _needs ownership review_

## gs/world  (14 files, 126 matches)

- [ ] `gs/world/wldgen.cpp` (22/17/0) — ⚪ leave · _pool/arena allocator_
- [ ] `gs/world/UnseenCell.cpp` (17/3/0) — ⚪ leave · _pool/arena allocator_
- [ ] `gs/world/WrlEnv.cpp` (6/10/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/world/WrldCont.cpp` (7/8/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/world/Cell.cpp` (5/9/0) — ⚪ leave · _pool/arena allocator_
- [ ] `gs/world/WrldPoll.cpp` (5/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/world/TileInfo.cpp` (2/3/0) — 🔴 hard · _`*this=*copy` copy-ctor blocks unique_ptr (needs custom operator=)_
- [ ] `gs/world/WrldCity.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/world/worldutils.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/world/WorldDistance.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/world/worldevent.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/world/cellunitlist.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/world/UnseenCell.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/world/UnseenCellQuadTree.h` (0/1/0) — 🟡 moderate · _needs ownership review_

## ctp  (6 files, 124 matches)

- [ ] `ctp/civapp.cpp` (83/9/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ctp/display.cpp` (9/3/0) — ⚪ leave · _pool/arena allocator_
- [ ] `ctp/civ3_main.cpp` (9/0/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ctp/headless_main.cpp` (7/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ctp/game_controller.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ctp/civapp.h` (0/2/0) — 🟡 moderate · _needs ownership review_

## ctp/ctp2_utils  (9 files, 120 matches)

- [ ] `ctp/ctp2_utils/c3cmdline.cpp` (50/30/0) — 🟡 moderate · _needs ownership review_
- [ ] `ctp/ctp2_utils/pointerlist.h` (4/6/0) — 🟡 moderate · _needs ownership review_
- [ ] `ctp/ctp2_utils/c3files.cpp` (7/3/0) — 🟡 moderate · _needs ownership review_
- [ ] `ctp/ctp2_utils/AvlTree.h` (2/6/0) — 🟡 moderate · _needs ownership review_
- [ ] `ctp/ctp2_utils/c3mem.cpp` (1/1/2) — 🟡 moderate · _needs ownership review_
- [ ] `ctp/ctp2_utils/appstrings.cpp` (2/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `ctp/ctp2_utils/netconsole.cpp` (1/1/0) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `ctp/ctp2_utils/c3errors.cpp` (0/0/2) — 🟡 moderate · _needs ownership review_
- [ ] `ctp/ctp2_utils/civlog.h` (1/0/0) — 🟡 moderate · _needs ownership review_

## gfx/tilesys  (9 files, 93 matches)

- [ ] `gfx/tilesys/tileutils.cpp` (15/5/7) — 🔴 hard · _mixed new[]/malloc buffers_
- [ ] `gfx/tilesys/tileset.cpp` (9/13/0) — 🔴 hard · _void* ownership handoff_
- [ ] `gfx/tilesys/tiledmap.cpp` (10/10/0) — 🟡 moderate · _single-owner member_
- [ ] `gfx/tilesys/workmap.cpp` (1/6/0) — 🔴 hard · _void* ownership handoff_
- [ ] `gfx/tilesys/tiledmap_observer_adapter.cpp` (2/3/0) — 🟡 moderate · _needs ownership review_
- [ ] `gfx/tilesys/BaseTile.cpp` (2/2/0) — 🔴 hard · _void* ownership handoff_
- [ ] `gfx/tilesys/resourcemap.cpp` (1/3/0) — 🔴 hard · _void* ownership handoff_
- [ ] `gfx/tilesys/tiledraw.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gfx/tilesys/tileset.h` (2/0/0) — 🟡 moderate · _needs ownership review_

## gs/newdb  (5 files, 66 matches)

- [ ] `gs/newdb/CTPDatabase.cpp` (24/13/0) — ⚪ leave · _pool/arena allocator_
- [ ] `gs/newdb/DBLexer.cpp` (9/7/0) — ⚪ leave · _pool/arena allocator_
- [ ] `gs/newdb/CTPRecord.cpp` (6/4/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/newdb/CTPRecord.h` (1/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/newdb/DBTokens.h` (1/0/0) — 🟡 moderate · _needs ownership review_

## gs/database  (11 files, 64 matches)

- [ ] `gs/database/StrDB.cpp` (17/5/0) — 🟡 moderate · _needs ownership review_
- [x] `gs/database/profileDB.cpp` — done 2026-08-03: PointerList member held by value (partial: other raw owners remain in this file)
- [ ] `gs/database/profileDB.h` (6/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/database/moviedb.cpp` (1/4/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/database/filenamedb.cpp` (1/4/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/database/PlayListDB.cpp` (1/3/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/database/highscoredb.cpp` (1/2/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/database/thronedb.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/database/thronedb.h` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/database/EndGameDB.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/database/UVDB.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_

## gs/dbgen  (4 files, 57 matches)

- [ ] `gs/dbgen/Datum.cpp` (14/15/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/dbgen/ctpdb.cpp` (4/3/5) — ⚪ leave · _mostly g_/s_ singletons_
- [ ] `gs/dbgen/RecordDescription.cpp` (9/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/dbgen/MemberClass.cpp` (5/2/0) — 🟡 moderate · _needs ownership review_

## 3rdparty/nlohmann  (1 files, 55 matches)

- [ ] `3rdparty/nlohmann/json.hpp` (21/34/0) — ⚪ leave · _vendored / upstream_

## net/io  (4 files, 51 matches)

- [ ] `net/io/net_thread.cpp` (20/13/0) — ⚪ leave · _pool/arena allocator_
- [ ] `net/io/net_anet.cpp` (5/5/0) — 🔴 hard · _void* ownership handoff_
- [ ] `net/io/net_list.h` (3/4/0) — 🟡 moderate · _needs ownership review_
- [ ] `net/io/net_util.h` (1/0/0) — 🟡 moderate · _needs ownership review_

## ui/slic_debug  (3 files, 51 matches)

- [x] `ui/slic_debug/sourcelist.cpp` — done 2026-08-03: 7 controls are unique_ptr; window now released last (hand-written order freed it third); local delete-macro removed
- [x] `ui/slic_debug/watchlist.cpp` — done 2026-08-03: 5 WatchList controls are unique_ptr; WatchListItem::m_watching deliberately left raw (different class)
- [ ] `ui/slic_debug/segmentlist.cpp` (7/4/0) — 🟡 moderate · _single-owner member_

## gs/events  (6 files, 34 matches)

- [x] `gs/events/GameEventManager.cpp` — done 2026-08-03: PointerList member held by value (partial: other raw owners remain in this file)
- [ ] `gs/events/GameEventDescription.h` (11/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/events/GameEventArgList.cpp` (3/1/0) — ⚪ leave · _pool/arena allocator_
- [ ] `gs/events/GameEventArgument.cpp` (1/1/0) — 🔴 hard · _void* ownership handoff_
- [ ] `gs/events/GameEvent.cpp` (0/2/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/events/GameEventHook.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_

## ui/ldl  (7 files, 33 matches)

- [ ] `ui/ldl/ldlif.cpp` (12/7/0) — ⚪ leave · _pool/arena allocator_
- [ ] `ui/ldl/ldl_attr.cpp` (4/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/ldl/ldl_data.cpp` (3/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/ldl/ldl_memmap.cpp` (1/2/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/ldl/ldl_data.hpp` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/ldl/ldl_file.hpp` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ui/ldl/ldl_list.h` (0/1/0) — 🟡 moderate · _needs ownership review_

## gs/core  (11 files, 31 matches)

- [ ] `gs/core/game.cpp` (1/6/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/core/battle_observer.h` (4/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/core/game_observer.h` (2/2/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/core/tiledmap_observer.h` (2/2/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/core/goodactor_factory.h` (2/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/core/goodactor_factory.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/core/game.h` (0/2/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/core/diplomacy_observer.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/core/game_observer_registration.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/core/progress_observer.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gs/core/player_view.h` (1/0/0) — 🟡 moderate · _needs ownership review_

## ai/strategy  (4 files, 31 matches)

- [ ] `ai/strategy/scheduler/scheduler.cpp` (14/8/0) — 🟡 moderate · _needs ownership review_
- [ ] `ai/strategy/goals/Goal.cpp` (2/2/0) — 🟡 moderate · _needs ownership review_
- [ ] `ai/strategy/scheduler/Plan.cpp` (2/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `ai/strategy/agents/agent.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_

## gfx/gfx_utils  (8 files, 22 matches)

- [ ] `gfx/gfx_utils/gfx_options.cpp` (5/5/0) — 🟡 moderate · _needs ownership review_
- [ ] `gfx/gfx_utils/tiffutils.cpp` (0/0/4) — 🟡 moderate · _needs ownership review_
- [ ] `gfx/gfx_utils/videoutils.cpp` (2/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `gfx/gfx_utils/Queue.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gfx/gfx_utils/pixeltypes.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gfx/gfx_utils/colorset.h` (1/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `gfx/gfx_utils/arproces.cpp` (0/0/1) — 🟡 moderate · _needs ownership review_
- [ ] `gfx/gfx_utils/pixelutils.cpp` (0/0/1) — 🟡 moderate · _needs ownership review_

## ai/diplomacy  (2 files, 18 matches)

- [ ] `ai/diplomacy/diplomat.cpp` (17/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `ai/diplomacy/regardevent.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_

## robot/aibackdoor  (3 files, 17 matches)

- [ ] `robot/aibackdoor/dynarr.h` (5/5/0) — 🟡 moderate · _needs ownership review_
- [ ] `robot/aibackdoor/semi_dynamic_array.h` (0/0/4) — 🟡 moderate · _needs ownership review_
- [ ] `robot/aibackdoor/pool.h` (1/2/0) — 🟡 moderate · _needs ownership review_

## ctp/fingerprint  (1 files, 17 matches)

- [ ] `ctp/fingerprint/shroud.c` (4/0/13) — 🟡 moderate · _needs ownership review_

## ai  (1 files, 16 matches)

- [ ] `ai/ctpai.cpp` (16/0/0) — 🟡 moderate · _needs ownership review_

## sound  (1 files, 15 matches)

- [x] `sound/soundmanager.cpp` — done 2026-08-03: sfx and voice lists held by value (partial: m_soundWalker remains)

## mapgen  (5 files, 15 matches)

- [ ] `mapgen/Geometric.cpp` (3/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `mapgen/Crater.cpp` (2/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `mapgen/FaultGen.cpp` (2/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `mapgen/PlasmaGen2.cpp` (2/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `mapgen/PlasmaGen1.cpp` (1/1/0) — 🟡 moderate · _needs ownership review_

## ai/CityManagement  (2 files, 15 matches)

- [ ] `ai/CityManagement/governor.cpp` (12/1/0) — 🟡 moderate · _needs ownership review_
- [ ] `ai/CityManagement/governor.h` (2/0/0) — 🟡 moderate · _needs ownership review_

## ui/aui_sdl  (2 files, 7 matches)

- [ ] `ui/aui_sdl/aui_sdlui.cpp` (3/3/0) — 🟡 moderate · _single-owner member_
- [ ] `ui/aui_sdl/aui_sdlsurface.cpp` (1/0/0) — 🟡 moderate · _needs ownership review_

## ai/mapanalysis  (1 files, 4 matches)

- [x] `ai/mapanalysis/settlemap.cpp` (1/3/0) — ✅ done · _local bool[] scratch -> vector<bool>_

## ui/freetype  (1 files, 4 matches)

- [ ] `ui/freetype/freetype.h` (4/0/0) — 🟡 moderate · _needs ownership review_

## robot/utility  (1 files, 3 matches)

- [ ] `robot/utility/roboinit.cpp` (1/2/0) — ⚪ leave · _mostly g_/s_ singletons_

## robot/pathing  (2 files, 3 matches)

- [ ] `robot/pathing/astar.cpp` (2/0/0) — 🟡 moderate · _needs ownership review_
- [ ] `robot/pathing/TestP.cpp` (1/0/0) — ⚪ leave · _mostly g_/s_ singletons_

## gs/outcom  (1 files, 3 matches)

- [ ] `gs/outcom/c3rand.cpp` (1/2/0) — ⚪ leave · _conditional/borrowed owner (m_rand=rand_ptr() when !ownGenerator) + refcounted_

## ui/aui_directx  (1 files, 2 matches)

- [ ] `ui/aui_directx/aui_directmoviemanager.cpp` (1/1/0) — 🟡 moderate · _single-owner member_

## ctp/ctp2_rsrc  (1 files, 1 matches)

- [ ] `ctp/ctp2_rsrc/resource.h` (1/0/0) — 🟡 moderate · _needs ownership review_

## ui/aui_utils  (1 files, 1 matches)

- [ ] `ui/aui_utils/textutils.cpp` (0/1/0) — 🟡 moderate · _needs ownership review_

## os/nowin32  (1 files, 1 matches)

- [ ] `os/nowin32/nowin32.cpp` (0/0/1) — 🟡 moderate · _needs ownership review_
