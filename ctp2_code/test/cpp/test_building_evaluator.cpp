// test/cpp/test_building_evaluator.cpp
// Tests for BuildingEvaluator / WonderEvaluator (P6.A).
//
// Strategy:
//  - Reuse the real-DB loading pattern from test_buildqueue / test_citydata.
//  - For the diff-based percent math, exercise the underlying
//    buildingutil_GetXxxPercent functions directly with synthetic masks
//    (no CityData needed). This locks in the math approach the evaluator
//    uses internally.
//  - For the record-side fields (cost, upkeep, qualitative flags), evaluate
//    against a stub-zero CityData and check the fields independent of the
//    city's base yields.

#include "doctest.h"
#include "ctp/c3.h"

#include "gs/gameobj/BuildingEvaluator.h"
#include "gs/gameobj/citydata.h"
#include "gs/gameobj/buildingutil.h"
#include "gs/gameobj/player.h"
#include "gs/utility/safety.h"
#include "gs/world/World.h"
#include "gs/world/MapPoint.h"
#include "gs/fileio/CivPaths.h"
#include "gs/utility/gameinit.h"
#include "gs/database/profileDB.h"
#include "BuildingRecord.h"
#include "WonderRecord.h"
#include "ctp/civapp.h"

struct EvaluatorFixture
{
    static bool s_dbsLoaded;
    static CivApp *s_app;

    Player **stubPlayers = nullptr;

    EvaluatorFixture()
    {
        if (!s_dbsLoaded)
        {
            set_headless(true);

            CivPaths_InitCivPaths();
            gameinit_InitializeGameFiles();

            profiledb_Set(new ProfileDB());
            profiledb_Get()->Init(FALSE);

            s_app = new CivApp();
            s_app->InitializeAppDB();

            // Required for CityData ctor: civapp must be set so trampolines
            // (world_Set, player_arr_Set) route through it.
            civapp_Set(s_app);
            world_Set(new World(MapPoint(20, 20), false, false));

            s_dbsLoaded = true;
        }

        stubPlayers = new Player *[k_MAX_PLAYERS];
        for (int i = 0; i < k_MAX_PLAYERS; ++i)
        {
            stubPlayers[i] = nullptr;
        }
        player_arr_Set(stubPlayers);
    }

    ~EvaluatorFixture()
    {
        delete[] stubPlayers;
        player_arr_Set(nullptr);
    }
};

bool EvaluatorFixture::s_dbsLoaded = false;
CivApp *EvaluatorFixture::s_app = nullptr;

namespace
{
// Find a building index by predicate over the record. Returns -1 if no match.
template <typename Predicate>
sint32 FindBuilding(Predicate pred)
{
    if (!g_theBuildingDB) return -1;
    for (sint32 i = 0; i < g_theBuildingDB->NumRecords(); ++i)
    {
        const BuildingRecord *r = g_theBuildingDB->Get(i);
        if (r && pred(r)) return i;
    }
    return -1;
}
} // namespace

TEST_CASE_FIXTURE(EvaluatorFixture, "Real building DB loaded")
{
    REQUIRE(g_theBuildingDB != nullptr);
    CHECK(g_theBuildingDB->NumRecords() > 0);
}

TEST_CASE_FIXTURE(EvaluatorFixture,
                  "BuildingEvaluator returns ProductionCost and Upkeep from record")
{
    // Pick any building — index 0 is sufficient for round-tripping the
    // record-side fields.
    REQUIRE(g_theBuildingDB->NumRecords() > 0);
    const sint32 building_index = 0;
    const BuildingRecord *rec = g_theBuildingDB->Get(building_index);
    REQUIRE(rec != nullptr);

    CityData city(0, Unit(), MapPoint(0, 0));

    Ctp2::BuildingEvaluation eval =
        Ctp2::BuildingEvaluator::Evaluate(building_index, city);

    CHECK(eval.record_index == building_index);
    CHECK(eval.kind == Ctp2::EvaluationItemKind::Building);
    CHECK(eval.production_cost == rec->GetProductionCost());
    CHECK(eval.upkeep_gold_per_turn == rec->GetUpkeep());
    // Stub city has zero production → turns_to_build is the "unknown" sentinel.
    CHECK(eval.turns_to_build == 0);
}

