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
