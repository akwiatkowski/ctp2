// test/cpp/scoped_rand.h
//
// RAII fixture that installs a local RandomGenerator as the process-wide
// rand_ptr() for the duration of a scope, then restores the previous pointer.
//
// Why this exists: gs/ code uses rand_ptr() directly (via civrand() or the raw
// pointer). Tests that exercise randomness need a known seed AND a way to
// guarantee no other test contaminates global state. The previous pattern
// was `rand_ptr_Set(new RandomGenerator(seed));` with a manual nullptr on
// teardown — leaks one generator per fixture and silently breaks any test
// that forgets to restore rand_ptr().
//
// Usage:
//
//     TEST_CASE("foo behaves deterministically") {
//         ScopedRand r(12345);          // rand_ptr() now points at a seeded RNG
//         // ... exercise code that calls civrand() or rand_ptr()-> ...
//     }                                  // rand_ptr() restored to prior value
//
// Test-only. Lives in test/cpp/ and is never compiled into game binaries.

#ifndef CTP2_TEST_SCOPED_RAND_H
#define CTP2_TEST_SCOPED_RAND_H

#include "ctp/c3.h"                 // sint32
#include "gs/utility/RandGen.h"     // RandomGenerator, rand_ptr()
#include "ctp/civapp.h"             // CivApp (trampoline host)

// After the rand_ptr trampoline migration, rand_ptr_Set adopts ownership
// into the active Game's m_rand.  ScopedRand can no longer stack-allocate
// the RNG and Set its address — that would let the trampoline's
// unique_ptr::reset try to delete a stack object.
//
// Instead, ScopedRand owns a process-local CivApp on the stack; its
// eager-constructed Game container hosts the seed RNG via the
// trampoline.  civapp_Get() is swapped to point at this local app for
// the scope's duration so rand_ptr() routes correctly.
class ScopedRand
{
public:
    explicit ScopedRand(sint32 seed)
        : m_savedApp(civapp_Get())
    {
        civapp_Set(&m_app);
        rand_ptr_Set(new RandomGenerator(seed));  // Game adopts ownership
    }

    ~ScopedRand()
    {
        rand_ptr_Set(nullptr);   // Game::m_rand.reset(null) deletes the seed RNG
        civapp_Set(m_savedApp);
    }

    ScopedRand(ScopedRand const &)             = delete;
    ScopedRand & operator=(ScopedRand const &) = delete;
    ScopedRand(ScopedRand &&)                  = delete;
    ScopedRand & operator=(ScopedRand &&)      = delete;

    RandomGenerator & get() { return *rand_ptr(); }

private:
    CivApp     m_app;        // local trampoline host
    CivApp *   m_savedApp;
};

#endif  // CTP2_TEST_SCOPED_RAND_H
