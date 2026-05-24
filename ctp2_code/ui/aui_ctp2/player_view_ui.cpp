//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : UI-side bindings for engine player_view queries
//
//----------------------------------------------------------------------------
//
// Registered from civapp.cpp's UI init path (alongside the UIGameObserver).
// Forwards player_view::VisiblePlayer/CurPlayer/PlayerAfter to the
// SelectedItem instance, which owns the actual state.  Headless build does
// not link or register this — engine defaults kick in instead.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/core/player_view.h"
#include "ui/aui_ctp2/SelItem.h"

namespace {

sint32 UIVisiblePlayer()
{
	return g_selected_item ? g_selected_item->GetVisiblePlayer() : -1;
}

sint32 UICurPlayer()
{
	return g_selected_item ? g_selected_item->GetCurPlayer() : 0;
}

sint32 UIPlayerAfter(sint32 p)
{
	return g_selected_item ? g_selected_item->GetPlayerAfterThis(p) : -1;
}

} // anonymous namespace

void RegisterUIPlayerView()
{
	player_view::RegisterVisiblePlayer(&UIVisiblePlayer);
	player_view::RegisterCurPlayer(&UICurPlayer);
	player_view::RegisterPlayerAfter(&UIPlayerAfter);
}
