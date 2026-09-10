// test/cpp/test_citydata.cpp
// First test against a real game object — CityData.
//
// This test deliberately keeps global setup minimal to prove the harness can
// instantiate and query a CityData without crashing. More complex tests that
// load databases and exercise production/combat/growth logic will follow.

#include "doctest.h"
#include "ctp/c3.h"
#include "gs/gameobj/citydata.h"
#include "gs/world/MapPoint.h"
#include "gs/world/World.h"
#include "gs/gameobj/player.h"
#include "gs/utility/gstypes.h"
#include "gs/newdb/CTPDatabase.h"
#include "CitySizeRecord.h"
#include "ResourceRecord.h"
#include "ConstRecord.h"
#include "BuildingRecord.h"
#include "gs/utility/Globals.h"
#include "gs/database/profileDB.h"
#include "gs/utility/safety.h"
#include "gs/gameobj/GameSettings.h"
#include "gs/gameobj/CivilisationPool.h"
#include "ctp/civapp.h"
#include "gs/core/game.h"
#include "gs/utility/MapFile.h"
#include "ai/strategy/scheduler/Scheduler.h"
#include <array>
#include <fstream>
#include <unistd.h>

// Minimal fixture: CityData ctor dereferences world_Get(), player_arr_Get(),
// g_theCitySizeDB and g_theResourceDB.  After the trampoline migration,
// world_Set/player_arr_Set route through civapp_Get()->GetGame(), so the
// fixture constructs a lightweight CivApp; its eager-constructed Game
// owns the world and tears it down on dtor.  The raw g_theX_DB pointers
// are pre-trampoline storage and still work as direct assignments.
struct CityDataFixture
{
    CivApp * app = nullptr;
    CTPDatabase<CitySizeRecord> *stubCitySizeDB = nullptr;
    CTPDatabase<ResourceRecord> *stubResourceDB = nullptr;
    CTPDatabase<ConstRecord> *stubConstDB = nullptr;
    CTPDatabase<BuildingRecord> *stubBuildingDB = nullptr;
    CTPDatabase<CitySizeRecord> *previousCitySizeDB = g_theCitySizeDB;
    CTPDatabase<ResourceRecord> *previousResourceDB = g_theResourceDB;
    CTPDatabase<ConstRecord> *previousConstDB = g_theConstDB;
    CTPDatabase<BuildingRecord> *previousBuildingDB = g_theBuildingDB;

    CityDataFixture()
    {
        // CivApp's eager m_game container hosts the trampoline targets
        // (world_Set / player_arr_Set / etc.).
        app = new CivApp();
        civapp_Set(app);

        world_Set(new World(MapPoint(20, 20), false, false));

        // CityData ctor checks player_Get(owner) before dereferencing
        Player ** players = new Player *[k_MAX_PLAYERS];
        for (int i = 0; i < k_MAX_PLAYERS; ++i)
        {
            players[i] = nullptr;
        }
        player_arr_Set(players);

        // CityData ctor allocates arrays sized by NumRecords() and calls
        // ResetStarvationTurns() which reads g_theConstDB and buildingutil_*
        // which reads g_theBuildingDB. Empty databases (NumRecords() == 0)
        // are sufficient for basic instantiation tests.
        stubCitySizeDB = new CTPDatabase<CitySizeRecord>();
        g_theCitySizeDB = stubCitySizeDB;

        stubResourceDB = new CTPDatabase<ResourceRecord>();
        g_theResourceDB = stubResourceDB;

        stubConstDB = new CTPDatabase<ConstRecord>();
        g_theConstDB = stubConstDB;

        stubBuildingDB = new CTPDatabase<BuildingRecord>();
        g_theBuildingDB = stubBuildingDB;
    }

    ~CityDataFixture()
    {
        // Game::Cleanup runs in ~CivApp via ~Game on m_game; it tears
        // down m_world and m_playerArr (incl. inner Players) for us.
        delete app;
        civapp_Set(nullptr);
        delete stubCitySizeDB;
        delete stubResourceDB;
        delete stubConstDB;
        delete stubBuildingDB;
        // Stub tests can run between tests using the cached real databases.
        g_theCitySizeDB = previousCitySizeDB;
        g_theResourceDB = previousResourceDB;
        g_theConstDB = previousConstDB;
        g_theBuildingDB = previousBuildingDB;
    }
};

TEST_CASE_FIXTURE(CityDataFixture, "Stub world is allocated")
{
    CHECK(world_Get() != nullptr);
}

