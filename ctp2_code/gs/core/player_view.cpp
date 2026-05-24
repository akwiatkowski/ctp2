//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Engine-side queries for "which player is being viewed"
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/core/player_view.h"
#include "gs/utility/gstypes.h"  // k_MAX_PLAYERS

namespace player_view {

static VisiblePlayerFn s_visiblePlayer = nullptr;
static CurPlayerFn     s_curPlayer     = nullptr;
static PlayerAfterFn   s_playerAfter   = nullptr;

void RegisterVisiblePlayer(VisiblePlayerFn fn) { s_visiblePlayer = fn; }
void RegisterCurPlayer(CurPlayerFn fn)         { s_curPlayer     = fn; }
void RegisterPlayerAfter(PlayerAfterFn fn)     { s_playerAfter   = fn; }

sint32 VisiblePlayer()
{
	return s_visiblePlayer ? s_visiblePlayer() : -1;
}

sint32 CurPlayer()
{
	return s_curPlayer ? s_curPlayer() : 0;
}

sint32 PlayerAfter(sint32 player)
{
	if (s_playerAfter) return s_playerAfter(player);
	return (player + 1) % k_MAX_PLAYERS;
}

} // namespace player_view
