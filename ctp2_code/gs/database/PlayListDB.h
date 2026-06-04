#ifndef __PLAYLISTDB_H__
#define __PLAYLISTDB_H__


class PlayListDB
{
public:
	PlayListDB();
	~PlayListDB();

	BOOL Parse(MBCHAR *filename);

	sint32		GetNumSongs() { return m_numSongs; }
	sint32		GetSong(sint32 songNum) { return m_playList[songNum]; }

private:
	sint32		m_numSongs;
	sint32		*m_playList;
};

#endif
