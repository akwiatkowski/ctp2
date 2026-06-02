/**
 * @file ui_game_observer.cpp
 * @brief UI observer implementation — forwards game events to existing UI globals.
 *
 * Compiled into both ctp2 and ctp2_headless targets, but only instantiated
 * in the UI build (civ3_main.cpp). Headless gets a no-op stub instead.
 *
 * This is a transitional file: once all UI globals are behind the observer
 * interface, we can delete the extern references and have the observer talk
 * directly to SDL/windows.
 */

#include "ctp/c3.h"
#include "ui/interface/backgroundwin.h"
#include "gs/core/game_observer.h"
#include "gs/gameobj/Player.h"
#include "gs/gameobj/Army.h"
#include "gs/utility/UnitDynArr.h"   // for Player::m_all_cities access
#include "gs/database/profileDB.h"   // profiledb_Get()
#include "gs/gameobj/Message.h"
#include "gs/gameobj/MessagePool.h"
#include "gs/gameobj/GameOver.h"
#include "gs/slic/SlicEngine.h"
#include "ui/aui_ctp2/SelItem.h"
#include "ui/aui_ctp2/background.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/ctp2_Window.h"
#include "ui/aui_ctp2/radarmap.h"
#include "gs/core/slic_screen.h"
#include "ui/interface/AttractWindow.h"
#include "ui/interface/cityespionage.h"
#include "ui/interface/citywindow.h"
#include "ui/interface/EditQueue.h"
#include "ui/interface/GreatLibraryTypes.h"   // DATABASE enum
#include "ui/interface/greatlibrary.h"
#include "ui/interface/tutorialwin.h"
#include "ui/interface/radarwindow.h"
#include "gfx/tilesys/tiledmap.h"
#include "ui/interface/c3dialogs.h"
#include "ui/interface/controlpanelwindow.h"
#include "ui/interface/infowin.h"
#include "ui/interface/MainControlPanel.h"
#include "ui/interface/messageactions.h"
#include "ui/interface/messagemodal.h"
#include "ui/interface/messagewin.h"
#include "ui/interface/EndgameWindow.h"      // Wave C: endgamewindow_Initialize/Cleanup
#include "ui/interface/messageiconwindow.h"
#include "ui/interface/messagewindow.h"
#include "ui/interface/sci_advancescreen.h"
#include "ui/interface/sciencewin.h"
#include "ui/interface/screenutils.h"
#include "ui/interface/trademanager.h"
#include "ui/interface/victorywin.h"
#include "ui/interface/dipwizard.h"
#include "gs/diplomacy/diplomacy_types.h"
#include "gfx/spritesys/director.h"
#include "sound/soundmanager.h"
#include "sound/gamesounds.h"
#include "ui/aui_ctp2/ui_unit_actor_registry.h"

extern MessageWindow         *g_currentMessageWindow;
extern MessageModal          *g_modalMessage;

class UIGameObserver : public IGameObserver {
public:
    // --- Turn lifecycle ---
    void OnTurnStart(sint32 player) override
    {
        if (controlpanel_Get()) {
            controlpanel_Get()->UpdatePlayerBeginProgress(player);
        }
    }

    void OnBuildPhaseComplete(sint32 player) override
    {
        if (controlpanel_Get() && controlpanel_Get()->GetWindow()) {
            controlpanel_Get()->GetWindow()->ShouldDraw(TRUE);
        }
    }

    // --- City events ---
    void OnCityFounded(sint32 player, const Unit& city,
                       const MapPoint& pos, sint32 cause) override
    {
        if (g_soundManager) {
            sint32 visiblePlayer = selitem_Get()->GetVisiblePlayer();
            if (visiblePlayer == player) {
                g_soundManager->AddSound(
                    SOUNDTYPE_SFX, (uint32)0,
                    gamesounds_GetGameSoundID(GAMESOUNDS_GOODY_CITY),
                    pos.x, pos.y);
            }
        }
    }

    void OnCityCaptured(const Unit& city, sint32 newOwner,
                        const MapPoint& pos) override
    {
        if (director_Get()) {
            director_Get()->AddCenterMap(pos);
        }
    }

