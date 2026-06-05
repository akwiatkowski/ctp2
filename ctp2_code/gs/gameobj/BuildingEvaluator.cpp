#include "gs/gameobj/BuildingEvaluator.h"

#include "ctp/c3.h"
#include "gs/gameobj/citydata.h"
#include "gs/gameobj/buildingutil.h"
#include "gs/gameobj/wonderutil.h"
#include "gs/newdb/CTPDatabase.h"
#include "BuildingRecord.h"
#include "WonderRecord.h"
#include "gs/gameobj/Player.h"

#include <algorithm>

// g_theBuildingDB / g_theWonderDB are declared in the generated
// BuildingRecord.h / WonderRecord.h headers (build dir).

namespace Ctp2 {

namespace {

inline uint64 shiftbit_local(sint32 index)
{
    return static_cast<uint64>(1) << static_cast<uint64>(index);
}

// Round up division for non-negative dividend and positive divisor.
inline sint32 ceil_div(sint32 num, sint32 den)
{
    if (den <= 0) return 0;
    return (num + den - 1) / den;
}

// Percent-bonus diff helpers. Pattern: re-run the canonical aggregator with
// the candidate bit ORed in, subtract baseline. Captures wonder modifiers
// (e.g. Brokerage on commerce) without re-implementing them.
double ProductionPercentDelta(uint64 base, sint32 candidate, sint32 owner)
{
    double b = 0.0, w = 0.0;
    buildingutil_GetProductionPercent(base,                            b, owner);
    buildingutil_GetProductionPercent(base | shiftbit_local(candidate), w, owner);
    return w - b;
}

double FoodPercentDelta(uint64 base, sint32 candidate, sint32 owner)
{
    double b = 0.0, w = 0.0;
    buildingutil_GetFoodPercent(base,                            b, owner);
    buildingutil_GetFoodPercent(base | shiftbit_local(candidate), w, owner);
    return w - b;
}

double CommercePercentDelta(uint64 base, sint32 candidate, sint32 owner)
{
    double b = 0.0, w = 0.0;
    buildingutil_GetCommercePercent(base,                            b, owner);
    buildingutil_GetCommercePercent(base | shiftbit_local(candidate), w, owner);
    return w - b;
}

double SciencePercentDelta(uint64 base, sint32 candidate, sint32 owner)
{
    double b = 0.0, w = 0.0;
    buildingutil_GetSciencePercent(base,                            b, owner);
    buildingutil_GetSciencePercent(base | shiftbit_local(candidate), w, owner);
    return w - b;
}

sint32 HappinessDelta(uint64 base, sint32 candidate, sint32 owner)
{
    sint32 b = 0, w = 0;
    buildingutil_GetHappinessIncrement(base,                            b, owner);
    buildingutil_GetHappinessIncrement(base | shiftbit_local(candidate), w, owner);
    return w - b;
}

double DefendersBonusDelta(uint64 base, sint32 candidate, sint32 owner)
{
    double b = 0.0, w = 0.0;
    buildingutil_GetDefendersBonus(base,                            b, owner);
    buildingutil_GetDefendersBonus(base | shiftbit_local(candidate), w, owner);
    return w - b;
}

// Curated qualitative-effect labels. English strings here are TODOs for P6.B.1
// (formatter pass) which will migrate them to stringdb format keys. Keeping
// them inline now keeps the calculator self-contained and testable.
void CollectQualitativeBuildingEffects(const BuildingRecord *rec,
                                       std::vector<std::string> &out)
{
    if (!rec) return;
    if (rec->GetCityWalls())          out.emplace_back("City walls");
    if (rec->GetForceField())         out.emplace_back("Force field");
    if (rec->GetProtectFromNukes())   out.emplace_back("Protects from nukes");
    if (rec->GetNoUnhappyPeople())    out.emplace_back("No unhappy citizens");
    double dummy = 0.0;
    if (rec->GetFoodVat(dummy))            out.emplace_back("Food vat");
    if (rec->GetAirport())                 out.emplace_back("Airport");
    if (rec->GetCapitol())                 out.emplace_back("Capitol");
    if (rec->GetCathedral())               out.emplace_back("Cathedral");
    if (rec->GetBrokerage())               out.emplace_back("Brokerage (+commerce from wonders)");
    if (rec->GetNuclearPlant())            out.emplace_back("Nuclear plant");
    if (rec->GetTelevision())              out.emplace_back("Television");
    if (rec->GetLowerCrime(dummy))         out.emplace_back("Lowers crime");
    if (rec->GetPreventConversion(dummy))  out.emplace_back("Prevents religious conversion");
    if (rec->GetPreventSlavery(dummy))     out.emplace_back("Prevents slave raids");
    if (rec->GetNoRushBuyPenalty())        out.emplace_back("No rush-buy penalty");
}

} // namespace

BuildingEvaluation BuildingEvaluator::Evaluate(sint32 building_index,
                                               const CityData& city)
{
    BuildingEvaluation eval;
    eval.kind = EvaluationItemKind::Building;
    eval.record_index = building_index;

    if (!g_theBuildingDB) return eval;
    const sint32 owner = city.GetOwner();
    const BuildingRecord *rec = buildingutil_Get(building_index, owner);
    if (!rec) return eval;

    // Cost side.
    eval.production_cost      = rec->GetProductionCost();
    eval.upkeep_gold_per_turn = rec->GetUpkeep();

    const sint32 city_prod = city.GetGrossCityProduction();
    eval.turns_to_build = (city_prod > 0)
        ? ceil_div(eval.production_cost, city_prod)
        : 0;

    // Yield deltas. Diff approach: re-run the canonical aggregator with the
    // candidate bit ORed in; subtract baseline. Automatically captures any
    // wonder modifier (e.g. Brokerage on commerce) and stays in sync with
    // future buildingutil_ changes.
    const uint64 base_mask = city.GetEffectiveBuildings();

    // If already built, deltas are zero by construction — that's a useful
    // signal for the tooltip ("(already built)") but we don't synthesize the
    // active contribution. Callers can detect via record_index in mask.
    const sint32 base_prod_pre = city.GetGrossCityProdBeforeBonuses();
    const sint32 base_food_pre = city.GetGrossCityFoodBeforeBonuses();
    const sint32 base_gold     = city.GetGrossCityGold();
    const sint32 base_science  = city.GetScience();

    const double dProdPct = ProductionPercentDelta(base_mask, building_index, owner);
    const double dFoodPct = FoodPercentDelta      (base_mask, building_index, owner);
    const double dGoldPct = CommercePercentDelta  (base_mask, building_index, owner);
    const double dSciPct  = SciencePercentDelta   (base_mask, building_index, owner);

    eval.delta_production = static_cast<sint32>(base_prod_pre * dProdPct / 100.0);
    eval.delta_food       = static_cast<sint32>(base_food_pre * dFoodPct / 100.0);
    eval.delta_commerce   = static_cast<sint32>(base_gold     * dGoldPct / 100.0);
    eval.delta_science    = static_cast<sint32>(base_science  * dSciPct  / 100.0);

    eval.delta_happy = HappinessDelta(base_mask, building_index, owner);

    // Net gold/turn and payback.
    eval.net_gold_per_turn = eval.delta_commerce - eval.upkeep_gold_per_turn;
    eval.payback_turns = (eval.net_gold_per_turn > 0 && eval.production_cost > 0)
        ? ceil_div(eval.production_cost, eval.net_gold_per_turn)
        : -1;

    // Defense bonus surfaces as a qualitative line ("+50% defense") rather
    // than a numeric yield; cleaner in the tooltip.
    const double dDef = DefendersBonusDelta(base_mask, building_index, owner);
    if (dDef > 0.0)
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "+%d%% city defense",
                 static_cast<int>(dDef * 100.0 + 0.5));
        eval.qualitative_effects.emplace_back(buf);
    }

    CollectQualitativeBuildingEffects(rec, eval.qualitative_effects);

    return eval;
}