TEST_CASE_FIXTURE(CityDataFixture, "CityData constructor initializes owner and home city")
{
    CityData city(3, Unit(), MapPoint(7, 9));

    CHECK(city.GetOwner() == 3);
    CHECK(city.GetHomeCity().m_id == 0);
}

TEST_CASE_FIXTURE(CityDataFixture, "CityData starts with zero population and resources")
{
    CityData city(0, Unit(), MapPoint(0, 0));

    CHECK(city.GetStoredCityFood() == 0);
    CHECK(city.GetStoredCityProduction() == 0);
    CHECK(city.GetNetCityGold() == 0);
    CHECK(city.GetGrossCityGold() == 0);
    CHECK(city.GetNetCityProduction() == 0);
    CHECK(city.GetGrossCityProduction() == 0);
}

TEST_CASE_FIXTURE(CityDataFixture, "CityData material and military contribution defaults")
{
    CityData city(1, Unit(), MapPoint(2, 3));

    // Default constructor sets both contributions to true
    CHECK(city.GetMaterialContribution() == true);
    CHECK(city.GetMilitaryContribution() == true);
}

TEST_CASE_FIXTURE(CityDataFixture, "CityData starvation starts at zero")
{
    CityData city(0, Unit(), MapPoint(5, 5));

    CHECK(city.GetStarvationTurns() == 0);
}

TEST_CASE_FIXTURE(CityDataFixture, "CityData name can be set and retrieved")
{
    CityData city(0, Unit(), MapPoint(1, 1));

    city.SetName("Testville");
    CHECK(strcmp(city.GetName(), "Testville") == 0);
}

TEST_CASE_FIXTURE(CityDataFixture, "CityData starts with no improvements or wonders")
{
    CityData city(0, Unit(), MapPoint(3, 3));

    CHECK(city.GetImprovements() == 0);
    CHECK(city.GetBuiltWonders() == 0);
    CHECK(city.HasBuilding(0) == false);
    CHECK(city.HasCityWonder(0) == false);
}

TEST_CASE_FIXTURE(CityDataFixture, "CityData improvement and wonder flags can be set")
{
    CityData city(0, Unit(), MapPoint(4, 4));

    city.SetImprovements(0x05);  // bits 0 and 2
    CHECK(city.HasBuilding(0) == true);
    CHECK(city.HasBuilding(1) == false);
    CHECK(city.HasBuilding(2) == true);

    city.SetWonders(0x02);  // bit 1
    CHECK(city.HasCityWonder(1) == true);
    CHECK(city.HasCityWonder(0) == false);
}

TEST_CASE_FIXTURE(CityDataFixture, "CityData is not a capitol by default")
{
    CityData city(0, Unit(), MapPoint(6, 6));

    CHECK(city.IsCapitol() == false);
}

TEST_CASE_FIXTURE(CityDataFixture, "CityData population defaults")
{
    CityData city(0, Unit(), MapPoint(7, 7));

    CHECK(city.GetNumPop() == 1);   // GetNumPop clamps to minimum 1
}

TEST_CASE_FIXTURE(CityDataFixture, "CityData contribution toggles work")
{
    CityData city(0, Unit(), MapPoint(8, 8));

    CHECK(city.GetMaterialContribution() == true);
    CHECK(city.GetMilitaryContribution() == true);

    city.SetMaterialContribution(false);
    city.SetMilitaryContribution(false);

    CHECK(city.GetMaterialContribution() == false);
    CHECK(city.GetMilitaryContribution() == false);
}

TEST_CASE_FIXTURE(CityDataFixture, "CityData status flags default to false")
{
    CityData city(0, Unit(), MapPoint(9, 9));

    CHECK(city.GetIsRioting() == false);
    CHECK(city.IsFranchised() == false);
    CHECK(city.IsConverted() == false);
    CHECK(city.IsBioInfected() == false);
    CHECK(city.IsNanoInfected() == false);
    CHECK(city.IsInjoined() == false);
}

TEST_CASE_FIXTURE(CityDataFixture, "CityData BuildQueue and Happy are allocated")
{
    CityData city(0, Unit(), MapPoint(2, 2));

    CHECK(city.GetBuildQueue() != nullptr);
    CHECK(city.GetHappy() != nullptr);
}

TEST_CASE_FIXTURE(CityDataFixture, "CityData shield store can be modified")
{
    CityData city(0, Unit(), MapPoint(3, 3));

    CHECK(city.GetStoredCityProduction() == 0);
    city.AddShields(50);
    CHECK(city.GetStoredCityProduction() == 50);
    city.AddShields(25);
    CHECK(city.GetStoredCityProduction() == 75);
}