    void OnCityOwnerReset(const Unit& city) override
    {
        // UnitData::ResetCityOwner hook — tell the city panel its city
        // changed hands so the open window updates its layout.
        CityWindow::NotifyCityCaptured(const_cast<Unit &>(city));
    }

    void OnWonderBuilt(const Unit& city, sint32 wonder) override
    {
        if (director_Get()) {
            director_Get()->AddPlayWonderMovie(wonder);
        }
    }

    // --- Unit lifecycle (Phase 3 slice 7a: observer-event pivot foundation) ---
    // Shadow registry so future per-mutation events (OnUnitMoved, …) can
    // look up the actor without going through gs/-side Unit::GetActor().
    // Today we just mirror UnitData::m_actor; once the rest of the pivot
    // lands the registry becomes the canonical owner.
    void OnUnitSpawned(const Unit& unit, UnitState const * /*state*/) override
    {
        uiunitactorregistry_Get().Insert(unit, unit.GetActor());
    }

    void OnUnitDestroyed(const Unit& unit) override
    {
        uiunitactorregistry_Get().Remove(unit);
    }

    // --- Army / combat ---
    void OnArmyMove(const Army& army,
                    const MapPoint& from, const MapPoint& to) override
    {
        if (director_Get()) {
            director_Get()->AddCenterMap(to);
        }
    }

    void OnArmyRemoved(sint32 player, const Army& army) override
    {
        if (selitem_Get()) {
            selitem_Get()->RegisterRemovedArmy(player, army);
        }
    }

    void OnPlayerRemoved(sint32 player) override
    {
        if (selitem_Get()) {
            selitem_Get()->RemovePlayer(static_cast<PLAYER_INDEX>(player));
        }
    }

    void OnCombatStart(const Army& attacker, const Army& defender,
                       const MapPoint& pos) override
    {
        // Sound effects handled by OnCombatEnd based on outcome
    }

    void OnCombatEnd(const Army& attacker, const Army& defender,
                     bool attackerWon) override
    {
        if (!g_soundManager) return;

        sint32 attack_owner = attacker.GetOwner();
        sint32 defense_owner = defender.GetOwner();

        if (attackerWon) {
            if (selitem_Get()->IsPlayerVisible(defense_owner)) {
                g_soundManager->AddGameSound(GAMESOUNDS_VICTORY_FANFARE);
            }
            if (selitem_Get()->IsPlayerVisible(attack_owner)) {
                g_soundManager->AddGameSound(GAMESOUNDS_VICTORY_FANFARE);
            }
        } else {
            if (selitem_Get()->IsPlayerVisible(attack_owner)) {
                g_soundManager->AddGameSound(GAMESOUNDS_LOSE_PLAYER_BATTLE);
            }
            if (selitem_Get()->IsPlayerVisible(defense_owner)) {
                g_soundManager->AddGameSound(GAMESOUNDS_LOSE_PLAYER_BATTLE);
            }
        }
    }

    // --- Research ---
    void OnAdvanceResearched(sint32 player, sint32 advance) override {}

    void OnResearchAdvanceDialog(sint32 player, sint32 advance,
                                 const MBCHAR* text) override
    {
        if (director_Get()) {
            director_Get()->AddInvokeResearchAdvance(const_cast<MBCHAR*>(text));
        }
    }

    // --- Vision ---
    void OnVisionAdded(sint32 player, const MapPoint& pos, double range) override
    {
        if (director_Get()) {
            director_Get()->AddAddVision(pos, range);
        }
    }

    void OnVisionRemoved(sint32 player, const MapPoint& pos, double range) override
    {
        if (director_Get()) {
            director_Get()->AddRemoveVision(pos, range);
        }
    }

    void OnVisionCopied(sint32 fromPlayer, sint32 toPlayer) override
    {
        if (!director_Get() || !selitem_Get()) return;
        if (toPlayer != selitem_Get()->GetVisiblePlayer()) return;
        director_Get()->AddCopyVision();
    }

