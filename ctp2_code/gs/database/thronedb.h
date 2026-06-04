#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef _BMH_THRONE_DB_H_
#define _BMH_THRONE_DB_H_

#include "gs/database/dbtypes.h"

class Token;

struct ThroneInfo {

	StringId		m_name;

	StringId		m_text;

	sint32			m_upgradeSoundID;


	MBCHAR			m_zoomedImageFilename[_MAX_PATH];

	bool			m_canZoom;


	bool			m_isCeiling;
};

class ThroneDB {
public:

	sint32		m_nThroneTypes;
	sint32		m_nThroneLevels;
	ThroneInfo	*m_throneInfo;

	ThroneDB();

	~ThroneDB();

	void Initialize();

	sint32 Init(MBCHAR *filename);

	ThroneInfo *GetThroneInfo( sint32 type, sint32 level ) const;

protected:

	sint32 ParseNumber(Token *token, sint32 *number);

	sint32 ParseThroneDatabase(MBCHAR *filename);

	sint32 CheckToken(Token *token, sint32 type, MBCHAR *error);

	sint32 ParseAThrone(Token *throneToken, ThroneInfo *throneInfo);
};


// Lifecycle (new in civapp + cleanup via allocated::clear) lives in
// civapp.cpp; the variable is file-scope `static` in gs/utility/gameinit.cpp.
// External readers go through thronedb_Get(); the civapp lifecycle code
// uses thronedb_Set() for the new/clear writes.
ThroneDB * thronedb_Get();
void       thronedb_Set(ThroneDB *p);

#endif
