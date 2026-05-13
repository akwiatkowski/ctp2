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

// Minimal fixture: CityData constructor dereferences g_theWorld, g_player,
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
        // CityData ctor calls g_theWorld->SetCapitolDistanceDirtyFlags()
        stubWorld = new World(MapPoint(20, 20), false, false);
        g_theWorld = stubWorld;

        // CityData ctor checks g_player[owner] before dereferencing
        stubPlayers = new Player *[k_MAX_PLAYERS];
        for (int i = 0; i < k_MAX_PLAYERS; ++i)
        {
            stubPlayers[i] = nullptr;
        }
        g_player = stubPlayers;

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
        g_theWorld = nullptr;
        g_player = nullptr;
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
    CHECK(g_theWorld != nullptr);
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

struct HeavyCityDataFixture
{
    static bool s_dbsLoaded;
    static CivApp *s_app;

    World *world = nullptr;

    HeavyCityDataFixture()
    {
        if (!s_dbsLoaded)
        {
            g_headlessMode = true;

            fprintf(stderr, "[HeavyFixture] Loading databases...\n");
            CivPaths_InitCivPaths();

            if (!gameinit_InitializeGameFiles())
            {
                fprintf(stderr, "[HeavyFixture] WARNING: gameinit_InitializeGameFiles failed\n");
            }

            g_theProfileDB = new ProfileDB();
            g_theProfileDB->Init(FALSE);

            s_app = new CivApp();
            if (!s_app->InitializeAppDB())
            {
                fprintf(stderr, "[HeavyFixture] WARNING: InitializeAppDB failed\n");
            }

            fprintf(stderr, "[HeavyFixture] Databases loaded.\n");
            s_dbsLoaded = true;
        }

        world = new World(MapPoint(64, 48), false, false);
        g_theWorld = world;

        g_player = new Player *[k_MAX_PLAYERS];
        for (int i = 0; i < k_MAX_PLAYERS; ++i)
        {
            g_player[i] = nullptr;
        }
    }

    ~HeavyCityDataFixture()
    {
        g_theWorld = nullptr;
        delete world;

        delete[] g_player;
        g_player = nullptr;
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
