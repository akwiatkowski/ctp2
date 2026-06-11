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
#include "gs/utility/TurnCnt.h"
#include "gs/events/GameEventManager.h"
#include "gs/core/game_observer.h"            // gameobservers_Get()
#include "gs/core/game_observer_registration.h"
#include "gs/core/player_view.h"              // player_view::RegisterCurPlayer
#include "gs/fileio/gamefile.h"               // GameFile::SaveGame / RestoreGame
#include "gs/fileio/json_save.h"              // json_save::SaveJson (Phase A)
#include "gs/gameobj/Score.h"                 // Score::GetTotalScore
#include "gs/gameobj/CityData.h"              // CityData::PopCount
#include "gs/gameobj/Unit.h"                  // Unit::GetName / GetPos / CD
#include "gs/utility/UnitDynArr.h"            // UnitDynamicArray
#include "gs/gameobj/Vision.h"                // Vision::IsVisible / IsExplored
#include "gs/gameobj/Events.h"                // GEV_AiBeginTurn / GEV_AiBeginMapAnalysis
#include "gs/gameobj/Army.h"                  // Army::NumOrders (resume queued paths)
#include "gs/events/GameEventManager.h"       // gevmanager_Get()
#include "ai/ctpai.h"                         // CtpAi::BeginDiplomacy
#include "ctp/game_controller.h"              // game_controller::Dispatch (--serve)
#include "ctp/crash_handler.h"                // crash_handler::Install
#include "test/smoketest_server.h"            // smoketest_server_* (--serve)

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <thread>

extern sint32  g_runInBackground;
#include "gs/utility/Globals.h"   // set_headless()
extern sint32  g_oldRandSeed;        // gameinit.cpp reads this as the RNG seed override

// Headless mode needs a CurPlayer callback because CtpAi::BeginTurn and
// BeginMapAnalysis assert(player == player_view::CurPlayer()).  In the
// interactive game CurPlayer is backed by selitem_Get(); headless has
// no selected item, so we track the currently processing player manually.
static sint32 s_headlessCurPlayer = 0;
static sint32 HeadlessCurPlayer() { return s_headlessCurPlayer; }

// File-static logger.  Anonymous namespace = internal linkage.  Name
// appears as [headless] in the spdlog pattern, matching the historical
// [HEADLESS] fprintf prefix.
namespace {
auto headless_log = civlog::Get("headless");
}  // namespace

// Run ONE full round: every live player takes a turn through the same event
// pipeline the interactive game uses — mirrors the body of
// STDEHANDLER(BeginTurnEvent) in TurnCntEvent.cpp. Without the AI events +
// scheduler, calling Player::BeginTurn directly does NOT dispatch the AI:
// settlers never settle, no cities are founded, and score stays flat.
// Shared by the batch --turns loop and the serve-mode end_turn verb.
static void headless_run_round(sint32 round)
{
    // The global TurnCount is the real clock: Player::BeginTurn overwrites
    // m_current_round from GetSessionRound(), and query_turn reads it back.
    // Align it to the round being played; advance it when the round ends —
    // so "round N" in queries means "N full rounds completed".
    if (turn_Get()) turn_Get()->SkipToRound(round);

    for (sint32 p = 0; p < k_MAX_PLAYERS; ++p) {
        if (!player_Get(p) || player_Get(p)->IsDead()) continue;

        s_headlessCurPlayer = p;

        if (profiledb_Get()->IsAIOn()) {
            gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_AiBeginMapAnalysis,
                                   GEA_Player, p, GEA_End);
        }
        CtpAi::BeginDiplomacy(p, round);
        if (profiledb_Get()->IsAIOn()) {
            gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_AiBeginTurn,
                                   GEA_Player, p, GEA_End);
        }

        // BeginTurn() already calls NotifyTurnStart internally; only
        // NotifyTurnEnd needs an explicit call because EndTurn() does
        // not notify observers.
        player_Get(p)->BeginTurn();

        // In the interactive game the director queues GEV_BeginScheduler
        // after BeginTurn(). Headless has no director loop, so we add the
        // scheduler event directly so the AI actually assigns orders to
        // units (settlers settle, armies move, etc.).
        if (gevmanager_Get()) {
            gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_BeginScheduler,
                                   GEA_Player, p, GEA_End);
        }

        // Drain queued AI events so the player's turn actually runs
        // before we move on to the next player.
        if (gevmanager_Get()) gevmanager_Get()->Process();

        // Resume queued multi-turn orders (move paths, explore). In the
        // interactive game GEV_BeginTurnExecute is emitted by the UI's
        // unit-selection flow (SelItem.cpp) for each army with pending
        // orders — headless has no UI, so without this every queued path
        // died after its first leg (auto_explore "stuck armies", settler
        // marches stalling 1-2 tiles in).
        if (gevmanager_Get() && player_Get(p)->m_all_armies) {
            for (sint32 a = 0; a < player_Get(p)->m_all_armies->Num(); ++a) {
                Army army = player_Get(p)->m_all_armies->Access(a);
                if (!army.IsValid() || army.NumOrders() == 0) continue;
                // Cargo never self-executes: the UI can't select an army
                // riding a transport, so interactive play never fires
                // BeginTurnExecute for it. Executing its stale orders here
                // crashes UpdateZOCForMove (the army isn't in any cell's
                // unit list while aboard).
                if (army.Num() > 0 && army.Access(0).IsBeingTransported())
                    continue;
                gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
                                           GEV_BeginTurnExecute,
                                           GEA_Army, army, GEA_End);
            }
            gevmanager_Get()->Process();
        }

        player_Get(p)->EndTurn();
        if (gameobservers_Get()) gameobservers_Get()->NotifyTurnEnd(p);
    }

    // Process any cross-player pending events.
    if (gevmanager_Get()) gevmanager_Get()->Process();

    // Round complete — the clock now reads "round+1 rounds have elapsed".
    if (turn_Get()) turn_Get()->SkipToRound(round + 1);
}

