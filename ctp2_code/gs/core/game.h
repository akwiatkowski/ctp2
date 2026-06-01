#pragma once

#include <memory>
#include <vector>

#include "os/include/ctp2_inttypes.h"

// Forward declarations — no heavy includes in header
class World;
class TurnCount;
class RandomGenerator;
class Player;
class UnitPool;
class ArmyPool;
class CityPool;
class SlicEngine;
class GameEventManager;
class CivArchive;

namespace Ctp2 {

class Game {
public:
    Game();
    ~Game();

    // Non-copyable
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    // Movable
    Game(Game&&) noexcept;
    Game& operator=(Game&&) noexcept;

    // Lifecycle
    void NewGame();
    void LoadGame(CivArchive& archive);
    void SaveGame(CivArchive& archive);
    void Cleanup();

    // Accessors
    TurnCount& GetTurn() { return *m_turn; }
    const TurnCount& GetTurn() const { return *m_turn; }

    World& GetWorld() { return *m_world; }
    const World& GetWorld() const { return *m_world; }

    RandomGenerator& GetRand() { return *m_rand; }

    Player* GetPlayer(sint32 idx) { return (idx >= 0 && idx < static_cast<sint32>(m_players.size())) ? m_players[idx].get() : nullptr; }
    const Player* GetPlayer(sint32 idx) const { return (idx >= 0 && idx < static_cast<sint32>(m_players.size())) ? m_players[idx].get() : nullptr; }

    size_t GetNumPlayers() const { return m_players.size(); }

    UnitPool& GetUnits() { return *m_unitPool; }
    ArmyPool& GetArmies() { return *m_armyPool; }
    CityPool& GetCities() { return *m_cityPool; }

    SlicEngine& GetSlic() { return *m_slic; }
    GameEventManager& GetEvents() { return *m_events; }

private:
    std::unique_ptr<TurnCount> m_turn;
    std::unique_ptr<World> m_world;
    std::unique_ptr<RandomGenerator> m_rand;

    std::vector<std::unique_ptr<::Player>> m_players;

    std::unique_ptr<UnitPool> m_unitPool;
    std::unique_ptr<ArmyPool> m_armyPool;
    std::unique_ptr<CityPool> m_cityPool;

    std::unique_ptr<SlicEngine> m_slic;
    std::unique_ptr<GameEventManager> m_events;
};

} // namespace Ctp2