TEST_CASE_FIXTURE(CityDataFixture, "CityData starts with no trade routes")
{
    CityData city(0, Unit(), MapPoint(4, 4));

    CHECK(city.GetNumTradeRoutes() == 0);
    CHECK(city.GetIncomingTrade() == 0);
    CHECK(city.GetOutgoingTrade() == 0);
}

TEST_CASE_FIXTURE(CityDataFixture, "CityData science and crime defaults")
{
    CityData city(0, Unit(), MapPoint(5, 5));

    CHECK(city.GetScience() == 0);
    CHECK(city.GetTradeCrime() == 0);
    CHECK(city.GetProdCrime() == 0);
}

//----------------------------------------------------------------------------
// Heavy fixture: loads all real game databases from ctp2_data/.
// Databases are loaded once (static) and reused across tests for speed.
// Each test owns a fresh session and one Player for the economy checks.
//----------------------------------------------------------------------------

#include "ctp/civapp.h"
#include "gs/fileio/CivPaths.h"
#include "gs/utility/gameinit.h"
#include "gs/core/game_observer.h"
#include "gs/fileio/json_save.h"
#include "gs/gameobj/MessagePool.h"
#include "gs/gameobj/MessageData.h"
#include "ui/aui_ctp2/SelItem.h"
#include "gs/slic/SlicEngine.h"
#include "gs/utility/RandGen.h"
#include "gs/utility/TurnCnt.h"


struct HeavyCityDataFixture
{
    static bool s_dbsLoaded;
    CivApp app;
    std::array<Player *, k_MAX_PLAYERS> players{};
    std::unique_ptr<Player> player;
    std::unique_ptr<SelectedItem> selected;

    HeavyCityDataFixture()
    {
        civapp_Set(&app);
        if (!s_dbsLoaded)
        {
            set_headless(true);

            fprintf(stderr, "[HeavyFixture] Loading databases...\n");
            CivPaths_InitCivPaths();

            if (!gameinit_InitializeGameFiles())
            {
                fprintf(stderr, "[HeavyFixture] WARNING: gameinit_InitializeGameFiles failed\n");
            }

            profiledb_Set(new ProfileDB());
            profiledb_Get()->Init(FALSE);

            if (!app.InitializeAppDB())
            {
                fprintf(stderr, "[HeavyFixture] WARNING: InitializeAppDB failed\n");
            }

            fprintf(stderr, "[HeavyFixture] Databases loaded.\n");
            s_dbsLoaded = true;
        }

        world_Set(new World(MapPoint(64, 48), false, false));
        gamesettings_Set(new GameSettings());
        civilisationpool_Set(new CivilisationPool());
        player_arr_Set(players.data());
        selected = std::make_unique<SelectedItem>(1);
        selitem_Set(selected.get());
        slicengine_Set(new SlicEngine());
        rand_ptr_Set(new RandomGenerator(12345));
        // Test fixture: no real game setup. Default to 0 players, year 0;
        // the test exercises CityData logic, not TurnCount semantics.
        turn_Set(new TurnCount(0, 0));

        // Player registers itself in the legacy array; the fixture owns it.
        player = std::make_unique<Player>(0, 0, PLAYER_TYPE_HUMAN);
    }

    ~HeavyCityDataFixture()
    {
        // Destroy session state while its trampoline accessors still work.
        player.reset();
        player_arr_Set(nullptr);
        app.GetGame()->Cleanup();
        selitem_Set(nullptr);
        civapp_Set(nullptr);
    }
};

bool HeavyCityDataFixture::s_dbsLoaded = false;

extern PointerList<Player> *g_deadPlayer;

TEST_CASE_FIXTURE(HeavyCityDataFixture, "Game cleanup releases live and retired players before their civilisation pool")
{
    // Transfer the fixture's real Player and a heap array to Game, just as
    // production startup does. Exercise both ownership paths with real handles.
    auto **ownedPlayers = new Player *[k_MAX_PLAYERS]{};
    ownedPlayers[0] = player.release();
    player_arr_Set(ownedPlayers);
    SUBCASE("retired player") {
        REQUIRE(g_deadPlayer == nullptr);
        g_deadPlayer = new PointerList<Player>;
        g_deadPlayer->AddTail(ownedPlayers[0]);
        ownedPlayers[0] = nullptr;
    }
    SUBCASE("live player") {}
    SUBCASE("partial startup before adoption") {
        gameinit_Cleanup();
        CHECK(player_arr_Get() == nullptr);
        CHECK(civilisationpool_Get() == nullptr);
        return;
    }
    app.GetGame()->NewGame(1, 0, 12345);
    app.GetGame()->Cleanup();
    CHECK(player_arr_Get() == nullptr);
    CHECK(g_deadPlayer == nullptr);
    CHECK(civilisationpool_Get() == nullptr);
    app.GetGame()->Cleanup();
}


