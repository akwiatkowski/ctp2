// Persistent scheduler graph. Indices preserve identity and ordering without
// putting process addresses into saves; matches and committed agents share nodes.
#include "ctp/c3.h"
#include "gs/core/game.h" // Ctp2::Game::GetActive guard in SaveAiHistory
#include "ai/strategy/scheduler/Scheduler.h"
#include "ai/strategy/agents/agent.h"
#include "gs/fileio/json_save.h"
#include "GoalRecord.h"
#include "ai/mapanalysis/settlemap.h"
#include "ai/mapanalysis/mapanalysis.h"
#include "gs/world/World.h"
#include <memory>
#include <cmath>
#include <limits>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

void to_json(nlohmann::json &j, Scheduler const &s)
{
    using Json = nlohmann::json;
    auto strength = [](Squad_Strength const &v) {
        return Json{{"attack_str", v.m_attack_str},
            {"defense_str", v.m_defense_str},
            {"ranged_str", v.m_ranged_str},
            {"land_bombard_str", v.m_land_bombard_str},
            {"water_bombard_str", v.m_water_bombard_str},
            {"air_bombard_str", v.m_air_bombard_str},
            {"value", v.m_value},
            {"agent_count", v.m_agent_count},
            {"transport", v.m_transport},
            {"defenders", v.m_defenders},
            {"ranged", v.m_ranged}};
    };
    std::unordered_map<Agent const *, size_t> agentIds;
    std::unordered_map<Goal const *, size_t> goalIds;
    auto reference = [](auto const &ids, auto pointer, char const *context) {
        auto it = ids.find(pointer);
        if (it == ids.end()) throw std::runtime_error(context);
        return it->second;
    };
    std::vector<Goal const *> goals;
    for (auto const &goal : s.m_generic_goals) {
        goalIds.emplace(goal.get(), goals.size());
        goals.push_back(goal.get());
    }
    for (auto const &type : s.m_goals_of_type)
        for (auto const &entry : type) {
            goalIds.emplace(entry.second.get(), goals.size());
            goals.push_back(entry.second.get());
        }
    j = {{"player", s.m_playerId}, {"needed_strength", strength(s.m_neededAgentStrength)},
         {"generic_count", s.m_generic_goals.size()}, {"agents", Json::array()},
         {"goals", Json::array()}, {"typed_goals", Json::array()},
         {"active_goals", Json::array()}};
    for (auto const &ptr : s.m_agents) {
        agentIds.emplace(ptr.get(), agentIds.size());
        auto const &a = *ptr;
        Json item{{"squad_strength", strength(a.m_squad_strength)},
            {"army", a.m_army},
            {"playerId", a.m_playerId},
            {"targetOrder", a.m_targetOrder},
            {"targetPos", a.m_targetPos},
            {"squad_class", a.m_squad_class},
            {"agent_type", a.m_agent_type},
            {"can_be_executed", a.m_can_be_executed},
            {"detached", a.m_detached},
            {"neededForGarrison", a.m_neededForGarrison}};
        item["goal"] = a.m_goal ? Json(reference(goalIds, a.m_goal, "scheduler agent refers to an unowned goal")) : Json(nullptr);
        j["agents"].push_back(std::move(item));
    }
    for (auto ptr : goals) {
        auto const &g = *ptr;
        Json item{{"current_needed_strength", strength(g.m_current_needed_strength)},
            {"current_attacking_strength", strength(g.m_current_attacking_strength)},
            {"playerId", g.m_playerId},
            {"raw_priority", g.m_raw_priority},
            {"combinedUtility", g.m_combinedUtility},
            {"target_pos", g.m_target_pos},
            {"target_city", g.m_target_city},
            {"target_army", g.m_target_army},
            {"sub_task", g.m_sub_task},
            {"goal_type", g.m_goal_type},
            {"needs_sorting", g.m_needs_sorting}};
        item["agents"] = Json::array();
        for (auto a : g.m_agents) item["agents"].push_back(reference(agentIds, a, "scheduler goal commits an unowned agent"));
        item["matches"] = Json::array();
        for (auto const &p : g.m_matches)
            item["matches"].push_back(Json{{"agent", reference(agentIds, p.m_the_agent, "scheduler match refers to an unowned agent")}, {"matching_value", p.m_matching_value},
            {"needs_cargo", p.m_needs_cargo},
            {"cannot_be_used", p.m_cannot_be_used},
            {"needs_transporter", p.m_needs_transporter}});
        j["goals"].push_back(std::move(item));
    }
    for (auto const &type : s.m_goals_of_type) {
        Json entries = Json::array();
        for (auto const &entry : type)
            entries.push_back({entry.first, reference(goalIds, entry.second.get(), "scheduler type refers to an unowned goal")});
        j["typed_goals"].push_back(std::move(entries));
    }
    for (auto goal : s.m_goals) j["active_goals"].push_back(reference(goalIds, goal, "scheduler active list refers to an unowned goal"));
}

