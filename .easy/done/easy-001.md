# Easy Ticket: SCOUT-1-001

## Source
/Users/olek/projects/llm/games/ctp2/.easy/../.scouts/tickets/H-container--ctp2_code-ai-CityManagement-governor-h--01.md

## Change Required
C-style array BuildUnitList should be std::array or std::vector

## Current State
The `Governor` class stores per-category unit build lists in a raw C-style array:
`BuildUnitList m_buildUnitList[BUILD_UNIT_LIST_MAX];`

This prevents easy bounds-checked iteration, prevents the class from being copyable
without manual deep-copy logic, and is inconsistent with the rest of the class which
already uses `std::vector` for other collections.

## Code Evidence
```cpp
// File: ctp2_code/ai/CityManagement/governor.h
// Lines: 472
    BuildUnitList       m_buildUnitList[BUILD_UNIT_LIST_MAX];
```

## Suggested Direction
Replace with `std::array<BuildUnitList, BUILD_UNIT_LIST_MAX>` or `std::vector<BuildUnitList>`.
The enum `BUILD_UNIT_LIST_MAX` is already defined at line 129, so `std::array` is a
drop-in replacement that preserves stack allocation while enabling bounds checking.

## Files You May Edit
ctp2_code/ai/CityManagement/governor.h

## Acceptance Criteria (ALL must pass)
1. Build passes: `mise exec -- meson compile -C build ctp2_fast_tests`
2. Fast tests pass: `./build/ctp2_fast_tests`
3. Diff touches ONLY the files listed above
4. No new compiler warnings
5. Commit with conventional commit message

## Commit (REQUIRED)
```bash
git add ctp2_code/ai/CityManagement/governor.h
git commit -m "refactor(ai): replace C arrays with std::array"
```
