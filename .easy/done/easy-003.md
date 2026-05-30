# Easy Ticket: SCOUT-1-008

## Source
/Users/olek/projects/llm/games/ctp2/.easy/../.scouts/tickets/H-container--ctp2_code-ai-diplomacy-foreigner-cpp--01.md

## Change Required
C-style arrays in Foreigner and raw 1024-byte buffer in Load()

## Current State
`Foreigner` uses C-style arrays for `m_regard` and `m_regardEventList`.  In addition,
`Load()` allocates a 1024-byte raw buffer on the stack for string loading, which is
both a potential stack-pressure issue and a vector for buffer overruns (the code
asserts `buf_size < 256` but does not truncate).

## Code Evidence
```cpp
// File: ctp2_code/ai/diplomacy/Foreigner.h
// Lines: 211-217
    ai::Regard m_regard[REGARD_EVENT_ALL];
    ai::Regard m_regardTotal;
    StringId m_bestRegardExplain;
    ai::Regard m_effectiveRegardModifier;
    ai::Regard m_trustworthiness;
    RegardEventList m_regardEventList[REGARD_EVENT_ALL];
```

```cpp
// File: ctp2_code/ai/diplomacy/foreigner.cpp
// Lines: 111
    uint8 name_str [1024];
    // ...
    archive.Load((uint8 *) &name_str[0], buf_size);
    g_theStringDB->GetStringID((char*)name_str, event.explainStrId);
```

## Suggested Direction
Replace `m_regard` and `m_regardEventList` with `std::array`.  In `Load()`, use a
`std::vector<uint8>` or a fixed `std::array<uint8, 256>` sized to the actual maximum
(`buf_size < 256`).

## Files You May Edit
ctp2_code/ai/diplomacy/foreigner.cpp, ctp2_code/ai/diplomacy/foreigner.h

## Acceptance Criteria (ALL must pass)
1. Build passes: `mise exec -- meson compile -C build ctp2_fast_tests`
2. Fast tests pass: `./build/ctp2_fast_tests`
3. Diff touches ONLY the files listed above
4. No new compiler warnings
5. Commit with conventional commit message

## Commit (REQUIRED)
```bash
git add ctp2_code/ai/diplomacy/foreigner.cpp ctp2_code/ai/diplomacy/foreigner.h
git commit -m "refactor(ai): replace C arrays with std::array"
```
