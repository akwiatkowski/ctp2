//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Order handling
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
// - Separated the Settle event drom the Settle in City event. (19-Feb-2008 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/gameobj/Order.h"
#include "robot/pathing/Path.h"
#include "robot/aibackdoor/pool.h"
#include "gs/events/GameEventArgList.h"

#include <vector>
#include "ctp/ctp2_utils/c3debugstl.h"
#include "OrderRecord.h"
#include "gs/events/GameEventManager.h"

extern Pool<Order> *g_theOrderPond;

static OrderInfo g_orderInfo[] = {
	{UNIT_ORDER_NONE,                       "ORDER_NONE",                       0, 0, 0, 0, nullptr}, // 00
	{UNIT_ORDER_MOVE,                       "ORDER_MOVE",                       0, 0, 0, 0, nullptr}, // 01
	{UNIT_ORDER_PATROL,                     "ORDER_PATROL",                     0, 0, 0, 0, nullptr}, // 02
	{UNIT_ORDER_CIRCULAR_PATROL,            "ORDER_CIRCULAR_PATROL",            0, 0, 0, 0, nullptr}, // 03
	{UNIT_ORDER_ENTRENCH,                   "ORDER_ENTRENCH",                   0, 0, 0, 0, nullptr}, // 04
	{UNIT_ORDER_SLEEP,                      "ORDER_SLEEP",                      0, 0, 0, 0, nullptr}, // 05
	{UNIT_ORDER_DETRENCH,                   "ORDER_DETRENCH",                   0, 0, 0, 0, nullptr}, // 06
	{UNIT_ORDER_UNLOAD,                     "ORDER_UNLOAD",                     0, 0, 0, 0, nullptr}, // 07
	{UNIT_ORDER_MOVE_TO,                    "ORDER_MOVE_TO",                    0, 0, 0, 0, nullptr}, // 08
	{UNIT_ORDER_TELEPORT_TO,                "ORDER_TELEPORT_TO",                0, 0, 0, 0, nullptr}, // 09
	{UNIT_ORDER_EXPEL_TO,                   "ORDER_EXPEL_TO",                   0, 0, 0, 0, nullptr}, // 10
	{UNIT_ORDER_GROUP,                      "ORDER_GROUP",                      0, 0, 0, 0, nullptr}, // 11
	{UNIT_ORDER_UNGROUP,                    "ORDER_UNGROUP",                    0, 0, 0, 0, nullptr}, // 12
	{UNIT_ORDER_INVESTIGATE_CITY,           "ORDER_INVESTIGATE_CITY",           0, 0, 0, 0, nullptr}, // 13
	{UNIT_ORDER_NULLIFY_WALLS,              "ORDER_NULLIFY_WALLS",              0, 0, 0, 0, nullptr}, // 14
	{UNIT_ORDER_STEAL_TECHNOLOGY,           "ORDER_STEAL_TECHNOLOGY",           0, 0, 0, 0, nullptr}, // 15
	{UNIT_ORDER_INCITE_REVOLUTION,          "ORDER_INCITE_REVOLUTION",          0, 0, 0, 0, nullptr}, // 16
	{UNIT_ORDER_ASSASSINATE,                "ORDER_ASSASSINATE",                0, 0, 0, 0, nullptr}, // 17
	{UNIT_ORDER_INVESTIGATE_READINESS,      "ORDER_INVESTIGATE_READINESS",      0, 0, 0, 0, nullptr}, // 18
	{UNIT_ORDER_BOMBARD,                    "ORDER_BOMBARD",                    0, 0, 0, 0, nullptr}, // 19
	{UNIT_ORDER_SUE,                        "ORDER_SUE",                        0, 0, 0, 0, nullptr}, // 20
	{UNIT_ORDER_FRANCHISE,                  "ORDER_FRANCHISE",                  0, 0, 0, 0, nullptr}, // 21
	{UNIT_ORDER_SUE_FRANCHISE,              "ORDER_SUE_FRANCHISE",              0, 0, 0, 0, nullptr}, // 22
	{UNIT_ORDER_EXPEL,                      "ORDER_EXPEL",                      0, 0, 0, 0, nullptr}, // 23
	{UNIT_ORDER_ESTABLISH_EMBASSY,          "ORDER_ESTABLISH_EMBASSY",          0, 0, 0, 0, nullptr}, // 24
	{UNIT_ORDER_THROW_PARTY,                "ORDER_THROW_PARTY",                0, 0, 0, 0, nullptr}, // 25
	{UNIT_ORDER_CAUSE_UNHAPPINESS,          "ORDER_CAUSE_UNHAPPINESS",          0, 0, 0, 0, nullptr}, // 26
	{UNIT_ORDER_PLANT_NUKE,                 "ORDER_PLANT_NUKE",                 0, 0, 0, 0, nullptr}, // 27
	{UNIT_ORDER_SLAVE_RAID,                 "ORDER_SLAVE_RAID",                 0, 0, 0, 0, nullptr}, // 28
	{UNIT_ORDER_ENSLAVE_SETTLER,            "ORDER_ENSLAVE_SETTLER",            0, 0, 0, 0, nullptr}, // 29
	{UNIT_ORDER_UNDERGROUND_RAILWAY,        "ORDER_UNDERGROUND_RAILWAY",        0, 0, 0, 0, nullptr}, // 30
	{UNIT_ORDER_INCITE_UPRISING,            "ORDER_INCITE_UPRISING",            0, 0, 0, 0, nullptr}, // 31
	{UNIT_ORDER_BIO_INFECT,                 "ORDER_BIO_INFECT",                 0, 0, 0, 0, nullptr}, // 32
	{UNIT_ORDER_NANO_INFECT,                "ORDER_NANO_INFECT",                0, 0, 0, 0, nullptr}, // 33
	{UNIT_ORDER_CONVERT,                    "ORDER_CONVERT",                    0, 0, 0, 0, nullptr}, // 34
	{UNIT_ORDER_REFORM,                     "ORDER_REFORM",                     0, 0, 0, 0, nullptr}, // 35
	{UNIT_ORDER_INDULGENCE,                 "ORDER_INDULGENCE",                 0, 0, 0, 0, nullptr}, // 36
	{UNIT_ORDER_SOOTHSAY,                   "ORDER_SOOTHSAY",                   0, 0, 0, 0, nullptr}, // 37
	{UNIT_ORDER_CREATE_PARK,                "ORDER_CREATE_PARK",                0, 0, 0, 0, nullptr}, // 38
	{UNIT_ORDER_PILLAGE,                    "ORDER_PILLAGE",                    0, 0, 0, 0, nullptr}, // 39
	{UNIT_ORDER_INJOIN,                     "ORDER_INJOIN",                     0, 0, 0, 0, nullptr}, // 40
	{UNIT_ORDER_INTERCEPT_TRADE,            "ORDER_INTERCEPT_TRADE",            0, 0, 0, 0, nullptr}, // 41
	{UNIT_ORDER_PARADROP_MOVE,              "ORDER_PARADROP_MOVE",              0, 0, 0, 0, nullptr}, // 42
	{UNIT_ORDER_SET_UNLOAD_MOVEMENT_POINTS, "ORDER_SET_UNLOAD_MOVEMENT_POINTS", 0, 0, 0, 0, nullptr}, // 43
	{UNIT_ORDER_GROUP_UNIT,                 "ORDER_GROUP_UNIT",                 0, 0, 0, 0, nullptr}, // 44
	{UNIT_ORDER_DISBAND,                    "ORDER_DISBAND_ARMY",               0, 0, 0, 0, nullptr}, // 45
	{UNIT_ORDER_FINISH_ATTACK,              "ORDER_FINISH_ATTACK",              0, 0, 0, 0, nullptr}, // 46
	{UNIT_ORDER_UNLOAD_ONE_UNIT,            "ORDER_UNLOAD_ONE_UNIT",            0, 0, 0, 0, nullptr}, // 47
	{UNIT_ORDER_BOARD_TRANSPORT,            "ORDER_BOARD_TRANSPORT",            0, 0, 0, 0, nullptr}, // 48
	{UNIT_ORDER_WAKE_UP,                    "ORDER_WAKE_UP",                    0, 0, 0, 0, nullptr}, // 49
	{UNIT_ORDER_PILLAGE_UNCONDITIONALLY,    "ORDER_PILLAGE_UNCONDITIONALLY",    0, 0, 0, 0, nullptr}, // 50
	{UNIT_ORDER_MOVE_THEN_UNLOAD,           "ORDER_MOVE_THEN_UNLOAD",           0, 0, 0, 0, nullptr}, // 51
	{UNIT_ORDER_ADVERTISE,                  "ORDER_ADVERTISE",                  0, 0, 0, 0, nullptr}, // 52
	{UNIT_ORDER_INFORM_AI_CAPTURE_CITY,     "ORDER_INFORM_AI_CAPTURE_CITY",     0, 0, 0, 0, nullptr}, // 53
	{UNIT_ORDER_UNLOAD_SELECTED_STACK,      "ORDER_UNLOAD_SELECTED_STACK",      0, 0, 0, 0, nullptr}, // 54
	{UNIT_ORDER_ADD_EVENT,                  "ORDER_ADD_EVENT",                  0, 0, 0, 0, nullptr}, // 55
	{UNIT_ORDER_SETTLE,                     "ORDER_SETTLE",                     0, 0, 0, 0, nullptr}, // 56
	{UNIT_ORDER_LAUNCH,                     "ORDER_LAUNCH",                     0, 0, 0, 0, nullptr}, // 57
	{UNIT_ORDER_TARGET,                     "ORDER_TARGET",                     0, 0, 0, 0, nullptr}, // 58
	{UNIT_ORDER_CLEAR_TARGET,               "ORDER_CLEAR_TARGET",               0, 0, 0, 0, nullptr}, // 59
	{UNIT_ORDER_PLAGUE,                     "ORDER_PLAGUE",                     0, 0, 0, 0, nullptr}, // 60
	{UNIT_ORDER_VICTORY_MOVE,               "ORDER_VICTORY_MOVE",               0, 0, 0, 0, nullptr}, // 61
	{UNIT_ORDER_SETTLE_IN_CITY,             "ORDER_SETTLE_IN_CITY",             0, 0, 0, 0, nullptr}, // 62
	{UNIT_ORDER_EXPLORE,                    "ORDER_EXPLORE",                    0, 0, 0, 0, nullptr}  // 63
};

