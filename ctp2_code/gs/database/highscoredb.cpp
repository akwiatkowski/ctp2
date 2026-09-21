#include "ctp/c3.h"
#include "ctp/ctp2_utils/c3errors.h"
#include "ctp/ctp2_utils/c3files.h"

#include "gs/fileio/Token.h"
#include "gs/database/StrDB.h"

#include "gs/database/highscoredb.h"

extern sint32	g_abort_parse;


HighScoreDB::HighScoreDB()
{
	Initialize();
}

HighScoreDB::~HighScoreDB()
{

	SaveHighScores();

	// m_highScoreInfo owns the whole list via unique_ptr links.
}

void HighScoreDB::Initialize()
{
	m_nHighScores = 0;
	m_highScoreInfo.reset();
	LoadHighScores();
}

sint32 HighScoreDB::AddHighScore(MBCHAR *name, sint32 score)
{
	HighScoreInfo *nextScore = nullptr;
	HighScoreInfo *prevScore = nullptr;
	sint32 i = 0;

	auto newScore = std::make_unique<HighScoreInfo>();
	strlcpy(newScore->m_name, name, sizeof(newScore->m_name));
	newScore->m_score = score;
	newScore->m_next = nullptr;

	if (!m_nHighScores)
	{

		m_highScoreInfo = std::move(newScore);
		m_nHighScores++;
		return 0;
	}

	if (m_highScoreInfo->m_score < newScore->m_score)
	{
		newScore->m_next = std::move(m_highScoreInfo);
		m_highScoreInfo = std::move(newScore);
		m_nHighScores++;

		CheckMaxScores();
		return 0;
	}

	nextScore = m_highScoreInfo->m_next.get();
	prevScore = m_highScoreInfo.get();
	for ( i = 0 ; i < m_nHighScores ; i++ )
	{

		if (!nextScore)
		{
			prevScore->m_next = std::move(newScore);
			m_nHighScores++;

			CheckMaxScores();
			return 0;
		}

		if (nextScore->m_score < newScore->m_score)
		{
			newScore->m_next = std::move(prevScore->m_next);
			prevScore->m_next = std::move(newScore);
			m_nHighScores++;

			CheckMaxScores();
			return 0;
		}
		prevScore = nextScore;
		nextScore = nextScore->m_next.get();
	}

	return 0;
}

sint32 HighScoreDB::CheckMaxScores( )
{
	HighScoreInfo *nextScore = m_highScoreInfo.get();
	HighScoreInfo *prevScore = nullptr;

	if (m_nHighScores > k_MAX_HIGH_SCORES)
	{

		Assert(m_nHighScores == k_MAX_HIGH_SCORES+1);
		for (sint32 i = 0 ; i < k_MAX_HIGH_SCORES; i++)
		{
			prevScore = nextScore;
			nextScore = nextScore->m_next.get();
		}

		// reset() unlinks and deletes the tail node.
		prevScore->m_next.reset();
		m_nHighScores--;
	}

	return 0;
}

HighScoreInfo *HighScoreDB::GetHighScoreInfo( sint32 index )
{

	if (index > m_nHighScores) index = m_nHighScores;
	if (index < 0) index = 0;

	if (!m_highScoreInfo) return nullptr;

	HighScoreInfo *nextScore = m_highScoreInfo.get();

	for (sint32 i = 0 ; i < index ; i++)
	{
		nextScore = nextScore->m_next.get();
	}

	return nextScore;
}


void HighScoreDB::LoadHighScores( )
{
	MBCHAR strbuf[256];
	MBCHAR scorebuf[256];
	MBCHAR rankname[256];
	sint32 score = 0;
	MBCHAR c = '\0';
	sint32 i = 0;
	sint32 j = 0;

	FILE *fp;

	fp = c3files_fopen(C3DIR_GAMEDATA, "hscore.txt", "r");

	if (!fp) return;

	while(true)
	{
		i = 0;
		j = 0;

		if (!fgets(strbuf, 255, fp)) break;

		while(i < 256)
		{

			c = strbuf[i++];
			if (c == '{')
			{
				while ((i < 256) && (j < 256))
				{

					c = strbuf[i++];
					if (c == '}') break;
					rankname[j++] = c;
				}

				rankname[j] = '\0';

				j = 0;
				while((i < 256) && (c != '\n'))
				{
					c = strbuf[i++];
					scorebuf[j++] = c;
				}
				scorebuf[j] = '\0';
				break;
			}
		}

		if (i == 256) break;

		if (sscanf(scorebuf, "%d", &score) != 1) break;

		AddHighScore(rankname,score);
	}

	fclose(fp);
}

void HighScoreDB::SaveHighScores( )
{
	FILE *fp;

	fp = c3files_fopen(C3DIR_GAMEDATA, "hscore.txt", "w");

	if (!fp) return;

	HighScoreInfo *nextScore = m_highScoreInfo.get();

	for ( sint32 i = 0 ; i < m_nHighScores ; i++)
	{

		fprintf(fp, "{%s} %d\n", nextScore->m_name, nextScore->m_score);
		nextScore = nextScore->m_next.get();
	}

	fclose(fp);
}
