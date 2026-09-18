//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Playlist database
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
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "gs/fileio/Token.h"

#include "gs/database/PlayListDB.h"
#include "ctp/ctp2_utils/c3files.h"

enum TOKEN_PLAYLIST {
	TOKEN_PLAYLIST_NUM_SONGS = TOKEN_MAX+1,
	TOKEN_PLAYLIST_SONG_LIST,

	TOKEN_PLAYLIST_MAX
};

TokenData g_playlist_token_data [static_cast<sint32>(TOKEN_PLAYLIST_MAX) - TOKEN_MAX] = {
    { TOKEN_PLAYLIST_NUM_SONGS,		"NUM_SONGS"},
	{ TOKEN_PLAYLIST_SONG_LIST,		"SONG_LIST"},
};

PlayListDB::PlayListDB()
{
	m_numSongs = 0;
}

PlayListDB::~PlayListDB() = default;

BOOL PlayListDB::Parse(MBCHAR *filename)
{
	auto playListToken = std::make_unique<Token>(filename, static_cast<sint32>(TOKEN_PLAYLIST_MAX) - TOKEN_MAX,
									g_playlist_token_data, C3DIR_GAMEDATA);

	sint32		val = 0;

	if (playListToken->GetType() == TOKEN_PLAYLIST_NUM_SONGS) {
		if (playListToken->Next() == TOKEN_NUMBER) {
			playListToken->GetNumber(val);
		}
	}

	m_numSongs = val;
	m_playList = std::make_unique<sint32[]>(m_numSongs);

	sint32 songListToken = playListToken->Next();

	if (songListToken == TOKEN_PLAYLIST_SONG_LIST) {
		for (sint32 i=0; i<m_numSongs; i++) {
			if (playListToken->Next() == TOKEN_NUMBER) {
				playListToken->GetNumber(val);
				m_playList[i] = val;
			} else {
				c3errors_ErrorDialog("PlayListDB", "Looking for a song number.");
				return FALSE;
			}
		}
	} else {
		c3errors_ErrorDialog("PlayListDB", "Missing token SONG_LIST.");
		return FALSE;
	}

	return TRUE;
}