static sint32 g_numOrderInfo = sizeof(g_orderInfo) / sizeof(OrderInfo);
static sint32 g_orderInfoMap[UNIT_ORDER_MAX];

// Read-only accessors for the file-static order-info table.  Bound
// checks fold in here so call sites stop having to repeat the
// `if(index >= 0 && index < g_numOrderInfo)` pattern.
OrderInfo const & orderinfo_Get(sint32 idx)
{
    Assert(idx >= 0 && idx < g_numOrderInfo);
    return g_orderInfo[idx];
}

sint32 orderinfo_Num()
{
    return g_numOrderInfo;
}

sint32 orderinfo_MapAt(sint32 unitOrder)
{
    Assert(unitOrder >= 0 && unitOrder < UNIT_ORDER_MAX);
    return g_orderInfoMap[unitOrder];
}

Order::Order(UNIT_ORDER_TYPE order, Path *path, const MapPoint &point, sint32 arg, sint32 currentRound)
{
	m_order = order;
	m_path = path;
	m_round = currentRound;
	m_point = point;
	m_argument = arg;
	m_gameEventArgs = nullptr;
	m_eventType = GEV_MAX;
}

Order::~Order()
{
	if(m_path) {
		delete m_path;
		m_path = nullptr;
	}

	if(m_gameEventArgs) {
		delete m_gameEventArgs;
		m_gameEventArgs = nullptr;
	}
}

