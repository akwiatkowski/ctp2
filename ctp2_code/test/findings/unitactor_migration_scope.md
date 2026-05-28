# Advances.cpp UnitActor Migration Scope

## All references

| Line | Code | Category | Approach |
|-----:|------|----------|----------|
| 59   | `#include "gfx/spritesys/UnitActor.h"` | Include | Drop once A+B are resolved |
| 226  | `UnitActorPtr cityActor = city.GetActor();` | Reference | A — forward-decl friendly |
| 228  | `cityActor->ChangeImage(city.GetSpriteState(), city.GetType(), city);` | MethodCall | B — render_observer-friendly |

## Detailed analysis

### Line 226: `UnitActorPtr cityActor = city.GetActor();`

- **What it does:** Obtains the city's `UnitActor` pointer (a `std::shared_ptr<UnitActor>`) so the next line can call a method on it.
- **Why it's Category A:** This file never dereferences `cityActor` for data access, never stores it in a member variable, and never returns it. It is only used as the `this` argument for a single method call on the very next line.
- **Forward-decl path:** `Unit.h` (already included at line 61) already forward-declares `class UnitActor;` at line 79 and uses `std::shared_ptr<UnitActor>` for `GetActor()` at line 370. The calling code can therefore switch from the typedef `UnitActorPtr` to the explicit `std::shared_ptr<UnitActor>` without needing the full `UnitActor.h` definition.

### Line 228: `cityActor->ChangeImage(city.GetSpriteState(), city.GetType(), city);`

- **What it does:** Synchronously tells the city's actor to re-skin itself (called when the player's age changes in `UpdateCitySprites`).
- **Why it's Category B:** It is a pure fire-and-forget void method. No data is returned to `Advances.cpp`, no conditional logic in `Advances.cpp` depends on the outcome, and the actor's internal state mutation is entirely opaque to the game-state layer.
- **Why it is NOT `render_observer::AddMorphUnit`:** The existing `AddMorphUnit` queues a deferred `DQActionMorph` that eventually calls `UnitActor::ChangeType` (asynchronous, Director-queue driven). `ChangeImage` is a separate, synchronous entry point that immediately dumps pending actions and reloads sprites. They are semantically different and NOT interchangeable.
- **Render_observer path:** Add a new `ChangeUnitImage` virtual to `render_observer::Impl` and a corresponding free function. The `DirectorRenderObserver` adapter forwards to `actor->ChangeImage(...)` synchronously (or no-ops when `actor` is null). This mirrors the pattern already used for `AddSetOwner`, `AddSetVisibility`, etc.

## Summary by category

- **Category A (forward-decl):** 1 hit — line 226
- **Category B (render_observer):** 1 hit — line 228
- **Category C (needs design):** 0 hits

## Recommended migration approach

This is an **A+B mixed** scenario — no blocking Category C references.

### Phase 1: Orchestrator extends `render_observer` (one PR)

1. **Add the new virtual to `render_observer::Impl`** (`gs/core/render_observer.h`):
   ```cpp
   virtual void ChangeUnitImage(std::shared_ptr<UnitActor> actor,
                                SpriteStatePtr ss, sint32 type, Unit id) = 0;
   ```
2. **Add the free-function fan-out** in `render_observer.h` and `render_observer.cpp`:
   ```cpp
   void ChangeUnitImage(std::shared_ptr<UnitActor> actor,
                        SpriteStatePtr ss, sint32 type, Unit id);
   ```
3. **Implement in `DirectorRenderObserver`** (`gfx/spritesys/director_render_observer.cpp`):
   ```cpp
   void DirectorRenderObserver::ChangeUnitImage(std::shared_ptr<UnitActor> actor,
                                                SpriteStatePtr ss, sint32 type, Unit id) {
       if (actor) actor->ChangeImage(ss, type, id);
   }
   ```
4. **Update test stubs** (`test/cpp/test_render_observer.cpp`) to implement the new virtual (can be a no-op or record-and-replay spy, matching existing stub style).
5. **Verify** that both UI and headless builds compile.

### Phase 2: Worker removes the include from `Advances.cpp` (follow-up PR)

1. Replace `#include "gfx/spritesys/UnitActor.h"` with:
   ```cpp
   class UnitActor;   // already forward-declared via Unit.h, but explicit is fine
   ```
   (No extra header needed for the forward decl; `Unit.h` already brings it in.)
2. On line 226, change `UnitActorPtr` to `std::shared_ptr<UnitActor>` (matching `Unit.h`'s signature style).
3. On line 228, replace the direct call with:
   ```cpp
   render_observer::ChangeUnitImage(cityActor,
                                    city.GetSpriteState(),
                                    city.GetType(),
                                    city);
   ```
4. **Compile check**: `mise exec -- meson compile -C build` must be clean.

## If pure-A: a worker can do it

Not applicable — there is a Category B method call, so `render_observer` must be extended first.

## If A+B mixed: orchestrator extends render_observer first

See Phase 1 above. The brief for the orchestrator PR is:
- Add one virtual + one free function + one adapter implementation + one test stub update.
- No changes to `Advances.cpp` in this PR.

## If C present: blocked

Not applicable — zero Category C hits. The migration is **unblocked**.

## Out of scope for this scope doc

- Do not confuse `ChangeUnitImage` with `AddMorphUnit`; they target different `UnitActor` methods (`ChangeImage` vs `ChangeType`) and have different execution models (synchronous immediate vs deferred Director queue).
- Do not attempt to move `UpdateCitySprites` itself out of `Advances.cpp`; only the `UnitActor.h` include is being scoped.