TEST_CASE_FIXTURE(HeavyCityDataFixture, "Message pool teardown passes live data to window observers")
{
    messagepool_Set(new MessagePool());
    struct Observer : IGameObserver {
        int destroyed = 0;
        void OnMessageWindowDestroy(MessageData const &data) override {
            CHECK(messagepool_Get() == nullptr);
            CHECK(data.GetMessageWindow() != nullptr);
            ++destroyed;
        }
    } observer;
    auto &registry = GameObserverRegistry::Instance();
    auto *previousRegistry = gameobservers_Get();
    gameobservers_Set(&registry);
    registry.Register(&observer);
    auto message = messagepool_Get()->ServerCreate();
    // Opaque identity only: the headless observer never dereferences the UI window.
    char windowIdentity;
    message.AccessData()->SetMessageWindow(reinterpret_cast<MessageWindow *>(&windowIdentity));
    app.GetGame()->SetMessagesPtr(nullptr);
    CHECK(observer.destroyed == 1);
    registry.Unregister(&observer);
    gameobservers_Set(previousRegistry);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "Heavy fixture loads real ConstDB")
{
    REQUIRE(g_theConstDB != nullptr);
    CHECK(g_theConstDB->NumRecords() > 0);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "Heavy fixture loads real BuildingDB")
{
    REQUIRE(g_theBuildingDB != nullptr);
    CHECK(g_theBuildingDB->NumRecords() > 0);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "Heavy fixture loads real CitySizeDB")
{
    REQUIRE(g_theCitySizeDB != nullptr);
    CHECK(g_theCitySizeDB->NumRecords() > 0);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "Heavy fixture loads real ResourceDB")
{
    REQUIRE(g_theResourceDB != nullptr);
    CHECK(g_theResourceDB->NumRecords() > 0);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "CityData with real DBs gets real starvation protection")
{
    CityData city(0, Unit(), MapPoint(5, 5));

    const ConstRecord *rec = g_theConstDB->Get(0);
    REQUIRE(rec != nullptr);

    CHECK(city.GetStarvationTurns() == rec->GetBaseStarvationProtection());
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "CityData with real DBs has non-zero city size records")
{
    CityData city(0, Unit(), MapPoint(3, 3));

    CHECK(g_theCitySizeDB->NumRecords() > 0);
    const CitySizeRecord *rec = g_theCitySizeDB->Get(0);
    REQUIRE(rec != nullptr);

    // CityData constructor allocates ring arrays sized by NumRecords()
    // and sets m_sizeIndex = 0. Verify the first record exists.
    CHECK(rec->GetPopulation() >= 0);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "CityData is not a capitol with empty improvements even with real DB")
{
    CityData city(0, Unit(), MapPoint(7, 7));

    CHECK(city.IsCapitol() == false);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "CityData with real DBs has real starvation protection value")
{
    CityData city(0, Unit(), MapPoint(5, 5));

    const ConstRecord *rec = g_theConstDB->Get(0);
    REQUIRE(rec != nullptr);

    // BaseStarvationProtection from default ConstDB is typically > 0
    CHECK(rec->GetBaseStarvationProtection() > 0);
    CHECK(city.GetStarvationTurns() == rec->GetBaseStarvationProtection());
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "CityData capitol detection works with real BuildingDB")
{
    CityData city(0, Unit(), MapPoint(8, 8));

    // Find the building that designates a capitol
    sint32 capitolIndex = -1;
    for (sint32 i = 0; i < g_theBuildingDB->NumRecords(); ++i)
    {
        if (g_theBuildingDB->Get(i)->GetCapitol())
        {
            capitolIndex = i;
            break;
        }
    }

    REQUIRE(capitolIndex >= 0);

    // Without the building, city is not a capitol
    CHECK(city.IsCapitol() == false);

    // Set the capitol bit
    city.SetImprovements(safe_shift_left_u64(capitolIndex));
    CHECK(city.IsCapitol() == true);

    // Clear it again
    city.SetImprovements(0);
    CHECK(city.IsCapitol() == false);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "CityData city walls detection works with real BuildingDB")
{
    CityData city(0, Unit(), MapPoint(9, 9));

    // Find the building that provides city walls
    sint32 wallsIndex = -1;
    for (sint32 i = 0; i < g_theBuildingDB->NumRecords(); ++i)
    {
        if (g_theBuildingDB->Get(i)->GetCityWalls())
        {
            wallsIndex = i;
            break;
        }
    }

    REQUIRE(wallsIndex >= 0);

    CHECK(city.HasCityWalls() == false);

    city.SetImprovements(safe_shift_left_u64(wallsIndex));
    CHECK(city.HasCityWalls() == true);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "CityData airport detection works with real BuildingDB")
{
    CityData city(0, Unit(), MapPoint(10, 10));

    sint32 airportIndex = -1;
    for (sint32 i = 0; i < g_theBuildingDB->NumRecords(); ++i)
    {
        if (g_theBuildingDB->Get(i)->GetAirport())
        {
            airportIndex = i;
            break;
        }
    }

    REQUIRE(airportIndex >= 0);

    CHECK(city.HasAirport() == false);

    city.SetImprovements(safe_shift_left_u64(airportIndex));
    CHECK(city.HasAirport() == true);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "CityData safe from nukes detection works with real BuildingDB")
{
    CityData city(0, Unit(), MapPoint(11, 11));

    sint32 shelterIndex = -1;
    for (sint32 i = 0; i < g_theBuildingDB->NumRecords(); ++i)
    {
        if (g_theBuildingDB->Get(i)->GetProtectFromNukes())
        {
            shelterIndex = i;
            break;
        }
    }

    REQUIRE(shelterIndex >= 0);

    CHECK(city.SafeFromNukes() == false);

    city.SetImprovements(safe_shift_left_u64(shelterIndex));
    CHECK(city.SafeFromNukes() == true);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "CityData first CitySizeDB record has valid population")
{
    CityData city(0, Unit(), MapPoint(12, 12));

    const CitySizeRecord *rec = g_theCitySizeDB->Get(0);
    REQUIRE(rec != nullptr);

    // First city size tier should have a non-negative population threshold
    CHECK(rec->GetPopulation() >= 0);
    // Growth rate should be positive
    CHECK(rec->GetGrowthRate() > 0);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "CityData ConstDB has positive border radius")
{
    CityData city(0, Unit(), MapPoint(13, 13));

    const ConstRecord *rec = g_theConstDB->Get(0);
    REQUIRE(rec != nullptr);

    CHECK(rec->GetBorderIntRadius() > 0);
    CHECK(rec->GetBorderSquaredRadius() > 0);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "CityData overcrowding bonus with real BuildingDB")
{
    CityData city(0, Unit(), MapPoint(14, 14));

    // Find a building that raises overcrowding level
    sint32 buildingIdx = -1;
    sint32 expectedLevel = 0;
    for (sint32 i = 0; i < g_theBuildingDB->NumRecords(); ++i)
    {
        sint32 level;
        if (g_theBuildingDB->Get(i)->GetRaiseOvercrowdingLevel(level))
        {
            buildingIdx = i;
            expectedLevel = level;
            break;
        }
    }

    REQUIRE(buildingIdx >= 0);

    CHECK(city.GetBuildingOvercrowdingBonus() == 0);

    city.SetImprovements(safe_shift_left_u64(buildingIdx));
    CHECK(city.GetBuildingOvercrowdingBonus() == expectedLevel);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "CityData defenders bonus with real BuildingDB")
{
    CityData city(0, Unit(), MapPoint(15, 15));

    // Find a building that provides defenders bonus
    sint32 buildingIdx = -1;
    for (sint32 i = 0; i < g_theBuildingDB->NumRecords(); ++i)
    {
        double bonus;
        if (g_theBuildingDB->Get(i)->GetDefendersPercent(bonus) && bonus > 0)
        {
            buildingIdx = i;
            break;
        }
    }

    REQUIRE(buildingIdx >= 0);

    CHECK(city.GetDefendersBonusNoWalls() == 0.0);

    city.SetImprovements(safe_shift_left_u64(buildingIdx));
    CHECK(city.GetDefendersBonusNoWalls() > 0.0);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "CityData land attack bonus with real BuildingDB")
{
    CityData city(0, Unit(), MapPoint(16, 16));

    sint32 buildingIdx = -1;
    for (sint32 i = 0; i < g_theBuildingDB->NumRecords(); ++i)
    {
        double bonus;
        if (g_theBuildingDB->Get(i)->GetOffenseBonusLand(bonus) && bonus > 0)
        {
            buildingIdx = i;
            break;
        }
    }

    REQUIRE(buildingIdx >= 0);

    CHECK(city.GetCityLandAttackBonus() == 0.0);

    city.SetImprovements(safe_shift_left_u64(buildingIdx));
    CHECK(city.GetCityLandAttackBonus() > 0.0);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "CityData air attack bonus with real BuildingDB")
{
    CityData city(0, Unit(), MapPoint(17, 17));

    sint32 buildingIdx = -1;
    for (sint32 i = 0; i < g_theBuildingDB->NumRecords(); ++i)
    {
        double bonus;
        if (g_theBuildingDB->Get(i)->GetOffenseBonusAir(bonus) && bonus > 0)
        {
            buildingIdx = i;
            break;
        }
    }

    REQUIRE(buildingIdx >= 0);

    CHECK(city.GetCityAirAttackBonus() == 0.0);

    city.SetImprovements(safe_shift_left_u64(buildingIdx));
    CHECK(city.GetCityAirAttackBonus() > 0.0);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "CityData sea attack bonus with real BuildingDB")
{
    CityData city(0, Unit(), MapPoint(18, 18));

    sint32 buildingIdx = -1;
    for (sint32 i = 0; i < g_theBuildingDB->NumRecords(); ++i)
    {
        double bonus;
        if (g_theBuildingDB->Get(i)->GetOffenseBonusWater(bonus) && bonus > 0)
        {
            buildingIdx = i;
            break;
        }
    }

    REQUIRE(buildingIdx >= 0);

    CHECK(city.GetCitySeaAttackBonus() == 0.0);

    city.SetImprovements(safe_shift_left_u64(buildingIdx));
    CHECK(city.GetCitySeaAttackBonus() > 0.0);
}

