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
    // All session subsystems trampoline through Ctp2::Game.  Production
    // gameinit calls foo_Set(new X(...)) BEFORE NewGame runs, which
    // populates the m_x unique_ptrs through the trampoline.  In
    // unit-test context (no gameinit) we create defaults for the
    // ones that have no-arg ctors; Turn and Rand still need explicit
    // ctor args (numPlayers/initialYear, randSeed) so they create
    // here if not pre-populated.

    auto ensure = [](auto& member, auto factory) {
        if (!member) member = factory();
    };

    ensure(m_turn,                   [&]{ return std::make_unique<TurnCount>(numPlayers, initialYear); });
    // Rand: legacy file-static, adopt into m_rand (see comment below).
    if (rand_ptr() && !m_rand) {
        m_rand.reset(rand_ptr());
    } else if (!m_rand) {
        m_rand = std::make_unique<RandomGenerator>(randSeed);
        rand_ptr_Set(m_rand.get());
    }
    ensure(m_pollution,              []{ return std::make_unique<Pollution>();              });
    ensure(m_topten,                 []{ return std::make_unique<TopTen>();                 });
    ensure(m_unitPool,               []{ return std::make_unique<UnitPool>();               });
    ensure(m_armyPool,               []{ return std::make_unique<ArmyPool>();               });
    ensure(m_messagePool,            []{ return std::make_unique<MessagePool>();            });
    ensure(m_civilisationPool,       []{ return std::make_unique<CivilisationPool>();       });
    ensure(m_wonderTracker,          []{ return std::make_unique<WonderTracker>();          });
    ensure(m_tradePool,              []{ return std::make_unique<TradePool>();              });
    ensure(m_tradeOfferPool,         []{ return std::make_unique<TradeOfferPool>();         });
    ensure(m_agreementPool,          []{ return std::make_unique<AgreementPool>();          });
    ensure(m_terrainImprovementPool, []{ return std::make_unique<TerrainImprovementPool>(); });
    ensure(m_installationPool,       []{ return std::make_unique<InstallationPool>();       });
    ensure(m_diplomaticRequestPool,  []{ return std::make_unique<DiplomaticRequestPool>();  });
    ensure(m_eventTracker,           []{ return std::make_unique<EventTracker>();           });
    ensure(m_achievementTracker,     []{ return std::make_unique<AchievementTracker>();     });
    ensure(m_tradeBids,              []{ return std::make_unique<TradeBids>();              });

    // World: legacy file-static storage (CityDataFixture uses
    // world_Set as a non-owning swap with manual delete).  Adopt the
    // legacy pointer here.
    if (world_Get() && !m_world) m_world.reset(world_Get());

    // Players[]: gameinit allocates the Player** array and per-slot
    // Players (gameinit_InitializePlayers /
    // gameinit_InitializeScenarioPlayers).  Game adopts the raw array
    // pointer matching the legacy g_player shape; Cleanup tears down
    // inner Players + array.  Adopt-only — no fresh-create branch.
    if (player_arr_Get()) m_playerArr = player_arr_Get();
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

    m_events.reset();
    m_slic.reset();

    // Trackers and pools: null the legacy pointer first, then destroy.
    // Trampolined subsystems: m_x.reset() also nulls the routed legacy
    // accessor.  Order matters: dependents before their dependencies.
    m_tradeBids.reset();
    m_achievementTracker.reset();
    m_eventTracker.reset();
    m_featTracker.reset();
    m_diplomaticRequestPool.reset();
    m_installationPool.reset();
    m_terrainImprovementPool.reset();
    m_agreementPool.reset();
    m_tradeOfferPool.reset();
    m_tradePool.reset();

    m_wonderTracker.reset();
    m_civilisationPool.reset();
    m_messagePool.reset();
    m_settings.reset();

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

    m_turn.reset();
}

// Trampoline accessor bodies — out-of-line because m_x.reset(p) needs
// the complete type for unique_ptr's deleter.
#define GAME_PTR_ACCESSORS(METHOD, TYPE, MEMBER)                    \
    TYPE * Game::Get##METHOD##Ptr()           { return MEMBER.get(); } \
    void   Game::Set##METHOD##Ptr(TYPE *p)    { MEMBER.reset(p);     }

GAME_PTR_ACCESSORS(Pollution,            Pollution,              m_pollution)
GAME_PTR_ACCESSORS(TopTen,               TopTen,                 m_topten)
GAME_PTR_ACCESSORS(Units,                UnitPool,               m_unitPool)
GAME_PTR_ACCESSORS(Armies,               ArmyPool,               m_armyPool)
GAME_PTR_ACCESSORS(Messages,             MessagePool,            m_messagePool)
GAME_PTR_ACCESSORS(Civilisations,        CivilisationPool,       m_civilisationPool)
GAME_PTR_ACCESSORS(Wonders,              WonderTracker,          m_wonderTracker)
GAME_PTR_ACCESSORS(Trades,               TradePool,              m_tradePool)
GAME_PTR_ACCESSORS(TradeOffers,          TradeOfferPool,         m_tradeOfferPool)
GAME_PTR_ACCESSORS(Agreements,           AgreementPool,          m_agreementPool)
GAME_PTR_ACCESSORS(TerrainImprovements,  TerrainImprovementPool, m_terrainImprovementPool)
GAME_PTR_ACCESSORS(Installations,        InstallationPool,       m_installationPool)
GAME_PTR_ACCESSORS(DiplomaticRequests,   DiplomaticRequestPool,  m_diplomaticRequestPool)
GAME_PTR_ACCESSORS(EventTracker,         EventTracker,           m_eventTracker)
GAME_PTR_ACCESSORS(Achievements,         AchievementTracker,     m_achievementTracker)
GAME_PTR_ACCESSORS(TradeBids,            TradeBids,              m_tradeBids)
GAME_PTR_ACCESSORS(Turn,                 TurnCount,              m_turn)
GAME_PTR_ACCESSORS(World,                World,                  m_world)
GAME_PTR_ACCESSORS(Rand,                 RandomGenerator,        m_rand)
GAME_PTR_ACCESSORS(Settings,             GameSettings,           m_settings)
GAME_PTR_ACCESSORS(Feats,                FeatTracker,            m_featTracker)
GAME_PTR_ACCESSORS(Slic,                 SlicEngine,             m_slic)
GAME_PTR_ACCESSORS(Events,               GameEventManager,       m_events)

#undef GAME_PTR_ACCESSORS

Player* Game::GetPlayer(sint32 idx) {
    if (!m_playerArr || idx < 0 || idx >= k_MAX_PLAYERS) return nullptr;
    return m_playerArr[idx];
}

const Player* Game::GetPlayer(sint32 idx) const {
    if (!m_playerArr || idx < 0 || idx >= k_MAX_PLAYERS) return nullptr;
    return m_playerArr[idx];
}

} // namespace Ctp2
