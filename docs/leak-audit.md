<!-- Generated and maintained by Claude -->

# Never-released members — audit (2026-08-03)

A by-product of converting owning members to `std::unique_ptr`: the converter
refuses any member it cannot find a release site for, on the grounds that a
member nobody frees is probably not owned here. Run over the whole tree, that
check becomes a leak detector.

**29 members across 15 files are allocated with `new` and never released
anywhere in the tree.** The list is at the bottom. It is a list of *candidates*,
not confirmed leaks — see the caveats first, because the first three passes of
this audit were mostly wrong.

## What made the earlier passes wrong

The naive question — "is there a `delete m_x;` in the same file?" — produced 129
candidates, most of them false. Three separate reasons:

1. **A base class frees it.** `c3_Ranger` allocates `m_thumb`, `m_incXButton`
   and friends; `aui_Ranger::~aui_Ranger` deletes them. Release has to be
   searched for across the whole tree, not per file.

2. **Macros hide the delete.** There is no single release idiom. Besides plain
   `delete m_x;`:

   | Idiom | Where |
   |---|---|
   | `DeleteControl(p)` | macro in `ui/interface/UIUtils.h` |
   | `mycleanup(p)` | the *same two lines*, redefined locally in at least 7 files — `victorywin.cpp` defines it twice |
   | `CleanUp(control)` | a method, on `EndGameWindow` and `CreditsScreen` |
   | `p.reset()` | `unique_ptr` members |

   Counting only the first of those put all 12 of `EndgameWindow.cpp`'s controls
   on the list; they are released through `CleanUp`.

3. **Nested struct members.** `messagewindow.cpp` reaches
   `m_messageEyePoint.m_messageEyePointStandard`. The scan keys on the trailing
   name and cannot tell that from a direct member, so entries whose owner is a
   nested struct need checking by hand.

4. **Destructors that free children generically.** `c3_ListItem::~c3_ListItem`
   walks `m_childList` and deletes every entry. Nothing names the members, so no
   name-based scan can see it. This one is easy to get wrong twice over:
   `c3_ListItem` and `ctp2_ListItem` are *different* classes that both derive
   from `aui_Item`, and only `ctp2_ListItem` frees children through the obvious
   `DeleteChildren()`. Checking the wrong one of the pair gives the wrong
   answer.

## Confidence

One entry is independently corroborated:

- `spriteeditor.cpp` `m_actionObj` — the file carries
  `// delete m_actionObj : TODO (crashes)`. Not released, and known not to be.

**One entry has since been disproved, and it was the largest.** `ns_item.cpp`'s
seven controls looked like the strongest case — allocated, `AddChild`'d, and
`ns_HPlayerItem` declares no destructor. But it derives from `c3_ListItem`,
whose destructor walks `m_childList` and deletes every child. They are freed.
That is caveat 4 above, discovered by trying to fix this entry. The row is
struck through below rather than deleted, because it is the clearest warning
this table carries: **every row needs the ownership chain walked by hand, and a
missing destructor is not evidence of anything.**

One entry from the same audit has already been fixed: `messagewindow.cpp`'s four
border bars (`m_leftBar`, `m_topBar`, `m_rightBar`, `m_bottomBar`) were
allocated, `AddControl`'d and never freed. Fixed in ebdc2804 by making them
owning members, after confirming nothing else could own them:

- `aui_Window::AddControl` forwards to `aui_Region::AddChild`;
- `aui_Window::~aui_Window` frees its surface, dirty list, grab region, focus
  control and focus list — not its children;
- `aui_Region::~aui_Region` frees `m_childList`, the list, not the children in
  it. Recursive child deletion lives in an explicit `DeleteChildren()`, called
  from exactly one place in the tree (`ctp2_listitem.cpp`).

That chain is the thing to re-check for every UI entry below: **a control being
added to a parent does not give the parent ownership of it.**

## Candidates

| Count | File | Members |
|---:|---|---|
| ~~7~~ | ~~`ui/netshell/ns_item.cpp`~~ | **NOT A LEAK** — freed by `c3_ListItem::~c3_ListItem`, which deletes every child in `m_childList` |
| 5 | `ui/interface/messagewindow.cpp` | `m_messageEyePointDropdown`, `m_messageEyePointListbox`, `m_messageEyePointStandard`, `m_messageResponseDropdown`, `m_messageResponseStandard` — nested in `m_messageEyePoint` / `m_messageResponse`, verify by hand |
| 3 | `ui/interface/messagemodal.cpp` | `m_messageEyePointDropdown`, `m_messageEyePointListbox`, `m_messageEyePointStandard` — same nesting caveat |
| 3 | `ui/aui_sdl/aui_sdlui.cpp` | `m_fogSurface`, `m_uiSurface`, `m_worldSurface` |
| 1 | `gs/fileio/json_save.cpp` | `m_struct` |
| 1 | `gs/gameobj/Vision.cpp` | `m_unseenCell` |
| 1 | `gs/slic/SlicArray.cpp` | `m_sym` |
| 1 | `gs/slic/SlicSymbol.cpp` | `m_struct` |
| 1 | `net/general/net_unit.cpp` | `m_unitData` |
| 1 | `net/general/net_traderoute.cpp` | `m_routeData` |
| 1 | `net/general/net_tradeoffer.cpp` | `m_offerData` |
| 1 | `net/general/net_vision.cpp` | `m_ucell` |
| 1 | `ui/interface/battle_observer_adapter.cpp` | `m_pendingPlacement` |
| 1 | `ui/interface/messageadvice.cpp` | `m_dismissButton` |
| 1 | `ui/interface/spriteeditor.cpp` | `m_actionObj` — known, releasing it crashes |

Of the remaining rows, none has been verified either way. The `net/general/*`
entries share a shape (`m_unitData`, `m_routeData`,
`m_offerData`, `m_ucell`) and are probably one decision rather than four.

## Reproducing

The scan is a few lines: collect every release site tree-wide using the idiom
table above plus `unique_ptr` declarations, collect every `m_x = new T`, and
subtract. Read sources as `latin-1` — the tree mixes encodings and a `utf-8`
read throws on the ISO-8859 files, which is also why plain `grep` silently skips
some of them.