//----------------------------------------------------------------------------
// Economy tests: real Player provides rations, real DBs provide thresholds.
//----------------------------------------------------------------------------

TEST_CASE_FIXTURE(HeavyCityDataFixture, "CityData food required per citizen with real Player")
{
    CityData city(0, Unit(), MapPoint(19, 19));

    double rations = city.GetFoodRequiredPerCitizen();

    // Player::InitPlayer calls SetRationsLevel with default expectation,
    // which uses ConstDB values. Rations should be positive.
    CHECK(rations > 0.0);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "CityData food required is zero for empty city")
{
    CityData city(0, Unit(), MapPoint(20, 20));

    // New city has PopCount() == 0, SlaveCount() == 0
    CHECK(city.GetFoodRequired() == 0.0);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "CityData max pop from real CitySizeDB is positive")
{
    CityData city(0, Unit(), MapPoint(21, 21));

    sint32 maxPop = city.GetMaxPop();

    // First city size tier should have a positive base max pop
    CHECK(maxPop > 0);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "CityData growth rate calculates without crash")
{
    CityData city(0, Unit(), MapPoint(22, 22));

    // With zero food delta and default population, this should not crash
    city.CalculateGrowthRate();

    // Growth rate should be set to some finite value
    sint32 rate = city.GetGrowthRate();
    CHECK(rate == 0);  // No food, no growth
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "CityData net and gross food start at zero")
{
    CityData city(0, Unit(), MapPoint(23, 23));

    CHECK(city.GetNetCityFood() == 0);
    CHECK(city.GetGrossCityFood() == 0);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "CityData gold and production start at zero")
{
    CityData city(0, Unit(), MapPoint(24, 24));

    CHECK(city.GetNetCityGold() == 0);
    CHECK(city.GetGrossCityGold() == 0);
    CHECK(city.GetNetCityProduction() == 0);
    CHECK(city.GetGrossCityProduction() == 0);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "CityData ConstDB base rations is positive")
{
    CityData city(0, Unit(), MapPoint(25, 25));

    const ConstRecord *rec = g_theConstDB->Get(0);
    REQUIRE(rec != nullptr);

    // Base rations should be a positive value (food each citizen needs)
    CHECK(rec->GetBaseRations() > 0.0);
}

