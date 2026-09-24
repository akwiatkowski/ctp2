//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Scheduler
// Id           : $Id$
//
//----------------------------------------------------------------------------
//
// Disclaimer
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
// This material has been developed at apolyton.net by the Apolyton CtP2
// Source Code Project. Contact the authors at ctp2source@apolyton.net.
//
//----------------------------------------------------------------------------
//
// Compiler flags
//
// _MSC_VER
// - Compiler version (for the Microsoft C++ compiler only)
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Marked MS version specific code.
// - Redesigned AI, so that the matching algorithm is now a greedy algorithm. (13-Aug-2008 Martin Gühmann)
// - Now the goals are used for the matching process, the goal match value
//   is the avarage match value of the matches needed for the goal.
// - Simplified the design the number of committed agents and number of
//   agents are now calculated inside the Match_Resources method. (21-Aug-2008 Martin Gühmann)
// - Fixed unit garrison assignment. (23-Jan-2009 Martin Gühmann)
//
//----------------------------------------------------------------------------

#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include "ai/strategy/scheduler/scheduler_types.h"

#include "ai/strategy/squads/squad_Strength.h"
#include "StrategyRecord.h"

#include <vector>
#include <utility>
#include <deque>

#include "ctp/ctp2_utils/c3debugstl.h"

#include "ai/strategy/goals/Goal.h"               // Needed here to instantaite std::greater<Goal_ptr> correctly

class GoalRecord;
class Scheduler;
class SchedulerRegistry;
class Army;

#include <nlohmann/json.hpp>
class Scheduler;

class Scheduler
{
    friend void to_json(nlohmann::json &j, Scheduler const &s);
    friend void from_json(nlohmann::json const &j, Scheduler &s);


public:

#ifdef _DEBUG

	typedef std::vector<sint16, dbgallocator<sint16> >                       Count_Vector;
	typedef std::list<GOAL_TYPE, dbgallocator<GOAL_TYPE> >                   Goal_Type_List;
	typedef std::list<Sorted_Goal_Entry, dbgallocator<Sorted_Goal_Entry> >   Sorted_Goal_List;
	typedef Sorted_Goal_List::iterator                                       Sorted_Goal_Iter;
	typedef Sorted_Goal_List::const_iterator                                 Sorted_Goal_Const_Iter;
	typedef std::list<Goal_ptr, dbgallocator<Goal_ptr> >                     Goal_List;
	typedef std::vector<std::unique_ptr<Goal>, dbgallocator<std::unique_ptr<Goal> > > Goal_Vector;
	typedef std::list<SQUAD_CLASS, dbgallocator<SQUAD_CLASS> >               Squad_Class_List;
	typedef std::vector<Sorted_Goal_List, dbgallocator<Sorted_Goal_List> >   Sorted_Goal_List_Vector;
	typedef std::vector<Sorted_Goal_List::iterator, dbgallocator<Sorted_Goal_List::iterator> > Sorted_Goal_List_Iter_Vector;
	typedef std::vector<Agent_List, dbgallocator<Agent_List> >               Agent_List_Vector;

	typedef std::list<Sorted_Agent_ptr, dbgallocator<Sorted_Agent_ptr> >     Sorted_Agent_List;
	typedef std::vector<Sorted_Agent_List, dbgallocator<Sorted_Agent_List> > Sorted_Agent_List_Vector;
	typedef Sorted_Agent_List::iterator                                      Sorted_Agent_Iter;

	typedef std::vector<Scheduler, dbgallocator<Scheduler> >                 Scheduler_Vector;

#else

	typedef std::vector<sint16>                                              Count_Vector;
	typedef std::list<GOAL_TYPE>                                             Goal_Type_List;

	typedef std::list<Sorted_Goal_Entry>                                     Sorted_Goal_List;
	typedef Sorted_Goal_List::iterator                                       Sorted_Goal_Iter;
	typedef Sorted_Goal_List::const_iterator                                 Sorted_Goal_Const_Iter;
	typedef std::list<Goal_ptr>                                              Goal_List;
	typedef std::vector<std::unique_ptr<Goal> >                              Goal_Vector;
	typedef std::list<SQUAD_CLASS>                                           Squad_Class_List;
	typedef std::vector<Sorted_Goal_List>                                    Sorted_Goal_List_Vector;
	typedef std::vector<Sorted_Goal_List::iterator>                          Sorted_Goal_List_Iter_Vector;
	typedef std::vector<Agent_List>                                          Agent_List_Vector;

	typedef std::list<Sorted_Agent_ptr>                                      Sorted_Agent_List;
	typedef std::vector<Sorted_Agent_List>                                   Sorted_Agent_List_Vector;
	typedef Sorted_Agent_List::iterator                                      Sorted_Agent_Iter;

	typedef std::vector<Scheduler>                                           Scheduler_Vector;
#endif

	// (removed) s_max_match_list_cycles was write-only; the live cycle
	// budget comes from ConstDB GetMaxMatchListCycles (ctpai.cpp).

	static void ResizeAll(const PLAYER_INDEX & newMaxPlayerId);
	static size_t Count();
	static Scheduler & GetScheduler(const sint32 & playerId);
	static void CleanupAll();