void from_json(nlohmann::json const &j, Scheduler &destination)
{
    using Json = nlohmann::json;
    auto require = [&](bool valid, char const *message) {
        if (!valid) throw Json::other_error::create(532, message, &j);
    };
    // Resource limits bound allocations even for valid JSON. A scheduler normally
    // has hundreds of nodes; these limits allow large maps without unbounded graphs.
    auto array = [&](Json const &value, size_t limit) {
        require(value.is_array() && value.size() <= limit, "invalid scheduler array");
    };
    auto index = [&](Json const &value, size_t limit) {
        require(value.is_number_unsigned() || value.is_number_integer(), "invalid scheduler reference");
        auto n = value.get<int64_t>();
        require(n >= 0 && static_cast<uint64_t>(n) < limit, "scheduler reference out of range");
        return static_cast<size_t>(n);
    };
    auto number = [&](Json const &value, auto &out) {
        using T = std::remove_reference_t<decltype(out)>;
        if constexpr (std::is_integral_v<T>) {
            require(value.is_number_integer(), "scheduler integer required");
            if (value.is_number_unsigned())
                require(value.get<uint64_t>() <= uint64_t(std::numeric_limits<T>::max()),
                        "scheduler integer overflow");
            else {
                auto n = value.get<int64_t>();
                require(n >= int64_t(std::numeric_limits<T>::lowest()) &&
                        n <= int64_t(std::numeric_limits<T>::max()), "scheduler integer overflow");
            }
        } else {
            require(value.is_number(), "scheduler number required");
            auto n = value.get<double>();
            require(std::isfinite(n) && n >= std::numeric_limits<T>::lowest() &&
                    n <= std::numeric_limits<T>::max(), "scheduler number overflow");
        }
        value.get_to(out);
    };
    auto position = [&](Json const &value, MapPoint &out) {
        value.get_to(out);
        require(world_Get() && out.x >= 0 && out.y >= 0 &&
                out.x < world_Get()->GetWidth() && out.y < world_Get()->GetHeight(),
                "scheduler target outside world");
    };
    auto strength = [&](Json const &v, Squad_Strength &out) {
        number(v.at("attack_str"), out.m_attack_str);
        number(v.at("defense_str"), out.m_defense_str);
        number(v.at("ranged_str"), out.m_ranged_str);
        number(v.at("land_bombard_str"), out.m_land_bombard_str);
        number(v.at("water_bombard_str"), out.m_water_bombard_str);
        number(v.at("air_bombard_str"), out.m_air_bombard_str);
        number(v.at("value"), out.m_value);
        number(v.at("agent_count"), out.m_agent_count);
        number(v.at("transport"), out.m_transport);
        number(v.at("defenders"), out.m_defenders);
        number(v.at("ranged"), out.m_ranged);
    };
    auto const &savedAgents = j.at("agents");
    auto const &savedGoals = j.at("goals");
    auto const &types = j.at("typed_goals");
    array(savedAgents, 65536);
    array(savedGoals, 262144);
    array(types, g_theGoalDB->NumRecords());
    require(j.at("generic_count") == 0 || j.at("generic_count") == g_theGoalDB->NumRecords(), "invalid generic goal count");
    int const genericCount = j.at("generic_count").get<int>();
    require(static_cast<size_t>(genericCount) <= savedGoals.size(), "missing generic goals");
    require(types.size() == static_cast<size_t>(genericCount), "missing goal type lists");
    require(!player_Get(destination.m_playerId) || genericCount == g_theGoalDB->NumRecords(), "empty scheduler for live player");
    require(j.at("player") == destination.m_playerId, "scheduler player mismatch");
    Scheduler s;
    s.Cleanup();
    s.m_playerId = destination.m_playerId;
    strength(j.at("needed_strength"), s.m_neededAgentStrength);
    std::vector<std::unique_ptr<Goal>> goals;
    std::vector<std::unique_ptr<Agent>> agents;
    for (size_t i = 0; i < savedGoals.size(); ++i) goals.push_back(std::make_unique<Goal>());
    for (auto const &v : savedAgents) {
        // The default constructor does not dereference the army: dead agents can
        // legitimately remain until Process_Agent_Changes on the next turn.
        auto ptr = std::unique_ptr<Agent>(new Agent());   // private ctor: make_unique can't name it
        auto &a = *ptr;
        strength(v.at("squad_strength"), a.m_squad_strength);
        v.at("army").get_to(a.m_army);
        number(v.at("playerId"), a.m_playerId);
        number(v.at("targetOrder"), a.m_targetOrder);
        position(v.at("targetPos"), a.m_targetPos);
        number(v.at("squad_class"), a.m_squad_class);
        number(v.at("agent_type"), a.m_agent_type);
        v.at("can_be_executed").get_to(a.m_can_be_executed);
        v.at("detached").get_to(a.m_detached);
        v.at("neededForGarrison").get_to(a.m_neededForGarrison);
        require(a.m_playerId == s.m_playerId, "agent player mismatch");
        if (!v.at("goal").is_null()) a.m_goal = goals[index(v.at("goal"), goals.size())].get();
        agents.push_back(std::move(ptr));
    }
    size_t matchCount = 0;
    for (size_t i = 0; i < goals.size(); ++i) {
        auto &g = *goals[i];
        auto const &v = savedGoals[i];
        strength(v.at("current_needed_strength"), g.m_current_needed_strength);
        strength(v.at("current_attacking_strength"), g.m_current_attacking_strength);
        number(v.at("playerId"), g.m_playerId);
        number(v.at("raw_priority"), g.m_raw_priority);
        number(v.at("combinedUtility"), g.m_combinedUtility);
        position(v.at("target_pos"), g.m_target_pos);
        v.at("target_city").get_to(g.m_target_city);
        v.at("target_army").get_to(g.m_target_army);
        g.m_sub_task = static_cast<SUB_TASK_TYPE>(index(v.at("sub_task"), SUB_TASK_UNGROUP + 1));
        g.m_goal_type = static_cast<GOAL_TYPE>(index(v.at("goal_type"), g_theGoalDB->NumRecords()));
        v.at("needs_sorting").get_to(g.m_needs_sorting);
        require(g.m_playerId == s.m_playerId, "goal player mismatch");
        require(g.m_goal_type >= 0 && g.m_goal_type < g_theGoalDB->NumRecords(), "invalid goal type");
        require(g.m_sub_task >= SUB_TASK_GOAL && g.m_sub_task <= SUB_TASK_UNGROUP, "invalid goal subtask");
        array(v.at("agents"), agents.size());
        std::unordered_set<size_t> committed;
        for (auto const &id : v.at("agents")) {
            auto n = index(id, agents.size());
            require(committed.insert(n).second, "duplicate committed agent");
            require(agents[n]->m_goal == &g, "inconsistent committed goal");
            g.m_agents.push_back(agents[n].get());
        }
        array(v.at("matches"), agents.size());
        matchCount += v.at("matches").size();
        require(matchCount <= 1048576, "too many scheduler matches");
        std::unordered_set<size_t> matched;
        for (auto const &match : v.at("matches")) {
            auto n = index(match.at("agent"), agents.size());
            require(matched.insert(n).second, "duplicate scheduler match");
            Plan p(agents[n].get(), false);
            number(match.at("matching_value"), p.m_matching_value);
            match.at("needs_cargo").get_to(p.m_needs_cargo);
            match.at("cannot_be_used").get_to(p.m_cannot_be_used);
            match.at("needs_transporter").get_to(p.m_needs_transporter);
            g.m_matches.push_back(p);
        }
    }
    // Validate ownership before moving any unique_ptr. Each non-generic goal
    // must occur in exactly one type list; active goals only borrow those nodes.
    std::unordered_set<size_t> owned;
    for (int i = 0; i < genericCount; ++i) {
        require(goals[i]->m_goal_type == i, "generic goal type mismatch");
        owned.insert(i);
    }
    for (size_t type = 0; type < types.size(); ++type) {
        array(types[type], goals.size());
        for (auto const &entry : types[type]) {
            require(entry.is_array() && entry.size() == 2, "invalid typed goal");
            auto n = index(entry[1], goals.size());
            require(owned.insert(n).second && goals[n]->m_goal_type == static_cast<int>(type), "invalid goal ownership");
            Utility utility;
            number(entry[0], utility);
        }
    }
    require(owned.size() == goals.size(), "unowned scheduler goal");
    array(j.at("active_goals"), goals.size());
    std::unordered_set<size_t> active;
    for (auto const &id : j.at("active_goals")) {
        auto n = index(id, goals.size());
        require(n >= static_cast<size_t>(genericCount) && active.insert(n).second, "invalid active goal");
        s.m_goals.push_back(goals[n].get());
    }
    for (auto &a : agents)
        s.m_agents.push_back(std::move(a));
    for (int i = 0; i < genericCount; ++i)
        s.m_generic_goals.push_back(std::move(goals[i]));
    s.m_goals_of_type.resize(types.size());
    for (size_t type = 0; type < types.size(); ++type)
        for (auto const &entry : types[type]) {
            auto n = index(entry[1], goals.size());
            s.m_goals_of_type[type].emplace_back(entry[0].get<Utility>(), std::move(goals[n]));
        }
    // Swap only after the complete graph is valid. The temporary then destroys
    // the old graph while all pools and the game-session trampoline are alive.
    destination.m_goals_of_type.swap(s.m_goals_of_type);
    destination.m_generic_goals.swap(s.m_generic_goals);
    destination.m_goals.swap(s.m_goals);
    destination.m_agents.swap(s.m_agents);
    destination.m_neededAgentStrength = s.m_neededAgentStrength;
}