BuildingEvaluation WonderEvaluator::Evaluate(sint32 wonder_index,
                                             const CityData& city)
{
    BuildingEvaluation eval;
    eval.kind = EvaluationItemKind::Wonder;
    eval.record_index = wonder_index;

    if (!g_theWonderDB) return eval;
    const WonderRecord *rec = g_theWonderDB->Get(wonder_index);
    if (!rec) return eval;

    eval.production_cost = rec->GetProductionCost();
    eval.upkeep_gold_per_turn = 0; // wonders typically have no upkeep
    const sint32 city_prod = city.GetGrossCityProduction();
    eval.turns_to_build = (city_prod > 0)
        ? ceil_div(eval.production_cost, city_prod)
        : 0;

    // Wonder gameplay effects are largely SLIC-driven. We deliberately do not
    // execute SLIC. Numeric effects that surface through wonderutil_* are
    // empire-wide rather than per-city, so we don't attempt to convert them
    // into city deltas here — the tooltip falls back to the wonder's
    // civilopedia description for qualitative information.
    //
    // If/when a wonder's effects are encoded in WonderRecord direct fields in
    // a future schema change, surface them here.

    return eval;
}

// ---------------------------------------------------------------------------
// Plain-text tooltip formatter (P6.B.1).
// Output is human-readable English at this stage; stringdb migration TODO.

