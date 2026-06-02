// test/cpp/test_happy.cpp
// Tests for Happy (city happiness tracker).

#include "doctest.h"
#include "ctp/c3.h"
#include "gs/gameobj/Happy.h"
#include "gs/gameobj/CityData.h"
#include "gs/utility/Globals.h"
#include "gs/fileio/CivPaths.h"
#include "gs/utility/gameinit.h"
#include "gs/database/profileDB.h"
#include "ctp/civapp.h"
#include "ConstRecord.h"

struct HappyFixture
{
    static bool s_dbsLoaded;
    static CivApp *s_app;

    HappyFixture()
    {
        if (!s_dbsLoaded)
        {
            set_headless(true);

            fprintf(stderr, "[HappyFixture] Loading databases...\n");
            CivPaths_InitCivPaths();
            gameinit_InitializeGameFiles();

            profiledb_Set(new ProfileDB());
            profiledb_Get()->Init(FALSE);

            s_app = new CivApp();
            s_app->InitializeAppDB();

            fprintf(stderr, "[HappyFixture] Databases loaded.\n");
            s_dbsLoaded = true;
        }
    }

    ~HappyFixture()
    {
    }
};

bool HappyFixture::s_dbsLoaded = false;
CivApp *HappyFixture::s_app = nullptr;

TEST_CASE_FIXTURE(HappyFixture, "Happy default values are zero")
{
    Happy h;

    CHECK(h.GetHappiness() == 0.0);
    CHECK(h.GetBase() == 0.0);
    CHECK(h.GetSize() == 0.0);
    CHECK(h.GetPollution() == 0.0);
    CHECK(h.GetConquestDistress() == 0.0);
    CHECK(h.GetEmpireDist() == 0.0);
    CHECK(h.GetEnemyAction() == 0.0);
    CHECK(h.GetPeace() == 0.0);
    CHECK(h.GetWorkday() == 0.0);
    CHECK(h.GetWages() == 0.0);
    CHECK(h.GetRations() == 0.0);
    CHECK(h.GetMartialLaw() == 0.0);
    CHECK(h.GetPopEntertainment() == 0.0);
    CHECK(h.GetImprovement() == 0.0);
    CHECK(h.GetWonders() == 0.0);
    CHECK(h.GetCrime() == 0.0);
    CHECK(h.GetTooManyCities() == 0.0);
}

TEST_CASE_FIXTURE(HappyFixture, "Happy tracker is allocated")
{
    Happy h;

    CHECK(h.GetHappyTracker() != nullptr);
}

TEST_CASE_FIXTURE(HappyFixture, "Happy timer add and remove")
{
    Happy h;

    h.AddTimer(5, 1.0, HAPPY_REASON_CITY_SIZE);
    h.AddTimer(3, 2.0, HAPPY_REASON_POLLUTION);
    h.AddTimer(7, -1.0, HAPPY_REASON_CITY_SIZE);

    // Remove all CITY_SIZE timers
    h.RemoveTimerReason(HAPPY_REASON_CITY_SIZE);

    // Clear remaining
    h.ClearTimedChanges();
}

TEST_CASE_FIXTURE(HappyFixture, "Happy cost to capitol can be set")
{
    Happy h;

    CHECK(h.GetCostToCapitol() == 0);
    h.SetCostToCapitol(42);
    CHECK(h.GetCostToCapitol() == 42);
}

TEST_CASE_FIXTURE(HappyFixture, "Happy IsVeryHappy uses real ConstDB threshold")
{
    Happy h;

    const ConstRecord *rec = g_theConstDB->Get(0);
    REQUIRE(rec != nullptr);

    double threshold = rec->GetVeryHappyThreshold();

    // Default happiness is 0, which is below threshold
    CHECK(h.IsVeryHappy() == (0.0 >= threshold));
}

TEST_CASE_FIXTURE(HappyFixture, "Happy ShouldRevolt uses real ConstDB revolution level")
{
    Happy h;

    const ConstRecord *rec = g_theConstDB->Get(0);
    REQUIRE(rec != nullptr);

    double revoltLevel = rec->GetRevolutionLevel();

    // Default happiness is 0; with no incite bonus, revolts if 0 < revoltLevel
    CHECK(h.ShouldRevolt(0) == (0.0 < revoltLevel));
}

TEST_CASE_FIXTURE(HappyFixture, "Happy conquest distress can be reset")
{
    Happy h;

    h.ResetConquestDistress(5.0);
    CHECK(h.GetConquestDistress() == 5.0);

    h.ResetConquestDistress(0.0);
    CHECK(h.GetConquestDistress() == 0.0);
}

TEST_CASE_FIXTURE(HappyFixture, "Happy IsVeryHappy reflects state transitions")
{
    Happy h;

    const ConstRecord *rec = g_theConstDB->Get(0);
    REQUIRE(rec != nullptr);

    double threshold = rec->GetVeryHappyThreshold();

    // Default: happiness is 0
    CHECK(h.IsVeryHappy() == (0.0 >= threshold));

    // SetFullHappinessTurns forces happiness to 100
    h.SetFullHappinessTurns(1);
    CHECK(h.GetHappiness() == 100.0);
    CHECK(h.IsVeryHappy() == (100.0 >= threshold));

    // ForceRevolt drops happiness to revolutionLevel - 10
    h.ForceRevolt();
    double revoltLevel = rec->GetRevolutionLevel();
    CHECK(h.GetHappiness() == revoltLevel - 10.0);
    CHECK(h.IsVeryHappy() == ((revoltLevel - 10.0) >= threshold));
}

TEST_CASE_FIXTURE(HappyFixture, "Happy ShouldRevolt reflects state transitions")
{
    Happy h;

    const ConstRecord *rec = g_theConstDB->Get(0);
    REQUIRE(rec != nullptr);

    double revoltLevel = rec->GetRevolutionLevel();

    // Default happiness is 0
    CHECK(h.ShouldRevolt(0) == (0.0 < revoltLevel));

    // ForceRevolt sets happiness to revoltLevel - 10, so should revolt
    h.ForceRevolt();
    CHECK(h.ShouldRevolt(0) == true);
    CHECK(h.GetHappiness() == revoltLevel - 10.0);

    // SetFullHappinessTurns sets happiness to 100, should not revolt
    h.SetFullHappinessTurns(1);
    CHECK(h.ShouldRevolt(0) == false);
    CHECK(h.GetHappiness() == 100.0);

    // Boundary with incite bonus: 100 < revoltLevel + 100 is always false
    CHECK(h.ShouldRevolt(100) == (100.0 < revoltLevel + 100.0));
}

TEST_CASE_FIXTURE(HappyFixture, "Happy Copy preserves scalar state")
{
    Happy src;
    src.ResetConquestDistress(7.5);
    src.SetCostToCapitol(42);

    Happy dst;
    dst.Copy(&src);

    CHECK(dst.GetConquestDistress() == 7.5);
    CHECK(dst.GetCostToCapitol() == 42);
    CHECK(dst.GetHappiness() == src.GetHappiness());
}

TEST_CASE_FIXTURE(HappyFixture, "Happy default-constructed dist-to-capitol and cost-to-capitol are zero")
{
    Happy h;

    CHECK(h.GetDistToCapitol() == 0.0);
    CHECK(h.GetCostToCapitol() == 0);
}
