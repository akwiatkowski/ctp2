#include "gs/core/game.h"

#include "gs/gameobj/ArmyPool.h"
#include "gs/gameobj/CityPool.h"
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

void Game::NewGame() {
    // TODO: move initialization from gameinit.cpp here
}

void Game::LoadGame(CivArchive& /*archive*/) {
    // TODO: move load logic from gameinit.cpp here
}

void Game::SaveGame(CivArchive& /*archive*/) {
    // TODO: move save logic here
}

void Game::Cleanup() {
    m_events.reset();
    m_slic.reset();
    m_cityPool.reset();
    m_armyPool.reset();
    m_unitPool.reset();
    m_players.clear();
    m_rand.reset();
    m_turn.reset();
    m_world.reset();
}

} // namespace Ctp2