void *Order::operator new(size_t size)
{
	int index;
	Order *order = g_theOrderPond->Get_Next_Pointer(index);
	order->m_index = index;
	return order;
}

void Order::operator delete(void *ptr)
{
	if (!ptr)
		return;
	Order *order = (Order *)ptr;

	g_theOrderPond->Release_Pointer(order->m_index);
}

bool Order::IsSpecialAttack(UNIT_ORDER_TYPE order)
{
	switch(order) {
		case UNIT_ORDER_INVESTIGATE_CITY:
		case UNIT_ORDER_NULLIFY_WALLS:
		case UNIT_ORDER_STEAL_TECHNOLOGY:
		case UNIT_ORDER_INCITE_REVOLUTION:
		case UNIT_ORDER_ASSASSINATE:
		case UNIT_ORDER_INVESTIGATE_READINESS:
		case UNIT_ORDER_BOMBARD:
		case UNIT_ORDER_SUE:
		case UNIT_ORDER_FRANCHISE:
		case UNIT_ORDER_SUE_FRANCHISE:
		case UNIT_ORDER_EXPEL:
		case UNIT_ORDER_ESTABLISH_EMBASSY:
		case UNIT_ORDER_THROW_PARTY:
		case UNIT_ORDER_CAUSE_UNHAPPINESS:
		case UNIT_ORDER_PLANT_NUKE:
		case UNIT_ORDER_SLAVE_RAID:
		case UNIT_ORDER_ENSLAVE_SETTLER:
		case UNIT_ORDER_UNDERGROUND_RAILWAY:
		case UNIT_ORDER_INCITE_UPRISING:
		case UNIT_ORDER_BIO_INFECT:
		case UNIT_ORDER_NANO_INFECT:
		case UNIT_ORDER_CONVERT:
		case UNIT_ORDER_REFORM:
		case UNIT_ORDER_INDULGENCE:
		case UNIT_ORDER_SOOTHSAY:
		case UNIT_ORDER_CREATE_PARK:
		case UNIT_ORDER_PILLAGE:
		case UNIT_ORDER_INJOIN:
		case UNIT_ORDER_INTERCEPT_TRADE:
		case UNIT_ORDER_ADVERTISE:
		case UNIT_ORDER_SETTLE:
		case UNIT_ORDER_DISBAND:
		case UNIT_ORDER_UNLOAD:
		case UNIT_ORDER_LAUNCH:
		case UNIT_ORDER_TARGET:
		case UNIT_ORDER_CLEAR_TARGET:
			return true;
		default:
			return false;
	}
}

