//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : UI-free command/query dispatch for the test/automation API.
//
//----------------------------------------------------------------------------
//
// GameController is the shared "presenter" seam between an external driver
// (the Python test harness today; a web/mobile client later) and the game
// simulation.  It operates purely on game-state objects (Player, City, Unit,
// CityData) and never touches AUI widgets, so the SAME command/query set
// behaves identically in the interactive (ctp2) and headless (ctp2_headless)
// builds.  Both the UI ProcessUI loop and the headless --serve loop dispatch
// through Dispatch().
//
// Verbs are split into three families:
//   * commands      — mutate state (build_city, set_production, save/load).
//                     They enforce the same preconditions the UI would (e.g. a
//                     settler must exist), so a driver cannot reach past a
//                     disabled action.
//   * queries       — read state, fog-of-war filtered (query_cities,
//                     query_city), i.e. only what the visible player can
//                     actually see.  Use these for anything player-shaped.
//   * admin queries — OMNISCIENT, no fog filter (query_players,
//                     query_player_cities).  Observability surface for the
//                     gateway admin panel and debugging only.
//
// Game creation (new_game / start_game) is deliberately NOT handled here: it is
// inherently frontend-specific (the UI navigates menu screens; headless calls
// CivApp::InitializeGame directly), so each frontend owns that step.
//
//----------------------------------------------------------------------------

#ifndef CTP_GAME_CONTROLLER_H
#define CTP_GAME_CONTROLLER_H

#include <string>

class Player;

namespace game_controller {

typedef void (*SetCurrentPlayerFn)(sint32 player);

// Execute a single command/query line (e.g. "build_city",
// "set_production 0 settler", "query_city 0").
//
// Returns the response as a single-line JSON string, ready to send back over
// the socket:
//   success : {"status":"ok","cmd":"<verb>","result":{...}}
//   failure : {"status":"error","cmd":"<verb>","detail":"<code>"}
//
// Sets `handled` to false when `line` is not a GameController verb; the caller
// then falls back to its legacy dispatch (UI ProcessUI chain) or reports an
// unknown command (headless).
std::string Dispatch(const std::string & line, bool & handled);

// Like Dispatch, but never lets an exception escape: a throwing handler is
// logged and converted to {"status":"error","cmd":"dispatch","detail":
// "exception"} with handled=true.  Use from dispatch loops that hold the
// smoke-server mutex, where an escaping exception would leave it locked and
// wedge the server forever.
std::string DispatchSafe(const std::string & line, bool & handled);

// The (single) human player, or nullptr when none exists.  Queries report
// from this player's viewpoint; frontends use it to point CurPlayer at the
// human after game creation.  Deliberately not selitem_Get()-based — that UI
// singleton may be absent/empty in the headless build.
Player * HumanPlayer();

// Run one complete simulation round: every live player gets BeginTurn, AI
// scheduler events, queued order resumption, and EndTurn. Frontends pass a
// current-player setter because UI and headless builds store CurPlayer in
// different places.
void RunRound(sint32 round, SetCurrentPlayerFn set_current_player);

}  // namespace game_controller

#endif  // CTP_GAME_CONTROLLER_H
