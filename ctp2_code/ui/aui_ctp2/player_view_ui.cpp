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
	return selitem_Get() ? selitem_Get()->GetVisiblePlayer() : -1;
}

sint32 UICurPlayer()
{
	return selitem_Get() ? selitem_Get()->GetCurPlayer() : 0;
}

sint32 UIPlayerAfter(sint32 p)
{
	return selitem_Get() ? selitem_Get()->GetPlayerAfterThis(p) : -1;
}

void UIInit(sint32 nPlayers)
{
	selitem_Set(new SelectedItem(nPlayers));
}

void UIInitFromArchive(CivArchive *archive)
{
	selitem_Set(new SelectedItem(*archive));
}

void UICleanup()
{
	delete selitem_Get(); selitem_Set(NULL);
}

void UISetCurrentPlayer(sint32 player)
{
	if (selitem_Get()) {
		selitem_Get()->SetPlayerOnScreen(player);
		selitem_Get()->SetCurPlayer(player);
	}
}

void UISetVisiblePlayer(sint32 player)
{
	if (selitem_Get()) {
		selitem_Get()->SetPlayerOnScreen(player);
	}
}

void UIRefresh()
{
	if (selitem_Get()) {
		selitem_Get()->Refresh();
	}
}

void UISetSelectUnit(const Unit &unit)
{
	if (selitem_Get()) {
		selitem_Get()->SetSelectUnit(const_cast<Unit &>(unit));
	}
}

void UISetSelectCity(const Unit &city)
{
	if (selitem_Get()) {
		selitem_Get()->SetSelectCity(const_cast<Unit &>(city));
	}
}

void UIEnterArmyMove(sint32 player, const MapPoint &pos)
{
	if (selitem_Get()) {
		selitem_Get()->EnterArmyMove(player, pos);
	}
}

void UISetAutoUnload(bool on)
{
	if (selitem_Get()) {
		selitem_Get()->SetAutoUnload(on);
	}
}

void UIDeselect(sint32 player)
{
	if (selitem_Get()) {
		selitem_Get()->Deselect(player);
	}
}

bool UIIsArmySelected()
{
	if (!selitem_Get()) return false;
	PLAYER_INDEX p;
	ID item;
	SELECT_TYPE state;
	selitem_Get()->GetTopCurItem(p, item, state);
	return state == SELECT_TYPE_LOCAL_ARMY;
}

bool UIIsCitySelected()
{
	if (!selitem_Get()) return false;
	PLAYER_INDEX p;
	ID item;
	SELECT_TYPE state;
	selitem_Get()->GetTopCurItem(p, item, state);
	return state == SELECT_TYPE_LOCAL_CITY;
}

sint32 UIGetSelectedArmyId()
{
	if (!selitem_Get()) return 0;
	PLAYER_INDEX p;
	ID item;
	SELECT_TYPE state;
	selitem_Get()->GetTopCurItem(p, item, state);
	return (state == SELECT_TYPE_LOCAL_ARMY) ? item.m_id : 0;
}

sint32 UIGetSelectedCityId()
{
	if (!selitem_Get()) return 0;
	PLAYER_INDEX p;
	ID item;
	SELECT_TYPE state;
	selitem_Get()->GetTopCurItem(p, item, state);
	return (state == SELECT_TYPE_LOCAL_CITY) ? static_cast<sint32>(item.m_id) : 0;
}

void UIGetTopCurItem(sint32 &player, sint32 &item, sint32 &state)
{
	if (!selitem_Get()) {
		player = 0;
		item = 0;
		state = 0;
		return;
	}
	PLAYER_INDEX p;
	ID id;
	SELECT_TYPE selState;
	selitem_Get()->GetTopCurItem(p, id, selState);
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
	if (selitem_Get()) selitem_Get()->NextPlayer();
}

void UIAddPlayer(sint32 player)
{
	if (selitem_Get()) selitem_Get()->AddPlayer(player);
}

void UIRegisterRemovedArmy(sint32 player, const Army &army)
{
	if (selitem_Get()) selitem_Get()->RegisterRemovedArmy(player, army);
}

void UIRegisterRemovedCity(sint32 player, const Unit &city)
{
	if (selitem_Get()) selitem_Get()->RegisterRemovedCity(player, city);
}

// --- Wave B (TODO(orchestrator) restoration) selection forwarders ---

bool UIIsAutoCenterOn()
{
	return selitem_Get() ? selitem_Get()->IsAutoCenterOn() : false;
}

sint32 UIGetPlayerOnScreen()
{
	return selitem_Get() ? selitem_Get()->GetPlayerOnScreen() : -1;
}

void UIForceDirectorSelect(const Army &army)
{
	if (selitem_Get()) selitem_Get()->ForceDirectorSelect(army);
}

void UIEnterMovePath(sint32 owner, Army &army,
                     const MapPoint &src, const MapPoint &dest)
{
	if (selitem_Get()) selitem_Get()->EnterMovePath(owner, army, src, dest);
}

void UINextRound()
{
	if (selitem_Get()) selitem_Get()->NextRound();
}

void UIRegisterManualEndTurn()
{
	if (selitem_Get()) selitem_Get()->RegisterManualEndTurn();
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

// --- Wave F (2026-05-29) save/load of SelectedItem state ---

void UISerializeSelectionVersion(CivArchive &archive)
{
	archive << SelectedItem_GetVersion();
}

bool UIDeserializeSelectionVersion(CivArchive &archive)
{
	uint32 ver = 0;
	archive >> ver;
	return ver == SelectedItem_GetVersion();
}

void UISerializeSelection(CivArchive &archive)
{
	if (selitem_Get())
	{
		selitem_Get()->Serialize(archive);
	}
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
	player_view::RegisterSerializeSelectionVersion(&UISerializeSelectionVersion);
	player_view::RegisterDeserializeSelectionVersion(&UIDeserializeSelectionVersion);
	player_view::RegisterSerializeSelection(&UISerializeSelection);
}
