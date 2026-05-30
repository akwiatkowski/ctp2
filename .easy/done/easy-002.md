# Easy Ticket: SCOUT-1-005

## Source
/Users/olek/projects/llm/games/ctp2/.easy/../.scouts/tickets/H-container--ctp2_code-ai-diplomacy-diplomattypes-cpp--01.md

## Change Required
C-style string arrays for enum name tables

## Current State
`diplomattypes.cpp` defines four global C-style arrays mapping enum values to
`std::string` names.  These arrays are sized by enum max values and are not bounds-checked.

## Code Evidence
```cpp
// File: ctp2_code/ai/diplomacy/diplomattypes.cpp
// Lines: 6-15
extern std::string s_regardEventNames[REGARD_EVENT_MAX] = { ... };

// Lines: 17-57
extern std::string s_proposalNames[PROPOSAL_MAX] = { ... };

// Lines: 59-67
extern std::string s_threatNames[THREAT_MAX] = { ... };

// Lines: 69-75
extern std::string s_responseNames[RESPONSE_MAX] = { ... };

// Lines: 77-102
extern std::string s_motivationNames[MOTIVATION_MAX] = { ... };
```

## Suggested Direction
Replace each array with `std::array<std::string, N>` or a `std::vector<std::string>`
initialized from an inline list.  Better yet, wrap each table in a small helper class
or function (e.g. `ProposalTypeToString(PROPOSAL_TYPE)`) so callers do not index into
a raw array at all.

## Files You May Edit
ctp2_code/ai/diplomacy/diplomattypes.cpp, ctp2_code/ai/diplomacy/diplomattypes.h

## Acceptance Criteria (ALL must pass)
1. Build passes: `mise exec -- meson compile -C build ctp2_fast_tests`
2. Fast tests pass: `./build/ctp2_fast_tests`
3. Diff touches ONLY the files listed above
4. No new compiler warnings
5. Commit with conventional commit message

## Commit (REQUIRED)
```bash
git add ctp2_code/ai/diplomacy/diplomattypes.cpp ctp2_code/ai/diplomacy/diplomattypes.h
git commit -m "refactor(ai): replace C arrays with std::array"
```
