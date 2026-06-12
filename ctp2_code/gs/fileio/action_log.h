//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Action Log — engine-native universal action/event ledger.
//
//----------------------------------------------------------------------------
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
//----------------------------------------------------------------------------
//
// The Action Log is a passive, round-tripping ledger embedded in the JSON
// savegame.  It records every *meaningful* game action/outcome regardless of
// source (human UI, gateway/agent command, AI turn, slic script), because all
// four funnel through GameEventManager.  A gateway `build_city` is logged
// because it caused GEV_CreateCity — identical to a human founding a city.
//
// Two layers:
//   1. Carrier — the in-memory JSON array that persists across save/load
//      (this file).  A function-local-static store: no init-order issues, empty
//      by default, tolerates missing-on-load.
//   2. Tap     — a single generic GEV_PRI_Post callback registered across the
//      curated allowlist of events; it stamps {turn, player, event, args} and
//      Appends here (action_log.cpp, actionlog_tap_Initialize).
//
//----------------------------------------------------------------------------

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __ACTION_LOG_H__
#define __ACTION_LOG_H__

#include <cstddef>          // size_t
#include <nlohmann/json.hpp>

namespace action_log
{
	// Push one stamped entry onto the ledger.  Engine-only — the tap is the
	// sole caller in normal operation; tests append directly.
	void                   Append(const nlohmann::json & entry);

	// Replace the whole ledger (LoadJson).  A non-array argument resets to [].
	void                   Set(const nlohmann::json & array);

	// Reset to an empty array (new game / compat-load with no action_log key).
	void                   Clear();

	// The ledger array.  Empty array by default — never null.
	const nlohmann::json & Get();

	// Number of entries currently held.
	std::size_t            Count();
}

// Register / unregister the event-bus tap.  Called from events_Initialize /
// events_Cleanup (gs/gameobj/Events.cpp), alongside trackerevent_Initialize.
void actionlog_tap_Initialize();
void actionlog_tap_Cleanup();

#endif // __ACTION_LOG_H__