//----------------------------------------------------------------------------
// Edge-case coverage on existing fixtures (wave 9a W4)
//----------------------------------------------------------------------------

TEST_CASE_FIXTURE(CityDataFixture, "CityData construction produces consistent default state")
{
    CityData city1(0, Unit(), MapPoint(1, 1));
    CityData city2(0, Unit(), MapPoint(2, 2));

    // Two fresh cities should share identical defaults
    CHECK(city1.GetStoredCityProduction() == city2.GetStoredCityProduction());
    CHECK(city1.GetImprovements() == city2.GetImprovements());
    CHECK(city1.GetBuiltWonders() == city2.GetBuiltWonders());
    CHECK(city1.GetScience() == city2.GetScience());
    CHECK(city1.GetCityStyle() == city2.GetCityStyle());

    // Verify specific known defaults
    CHECK(city1.GetStoredCityProduction() == 0);
    CHECK(city1.GetScience() == 0);
    CHECK(strlen(city1.GetName()) == 0);
    CHECK(city1.GetImprovements() == 0);
    CHECK(city1.GetBuiltWonders() == 0);
}

TEST_CASE_FIXTURE(CityDataFixture, "CityData shield store boundary conditions")
{
    CityData city(0, Unit(), MapPoint(3, 3));

    CHECK(city.GetStoredCityProduction() == 0);

    // Zero addition is a no-op
    city.AddShields(0);
    CHECK(city.GetStoredCityProduction() == 0);

    // Positive addition
    city.AddShields(100);
    CHECK(city.GetStoredCityProduction() == 100);

    // Negative addition subtracts
    city.AddShields(-30);
    CHECK(city.GetStoredCityProduction() == 70);

    // Direct assignment to zero
    city.SetShieldstore(0);
    CHECK(city.GetStoredCityProduction() == 0);

    // Direct assignment to negative
    city.SetShieldstore(-50);
    CHECK(city.GetStoredCityProduction() == -50);

    // Direct assignment to max sint32
    city.SetShieldstore(2147483647);
    CHECK(city.GetStoredCityProduction() == 2147483647);
}

