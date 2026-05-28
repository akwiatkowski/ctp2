# ctp2_headless --seed 0 Non-Determinism Investigation

## Reproduction

Two runs of `./build/ctp2_headless --new-game --turns 10 --players 4 --seed 0`
produce different per-player metrics (leader names, starting positions, city
counts, scores).  Seeds 42, 99, 2147483647, etc. are byte-identical across
runs.

The disabled test is in
`ctp2_code/test/cpp/test_headless_determinism.cpp:163-172`:

```cpp
TEST_CASE("Determinism: same seed produces identical metrics with seed 0"
          * doctest::skip(true))
```

---

## Code path

### 1. CLI parser

**Location:** `ctp2_code/ctp/headless_main.cpp:86-87`

```cpp
} else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
    seed = atoi(argv[++i]);
}
```

`seed` is a local `sint32` defaulting to `42` (line 74).

---

### 2. Seed propagation to the engine

**Location:** `ctp2_code/ctp/headless_main.cpp:162-170`

```cpp
// Wire --seed to the RNG.  gameinit_Initialize reads g_oldRandSeed
// and uses it as the seed for g_rand when non-zero; otherwise it
// falls back to GetTickCount().  Map generation, AI decisions, and
// combat all draw from g_rand, so this is the single knob that
// makes two runs deterministic.  Seed 0 keeps the legacy "use
// system time" semantic for users who want non-deterministic runs.
if (seed != 0) {
    g_oldRandSeed = seed;
}
```

When `--seed 0` is passed, the `if (seed != 0)` guard is **false**, so
`g_oldRandSeed` is **never written** and keeps its default value.

---

### 3. Default value of `g_oldRandSeed`

**Location:** `ctp2_code/ctp/civapp.cpp:410`

```cpp
sint32 g_oldRandSeed = FALSE;   // FALSE == 0
```

---

### 4. RNG initialisation in the game-init path

**Location:** `ctp2_code/gs/utility/gameinit.cpp:1445-1470`

The headless new-game path goes through `gameinit_Initialize(-1, -1, NULL)`
(called from `CivApp::InitializeGameHeadless` at `civapp.cpp:3430`).
Inside `gameinit_Initialize`:

```cpp
if (archive) {
    g_rand = new RandomGenerator(*archive);
} else {
#ifdef _DEBUG
    FILE * fin = fopen ("dbgseed.txt", "r");
    if (fin) {
        fscanf (fin, "%d", &seed);
    } else {
        seed = g_oldRandSeed ? g_oldRandSeed : GetTickCount();
        fin = fopen("logs\\oldseed.txt", "w");
        fprintf (fin, "%d\n", seed);
    }
    DPRINTF(k_DBG_FIX, ("** RANDOM SEED %d\n", seed));
    fclose (fin);
#else
    seed = g_oldRandSeed ? g_oldRandSeed : GetTickCount();
#endif
    srand(seed);
    g_rand = new RandomGenerator(seed);
}
```

Because `g_oldRandSeed == 0` when `--seed 0` was passed, the ternary
`g_oldRandSeed ? g_oldRandSeed : GetTickCount()` evaluates to
`GetTickCount()`, a millisecond-resolution system timer.  Two runs started
at different wall-clock times therefore get different seeds, and the
output diverges.

A second init site with the same pattern exists at
`gameinit.cpp:1069` (sprite-editor path):

```cpp
uint32 seed = g_oldRandSeed ? g_oldRandSeed : GetTickCount();
srand(seed);
g_rand = new RandomGenerator(seed);
```

---

### 5. The `RandomGenerator` class itself is *not* the problem

**Location:** `ctp2_code/gs/utility/RandGen.cpp:6-97`

`RandomGenerator::Initialize(sint32 seed)` uses a deterministic
subtract-with-borrow algorithm (Numerical Recipes `ran3`).  The unit
test `test_randgen.cpp:72-82` explicitly verifies that seed `0` produces
an identical sequence:

```cpp
TEST_CASE("RandomGenerator with seed 0 is deterministic")
{
    RandomGenerator r1(static_cast<sint32>(0));
    RandomGenerator r2(static_cast<sint32>(0));
    CHECK(r1.Next() == r2.Next());
    CHECK(r1.Next() == r2.Next());
    CHECK(r1.Next() == r2.Next());
}
```

The RNG correctly handles `0`; the non-determinism is introduced **before**
the RNG is constructed.

---

### 6. Additional `time(0)` re-seed (scenarios only, not on the `--new-game` path)

**Location:** `civapp.cpp:1956-1963`

```cpp
if(g_isScenario && !g_oldRandSeed) {
    g_rand->Initialize(static_cast<sint32>(time(0)));
}
```

This is a second, independent time-based re-seed that only fires for
scenarios when `g_oldRandSeed` is zero.  It does not affect the
`--new-game` path investigated here, but it confirms the pattern:
"zero means random" is baked into multiple places.

---

## Special case for seed == 0

- **Found at:** `ctp2_code/gs/utility/gameinit.cpp:1455` (debug) and
  `1465` (release)
- **Behaviour:** `g_oldRandSeed` is used as a boolean sentinel.
  When it is `0` (the default, or when `--seed 0` is passed and
  `headless_main.cpp:168` skips the assignment), the engine falls
  back to `GetTickCount()`, making every run non-deterministic.

The sentinel logic is intentional — it allows the original interactive
client to start a game without specifying a seed and still get a
"random" experience.  In the headless binary the same logic is
preserved, but the `--seed 0` CLI argument is then silently ignored
instead of being treated as a valid deterministic seed.

---

## Fix options

1. **Treat `0` as a valid deterministic seed in headless mode.**
   Remove the `if (seed != 0)` guard in `headless_main.cpp:168` so
   that `g_oldRandSeed = seed` is always executed.  Change the
   ternary in `gameinit.cpp` from:
   ```cpp
   seed = g_oldRandSeed ? g_oldRandSeed : GetTickCount();
   ```
   to a sentinel that cannot collide with a legitimate seed, e.g.:
   ```cpp
   seed = (g_oldRandSeed != 0) ? g_oldRandSeed : GetTickCount();
   ```
   is already what it does — the real change is ensuring headless
   **always** writes `g_oldRandSeed`, even when the value is `0`.
   This would require replacing the sentinel with something else
   (e.g. a separate `bool g_randSeedSet` flag) so the engine can
   distinguish "user asked for seed 0" from "user did not specify a
   seed".

2. **Document the current behaviour in `--help` and skip the determinism
   test permanently.**
   Update `headless_main.cpp:55` from:
   ```
   --seed N                Pin RNG seed for determinism
   ```
   to:
   ```
   --seed N                Pin RNG seed for determinism (0 = random)
   ```
   This keeps the existing interactive-game semantics intact and
   makes the CLI contract explicit.  The disabled test can then be
   removed or converted into a test that asserts seed-0 runs are
   *different* from each other.

---

## Recommendation

**Option 1** (treat `0` as valid) is preferable for headless/scripting
use cases where deterministic replay is the primary value proposition.
However, it requires a non-trivial refactor of the sentinel logic
because `g_oldRandSeed` is an `extern` shared by the UI and headless
builds, and its "zero means random" contract is relied upon by the
interactive new-game flow.

If a quick minimal fix is needed, **Option 2** (document `0 = random`)
is zero-risk and immediately unblocks removing the confusing disabled
test.  The long-term clean fix is to introduce a `bool g_randSeedSet`
flag so `0` can be an honest seed while preserving the
"unspecified = random" default.
