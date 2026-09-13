//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header file
// Description  : declarations for the scheduler_types class
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
// - None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Removed MSVC specific code.
// - Standardised imports.
//
//----------------------------------------------------------------------------

#ifndef __SCHEDULER_TYPES_H__
#define __SCHEDULER_TYPES_H__ 1

#include <list>       // std::list
#include <memory>     // std::unique_ptr
#include <utility>    // std::pair

#include "ai/strategy/scheduler/Goal_And_Squad_Types.h"

enum GOAL_RESULT
{
    GOAL_FAILED,
    GOAL_NEEDS_TRANSPORT,
    GOAL_ALREADY_MOVED,
    GOAL_IN_PROGRESS,
    GOAL_COMPLETE
};

typedef sint32				Utility;
typedef sint32				PLAYER_INDEX;

class Agent;
class Squad_Strength;
class Goal;
class Plan;

typedef Agent*			Agent_ptr;
typedef Goal*			Goal_ptr;
typedef Plan*			Plan_ptr;

// Sorted_Goal_Entry is the owning handle for a goal: it only appears in
// Scheduler::m_goals_of_type. Everything else (Scheduler::m_goals,
// Goal::m_agents, Agent::m_goal, Plan::m_the_agent) borrows raw pointers.
typedef std::pair<Utility, std::unique_ptr<Goal> > Sorted_Goal_Entry;
typedef std::pair<double, Agent_ptr>               Sorted_Agent_ptr;

template<class _T1, class _T2>
bool operator <
(
    const Sorted_Goal_Entry& _X,
    const Sorted_Goal_Entry& _Y
)
{
	return (_X.first < _Y.first);
};

typedef std::list<Agent_ptr>             Agent_List;        // non-owning references
typedef std::list<std::unique_ptr<Agent>> Agent_Owning_List; // sole owner: Scheduler::m_agents
typedef std::list<Plan>                  Plan_List;

#endif //__SCHEDULER_TYPES_H__
