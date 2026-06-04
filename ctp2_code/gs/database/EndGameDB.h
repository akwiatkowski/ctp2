#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __ENDGAME_DB_H__
#define __ENDGAME_DB_H__

#include "gs/database/Rec.h"
#include "gs/database/DB.h"
#include "gs/database/EndGameRecord.h"
#include "ctp/ctp2_utils/c3files.h"
#include "gs/fileio/Token.h"

class EndGameDatabase : public Database<EndGameRecord> {
private:
	BOOL m_abort_parse;

public:
	sint32 m_numStages;

	EndGameDatabase();

	BOOL Initialize(char *filename, C3DIR dir);
	BOOL ParseAnEndGameObject(Token *token, sint32 index);

	sint32 GetNumStages() const { return m_numStages; }

};

// Definition is file-scope `static` in EndGameDB.cpp.  The pointer is
// initialised to NULL and never assigned anywhere in the current
// codebase — the EndGameDatabase feature was scaffolded but the
// init/load path was never wired up.  Reads still happen (endgame.cpp,
// EndgameWindow.cpp, etc.) but with the pointer NULL they fall through
// to no-op stages.  The accessor preserves that behaviour.
EndGameDatabase * endgamedb_Get();
void              endgamedb_Set(EndGameDatabase *p);

#endif
