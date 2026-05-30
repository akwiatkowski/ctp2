// test/cpp/scoped_rand.h
//
// RAII fixture that installs a local RandomGenerator as the process-wide
// g_rand for the duration of a scope, then restores the previous pointer.
//
// Why this exists: gs/ code uses g_rand directly (via civrand() or the raw
// pointer). Tests that exercise randomness need a known seed AND a way to
// guarantee no other test contaminates global state. The previous pattern
// was `g_rand = new RandomGenerator(seed);` with a manual nullptr on
// teardown — leaks one generator per fixture and silently breaks any test
// that forgets to restore g_rand.
//
// Usage:
//
//     TEST_CASE("foo behaves deterministically") {
//         ScopedRand r(12345);          // g_rand now points at a seeded RNG
//         // ... exercise code that calls civrand() or g_rand-> ...
//     }                                  // g_rand restored to prior value
//
// Test-only. Lives in test/cpp/ and is never compiled into game binaries.

#ifndef CTP2_TEST_SCOPED_RAND_H
#define CTP2_TEST_SCOPED_RAND_H

#include "ctp/c3.h"                 // sint32
#include "gs/utility/RandGen.h"     // RandomGenerator, g_rand

class ScopedRand
{
public:
    explicit ScopedRand(sint32 seed)
        : m_local(seed)
        , m_saved(g_rand)
    {
        g_rand = &m_local;
    }

    ~ScopedRand()
    {
        g_rand = m_saved;
    }

    // Non-copyable, non-movable — the RNG state is positional and copying
    // would silently double-install the same buffer.
    ScopedRand(ScopedRand const &)             = delete;
    ScopedRand & operator=(ScopedRand const &) = delete;
    ScopedRand(ScopedRand &&)                  = delete;
    ScopedRand & operator=(ScopedRand &&)      = delete;

    RandomGenerator & get() { return m_local; }

private:
    RandomGenerator   m_local;
    RandomGenerator * m_saved;
};

#endif  // CTP2_TEST_SCOPED_RAND_H
