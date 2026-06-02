#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __RAND_GEN_H__
#define __RAND_GEN_H__ 1

#include <nlohmann/json.hpp>

class CivArchive;

#define k_RAND_MBIG 1000000000
#define k_RAND_MSEED 161803398
#define k_RAND_MZ 0
#define k_RAND_FAC (1.0/double(k_RAND_MBIG))

class RandomGenerator;

// Storage is file-scope `static` in gs/utility/gameinit.cpp.  Most
// callers go through civrand() (defined below) which dereferences to a
// reference.  Lifecycle code (gameinit, net_rand, c3rand, the JSON
// save/load bridges) needs the raw pointer; rand_ptr() returns it and
// rand_ptr_Set() lets gameinit reseat the pointer on new-game / load.
RandomGenerator * rand_ptr(void);
void              rand_ptr_Set(RandomGenerator *p);

class RandomGenerator

{

    sint32 m_start_seed;
	sint32 m_buffer[56];
    sint32 *m_firstp, *m_secondp, *m_endp;
	sint32 m_callCount;

	friend class NetRand;
	// JSON savegame bridges (json_save.cpp).  Friends so the private
	// fields can be serialised without expanding the public API just
	// for serialisation.
	friend void to_json(nlohmann::json &j, RandomGenerator const &rng);
	friend void from_json(nlohmann::json const &j, RandomGenerator &rng);

public:
    RandomGenerator(sint32 seed);
    RandomGenerator(CivArchive &archive);
	RandomGenerator(RandomGenerator &copyme);
    void Initialize(sint32 seed);

	sint32 CallCount() { return m_callCount; }

    sint32 Next();
    sint32 Next(sint32 r)
	{
		Assert(0 <r);

		if(r <= 0)
			return 0;

#ifdef LOG_RAND
		if(this == rand_ptr()) {
			sint32 res = Next() % r;
			DPRINTF(k_DBG_GAMESTATE, ("RandomGenerator::Next(%d) = %d\n",
									  r, res));
			return res;
		}
#endif
		return Next() % r;
	}

    double NextF() { return Next() * k_RAND_FAC; }
    double NextB()
    {   sint32 val = Next();
        return 0x00000001 & (val ^ (val >> 5) ^ (val >> 13) ^ (val >> 19));
    }

    sint32 GetSeed() const { return m_start_seed; }
    void Serialize (CivArchive &archive);
};

// (g_rand extern declared at the top of this header, before the class def.)

// Reference accessor for the active random generator.
//
// Why this exists: g_rand is a raw pointer touched by ~215 call sites across
// 32 files. Tests cannot swap it cleanly today — every fixture either leaks
// it or hand-rolls a swap. Routing reads through civrand() gives us a single
// pluggable seam without renaming the global (NetRand has friend access to
// g_rand and the network layer assumes the pointer name).
//
// Migration is opt-in per file: new code uses civrand(), old code keeps
// g_rand-> until it is touched for other reasons. ScopedRand (test-only)
// swaps the pointer for the duration of a test, so civrand() returns the
// fixture's generator inside the scope.
//
// Not named rand() because that collides with libc's <cstdlib> rand().
inline RandomGenerator & civrand() { return *rand_ptr(); }

#else

class RandomGenerator;

#endif
