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

static VisiblePlayerFn    s_visiblePlayer    = nullptr;
static CurPlayerFn        s_curPlayer        = nullptr;
static PlayerAfterFn      s_playerAfter      = nullptr;
static InitFn             s_init             = nullptr;
static InitFromArchiveFn  s_initFromArchive  = nullptr;
static CleanupFn          s_cleanup          = nullptr;
static SetCurrentPlayerFn s_setCurrentPlayer = nullptr;
static SetVisiblePlayerFn s_setVisiblePlayer = nullptr;
static RefreshFn          s_refresh          = nullptr;

void RegisterVisiblePlayer(VisiblePlayerFn fn)        { s_visiblePlayer    = fn; }
void RegisterCurPlayer(CurPlayerFn fn)                { s_curPlayer        = fn; }
void RegisterPlayerAfter(PlayerAfterFn fn)            { s_playerAfter      = fn; }
void RegisterInit(InitFn fn)                          { s_init             = fn; }
void RegisterInitFromArchive(InitFromArchiveFn fn)    { s_initFromArchive  = fn; }
void RegisterCleanup(CleanupFn fn)                    { s_cleanup          = fn; }
void RegisterSetCurrentPlayer(SetCurrentPlayerFn fn)  { s_setCurrentPlayer = fn; }
void RegisterSetVisiblePlayer(SetVisiblePlayerFn fn)  { s_setVisiblePlayer = fn; }
void RegisterRefresh(RefreshFn fn)                    { s_refresh          = fn; }

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

void Init(sint32 nPlayers)
{
	if (s_init) s_init(nPlayers);
}

void InitFromArchive(CivArchive *archive)
{
	if (s_initFromArchive) s_initFromArchive(archive);
}

void Cleanup()
{
	if (s_cleanup) s_cleanup();
}

void SetCurrentPlayer(sint32 player)
{
	if (s_setCurrentPlayer) s_setCurrentPlayer(player);
}

void SetVisiblePlayer(sint32 player)
{
	if (s_setVisiblePlayer) s_setVisiblePlayer(player);
}

void Refresh()
{
	if (s_refresh) s_refresh();
}

} // namespace player_view