    // --- Government ---
    void OnGovernmentChanged(sint32 player, sint32 type) override
    {
        if (!director_Get() || !selitem_Get() || type == 0) return;
        if (player != selitem_Get()->GetVisiblePlayer()) return;
        director_Get()->AddGameSound(GAMESOUNDS_CHANGE_GOV);
    }

    // --- Wave C: endgame-statistics window ---
    void OnRequestEndGameShow(EndGame *endGame) override
    {
        if (!endgamewindow_Get())
        {
            endgamewindow_Initialize();
            if (EndGameWindow *egw = endgamewindow_Get(); egw && c3ui_Get())
            {
                c3ui_Get()->AddWindow(egw);
            }
        }
        if (EndGameWindow *egw = endgamewindow_Get())
        {
            egw->Update(endGame);
        }
    }

    void OnRequestEndGameClose() override
    {
        // endgamewindow_Cleanup() handles: stop sound, RemoveWindow,
        // RemoveHandler, delete, null.  Same end-state as the action-queued
        // path used by the exit-button callback, but synchronous — fine for
        // explicit close requests from gs/.
        endgamewindow_Cleanup();
    }

    // --- Wave C: modal alert & turn-start message refresh ---
    void OnRequestModalMessage(const Message& msg) override
    {
        // messagewin_CreateModalMessage takes Message by value (legacy
        // signature); pass through the const reference.
        messagewin_CreateModalMessage(msg);
    }

    void OnBeginTurnMessage(sint32 player) override
    {
        messagewin_BeginTurn(player);
    }

    // --- Game over (defeat/victory presentation) ---
    void OnGameOver(sint32 player, sint32 reason,
                    sint32 previouslyWon, sint32 previouslyLost) override
    {
        if (!selitem_Get() || player != selitem_Get()->GetVisiblePlayer())
            return;
        if (slicengine_Get() && slicengine_Get()->GetTutorialActive() &&
            slicengine_Get()->GetTutorialPlayer() != player)
            return;

        close_AllScreens();

        if (reason == GAME_OVER_LOST_SCIENCE ||
            reason == GAME_OVER_LOST_DIPLOMACY) {
            infowin_Initialize();
            victorywin_Initialize(k_VICWIN_DEFEAT);
            victorywin_DisplayWindow(k_VICWIN_DEFEAT);
        } else if (director_Get()) {
            director_Get()->CatchUp();
            director_Get()->AddPlayVictoryMovie(static_cast<GAME_OVER>(reason),
                                            previouslyWon, previouslyLost);
        }
    }

    // --- Trade ---
    void OnTradeChanged() override
    {
        TradeManager::Notify();
    }

    void OnForeignTradeBid(sint32 player, const Unit& fromCity,
                           const Unit& toCity, sint32 resource) override
    {
        // c3dialogs takes Unit&, not const Unit&.  These are PoD-ish handles
        // (just IDs into the unit pool); the dialog code doesn't mutate them.
        Unit f = fromCity;
        Unit t = toCity;
        c3dialogs_PostForeignTradeBidDialog(player, f, t, resource);
    }

    // --- Messages ---
    //
    // Encapsulates the three-way UI dispatch (director movie vs. modal alert
    // vs. message-list entry) plus the gating "is a message window currently
    // displaying a still-valid message?" check that used to live in
    // Player::AddMessage.  Player.cpp now just calls NotifyMessageReceived;
    // the UI observer owns the presentation policy.
    void OnMessageReceived(const Message& msg, sint32 player) override
    {
        Message localMsg = msg;
        bool noActiveMsgWindow =
            !g_currentMessageWindow ||
            !g_currentMessageWindow->GetMessage() ||
            !messagepool_Get()->IsValid(*g_currentMessageWindow->GetMessage());

        if (localMsg.UseDirector() && noActiveMsgWindow) {
            if (director_Get()) {
                director_Get()->AddMessage(localMsg);
            }
        } else if (localMsg.IsAlertBox()) {
            if (!messagewin_IsModalMessageDisplayed()) {
                messagewin_CreateModalMessage(localMsg);
            }
        } else {
            messagewin_CreateMessage(localMsg);
            if (localMsg.IsInstantMessage() &&
                selitem_Get() &&
                selitem_Get()->GetVisiblePlayer() == player &&
                noActiveMsgWindow) {
                localMsg.Show();
            }
        }
    }

