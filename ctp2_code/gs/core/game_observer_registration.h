#pragma once

/**
 * @brief Register the UI game observer (forwards events to c3ui_Get(), g_controlPanel, etc.)
 *
 * Call once during UI app initialization, after all UI globals are created.
 */
void RegisterUIGameObserver();

/**
 * @brief Register the headless game observer (logging only, no UI).
 *
 * Call once during headless app initialization.
 */
void RegisterHeadlessGameObserver();

/**
 * @brief Register UI-side callbacks for player_view queries.
 *
 * Bind player_view::VisiblePlayer / CurPlayer / PlayerAfter to the
 * SelectedItem instance.  Call once during UI app initialization, after
 * selitem_Get() is created.  Headless build leaves these unregistered.
 */
void RegisterUIPlayerView();

/**
 * @brief Register UI-side ProgressWindow adapter for progress_observer.
 *
 * Forwards progress_observer::BeginProgress / StartCountingTo / EndProgress
 * to the singleton ProgressWindow (ui/interface/progresswindow.h).  Call
 * once during UI app initialization.  Headless build leaves this
 * unregistered, so progress calls in engine code become no-ops.
 */
void RegisterProgressWindowObserver();
void UnregisterProgressWindowObserver();

/**
 * @brief Register UI-side adapter for text_observer::DrawText.
 *
 * Forwards text_observer::DrawText to ui/aui_utils/primitives.h's
 * primitives_DrawText (currently __AUI_USE_DIRECTX__-gated; no-op on
 * Mac/SDL).  Call once during UI app initialization.
 */
void RegisterTextObserverAdapter();
void UnregisterTextObserverAdapter();

/**
 * @brief Register UI-side adapter for battle_observer.
 *
 * Forwards battle_observer notifications (StartBattle, AddAttack,
 * AddDeath, AddExplosion, UpdateBattle, EndBattle, CloseBattleView)
 * to the ui/interface/battle.h Battle class and g_battleViewWindow.
 * The adapter owns the active Battle*.  Call once during UI app
 * initialization.
 */
void RegisterBattleObserverAdapter();
void UnregisterBattleObserverAdapter();

/**
 * @brief Register UI-side adapter for diplomacy_observer.
 *
 * Forwards diplomacy_observer::NotifyResponse / NotifyThreatRejected
 * (fired from ai/diplomacy/diplomat.cpp::ExecuteResponse) to the static
 * DipWizard::Notify* methods that render the wizard's "new agreement"
 * and "threat rejected" UI.  Call once during UI app initialization.
 * Headless build leaves this unregistered; the engine free functions
 * short-circuit to no-op.
 */
void RegisterDipWizardObserverAdapter();
void UnregisterDipWizardObserverAdapter();