// Settlement scores include city-growth penalties and rejected positions.
// They are history, not a pure cache of the current world.
void to_json(nlohmann::json &j, SettleMap const &s)
{
    j = nullptr;
    if (!world_Get() || s.m_settleValues.GetValuesCount() == 0) return;
    j = nlohmann::json::array();
    for (sint16 x = 0; x < world_Get()->GetWidth(); ++x)
        for (sint16 y = 0; y < world_Get()->GetHeight(); ++y)
            j.push_back({s.m_settleValues.GetGridValue(MapPoint(x, y)),
                         s.m_invalidCells.Get(x, y) != 0});
}

void from_json(nlohmann::json const &j, SettleMap &s)
{
    if (j.is_null()) { s.Cleanup(); return; }
    if (!world_Get() || !j.is_array()
        || j.size() != static_cast<size_t>(world_Get()->GetWidth()) * world_Get()->GetHeight())
        throw nlohmann::json::other_error::create(532, "invalid settlement map size", &j);
    for (auto const &cell : j)
        if (!cell.is_array() || cell.size() != 2 || !cell[0].is_number() || !cell[1].is_boolean())
            throw nlohmann::json::other_error::create(532, "invalid settlement map cell", &cell);
    s.m_settleValues.Clear();
    s.m_settleValues.Resize(world_Get()->GetWidth(), world_Get()->GetHeight(), 1);
    s.m_invalidCells.Resize(world_Get()->GetWidth(), world_Get()->GetHeight(), false);
    size_t i = 0;
    for (sint16 x = 0; x < world_Get()->GetWidth(); ++x)
        for (sint16 y = 0; y < world_Get()->GetHeight(); ++y) {
            s.m_settleValues.AddValue(MapPoint(x, y), j[i][0].get<double>());
            s.m_invalidCells.Set(x, y, j[i][1].get<bool>());
            ++i;
        }
}

