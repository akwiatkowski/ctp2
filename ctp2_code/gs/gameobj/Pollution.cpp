//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Pollution handling
// Id           : $Id$
//
//----------------------------------------------------------------------------
//
// Disclaimer
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
// This material has been developed at apolyton.net by the Apolyton CtP2
// Source Code Project. Contact the authors at ctp2source@apolyton.net.
//
//----------------------------------------------------------------------------
//
// Compiler flags
//
// - None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Do not trigger disaster warnings when there is no pollution at all.
// - Memory leak repaired.
// - Improved pollution warning recipient handling.
// - Replaced old const database by new one. (5-Aug-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/ctp2_utils/c3debug.h"
#include "ctp/c3.h"
#include "gs/utility/Globals.h"

#include <memory>

#include "gs/gameobj/XY_Coordinates.h"
#include "gs/world/World.h"
#include "gs/gameobj/pollution.h"
#include "gs/gameobj/player.h"
#include "WonderRecord.h"
#include "net/general/network.h"
#include "gs/slic/SlicSegment.h"
#include "gs/slic/SlicObject.h"
#include "gs/slic/SlicEngine.h"
#include "PollutionRecord.h"
#include "gs/database/profileDB.h"
#include "gs/gameobj/GameSettings.h"
#include "gs/world/Cell.h"
#include "ConstRecord.h"
#include "gs/utility/RandGen.h"

#include "gs/gameobj/installationtree.h"
#include "gs/gameobj/wonderutil.h"
#include "gs/utility/TurnCnt.h"

#include "gs/events/GameEventManager.h"

sint32 const    Pollution::ROUNDS_COUNT_IMMEASURABLE    = 9999;

Pollution::Pollution()
{
	sint32	i;

	m_phase = 0;
	m_gwPhase = 0;

	for (i=0; i<k_MAX_GLOBAL_POLLUTION_RECORD_TURNS; i++)
		m_history[i] = 0;

	m_eventTriggered = FALSE;

	m_eventTriggerNextRound = 0;

	m_trend = 0;
	m_next_level = 1;
}

Pollution::~Pollution()
= default;

//----------------------------------------------------------------------------
//
// Name       : Pollution::WarnPlayers
//
// Description: Warn all players about an imminent pollution disaster.
//
// Parameters : -
//
// Globals    : slicengine_Get()    : message display handler
//              player_Get()    : player accessor
//
// Returns    : -
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
void Pollution::WarnPlayers()
{
	auto		so	= std::make_unique<SlicObject>("911ImminentFlood");
	SlicSegment *	seg	= slicengine_Get()->GetSegment("911ImminentFlood");

	// Start at 1: skip the barbarians.
	for(PLAYER_INDEX i = 1; i < k_MAX_PLAYERS; ++i)
	{
		if(player_Get(i)				&&
			!player_Get(i)->IsDead()	&&
			!seg->TestLastShown(i, k_ROUNDS_BEFORE_DISASTER, turn_Get()->GetRound())
		  )
		{
			so->AddRecipient(i);
		}
	}

	if (so->GetNumRecipients())
	{
		slicengine_Get()->Execute(std::move(so));
	}
}

sint32 Pollution::AtTriggerLevel()
{
	if(!gamesettings_Get()->GetPollution())
		return FALSE;

	sint32 i;
	for(i = 0; i < k_MAX_PLAYERS; i++)
	{
		if(!player_Get(i))
			continue;

		if(wonderutil_GetReduceWorldPollution(player_Get(i)->GetBuiltWonders()))
		{
			return FALSE;
		}
	}

	const PollutionRecord::Phase* pprec = g_thePollutionDB->Get(profiledb_Get()->GetMapSize())->GetPhase(m_phase);
	sint32 trigger = pprec->GetPollutionTrigger();

	sint32 pollution = GetGlobalPollutionLevel();

	if(pollution > trigger)
	{
		if(pprec->GetOzoneDisaster())
		{
			// Missing stuff
		}

		if(pprec->GetFloodDisaster())
		{
			if (m_phase < (g_thePollutionDB->Get(profiledb_Get()->GetMapSize())->GetNumPhase()) / 2)
			{
				// Missing stuff
			}
			else
			{
				// Missing stuff
			}
		}

		if(pprec->GetWarning())
		{
			// Missing stuff
		}

		return TRUE;
	}

	if (m_history[0] <= m_history[1])
	{
		if(pollution > g_thePollutionDB->Get(profiledb_Get()->GetMapSize())->GetPhase(0)->GetPollutionTrigger()) {
			// Missing stuff
		}
	}
	else if(pollution > (sint32)((double)trigger * 0.80))
	{
		if(m_phase == 0)
		{
			// Missing stuff
		}

		if(pprec->GetOzoneDisaster())
		{
			// Missing stuff
		}

		if(pprec->GetFloodDisaster())
		{
			// Missing stuff
		}
	}
	return FALSE;
}

