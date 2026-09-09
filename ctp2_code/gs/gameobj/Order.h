#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __ORDER_H__
#define __ORDER_H__

#include <string>

class Order;

enum ORDER_RESULT {
	ORDER_RESULT_ILLEGAL,
	ORDER_RESULT_FAILED,
	ORDER_RESULT_SUCCEEDED,
	ORDER_RESULT_INCOMPLETE,
	ORDER_RESULT_SUCCEEDED_INCOMPLETE,
};

#include "gs/gameobj/Unit.h"       // UNIT_ORDER_TYPE

struct OrderInfo {
	UNIT_ORDER_TYPE m_type;
	std::string m_name;
	sint32 m_goldCost;
	sint32 m_moveCost;
	sint32 m_failSound;
	sint32 m_workSound;
	void *m_userData;
};

// Order-info table accessors.  The actual storage is file-static in
// Order.cpp.  The table is read-only after static init — these
// accessors are safe to call from any thread once the program is
// past startup (future multi-threading hardening can add `const`-only
// guarantees here).
OrderInfo const & orderinfo_Get(sint32 idx);
sint32           orderinfo_Num();
sint32           orderinfo_MapAt(sint32 unitOrder);

#include "gs/events/GameEventDescription.h"
#include "gs/world/MapPoint.h"

#include <nlohmann/json.hpp>

class GameEventArgList;
class OrderRecord;
class Path;

class Order {
	// JSON bridge — mirrors Order::Serialize at Order.cpp:146.
	// Captures the 5 scalars + m_point (MapPoint).  OMITS m_path
	// (Path *) and m_gameEventArgs (GameEventArgList *) — both
	// pointer-typed sub-objects need their own bridges (Phase E-2
	// or later).  Also omits m_index (pool bookkeeping).
	friend void to_json(nlohmann::json &j, Order const &o);
	friend void from_json(nlohmann::json const &j, Order &o);

public:
	UNIT_ORDER_TYPE m_order;
	Path *m_path;
	sint32 m_round;
	MapPoint m_point;
	sint32 m_argument;
	sint32 m_index;

	GAME_EVENT m_eventType;
	GameEventArgList *m_gameEventArgs;

	Order(UNIT_ORDER_TYPE order, Path *path, const MapPoint &point,
		  sint32 argument, sint32 currentRound);
	Order()
	{
		m_order = UNIT_ORDER_NONE;
		m_path = nullptr;
		m_round = -1;
		m_argument = 0;
		// m_index is intentionally NOT initialised here — Order::operator
		// new (Order.cpp:209) has already populated it with the pool
		// slot returned by g_theOrderPond->Get_Next_Pointer. Writing -1
		// here would clobber that and corrupt Pool bookkeeping at delete
		// time (caught 2026-06-03 by Phase 1j determinism test —
		// json_save uses `new Order` and the prior default-ctor wipe
		// caused Pool::Release_Pointer(-1) at first turn after load).
		m_gameEventArgs = nullptr;
		// Uninitialised m_eventType holds an indeterminate bit pattern;
		// reading it (e.g. serialising a default-constructed Order) is an
		// invalid enum load and aborts under UBSan halt_on_error. GEV_MAX
		// is the "no event" sentinel, matching the parameterized ctor.
		m_eventType = GEV_MAX;
	}

	~Order();

	void *operator new(size_t size);
	void operator delete (void *ptr);

	static bool IsSpecialAttack(UNIT_ORDER_TYPE order);
	// Parameter is plain sint32 (not UNIT_ORDER_TYPE) so out-of-range
	// sentinels like -1 don't trip UBSan via an enum-load of a bit
	// pattern with no declared constant.  Range-checked inside.
	static GAME_EVENT OrderToEvent(sint32 order);
	static void AssociateEventsWithOrders();

	static sint32 GetCursor(OrderRecord *order);          // cursor id (CURSORINDEX enum value, cast at call site)
	static sint32 GetInvalidCursor(OrderRecord *order);   // cursor id (CURSORINDEX enum value, cast at call site)
};
#endif
