#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __MESSAGELIST_H__
#define __MESSAGELIST_H__

class MessageList;

#include "gs/gameobj/Player.h"         // PLAYER_INDEX
#include "ui/aui_common/tech_wllist.h"    // tech_WLList

class Message;
class MessageIconWindow;
class MessageWindow;

class MessageList
{
public:
	MessageList( PLAYER_INDEX player );
	virtual ~MessageList();

	PLAYER_INDEX	GetPlayer( ) { return m_player; }
	tech_WLList<MessageIconWindow *>	*GetList( ) { return m_iconList; }

	AUI_ERRCODE CreateMessage( Message data );

	void HideVisibleWindows( );
	void CheckVisibleMessages( );
	void CheckMaxMessages( );
	void ChangeOffset( sint32 offset, int flag );
	void Remove( MessageIconWindow *iconWindow, MessageWindow *window );

	uint32 GetOffset( ) { return m_offset; }

private:
	PLAYER_INDEX							m_player;
	tech_WLList<MessageIconWindow *>		*m_iconList;
	uint32									m_offset;

};




#endif
