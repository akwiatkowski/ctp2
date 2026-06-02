#include "ctp/c3.h"
#include "gs/core/game.h"

#include "gs/gameobj/ArmyPool.h"
// CityPool: forward-declared in game.h as a future-tense placeholder; no
// concrete class exists today (cities are owned per-player, not in a
// dedicated pool). Drop the include until the class is introduced.
#include "gs/gameobj/Player.h"
#include "gs/gameobj/pollution.h"
#include "gs/gameobj/TopTen.h"
#include "gs/gameobj/UnitPool.h"
#include "gs/utility/RandGen.h"
#include "gs/utility/TurnCnt.h"
#include "gs/world/World.h"
#include "gs/slic/SlicEngine.h"
#include "gs/events/GameEventManager.h"

namespace Ctp2 {

Game::Game() = default;

Game::~Game() = default;

Game::Game(Game&&) noexcept = default;
Game& Game::operator=(Game&&) noexcept = default;

void Game::NewGame(sint32 numPlayers, sint32 initialYear, sint32 randSeed) {
    // Subsystems with clean (globals-free) ctors get created here.
    // They coexist with the legacy gameinit-allocated globals during
    // the transition; callers will migrate to Game's accessors as the
    // long-running globals refactor progresses.
    m_turn      = std::make_unique<TurnCount>(numPlayers, initialYear);
    m_rand      = std::make_unique<RandomGenerator>(randSeed);
    m_pollution = std::make_unique<Pollution>();
    m_topten    = std::make_unique<TopTen>();
    m_unitPool  = std::make_unique<UnitPool>();
    m_armyPool  = std::make_unique<ArmyPool>();

    // SlicEngine and GameEventManager need more orchestration to spin up
    // (event registration, SLIC file loading) — they stay legacy-owned
    // for now and migrate in a later step.
}

void Game::LoadGame(CivArchive& archive) {
    m_turn = std::make_unique<TurnCount>(archive);
}

void Game::SaveGame(CivArchive& /*archive*/) {
    // TODO: move save logic here
}

void Game::Cleanup() {
    // Reverse-dependency-order destruction.  Trackers / pools depend on
    // World and Players; those depend on TurnCount/RNG; RNG and TurnCount
    // have no dependencies, so they go last.
    m_events.reset();
    m_slic.reset();
    m_topten.reset();
    m_pollution.reset();
    m_armyPool.reset();
    m_unitPool.reset();
    m_players.clear();
    m_world.reset();
    m_rand.reset();
    m_turn.reset();
}

} // namespace Ctp2
