// test/cpp/test_citydata.cpp
// First test against a real game object — CityData.
//
// This test deliberately keeps global setup minimal to prove the harness can
// instantiate and query a CityData without crashing. More complex tests that
// load databases and exercise production/combat/growth logic will follow.

#include "doctest.h"
#include "ctp/c3.h"
#include "gs/gameobj/CityData.h"
#include "gs/world/MapPoint.h"
#include "gs/world/World.h"
#include "gs/gameobj/Player.h"
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

// Minimal fixture: CityData constructor dereferences world_Get(), player_arr_Get(),
// g_theCitySizeDB and g_theResourceDB. We provide bare-bones stubs so the
// constructor completes without crashing.
struct CityDataFixture
{
    World *stubWorld = nullptr;
    Player **stubPlayers = nullptr;
    CTPDatabase<CitySizeRecord> *stubCitySizeDB = nullptr;
    CTPDatabase<ResourceRecord> *stubResourceDB = nullptr;
    CTPDatabase<ConstRecord> *stubConstDB = nullptr;
    CTPDatabase<BuildingRecord> *stubBuildingDB = nullptr;

    CityDataFixture()
    {
        // CityData ctor calls world_Get()->SetCapitolDistanceDirtyFlags()
        stubWorld = new World(MapPoint(20, 20), false, false);
        world_Set(stubWorld);

        // CityData ctor checks player_Get(owner) before dereferencing
        stubPlayers = new Player *[k_MAX_PLAYERS];
        for (int i = 0; i < k_MAX_PLAYERS; ++i)
        {
            stubPlayers[i] = nullptr;
        }
        player_arr_Set(stubPlayers);

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
        world_Set(nullptr);
        player_arr_Set(nullptr);
        g_theCitySizeDB = nullptr;
        g_theResourceDB = nullptr;
        g_theConstDB = nullptr;
        g_theBuildingDB = nullptr;
        delete stubWorld;
        delete[] stubPlayers;
        delete stubCitySizeDB;
        delete stubResourceDB;
        delete stubConstDB;
        delete stubBuildingDB;
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
// Each test gets a fresh World; Player array remains null (defensive null
// checks in production code handle this gracefully).
//----------------------------------------------------------------------------

#include "ctp/civapp.h"
#include "gs/fileio/CivPaths.h"
#include "gs/utility/gameinit.h"
#include "ui/aui_ctp2/SelItem.h"
#include "gs/slic/SlicEngine.h"
#include "gs/utility/RandGen.h"
#include "gs/utility/TurnCnt.h"

extern CivApp *g_civApp;

struct HeavyCityDataFixture
{
    static bool s_dbsLoaded;
    static CivApp *s_app;

    World *world = nullptr;
    Player *player = nullptr;

    HeavyCityDataFixture()
    {
        if (!s_dbsLoaded)
        {
            set_headless(true);

            fprintf(stderr, "[HeavyFixture] Loading databases...\n");
            CivPaths_InitCivPaths();

            if (!gameinit_InitializeGameFiles())
            {
                fprintf(stderr, "[HeavyFixture] WARNING: gameinit_InitializeGameFiles failed\n");
            }

            g_theProfileDB = new ProfileDB();
            g_theProfileDB->Init(FALSE);

            gamesettings_Set(new GameSettings());
            civilisationpool_Set(new CivilisationPool());

            s_app = new CivApp();
            if (!s_app->InitializeAppDB())
            {
                fprintf(stderr, "[HeavyFixture] WARNING: InitializeAppDB failed\n");
            }

            fprintf(stderr, "[HeavyFixture] Databases loaded.\n");
            s_dbsLoaded = true;
        }

        world = new World(MapPoint(64, 48), false, false);
        world_Set(world);

        player_arr_Set(new Player *[k_MAX_PLAYERS]);
        for (int i = 0; i < k_MAX_PLAYERS; ++i)
        {
            player_arr_Get()[i] = nullptr;
        }

        // g_civApp may have been nulled by a previous test's fixture destructor.
        // Always restore it since s_app is a process-wide singleton.
        g_civApp = s_app;

        // SelectedItem and SlicEngine must exist before Player construction.
        // Player::InitPlayer calls Advances::InitialAdvance which calls
        // slicengine_Get()->CallMod, and other sub-objects may query g_selected_item.
        g_selected_item = new SelectedItem(1);
        slicengine_Set(new SlicEngine());
        rand_ptr_Set(new RandomGenerator(12345));
        // Test fixture: no real game setup. Default to 0 players, year 0;
        // the test exercises CityData logic, not TurnCount semantics.
        turn_Set(new TurnCount(0, 0));

        player = new Player(0, 0, PLAYER_TYPE_HUMAN);
    }

    ~HeavyCityDataFixture()
    {
        // Intentionally leak player, world, player array, selected_item,
        // and slic_engine. Their destructors access globals in ways not set
        // up in the test harness.
        world_Set(nullptr);
        player_arr_Set(nullptr);
        g_selected_item = nullptr;
        slicengine_Set(nullptr);
        g_civApp = nullptr;
        rand_ptr_Set(nullptr);
        turn_Set(nullptr);
    }
};

bool HeavyCityDataFixture::s_dbsLoaded = false;
CivApp *HeavyCityDataFixture::s_app = nullptr;

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