static void print_usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s [options]\n"
        "Options:\n"
        "  --serve                 Interactive mode: listen on the command socket\n"
        "                          and dispatch commands/queries (for the test\n"
        "                          harness); keeps one human player.\n"
        "  --new-game              Start a new game immediately\n"
        "  --players N             Number of AI players (default: 3)\n"
        "  --turns N               Run N turns then exit (default: 10)\n"
        "  --seed N                Pin RNG seed for determinism\n"
        "  --save-interval N       Save every N turns (currently unimplemented)\n"
        "  --save-game PATH        After running turns, save to PATH then exit\n"
        "  --load-game PATH        Load saved binary game from PATH instead of --new-game\n"
        "  --json-load PATH        Overlay JSON savegame onto --new-game state\n"
        "  --export-metrics PATH   After running turns, dump per-player and per-city\n"
        "                          metrics as CSV to PATH (or '-' for stdout)\n"
        "  --help                  Show this message\n",
        prog);
}

int main(int argc, char **argv)
{
    civlog::Init();
    // Self-reporting crashes: symbolised backtrace + recent game events
    // land in the log, so most crashes never need a debugger session.
    crash_handler::Install("/tmp/ctp2-crash.log");
    headless_log->info("CTP2 Headless Engine starting (crash reports -> /tmp/ctp2-crash.log)");

    // Parse arguments
    bool newGame = false;
    bool serveMode = false;
    sint32 numPlayers = 3;
    sint32 maxTurns = 10;
    sint32 saveInterval = 0;
    sint32 seed = 42;
    const char *saveGamePath = nullptr;
    const char *loadGamePath = nullptr;
    const char *exportMetricsPath = nullptr;
    // Phase A scaffold flag — writes the JSON skeleton header
    // ({"magic": "CTP2-JSON", "schema_version": 1}) after turns
    // complete.  Hidden from --help on purpose; not yet a real save
    // path.  See ~/projects/claude/plans/ctp2-json-savegame.md.
    const char *jsonSavePath = nullptr;
    const char *jsonLoadPath = nullptr;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--serve") == 0) {
            serveMode = true;
        } else if (strcmp(argv[i], "--new-game") == 0) {
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
        } else if (strcmp(argv[i], "--json-save") == 0 && i + 1 < argc) {
            jsonSavePath = argv[++i];
        } else if (strcmp(argv[i], "--json-load") == 0 && i + 1 < argc) {
            jsonLoadPath = argv[++i];
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
    set_headless(true);
    g_runInBackground = true;

    civapp_Set(new CivApp());

    headless_log->info("Initializing engine...");
    sint32 err = civapp_Get()->InitializeEngine();
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
    if (!civapp_Get()->InitializeAppDB()) {
        headless_log->error("InitializeAppDB failed");
        return 1;
    }

    CivScenarios::Initialize();

    headless_log->info("Engine + DBs initialized OK");

    RegisterHeadlessGameObserver();
    // Headless still links SelItem.cpp (it's in game_core_sources) and many
    // non-UI files — notably newturncount.cpp — dereference selitem_Get()
    // directly.  Until those callers are migrated to player_view::CurPlayer
    // etc., the headless build needs to allocate a SelectedItem instance.
    // Reuse the same SelectedItem-backed callbacks the UI build uses.
    RegisterUIPlayerView();
    // Override CurPlayer so AI asserts (player == CurPlayer()) pass and
    // the active player's round is returned.
    player_view::RegisterCurPlayer(&HeadlessCurPlayer);
    headless_log->info("observers + player_view registered");

    // ---- Interactive serve mode -----------------------------------------
    // Listen on the command socket and dispatch the same command/query set the
    // UI build does (via game_controller::Dispatch).  This is what lets one
    // Python test drive both binaries.  Unlike the batch path, we KEEP the
    // default human player (player 1) instead of forcing all-ROBOT, so
    // player-facing commands (build_city, set_production) are meaningful.
    if (serveMode) {
        headless_log->info("Serve mode: configuring profile (players={}, seed={})",
                           numPlayers, seed);
        profiledb_Get()->SetNPlayers(numPlayers);
        if (seed != 0) g_oldRandSeed = seed;
        profiledb_Get()->SetAI(TRUE);   // AI drives the non-human players

        smoketest_server_init();
        headless_log->info("Serve mode: command server up, entering poll loop");

        char cmd[256];
        bool done = false;
        while (!done) {
            if (!smoketest_poll_command(cmd, sizeof(cmd))) {
                // No command pending; yield to avoid a busy spin.
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            headless_log->info("serve cmd: {}", cmd);

            // Shared, UI-free verbs (build_city, set_production, save/load,
            // queries) behave identically to the UI build. DispatchSafe (not
            // Dispatch) because the poll left the smoke mutex LOCKED (released
            // only by a send_*) — an escaping exception would wedge the loop.
            bool handled = false;
            std::string resp = game_controller::DispatchSafe(cmd, handled);
            if (handled) {
                smoketest_send_json(resp.c_str());
                continue;
            }

            // Frontend-specific verbs: game creation has no menus headless.
            if (strcmp(cmd, "new_game") == 0) {
                // Nothing to navigate; the game is created on start_game.
                smoketest_send_response("ok", cmd, nullptr);
            } else if (strcmp(cmd, "start_game") == 0) {
                // Call the headless init path DIRECTLY. CivApp::InitializeGame()
                // guards on (!c3ui_Get()), but c3ui is non-null even headless
                // (InitializeEngine dereferences it), so that guard would route
                // us into the UI path and hang. The batch path uses this same
                // direct call.
                sint32 e = civapp_Get()->InitializeGameHeadless();
                if (e == 0) {
                    // Point HeadlessCurPlayer at the human so AI asserts that
                    // compare player == CurPlayer() hold for human-owned actions.
                    if (Player * human = game_controller::HumanPlayer())
                        s_headlessCurPlayer = human->GetOwner();
                    smoketest_send_response("ok", cmd, nullptr);
                } else {
                    smoketest_send_response("error", cmd, "init_failed");
                }
            } else if (strncmp(cmd, "end_turn", 8) == 0) {
                // "end_turn" or "end_turn N": advance N FULL ROUNDS (every
                // player takes a turn). Semantic note: the UI build's
                // end_turn queues director->AddEndTurn for the human only;
                // headless has no director, so a round is the meaningful
                // unit of time here.
                if (!civapp_Get()->IsGameLoaded()) {
                    smoketest_send_response("error", cmd, "game_not_loaded");
                } else {
                    int n = 1;
                    if (cmd[8] != '\0' && sscanf(cmd + 8, "%d", &n) != 1) n = -1;
                    if (n < 1 || n > 1000) {
                        smoketest_send_response("error", cmd, "bad_args");
                    } else {
                        // The GLOBAL TurnCount is the round source — a local
                        // counter reset on process restart and, worse, made
                        // end_turn after load_game stomp a loaded game's
                        // clock backwards via SkipToRound.
                        for (int i = 0; i < n; ++i) {
                            headless_run_round(turn_Get() ? turn_Get()->GetSessionRound() : 0);
                        }
                        // Park CurPlayer back on the human so queries
                        // (query_turn reads CurPlayer's round) and AI
                        // asserts see the driver's viewpoint.
                        if (Player * human = game_controller::HumanPlayer())
                            s_headlessCurPlayer = human->GetOwner();
                        char detail[48];
                        snprintf(detail, sizeof(detail), "round=%d",
                                 (int)(turn_Get() ? turn_Get()->GetSessionRound() : 0));
                        smoketest_send_response("ok", "end_turn", detail);
                    }
                }
            } else if (strcmp(cmd, "quit") == 0) {
                smoketest_send_response("ok", cmd, nullptr);
                done = true;
            } else {
                smoketest_send_response("error", cmd, "unknown_command");
            }
        }

        smoketest_server_shutdown();
        headless_log->info("Serve mode: shut down");
        return 0;
    }

    if (loadGamePath) {
        headless_log->info("Loading saved game from {}", loadGamePath);
        if (!GameFile::RestoreGame(loadGamePath)) {
            headless_log->error("RestoreGame failed for {}", loadGamePath);
            return 1;
        }
        headless_log->info("RestoreGame ok");
    }

    if (newGame || loadGamePath) {
        if (newGame) {
            headless_log->info("Starting new game (players={}, seed={})...", numPlayers, seed);

            // Set player count and seed in ProfileDB
            profiledb_Get()->SetNPlayers(numPlayers);

            // Wire --seed to the RNG.  gameinit_Initialize reads g_oldRandSeed
            // and uses it as the seed for rand_ptr() when non-zero; otherwise it
            // falls back to GetTickCount().  Map generation, AI decisions, and
            // combat all draw from rand_ptr(), so this is the single knob that
            // makes two runs deterministic.  Seed 0 keeps the legacy "use
            // system time" semantic for users who want non-deterministic runs.
            if (seed != 0) {
                g_oldRandSeed = seed;
            }

            // Enable AI for non-human players — otherwise the turn pipeline
            // ticks but no decisions are dispatched (settlers never settle,
            // builders never build, score stays flat).
            profiledb_Get()->SetAI(TRUE);

            // Use the headless game init path (no UI windows)
            err = civapp_Get()->InitializeGameHeadless();
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
                if (player_Get(p)) player_Get(p)->SetPlayerType(PLAYER_TYPE_ROBOT);
            }

            headless_log->info("Game initialized OK — running {} turns", maxTurns);

            // --json-load overlays a JSON savegame on top of the fresh-
            // game state.  Runs AFTER InitializeGameHeadless so the
            // singletons exist before LoadJson populates them.  See
            // json_save::LoadJson for the in-place from_json pattern.
            if (jsonLoadPath) {
                headless_log->info("Loading JSON from {}", jsonLoadPath);
                bool const ok = json_save::LoadJson(jsonLoadPath);
                headless_log->info("LoadJson returned {}", ok ? "ok" : "FAIL");
                if (!ok) return 1;
            }
        } else {
            headless_log->info("Loaded — running {} turns", maxTurns);
        }

        // Run turns
        for (sint32 t = 0; t < maxTurns; ++t) {
            headless_log->info("Turn {} / {}", t + 1, maxTurns);
            headless_run_round(t);
        }

        headless_log->info("Completed {} turns", maxTurns);

        if (saveGamePath) {
            headless_log->info("Saving game to {} (json)", saveGamePath);
            GameFile::SaveGame(saveGamePath, nullptr);
            headless_log->info("SaveGame returned");
        }

        if (jsonSavePath) {
            headless_log->info("Saving JSON to {}", jsonSavePath);
            bool ok = json_save::SaveJson(jsonSavePath);
            headless_log->info("SaveJson returned {}", ok ? "ok" : "FAIL");
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
                    if (!player_Get(p)) continue;
                    const char *name = player_Get(p)->GetLeaderName();
                    if (!name) name = "";
                    sint32 score = player_Get(p)->m_score
                                 ? player_Get(p)->m_score->GetTotalScore() : 0;
                    sint32 gold     = player_Get(p)->GetGold();
                    sint32 nCities  = player_Get(p)->GetNumCities();
                    std::fprintf(fp, "%d,%s,%s,%d,%d,%d\n",
                                 (int)p, name,
                                 player_Get(p)->IsDead() ? "yes" : "no",
                                 (int)score, (int)gold, (int)nCities);
                }

                // --- per-city section ---
                // visible_owner / explored_owner: queried against the city
                // owner's m_vision at the city tile.  A founded city must
                // be visible to its own owner, otherwise the UI renders
                // it fogged and the cell can't be clicked.  See
                // test_city_visibility.cpp.
                std::fprintf(fp, "\n# CITIES\n");
                std::fprintf(fp, "player_idx,city_name,pos_x,pos_y,population,"
                                 "visible_owner,explored_owner\n");
                for (sint32 p = 0; p < k_MAX_PLAYERS; ++p) {
                    if (!player_Get(p)) continue;
                    UnitDynamicArray *cities = player_Get(p)->GetAllCitiesList();
                    if (!cities) continue;
                    for (sint32 ci = 0; ci < cities->Num(); ++ci) {
                        Unit u = cities->Access(ci);
                        const char *cname = u.GetName();
                        if (!cname) cname = "";
                        MapPoint pos;
                        u.GetPos(pos);
                        CityData *cd = u.GetCityData();
                        sint32 pop = cd ? cd->PopCount() : 0;
                        bool visible  = player_Get(p)->m_vision &&
                                        player_Get(p)->m_vision->IsVisible(pos);
                        bool explored = player_Get(p)->m_vision &&
                                        player_Get(p)->m_vision->IsExplored(pos);
                        std::fprintf(fp, "%d,%s,%d,%d,%d,%s,%s\n",
                                     (int)p, cname,
                                     (int)pos.x, (int)pos.y, (int)pop,
                                     visible  ? "yes" : "no",
                                     explored ? "yes" : "no");
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
