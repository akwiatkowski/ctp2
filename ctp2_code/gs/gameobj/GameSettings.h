// - removed new rules attempt - E 12.27.2006
#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __GAME_SETTINGS_H__
#define __GAME_SETTINGS_H__

#include <nlohmann/json.hpp>

class CivArchive;

class GameSettings {
private:

	sint32 m_difficulty;
	sint32 m_risk;
	BOOL m_alienEndGame;
	BOOL m_keepScore;

	sint32 m_startingAge, m_endingAge;

	BOOL m_pollution;

	friend class NetGameSettings;
	// JSON savegame bridges (json_save.cpp).  Friends so the private
	// fields can be serialised without expanding the public API just
	// for serialisation.
	friend void to_json(nlohmann::json &j, GameSettings const &gs);
	friend void from_json(nlohmann::json const &j, GameSettings &gs);

public:
	GameSettings();
	GameSettings(CivArchive &archive);

	void SetKeepScore( BOOL keepScore );
	void SetPollution( BOOL pollution );
	void SetStartingAge(sint32 age) { m_startingAge = age; }
	void SetEndingAge(sint32 age) { m_endingAge = age; }
	void SetAlienEndGameWon(sint32 player);

	sint32 GetDifficulty() const { return m_difficulty; }
	sint32 GetRisk() const { return m_risk; }
	BOOL GetAlienEndGame() const;
	BOOL GetKeeppScore() const { return m_keepScore; }
	BOOL GetPollution() const { return m_pollution; }
	sint32 GetStartingAge() const { return m_startingAge; }
	sint32 GetEndingAge() const { return m_endingAge; }

	void Serialize(CivArchive &archive);
};

// Lifecycle in gs/utility/gameinit.cpp; backing static there.
// test_citydata.cpp re-allocates the singleton via gamesettings_Set().
GameSettings * gamesettings_Get();
void           gamesettings_Set(GameSettings *p);
#endif
