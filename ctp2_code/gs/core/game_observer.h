/**
 * @file game_observer.h
 * @brief Clean architecture observer interface for game events.
 *
 * Separates game simulation logic from UI/audio/render callbacks.
 * UI build registers a UIGameObserver that forwards to existing globals.
 * Headless build registers no observers (or a logging stub).
 *
 * All game logic calls NotifyXxx() on the registry; the registry dispatches
 * to all registered observers. No game logic knows whether UI exists.
 */

#pragma once

#include "gs/gameobj/Unit.h"  // for Unit (by value in interface)
#include "gs/world/MapPoint.h"

class Army;
class Player;

/**
 * @brief Pure virtual interface for observing game events.
 *
 * Implementations live in ui/ (UIGameObserver) and ctp/ (HeadlessGameObserver).
 * The registry owns no memory — observers must outlive the registry.
 */
class IGameObserver {
public:
    virtual ~IGameObserver() = default;

    // --- Turn lifecycle ---
    virtual void OnTurnStart(sint32 player) {}
    virtual void OnTurnEnd(sint32 player) {}
    virtual void OnBuildPhaseComplete(sint32 player) {}

    // --- City events ---
    virtual void OnCityFounded(sint32 player, const Unit& city,
                               const MapPoint& pos, sint32 cause) {}
    virtual void OnCityCaptured(const Unit& city, sint32 newOwner,
                                const MapPoint& pos) {}
    virtual void OnWonderBuilt(const Unit& city, sint32 wonder) {}

    // --- Army / combat events ---
    virtual void OnArmyMove(const Army& army,
                            const MapPoint& from, const MapPoint& to) {}
    virtual void OnCombatStart(const Army& attacker, const Army& defender,
                               const MapPoint& pos) {}
    virtual void OnCombatEnd(const Army& attacker, const Army& defender,
                             bool attackerWon) {}

    // --- Research ---
    virtual void OnAdvanceResearched(sint32 player, sint32 advance) {}

    // --- UI refresh requests (no-ops in headless) ---
    virtual void OnUpdateScienceWindow(sint32 player) {}
    virtual void OnUpdateCityList() {}
    virtual void OnUpdateUnitPanel(sint32 player) {}
    virtual void OnUpdateControlPanel(sint32 player) {}
    virtual void OnUpdateMessages(sint32 player) {}
};

/**
 * @brief Singleton registry that dispatches events to all observers.
 *
 * Thread-unsafe (CTP2 is single-threaded). Observers register during app init
 * and unregister at shutdown.
 */
class GameObserverRegistry {
public:
    static GameObserverRegistry& Instance();

    void Register(IGameObserver* observer);
    void Unregister(IGameObserver* observer);

    // --- Turn lifecycle ---
    void NotifyTurnStart(sint32 player);
    void NotifyTurnEnd(sint32 player);
    void NotifyBuildPhaseComplete(sint32 player);

    // --- City events ---
    void NotifyCityFounded(sint32 player, const Unit& city,
                           const MapPoint& pos, sint32 cause);
    void NotifyCityCaptured(const Unit& city, sint32 newOwner,
                            const MapPoint& pos);
    void NotifyWonderBuilt(const Unit& city, sint32 wonder);

    // --- Army / combat ---
    void NotifyArmyMove(const Army& army,
                        const MapPoint& from, const MapPoint& to);
    void NotifyCombatStart(const Army& attacker, const Army& defender,
                           const MapPoint& pos);
    void NotifyCombatEnd(const Army& attacker, const Army& defender,
                         bool attackerWon);

    // --- Research ---
    void NotifyAdvanceResearched(sint32 player, sint32 advance);

    // --- UI refresh ---
    void NotifyUpdateScienceWindow(sint32 player);
    void NotifyUpdateCityList();
    void NotifyUpdateUnitPanel(sint32 player);
    void NotifyUpdateControlPanel(sint32 player);
    void NotifyUpdateMessages(sint32 player);

private:
    GameObserverRegistry() = default;
    GameObserverRegistry(const GameObserverRegistry&) = delete;
    GameObserverRegistry& operator=(const GameObserverRegistry&) = delete;

    std::vector<IGameObserver*> m_observers;
};

/**
 * @brief Global accessor for convenience.
 *
 * Game logic should call g_gameObservers->NotifyXxx() instead of
 * hard-coded UI globals like g_c3ui, g_controlPanel, g_director.
 */
extern GameObserverRegistry* g_gameObservers;
