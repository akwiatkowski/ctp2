#pragma once

// P6: city-specific evaluation of a building or wonder for the EditQueue
// hover tooltip. Pure math against existing buildingutil_/wonderutil_/CityData
// accessors — does NOT execute SLIC. Wonder effects driven by SLIC scripts
// surface as opaque qualitative entries with civilopedia text only.

#include "c3types.h"
#include <string>
#include <vector>

class CityData;
class BuildingRecord;
class WonderRecord;

namespace Ctp2 {

enum class EvaluationItemKind { Building, Wonder };

struct BuildingEvaluation
{
    EvaluationItemKind kind = EvaluationItemKind::Building;
    sint32 record_index = -1;

    // Cost side.
    sint32 production_cost = 0;
    sint32 turns_to_build = 0;       // ceil(cost / current_production); 0 if production==0
    sint32 upkeep_gold_per_turn = 0;

    // Yield deltas applied to THIS city's current base.
    // Percent fields are added to existing bonus coefficients, then converted
    // to per-turn deltas using the city's pre-bonus base values.
    sint32 delta_production = 0;
    sint32 delta_food = 0;
    sint32 delta_commerce = 0;       // gold yield (gross, before upkeep)
    sint32 delta_science = 0;
    sint32 delta_happy = 0;

    // Net gold/turn after subtracting upkeep.
    sint32 net_gold_per_turn = 0;

    // Payback turns for a positive net_gold building: ceil(cost / net_gold).
    // -1 when not applicable (net_gold <= 0 or no commerce contribution).
    sint32 payback_turns = -1;

    // Localized labels for non-numeric effects (e.g. "Protects from nukes",
    // "Prevents conversion", "+50% city defense"). Resolved via stringdb.
    std::vector<std::string> qualitative_effects;

    // For greyed-out / not-yet-buildable items: list of unmet prerequisites
    // (localized).
    std::vector<std::string> prereqs_missing;
};

class BuildingEvaluator
{
public:
    // city must not be null. record_index is BuildingDB row.
    static BuildingEvaluation Evaluate(sint32 building_record_index,
                                       const CityData& city);
};

class WonderEvaluator
{
public:
    static BuildingEvaluation Evaluate(sint32 wonder_record_index,
                                       const CityData& city);
};

// Plain-text tooltip formatter. Compatible with aui_TipWindow::SetTipText.
// HyperText markup variant can be added later if c3_HyperTipWindow becomes
// the surface.
std::string FormatEvaluationTooltip(const BuildingEvaluation& eval);

} // namespace Ctp2