TEST_CASE_FIXTURE(CityDataFixture, "CityData getters are well-defined without prior setter calls")
{
    CityData city(0, Unit(), MapPoint(4, 4));

    // These fields have no public setter; the getter must still return a
    // well-defined default immediately after construction.
    CHECK(city.GetTurnFounded() == 0);
    CHECK(city.GetScience() == 0);
    CHECK(strlen(city.GetName()) == 0);
    CHECK(city.GetBuildCategoryAtBeginTurn() == -4);
}

TEST_CASE_FIXTURE(CityDataFixture, "CityData repeated set-read cycles persist values")
{
    CityData city(0, Unit(), MapPoint(5, 5));

    // Name cycles
    city.SetName("Alpha");
    CHECK(strcmp(city.GetName(), "Alpha") == 0);
    city.SetName("Beta");
    CHECK(strcmp(city.GetName(), "Beta") == 0);
    city.SetName("");
    CHECK(strlen(city.GetName()) == 0);

    // Improvement bit-mask cycles
    city.SetImprovements(0x01);
    CHECK(city.GetImprovements() == 0x01);
    city.SetImprovements(0x03);
    CHECK(city.GetImprovements() == 0x03);
    city.SetImprovements(0);
    CHECK(city.GetImprovements() == 0);

    // Wonder bit-mask cycles
    city.SetWonders(0x01);
    CHECK(city.GetBuiltWonders() == 0x01);
    city.SetWonders(0x00);
    CHECK(city.GetBuiltWonders() == 0x00);
}

TEST_CASE_FIXTURE(CityDataFixture, "CityData probe recovered boolean toggles both ways")
{
    CityData city(0, Unit(), MapPoint(6, 6));

    CHECK(city.GetProbeRecoveredHere() == false);
    city.SetProbeRecoveredHere(true);
    CHECK(city.GetProbeRecoveredHere() == true);
    city.SetProbeRecoveredHere(false);
    CHECK(city.GetProbeRecoveredHere() == false);
}

