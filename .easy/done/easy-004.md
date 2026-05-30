# Easy Ticket: SCOUT-2-008

## Source
/Users/olek/projects/llm/games/ctp2/.easy/../.scouts/tickets/H-container--ctp2_code-ai-personality-aip2-h--01.md

## Change Required
DipAIP uses C-style arrays for event decay values

## Current State
`DipAIP` stores per-event-type decay values as raw C-style arrays:
- `float m_GoodEventDecay[REGARD_EVENT_ALL]`
- `float m_BadEventDecay[REGARD_EVENT_ALL]`

These should be `std::array<float, REGARD_EVENT_ALL>` or `std::vector<float>` for type safety, bounds checking, and modern C++ idioms.

## Code Evidence
```cpp
// File: ctp2_code/ai/personality/aip2.h
// Lines: 26-27
float m_GoodEventDecay[REGARD_EVENT_ALL];
float m_BadEventDecay[REGARD_EVENT_ALL];
```

## Suggested Direction
Replace the C-style arrays with `std::array<float, REGARD_EVENT_ALL>`. Update any code that iterates over or indexes into these arrays (the syntax is identical, but `.size()` and range-based for loops become available).

## Files You May Edit
ctp2_code/ai/personality/aip2.h

## Acceptance Criteria (ALL must pass)
1. Build passes: `mise exec -- meson compile -C build ctp2_fast_tests`
2. Fast tests pass: `./build/ctp2_fast_tests`
3. Diff touches ONLY the files listed above
4. No new compiler warnings
5. Commit with conventional commit message

## Commit (REQUIRED)
```bash
git add ctp2_code/ai/personality/aip2.h
git commit -m "refactor(ai): replace C arrays with std::array"
```
