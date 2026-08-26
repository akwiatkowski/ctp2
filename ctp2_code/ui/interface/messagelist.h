#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __MESSAGELIST_H__
#define __MESSAGELIST_H__

class MessageList;

#include <memory>
#include <vector>

#include "gs/gameobj/Player.h"         // PLAYER_INDEX

class Message;
class MessageIconWindow;
class MessageWindow;

class MessageList
{
public:
	MessageList( PLAYER_INDEX player );
	virtual ~MessageList();

	PLAYER_INDEX	GetPlayer( ) { return m_player; }

	// The icons form a prev/next chain used for animation; new icons need
	// the current tail and the count to place themselves.
	MessageIconWindow	*GetTailIcon()
	{
		return m_iconList.empty() ? nullptr : m_iconList.back().get();
	}
	uint32 GetIconCount( ) const { return static_cast<uint32>(m_iconList.size()); }

	AUI_ERRCODE CreateMessage( Message data );

	void HideVisibleWindows( );
	void CheckVisibleMessages( );
	void CheckMaxMessages( );
	void ChangeOffset( sint32 offset, int flag );
	void Remove( MessageIconWindow *iconWindow, MessageWindow *window );

	uint32 GetOffset( ) { return m_offset; }

private:
	PLAYER_INDEX							m_player;

	// The icon windows are owned here, in creation order; each icon window
	// carries a non-owning back-pointer to its MessageWindow, which this
	// class deletes first.
	std::vector<std::unique_ptr<MessageIconWindow>>	m_iconList;
	uint32									m_offset = 0;

};




#endif
