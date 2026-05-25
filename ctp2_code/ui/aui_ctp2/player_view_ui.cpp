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
#include "gs/gameobj/Army.h"
#include "gs/gameobj/Unit.h"
#include "gs/world/MapPoint.h"
#include "gs/utility/Globals.h"        // allocated::clear
#include "ui/aui_ctp2/SelItem.h"
#include "ui/interface/messagemodal.h"

extern MessageModal *g_modalMessage;

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

void UIInit(sint32 nPlayers)
{
	g_selected_item = new SelectedItem(nPlayers);
}

void UIInitFromArchive(CivArchive *archive)
{
	g_selected_item = new SelectedItem(*archive);
}

void UICleanup()
{
	allocated::clear(g_selected_item);
}

void UISetCurrentPlayer(sint32 player)
{
	if (g_selected_item) {
		g_selected_item->SetPlayerOnScreen(player);
		g_selected_item->SetCurPlayer(player);
	}
}

void UISetVisiblePlayer(sint32 player)
{
	if (g_selected_item) {
		g_selected_item->SetPlayerOnScreen(player);
	}
}

void UIRefresh()
{
	if (g_selected_item) {
		g_selected_item->Refresh();
	}
}

void UISetSelectUnit(const Unit &unit)
{
	if (g_selected_item) {
		g_selected_item->SetSelectUnit(const_cast<Unit &>(unit));
	}
}

void UISetSelectCity(const Unit &city)
{
	if (g_selected_item) {
		g_selected_item->SetSelectCity(const_cast<Unit &>(city));
	}
}

void UIEnterArmyMove(sint32 player, const MapPoint &pos)
{
	if (g_selected_item) {
		g_selected_item->EnterArmyMove(player, pos);
	}
}

void UISetAutoUnload(bool on)
{
	if (g_selected_item) {
		g_selected_item->SetAutoUnload(on);
	}
}

void UIDeselect(sint32 player)
{
	if (g_selected_item) {
		g_selected_item->Deselect(player);
	}
}

bool UIIsArmySelected()
{
	if (!g_selected_item) return false;
	PLAYER_INDEX p;
	ID item;
	SELECT_TYPE state;
	g_selected_item->GetTopCurItem(p, item, state);
	return state == SELECT_TYPE_LOCAL_ARMY;
}

bool UIIsCitySelected()
{
	if (!g_selected_item) return false;
	PLAYER_INDEX p;
	ID item;
	SELECT_TYPE state;
	g_selected_item->GetTopCurItem(p, item, state);
	return state == SELECT_TYPE_LOCAL_CITY;
}

sint32 UIGetSelectedArmyId()
{
	if (!g_selected_item) return 0;
	PLAYER_INDEX p;
	ID item;
	SELECT_TYPE state;
	g_selected_item->GetTopCurItem(p, item, state);
	return (state == SELECT_TYPE_LOCAL_ARMY) ? item.m_id : 0;
}

sint32 UIGetSelectedCityId()
{
	if (!g_selected_item) return 0;
	PLAYER_INDEX p;
	ID item;
	SELECT_TYPE state;
	g_selected_item->GetTopCurItem(p, item, state);
	return (state == SELECT_TYPE_LOCAL_CITY) ? static_cast<sint32>(item.m_id) : 0;
}

void UIGetTopCurItem(sint32 &player, sint32 &item, sint32 &state)
{
	if (!g_selected_item) {
		player = 0;
		item = 0;
		state = 0;
		return;
	}
	PLAYER_INDEX p;
	ID id;
	SELECT_TYPE selState;
	g_selected_item->GetTopCurItem(p, id, selState);
	player = p;
	item = id;
	state = selState;
}

bool UIIsModalMessageActive()
{
	return g_modalMessage != nullptr;
}

void UINextPlayer()
{
	if (g_selected_item) g_selected_item->NextPlayer();
}

void UIAddPlayer(sint32 player)
{
	if (g_selected_item) g_selected_item->AddPlayer(player);
}

void UIRegisterRemovedArmy(sint32 player, const Army &army)
{
	if (g_selected_item) g_selected_item->RegisterRemovedArmy(player, army);
}

} // anonymous namespace

void RegisterUIPlayerView()
{
	player_view::RegisterVisiblePlayer(&UIVisiblePlayer);
	player_view::RegisterCurPlayer(&UICurPlayer);
	player_view::RegisterPlayerAfter(&UIPlayerAfter);
	player_view::RegisterInit(&UIInit);
	player_view::RegisterInitFromArchive(&UIInitFromArchive);
	player_view::RegisterCleanup(&UICleanup);
	player_view::RegisterSetCurrentPlayer(&UISetCurrentPlayer);
	player_view::RegisterSetVisiblePlayer(&UISetVisiblePlayer);
	player_view::RegisterRefresh(&UIRefresh);
	player_view::RegisterSetSelectUnit(&UISetSelectUnit);
	player_view::RegisterSetSelectCity(&UISetSelectCity);
	player_view::RegisterEnterArmyMove(&UIEnterArmyMove);
	player_view::RegisterSetAutoUnload(&UISetAutoUnload);
	player_view::RegisterDeselect(&UIDeselect);
	player_view::RegisterIsArmySelected(&UIIsArmySelected);
	player_view::RegisterIsCitySelected(&UIIsCitySelected);
	player_view::RegisterGetSelectedArmyId(&UIGetSelectedArmyId);
	player_view::RegisterGetSelectedCityId(&UIGetSelectedCityId);
	player_view::RegisterGetTopCurItem(&UIGetTopCurItem);
	player_view::RegisterIsModalMessageActive(&UIIsModalMessageActive);
	player_view::RegisterNextPlayer(&UINextPlayer);
	player_view::RegisterAddPlayer(&UIAddPlayer);
	player_view::RegisterRemovedArmy(&UIRegisterRemovedArmy);
}