// Empire bounds grow as cities and armies explore; recomputing from their
// current positions loses that history and changes distance bonuses for goals.
void to_json(nlohmann::json &j, MapAnalysis const &m)
{
    j = {{"centers", m.m_empireCenter}, {"bounds", nlohmann::json::array()}};
    for (auto const &r : m.m_empireBoundingRect) {
        if (!r.m_isValid) j["bounds"].push_back(nullptr);
        else j["bounds"].push_back(nlohmann::json{{"upperLeft", r.m_upperLeft},
            {"lowerRight", r.m_lowerRight},
            {"xWrapOk", r.m_xWrapOk},
            {"yWrapOk", r.m_yWrapOk},
            {"mapSize", r.m_mapSize},
            {"ul_x_wrap", r.m_ul_x_wrap},
            {"lr_x_wrap", r.m_lr_x_wrap},
            {"ul_y_wrap", r.m_ul_y_wrap},
            {"lr_y_wrap", r.m_lr_y_wrap}});
    }
}

void from_json(nlohmann::json const &j, MapAnalysis &m)
{
    auto const &centers = j.at("centers");
    auto const &bounds = j.at("bounds");
    // Sizes must match the live vectors, which CtpAi::Resize sized from the
    // post-load player slots. A mismatch means the game was initialized with
    // a different player count than the save was written with (e.g. headless
    // --load-game without the --players N the save side used — gameinit
    // inherits NumPlayers from the profile, not the save). Report both
    // sizes; guessing across shapes would silently re-bin empire history.
    if (!centers.is_array() || centers.size() != m.m_empireCenter.size()
        || !bounds.is_array() || bounds.size() != m.m_empireBoundingRect.size()) {
        std::string detail = "invalid empire bounds count: save centers="
            + std::to_string(centers.size()) + " bounds=" + std::to_string(bounds.size())
            + ", live centers=" + std::to_string(m.m_empireCenter.size())
            + " bounds=" + std::to_string(m.m_empireBoundingRect.size());
        throw nlohmann::json::other_error::create(532, detail, &j);
    }
    centers.get_to(m.m_empireCenter);
    for (size_t i = 0; i < bounds.size(); ++i) {
        auto const &v = bounds[i];
        auto &r = m.m_empireBoundingRect[i];
        r.m_isValid = !v.is_null();
        if (!r.m_isValid) continue;
        v.at("upperLeft").get_to(r.m_upperLeft);
        v.at("lowerRight").get_to(r.m_lowerRight);
        v.at("xWrapOk").get_to(r.m_xWrapOk);
        v.at("yWrapOk").get_to(r.m_yWrapOk);
        v.at("mapSize").get_to(r.m_mapSize);
        v.at("ul_x_wrap").get_to(r.m_ul_x_wrap);
        v.at("lr_x_wrap").get_to(r.m_lr_x_wrap);
        v.at("ul_y_wrap").get_to(r.m_ul_y_wrap);
        v.at("lr_y_wrap").get_to(r.m_lr_y_wrap);
        if (!world_Get() || r.m_mapSize.x != world_Get()->GetWidth() * 2
            || r.m_mapSize.y != world_Get()->GetHeight())
            throw nlohmann::json::other_error::create(532, "invalid empire bounds dimensions", &v);
    }
}