sint32 Pollution::GetNextTrigger()
{
	return g_thePollutionDB->Get(profiledb_Get()->GetMapSize())->GetPhase(m_phase)->GetPollutionTrigger();
}

sint32 Pollution::GetGlobalPollutionLevel()
{
	sint32 pollution      = m_history[0];
	sint32 gaiaController = 0;

	for(sint32 i = 0; i < k_MAX_PLAYERS; i++)
	{
		if(!player_Get(i)) continue;

		pollution      += player_Get(i)->GetPollutionLevel();
		gaiaController += wonderutil_GetReduceWorldPollution(player_Get(i)->GetBuiltWonders());
	}

	pollution -= gaiaController;

	return pollution;
}

void Pollution::SetGlobalPollutionLevel(sint32 requiredPollution)
{
	sint32 playerPollution = 0;
	sint32 gaiaController  = 0;

	for(sint32 i = 0; i < k_MAX_PLAYERS; i++)
	{
		if(player_Get(i) != nullptr)
		{
			playerPollution += player_Get(i)->GetPollutionLevel();
			gaiaController  += wonderutil_GetReduceWorldPollution(player_Get(i)->GetBuiltWonders());
		}
	}

	requiredPollution -= playerPollution;
	requiredPollution += gaiaController;

	if (requiredPollution < 0)
		requiredPollution = 0;

	SetHistory(requiredPollution);
}

void Pollution::BeginTurn()
{
	if(!gamesettings_Get()->GetPollution())
		return;

	if(GetRoundsToNextDisaster() < k_ROUNDS_BEFORE_DISASTER)
	{
		WarnPlayers();
	}

	if(AtTriggerLevel())
	{
		if(m_phase < g_thePollutionDB->Get(profiledb_Get()->GetMapSize())->GetNumPhase())
		{
			const PollutionRecord::Phase* pprec = g_thePollutionDB->Get(profiledb_Get()->GetMapSize())->GetPhase(m_phase);

			if(pprec->GetOzoneDisaster())
			{
				world_Get()->OzoneDepletion();
			}
			if(pprec->GetFloodDisaster())
			{
				world_Get()->GlobalWarming(m_gwPhase);
				m_gwPhase++;
			}

			GotoNextLevel();
			m_eventTriggered = TRUE;
		}

		m_eventTriggerNextRound = 0;
	}
	else
		m_eventTriggerNextRound-- ;

	if(network_Get().IsHost())
	{
		network_Get().EnqueuePollution();
	}
}

void Pollution::GotoNextLevel()
{
	if(m_phase < (g_thePollutionDB->Get(profiledb_Get()->GetMapSize())->GetNumPhase() - 1))
		m_phase++;
}

void Pollution::EndRound()
{
	double	 offset;
	double	 slope;

	sint32 pollution = m_history[0];

	/* Shift the pollution history array one slot to the right (older entries
	   move to higher indices).  Source and destination overlap, so memmove
	   is required; memcpy is undefined behaviour for overlapping ranges. */
	memmove(&m_history[1], &m_history[0], sizeof(m_history)-sizeof(m_history[0]));

	for(sint32 i = 0; i < k_MAX_PLAYERS; i++)
	{
		if(!player_Get(i)) continue;
		pollution += player_Get(i)->GetPollutionLevel();
	}

	m_history[0] = pollution;

	m_trend = CalcTrend(m_history, k_MAX_GLOBAL_POLLUTION_RECORD_TURNS, offset, slope);

	if(network_Get().IsHost())
	{
		network_Get().EnqueuePollution();
	}

	DPRINTF(k_DBG_FIX, ("Global Pollution Level: %d\n", pollution));
}

