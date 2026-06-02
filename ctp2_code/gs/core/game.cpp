#include "ctp/c3.h"
#include "gs/core/game.h"

#include "gs/gameobj/Player.h"  // player_arr_Get / player_arr_Set, k_MAX_PLAYERS via c3.h
#include "gs/gameobj/ArmyPool.h"
// CityPool: forward-declared in game.h as a future-tense placeholder; no
// concrete class exists today (cities are owned per-player, not in a
// dedicated pool). Drop the include until the class is introduced.
#include "gs/gameobj/Player.h"
#include "gs/gameobj/pollution.h"
#include "gs/gameobj/TopTen.h"
#include "gs/gameobj/UnitPool.h"
#include "gs/gameobj/GameSettings.h"
#include "gs/gameobj/MessagePool.h"
#include "gs/gameobj/CivilisationPool.h"
#include "gs/gameobj/WonderTracker.h"
#include "gs/gameobj/TradePool.h"
#include "gs/gameobj/TradeOfferPool.h"
#include "gs/gameobj/AgreementPool.h"
#include "gs/gameobj/TerrImprovePool.h"
#include "gs/gameobj/installationpool.h"
#include "gs/gameobj/DiplomaticRequestPool.h"
#include "gs/gameobj/FeatTracker.h"
#include "gs/gameobj/EventTracker.h"
#include "gs/gameobj/AchievementTracker.h"
#include "gs/gameobj/TradeBids.h"
#include "gs/utility/RandGen.h"
#include "gs/utility/TurnCnt.h"
#include "gs/world/World.h"
#include "gs/slic/SlicEngine.h"
#include "gs/events/GameEventManager.h"

