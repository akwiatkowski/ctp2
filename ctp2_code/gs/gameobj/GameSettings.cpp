//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Game settings
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
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Use the difficulty and barbarian risk as selected by the user.
// - removed new rules attempt - E 12.27.2006
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/gameobj/GameSettings.h"
#include "gs/database/profileDB.h"
#include "net/general/network.h"
#include "gs/gameobj/player.h"
#include "gs/gameobj/EndGame.h"

extern BOOL			g_setDifficultyUponLaunch;
extern sint32		g_difficultyToSetUponLaunch;
extern BOOL			g_setBarbarianRiskUponLaunch;
extern sint32		g_barbarianRiskUponLaunch;

// g_theGameSettings definition moved to gs/utility/gameinit.cpp (where
// the lifecycle lives).  External callers go through gamesettings_Get().

GameSettings::GameSettings()
{
	if (g_setDifficultyUponLaunch)
	{
		m_difficulty	= g_difficultyToSetUponLaunch;
	}
	else
	{
		m_difficulty	= profiledb_Get()->GetDifficulty();
	}

	if (g_setBarbarianRiskUponLaunch)
	{
		m_risk			= g_barbarianRiskUponLaunch;
	}
	else
	{
		m_risk			= profiledb_Get()->GetRiskLevel();
	}
	m_alienEndGame = profiledb_Get()->IsAlienEndGameOn();
	m_pollution = profiledb_Get()->IsPollutionRule();
	m_keepScore = TRUE;

	m_startingAge = network_Get().GetStartingAge();
	m_endingAge = network_Get().GetEndingAge();
}


void GameSettings::SetKeepScore( BOOL keepScore )
{
	m_keepScore = keepScore;
}

void GameSettings::SetPollution( BOOL pollution )
{
	m_pollution = pollution;
}


void GameSettings::SetAlienEndGameWon(sint32 player)
{













}

BOOL GameSettings::GetAlienEndGame() const
{
	if(network_Get().IsActive())
		return FALSE;
	return m_alienEndGame;
}
