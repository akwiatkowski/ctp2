#pragma once

/**
 * @brief Register the UI game observer (forwards events to g_c3ui, g_controlPanel, etc.)
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
 * g_selected_item is created.  Headless build leaves these unregistered.
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