    void OnMessagesRedisplay(sint32 player) override
    {
        // Player::RecreateMessageIcons collaborator: walk this player's
        // pending messages and re-render their icons + control-panel list.
        Player* pl = player_Get(player);
        if (!pl || !pl->m_messages) return;

        DynamicArray<Message> messages = *pl->m_messages;
        for (sint32 m = 0; m < messages.Num(); m++) {
            Message msg = messages.Access(m);
            if (msg.IsAlertBox()) {
                if (!messagewin_IsModalMessageDisplayed()) {
                    messagewin_CreateModalMessage(msg);
                }
            } else {
                messagewin_CreateMessage(msg);
            }
        }

        if (controlpanel_Get() && selitem_Get() &&
            player == selitem_Get()->GetVisiblePlayer()) {
            controlpanel_Get()->PopulateMessageList(player);
        }
    }

    void OnMessageShow(const Message& msg) override
    {
        Message m = msg;
        MessageData *data = m.AccessData();
        if (!data || !data->GetMessageWindow() ||
            !data->GetMessageWindow()->GetIconWindow()) {
            return;
        }
        if (c3ui_Get()) {
            c3ui_Get()->AddAction(new MessageOpenAction(
                data->GetMessageWindow()->GetIconWindow()));
        }
    }

    void OnMessageWindowDestroy(const Message& msg) override
    {
        Message m = msg;
        MessageData *data = m.AccessData();
        if (data && data->GetMessageWindow()) {
            messagewin_PrepareDestroyWindow(data->GetMessageWindow());
        } else if (g_modalMessage &&
                   g_modalMessage->GetMessage() &&
                   g_modalMessage->GetMessage()->m_id == m.m_id) {
            messagemodal_PrepareDestroyWindow();
        }
    }

    void OnMessageRead(const Message& msg) override
    {
        if (controlpanel_Get()) {
            controlpanel_Get()->SetMessageRead(const_cast<Message &>(msg));
        }
    }

    void OnMessageMinimize(const Message& msg) override
    {
        Message m = msg;
        MessageData *data = m.AccessData();
        if (!data || !data->GetMessageWindow()) return;
        data->GetMessageWindow()->ShowWindow(FALSE);
        if (data->GetMessageWindow()->GetIconWindow()) {
            data->GetMessageWindow()->GetIconWindow()->SetCurrentIconButton(NULL);
        }
    }

    void OnModalMessageDismissed(sint32 player) override
    {
        // Player::NotifyModalMessageDestroyed collaborator: find the next
        // pending alert-box message for this player and pop the modal.
        Player* pl = player_Get(player);
        if (!pl || !pl->m_messages) return;
        for (sint32 i = 0; i < pl->m_messages->Num(); i++) {
            if (pl->m_messages->Access(i).IsAlertBox()) {
                messagewin_CreateModalMessage(pl->m_messages->Access(i));
                break;
            }
        }
    }

    // ShowSpaceButton was a removed ControlPanelWindow feature.
    // Kept as no-op for interface compatibility.
    void OnShowSpaceButton(sint32 player) override {}

    // USS_UpdateAction and CSW_UpdateAction reference UI windows
    // (UnitSelection, CityStatus) that do not exist in the current
    // codebase. They are kept as no-ops for compatibility.
    void OnUpdateUnitSelectionWindow(sint32 player) override {}
    void OnUpdateCityStatusWindow(sint32 player) override {}

    void OnUpdateMainControlPanel(sint32 player) override
    {
        if (controlpanel_Get()) MainControlPanel::Update();
    }

    void OnHideMainUI() override
    {
        if (controlpanel_Get()) controlpanel_Get()->Hide();
        radarwindow_Hide();
        close_AllScreens();
    }

