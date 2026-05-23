#include "ctp/c3.h"
#include "gs/core/game_observer.h"

GameObserverRegistry& GameObserverRegistry::Instance()
{
    static GameObserverRegistry s_instance;
    return s_instance;
}

void GameObserverRegistry::Register(IGameObserver* observer)
{
    if (observer) {
        m_observers.push_back(observer);
    }
}

void GameObserverRegistry::Unregister(IGameObserver* observer)
{
    auto it = std::find(m_observers.begin(), m_observers.end(), observer);
    if (it != m_observers.end()) {
        m_observers.erase(it);
    }
}

// --- Turn lifecycle ---

void GameObserverRegistry::NotifyTurnStart(sint32 player)
{
    for (auto* obs : m_observers) {
        obs->OnTurnStart(player);
    }
}

void GameObserverRegistry::NotifyTurnEnd(sint32 player)
{
    for (auto* obs : m_observers) {
        obs->OnTurnEnd(player);
    }
}

void GameObserverRegistry::NotifyBuildPhaseComplete(sint32 player)
{
    for (auto* obs : m_observers) {
        obs->OnBuildPhaseComplete(player);
    }
}

// --- City events ---

void GameObserverRegistry::NotifyCityFounded(sint32 player, const Unit& city,
                                             const MapPoint& pos, sint32 cause)
{
    for (auto* obs : m_observers) {
        obs->OnCityFounded(player, city, pos, cause);
    }
}

void GameObserverRegistry::NotifyCityCaptured(const Unit& city, sint32 newOwner,
                                              const MapPoint& pos)
{
    for (auto* obs : m_observers) {
        obs->OnCityCaptured(city, newOwner, pos);
    }
}

void GameObserverRegistry::NotifyWonderBuilt(const Unit& city, sint32 wonder)
{
    for (auto* obs : m_observers) {
        obs->OnWonderBuilt(city, wonder);
    }
}

// --- Army / combat ---

void GameObserverRegistry::NotifyArmyMove(const Army& army,
                                          const MapPoint& from, const MapPoint& to)
{
    for (auto* obs : m_observers) {
        obs->OnArmyMove(army, from, to);
    }
}

void GameObserverRegistry::NotifyCombatStart(const Army& attacker,
                                             const Army& defender,
                                             const MapPoint& pos)
{
    for (auto* obs : m_observers) {
        obs->OnCombatStart(attacker, defender, pos);
    }
}

void GameObserverRegistry::NotifyCombatEnd(const Army& attacker,
                                           const Army& defender,
                                           bool attackerWon)
{
    for (auto* obs : m_observers) {
        obs->OnCombatEnd(attacker, defender, attackerWon);
    }
}

// --- Research ---

void GameObserverRegistry::NotifyAdvanceResearched(sint32 player, sint32 advance)
{
    for (auto* obs : m_observers) {
        obs->OnAdvanceResearched(player, advance);
    }
}

// --- UI refresh ---

void GameObserverRegistry::NotifyUpdateScienceWindow(sint32 player)
{
    for (auto* obs : m_observers) {
        obs->OnUpdateScienceWindow(player);
    }
}

void GameObserverRegistry::NotifyUpdateCityList()
{
    for (auto* obs : m_observers) {
        obs->OnUpdateCityList();
    }
}

void GameObserverRegistry::NotifyUpdateUnitPanel(sint32 player)
{
    for (auto* obs : m_observers) {
        obs->OnUpdateUnitPanel(player);
    }
}

void GameObserverRegistry::NotifyUpdateControlPanel(sint32 player)
{
    for (auto* obs : m_observers) {
        obs->OnUpdateControlPanel(player);
    }
}

void GameObserverRegistry::NotifyUpdateMessages(sint32 player)
{
    for (auto* obs : m_observers) {
        obs->OnUpdateMessages(player);
    }
}

// Global instance pointer.  Initialized inside CivApp::InitializeEngine
// (called by both the UI and headless entry points) before any observer
// registration runs.  Eager static-init via `= &Instance()` would have been
// undefined-behavior if some other TU's global constructor reached
// g_gameObservers->Notify() before this one ran — C++ doesn't guarantee
// inter-TU init order.  Deferring to InitializeEngine makes the lifetime
// explicit and ordered.
GameObserverRegistry* g_gameObservers = nullptr;
