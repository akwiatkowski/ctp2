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

    // Bounds-checked slot access into the Player** array.  Returns null
    // when out of range or when no player occupies the slot.  Bodies live
    // in game.cpp so this header doesn't need k_MAX_PLAYERS.
    Player*       GetPlayer(sint32 idx);
    const Player* GetPlayer(sint32 idx) const;

    // Raw access to the adopted Player** array (matches legacy g_player
    // shape).  Returns nullptr before NewGame.
    Player **       GetPlayerArray()       { return m_playerArr; }
    Player * const* GetPlayerArray() const { return m_playerArr; }

    UnitPool& GetUnits() { return *m_unitPool; }
    ArmyPool& GetArmies() { return *m_armyPool; }
    // CityPool: no such class exists today; cities are owned per-player.
    // Re-add when a pool is introduced.

    // Trampoline accessors — bodies in game.cpp.  These let the legacy
    // foo_Get/foo_Set free functions forward into Game's owned storage
    // so callers that haven't migrated to game.Get*() still work.
    UnitPool * GetUnitsPtr();
    void       SetUnitsPtr(UnitPool *p);
    ArmyPool * GetArmiesPtr();
    void       SetArmiesPtr(ArmyPool *p);

    Pollution& GetPollution() { return *m_pollution; }
    const Pollution& GetPollution() const { return *m_pollution; }
    Pollution * GetPollutionPtr();
    void        SetPollutionPtr(Pollution *p);

    TopTen& GetTopTen() { return *m_topten; }
    const TopTen& GetTopTen() const { return *m_topten; }
    TopTen * GetTopTenPtr();
    void     SetTopTenPtr(TopTen *p);

    GameSettings& GetSettings() { return *m_settings; }
    const GameSettings& GetSettings() const { return *m_settings; }

    MessagePool& GetMessages() { return *m_messagePool; }
    const MessagePool& GetMessages() const { return *m_messagePool; }
    MessagePool * GetMessagesPtr();
    void          SetMessagesPtr(MessagePool *p);

    CivilisationPool& GetCivilisations() { return *m_civilisationPool; }
    const CivilisationPool& GetCivilisations() const { return *m_civilisationPool; }
    CivilisationPool * GetCivilisationsPtr();
    void               SetCivilisationsPtr(CivilisationPool *p);

    WonderTracker& GetWonders() { return *m_wonderTracker; }
    const WonderTracker& GetWonders() const { return *m_wonderTracker; }
    WonderTracker * GetWondersPtr();
    void            SetWondersPtr(WonderTracker *p);

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

    // Trampoline pointer accessors for the remaining pool/tracker subsystems.
    TradePool *              GetTradesPtr();                void SetTradesPtr(TradePool *p);
    TradeOfferPool *         GetTradeOffersPtr();           void SetTradeOffersPtr(TradeOfferPool *p);
    AgreementPool *          GetAgreementsPtr();            void SetAgreementsPtr(AgreementPool *p);
    TerrainImprovementPool * GetTerrainImprovementsPtr();   void SetTerrainImprovementsPtr(TerrainImprovementPool *p);
    InstallationPool *       GetInstallationsPtr();         void SetInstallationsPtr(InstallationPool *p);
    DiplomaticRequestPool *  GetDiplomaticRequestsPtr();    void SetDiplomaticRequestsPtr(DiplomaticRequestPool *p);
    EventTracker *           GetEventTrackerPtr();          void SetEventTrackerPtr(EventTracker *p);
    AchievementTracker *     GetAchievementsPtr();          void SetAchievementsPtr(AchievementTracker *p);
    TradeBids *              GetTradeBidsPtr();             void SetTradeBidsPtr(TradeBids *p);

    SlicEngine& GetSlic() { return *m_slic; }
    GameEventManager& GetEvents() { return *m_events; }

private:
    std::unique_ptr<TurnCount> m_turn;
    std::unique_ptr<World> m_world;
    std::unique_ptr<RandomGenerator> m_rand;

    // The per-session player roster.  Shape matches legacy g_player:
    // a heap-allocated array of k_MAX_PLAYERS slots, each holding a raw
    // Player* (null when the slot is empty).  Game adopts the legacy
    // pointer in NewGame and tears down inner Players + array in Cleanup.
    ::Player ** m_playerArr = nullptr;

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
