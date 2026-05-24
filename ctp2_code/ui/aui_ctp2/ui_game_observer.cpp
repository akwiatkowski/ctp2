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
#include "gs/core/game_observer.h"
#include "gs/gameobj/Player.h"
#include "gs/gameobj/Army.h"
#include "gs/gameobj/Message.h"
#include "gs/gameobj/MessagePool.h"
#include "gs/gameobj/GameOver.h"
#include "gs/slic/SlicEngine.h"
#include "ui/aui_ctp2/SelItem.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/ctp2_Window.h"
#include "ui/aui_ctp2/radarmap.h"
#include "ui/interface/c3dialogs.h"
#include "ui/interface/controlpanelwindow.h"
#include "ui/interface/infowin.h"
#include "ui/interface/MainControlPanel.h"
#include "ui/interface/messagewin.h"
#include "ui/interface/messagewindow.h"
#include "ui/interface/sci_advancescreen.h"
#include "ui/interface/sciencewin.h"
#include "ui/interface/screenutils.h"
#include "ui/interface/trademanager.h"
#include "ui/interface/victorywin.h"
#include "gfx/spritesys/director.h"
#include "sound/soundmanager.h"
#include "sound/gamesounds.h"

extern ControlPanelWindow    *g_controlPanel;
extern C3UI                  *g_c3ui;
extern MessageWindow         *g_currentMessageWindow;

class UIGameObserver : public IGameObserver {
public:
    // --- Turn lifecycle ---
    void OnTurnStart(sint32 player) override
    {
        if (g_controlPanel) {
            g_controlPanel->UpdatePlayerBeginProgress(player);
        }
    }

    void OnBuildPhaseComplete(sint32 player) override
    {
        if (g_controlPanel && g_controlPanel->GetWindow()) {
            g_controlPanel->GetWindow()->ShouldDraw(TRUE);
        }
    }

