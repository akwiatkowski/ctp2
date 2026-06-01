#include "ctp/c3.h"
#include "gs/core/game.h"

#include "gs/gameobj/ArmyPool.h"
// CityPool: forward-declared in game.h as a future-tense placeholder; no
// concrete class exists today (cities are owned per-player, not in a
// dedicated pool). Drop the include until the class is introduced.
#include "gs/gameobj/Player.h"
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

void Game::NewGame(sint32 numPlayers, sint32 initialYear) {
    m_turn = std::make_unique<TurnCount>(numPlayers, initialYear);
}

void Game::LoadGame(CivArchive& archive) {
    m_turn = std::make_unique<TurnCount>(archive);
}

void Game::SaveGame(CivArchive& /*archive*/) {
    // TODO: move save logic here
}

void Game::Cleanup() {
    m_events.reset();
    m_slic.reset();
    m_armyPool.reset();
    m_unitPool.reset();
    m_players.clear();
    m_rand.reset();
    m_turn.reset();
    m_world.reset();
}

} // namespace Ctp2