TEST_CASE_FIXTURE(EvaluatorFixture,
                  "Percent-bonus diff math matches direct record field")
{
    // Find a building that has a ProductionPercent bonus. Verify that the
    // diff (baseline=empty mask vs mask with this bit) equals the building's
    // own GetProductionPercent value. This is the invariant the evaluator
    // relies on.
    const sint32 idx = FindBuilding([](const BuildingRecord *r) {
        double p;
        return r->GetProductionPercent(p) && p > 0.0;
    });

    if (idx < 0)
    {
        // Data-dependent: skip if no such building exists in the loaded DB.
        WARN("No building with ProductionPercent > 0 in DB; skipping diff check");
        return;
    }

    const BuildingRecord *rec = g_theBuildingDB->Get(idx);
    double own = 0.0;
    REQUIRE(rec->GetProductionPercent(own));

    const sint32 owner = 0;
    const uint64 bit = (static_cast<uint64>(1) << static_cast<uint64>(idx));

    double baseline = 0.0;
    double with_it  = 0.0;
    buildingutil_GetProductionPercent(0,   baseline, owner);
    buildingutil_GetProductionPercent(bit, with_it,  owner);

    // Tolerance for double arithmetic.
    CHECK(doctest::Approx(with_it - baseline).epsilon(0.001) == own);
}

TEST_CASE_FIXTURE(EvaluatorFixture,
                  "Qualitative flags surface in BuildingEvaluation")
{
    // Find a building with NoUnhappyPeople. The evaluator should produce a
    // qualitative_effects entry for it.
    const sint32 idx = FindBuilding([](const BuildingRecord *r) {
        return r->GetNoUnhappyPeople();
    });

    if (idx < 0)
    {
        WARN("No NoUnhappyPeople building in DB; skipping flag check");
        return;
    }

    CityData city(0, Unit(), MapPoint(0, 0));
    Ctp2::BuildingEvaluation eval =
        Ctp2::BuildingEvaluator::Evaluate(idx, city);

    bool found = false;
    for (const auto &e : eval.qualitative_effects)
    {
        if (e.find("unhappy") != std::string::npos
         || e.find("Unhappy") != std::string::npos)
        {
            found = true;
            break;
        }
    }
    CHECK(found);
}

TEST_CASE("FormatEvaluationTooltip produces lines for non-zero fields")
{
    Ctp2::BuildingEvaluation eval;
    eval.kind = Ctp2::EvaluationItemKind::Building;
    eval.production_cost = 200;
    eval.turns_to_build = 5;
    eval.upkeep_gold_per_turn = 3;
    eval.delta_production = 12;
    eval.delta_commerce = 4;
    eval.net_gold_per_turn = 1;
    eval.payback_turns = 200;
    eval.qualitative_effects.emplace_back("City walls");

    const std::string tip = Ctp2::FormatEvaluationTooltip(eval);

    CHECK(tip.find("Cost: 200")        != std::string::npos);
    CHECK(tip.find("5 turns")          != std::string::npos);
    CHECK(tip.find("Upkeep: 3")        != std::string::npos);
    CHECK(tip.find("Production: +12") != std::string::npos);
    CHECK(tip.find("Commerce: +4")    != std::string::npos);
    CHECK(tip.find("Pays back")        != std::string::npos);
    CHECK(tip.find("City walls")       != std::string::npos);
    // Zero deltas must not appear.
    CHECK(tip.find("Food")             == std::string::npos);
    CHECK(tip.find("Science")          == std::string::npos);
}

TEST_CASE("FormatEvaluationTooltip empty wonder falls back to civilopedia hint")
{
    Ctp2::BuildingEvaluation eval;
    eval.kind = Ctp2::EvaluationItemKind::Wonder;
    // All other fields zero.

    const std::string tip = Ctp2::FormatEvaluationTooltip(eval);
    CHECK(tip.find("civilopedia") != std::string::npos);
}

TEST_CASE_FIXTURE(EvaluatorFixture,
                  "WonderEvaluator returns wonder ProductionCost")
{
    REQUIRE(g_theWonderDB != nullptr);
    if (g_theWonderDB->NumRecords() == 0)
    {
        WARN("Wonder DB empty; skipping wonder evaluation check");
        return;
    }

    const sint32 wonder_index = 0;
    const WonderRecord *rec = g_theWonderDB->Get(wonder_index);
    REQUIRE(rec != nullptr);

    CityData city(0, Unit(), MapPoint(0, 0));
    Ctp2::BuildingEvaluation eval =
        Ctp2::WonderEvaluator::Evaluate(wonder_index, city);

    CHECK(eval.record_index == wonder_index);
    CHECK(eval.kind == Ctp2::EvaluationItemKind::Wonder);
    CHECK(eval.production_cost == rec->GetProductionCost());
    CHECK(eval.upkeep_gold_per_turn == 0);  // wonders are upkeep-free in our model
}