    // --- City events ---
    void OnCityFounded(sint32 player, const Unit& city,
                       const MapPoint& pos, sint32 cause) override
    {
        if (g_soundManager) {
            sint32 visiblePlayer = g_selected_item->GetVisiblePlayer();
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
        if (g_director) {
            g_director->AddCenterMap(pos);
        }
    }

    void OnWonderBuilt(const Unit& city, sint32 wonder) override
    {
        if (g_director) {
            g_director->AddPlayWonderMovie(wonder);
        }
    }

    // --- Army / combat ---
    void OnArmyMove(const Army& army,
                    const MapPoint& from, const MapPoint& to) override
    {
        if (g_director) {
            g_director->AddCenterMap(to);
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
            if (g_selected_item->IsPlayerVisible(defense_owner)) {
                g_soundManager->AddGameSound(GAMESOUNDS_VICTORY_FANFARE);
            }
            if (g_selected_item->IsPlayerVisible(attack_owner)) {
                g_soundManager->AddGameSound(GAMESOUNDS_VICTORY_FANFARE);
            }
        } else {
            if (g_selected_item->IsPlayerVisible(attack_owner)) {
                g_soundManager->AddGameSound(GAMESOUNDS_LOSE_PLAYER_BATTLE);
            }
            if (g_selected_item->IsPlayerVisible(defense_owner)) {
                g_soundManager->AddGameSound(GAMESOUNDS_LOSE_PLAYER_BATTLE);
            }
        }
    }

    // --- Research ---
    void OnAdvanceResearched(sint32 player, sint32 advance) override {}

    void OnResearchAdvanceDialog(sint32 player, sint32 advance,
                                 const MBCHAR* text) override
    {
        if (g_director) {
            g_director->AddInvokeResearchAdvance(const_cast<MBCHAR*>(text));
        }
    }

    // --- Vision ---
    void OnVisionAdded(sint32 player, const MapPoint& pos, double range) override
    {
        if (g_director) {
            g_director->AddAddVision(pos, range);
        }
    }

    void OnVisionRemoved(sint32 player, const MapPoint& pos, double range) override
    {
        if (g_director) {
            g_director->AddRemoveVision(pos, range);
        }
    }

    void OnVisionCopied(sint32 fromPlayer, sint32 toPlayer) override
    {
        if (!g_director || !g_selected_item) return;
        if (toPlayer != g_selected_item->GetVisiblePlayer()) return;
        g_director->AddCopyVision();
    }

    // --- Government ---
    void OnGovernmentChanged(sint32 player, sint32 type) override
    {
        if (!g_director || !g_selected_item || type == 0) return;
        if (player != g_selected_item->GetVisiblePlayer()) return;
        g_director->AddGameSound(GAMESOUNDS_CHANGE_GOV);
    }

    // --- Game over (defeat/victory presentation) ---
    void OnGameOver(sint32 player, sint32 reason,
                    sint32 previouslyWon, sint32 previouslyLost) override
    {
        if (!g_selected_item || player != g_selected_item->GetVisiblePlayer())
            return;
        if (g_slicEngine && g_slicEngine->GetTutorialActive() &&
            g_slicEngine->GetTutorialPlayer() != player)
            return;

        close_AllScreens();

        if (reason == GAME_OVER_LOST_SCIENCE ||
            reason == GAME_OVER_LOST_DIPLOMACY) {
            infowin_Initialize();
            victorywin_Initialize(k_VICWIN_DEFEAT);
            victorywin_DisplayWindow(k_VICWIN_DEFEAT);
        } else if (g_director) {
            g_director->CatchUp();
            g_director->AddPlayVictoryMovie(static_cast<GAME_OVER>(reason),
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
            !g_theMessagePool->IsValid(*g_currentMessageWindow->GetMessage());

        if (localMsg.UseDirector() && noActiveMsgWindow) {
            if (g_director) {
                g_director->AddMessage(localMsg);
            }
        } else if (localMsg.IsAlertBox()) {
            if (!messagewin_IsModalMessageDisplayed()) {
                messagewin_CreateModalMessage(localMsg);
            }
        } else {
            messagewin_CreateMessage(localMsg);
            if (localMsg.IsInstantMessage() &&
                g_selected_item &&
                g_selected_item->GetVisiblePlayer() == player &&
                noActiveMsgWindow) {
                localMsg.Show();
            }
        }
    }

    void OnMessagesRedisplay(sint32 player) override
    {
        // Player::RecreateMessageIcons collaborator: walk this player's
        // pending messages and re-render their icons + control-panel list.
        Player* pl = g_player[player];
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

        if (g_controlPanel && g_selected_item &&
            player == g_selected_item->GetVisiblePlayer()) {
            g_controlPanel->PopulateMessageList(player);
        }
    }

    void OnModalMessageDismissed(sint32 player) override
    {
        // Player::NotifyModalMessageDestroyed collaborator: find the next
        // pending alert-box message for this player and pop the modal.
        Player* pl = g_player[player];
        if (!pl || !pl->m_messages) return;
        for (sint32 i = 0; i < pl->m_messages->Num(); i++) {
            if (pl->m_messages->Access(i).IsAlertBox()) {
                messagewin_CreateModalMessage(pl->m_messages->Access(i));
                break;
            }
        }
    }

    // OnUpdateScienceSubWindows and OnSpaceButtonAvailable existed in the
    // observer interface for a moment but the only Player.cpp call sites
    // referencing them (USS_UpdateAction, CSW_UpdateAction,
    // ControlPanelWindow::ShowSpaceButton) sit inside a long-dead #if 0
    // block.  No live caller, no implementation needed.

    void OnControlPanelRedraw(sint32 player) override
    {
        if (g_controlPanel && g_controlPanel->GetWindow()) {
            g_controlPanel->GetWindow()->DrawChildren();
        }
    }

    void OnRadarMapUpdate(sint32 player) override
    {
        if (!g_radarMap || !g_selected_item) return;
        if (player != g_selected_item->GetVisiblePlayer()) return;
        g_radarMap->Update();
    }

    void OnAdvanceListReload(sint32 player) override
    {
        if (!g_selected_item || player != g_selected_item->GetVisiblePlayer())
            return;
        if (sci_advancescreen_isOnScreen()) {
            sci_advancescreen_loadList();
        }
    }

    // --- UI refresh ---
    void OnUpdateScienceWindow(sint32 player) override
    {
        if (g_c3ui && player == g_selected_item->GetVisiblePlayer()) {
            g_c3ui->AddAction(new SW_UpdateAction);
        }
    }

    void OnUpdateCityList() override
    {
        MainControlPanel::UpdateCityList();
    }

    void OnUpdateUnitPanel(sint32 player) override
    {
        if (g_controlPanel && player == g_selected_item->GetVisiblePlayer()) {
            g_controlPanel->PopulateMessageList(player);
        }
    }

    void OnUpdateControlPanel(sint32 player) override
    {
        if (g_controlPanel) {
            g_controlPanel->UpdatePlayerBeginProgress(player);
        }
    }

    void OnUpdateMessages(sint32 player) override
    {
        if (g_controlPanel && player == g_selected_item->GetVisiblePlayer()) {
            g_controlPanel->TileImpPanelRedisplay();
        }
    }
};

static UIGameObserver s_uiGameObserver;

void RegisterUIGameObserver()
{
    g_gameObservers->Register(&s_uiGameObserver);
}