    void OnUpdatePlayerEndProgress(sint32 player) override
    {
        if (controlpanel_Get()) controlpanel_Get()->UpdatePlayerEndProgress(player);
    }

    void OnAutoSelectFirstUnit(sint32 player) override
    {
        if (!selitem_Get()) return;
        if (player != selitem_Get()->GetVisiblePlayer()) return;
        if (!profiledb_Get() || !profiledb_Get()->IsAutoSelectFirstUnit()) return;

        if (selitem_Get()->GetState() == SELECT_TYPE_NONE) {
            selitem_Get()->NextUnmovedUnit(TRUE);
        } else if (selitem_Get()->GetState() != SELECT_TYPE_LOCAL_ARMY) {
            selitem_Get()->MaybeAutoEndTurn(TRUE);
        }
    }

    void OnControlPanelRedraw(sint32 player) override
    {
        if (controlpanel_Get() && controlpanel_Get()->GetWindow()) {
            controlpanel_Get()->GetWindow()->DrawChildren();
        }
    }

    void OnRadarMapUpdate(sint32 player) override
    {
        if (!radar_map_Get() || !selitem_Get()) return;
        if (player != selitem_Get()->GetVisiblePlayer()) return;
        radar_map_Get()->Update();
    }

    void OnRadarMapRedrawTile(const MapPoint& pos) override
    {
        if (radar_map_Get()) radar_map_Get()->RedrawTile(&pos);
    }

    void OnAdvanceListReload(sint32 player) override
    {
        if (!selitem_Get() || player != selitem_Get()->GetVisiblePlayer())
            return;
        if (sci_advancescreen_isOnScreen()) {
            sci_advancescreen_loadList();
        }
    }

    void OnSetGraphMinRound(sint32 round) override
    {
        infowin_SetMinRoundForGraphs(round);
    }

    // --- SLIC-driven UI commands ---
    void OnRequestOpenGreatLibrary(sint32 entry, sint32 database) override
    {
        if (open_GreatLibrary()) {
            if (GreatLibrary *gl = greatlibrary_Get()) {
                gl->SetLibrary(entry, static_cast<DATABASE>(database));
            }
        }
    }

    void OnRequestOpenScenarioEditor() override
    {
        open_ScenarioEditor();
    }

    void OnRequestOpenScreen(sint32 screen) override
    {
        switch (screen) {
            case SLIC_SCREEN_CIV:       open_CivStatus();          break;
            case SLIC_SCREEN_CITY:      open_CityStatus();         break;
            case SLIC_SCREEN_UNIT:      open_UnitStatus();         break;
            case SLIC_SCREEN_SCIENCE:   open_ScienceStatus();      break;
            case SLIC_SCREEN_DIPLOMACY: open_Diplomacy();          break;
            case SLIC_SCREEN_TRADE:     open_TradeStatus();        break;
            case SLIC_SCREEN_INFO:      open_InfoScreen();         break;
            case SLIC_SCREEN_OPTIONS:   open_OptionsScreen(1);     break;
        }
    }

    void OnRequestAttract(const char *control) override
    {
        if (!attractwindow_Get()) {
            AttractWindow::Initialize();
        }
        if (AttractWindow *aw = attractwindow_Get()) {
            aw->HighlightControl(const_cast<char *>(control));
        }
    }

    void OnRequestStopAttract(const char *control) override
    {
        if (!attractwindow_Get()) {
            AttractWindow::Initialize();
        }
        if (AttractWindow *aw = attractwindow_Get()) {
            aw->RemoveControl(const_cast<char *>(control));
        }
    }

    void OnRequestEditQueue(CityData *city) override
    {
        EditQueue::Display(city);
    }

