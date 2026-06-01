// test/cpp/test_game.cpp
// Smoke tests for Ctp2::Game.
//
// Now that TurnCount's ctor takes its inputs explicitly (numPlayers,
// initialYear), Game::NewGame can run end-to-end in unit-test context
// without reaching into g_theProfileDB or gamesettings_Get.

#include "ctp/c3.h"
#include "doctest.h"
#include "gs/core/game.h"
#include "gs/utility/TurnCnt.h"

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

TEST_CASE("Ctp2::Game::NewGame creates a TurnCount with the given setup") {
    // NewGame now takes (numPlayers, initialYear) explicitly — the inputs
    // that TurnCount needs.  No globals required.
    Ctp2::Game game;
    game.NewGame(4, -4000);

    // m_turn is the per-round per-player counter, initialised to 0 by
    // Init(); GetTurn() returns it directly without reaching into
    // NewTurnCount/g_player, so it's safe to observe in isolation.
    // (GetRound()/GetYear() route through NewTurnCount which queries
    // g_player and aborts in test context.  Verifying actual round/year
    // values from the static API will become possible once g_player
    // ownership is also threaded out of NewTurnCount.)
    CHECK(game.GetTurn().GetTurn() == 0);
}

TEST_CASE("Two Ctp2::Game instances coexist") {
    Ctp2::Game gameA;
    Ctp2::Game gameB;
    gameA.NewGame(4, -4000);
    gameB.NewGame(2, 1500);

    // Both Games own independent TurnCount instances; mutating one
    // doesn't blow up the other.
    CHECK(gameA.GetTurn().GetTurn() == 0);
    CHECK(gameB.GetTurn().GetTurn() == 0);
}

TEST_CASE("Ctp2::Game::Cleanup releases the TurnCount and allows re-init") {
    Ctp2::Game game;
    game.NewGame(2, 0);
    game.Cleanup();
    game.NewGame(2, 0);  // must not crash; cleanup released previous TurnCount
    CHECK(game.GetTurn().GetTurn() == 0);
}
