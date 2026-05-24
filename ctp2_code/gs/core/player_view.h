//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Engine-side queries for "which player is being viewed"
//
//----------------------------------------------------------------------------
//
// CTP2's SelectedItem (ui/aui_ctp2/SelItem.h) tracks both UI selection state
// and the human-visible-player / current-player / turn-order chain.  Game
// logic frequently needs to ask "which player is on screen?" or "who's next
// after this one?" — without that, the simulation core can't generate
// correctly-targeted SLIC notifications, intercept the right unit, or
// advance the AI stop-player when one dies.
//
// This header exposes those queries as plain functions registered by the
// UI build at startup.  The headless build leaves them unregistered, in
// which case the defaults below apply:
//   VisiblePlayer() → -1 (no human viewer)
//   CurPlayer()    → 0  (first player slot)
//   PlayerAfter(p) → (p + 1) % k_MAX_PLAYERS (linear turn order)
//
// gs/gameobj/Player.cpp uses these instead of including
// ui/aui_ctp2/SelItem.h directly.
//
//----------------------------------------------------------------------------

#pragma once

#include "ctp2_inttypes.h"

namespace player_view {

using VisiblePlayerFn = sint32 (*)();
using CurPlayerFn     = sint32 (*)();
using PlayerAfterFn   = sint32 (*)(sint32);

void RegisterVisiblePlayer(VisiblePlayerFn fn);
void RegisterCurPlayer(CurPlayerFn fn);
void RegisterPlayerAfter(PlayerAfterFn fn);

sint32 VisiblePlayer();
sint32 CurPlayer();
sint32 PlayerAfter(sint32 player);

} // namespace player_view