    void OnRequestUnblankScreen() override
    {
        if (!selitem_Get()) return;
        selitem_Get()->KeyboardSelectFirstUnit();
        sint32 visible = selitem_Get()->GetVisiblePlayer();
        if (selitem_Get()->GetState() != SELECT_TYPE_LOCAL_ARMY &&
            visible >= 0 && player_Get(visible) &&
            player_Get(visible)->m_all_cities->Num() > 0) {
            selitem_Get()->SetSelectCity(player_Get(visible)->m_all_cities->Access(0));
            if (director_Get()) {
                director_Get()->AddCenterMap(player_Get(visible)->m_all_cities->Access(0).RetPos());
            }
        }
        if (director_Get()) {
            director_Get()->AddCenterMap(selitem_Get()->GetCurSelectPos());
        }
        radarwindow_Show();
        if (controlpanel_Get()) {
            controlpanel_Get()->Show();
        }
    }

    void OnCityEspionageDisplay(const Unit& city) override
    {
        CityEspionage::Display(city);
    }

    void OnBlankScreenChanged(bool blank, sint32 visiblePlayer,
                              sint32 researchingAdvance) override
    {
        if (radar_map_Get()) radar_map_Get()->Update();

        if (blank && controlpanel_Get()) {
            MainControlPanel::Blank();
            if (GreatLibrary *gl = greatlibrary_Get()) {
                gl->ClearHistory();
            }
        } else if (controlpanel_Get()) {
            MainControlPanel::UpdatePlayer(visiblePlayer);
            MainControlPanel::UpdateCityList();
            MainControlPanel::Update();
            if (GreatLibrary *gl = greatlibrary_Get()) {
                gl->SetLibrary(researchingAdvance, DATABASE_ADVANCES);
            }
        }
    }

    void OnTutorialAddRecord(const char *title, sint32 index) override
    {
        if (tutorialwin_Get() && title) {
            tutorialwin_Get()->AddToList(const_cast<char *>(title), index);
        }
    }

    void OnTutorialRecreate() override
    {
        // No-op here; SlicEngine still walks its own record list and
        // emits OnTutorialAddRecord per entry.
    }

    void OnMapResized() override
    {
        // gameinit_ResetMapSize has already rebuilt tiledmap_Get() and the
        // world pools.  Load the tileset graphics, recreate the radar
        // window, and redraw the background — all UI work that the engine
        // build skips.
        radarwindow_Cleanup();

        if (tiledmap_Get()) {
            tiledmap_Get()->LoadTileset();
        }

        if (tiledmap_Get() && background_Get()) {
            RECT rect = {
                background_Get()->X(),
                background_Get()->Y(),
                background_Get()->X() + background_Get()->Width(),
                background_Get()->Y() + background_Get()->Height()
            };
            tiledmap_Get()->Initialize(&rect);
            tiledmap_Get()->Refresh();
        }

        radarwindow_Initialize();
        radarwindow_Display();

        if (tiledmap_Get()) {
            tiledmap_Get()->PostProcessMap();
            tiledmap_Get()->Refresh();
        }

        if (background_Get()) {
            background_Get()->Draw();
        }
    }

    // --- UI refresh ---
    void OnUpdateScienceWindow(sint32 player) override
    {
        if (c3ui_Get() && player == selitem_Get()->GetVisiblePlayer()) {
            c3ui_Get()->AddAction(new SW_UpdateAction);
        }
    }

    void OnUpdateCityList() override
    {
        MainControlPanel::UpdateCityList();
    }

    void OnUpdateUnitPanel(sint32 player) override
    {
        if (controlpanel_Get() && player == selitem_Get()->GetVisiblePlayer()) {
            controlpanel_Get()->PopulateMessageList(player);
        }
    }

    void OnUpdateControlPanel(sint32 player) override
    {
        if (controlpanel_Get()) {
            controlpanel_Get()->UpdatePlayerBeginProgress(player);
        }
    }

    void OnSelectedCity(sint32 player) override
    {
        if (controlpanel_Get() && player == selitem_Get()->GetVisiblePlayer()) {
            MainControlPanel::SelectedCity();
        }
    }

    void OnUpdateMessages(sint32 player) override
    {
        if (controlpanel_Get() && player == selitem_Get()->GetVisiblePlayer()) {
            controlpanel_Get()->TileImpPanelRedisplay();
        }
    }
};

static UIGameObserver s_uiGameObserver;

void RegisterUIGameObserver()
{
    g_gameObservers->Register(&s_uiGameObserver);
}
