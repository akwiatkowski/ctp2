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
class CityData;
class Message;
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
    virtual void OnCityOwnerReset(const Unit& city) {}
    virtual void OnWonderBuilt(const Unit& city, sint32 wonder) {}

    // --- Army / combat events ---
    virtual void OnArmyMove(const Army& army,
                            const MapPoint& from, const MapPoint& to) {}
    virtual void OnArmyRemoved(sint32 player, const Army& army) {}
    virtual void OnCombatStart(const Army& attacker, const Army& defender,
                               const MapPoint& pos) {}
    virtual void OnCombatEnd(const Army& attacker, const Army& defender,
                             bool attackerWon) {}

    // --- Player lifecycle ---
    virtual void OnPlayerRemoved(sint32 player) {}

    // --- Research ---
    virtual void OnAdvanceResearched(sint32 player, sint32 advance) {}
    virtual void OnResearchAdvanceDialog(sint32 player, sint32 advance,
                                         const MBCHAR* text) {}

    // --- Vision / map reveal ---
    virtual void OnVisionAdded(sint32 player, const MapPoint& pos, double range) {}
    virtual void OnVisionRemoved(sint32 player, const MapPoint& pos, double range) {}
    virtual void OnVisionCopied(sint32 fromPlayer, sint32 toPlayer) {}

    // --- Government / sound effects driven by Player ---
    virtual void OnGovernmentChanged(sint32 player, sint32 type) {}

    // --- Game over / victory presentation ---
    virtual void OnGameOver(sint32 player, sint32 reason,
                            sint32 previouslyWon, sint32 previouslyLost) {}

    // --- Trade ---
    virtual void OnTradeChanged() {}
    virtual void OnForeignTradeBid(sint32 player, const Unit& fromCity,
                                   const Unit& toCity, sint32 resource) {}

    // --- Player messages (alert boxes, instant messages, message list) ---
    virtual void OnMessageReceived(const Message& msg, sint32 player) {}
    virtual void OnMessagesRedisplay(sint32 player) {}
    virtual void OnModalMessageDismissed(sint32 player) {}
    virtual void OnMessageShow(const Message& msg) {}
    virtual void OnMessageMinimize(const Message& msg) {}
    virtual void OnMessageWindowDestroy(const Message& msg) {}
    virtual void OnMessageRead(const Message& msg) {}

    // --- UI refresh requests (no-ops in headless) ---
    virtual void OnUpdateScienceWindow(sint32 player) {}
    virtual void OnUpdateCityList() {}
    virtual void OnUpdateUnitPanel(sint32 player) {}
    virtual void OnUpdateControlPanel(sint32 player) {}
    virtual void OnControlPanelRedraw(sint32 player) {}
    virtual void OnUpdateMessages(sint32 player) {}
    virtual void OnSelectedCity(sint32 player) {}
    virtual void OnRadarMapUpdate(sint32 player) {}
    virtual void OnRadarMapRedrawTile(const MapPoint& pos) {}
    virtual void OnAutoSelectFirstUnit(sint32 player) {}
    virtual void OnHideMainUI() {}
    virtual void OnUpdatePlayerEndProgress(sint32 player) {}
    virtual void OnAdvanceListReload(sint32 player) {}
    virtual void OnShowSpaceButton(sint32 player) {}
    virtual void OnUpdateUnitSelectionWindow(sint32 player) {}
    virtual void OnUpdateCityStatusWindow(sint32 player) {}
    virtual void OnUpdateMainControlPanel(sint32 player) {}

    // --- Stat-graph time window ---
    // Set after game init / scenario load so the score/research graphs in
    // infowin only plot from the right starting round.
    virtual void OnSetGraphMinRound(sint32 round) {}

    // --- Map size reset (custom-map load) ---
    // Fired after gameinit_ResetMapSize has rebuilt the world pools and
    // tiledMap, so the UI observer can reload tileset graphics, recreate
    // the radar window, and redraw the background.
    virtual void OnMapResized() {}

    // --- SLIC-driven UI commands ---
    // Triggered when a SLIC script calls one of the UI-opening built-ins
    // (LibraryUnit, OpenScenarioEditor, Attract, etc.).  Headless ignores
    // them; UI observer pops the relevant screen.
    virtual void OnRequestOpenGreatLibrary(sint32 entry, sint32 database) {}
    virtual void OnRequestOpenScreen(sint32 screen) {}
    virtual void OnRequestOpenScenarioEditor() {}
    virtual void OnRequestAttract(const char *control) {}
    virtual void OnRequestStopAttract(const char *control) {}
    virtual void OnRequestEditQueue(CityData *city) {}
    virtual void OnRequestUnblankScreen() {}
    virtual void OnCityEspionageDisplay(const Unit& city) {}

    // --- Engine-side blank-screen toggle ---
    // Fired by SlicEngine::BlankScreen when the SLIC engine asks for the
    // game UI to be blanked / un-blanked.  Headless ignores it; UI observer
    // updates the control panel, great-library, etc.
    virtual void OnBlankScreenChanged(bool blank,
                                      sint32 visiblePlayer,
                                      sint32 researchingAdvance) {}

    // --- Tutorial window state (driven from SlicEngine) ---
    virtual void OnTutorialAddRecord(const char *title, sint32 index) {}
    virtual void OnTutorialRecreate() {}
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
    void NotifyCityOwnerReset(const Unit& city);
    void NotifyWonderBuilt(const Unit& city, sint32 wonder);

    // --- Army / combat ---
    void NotifyArmyMove(const Army& army,
                        const MapPoint& from, const MapPoint& to);
    void NotifyArmyRemoved(sint32 player, const Army& army);
    void NotifyCombatStart(const Army& attacker, const Army& defender,
                           const MapPoint& pos);
    void NotifyCombatEnd(const Army& attacker, const Army& defender,
                         bool attackerWon);

    // --- Player lifecycle ---
    void NotifyPlayerRemoved(sint32 player);

    // --- Research ---
    void NotifyAdvanceResearched(sint32 player, sint32 advance);
    void NotifyResearchAdvanceDialog(sint32 player, sint32 advance,
                                     const MBCHAR* text);

    // --- Vision ---
    void NotifyVisionAdded(sint32 player, const MapPoint& pos, double range);
    void NotifyVisionRemoved(sint32 player, const MapPoint& pos, double range);
    void NotifyVisionCopied(sint32 fromPlayer, sint32 toPlayer);

    // --- Government ---
    void NotifyGovernmentChanged(sint32 player, sint32 type);

    // --- Game over ---
    void NotifyGameOver(sint32 player, sint32 reason,
                        sint32 previouslyWon, sint32 previouslyLost);

    // --- Trade ---
    void NotifyTradeChanged();
    void NotifyForeignTradeBid(sint32 player, const Unit& fromCity,
                               const Unit& toCity, sint32 resource);

    // --- Messages ---
    void NotifyMessageReceived(const Message& msg, sint32 player);
    void NotifyMessagesRedisplay(sint32 player);
    void NotifyModalMessageDismissed(sint32 player);
    void NotifyMessageShow(const Message& msg);
    void NotifyMessageMinimize(const Message& msg);
    void NotifyMessageWindowDestroy(const Message& msg);
    void NotifyMessageRead(const Message& msg);

    // --- UI refresh ---
    void NotifyUpdateScienceWindow(sint32 player);
    void NotifyUpdateCityList();
    void NotifyUpdateUnitPanel(sint32 player);
    void NotifyUpdateControlPanel(sint32 player);
    void NotifyControlPanelRedraw(sint32 player);
    void NotifyUpdateMessages(sint32 player);
    void NotifySelectedCity(sint32 player);
    void NotifyRadarMapUpdate(sint32 player);
    void NotifyRadarMapRedrawTile(const MapPoint& pos);
    void NotifyAutoSelectFirstUnit(sint32 player);
    void NotifyHideMainUI();
    void NotifyUpdatePlayerEndProgress(sint32 player);
    void NotifyAdvanceListReload(sint32 player);
    void NotifyShowSpaceButton(sint32 player);
    void NotifyUpdateUnitSelectionWindow(sint32 player);
    void NotifyUpdateCityStatusWindow(sint32 player);
    void NotifyUpdateMainControlPanel(sint32 player);
    void NotifySetGraphMinRound(sint32 round);
    void NotifyMapResized();

    void NotifyRequestOpenGreatLibrary(sint32 entry, sint32 database);
    void NotifyRequestOpenScreen(sint32 screen);
    void NotifyRequestOpenScenarioEditor();
    void NotifyRequestAttract(const char *control);
    void NotifyRequestStopAttract(const char *control);
    void NotifyRequestEditQueue(CityData *city);
    void NotifyRequestUnblankScreen();
    void NotifyCityEspionageDisplay(const Unit& city);
    void NotifyBlankScreenChanged(bool blank, sint32 visiblePlayer,
                                  sint32 researchingAdvance);
    void NotifyTutorialAddRecord(const char *title, sint32 index);
    void NotifyTutorialRecreate();

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
