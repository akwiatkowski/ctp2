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
#include "ui/aui_ctp2/SelItem.h"
#include "ui/interface/controlpanelwindow.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/ctp2_Window.h"
#include "ui/interface/sciencewin.h"
#include "ui/interface/MainControlPanel.h"
#include "gfx/spritesys/director.h"
#include "sound/soundmanager.h"
#include "sound/gamesounds.h"

extern ControlPanelWindow    *g_controlPanel;
extern C3UI                  *g_c3ui;

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
    void OnAdvanceResearched(sint32 player, sint32 advance) override
    {
        if (g_director) {
            // g_director->AddInvokeResearchAdvance(...) — called from Player.cpp
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