namespace Ctp2 {

Game::Game() = default;

Game::~Game() {
    // Default dtor would reset members in reverse declaration order
    // but would NOT clear the legacy global pointers (g_turn,
    // g_thePollution, ...) that NewGame's adopt-or-create path
    // publishes to.  Forgetting to clear them leaves dangling pointers
    // that confuse the next Game instance's adoption logic.  Cleanup()
    // nulls legacy pointers first, then destroys via unique_ptr.  Safe
    // to call multiple times.
    Cleanup();
}

Game::Game(Game&&) noexcept = default;
Game& Game::operator=(Game&&) noexcept = default;

void Game::NewGame(sint32 numPlayers, sint32 initialYear, sint32 randSeed) {
    // Adoption-or-create pattern: if gameinit has already allocated the
    // legacy global, adopt that instance into our unique_ptr (single
    // instance, Game owns the lifetime).  If no legacy global exists
    // (e.g. unit-test path where gameinit didn't run), create a fresh
    // instance and publish it back to the legacy pointer so any
    // accessor-based callers still see the same object.

    auto adoptOrCreateTurn = [&]() {
        if (turn_Get()) {
            m_turn.reset(turn_Get());
        } else {
            m_turn = std::make_unique<TurnCount>(numPlayers, initialYear);
            turn_Set(m_turn.get());
        }
    };
    auto adoptOrCreateRand = [&]() {
        if (rand_ptr()) {
            m_rand.reset(rand_ptr());
        } else {
            m_rand = std::make_unique<RandomGenerator>(randSeed);
            rand_ptr_Set(m_rand.get());
        }
    };
    auto ensurePollution = [&]() {
        // pollution_Set() now trampolines into m_pollution, so by the
        // time NewGame runs in production gameinit has already
        // populated m_pollution.  In unit-test context (no gameinit)
        // create a fresh instance so callers can rely on GetPollution.
        if (!m_pollution) {
            m_pollution = std::make_unique<Pollution>();
        }
    };
    // These subsystems now trampoline through Game — gameinit's
    // *_Set(new X()) calls populate m_x directly.  ensureX creates the
    // instance for the unit-test path (no gameinit).
    auto ensureTopTen   = [&]() { if (!m_topten)   m_topten   = std::make_unique<TopTen>();   };
    auto ensureUnitPool = [&]() { if (!m_unitPool) m_unitPool = std::make_unique<UnitPool>(); };
    auto ensureArmyPool = [&]() { if (!m_armyPool) m_armyPool = std::make_unique<ArmyPool>(); };

    // Reduce boilerplate for subsystems that follow the same pattern
    // (no-arg ctor, standard Get/Set accessor pair).
#define ADOPT_OR_CREATE(MEMBER, TYPE, GETTER, SETTER) \
    do {                                              \
        if (GETTER()) {                                \
            MEMBER.reset(GETTER());                    \
        } else {                                       \
            MEMBER = std::make_unique<TYPE>();         \
            SETTER(MEMBER.get());                      \
        }                                              \
    } while (0)

    adoptOrCreateTurn();
    adoptOrCreateRand();
    ensurePollution();
    ensureTopTen();
    ensureUnitPool();
    ensureArmyPool();

    // Subsystems with clean ctors (no global reads): adopt-or-create.
    ADOPT_OR_CREATE(m_messagePool,            MessagePool,            messagepool_Get,            messagepool_Set);
    ADOPT_OR_CREATE(m_civilisationPool,       CivilisationPool,       civilisationpool_Get,       civilisationpool_Set);
    ADOPT_OR_CREATE(m_wonderTracker,          WonderTracker,          wonder_tracker_Get,         wonder_tracker_Set);
    ADOPT_OR_CREATE(m_tradePool,              TradePool,              tradepool_Get,              tradepool_Set);
    ADOPT_OR_CREATE(m_tradeOfferPool,         TradeOfferPool,         tradeofferpool_Get,         tradeofferpool_Set);
    ADOPT_OR_CREATE(m_agreementPool,          AgreementPool,          agreementpool_Get,          agreementpool_Set);
    ADOPT_OR_CREATE(m_terrainImprovementPool, TerrainImprovementPool, terrimprovepool_Get,        terrimprovepool_Set);
    ADOPT_OR_CREATE(m_installationPool,       InstallationPool,       installationpool_Get,       installationpool_Set);
    ADOPT_OR_CREATE(m_diplomaticRequestPool,  DiplomaticRequestPool,  diplomaticrequestpool_Get,  diplomaticrequestpool_Set);
    ADOPT_OR_CREATE(m_eventTracker,           EventTracker,           eventtracker_Get,           eventtracker_Set);
    ADOPT_OR_CREATE(m_achievementTracker,     AchievementTracker,     achievementtracker_Get,     achievementtracker_Set);
    ADOPT_OR_CREATE(m_tradeBids,              TradeBids,              tradebids_Get,              tradebids_Set);

    // Subsystems whose ctors dereference app-lifetime globals (DBs,
    // network) and so can't be created from scratch in unit-test
    // context — adopt-only.  Production gameinit always allocates
    // these before Game::NewGame runs, so the adoption branch fires.
    //   GameSettings : reads profiledb_Get(), g_network in ctor.
    //   FeatTracker  : reads g_theFeatDB, g_theBuildingDB in ctor.
    //   World        : ctor needs map config (size + wrap flags) read
    //                  from profiledb_Get(); adoption is the natural fit.
    if (gamesettings_Get()) m_settings.reset(gamesettings_Get());
    if (feattracker_Get())  m_featTracker.reset(feattracker_Get());
    if (world_Get())        m_world.reset(world_Get());

    // SlicEngine and GameEventManager have non-trivial init (SLIC file
    // loading, event-hook registration) that gameinit handles. Game
    // adopts the already-initialised instances.
    if (slicengine_Get())   m_slic.reset(slicengine_Get());
    if (gevmanager_Get())   m_events.reset(gevmanager_Get());

    // Players[]: gameinit allocates the Player** array and per-slot
    // Players (gameinit_InitializePlayers /
    // gameinit_InitializeScenarioPlayers).  Game adopts the raw array
    // pointer matching the legacy g_player shape; Cleanup tears down
    // inner Players + array.  Adopt-only — no fresh-create branch.
    if (player_arr_Get()) m_playerArr = player_arr_Get();

#undef ADOPT_OR_CREATE

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
    // Reverse-dependency-order destruction.
    //
    // We clear the legacy global pointer BEFORE destroying via unique_ptr.
    // That way gameinit_Cleanup()'s subsequent `allocated::clear(...)`
    // calls become no-ops (delete NULL is safe) and we avoid a
    // double-delete on the single shared instance.

    gevmanager_Set(nullptr);
    m_events.reset();
    slicengine_Set(nullptr);
    m_slic.reset();

    // Trackers and pools: null the legacy pointer first, then destroy.
    tradebids_Set(nullptr);             m_tradeBids.reset();
    achievementtracker_Set(nullptr);    m_achievementTracker.reset();
    eventtracker_Set(nullptr);          m_eventTracker.reset();
    feattracker_Set(nullptr);           m_featTracker.reset();
    diplomaticrequestpool_Set(nullptr); m_diplomaticRequestPool.reset();
    installationpool_Set(nullptr);      m_installationPool.reset();
    terrimprovepool_Set(nullptr);       m_terrainImprovementPool.reset();
    agreementpool_Set(nullptr);         m_agreementPool.reset();
    tradeofferpool_Set(nullptr);        m_tradeOfferPool.reset();
    tradepool_Set(nullptr);             m_tradePool.reset();

    wonder_tracker_Set(nullptr);        m_wonderTracker.reset();
    civilisationpool_Set(nullptr);      m_civilisationPool.reset();
    messagepool_Set(nullptr);           m_messagePool.reset();
    gamesettings_Set(nullptr);          m_settings.reset();

    m_topten.reset();

    // pollution storage now lives entirely in m_pollution; resetting
    // the unique_ptr also nulls the trampoline-routed legacy accessor.
    m_pollution.reset();

    m_armyPool.reset();
    m_unitPool.reset();

    // Players[]: null the legacy global first so gameinit_Cleanup's
    // own `delete g_player[i]; delete[] g_player` block becomes a no-op
    // (it guards on `if (g_player)`).  Then free the adopted storage.
    if (m_playerArr) {
        player_arr_Set(nullptr);
        for (sint32 i = 0; i < k_MAX_PLAYERS; ++i) {
            delete m_playerArr[i];
        }
        delete[] m_playerArr;
        m_playerArr = nullptr;
    }

    world_Set(nullptr);
    m_world.reset();

    rand_ptr_Set(nullptr);
    m_rand.reset();

    turn_Set(nullptr);
    m_turn.reset();
}

Pollution * Game::GetPollutionPtr()             { return m_pollution.get(); }
void        Game::SetPollutionPtr(Pollution *p) { m_pollution.reset(p);     }

TopTen *   Game::GetTopTenPtr()           { return m_topten.get();   }
void       Game::SetTopTenPtr(TopTen *p)  { m_topten.reset(p);       }
UnitPool * Game::GetUnitsPtr()            { return m_unitPool.get(); }
void       Game::SetUnitsPtr(UnitPool *p) { m_unitPool.reset(p);     }
ArmyPool * Game::GetArmiesPtr()           { return m_armyPool.get(); }
void       Game::SetArmiesPtr(ArmyPool *p){ m_armyPool.reset(p);     }

Player* Game::GetPlayer(sint32 idx) {
    if (!m_playerArr || idx < 0 || idx >= k_MAX_PLAYERS) return nullptr;
    return m_playerArr[idx];
}

const Player* Game::GetPlayer(sint32 idx) const {
    if (!m_playerArr || idx < 0 || idx >= k_MAX_PLAYERS) return nullptr;
    return m_playerArr[idx];
}

} // namespace Ctp2
