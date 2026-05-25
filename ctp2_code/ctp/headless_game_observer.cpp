/**
 * @file headless_game_observer.cpp
 * @brief Headless observer — logs events but performs no UI/audio/render.
 *
 * Compiled into ctp2_headless target only. Provides visibility into what
 * the simulation is doing without requiring SDL, OpenGL, or sound.
 */

#include "ctp/c3.h"
#include "gs/core/game_observer.h"
#include "gs/gameobj/Player.h"
#include "gs/gameobj/Army.h"

class HeadlessGameObserver : public IGameObserver {
public:
    void OnTurnStart(sint32 player) override
    {
        // Use fprintf(stderr) instead of DPRINTF so headless output is visible
        // to smoke tests that capture stderr via popen(). DPRINTF writes to a
        // log file, not stderr.
        fprintf(stderr, "[HEADLESS] Turn start for player %d (round %d)\n",
                player,
                g_player[player] ? g_player[player]->GetCurRound() : -1);
    }

    void OnTurnEnd(sint32 player) override
    {
        fprintf(stderr, "[HEADLESS] Turn end for player %d\n", player);
    }

    void OnBuildPhaseComplete(sint32 player) override
    {
        DPRINTF(k_DBG_GAMESTATE,
                ("[HEADLESS] Build phase complete for player %d\n", player));
    }

    void OnCityFounded(sint32 player, const Unit& city,
                       const MapPoint& pos, sint32 cause) override
    {
        DPRINTF(k_DBG_GAMESTATE,
                ("[HEADLESS] Player %d founded city at (%d,%d) cause=%d\n",
                 player, pos.x, pos.y, cause));
    }

    void OnCityCaptured(const Unit& city, sint32 newOwner,
                        const MapPoint& pos) override
    {
        DPRINTF(k_DBG_GAMESTATE,
                ("[HEADLESS] City captured by player %d at (%d,%d)\n",
                 newOwner, pos.x, pos.y));
    }

    void OnWonderBuilt(const Unit& city, sint32 wonder) override
    {
        DPRINTF(k_DBG_GAMESTATE,
                ("[HEADLESS] Wonder %d built in city at (%d,%d)\n",
                 wonder, city.RetPos().x, city.RetPos().y));
    }

    void OnArmyMove(const Army& army,
                    const MapPoint& from, const MapPoint& to) override
    {
        DPRINTF(k_DBG_GAMESTATE,
                ("[HEADLESS] Army %d moved from (%d,%d) to (%d,%d)\n",
                 army.m_id, from.x, from.y, to.x, to.y));
    }

    void OnCombatStart(const Army& attacker, const Army& defender,
                       const MapPoint& pos) override
    {
        DPRINTF(k_DBG_GAMESTATE,
                ("[HEADLESS] Combat at (%d,%d): attacker=%d defender=%d\n",
                 pos.x, pos.y, attacker.m_id, defender.m_id));
    }

    void OnCombatEnd(const Army& attacker, const Army& defender,
                     bool attackerWon) override
    {
        DPRINTF(k_DBG_GAMESTATE,
                ("[HEADLESS] Combat ended: attackerWon=%d\n", attackerWon));
    }

    void OnAdvanceResearched(sint32 player, sint32 advance) override
    {
        DPRINTF(k_DBG_GAMESTATE,
                ("[HEADLESS] Player %d researched advance %d\n",
                 player, advance));
    }
};

static HeadlessGameObserver s_headlessGameObserver;

void RegisterHeadlessGameObserver()
{
    g_gameObservers->Register(&s_headlessGameObserver);
}
