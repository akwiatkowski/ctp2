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

void GameObserverRegistry::NotifyCityOwnerReset(const Unit& city)
{
    for (auto* obs : m_observers) {
        obs->OnCityOwnerReset(city);
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

void GameObserverRegistry::NotifyArmyRemoved(sint32 player, const Army& army)
{
    for (auto* obs : m_observers) {
        obs->OnArmyRemoved(player, army);
    }
}

void GameObserverRegistry::NotifyPlayerRemoved(sint32 player)
{
    for (auto* obs : m_observers) {
        obs->OnPlayerRemoved(player);
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

void GameObserverRegistry::NotifyResearchAdvanceDialog(sint32 player, sint32 advance,
                                                       const MBCHAR* text)
{
    for (auto* obs : m_observers) {
        obs->OnResearchAdvanceDialog(player, advance, text);
    }
}

// --- Vision ---

void GameObserverRegistry::NotifyVisionAdded(sint32 player, const MapPoint& pos,
                                             double range)
{
    for (auto* obs : m_observers) {
        obs->OnVisionAdded(player, pos, range);
    }
}

void GameObserverRegistry::NotifyVisionRemoved(sint32 player, const MapPoint& pos,
                                               double range)
{
    for (auto* obs : m_observers) {
        obs->OnVisionRemoved(player, pos, range);
    }
}

void GameObserverRegistry::NotifyVisionCopied(sint32 fromPlayer, sint32 toPlayer)
{
    for (auto* obs : m_observers) {
        obs->OnVisionCopied(fromPlayer, toPlayer);
    }
}

// --- Government ---

void GameObserverRegistry::NotifyGovernmentChanged(sint32 player, sint32 type)
{
    for (auto* obs : m_observers) {
        obs->OnGovernmentChanged(player, type);
    }
}

// --- Game over ---

void GameObserverRegistry::NotifyGameOver(sint32 player, sint32 reason,
                                          sint32 previouslyWon, sint32 previouslyLost)
{
    for (auto* obs : m_observers) {
        obs->OnGameOver(player, reason, previouslyWon, previouslyLost);
    }
}

// --- Trade ---

void GameObserverRegistry::NotifyTradeChanged()
{
    for (auto* obs : m_observers) {
        obs->OnTradeChanged();
    }
}

void GameObserverRegistry::NotifyForeignTradeBid(sint32 player, const Unit& fromCity,
                                                 const Unit& toCity, sint32 resource)
{
    for (auto* obs : m_observers) {
        obs->OnForeignTradeBid(player, fromCity, toCity, resource);
    }
}

// --- Messages ---

void GameObserverRegistry::NotifyMessageReceived(const Message& msg, sint32 player)
{
    for (auto* obs : m_observers) {
        obs->OnMessageReceived(msg, player);
    }
}

void GameObserverRegistry::NotifyMessagesRedisplay(sint32 player)
{
    for (auto* obs : m_observers) {
        obs->OnMessagesRedisplay(player);
    }
}

void GameObserverRegistry::NotifyModalMessageDismissed(sint32 player)
{
    for (auto* obs : m_observers) {
        obs->OnModalMessageDismissed(player);
    }
}

void GameObserverRegistry::NotifyMessageShow(const Message& msg)
{
    for (auto* obs : m_observers) {
        obs->OnMessageShow(msg);
    }
}

void GameObserverRegistry::NotifyMessageMinimize(const Message& msg)
{
    for (auto* obs : m_observers) {
        obs->OnMessageMinimize(msg);
    }
}

void GameObserverRegistry::NotifyMessageWindowDestroy(const Message& msg)
{
    for (auto* obs : m_observers) {
        obs->OnMessageWindowDestroy(msg);
    }
}

void GameObserverRegistry::NotifyMessageRead(const Message& msg)
{
    for (auto* obs : m_observers) {
        obs->OnMessageRead(msg);
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

void GameObserverRegistry::NotifyControlPanelRedraw(sint32 player)
{
    for (auto* obs : m_observers) {
        obs->OnControlPanelRedraw(player);
    }
}

void GameObserverRegistry::NotifyRadarMapUpdate(sint32 player)
{
    for (auto* obs : m_observers) {
        obs->OnRadarMapUpdate(player);
    }
}

void GameObserverRegistry::NotifyRadarMapRedrawTile(const MapPoint& pos)
{
    for (auto* obs : m_observers) {
        obs->OnRadarMapRedrawTile(pos);
    }
}

void GameObserverRegistry::NotifyAutoSelectFirstUnit(sint32 player)
{
    for (auto* obs : m_observers) {
        obs->OnAutoSelectFirstUnit(player);
    }
}

void GameObserverRegistry::NotifyHideMainUI()
{
    for (auto* obs : m_observers) {
        obs->OnHideMainUI();
    }
}

void GameObserverRegistry::NotifyUpdatePlayerEndProgress(sint32 player)
{
    for (auto* obs : m_observers) {
        obs->OnUpdatePlayerEndProgress(player);
    }
}

void GameObserverRegistry::NotifyAdvanceListReload(sint32 player)
{
    for (auto* obs : m_observers) {
        obs->OnAdvanceListReload(player);
    }
}

void GameObserverRegistry::NotifyShowSpaceButton(sint32 player)
{
    for (auto* obs : m_observers) {
        obs->OnShowSpaceButton(player);
    }
}

void GameObserverRegistry::NotifyUpdateUnitSelectionWindow(sint32 player)
{
    for (auto* obs : m_observers) {
        obs->OnUpdateUnitSelectionWindow(player);
    }
}

void GameObserverRegistry::NotifyUpdateCityStatusWindow(sint32 player)
{
    for (auto* obs : m_observers) {
        obs->OnUpdateCityStatusWindow(player);
    }
}

void GameObserverRegistry::NotifyUpdateMainControlPanel(sint32 player)
{
    for (auto* obs : m_observers) {
        obs->OnUpdateMainControlPanel(player);
    }
}

void GameObserverRegistry::NotifySetGraphMinRound(sint32 round)
{
    for (auto* obs : m_observers) {
        obs->OnSetGraphMinRound(round);
    }
}

void GameObserverRegistry::NotifyMapResized()
{
    for (auto* obs : m_observers) {
        obs->OnMapResized();
    }
}

void GameObserverRegistry::NotifyRequestOpenGreatLibrary(sint32 entry, sint32 database)
{
    for (auto* obs : m_observers) {
        obs->OnRequestOpenGreatLibrary(entry, database);
    }
}

void GameObserverRegistry::NotifyRequestOpenScenarioEditor()
{
    for (auto* obs : m_observers) {
        obs->OnRequestOpenScenarioEditor();
    }
}

void GameObserverRegistry::NotifyRequestOpenScreen(sint32 screen)
{
    for (auto* obs : m_observers) {
        obs->OnRequestOpenScreen(screen);
    }
}

void GameObserverRegistry::NotifyRequestAttract(const char *control)
{
    for (auto* obs : m_observers) {
        obs->OnRequestAttract(control);
    }
}

void GameObserverRegistry::NotifyRequestStopAttract(const char *control)
{
    for (auto* obs : m_observers) {
        obs->OnRequestStopAttract(control);
    }
}

void GameObserverRegistry::NotifyRequestEditQueue(CityData *city)
{
    for (auto* obs : m_observers) {
        obs->OnRequestEditQueue(city);
    }
}

void GameObserverRegistry::NotifyRequestUnblankScreen()
{
    for (auto* obs : m_observers) {
        obs->OnRequestUnblankScreen();
    }
}

void GameObserverRegistry::NotifyCityEspionageDisplay(const Unit& city)
{
    for (auto* obs : m_observers) {
        obs->OnCityEspionageDisplay(city);
    }
}

void GameObserverRegistry::NotifyBlankScreenChanged(bool blank,
                                                    sint32 visiblePlayer,
                                                    sint32 researchingAdvance)
{
    for (auto* obs : m_observers) {
        obs->OnBlankScreenChanged(blank, visiblePlayer, researchingAdvance);
    }
}

void GameObserverRegistry::NotifyTutorialAddRecord(const char *title, sint32 index)
{
    for (auto* obs : m_observers) {
        obs->OnTutorialAddRecord(title, index);
    }
}

void GameObserverRegistry::NotifyTutorialRecreate()
{
    for (auto* obs : m_observers) {
        obs->OnTutorialRecreate();
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
