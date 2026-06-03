#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __ENDGAME_H__
#define __ENDGAME_H__

class CivArchive;
#include "gs/utility/gstypes.h"

#include <nlohmann/json.hpp>

class EndGame {
private:

	sint32 m_owner;
	sint32 m_currentStage;

	sint32 m_savedCurrentStage;

	sint32 m_currentStageBegan;

	sint32 *m_numBuilt;


	sint32 *m_savedNumBuilt;

	friend class NetEndGame;

	// JSON bridge — mirrors EndGame::Serialize.  Persists scalars +
	// the two num-built arrays (sized by g_theEndGameDB->m_nRec).
	friend void to_json(nlohmann::json &j, EndGame const &g);
	friend void from_json(nlohmann::json const &j, EndGame &g);

public:
	EndGame(PLAYER_INDEX owner);
	EndGame(CivArchive &archive);
	~EndGame();
	void Serialize(CivArchive &archive);
	void Init();

	void AddObject(sint32 type);
	void ClearAll();
	BOOL BeginSequence(sint32 currentRound);
	void BeginTurn(sint32 currentRound);
	void AdvanceStage(sint32 currentRound);
	sint32 GetCataclysmChance();
	sint32 GetTurnsForNextStage();
	BOOL MetRequirementsForNextStage();
	BOOL HaveAllPrerequisites();
	BOOL HaveEnoughFields();
    BOOL HaveEnoughECDs();
	BOOL HaveMaxSplicers();

	void XLabCaptured();
	void Cataclysm();

	sint32 GetTurnsSinceStageBegan(sint32 currentRound) const;

	sint32 GetStage();
	sint32 GetNumberBuilt(sint32 type);

	sint32 GetDisplayedStage();
	sint32 GetNumberShown(sint32 type);

	void UpdateDisplayState();

	BOOL HasLab();
};

#endif
