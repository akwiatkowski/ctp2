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

class Army;
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
// The UI build owns g_selected_item; the engine just asks it to be
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
using IsModalActiveFn   = bool (*)();
using NextPlayerFn      = void (*)();
using NextRoundFn       = void (*)();
using SetCurOnlyFn      = void (*)(sint32 player);
using RegisterManualEndTurnFn = void (*)();
using AddPlayerFn       = void (*)(sint32 player);

void RegisterSetSelectUnit(SetSelectUnitFn fn);
void RegisterSetSelectCity(SetSelectCityFn fn);
void RegisterEnterArmyMove(EnterArmyMoveFn fn);
void RegisterSetAutoUnload(SetAutoUnloadFn fn);
void RegisterDeselect(DeselectFn fn);
void RegisterIsArmySelected(IsArmySelectedFn fn);
void RegisterIsCitySelected(IsCitySelectedFn fn);
void RegisterGetSelectedArmyId(GetSelectedIdFn fn);
void RegisterGetSelectedCityId(GetSelectedIdFn fn);
void RegisterIsModalMessageActive(IsModalActiveFn fn);
void RegisterNextPlayer(NextPlayerFn fn);
void RegisterAddPlayer(AddPlayerFn fn);

void SetSelectUnit(const Unit &unit);
void SetSelectCity(const Unit &city);
void EnterArmyMove(sint32 player, const MapPoint &pos);
void SetAutoUnload(bool on);
void Deselect(sint32 player);
bool IsArmySelected();   // false in headless
bool IsCitySelected();   // false in headless
sint32 GetSelectedArmyId();   // 0 in headless
sint32 GetSelectedCityId();   // 0 in headless
bool IsModalMessageActive();  // false in headless
void NextPlayer();            // rotates current player in turn order
void AddPlayer(sint32 player); // notifies UI of new player (no-op in headless)

} // namespace player_view
