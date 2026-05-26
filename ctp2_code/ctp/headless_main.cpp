//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Headless entry point — game simulation without UI/audio/render
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ctp/civapp.h"
#include "gs/utility/gameinit.h"
#include "gs/database/profileDB.h"
#include "gs/fileio/civscenarios.h"
#include "gs/gameobj/Player.h"
#include "gs/world/World.h"
#include "gs/utility/newturncount.h"
#include "gs/utility/TurnCnt.h"
#include "gs/events/GameEventManager.h"
#include "gs/core/game_observer.h"            // g_gameObservers
#include "gs/core/game_observer_registration.h"
#include "gs/core/player_view.h"              // player_view::RegisterCurPlayer
#include "gs/fileio/gamefile.h"               // GameFile::SaveGame / RestoreGame
#include "gs/gameobj/Score.h"                 // Score::GetTotalScore
#include "gs/gameobj/CityData.h"              // CityData::PopCount
#include "gs/gameobj/Unit.h"                  // Unit::GetName / GetPos / CD
#include "gs/utility/UnitDynArr.h"            // UnitDynamicArray
#include "gs/gameobj/Events.h"                // GEV_AiBeginTurn / GEV_AiBeginMapAnalysis
#include "gs/events/GameEventManager.h"       // g_gevManager
#include "ai/ctpai.h"                         // CtpAi::BeginDiplomacy

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern CivApp *g_civApp;
extern bool    g_headlessMode;
extern sint32  g_runInBackground;

// Headless mode needs a CurPlayer callback because CtpAi::BeginTurn and
// BeginMapAnalysis assert(player == player_view::CurPlayer()).  In the
// interactive game CurPlayer is backed by g_selected_item; headless has
// no selected item, so we track the currently processing player manually.
static sint32 s_headlessCurPlayer = 0;
static sint32 HeadlessCurPlayer() { return s_headlessCurPlayer; }

static void print_usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s [options]\n"
        "Options:\n"
        "  --new-game              Start a new game immediately\n"
        "  --players N             Number of AI players (default: 3)\n"
        "  --turns N               Run N turns then exit (default: 10)\n"
        "  --seed N                Pin RNG seed for determinism\n"
        "  --save-interval N       Save every N turns (currently unimplemented)\n"
        "  --save-game PATH        After running turns, save to PATH then exit\n"
        "  --load-game PATH        Load saved game from PATH instead of --new-game\n"
        "  --export-metrics PATH   After running turns, dump per-player and per-city\n"
        "                          metrics as CSV to PATH (or '-' for stdout)\n"
        "  --help                  Show this message\n",
        prog);
}

