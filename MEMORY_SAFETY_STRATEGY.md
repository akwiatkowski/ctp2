# Memory Safety Strategy for CTP2

## The Hard Truth

There is no "once and for all" fix for a 300K+ LOC C++ codebase from 1999 with manual memory management throughout. A full rewrite would take years and introduce more bugs than it fixes.

**But** there is a systematic, proven approach that can reduce memory errors by 90%+ within weeks of focused effort. It combines runtime detection, targeted refactoring, and test-driven validation.

---

## The Three-Layer Defense

### Layer 1: Build-Time Detectors (Immediate, Low Effort)

These catch bugs at runtime during development and CI. They add ~2x memory overhead and ~20% CPU slowdown, so they're debug-only.

| Tool | What it catches | Build flag |
|------|----------------|------------|
| **AddressSanitizer** (ASan) | Use-after-free, buffer overflow, heap buffer overflow, stack buffer overflow | `-fsanitize=address` |
| **UndefinedBehaviorSanitizer** (UBSan) | Signed overflow, null dereference, misaligned access, out-of-bounds array index | `-fsanitize=undefined` |
| **LeakSanitizer** (LSan) | Memory leaks | Built into ASan |
| **ThreadSanitizer** (TSan) | Data races | `-fsanitize=thread` |

**For CTP2 specifically:**

```bash
# meson build type: debug with sanitizers
meson setup build --buildtype=debug -Db_sanitize=address,undefined
```

**What we'd find immediately:**
- The `UNITACTION` underflow (UBSan catches negative array index)
- The `UnitData::GetDBRec(this=NULL)` crash (ASan catches null dereference)
- Sprite RLE buffer overruns (ASan catches heap buffer overflow)
- Any use-after-free in the event queue (ASan catches freed memory access)

**Why this is the highest-impact single change:** Most of our crashes are deterministic once the code path is exercised. ASan turns silent corruption into immediate, actionable crashes with full stack traces.

### Layer 2: Targeted Refactoring (Medium Effort, High Reward)

Instead of rewriting everything, we systematically eliminate the **categories** of bugs we're seeing.

#### Category A: Array Underflow/Overflow

**Pattern:** Enums with negative values used as array indices.

**Fix:** Every array access through an enum gets a bounds-checked wrapper.

```cpp
// BEFORE (crash-prone)
sint32 pos = m_holdingCurAnimPos[action];  // action can be -4

// AFTER (safe)
sint32 GetHoldingCurAnimPos(UNITACTION action) const {
    if (action < 0 || action >= UNITACTION_MAX) {
        fprintf(stderr, "[SAFETY] Invalid UNITACTION %d\n", action);
        return 0;
    }
    return m_holdingCurAnimPos[action];
}
```

**Audit targets:**
- `UNITACTION` arrays in `UnitActor`
- `ORDER_TYPE` arrays in `ArmyData`
- `GOVERNMENT_TYPE` arrays in `Player`
- Any array indexed by a database enum

#### Category B: Null `UnitData*`

**Pattern:** `Unit` object wraps a null `UnitData*` because the unit was destroyed or never properly initialized.

**Fix:** Add a `Validate()` check at the start of any function that dereferences `UnitData*`.

```cpp
// BEFORE
cost = rec->GetShieldCost();  // rec could be null

// AFTER
if (!rec) {
    fprintf(stderr, "[SAFETY] Null UnitRecord in BuildUnit\n");
    return false;
}
cost = rec->GetShieldCost();
```

**Audit targets:**
- `UnitData::GetDBRec()`
- `Unit::GetArmy()` when used without `IsValid()` check
- All `GetData()` calls on `Unit`
- City-to-unit casts where the city data is accessed as unit data

#### Category C: Use-After-Free in Event Queue

**Pattern:** `GEV_Settle` queues `GEV_KillUnit`, but the unit is referenced again later in the same turn.

**Fix:** Use `ID` handles instead of raw pointers for all event arguments. The `ID` class already has reference counting — we just need to ensure it's actually used.

#### Category D: C Variadics with C++ Objects

**Pattern:** `GameEventManager::AddEvent(...)` is a C variadic function that takes `Unit`, `Army`, `City` objects. Non-trivially-copyable types cause `EXC_BREAKPOINT` on ARM64.

**Fix:** Already done — we made `ID` trivially copyable with `= default`. But we should audit all other types passed through variadics.

#### Category E: Raw new/delete

**Pattern:** `new UnitData`, `new ArmyData`, `delete` called manually.

