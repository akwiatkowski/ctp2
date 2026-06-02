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
#include "gs/gameobj/GameSettings.h"
#include "gs/gameobj/MessagePool.h"
#include "gs/gameobj/CivilisationPool.h"
#include "gs/gameobj/WonderTracker.h"
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
    auto adoptOrCreatePollution = [&]() {
        if (pollution_Get()) {
            m_pollution.reset(pollution_Get());
        } else {
            m_pollution = std::make_unique<Pollution>();
            pollution_Set(m_pollution.get());
        }
    };
    auto adoptOrCreateTopTen = [&]() {
        if (topten_Get()) {
            m_topten.reset(topten_Get());
        } else {
            m_topten = std::make_unique<TopTen>();
            topten_Set(m_topten.get());
        }
    };
    auto adoptOrCreateUnitPool = [&]() {
        if (unitpool_Get()) {
            m_unitPool.reset(unitpool_Get());
        } else {
            m_unitPool = std::make_unique<UnitPool>();
            unitpool_Set(m_unitPool.get());
        }
    };
    auto adoptOrCreateArmyPool = [&]() {
        if (armypool_Get()) {
            m_armyPool.reset(armypool_Get());
        } else {
            m_armyPool = std::make_unique<ArmyPool>();
            armypool_Set(m_armyPool.get());
        }
    };

    adoptOrCreateTurn();
    adoptOrCreateRand();
    adoptOrCreatePollution();
    adoptOrCreateTopTen();
    adoptOrCreateUnitPool();
    adoptOrCreateArmyPool();

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

    m_events.reset();
    m_slic.reset();

    topten_Set(nullptr);
    m_topten.reset();

    pollution_Set(nullptr);
    m_pollution.reset();

    armypool_Set(nullptr);
    m_armyPool.reset();

    unitpool_Set(nullptr);
    m_unitPool.reset();

    m_players.clear();
    m_world.reset();

    rand_ptr_Set(nullptr);
    m_rand.reset();

    turn_Set(nullptr);
    m_turn.reset();
}

} // namespace Ctp2
