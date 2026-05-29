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
#include "ui/interface/TurnYearStatus.h"        // GetCurrentYear
#include "ui/interface/CityControlPanel.h"      // GetBuildName / GetBuildStringId
#include "ui/interface/MainControlPanel.h"      // GetSelectedCargo
#include "ui/interface/scenarioeditor.h"        // Wave D: ScenarioEditor statics
#include "ui/interface/EditQueue.h"             // Wave D: EditQueue::GetEditQueueWindow
#include "ui/interface/citywindow.h"            // Wave D: CityWindow::GetCityWindow
#include "gs/gameobj/citydata.h"                // CityData::GetBuildQueue
#include "gs/gameobj/BldQue.h"                  // BuildQueue::GetHead
#include "gs/world/cellunitlist.h"              // CellUnitList

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

void UIRegisterRemovedCity(sint32 player, const Unit &city)
{
	if (g_selected_item) g_selected_item->RegisterRemovedCity(player, city);
}

// --- Wave B (TODO(orchestrator) restoration) selection forwarders ---

bool UIIsAutoCenterOn()
{
	return g_selected_item ? g_selected_item->IsAutoCenterOn() : false;
}

sint32 UIGetPlayerOnScreen()
{
	return g_selected_item ? g_selected_item->GetPlayerOnScreen() : -1;
}

void UIForceDirectorSelect(const Army &army)
{
	if (g_selected_item) g_selected_item->ForceDirectorSelect(army);
}

void UIEnterMovePath(sint32 owner, Army &army,
                     const MapPoint &src, const MapPoint &dest)
{
	if (g_selected_item) g_selected_item->EnterMovePath(owner, army, src, dest);
}

void UINextRound()
{
	if (g_selected_item) g_selected_item->NextRound();
}

void UIRegisterManualEndTurn()
{
	if (g_selected_item) g_selected_item->RegisterManualEndTurn();
}

// --- Wave C (2026-05-29) HUD-query forwarders ---

void UIGetCurrentYearString(char *out, size_t cap)
{
	if (!out || cap == 0) return;
	const MBCHAR *src = TurnYearStatus::GetCurrentYear();
	if (src)
	{
		strncpy(out, src, cap - 1);
		out[cap - 1] = '\0';
	}
	else
	{
		out[0] = '\0';
	}
}

void UIGetBuildQueueHeadName(const CityData *city, char *out, size_t cap)
{
	if (!out || cap == 0) return;
	out[0] = '\0';
	if (!city) return;
	// CityData::GetBuildQueue returns BuildQueue*; const-cast because the
	// legacy accessor is not const-qualified.
	BuildQueue *queue = const_cast<CityData *>(city)->GetBuildQueue();
	if (!queue) return;
	const BuildNode *head = queue->GetHead();
	if (!head) return;
	const MBCHAR *name = CityControlPanel::GetBuildName(head);
	if (name)
	{
		strncpy(out, name, cap - 1);
		out[cap - 1] = '\0';
	}
}

sint32 UIGetBuildQueueHeadStringId(const CityData *city)
{
	if (!city) return -1;
	BuildQueue *queue = const_cast<CityData *>(city)->GetBuildQueue();
	if (!queue) return -1;
	const BuildNode *head = queue->GetHead();
	if (!head) return -1;
	return CityControlPanel::GetBuildStringId(head);
}

bool UIGetSelectedCargo(CellUnitList &out)
{
	return MainControlPanel::GetSelectedCargo(out);
}

// --- Wave D (2026-05-29) scenario editor / edit queue / city window ---

bool UIIsScenarioEditorPlaceCityMode()
{
	return ScenarioEditor::PlaceCityMode();
}

sint32 UIGetScenarioEditorCitySize()
{
	return ScenarioEditor::CitySize();
}

sint32 UIGetScenarioEditorCityStyle()
{
	return ScenarioEditor::CityStyle();
}

void UIEditQueueSyncShieldstore(const Unit &homeCity, sint32 s)
{
	EditQueue *eq = EditQueue::GetEditQueueWindow();
	if (!eq) return;
	CityData *cd = eq->GetCityData();
	if (cd && cd->GetHomeCity() == homeCity)
	{
		cd->SetShieldstore(s);
	}
}

void UIEditQueueSyncBuildCategory(const Unit &homeCity, sint32 cat)
{
	EditQueue *eq = EditQueue::GetEditQueueWindow();
	if (!eq) return;
	CityData *cd = eq->GetCityData();
	if (cd && cd->GetHomeCity() == homeCity)
	{
		cd->SetBuildCategoryAtBeginTurn(cat);
	}
}

bool UICityWindowAddShieldsIfShowing(const Unit &city, sint32 amount)
{
	CityWindow *cw = CityWindow::GetCityWindow();
	if (!cw) return false;
	CityData *cd = cw->GetCityData();
	if (cd && cd->GetHomeCity() == city)
	{
		cd->AddShields(amount);
		return true;
	}
	return false;
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
	player_view::RegisterRemovedCity(&UIRegisterRemovedCity);
	player_view::RegisterIsAutoCenterOn(&UIIsAutoCenterOn);
	player_view::RegisterGetPlayerOnScreen(&UIGetPlayerOnScreen);
	player_view::RegisterForceDirectorSelect(&UIForceDirectorSelect);
	player_view::RegisterEnterMovePath(&UIEnterMovePath);
	player_view::RegisterNextRound(&UINextRound);
	player_view::RegisterRegisterManualEndTurn(&UIRegisterManualEndTurn);
	player_view::RegisterGetCurrentYearString(&UIGetCurrentYearString);
	player_view::RegisterGetBuildQueueHeadName(&UIGetBuildQueueHeadName);
	player_view::RegisterGetBuildQueueHeadStringId(&UIGetBuildQueueHeadStringId);
	player_view::RegisterGetSelectedCargo(&UIGetSelectedCargo);
	player_view::RegisterIsScenarioEditorPlaceCityMode(&UIIsScenarioEditorPlaceCityMode);
	player_view::RegisterGetScenarioEditorCitySize(&UIGetScenarioEditorCitySize);
	player_view::RegisterGetScenarioEditorCityStyle(&UIGetScenarioEditorCityStyle);
	player_view::RegisterEditQueueSyncShieldstore(&UIEditQueueSyncShieldstore);
	player_view::RegisterEditQueueSyncBuildCategory(&UIEditQueueSyncBuildCategory);
	player_view::RegisterCityWindowAddShieldsIfShowing(&UICityWindowAddShieldsIfShowing);
}