sint32 Pollution::CalcTrend(sint32 level[], sint32 numPoints, double &offset, double &slope)
{
	sint32	i;

	double	 t;
	double	 sxoss;
	double	 sx=0.0;
	double	 sy=0.0;
	double	 st2=0.0;
	double	 ss;

	slope=0.0;

	for(i=1; i<=numPoints; i++)
	{
		sx += i;
		sy += level[i];
	}

	ss = numPoints;

	sxoss=sx/ss ;
	for(i = 1; i <= numPoints; i++)
	{
		t      = i-sxoss;
		st2   += t*t;
		slope += t*(level[i]);
	}

	slope /= st2;
	offset =(sy-sx*slope)/ss;

	if (slope<0.0)
		return (k_TREND_DOWNWARD);
	else if (slope>0.0)
		return (k_TREND_UPWARD);

	return (k_TREND_LEVEL);
}

sint32 Pollution::GetTrend() const
{
	return (m_trend);
}

//----------------------------------------------------------------------------
//
// Name       : Pollution::GetRoundsToNextDisaster
//
// Description: Estimate number of turns to next disaster.
//
// Parameters : -
//
// Globals    : -
//
// Returns    : sint32          : estimated number of turns to next disaster
//
// Remark(s)  : Will return ROUNDS_COUNT_INMEASURABLE when there is no
//              pollution at all.
//
//              m_history[0]    : pollution level this turn
//              m_history[1]    : pollution level previous turn
//
//----------------------------------------------------------------------------
sint32 Pollution::GetRoundsToNextDisaster()
{
	// Check if there has been any pollution at all in this turn.
	if((m_history[0] <= 0) || (m_history[0] <= m_history[1]))
	{
		return ROUNDS_COUNT_IMMEASURABLE;
	}

	// Check for a pollution suppressing wonder built by some player.
	for(int i = 0; i < k_MAX_PLAYERS; ++i)
	{
		if(player_Get(i) &&
			wonderutil_GetReduceWorldPollution(player_Get(i)->GetBuiltWonders())
		  )
		{
			return ROUNDS_COUNT_IMMEASURABLE;
		}
	}

	// Estimate the number of turns until the next disaster.
	sint32 const pollutionDeltaPerTurn	= m_history[0] - m_history[1];
	sint32 const pollutionUntilTrigger	=
		g_thePollutionDB->Get(profiledb_Get()->GetMapSize())->GetPhase(m_phase)->GetPollutionTrigger() - m_history[0];
	return pollutionUntilTrigger / pollutionDeltaPerTurn;
}

void pollution_NukeCell(MapPoint &pos, Cell *cell)
{
	bool CutNPasteCodeIsBad = false;
	Assert(CutNPasteCodeIsBad);
}

void Pollution::AddNukePollution(const MapPoint &cpos)
{
	if(!world_Get()->GetCell(cpos)->HasCity())
	{
		MapPoint stupidNonConstMapPoint = cpos;
		gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent,
							   GEV_KillTile,
							   GEA_MapPoint, stupidNonConstMapPoint,
							   GEA_End);
	}

	MapPoint pos;
	for (sint32 i = 0; i < g_theConstDB->Get(0)->GetNukeKillsTiles(); i++)
	{
		if(cpos.GetNeighborPosition((WORLD_DIRECTION)civrand().Next(sint32(NOWHERE)), pos))
		{
			gevmanager_Get()->AddEvent(GEV_INSERT_AfterCurrent,
								   GEV_KillTile,
								   GEA_MapPoint, pos,
								   GEA_End);
		}
	}
	m_history[0]+=g_theConstDB->Get(0)->GetPollutionCausedByNuke();
}

uint32 Pollution::GetPollutionAtRound(const PLAYER_INDEX player, const sint32 round)
{
	Assert(player_Get(player) != nullptr);
	if (player_Get(player) == nullptr)
		return 0;

	sint32 current_round = turn_Get()->GetSessionRound();

	if (current_round < round)
		return 0;

	if ((current_round - round) >= k_MAX_POLLUTION_HISTORY)
		return 0;

	return player_Get(player)->GetPollutionHistory()[current_round - round];
}
