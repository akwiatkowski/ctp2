//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Game event argument list
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
// - Improved slic event debugging. (7-Nov-2007 Martin Gühmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/events/GameEventArgList.h"

#include <algorithm>
#include "gs/gameobj/Army.h"
#include "robot/aibackdoor/civarchive.h"
#include "gs/events/GameEventArgument.h"
#include "ctp/ctp2_utils/pointerlist.h"
#include "gs/world/MapPoint.h"
#include "gs/gameobj/TerrImprove.h"
#include "gs/gameobj/TradeRoute.h"
#include "gs/gameobj/Unit.h"
#include "gs/events/GameEventDescription.h"

GameEventArgList::GameEventArgList(va_list *vl, GAME_EVENT eventType)
{
	std::fill(m_argLists,
	          m_argLists + GEA_End,
	          (PointerList<GameEventArgument> *) nullptr
	         );

	char *argString = event_description(eventType).args;

	while(vl) {
		GAME_EVENT_ARGUMENT arg = va_arg(*vl, GAME_EVENT_ARGUMENT);
		Assert(arg > GEA_Null);
		Assert(arg <= GEA_End);
		if(arg <= GEA_Null || arg >= GEA_End)
			break;

		if(!m_argLists[arg]) {
			m_argLists[arg] = new PointerList<GameEventArgument>;
		}
		m_argLists[arg]->AddTail(new GameEventArgument(arg, vl, *argString == '$'));

		argString++;
		argString++;
	}
}


GameEventArgList::~GameEventArgList()
{
	for (auto & m_argList : m_argLists)
	{
		if (m_argList)
		{
			m_argList->DeleteAll();
			delete m_argList;
		}
	}
}


void GameEventArgList::Add(GameEventArgument *arg)
{
	Assert(arg->GetType() > GEA_Null);
	Assert(arg->GetType() < GEA_End);

	if(!m_argLists[arg->GetType()]) {
		m_argLists[arg->GetType()] = new PointerList<GameEventArgument>;
	}
	m_argLists[arg->GetType()]->AddTail(arg);
}


GameEventArgument *GameEventArgList::GetArg(GAME_EVENT_ARGUMENT argType,
											sint32 index) const
{
	Assert(argType > GEA_Null);
	Assert(argType < GEA_End);

	if(argType <= GEA_Null || argType >= GEA_End)
		return nullptr;

	if(!m_argLists[argType])
		return nullptr;

	sint32  i = 0;

	for
	(
	    PointerList<GameEventArgument>::Walker walk(m_argLists[argType]);
	    walk.IsValid();
	    walk.Next()
	)
	{
		if (i == index)
		{
			return walk.GetObj();
		}

		++i;
	}

	return nullptr;
}

bool GameEventArgList::TestArgs(GAME_EVENT type, GameEvent* event) const
{
	bool argsAreValid = true;
	for(sint32 i = GEA_Army; i < GEA_End; ++i)
	{
		// Test all arguments for validity, in report if necessary
		argsAreValid &= TestArgsOfType(type, (GAME_EVENT_ARGUMENT)i, event);
	}

	return argsAreValid;
}

bool GameEventArgList::TestArgsOfType(GAME_EVENT type, GAME_EVENT_ARGUMENT argType, GameEvent* event) const
{
	Assert(argType > GEA_Null);
	Assert(argType < GEA_End);

	if(argType <= GEA_Null || argType >= GEA_End)
	{
		// Per default invalid
		Assert(false);
		return false;
	}

	if(!m_argLists[argType])
		return true; // Per default valid

	bool valid = true;
	sint32 i = 0;
	for
	(
	    PointerList<GameEventArgument>::Walker walk(m_argLists[argType]);
	    walk.IsValid();
	    walk.Next()
	)
	{
		if(!walk.GetObj()->IsValid())
		{
			walk.GetObj()->NotifyArgIsInvalid(type, i, event);
			valid = false;
		}

		i++;
	}

	return valid;
}

sint32 GameEventArgList::GetArgCount(GAME_EVENT_ARGUMENT argType) const
{
	Assert(argType > GEA_Null);
	Assert(argType < GEA_End);

	if(argType <= GEA_Null || argType >= GEA_End)
		return 0;

	if(!m_argLists[argType])
		return 0;

	return m_argLists[argType]->GetCount();
}

bool GameEventArgList::GetCity(sint32 index, Unit &c) const
{
	GameEventArgument * arg = GetArg(GEA_City, index);

	return arg && arg->GetCity(c) && c.IsValid();
}

bool GameEventArgList::GetUnit(sint32 index, Unit &u) const
{
	GameEventArgument * arg = GetArg(GEA_Unit, index);

	return arg && arg->GetUnit(u) && u.IsValid();
}

bool GameEventArgList::GetArmy(sint32 index, Army &a) const
{
	GameEventArgument * arg = GetArg(GEA_Army, index);

	return arg && arg->GetArmy(a) && a.IsValid();
}

bool GameEventArgList::GetInt(sint32 index, sint32 &value) const
{
	GameEventArgument * arg = GetArg(GEA_Int, index);

	return arg && arg->GetInt(value);
}

bool GameEventArgList::GetPlayer(sint32 index, sint32 &player) const
{
	GameEventArgument * arg = GetArg(GEA_Player, index);

	return arg && arg->GetPlayer(player);
}

bool GameEventArgList::GetPos(sint32 index, MapPoint & pos) const
{
	GameEventArgument * arg = GetArg(GEA_MapPoint, index);

	return arg && arg->GetPos(pos); /// @todo Add && pos.IsValid()?
}

bool GameEventArgList::GetPath(sint32 index, Path *&path) const
{
	GameEventArgument * arg = GetArg(GEA_Path, index);

	return arg && arg->GetPath(path);
}

bool GameEventArgList::GetDirection(sint32 index, WORLD_DIRECTION &d) const
{
	GameEventArgument * arg = GetArg(GEA_Direction, index);

	return arg && arg->GetDirection(d);
}

bool GameEventArgList::GetAdvance(sint32 index, sint32 &a) const
{
	GameEventArgument * arg = GetArg(GEA_Advance, index);

	return arg && arg->GetAdvance(a);
}

bool GameEventArgList::GetWonder(sint32 index, sint32 &w) const
{
	GameEventArgument * arg = GetArg(GEA_Wonder, index);

	return arg && arg->GetWonder(w);
}

bool GameEventArgList::GetImprovement(sint32 index, TerrainImprovement &imp) const
{
	GameEventArgument * arg = GetArg(GEA_Improvement, index);

	return arg && arg->GetImprovement(imp) && imp.IsValid();
}

bool GameEventArgList::GetTradeRoute(sint32 index, TradeRoute &route) const
{
	GameEventArgument * arg =GetArg(GEA_TradeRoute, index);

	return arg && arg->GetTradeRoute(route) && route.IsValid();
}