void json_save::SaveAiHistory(nlohmann::json &j)
{
    // Fixture-free unit tests call SaveJson with no live game (no CivApp,
    // no NewGame). Old statics always existed; now guard and emit empty
    // defaults so the header round-trip stays fixture-free. LoadJson
    // already tolerates missing keys.
    if (!Ctp2::Game::GetActive()) {
        j["empire_bounds"] = nlohmann::json::object();
        j["settle_map"] = nlohmann::json::object();
        j["schedulers"] = nlohmann::json::array();
        return;
    }
    j["empire_bounds"] = MapAnalysis::GetMapAnalysis();
    j["settle_map"] = SettleMap::Ref();
    j["schedulers"] = nlohmann::json::array();
    for (size_t i = 0; i < Scheduler::Count(); ++i)
        j["schedulers"].push_back(Scheduler::GetScheduler(static_cast<sint32>(i)));
}

void json_save::RestoreAiHistory(nlohmann::json const &j)
{
    if (j.contains("empire_bounds"))
        j["empire_bounds"].get_to(MapAnalysis::GetMapAnalysis());
    if (j.contains("settle_map"))
        j["settle_map"].get_to(SettleMap::Ref());
    else if (world_Get())
        SettleMap::Ref().Initialize(); // Older saves lack settlement history.
    if (j.contains("schedulers")) {
        auto const &schedulers = j["schedulers"];
        if (!schedulers.is_array() || schedulers.size() != Scheduler::Count())
            throw nlohmann::json::other_error::create(532, "invalid scheduler count", &schedulers);
        for (size_t i = 0; i < schedulers.size(); ++i)
            schedulers[i].get_to(Scheduler::GetScheduler(static_cast<sint32>(i)));
    }
}
