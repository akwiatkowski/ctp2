//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Turncounter handles the clockwork of turn progression
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
// _DEBUG
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Put SendNextPlayerMessage into its own event. (14-Nov-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __TURN_COUNT_H__
#define __TURN_COUNT_H__

#include "gs/utility/newturncount.h"
#include <nlohmann/json.hpp>

class CivArchive ;
template <class T> class SimpleDynamicArray;

typedef sint32 PLAYER_INDEX;

class TurnCount {
	sint32 m_round;
	sint32 m_turn;
	sint32 m_year;
	sint32 m_activePlayers;
	BOOL m_simultaneousMode;
	SimpleDynamicArray<sint32> *m_sliceList;
	sint32 m_lastBeginTurn;
	BOOL m_isHotSeat;
	BOOL m_isEmail;
	uint8 m_sentGameAlmostOverMessage;
	uint8 m_sentGameOverMessage;
	sint32 m_happinessPlayer;

	friend class NetInfo;
	// JSON savegame bridges (json_save.cpp).  Friends so the private
	// fields can be serialised without expanding the public API just
	// for serialisation.
	friend void to_json(nlohmann::json &j, TurnCount const &tc);
	friend void from_json(nlohmann::json const &j, TurnCount &tc);

private:
	void RunNewYearMessages();
	void SendMsgEndOfGameEarlyWarning();
	void SendMsgToAllPlayers(MBCHAR *s);
	static sint32 sm_the_stop_player;

public:

	static sint32 GetStopPlayer() { return sm_the_stop_player; }
	static void SetStopPlayer(const sint32 &player_index) { sm_the_stop_player = player_index; }

	// Explicit initial state: number of active players + initial calendar
	// year. Callers compute these from session-state sources (profile DB +
	// difficulty record); we don't reach for globals from inside Init.
	TurnCount(sint32 numPlayers, sint32 initialYear);
	TurnCount(CivArchive &archive);
	~TurnCount();

	void Init(sint32 numPlayers, sint32 initialYear);
	void Init(CivArchive &archive);

	void ChooseNextActivePlayer();
	void EndThisTurn();
	void BeginNewRound();
	void InformNetwork();
	void SliceInformNetwork();
	void InformMessages();
	void BeginNewTurn(BOOL clientVerification);
	void EndThisTurnBeginNewTurn (BOOL clientRequest = FALSE);
	void EndThisSlice();
	BOOL BeginNewSlice();
	void EndThisSliceBeginNewSlice();

	sint32 GetRound() const { return NewTurnCount::GetCurrentRound(); }
	sint32 GetTurn() const { return m_turn; }
	sint32 GetYear() const { return NewTurnCount::GetCurrentYear(); }

	// Direct-field accessors that don't route through NewTurnCount /
	// the player array.  Use these when you specifically want the session-level
	// round/year tracked by this TurnCount instance, and when you want
	// the read to be safe without global game state initialised (e.g.
	// in unit tests).  The legacy GetRound/GetYear accessors above query
	// the currently-viewing player's recorded round, which for most
	// callers is the same value but reaches into the player array to find it.
	sint32 GetSessionRound() const { return m_round; }
	sint32 GetSessionYear()  const { return m_year;  }

	void SkipToRound(sint32 round);

	void SetSliceTo(sint32 player);
	void QueueSliceFor(sint32 player);

	BOOL SimultaneousMode() const;
	void SetSimultaneousMode(BOOL on);
	void NetworkEndTurn(BOOL force = FALSE);

	void Serialize(CivArchive &archive) ;

	void CountActivePlayers();
	void PlayerDead(PLAYER_INDEX player);
	void RegisterNewPlayer(PLAYER_INDEX player);

	void SetHotSeat(BOOL on);
	void SetEmail(BOOL on);
	BOOL IsHotSeat();
	BOOL IsEmail();

	void NextRound(BOOL fromDirector = FALSE, BOOL force = FALSE);

	BOOL VerifyEndTurn(BOOL force);

#ifdef _DEBUG
	void LogPlayerStats();
#endif

	void ChooseHappinessPlayer();
	sint32 GetHappinessPlayer() { return m_happinessPlayer; }

	void NotifyBecameHost();
	void SendNextPlayerMessage();
	void SendNextPlayerMessageEvent();
};

// g_turn storage now lives in Ctp2::Game; access via turn_Get / turn_Set.

// Accessor pair, mirroring world_Get/world_Set and friends.  Used by
// Ctp2::Game to adopt the legacy global into its unique_ptr.
TurnCount * turn_Get();
void        turn_Set(TurnCount *p);

#endif