TEST_CASE_FIXTURE(CityDataFixture, "CityData InitBeginTurnVariables resets turn flags to default")
{
    CityData city(0, Unit(), MapPoint(7, 7));

    // Set turn-local flags via their indicator methods
    city.IndicateTerrainPolluted();
    city.IndicateTerrainImprovementBuilt();
    city.IndicateImprovementBuilt();

    CHECK(city.WasTerrainPolluted() == true);
    CHECK(city.WasTerrainImprovementBuilt() == true);
    CHECK(city.WasImprovementBuilt() == true);

    // Reset all turn-local flags
    city.InitBeginTurnVariables();

    CHECK(city.WasTerrainPolluted() == false);
    CHECK(city.WasTerrainImprovementBuilt() == false);
    CHECK(city.WasImprovementBuilt() == false);
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "JSON world dimensions reject narrowing before replacing storage")
{
    for (auto n : {nlohmann::json(-1), nlohmann::json(0), nlohmann::json(65536),
                   nlohmann::json(UINT64_MAX), nlohmann::json(0.5)}) {
        nlohmann::json invalid = {{"size_x", n}, {"size_y", 48}};
        CHECK_THROWS_AS(invalid.get_to(*world_Get()), nlohmann::json::exception);
        CHECK(world_Get()->GetWidth() == 64);
        CHECK(world_Get()->GetHeight() == 48);
    }
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "Scheduler rejects corrupt graph and numeric fields without replacing goals")
{
    Scheduler scheduler;
    scheduler.SetPlayerId(0);
    nlohmann::json valid = scheduler;
    REQUIRE_FALSE(valid["goals"].empty());
    REQUIRE_NOTHROW(valid.get_to(scheduler));
    CHECK(nlohmann::json(scheduler) == valid);
    std::vector<nlohmann::json> invalid;
    auto j = valid;
    j["goals"][0]["goal_type"] = UINT64_MAX;
    invalid.push_back(j);
    j = valid;
    j["goals"][0]["playerId"] = uint64_t(1) << 32;
    invalid.push_back(j);
    j = valid;
    j["goals"][0]["target_pos"]["x"] = 64;
    invalid.push_back(j);
    j = valid;
    j["needed_strength"]["agent_count"] = 65536;
    invalid.push_back(j);
    j = valid;
    j["needed_strength"]["attack_str"] = 1e100;
    invalid.push_back(j);
    j = valid;
    j["active_goals"] = {0}; // Generic templates cannot own active slots.
    invalid.push_back(j);
    j = valid;
    j["goals"][0]["matches"] = {{{"agent", 0}}}; // No agents exist.
    invalid.push_back(j);
    for (auto const &bad : invalid) {
        CHECK_THROWS_AS(bad.get_to(scheduler), nlohmann::json::exception);
        CHECK(nlohmann::json(scheduler) == valid);
    }
}

TEST_CASE_FIXTURE(HeavyCityDataFixture, "Map JSON rejects malformed sections before changing the world")
{
    using nlohmann::json;
    char path[] = "/tmp/ctp2-map-bounds-XXXXXX";
    int fd = mkstemp(path);
    REQUIRE(fd >= 0);
    close(fd);
    struct RemoveFile { char const *path; ~RemoveFile() { std::remove(path); } } cleanup{path};
    auto load = [&](json const &doc) {
        { std::ofstream out(path); out << doc; }
        MapFile map;
        return map.Load(path);
    };
    json const header = {{"magic", "CTP2-MAP"}, {"schema_version", 1}};
    REQUIRE(load(header));
    auto *world = world_Get();
    auto *cell = world->GetCell(0, 0);
    json base = header;
    // A valid terrain section would replace storage if validation ran too late.
    base["terrain"] = {{"width", 64}, {"height", 48},
                       {"cells", std::vector<int>(64 * 48, 0)}};
    for (auto bad : {json(-1), json(0), json(65536), json(UINT64_MAX), json(0.5)}) {
        auto doc = base;
        doc["terrain"]["width"] = bad;
        CHECK_FALSE(load(doc));
        CHECK(world->GetCell(0, 0) == cell);
    }
    std::vector<json> invalid;
    auto doc = base;
    doc["terrain"]["cells"][0] = 256;
    invalid.push_back(doc);
    doc = base;
    doc["terrain_env"] = {{"width", 1}, {"height", 1}, {"cells", {0}}};
    invalid.push_back(doc);
    for (auto bad : {json(-1), json(k_MAX_PLAYERS), json(UINT64_MAX), json(0.5)}) {
        doc = base;
        doc["unit_types"] = {"UNIT_SETTLER"};
        doc["units"] = {{{"x", 0}, {"y", 0}, {"stack", {{{"owner", bad}, {"type", 0}}}}}};
        invalid.push_back(doc);
    }
    doc = base;
    doc["cities"] = {{{"x", 64}, {"y", 0}}};
    invalid.push_back(doc);
    doc = base;
    doc["vision"] = {{{"player", 0}, {"width", 1}, {"height", 1}, {"fog", {0}}}};
    invalid.push_back(doc);
    doc = base;
    doc["advance_types"] = {"ADVANCE_AGRICULTURE"};
    doc["advances"] = {{{"player", 0}, {"has", {1}}}};
    invalid.push_back(doc);
    doc = base;
    doc["civilizations"] = {{{"civ", UINT64_MAX}}};
    invalid.push_back(doc);
    doc = base;
    doc["cities"] = {{{"x", 0}, {"y", 0}, {"owner", 0}, {"size", 1},
                       {"improvements", "0x1garbage"}, {"wonders", "0x0"}, {"name", "City"}}};
    invalid.push_back(doc);
    for (auto const &bad : invalid) {
        CHECK_FALSE(load(bad));
        CHECK(world_Get() == world);
        CHECK(world->GetCell(0, 0) == cell);
        CHECK(player_Get(0) == player.get());
        CHECK_FALSE(player->m_disableChooseResearch);
    }
}
