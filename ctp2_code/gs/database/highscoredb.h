#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __HIGHSCOREDB_H__
#define __HIGHSCOREDB_H__

class Token;

#include <memory>

#define k_MAX_HIGH_SCORES	10

struct HighScoreInfo {
	MBCHAR		m_name[256];
	sint32		m_score;
	std::unique_ptr<HighScoreInfo>	m_next;
};

class HighScoreDB {

public:
	sint32		m_nHighScores;
	std::unique_ptr<HighScoreInfo>	m_highScoreInfo;

	HighScoreDB();
	~HighScoreDB();

	void Initialize();
	void LoadHighScores( );
	void SaveHighScores( );

	sint32 AddHighScore(MBCHAR *name, sint32 score);
	sint32 CheckMaxScores( );
	sint32 GetNumHighScores() const { return m_nHighScores; }
	HighScoreInfo *GetHighScoreInfo( sint32 index );
};

#endif