**Fix:** For the most-allocated types, provide factory functions that return `std::unique_ptr`:

```cpp
// BEFORE
UnitData* data = new UnitData(args);
// ... later ...
delete data;

// AFTER
auto data = std::make_unique<UnitData>(args);
// Automatically freed when unique_ptr goes out of scope
```

**Note:** This is the slowest layer because it requires understanding ownership semantics across the entire call graph. Start with leaf types ( `UnitData`, `CityData`) and work upward.

### Layer 3: Smoke Test as Coverage Driver (Ongoing)

The smoke test is our **code coverage tool**. Every new command exercises new code paths. Every crash tells us exactly which subsystem needs auditing.

**Strategy:**
1. Add a smoke test command for each major subsystem
2. Run with ASan+UBSan enabled
3. Fix every crash that ASan reports
4. Repeat until the full scenario runs clean

**Subsystems to cover:**
- [x] Game init, new game, start game
- [x] City building, production queue
- [x] Turn processing, end turn
- [x] Unit movement
- [ ] Combat (attack, defend)
- [ ] Diplomacy (make contact, trade, war)
- [ ] Trade routes
- [ ] Pollution / global warming
- [ ] Wonders
- [ ] Space colonization
- [ ] Victory conditions

---

## The "No Silver Bullet" Honest Assessment

| Approach | Feasibility | Time | Effectiveness |
|----------|------------|------|---------------|
| Full rewrite in Rust | Impossible | Years | 100% memory-safe |
| Incremental smart pointer migration | Hard | Months | ~60% reduction |
| ASan + UBSan in CI | Easy | Days | ~80% reduction |
| Bounds-check all array access | Medium | Weeks | ~40% reduction |
| Null-check all pointer derefs | Medium | Weeks | ~30% reduction |
| Custom allocator with guard pages | Hard | Weeks | ~20% reduction |
| Static analysis (Clang, PVS-Studio) | Easy | Days | ~15% reduction |

**Recommended combination for maximum impact:**
1. **ASan + UBSan in CI** (immediate, catches most bugs)
2. **Bounds-check all enum-indexed arrays** (prevents the crashes we're seeing)
3. **Smoke test coverage expansion** (exercises more code paths)
4. **Incremental smart pointer migration** (prevents new leaks)

---

## Implementation Plan

### Week 1: Enable Sanitizers
- Add `-fsanitize=address,undefined` to meson debug builds
- Fix any build errors sanitizer headers cause
- Run the smoke test — expect ~5-10 new crashes
- Fix the ones that reproduce

### Week 2: Enum Array Audit
- Write a script that finds all array accesses indexed by enums
- Add bounds checks to the top 20 most dangerous ones
- Focus on `UNITACTION`, `ORDER_TYPE`, `GOVERNMENT_TYPE`

### Week 3: Null Pointer Audit
- Find all `GetDBRec()`, `GetData()`, `AccessData()` calls
- Add null checks where missing
- Run smoke test with governor enabled (the crash we found)

### Week 4+: Smoke Test Expansion
- Add combat scenarios
- Add diplomacy scenarios
- Add late-game scenarios (space, wonders)
- Each new scenario will find new crashes — fix them

### Month 2+: Smart Pointer Migration
- Start with `UnitData` allocation in `UnitPool`
- Move to `ArmyData`, `CityData`
- Only refactor code that's already well-covered by smoke tests

---

## Why This Works

The key insight is **feedback loops**:

1. **Sanitizers give fast feedback** — run the game, see the crash, fix it. No guessing.
2. **Smoke tests give coverage feedback** — every new command exercises code that was previously untested.
3. **Bounds checks give defensive feedback** — even if we miss a bug, the check prevents corruption.

Over time, the combination shrinks the "unsafe surface area" of the codebase until the remaining bugs are in rarely-exercised code paths.

We won't get to 100% memory safety without a rewrite. But we can get to **"crashes are rare and reproducible"** within a month — which is the difference between an unplayable port and a stable game.

---

## Quick Wins (Do These First)

1. **Enable ASan in meson debug builds** — 1 day, catches 80% of crashes
2. **Bounds-check UNITACTION accessors** — already done, apply same pattern to other enums
3. **Add `Assert(unit.GetData())` before all `GetDBRec()` calls** — 1 day, prevents null dereference
4. **Add `advance_turns 10` to smoke test** — exercises deep game state, finds late-game bugs
5. **Run smoke test with governor enabled under ASan** — find and fix the `UnitData` null crash