	static SchedulerRegistry & Schedulers();
	using Registry = SchedulerRegistry;


	void       SetContactCache         (sint32 player);
	bool CachedHasContactWithExceptSelf(sint32 player1, sint32 player2);

	void    SetIsNeutralRegardCache(sint32 player);
	bool CachedIsNeutralRegard     (sint32 player, sint32 opponent);

	void    SetIsAllyRegardCache(sint32 player);
	bool CachedIsAllyRegard     (sint32 player, sint32 ally);

	// Static read shims for non-scheduler callers (mapanalysis threat
	// grids, radarmap borders). These forward to the owning player's
	// per-instance cache, which Process_Goal_Changes refreshes each turn;
	// outside that window they fall back to live diplomacy lookups, exactly
	// as the old static cache did on a miss.
	static bool StaticCachedHasContactWithExceptSelf(sint32 player1, sint32 player2);
	static bool StaticCachedIsNeutralRegard(sint32 player, sint32 opponent);
	static bool StaticCachedIsAllyRegard(sint32 player, sint32 ally);

	Scheduler();


	Scheduler(const Scheduler &scheduler);


	~Scheduler();

	Scheduler& operator= (const Scheduler &scheduler);

	void Cleanup();

	void Initialize();

	void SetPlayerId(const PLAYER_INDEX &team_index);

	void Process_Agent_Changes();
	void Process_Goal_Changes();

	void Reset_Agent_Execution();

	void Sort_Goals();

	void Match_Resources(const bool move_armies);


	void Add_New_Goal(std::unique_ptr<Goal> new_goal);

	void Add_New_Agent(std::unique_ptr<Agent> new_agent);
	Agent_Owning_List::iterator Add_Agent(std::unique_ptr<Agent> agent);

	Sorted_Goal_Iter Remove_Goal(const Sorted_Goal_Iter & sorted_goal_iter);

	void Remove_Goals_Type(const GoalRecord *rec);


	Squad_Strength GetMostNeededStrength() const;

	sint32 GetValueUnsatisfiedGoals(const GOAL_TYPE & type) const;

	Goal_ptr GetHighestPriorityGoal(const GOAL_TYPE & type, const bool satisfied) const;

	sint16 CountGoalsOfType(const GOAL_TYPE & type) const;

	void DisbandObsoleteArmies(const sint16 max_count);

	bool Prioritize_Goals();

	bool Prune_Goals();

	void SetArmyDetachState(const Army & army, const bool detach);
	void Recompute_Goal_Strength();
	void Compute_Agent_Strength();
	void Rollback_Emptied_Transporters();
	void Sort_Goal_Matches_If_Necessary();
	void Assign_Garrison();
	void ResetTransport();

	/// Cross-turn "run another match cycle" flag. Single-threaded turn loop
	/// sets it from ArmyData and drains it in ctpai; accessor keeps the
	/// storage private to the registry owner.
	static bool NeedAnotherCycle();
	static void SetNeedAnotherCycle(bool needed);
	static void ClearNeedAnotherCycle();

protected:

	void Add_New_Matches_For_Goal
	(
	    const Goal_ptr & goal_iter,
	    const bool       update_match_value = true
	);

	void Add_New_Matches_For_Agent(const Agent_ptr & agent);

	void Remove_Matches_For_Goal( const Goal_ptr & goal );

	void Remove_Matches_For_Agent( const Agent_ptr & agnet );

	void Rollback_Matches_For_Goal(const Goal_ptr & goal_ptr);

	void Reprioritize_Goal(Goal_List::iterator &goal_iter);

	bool Add_Transport_Matches_For_Goal(const Goal_ptr & goal_ptr);

	GOAL_TYPE GetMaxEvalExec(const StrategyRecord::GoalElement *goal_element_ptr, sint16 & max_eval, sint16 & max_exec);

	bool GetArmyDetachState(const Army & army) const;

private:

	// (removed) s_theSchedulers now lives in Scheduler::Registry (Schedulers()).

	Sorted_Goal_List_Vector      m_goals_of_type;
	Agent_Owning_List            m_agents;
	PLAYER_INDEX                 m_playerId;
	Squad_Strength               m_neededAgentStrength;
	Goal_List                    m_goals;
	Goal_Vector                  m_generic_goals;

	sint32 m_contactCachedPlayer = -1;
	uint32 m_contactCache = 0;
	sint32 m_neutralRegardCachedPlayer = -1;
	uint32 m_neutralRegardCache = 0;
	sint32 m_allyRegardCachedPlayer = -1;
	uint32 m_allyRegardCache = 0;
};

// Explicit registry owning all per-player schedulers. Replaces the old
// s_theSchedulers static; Scheduler::Schedulers() forwards here (via the
// active Game) so existing callers keep working while lifetime is explicit,
// per-game, and independently testable.
class SchedulerRegistry {
public:
	void Resize(const PLAYER_INDEX & newMaxPlayerId);
	void Clear();
	Scheduler & Get(const sint32 & playerId);
	size_t Size() const { return m_schedulers.size(); }
private:
	Scheduler::Scheduler_Vector m_schedulers;
};

#endif