int main(int argc, char **argv)
{
    fprintf(stderr, "[HEADLESS] CTP2 Headless Engine starting\n");

    // Parse arguments
    bool newGame = false;
    sint32 numPlayers = 3;
    sint32 maxTurns = 10;
    sint32 saveInterval = 0;
    sint32 seed = 42;
    const char *saveGamePath = nullptr;
    const char *loadGamePath = nullptr;
    const char *exportMetricsPath = nullptr;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--new-game") == 0) {
            newGame = true;
        } else if (strcmp(argv[i], "--players") == 0 && i + 1 < argc) {
            numPlayers = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--turns") == 0 && i + 1 < argc) {
            maxTurns = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            seed = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--save-interval") == 0 && i + 1 < argc) {
            saveInterval = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--save-game") == 0 && i + 1 < argc) {
            saveGamePath = argv[++i];
        } else if (strcmp(argv[i], "--load-game") == 0 && i + 1 < argc) {
            loadGamePath = argv[++i];
        } else if (strcmp(argv[i], "--export-metrics") == 0 && i + 1 < argc) {
            exportMetricsPath = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "[HEADLESS] Unknown argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    // Force headless mode before any initialization
    g_headlessMode = true;
    g_runInBackground = true;

    g_civApp = new CivApp();

    fprintf(stderr, "[HEADLESS] Initializing engine...\n");
    sint32 err = g_civApp->InitializeEngine();
    if (err != 0) {
        fprintf(stderr, "[HEADLESS] Engine initialization failed: %d\n", err);
        return 1;
    }

    // Load game file list and databases (normally done inside InitializeApp)
    fprintf(stderr, "[HEADLESS] Loading game files...\n");
    if (!gameinit_InitializeGameFiles()) {
        fprintf(stderr, "[HEADLESS] gameinit_InitializeGameFiles failed\n");
        return 1;
    }

    fprintf(stderr, "[HEADLESS] Loading databases...\n");
    if (!g_civApp->InitializeAppDB()) {
        fprintf(stderr, "[HEADLESS] InitializeAppDB failed\n");
        return 1;
    }

    CivScenarios::Initialize();

    fprintf(stderr, "[HEADLESS] Engine + DBs initialized OK\n");

    RegisterHeadlessGameObserver();
    // Headless still links SelItem.cpp (it's in game_core_sources) and many
    // non-UI files — notably newturncount.cpp — dereference g_selected_item
    // directly.  Until those callers are migrated to player_view::CurPlayer
    // etc., the headless build needs to allocate a SelectedItem instance.
    // Reuse the same SelectedItem-backed callbacks the UI build uses.
    RegisterUIPlayerView();
    // Override CurPlayer so AI asserts (player == CurPlayer()) pass and
    // NewTurnCount::GetCurrentRound() returns the active player's round.
    player_view::RegisterCurPlayer(&HeadlessCurPlayer);
    fprintf(stderr, "[HEADLESS] observers + player_view registered\n");

    if (loadGamePath) {
        fprintf(stderr, "[HEADLESS] Loading saved game from %s\n", loadGamePath);
        GameFile::RestoreGame(loadGamePath);
        fprintf(stderr, "[HEADLESS] RestoreGame returned (state may or may not be valid)\n");
    }

    if (newGame || loadGamePath) {
        if (newGame) {
            fprintf(stderr, "[HEADLESS] Starting new game (players=%d, seed=%d)...\n",
                    numPlayers, seed);

            // Set player count and seed in ProfileDB
            g_theProfileDB->SetNPlayers(numPlayers);

            // Enable AI for non-human players — otherwise the turn pipeline
            // ticks but no decisions are dispatched (settlers never settle,
            // builders never build, score stays flat).
            g_theProfileDB->SetAI(TRUE);

            // Use the headless game init path (no UI windows)
            err = g_civApp->InitializeGameHeadless();
            if (err != 0) {
                fprintf(stderr, "[HEADLESS] Game initialization failed: %d\n", err);
                return 1;
            }

            // Force ALL players to AI-driven. By default player 1 (typically
            // "Julius Caesar") is created as PLAYER_TYPE_HUMAN, and CtpAi
            // skips human-controlled players — they sit on their starting
            // settler forever and never found a city.  In headless there
            // is no human, so promote everyone to ROBOT.
            for (sint32 p = 0; p < k_MAX_PLAYERS; ++p) {
                if (g_player[p]) g_player[p]->SetPlayerType(PLAYER_TYPE_ROBOT);
            }

            fprintf(stderr, "[HEADLESS] Game initialized OK — running %d turns\n", maxTurns);
        } else {
            fprintf(stderr, "[HEADLESS] Loaded — running %d turns\n", maxTurns);
        }

        // Run turns
        for (sint32 t = 0; t < maxTurns; ++t) {
            fprintf(stderr, "[HEADLESS] Turn %d / %d\n", t + 1, maxTurns);

            // Process one turn for each active player.  Drive AI through
            // the same event pipeline the interactive game uses — mirrors
            // the body of STDEHANDLER(BeginTurnEvent) in TurnCntEvent.cpp.
            // Without this, calling Player::BeginTurn directly does NOT
            // dispatch the AI: settlers never settle, no cities are
            // founded, and score stays flat across thousands of turns.
            for (sint32 p = 0; p < k_MAX_PLAYERS; ++p) {
                if (!g_player[p] || g_player[p]->IsDead()) continue;

                s_headlessCurPlayer = p;
                g_player[p]->m_current_round = t;

                if (g_theProfileDB->IsAIOn()) {
                    g_gevManager->AddEvent(GEV_INSERT_Tail, GEV_AiBeginMapAnalysis,
                                           GEA_Player, p, GEA_End);
                }
                CtpAi::BeginDiplomacy(p, t);
                if (g_theProfileDB->IsAIOn()) {
                    g_gevManager->AddEvent(GEV_INSERT_Tail, GEV_AiBeginTurn,
                                           GEA_Player, p, GEA_End);
                }

                // BeginTurn() already calls NotifyTurnStart internally; only
                // NotifyTurnEnd needs an explicit call because EndTurn() does
                // not notify observers.
                g_player[p]->BeginTurn();

                // In the interactive game the director queues GEV_BeginScheduler
                // after BeginTurn().  Headless has no director loop, so we add
                // the scheduler event directly so the AI actually assigns orders
                // to units (settlers settle, armies move, etc.).
                if (g_gevManager) {
                    g_gevManager->AddEvent(GEV_INSERT_Tail, GEV_BeginScheduler,
                                           GEA_Player, p, GEA_End);
                }

                // Drain queued AI events so the player's turn actually runs
                // before we move on to the next player.
                if (g_gevManager) g_gevManager->Process();

                g_player[p]->EndTurn();
                if (g_gameObservers) g_gameObservers->NotifyTurnEnd(p);
            }

            // Process any cross-player pending events.
            if (g_gevManager) g_gevManager->Process();
        }

        fprintf(stderr, "[HEADLESS] Completed %d turns\n", maxTurns);

        if (saveGamePath) {
            fprintf(stderr, "[HEADLESS] Saving game to %s\n", saveGamePath);
            GameFile::SaveGame(saveGamePath, NULL);
            fprintf(stderr, "[HEADLESS] SaveGame returned\n");
        }

        if (exportMetricsPath) {
            fprintf(stderr, "[HEADLESS] Exporting metrics to %s\n", exportMetricsPath);
            FILE *fp = (strcmp(exportMetricsPath, "-") == 0)
                ? stdout
                : std::fopen(exportMetricsPath, "w");
            if (!fp) {
                fprintf(stderr, "[HEADLESS] Could not open %s for writing\n",
                        exportMetricsPath);
            } else {
                // --- per-player section ---
                std::fprintf(fp, "# PLAYERS\n");
                std::fprintf(fp, "player_idx,leader_name,is_dead,total_score,"
                                 "gold,num_cities\n");
                for (sint32 p = 0; p < k_MAX_PLAYERS; ++p) {
                    if (!g_player[p]) continue;
                    const char *name = g_player[p]->GetLeaderName();
                    if (!name) name = "";
                    sint32 score = g_player[p]->m_score
                                 ? g_player[p]->m_score->GetTotalScore() : 0;
                    sint32 gold     = g_player[p]->GetGold();
                    sint32 nCities  = g_player[p]->GetNumCities();
                    std::fprintf(fp, "%d,%s,%s,%d,%d,%d\n",
                                 (int)p, name,
                                 g_player[p]->IsDead() ? "yes" : "no",
                                 (int)score, (int)gold, (int)nCities);
                }

                // --- per-city section ---
                std::fprintf(fp, "\n# CITIES\n");
                std::fprintf(fp, "player_idx,city_name,pos_x,pos_y,population\n");
                for (sint32 p = 0; p < k_MAX_PLAYERS; ++p) {
                    if (!g_player[p]) continue;
                    UnitDynamicArray *cities = g_player[p]->GetAllCitiesList();
                    if (!cities) continue;
                    for (sint32 ci = 0; ci < cities->Num(); ++ci) {
                        Unit u = cities->Access(ci);
                        const char *cname = u.GetName();
                        if (!cname) cname = "";
                        MapPoint pos;
                        u.GetPos(pos);
                        CityData *cd = u.GetCityData();
                        sint32 pop = cd ? cd->PopCount() : 0;
                        std::fprintf(fp, "%d,%s,%d,%d,%d\n",
                                     (int)p, cname,
                                     (int)pos.x, (int)pos.y, (int)pop);
                    }
                }

                if (fp != stdout) std::fclose(fp);
                fprintf(stderr, "[HEADLESS] Metrics export complete\n");
            }
        }
    } else {
        fprintf(stderr, "[HEADLESS] Would run %d turns with %d players, seed=%d\n",
                maxTurns, numPlayers, seed);
    }

    fprintf(stderr, "[HEADLESS] Shutting down\n");
    return 0;
}
