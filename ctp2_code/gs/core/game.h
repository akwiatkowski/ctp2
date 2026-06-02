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
class Pollution;
class TopTen;
class GameSettings;
class MessagePool;
class CivilisationPool;
class WonderTracker;
class TradePool;
class TradeOfferPool;
class AgreementPool;
class TerrainImprovementPool;
class InstallationPool;
class DiplomaticRequestPool;
class FeatTracker;
class EventTracker;
class AchievementTracker;
class TradeBids;
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

    // Lifecycle.
    //
    // NewGame's parameters are the session-state inputs the owned
    // subsystems need: number of active players, calendar's starting
    // year (computed by the caller from the difficulty record), and
    // an RNG seed.  We pass them explicitly rather than reaching for
    // profiledb_Get()/gamesettings_Get from inside the subsystems, so
    // the class is constructible in isolation (e.g. from unit tests).
    void NewGame(sint32 numPlayers, sint32 initialYear, sint32 randSeed = 0);
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
    // CityPool: no such class exists today; cities are owned per-player.
    // Re-add when a pool is introduced.

    Pollution& GetPollution() { return *m_pollution; }
    const Pollution& GetPollution() const { return *m_pollution; }

    TopTen& GetTopTen() { return *m_topten; }
    const TopTen& GetTopTen() const { return *m_topten; }

    GameSettings& GetSettings() { return *m_settings; }
    const GameSettings& GetSettings() const { return *m_settings; }

    MessagePool& GetMessages() { return *m_messagePool; }
    const MessagePool& GetMessages() const { return *m_messagePool; }

    CivilisationPool& GetCivilisations() { return *m_civilisationPool; }
    const CivilisationPool& GetCivilisations() const { return *m_civilisationPool; }

    WonderTracker& GetWonders() { return *m_wonderTracker; }
    const WonderTracker& GetWonders() const { return *m_wonderTracker; }

    TradePool&              GetTrades()                { return *m_tradePool; }
    TradeOfferPool&         GetTradeOffers()           { return *m_tradeOfferPool; }
    AgreementPool&          GetAgreements()            { return *m_agreementPool; }
    TerrainImprovementPool& GetTerrainImprovements()   { return *m_terrainImprovementPool; }
    InstallationPool&       GetInstallations()         { return *m_installationPool; }
    DiplomaticRequestPool&  GetDiplomaticRequests()    { return *m_diplomaticRequestPool; }
    FeatTracker&            GetFeats()                 { return *m_featTracker; }
    EventTracker&           GetEventTracker()          { return *m_eventTracker; }
    AchievementTracker&     GetAchievements()          { return *m_achievementTracker; }
    TradeBids&              GetTradeBids()             { return *m_tradeBids; }

    SlicEngine& GetSlic() { return *m_slic; }
    GameEventManager& GetEvents() { return *m_events; }

private:
    std::unique_ptr<TurnCount> m_turn;
    std::unique_ptr<World> m_world;
    std::unique_ptr<RandomGenerator> m_rand;

    std::vector<std::unique_ptr<::Player>> m_players;

    std::unique_ptr<UnitPool> m_unitPool;
    std::unique_ptr<ArmyPool> m_armyPool;

    std::unique_ptr<Pollution>        m_pollution;
    std::unique_ptr<TopTen>           m_topten;
    std::unique_ptr<GameSettings>     m_settings;
    std::unique_ptr<MessagePool>      m_messagePool;
    std::unique_ptr<CivilisationPool> m_civilisationPool;
    std::unique_ptr<WonderTracker>    m_wonderTracker;

    std::unique_ptr<TradePool>              m_tradePool;
    std::unique_ptr<TradeOfferPool>         m_tradeOfferPool;
    std::unique_ptr<AgreementPool>          m_agreementPool;
    std::unique_ptr<TerrainImprovementPool> m_terrainImprovementPool;
    std::unique_ptr<InstallationPool>       m_installationPool;
    std::unique_ptr<DiplomaticRequestPool>  m_diplomaticRequestPool;
    std::unique_ptr<FeatTracker>            m_featTracker;
    std::unique_ptr<EventTracker>           m_eventTracker;
    std::unique_ptr<AchievementTracker>     m_achievementTracker;
    std::unique_ptr<TradeBids>              m_tradeBids;

    std::unique_ptr<SlicEngine> m_slic;
    std::unique_ptr<GameEventManager> m_events;
};

} // namespace Ctp2