static GAME_EVENT s_orderToEventMap[UNIT_ORDER_MAX];

GAME_EVENT Order::OrderToEvent(UNIT_ORDER_TYPE order)
{
	if (order < 0 || order >= UNIT_ORDER_MAX)
		return GEV_MAX;
	return s_orderToEventMap[order];
}


void Order::AssociateEventsWithOrders()
{

	const char *event_name;
	for(auto & i : s_orderToEventMap) {
		i = GEV_MAX;
	}

	for (sint32 order_index = 0; order_index < g_numOrderInfo; order_index++)
	{
		sint32 dbIndex;
		if(g_theOrderDB->GetNamedItem(g_orderInfo[order_index].m_name, dbIndex)) {
			event_name = g_theOrderDB->Get(dbIndex)->GetEventName();
			if (strlen(event_name) > 0)
				s_orderToEventMap[g_orderInfo[order_index].m_type] = GameEventManager::GetEventIndex(event_name);
			else {
				s_orderToEventMap[g_orderInfo[order_index].m_type] = GEV_MAX;
			}

		}
	}
}

sint32 Order::GetCursor(OrderRecord *order)
{
	return order->GetCursor();
}

sint32 Order::GetInvalidCursor(OrderRecord *order)
{
	return order->GetInvalidCursor();
}
