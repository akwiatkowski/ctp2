# Easy Ticket: SCOUT-4-004

## Source
/Users/olek/projects/llm/games/ctp2/.easy/../.scouts/tickets/H-container--ctp2_code-build-generated-db-records--01.md

## Change Required
Generated DB records use C-style fixed-size arrays where std::array belongs

## Current State
Several generated record headers declare fixed-size C arrays with magic-number sizes. These should be `std::array` for type safety, bounds checking, and cleaner semantics.

## Code Evidence
```cpp
// File: ctp2_code/build/CultureRecord.h
// Lines: 117, 221
#define k_MAX_ObsoleteAdvance 5
sint32 m_ObsoleteAdvance[k_MAX_ObsoleteAdvance]; // Index into Advance database
```

```cpp
// File: ctp2_code/build/DiplomacyRecord.h
// Lines: 79, 221
#define k_MAX_Inherit 1
sint32 m_Inherit[k_MAX_Inherit]; // Index into Diplomacy database
```

```cpp
// File: ctp2_code/build/GovernmentRecord.h
// Lines: (similar pattern for m_ObsoleteAdvance)
```

These arrays are serialized with raw `sizeof()` and copied with `std::copy` without bounds checking:

```cpp
// File: ctp2_code/build/CultureRecord.cpp
// Line: 88
archive.Store((uint8*)&m_ObsoleteAdvance, sizeof(m_ObsoleteAdvance));

// Line: 398
std::copy(rval.m_ObsoleteAdvance, rval.m_ObsoleteAdvance + rval.m_numObsoleteAdvance, m_ObsoleteAdvance);
```

## Suggested Direction
Replace C arrays with `std::array<sint32, N>`. This provides:
- Fixed size encoded in the type
- Bounds-safe access via `.at()`
- Standard copy/assignment semantics
- Iterator support for `std::copy`

## Files You May Edit
ctp2_code/build/CultureRecord.h

## Acceptance Criteria (ALL must pass)
1. Build passes: `mise exec -- meson compile -C build ctp2_fast_tests`
2. Fast tests pass: `./build/ctp2_fast_tests`
3. Diff touches ONLY the files listed above
4. No new compiler warnings
5. Commit with conventional commit message

## Commit (REQUIRED)
```bash
git add ctp2_code/build/CultureRecord.h
git commit -m "refactor(build): replace C arrays with std::array"
```


---

## WORKER ATTEMPT FAILED

**Date:** 2026-05-28T17:18:19.932452
**Reason:** no changes made

### Diff Attempt

```diff

```

### Next Steps

This ticket needs manual review or a different approach.
