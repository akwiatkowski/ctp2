//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Headless entry point — game simulation without UI/audio/render
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ctp/civapp.h"
#include "ctp/ctp2_utils/civlog.h"
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
extern sint32  g_oldRandSeed;        // gameinit.cpp reads this as the RNG seed override

// Headless mode needs a CurPlayer callback because CtpAi::BeginTurn and
// BeginMapAnalysis assert(player == player_view::CurPlayer()).  In the
// interactive game CurPlayer is backed by g_selected_item; headless has
// no selected item, so we track the currently processing player manually.
static sint32 s_headlessCurPlayer = 0;
static sint32 HeadlessCurPlayer() { return s_headlessCurPlayer; }

// File-static logger.  Anonymous namespace = internal linkage.  Name
// appears as [headless] in the spdlog pattern, matching the historical
// [HEADLESS] fprintf prefix.
namespace {
auto headless_log = civlog::Get("headless");
}  // namespace

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
    civlog::Init();
    headless_log->info("CTP2 Headless Engine starting");

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
            headless_log->error("Unknown argument: {}", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    // Force headless mode before any initialization
    g_headlessMode = true;
    g_runInBackground = true;

    g_civApp = new CivApp();

    headless_log->info("Initializing engine...");
    sint32 err = g_civApp->InitializeEngine();
    if (err != 0) {
        headless_log->error("Engine initialization failed: {}", err);
        return 1;
    }

    // Load game file list and databases (normally done inside InitializeApp)
    headless_log->info("Loading game files...");
    if (!gameinit_InitializeGameFiles()) {
        headless_log->error("gameinit_InitializeGameFiles failed");
        return 1;
    }

    headless_log->info("Loading databases...");
    if (!g_civApp->InitializeAppDB()) {
        headless_log->error("InitializeAppDB failed");
        return 1;
    }

    CivScenarios::Initialize();

    headless_log->info("Engine + DBs initialized OK");

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
    headless_log->info("observers + player_view registered");

    if (loadGamePath) {
        headless_log->info("Loading saved game from {}", loadGamePath);
        GameFile::RestoreGame(loadGamePath);
        headless_log->info("RestoreGame returned (state may or may not be valid)");
    }

    if (newGame || loadGamePath) {
        if (newGame) {
            headless_log->info("Starting new game (players={}, seed={})...", numPlayers, seed);

            // Set player count and seed in ProfileDB
            g_theProfileDB->SetNPlayers(numPlayers);

            // Wire --seed to the RNG.  gameinit_Initialize reads g_oldRandSeed
            // and uses it as the seed for g_rand when non-zero; otherwise it
            // falls back to GetTickCount().  Map generation, AI decisions, and
            // combat all draw from g_rand, so this is the single knob that
            // makes two runs deterministic.  Seed 0 keeps the legacy "use
            // system time" semantic for users who want non-deterministic runs.
            if (seed != 0) {
                g_oldRandSeed = seed;
            }

            // Enable AI for non-human players — otherwise the turn pipeline
            // ticks but no decisions are dispatched (settlers never settle,
            // builders never build, score stays flat).
            g_theProfileDB->SetAI(TRUE);

            // Use the headless game init path (no UI windows)
            err = g_civApp->InitializeGameHeadless();
            if (err != 0) {
                headless_log->error("Game initialization failed: {}", err);
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

            headless_log->info("Game initialized OK — running {} turns", maxTurns);
        } else {
            headless_log->info("Loaded — running {} turns", maxTurns);
        }

        // Run turns
        for (sint32 t = 0; t < maxTurns; ++t) {
            headless_log->info("Turn {} / {}", t + 1, maxTurns);

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

        headless_log->info("Completed {} turns", maxTurns);

        if (saveGamePath) {
            headless_log->info("Saving game to {}", saveGamePath);
            GameFile::SaveGame(saveGamePath, NULL);
            headless_log->info("SaveGame returned");
        }

        if (exportMetricsPath) {
            headless_log->info("Exporting metrics to {}", exportMetricsPath);
            FILE *fp = (strcmp(exportMetricsPath, "-") == 0)
                ? stdout
                : std::fopen(exportMetricsPath, "w");
            if (!fp) {
                headless_log->error("Could not open {} for writing", exportMetricsPath);
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
                headless_log->info("Metrics export complete");
            }
        }
    } else {
        headless_log->info("Would run {} turns with {} players, seed={}",
                  maxTurns, numPlayers, seed);
    }

    headless_log->info("Shutting down");
    return 0;
}
