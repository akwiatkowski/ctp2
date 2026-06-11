//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Game event argument
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
// HAVE_PRAGMA_ONCE
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Improved slic event debugging. (7-Nov-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/events/GameEventArgument.h"
#include "ctp/ctp2_utils/civlog.h"

#include "gs/gameobj/Unit.h"
#include "gs/world/MapPoint.h"
#include "gs/gameobj/Army.h"
#include "robot/pathing/Path.h"
#include "gs/gameobj/Player.h"
#include "gs/gameobj/TerrImprove.h"
#include "gs/gameobj/TradeRoute.h"
#include "AdvanceRecord.h"       // g_theAdvanceDB
#include "WonderRecord.h"        // g_theWonderDB
#include "gs/database/profileDB.h"           // profiledb_Get()
#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicObject.h"
#include "gs/slic/SlicSegment.h"
#include "gs/slic/SlicFrame.h"
#include "gs/events/GameEventManager.h"    // gevmanager_Get()

GameEventArgument::GameEventArgument(GAME_EVENT_ARGUMENT type, va_list *vl, bool isAlwaysValid)
{
	Init(type, vl, isAlwaysValid);
}

GameEventArgument::GameEventArgument(GAME_EVENT_ARGUMENT type, ...)
{
	va_list vl;
	va_start(vl, type);
	Init(type, &vl);
	va_end(vl);
}

void GameEventArgument::Init(GAME_EVENT_ARGUMENT type, va_list *vl, bool isAlwaysValid)
{
	m_IsAlwaysValid = isAlwaysValid;
	m_type = type;
	Assert(type >= (GAME_EVENT_ARGUMENT)0);
	Assert(type < GEA_End);
	if(type < (GAME_EVENT_ARGUMENT)0 || type >= GEA_End)
		return;

	static Unit unit;
	static Army army;
	static MapPoint pos;
	static TerrainImprovement imp;
	static TradeRoute route;

	sint32 value;
	Path *path;

	switch(m_type) {
		case GEA_Army:
			army = va_arg(*vl, Army);
			m_data.m_id = army.m_id;
			break;
		case GEA_Unit:
			unit = va_arg(*vl, Unit);
			m_data.m_id = unit.m_id;
			break;
		case GEA_City:
			unit = va_arg(*vl, Unit);
			m_data.m_id = unit.m_id;
			break;
		case GEA_Gold:
			value = va_arg(*vl, sint32);
			m_data.m_value = value;
			break;
		case GEA_Path:
			path = va_arg(*vl, Path *);
			m_data.m_ptr = (void *)path;
			break;
		case GEA_MapPoint:
			pos = va_arg(*vl, MapPoint);
			m_data.m_pos.x = pos.x;
			m_data.m_pos.y = pos.y;

			break;

		case GEA_Player:
			value = va_arg(*vl, sint32);
			m_data.m_value = value;
			break;

		case GEA_Int:
			value = va_arg(*vl, sint32);
			m_data.m_value = value;
			break;
		case GEA_Direction:
			value = va_arg(*vl, WORLD_DIRECTION);
			m_data.m_value = value;
			break;

		case GEA_Wonder:
			value = va_arg(*vl, sint32);
			m_data.m_value = value;
			break;
		case GEA_Advance:
			value = va_arg(*vl, sint32);
			m_data.m_value = value;
			break;
		case GEA_Improvement:
			imp = va_arg(*vl, TerrainImprovement);
			m_data.m_id = imp.m_id;
			break;
		case GEA_TradeRoute:
			route = va_arg(*vl, TradeRoute);
			m_data.m_id = route.m_id;
			break;

		default:
			Assert(false);
	}
}

GameEventArgument::~GameEventArgument()
{
	Path *path;

	switch(m_type) {
		case GEA_Path:
			path = (Path *)m_data.m_ptr;
			delete path;
			break;
		default:
			break;
	}
}

bool GameEventArgument::GetCity(Unit &c) const
{
	if(m_type != GEA_City)
		return false;

	c.m_id = m_data.m_id;
	return true;
}

bool GameEventArgument::GetUnit(Unit &u) const
{
	if(m_type != GEA_Unit)
		return false;

	u.m_id = m_data.m_id;
	return true;
}

bool GameEventArgument::GetArmy(Army &a) const
{
	if(m_type != GEA_Army)
		return false;

	a.m_id = m_data.m_id;
	return true;
}

bool GameEventArgument::GetInt(sint32 &value) const
{
	switch(m_type) {
		case GEA_Gold:
		case GEA_Int:
			value = m_data.m_value;
			return true;
		default:
			return false;
	}
}

bool GameEventArgument::GetPos(MapPoint &pos) const
{
	if(m_type != GEA_MapPoint)
		return false;

	pos.x = m_data.m_pos.x;
	pos.y = m_data.m_pos.y;

	return true;
}

bool GameEventArgument::GetPath(Path *&path) const
{
	if(m_type != GEA_Path)
		return false;

	path = new Path((Path *)m_data.m_ptr);
	return true;
}

bool GameEventArgument::GetPlayer(sint32 &player) const
{
	if(m_type != GEA_Player)
		return false;

	player = m_data.m_value;

	if(player < 0 || player >= k_MAX_PLAYERS)
		return false;

	if(!player_Get(player))
		return false;

	return true;
}

bool GameEventArgument::GetDirection(WORLD_DIRECTION &d) const
{
	if(m_type != GEA_Direction)
		return false;

	d = (WORLD_DIRECTION)m_data.m_value;
	return true;
}

bool GameEventArgument::GetAdvance(sint32 &a) const
{
	if(m_type != GEA_Advance)
		return false;

	a = m_data.m_value;
	return true;
}

bool GameEventArgument::GetWonder(sint32 &w) const
{
	if(m_type != GEA_Wonder)
		return false;

	w = m_data.m_value;
	return true;
}

bool GameEventArgument::GetImprovement(TerrainImprovement &imp) const
{
	if(m_type != GEA_Improvement)
		return false;

	imp.m_id = m_data.m_id;
	return true;
}

bool GameEventArgument::GetTradeRoute(TradeRoute &route) const
{
	if(m_type != GEA_TradeRoute)
		return false;

	route.m_id = m_data.m_id;
	return true;
}

bool GameEventArgument::IsValid() const
{
	if(m_IsAlwaysValid)
	{
		return true;
	}

	switch(m_type)
	{
		case GEA_Army:
		{
			Army army(m_data.m_id);
			return army.IsValid();
		}
		case GEA_Unit:
		case GEA_City:
		{
			Unit unit(m_data.m_id);
			return unit.IsValid();
		}
		case GEA_Gold:
		case GEA_Int:
			return true; // All values should be valid
		case GEA_Path:
		//	Path* path = static_cast<Path*>(m_ptr);
		//	return path->IsValid(); // Does not exist for now lets assume that all paths are valid
			return true;
		case GEA_MapPoint:
		{
			MapPoint pos(m_data.m_pos.x, m_data.m_pos.y);
			return pos.IsValid();
		}
		case GEA_Player:
		{
			sint32 player = m_data.m_value;
			return player == -1 || (player >= 0 && player < k_MAX_PLAYERS && player_Get(player));
		}
		case GEA_Direction:
		{
			sint32 direction = m_data.m_value;
			return direction >= NORTH && direction < NOWHERE;
		}
		case GEA_Wonder:
		{
			sint32 wonder = m_data.m_value;
			return wonder >= 0 && wonder < g_theWonderDB->NumRecords();
		}
		case GEA_Advance:
		{
			sint32 advance = m_data.m_value;
			return advance >= 0 && advance < g_theAdvanceDB->NumRecords();
		}
		case GEA_Improvement:
		{
			TerrainImprovement imp(m_data.m_id);
			return imp.IsValid();
		}
		case GEA_TradeRoute:
		{
			TradeRoute route(m_data.m_id);
			return route.IsValid();
		}
		default:
			Assert(false);
	}

	return false;
}

void GameEventArgument::NotifyArgIsInvalid(GAME_EVENT type, sint32 argIndex, GameEvent* event) const
{
	// Always-on visibility: a dropped event used to vanish silently
	// (GEV_ERR_ArgsInvalid) unless a debug profile flag was set, making
	// "the order did nothing" bugs invisible. One line, always logged.
	civlog::Get("gevent")->warn(
	    "event {} dropped: arg #{} ({}) invalid (id {:#x}), added during {}",
	    GameEventManager::GetEventName(type), (int)argIndex,
	    GameEventManager::ArgToName(m_type), (uint32)m_data.m_id,
	    event ? GameEventManager::GetEventName(event->AddedDuring()) : "?");

	DPRINTF(k_DBG_GAMESTATE, ("Missing object id %lx\n", (uint32)m_data.m_id));

	if(profiledb_Get() && profiledb_Get()->IsDebugSlicEvents())
	{
		if(event != nullptr
		&& event->GetContextName() != nullptr)
		{
			char buf[1024];
			snprintf(buf, sizeof(buf), "Parameter #%i of type %s of event %s is invalid.\nThe event was called during execution of event %s.\nIt was called from object %s at line %i in file:\n%s\nPossible Explanation: Data became invlid during internal or slic code executation.",
			             argIndex,
			             GameEventManager::ArgToName(m_type),
			             GameEventManager::GetEventName(type),
			             GameEventManager::GetEventName(event->AddedDuring()),
			             event->GetContextName(),
			             event->GetLine(),
			             event->GetFile());

			c3errors_ErrorDialog("Slic Event Error", buf);
		}
		else if(slicengine_Get()->GetContext())
		{
			char buf[1024];
			snprintf(buf, sizeof(buf), "Parameter #%i of type %s of event %s is invalid, the argument is invalid at event call.\nThe event was called during execution of event %s\nThe event was called from object %s at line %i in file:\n%s\nThe argument was already invalid at event call time.",
			             argIndex,
			             GameEventManager::ArgToName(m_type),
			             GameEventManager::GetEventName(type),
			             GameEventManager::GetEventName(gevmanager_Get()->GetProcessingEvent()),
			             slicengine_Get()->GetContext()->GetFrame()->GetSlicSegment()->GetName(),
			             slicengine_Get()->GetContext()->GetFrame()->GetCurrentLine(),
			             slicengine_Get()->GetContext()->GetFrame()->GetSlicSegment()->GetFilename());

			c3errors_ErrorDialog("Slic Event Error", buf);
		}
		else
		{
			char buf[1024];

			if(event != nullptr)
			{
				snprintf(buf, sizeof(buf), "Parameter #%i of type %s of event %s is invalid.\nThe event was added during the event %s.\nIt was called from the executable and is a serious problem that needs to be fixed if it was not caused by slic interference.\nPossible reason for the problem: The data became invalid between event call and event execution.",
				             argIndex,
			             GameEventManager::ArgToName(m_type),
			             GameEventManager::GetEventName(type),
			             GameEventManager::GetEventName(event->AddedDuring()));
			}
			else
			{
				snprintf(buf, sizeof(buf), "Parameter #%i of type %s of event %s is invalid.The event was added during the event %s.\nIt was called from the executable and is a serious problem that needs to be fixed.\nThe data was already invalid at event call time.",
				             argIndex,
			             GameEventManager::ArgToName(m_type),
			             GameEventManager::GetEventName(type),
			             GameEventManager::GetEventName(gevmanager_Get()->GetProcessingEvent()));
			}

			c3errors_ErrorDialog("Slic Event Source Error", buf);
		}
	}
}
