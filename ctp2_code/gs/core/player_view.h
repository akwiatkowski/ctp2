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
#include <cstddef>  // size_t (used by Wave C bridge methods)

class Army;
class CellUnitList;
class CityData;
class CivArchive;
class MapPoint;
class Unit;

namespace player_view {

// --- Queries (engine asks UI "what's being viewed?") ---
using VisiblePlayerFn = sint32 (*)();
using CurPlayerFn     = sint32 (*)();
using PlayerAfterFn   = sint32 (*)(sint32);

void RegisterVisiblePlayer(VisiblePlayerFn fn);
void RegisterCurPlayer(CurPlayerFn fn);
void RegisterPlayerAfter(PlayerAfterFn fn);

sint32 VisiblePlayer();
sint32 CurPlayer();
sint32 PlayerAfter(sint32 player);

// --- Lifecycle of the UI-side selection state ---
// The UI build owns selitem_Get(); the engine just asks it to be
// created/destroyed at the right points in game init/cleanup.  Headless
// leaves these unregistered (no-op).
using InitFn            = void (*)(sint32 nPlayers);
using InitFromArchiveFn = void (*)(CivArchive *archive);
using CleanupFn         = void (*)();
using SetCurrentPlayerFn = void (*)(sint32 player);
using SetVisiblePlayerFn = void (*)(sint32 player);
using RefreshFn         = void (*)();

void RegisterInit(InitFn fn);
void RegisterInitFromArchive(InitFromArchiveFn fn);
void RegisterCleanup(CleanupFn fn);
void RegisterSetCurrentPlayer(SetCurrentPlayerFn fn);
void RegisterSetVisiblePlayer(SetVisiblePlayerFn fn);
void RegisterRefresh(RefreshFn fn);

void Init(sint32 nPlayers);
void InitFromArchive(CivArchive *archive);
void Cleanup();
void SetCurrentPlayer(sint32 player);
void SetVisiblePlayer(sint32 player);
void Refresh();

// --- Selection commands & queries (used by SLIC built-ins) ---
using SetSelectUnitFn   = void (*)(const Unit &);
using SetSelectCityFn   = void (*)(const Unit &);
using EnterArmyMoveFn   = void (*)(sint32 player, const MapPoint &);
using SetAutoUnloadFn   = void (*)(bool);
using DeselectFn        = void (*)(sint32 player);
using IsArmySelectedFn  = bool (*)();
using IsCitySelectedFn  = bool (*)();
using GetSelectedIdFn   = sint32 (*)();
using GetTopCurItemFn   = void (*)(sint32 &player, sint32 &item, sint32 &state);
using IsModalActiveFn   = bool (*)();
using NextPlayerFn      = void (*)();
using NextRoundFn       = void (*)();
using SetCurOnlyFn      = void (*)(sint32 player);
using RegisterManualEndTurnFn = void (*)();
using AddPlayerFn       = void (*)(sint32 player);
using RegisterRemovedArmyFn = void (*)(sint32 player, const Army &army);
using RegisterRemovedCityFn = void (*)(sint32 player, const Unit &city);
// --- Wave B (TODO(orchestrator) restoration, 2026-05-29): the SelectedItem
//     surface used by ArmyData / TurnCnt / armyevent.
using IsAutoCenterOnFn      = bool   (*)();
using GetPlayerOnScreenFn   = sint32 (*)();
using ForceDirectorSelectFn = void   (*)(const Army &army);
using EnterMovePathFn       = void   (*)(sint32 owner, Army &army,
                                          const MapPoint &src,
                                          const MapPoint &dest);

// --- Wave C (2026-05-29): HUD queries (turn-year status, build-queue head
//     name/string-id) consumed by SlicBuiltin, and the unload-selected-cargo
//     query consumed by armyevent.  `char` rather than MBCHAR in the typedef
//     since MBCHAR's typedef lives in ui/ headers and gs/core/ must not
//     include those.
using GetCurrentYearStringFn = void (*)(char *out, size_t cap);
using GetBuildQueueHeadNameFn = void (*)(const CityData *city,
                                          char *out, size_t cap);
using GetBuildQueueHeadStringIdFn = sint32 (*)(const CityData *city);
using GetSelectedCargoFn = bool (*)(CellUnitList &out);

// --- Wave D (2026-05-29): scenario editor / edit queue / city window
//     state queries + edit-queue / city-window sync write-throughs.
//     Consumed by CityData and ArmyData.  Each sync method is a no-op
//     when the relevant UI window is not open or not showing the named
//     city — caller does not need to gate.
using IsScenarioEditorPlaceCityModeFn = bool   (*)();
using GetScenarioEditorCitySizeFn     = sint32 (*)();
using GetScenarioEditorCityStyleFn    = sint32 (*)();
using EditQueueSyncShieldstoreFn      = void   (*)(const Unit &homeCity, sint32 s);
using EditQueueSyncBuildCategoryFn    = void   (*)(const Unit &homeCity, sint32 cat);
using CityWindowAddShieldsIfShowingFn = bool   (*)(const Unit &city, sint32 amount);

// --- Wave F (2026-05-29): save/load of the SelectedItem-side cursor
//     state.  Save format reset accepted — old saves (with no selection
//     bytes) will no longer load by the post-Wave-F binary, and saves
//     made by headless / by UI are not interchangeable.
//     SerializeSelectionVersion writes the version stamp; the matching
//     DeserializeSelectionVersion returns false on mismatch so the
//     caller can raise LOAD_INCORRECT_VERSION_INFO.  SerializeSelection
//     writes the full SelectedItem state; InitFromArchive (already
//     present in this header above) reads it on load.
using SerializeSelectionVersionFn   = void (*)(CivArchive &archive);
using DeserializeSelectionVersionFn = bool (*)(CivArchive &archive);
using SerializeSelectionFn          = void (*)(CivArchive &archive);

void RegisterSetSelectUnit(SetSelectUnitFn fn);
void RegisterSetSelectCity(SetSelectCityFn fn);
void RegisterEnterArmyMove(EnterArmyMoveFn fn);
void RegisterSetAutoUnload(SetAutoUnloadFn fn);
void RegisterDeselect(DeselectFn fn);
void RegisterIsArmySelected(IsArmySelectedFn fn);
void RegisterIsCitySelected(IsCitySelectedFn fn);
void RegisterGetSelectedArmyId(GetSelectedIdFn fn);
void RegisterGetSelectedCityId(GetSelectedIdFn fn);
void RegisterGetTopCurItem(GetTopCurItemFn fn);
void RegisterIsModalMessageActive(IsModalActiveFn fn);
void RegisterNextPlayer(NextPlayerFn fn);
void RegisterAddPlayer(AddPlayerFn fn);
void RegisterRemovedArmy(RegisterRemovedArmyFn fn);
void RegisterRemovedCity(RegisterRemovedCityFn fn);
void RegisterNextRound(NextRoundFn fn);
void RegisterRegisterManualEndTurn(RegisterManualEndTurnFn fn);
void RegisterIsAutoCenterOn(IsAutoCenterOnFn fn);
void RegisterGetPlayerOnScreen(GetPlayerOnScreenFn fn);
void RegisterForceDirectorSelect(ForceDirectorSelectFn fn);
void RegisterEnterMovePath(EnterMovePathFn fn);
void RegisterGetCurrentYearString(GetCurrentYearStringFn fn);
void RegisterGetBuildQueueHeadName(GetBuildQueueHeadNameFn fn);
void RegisterGetBuildQueueHeadStringId(GetBuildQueueHeadStringIdFn fn);
void RegisterGetSelectedCargo(GetSelectedCargoFn fn);
void RegisterIsScenarioEditorPlaceCityMode(IsScenarioEditorPlaceCityModeFn fn);
void RegisterGetScenarioEditorCitySize(GetScenarioEditorCitySizeFn fn);
void RegisterGetScenarioEditorCityStyle(GetScenarioEditorCityStyleFn fn);
void RegisterEditQueueSyncShieldstore(EditQueueSyncShieldstoreFn fn);
void RegisterEditQueueSyncBuildCategory(EditQueueSyncBuildCategoryFn fn);
void RegisterCityWindowAddShieldsIfShowing(CityWindowAddShieldsIfShowingFn fn);
void RegisterSerializeSelectionVersion(SerializeSelectionVersionFn fn);
void RegisterDeserializeSelectionVersion(DeserializeSelectionVersionFn fn);
void RegisterSerializeSelection(SerializeSelectionFn fn);

void SetSelectUnit(const Unit &unit);
void SetSelectCity(const Unit &city);
void EnterArmyMove(sint32 player, const MapPoint &pos);
void SetAutoUnload(bool on);
void Deselect(sint32 player);
bool IsArmySelected();   // false in headless
bool IsCitySelected();   // false in headless
sint32 GetSelectedArmyId();   // 0 in headless
sint32 GetSelectedCityId();   // 0 in headless
void GetTopCurItem(sint32 &player, sint32 &item, sint32 &state); // no-op in headless
bool IsModalMessageActive();  // false in headless
void NextPlayer();            // rotates current player in turn order
void AddPlayer(sint32 player); // notifies UI of new player (no-op in headless)
void ArmyRemoved(sint32 player, const Army &army); // notifies UI of removed army (no-op in headless)
void CityRemoved(sint32 player, const Unit &city); // notifies UI of removed city (no-op in headless)

// --- Wave B selection surface ---
// Defaults in headless / when no Impl registered:
//   IsAutoCenterOn() → false
//   GetPlayerOnScreen() → -1
//   All notifications → no-op
bool   IsAutoCenterOn();
sint32 GetPlayerOnScreen();
void   ForceDirectorSelect(const Army &army);
void   EnterMovePath(sint32 owner, Army &army,
                     const MapPoint &src, const MapPoint &dest);
void   NextRound();
void   RegisterManualEndTurn();

// --- Wave C surface ---
// Defaults when no Impl is registered:
//   GetCurrentYearString → writes empty string (out[0] = '\0' if cap > 0)
//   GetBuildQueueHeadName → empty string
//   GetBuildQueueHeadStringId → -1
//   GetSelectedCargo → false (out stays untouched)
void   GetCurrentYearString(char *out, size_t cap);
void   GetBuildQueueHeadName(const CityData *city, char *out, size_t cap);
sint32 GetBuildQueueHeadStringId(const CityData *city);
bool   GetSelectedCargo(CellUnitList &out);

// --- Wave D surface ---
// Defaults when no Impl is registered:
//   IsScenarioEditorPlaceCityMode → false
//   GetScenarioEditorCitySize → 0
//   GetScenarioEditorCityStyle → 0
//   EditQueueSync* → no-op (sync write-through is harmless to skip)
//   CityWindowAddShieldsIfShowing → false (caller falls back to real city)
bool   IsScenarioEditorPlaceCityMode();
sint32 GetScenarioEditorCitySize();
sint32 GetScenarioEditorCityStyle();
void   EditQueueSyncShieldstore(const Unit &homeCity, sint32 s);
void   EditQueueSyncBuildCategory(const Unit &homeCity, sint32 cat);
bool   CityWindowAddShieldsIfShowing(const Unit &city, sint32 amount);

// --- Wave F surface ---
// When no Impl is registered (headless / pre-registration):
//   SerializeSelectionVersion → writes nothing
//   DeserializeSelectionVersion → returns true (no bytes consumed; version "matches" trivially)
//   SerializeSelection → writes nothing
// The headless build therefore produces a save file that is symmetric
// with its own load path — but a UI-saved file CANNOT be loaded by
// headless and vice versa (the byte count diverges).
void SerializeSelectionVersion(CivArchive &archive);
bool DeserializeSelectionVersion(CivArchive &archive);
void SerializeSelection(CivArchive &archive);

} // namespace player_view
