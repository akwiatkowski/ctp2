// test/cpp/test_game.cpp
// Smoke tests for Ctp2::Game.
//
// The full Ctp2::Game::NewGame() can't run in isolation yet — TurnCount's
// ctor reaches into g_theProfileDB and gamesettings_Get() which aren't
// initialised in unit-test context.  Real construction will become testable
// once those upstream globals are also threaded explicitly, or once we have
// a doubles harness that mocks them.
//
// For now we exercise the parts that don't reach into globals:
//  - default-construction
//  - Cleanup() on an empty Game (no allocated subsystems)
//  - non-copyability / movability of the class
//
// Future tests will replace these stubs with real lifecycle checks
// once Game::NewGame becomes self-sufficient.

#include "ctp/c3.h"
#include "doctest.h"
#include "gs/core/game.h"

TEST_CASE("Ctp2::Game can be default-constructed and destroyed") {
    Ctp2::Game game;
    // No allocated subsystems yet; destruction must be safe.
}

TEST_CASE("Ctp2::Game::Cleanup is safe on an unpopulated instance") {
    Ctp2::Game game;
    game.Cleanup();  // every unique_ptr is null; nothing to do
    game.Cleanup();  // idempotent
}

TEST_CASE("Ctp2::Game is non-copyable and movable") {
    // Compile-time-only check via type traits.
    static_assert(!std::is_copy_constructible<Ctp2::Game>::value,
                  "Game must not be copy-constructible");
    static_assert(!std::is_copy_assignable<Ctp2::Game>::value,
                  "Game must not be copy-assignable");
    static_assert(std::is_move_constructible<Ctp2::Game>::value,
                  "Game must be move-constructible");
    static_assert(std::is_move_assignable<Ctp2::Game>::value,
                  "Game must be move-assignable");

    Ctp2::Game a;
    Ctp2::Game b = std::move(a);
    (void)b;
}
