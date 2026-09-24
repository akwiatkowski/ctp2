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
// Per-game AI + pathing state. Forward-declared here so game.h stays free
// of ai/ and robot/ include chains; full types only in game.cpp.
class Scheduler;
class SchedulerRegistry;
class Governor;
class GovernorRegistry;
class Diplomat;
class DiplomatRegistry;
class AgreementMatrix;
class SettleMap;
class MapAnalysis;
class PathingContext;
class CityAstar;
class TradeAstar;
class RobotAstar2;

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
    void Cleanup();

    // Accessors
    TurnCount& GetTurn() { return *m_turn; }
    const TurnCount& GetTurn() const { return *m_turn; }
    TurnCount * GetTurnPtr();
    void        SetTurnPtr(TurnCount *p);

    World& GetWorld() { return *m_world; }
    const World& GetWorld() const { return *m_world; }
    World * GetWorldPtr();
    void    SetWorldPtr(World *p);

    RandomGenerator& GetRand() { return *m_rand; }
    RandomGenerator * GetRandPtr();
    void              SetRandPtr(RandomGenerator *p);

    // Bounds-checked slot access into the Player** array.  Returns null
    // when out of range or when no player occupies the slot.  Bodies live
    // in game.cpp so this header doesn't need k_MAX_PLAYERS.
    Player*       GetPlayer(sint32 idx);
    const Player* GetPlayer(sint32 idx) const;

    // Raw access to the adopted Player** array (matches legacy g_player
    // shape).  Returns nullptr before NewGame.
    void AdoptPlayers(Player **players) { m_playerArr.reset(players); }
    Player **       GetPlayerArray()       { return m_playerArr.get(); }
    Player * const* GetPlayerArray() const { return m_playerArr.get(); }

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
    GameSettings * GetSettingsPtr();
    void           SetSettingsPtr(GameSettings *p);

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
    SlicEngine * GetSlicPtr();
    void         SetSlicPtr(SlicEngine *p);
    GameEventManager& GetEvents() { return *m_events; }
    GameEventManager * GetEventsPtr();
    void               SetEventsPtr(GameEventManager *p);
    FeatTracker * GetFeatsPtr();
    void          SetFeatsPtr(FeatTracker *p);
    // Per-game AI + pathing state. Registries own the per-player vectors
    // (replacing Scheduler::Schedulers()/Governor::Governors()/Diplomat
    // statics); finders + PathingContext replace the file-scope singletons
    // (AstarPathing()/CityPathing()/TradePathing()/AiPathing()). Bodies in
    // game.cpp, where the full types are visible.
    SchedulerRegistry & GetSchedulers();
    GovernorRegistry &  GetGovernors();
    DiplomatRegistry &  GetDiplomats();
    AgreementMatrix & GetAgreementsAI();
    SettleMap &       GetSettleMap();
    MapAnalysis &     GetMapAnalysisAI();
    PathingContext &  GetPathing();
    CityAstar &       GetCityPather();
    TradeAstar &      GetTradePather();
    RobotAstar2 &     GetAiPather();
    bool              NeedAnotherMatchCycle() const;
    void              SetNeedAnotherMatchCycle(bool needed);
    // Active-game routing for the legacy static shims (AstarPathing(),
    // CityPathing(), Scheduler::GetScheduler(), ...). Single process-wide
    // pointer set by gameinit/CivApp on NewGame; two sequential games each
    // re-point it, so the second never reads the first's state. True
    // concurrent games must hold their own Game& instead (callers migrate
    // to game.GetX() slice by slice).
    static Game *& ActiveRef();
    static Game * GetActive();
    static void SetActive(Game * game);

private:
    std::unique_ptr<TurnCount> m_turn;
    std::unique_ptr<World> m_world;
    std::unique_ptr<RandomGenerator> m_rand;

    // The per-session player roster.  Shape matches legacy g_player:
    // a heap-allocated array of k_MAX_PLAYERS slots, each holding a raw
    // Player* (null when the slot is empty).  Game adopts the legacy
    // pointer in NewGame and tears down inner Players + array in Cleanup.
    ::std::unique_ptr<::Player *[]> m_playerArr;

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
    // Per-game AI registries + pathing state (see accessors above).
    // Schedulers/Governors/Diplomats hold the Registry-by-value members;
    // AgreementMatrix/SettleMap/MapAnalysis are values; finders are values
    // (they reset per-call state up front). m_needAnotherMatchCycle replaces
    // Scheduler::s_needAnotherCycle.
    std::unique_ptr<SchedulerRegistry> m_schedulers;
    std::unique_ptr<GovernorRegistry>  m_governors;
    std::unique_ptr<DiplomatRegistry> m_diplomats;
    std::unique_ptr<AgreementMatrix> m_agreementsAI;
    std::unique_ptr<SettleMap>       m_settleMap;
    std::unique_ptr<MapAnalysis>     m_mapAnalysis;
    std::unique_ptr<PathingContext>  m_pathing;
    std::unique_ptr<CityAstar>       m_cityPather;
    std::unique_ptr<TradeAstar>      m_tradePather;
    std::unique_ptr<RobotAstar2>     m_aiPather;
    bool m_needAnotherMatchCycle = false;
};

} // namespace Ctp2
