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
