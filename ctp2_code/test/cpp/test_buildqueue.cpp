// test/cpp/test_buildqueue.cpp
// Tests for BuildQueue — queue management, cost lookup against real DBs.

#include "doctest.h"
#include "ctp/c3.h"
#include "gs/gameobj/BldQue.h"
#include "gs/gameobj/CityData.h"
#include "gs/utility/Globals.h"
#include "gs/utility/safety.h"
#include "UnitRecord.h"
#include "BuildingRecord.h"
#include "WonderRecord.h"
#include "gs/fileio/CivPaths.h"
#include "gs/utility/gameinit.h"
#include "gs/database/profileDB.h"
#include "ctp/civapp.h"

struct BuildQueueFixture
{
    static bool s_dbsLoaded;
    static CivApp *s_app;

    Player **stubPlayers = nullptr;

    BuildQueueFixture()
    {
        if (!s_dbsLoaded)
        {
            g_headlessMode = true;

            fprintf(stderr, "[BuildQueueFixture] Loading databases...\n");
            CivPaths_InitCivPaths();
            gameinit_InitializeGameFiles();

            g_theProfileDB = new ProfileDB();
            g_theProfileDB->Init(FALSE);

            s_app = new CivApp();
            s_app->InitializeAppDB();

            fprintf(stderr, "[BuildQueueFixture] Databases loaded.\n");
            s_dbsLoaded = true;
        }

        stubPlayers = new Player *[k_MAX_PLAYERS];
        for (int i = 0; i < k_MAX_PLAYERS; ++i)
        {
            stubPlayers[i] = nullptr;
        }
        g_player = stubPlayers;
    }

    ~BuildQueueFixture()
    {
        delete[] stubPlayers;
        g_player = nullptr;
    }
};

bool BuildQueueFixture::s_dbsLoaded = false;
CivApp *BuildQueueFixture::s_app = nullptr;

TEST_CASE_FIXTURE(BuildQueueFixture, "BuildQueue starts empty")
{
    BuildQueue bq;
    bq.SetOwner(0);

    CHECK(bq.GetLen() == 0);
    CHECK(bq.GetHead() == nullptr);
}

TEST_CASE_FIXTURE(BuildQueueFixture, "BuildQueue can insert items via RawInsertTail")
{
    BuildQueue bq;
    bq.SetOwner(0);

    bq.RawInsertTail(k_GAME_OBJ_TYPE_UNIT, 0, 100);
    CHECK(bq.GetLen() == 1);
    REQUIRE(bq.GetHead() != nullptr);
    CHECK(bq.GetHead()->m_category == k_GAME_OBJ_TYPE_UNIT);
    CHECK(bq.GetHead()->m_type == 0);
    CHECK(bq.GetHead()->m_cost == 100);
}

TEST_CASE_FIXTURE(BuildQueueFixture, "BuildQueue can insert multiple items")
{
    BuildQueue bq;
    bq.SetOwner(0);

    bq.RawInsertTail(k_GAME_OBJ_TYPE_UNIT, 0, 10);
    bq.RawInsertTail(k_GAME_OBJ_TYPE_IMPROVEMENT, 1, 20);
    bq.RawInsertTail(k_GAME_OBJ_TYPE_WONDER, 2, 30);

    CHECK(bq.GetLen() == 3);

    BuildNode *node = bq.GetNodeByIndex(0);
    REQUIRE(node != nullptr);
    CHECK(node->m_category == k_GAME_OBJ_TYPE_UNIT);

    node = bq.GetNodeByIndex(1);
    REQUIRE(node != nullptr);
    CHECK(node->m_category == k_GAME_OBJ_TYPE_IMPROVEMENT);

    node = bq.GetNodeByIndex(2);
    REQUIRE(node != nullptr);
    CHECK(node->m_category == k_GAME_OBJ_TYPE_WONDER);
}

TEST_CASE_FIXTURE(BuildQueueFixture, "BuildQueue can remove items by index")
{
    BuildQueue bq;
    bq.SetOwner(0);

    bq.RawInsertTail(k_GAME_OBJ_TYPE_UNIT, 0, 10);
    bq.RawInsertTail(k_GAME_OBJ_TYPE_UNIT, 1, 20);
    bq.RawInsertTail(k_GAME_OBJ_TYPE_UNIT, 2, 30);

    CHECK(bq.GetLen() == 3);

    bq.RemoveNodeByIndex(1, CAUSE_REMOVE_BUILD_ITEM_MANUAL);
    CHECK(bq.GetLen() == 2);

    BuildNode *head = bq.GetHead();
    REQUIRE(head != nullptr);
    CHECK(head->m_type == 0);

    BuildNode *tail = bq.GetNodeByIndex(1);
    REQUIRE(tail != nullptr);
    CHECK(tail->m_type == 2);
}

TEST_CASE_FIXTURE(BuildQueueFixture, "BuildQueue Clear empties the queue")
{
    BuildQueue bq;
    bq.SetOwner(0);

    bq.RawInsertTail(k_GAME_OBJ_TYPE_UNIT, 0, 10);
    bq.RawInsertTail(k_GAME_OBJ_TYPE_IMPROVEMENT, 1, 20);

    CHECK(bq.GetLen() == 2);
    bq.Clear();
    CHECK(bq.GetLen() == 0);
    CHECK(bq.GetHead() == nullptr);
}

TEST_CASE_FIXTURE(BuildQueueFixture, "BuildQueue GetCost returns real unit cost from UnitDB")
{
    BuildQueue bq;
    bq.SetOwner(0);

    REQUIRE(g_theUnitDB->NumRecords() > 0);

    sint32 firstUnitCost = g_theUnitDB->Get(0)->GetShieldCost();
    CHECK(bq.GetCost(k_GAME_OBJ_TYPE_UNIT, 0) == firstUnitCost);
}

TEST_CASE_FIXTURE(BuildQueueFixture, "BuildQueue GetCost returns real wonder cost from WonderDB")
{
    BuildQueue bq;
    bq.SetOwner(0);

    REQUIRE(g_theWonderDB->NumRecords() > 0);

    sint32 firstWonderCost = g_theWonderDB->Get(0)->GetProductionCost();
    CHECK(bq.GetCost(k_GAME_OBJ_TYPE_WONDER, 0) == firstWonderCost);
}

TEST_CASE_FIXTURE(BuildQueueFixture, "BuildQueue IsItemInQueue finds inserted items")
{
    BuildQueue bq;
    bq.SetOwner(0);

    bq.RawInsertTail(k_GAME_OBJ_TYPE_UNIT, 5, 100);
    bq.RawInsertTail(k_GAME_OBJ_TYPE_IMPROVEMENT, 3, 200);

    CHECK(bq.IsItemInQueue(k_GAME_OBJ_TYPE_UNIT, 5) == true);
    CHECK(bq.IsItemInQueue(k_GAME_OBJ_TYPE_IMPROVEMENT, 3) == true);
    CHECK(bq.IsItemInQueue(k_GAME_OBJ_TYPE_UNIT, 99) == false);
    CHECK(bq.IsItemInQueue(k_GAME_OBJ_TYPE_WONDER, 5) == false);
}