namespace {

void append_line(std::string &out, const std::string &line)
{
    if (!out.empty()) out += '\n';
    out += line;
}

void append_kv(std::string &out, const char *label, sint32 value, const char *unit)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%s: %+d %s", label, static_cast<int>(value), unit);
    append_line(out, buf);
}

void append_kv_abs(std::string &out, const char *label, sint32 value, const char *unit)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%s: %d %s", label, static_cast<int>(value), unit);
    append_line(out, buf);
}

} // namespace

std::string FormatEvaluationTooltip(const BuildingEvaluation &eval)
{
    std::string out;

    // Cost / turns header.
    if (eval.production_cost > 0)
    {
        char buf[96];
        if (eval.turns_to_build > 0)
        {
            snprintf(buf, sizeof(buf), "Cost: %d (%d turns)",
                     static_cast<int>(eval.production_cost),
                     static_cast<int>(eval.turns_to_build));
        }
        else
        {
            snprintf(buf, sizeof(buf), "Cost: %d",
                     static_cast<int>(eval.production_cost));
        }
        append_line(out, buf);
    }

    if (eval.upkeep_gold_per_turn > 0)
    {
        append_kv_abs(out, "Upkeep", eval.upkeep_gold_per_turn, "gold/turn");
    }

    // Yield deltas (only those that are non-zero — keep the tooltip tight).
    if (eval.delta_production != 0) append_kv(out, "Production", eval.delta_production, "/turn");
    if (eval.delta_food       != 0) append_kv(out, "Food",       eval.delta_food,       "/turn");
    if (eval.delta_commerce   != 0) append_kv(out, "Commerce",   eval.delta_commerce,   "gold/turn");
    if (eval.delta_science    != 0) append_kv(out, "Science",    eval.delta_science,    "/turn");
    if (eval.delta_happy      != 0) append_kv(out, "Happiness",  eval.delta_happy,      "");

    // Payback / net gold summary.
    if (eval.upkeep_gold_per_turn > 0 || eval.delta_commerce > 0)
    {
        char buf[96];
        snprintf(buf, sizeof(buf), "Net gold: %+d/turn",
                 static_cast<int>(eval.net_gold_per_turn));
        append_line(out, buf);

        if (eval.payback_turns > 0)
        {
            char pbuf[64];
            snprintf(pbuf, sizeof(pbuf), "Pays back in %d turns",
                     static_cast<int>(eval.payback_turns));
            append_line(out, pbuf);
        }
    }

    // Qualitative effects.
    for (const auto &e : eval.qualitative_effects)
    {
        append_line(out, "- " + e);
    }

    if (eval.kind == EvaluationItemKind::Wonder && out.empty())
    {
        append_line(out, "(See civilopedia for wonder effects.)");
    }

    return out;
}

} // namespace Ctp2
