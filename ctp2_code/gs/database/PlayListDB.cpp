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

TokenData g_playlist_token_data [TOKEN_PLAYLIST_MAX - TOKEN_MAX] = {
    { TOKEN_PLAYLIST_NUM_SONGS,		"NUM_SONGS"},
	{ TOKEN_PLAYLIST_SONG_LIST,		"SONG_LIST"},
};

PlayListDB::PlayListDB()
{
	m_numSongs = 0;
	m_playList = nullptr;
}

PlayListDB::~PlayListDB()
{
	
		delete m_playList;
}

BOOL PlayListDB::Parse(MBCHAR *filename)
{
	fprintf(stderr, "[PLDB] Parse: opening %s\n", filename);
	Token *playListToken = new Token(filename, TOKEN_PLAYLIST_MAX - TOKEN_MAX,
									g_playlist_token_data, C3DIR_GAMEDATA);

	Assert(playListToken);
	if (!playListToken) return FALSE;

	fprintf(stderr, "[PLDB] Parse: first token type=%d (NUM_SONGS=%d)\n",
		playListToken->GetType(), TOKEN_PLAYLIST_NUM_SONGS);

	sint32		val = 0;

	if (playListToken->GetType() == TOKEN_PLAYLIST_NUM_SONGS) {
		if (playListToken->Next() == TOKEN_NUMBER) {
			playListToken->GetNumber(val);
		}
	}

	fprintf(stderr, "[PLDB] Parse: numSongs=%d\n", val);

	m_numSongs = val;
	m_playList = new sint32[m_numSongs];

	fprintf(stderr, "[PLDB] Parse: getting SONG_LIST\n");
	sint32 songListToken = playListToken->Next();
	fprintf(stderr, "[PLDB] Parse: SONG_LIST token=%d (expected=%d)\n",
		songListToken, TOKEN_PLAYLIST_SONG_LIST);

	if (songListToken == TOKEN_PLAYLIST_SONG_LIST) {
		for (sint32 i=0; i<m_numSongs; i++) {
			fprintf(stderr, "[PLDB] Parse: getting song %d/%d\n", i+1, m_numSongs);
			if (playListToken->Next() == TOKEN_NUMBER) {
				playListToken->GetNumber(val);
				m_playList[i] = val;
			} else {
				fprintf(stderr, "[PLDB] Parse: ERROR expected number at song %d\n", i+1);
				c3errors_ErrorDialog("PlayListDB", "Looking for a song number.");
				delete playListToken;
				return FALSE;
			}
		}
	} else {
		fprintf(stderr, "[PLDB] Parse: ERROR missing SONG_LIST\n");
		c3errors_ErrorDialog("PlayListDB", "Missing token SONG_LIST.");
		delete playListToken;
		return FALSE;
	}

	fprintf(stderr, "[PLDB] Parse: done, deleting token\n");
	delete playListToken;

	fprintf(stderr, "[PLDB] Parse: returning TRUE\n");
	return TRUE;
}


