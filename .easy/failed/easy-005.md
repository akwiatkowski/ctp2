# Easy Ticket: SCOUT-3-073

## Source
/Users/olek/projects/llm/games/ctp2/.easy/../.scouts/tickets/H-container--ctp2_code-build-Record-files--01.md

## Change Required
Pattern: C-style fixed arrays in generated Record headers

## Current State
Several generated Record classes use raw C-style fixed arrays where `std::array` (or `std::vector` for variable-size) would be safer and more idiomatic:

```cpp
// File: ctp2_code/build/AdvanceRecord.h
// Lines: 73, 90, 56
sint32 m_Prerequisites[k_MAX_Prerequisites];        // k_MAX_Prerequisites = 4
sint32 m_EitherPrerequisites[k_MAX_EitherPrerequisites]; // k_MAX_EitherPrerequisites = 4
```

```cpp
// File: ctp2_code/build/BuildingRecord.h
// Line: 189
sint32 m_ObsoleteAdvance[k_MAX_ObsoleteAdvance];    // k_MAX_ObsoleteAdvance = 5
```

```cpp
// File: ctp2_code/build/CivilisationRecord.h
// Line: 74
sint32 m_CityName[k_MAX_CityName];                  // k_MAX_CityName = 500
```

## Code Evidence


## Suggested Direction
Replace fixed C arrays with `std::array<sint32, N>` for type safety and bounds checking. For the variable-size cases that already track `m_numFoo`, consider promoting to `std::vector<sint32>` entirely (see companion memory ticket SCOUT-3-071).

Because these files are auto-generated, the fix belongs in the ctpdb code generator templates.

## Files You May Edit
ctp2_code/build/*Record.h

## Acceptance Criteria (ALL must pass)
1. Build passes: `mise exec -- meson compile -C build ctp2_fast_tests`
2. Fast tests pass: `./build/ctp2_fast_tests`
3. Diff touches ONLY the files listed above
4. No new compiler warnings
5. Commit with conventional commit message

## Commit (REQUIRED)
```bash
git add ctp2_code/build/*Record.h
git commit -m "refactor(build): replace C arrays with std::array"
```


---

## WORKER ATTEMPT FAILED

**Date:** 2026-05-28T17:17:45.324087
**Reason:** no changes made

### Diff Attempt

```diff

```

### Next Steps

This ticket needs manual review or a different approach.
