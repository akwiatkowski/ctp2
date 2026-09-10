#include "ctp/c3.h"
#include "ui/interface/messagewin.h"

#include <memory>
#include <vector>

#include "gs/utility/Globals.h"        // allocated::...
#include "ui/aui_ctp2/SelItem.h"        // selitem_Get()

#include "ui/aui_common/aui.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_uniqueid.h"

#include "ui/aui_common/aui_static.h"
#include "ui/aui_common/aui_surface.h"

#include "ui/aui_ctp2/textbox.h"

#include "ui/aui_ctp2/textbutton.h"
#include "ui/aui_common/aui_button.h"
#include "ui/aui_common/aui_window.h"

#include "gs/gameobj/message.h"

#include "ui/aui_ctp2/c3windows.h"
#include "ui/interface/messageiconwindow.h"
#include "ui/interface/messagewindow.h"
#include "ui/interface/messageactions.h"
#include "ui/interface/messagelist.h"
#include "ui/interface/messagemodal.h"
#include "ui/interface/messageadvice.h"

#include "ui/interface/controlpanelwindow.h"

#include "ui/ldl/ldl_data.hpp"

extern MessageModal *   g_modalMessage;
extern sint32           g_ScreenWidth;
extern sint32           g_ScreenHeight;

// Per-player message lists, owned here; entries live from
// messagewin_InitializePlayerMessage until messagewin_PurgeMessages.
static std::vector<std::unique_ptr<MessageList>> g_messageUserList;

PLAYER_INDEX			g_currentPlayerMessages = 0;

uint16 g_messageReadPositionX = 32;
uint16 g_messageReadPositionY = 241;
uint8 g_messageMaxVisible = 6;

uint8 g_messageRespButtonSpacing = 4;
uint8 g_messageRespTextPadding = 5;
uint8 g_messageRespButtonWidth = 50;
uint8 g_messageRespDropPadding = 25;
uint8 g_messageEyeDropWidth = 70;
uint8 g_messageEyeGreatPadding = 3;
uint16 g_messageMoreX = 32;
uint16 g_messageMoreY = 30;
uint16 g_messageLessX = 32;
uint16 g_messageLessY = 276;
uint8 g_messageIconSpacing = 2;
uint8 g_messageIconHeight = 33;
uint8 g_messageIconWidth = 33;






















AUI_ERRCODE messagewin_InitializeMessages( )
{
	g_messageUserList.clear();

	ldl_datablock * block = aui_Ldl::FindDataBlock("MessageboxAttributes");
	Assert( block != nullptr );
	if ( !block ) {
		return AUI_ERRCODE_LDLFINDDATABLOCKFAILED;
	}

	if ( block->GetAttributeType( "readx" )
			== ATTRIBUTE_TYPE_INT )
		g_messageReadPositionX = static_cast<uint16>(block->GetInt("readx"));

	if ( block->GetAttributeType( "ready" )
			== ATTRIBUTE_TYPE_INT )
		g_messageReadPositionY = static_cast<uint16>(block->GetInt("ready"));

	if ( block->GetAttributeType( "maxvisible" )
			== ATTRIBUTE_TYPE_INT )
		g_messageMaxVisible = static_cast<uint8>(block->GetInt("maxvisible"));

	if ( block->GetAttributeType( "buttonspacing" )
			== ATTRIBUTE_TYPE_INT )
		g_messageRespButtonSpacing = static_cast<uint8>(block->GetInt("buttonspacing"));

	if ( block->GetAttributeType( "textpadding" )
			== ATTRIBUTE_TYPE_INT )
		g_messageRespTextPadding = static_cast<uint8>(block->GetInt("textpadding"));

	if ( block->GetAttributeType( "buttonwidth" )
			== ATTRIBUTE_TYPE_INT )
		g_messageRespButtonWidth = static_cast<uint8>(block->GetInt("buttonwidth"));

	if ( block->GetAttributeType( "dropdownpadding" )
			== ATTRIBUTE_TYPE_INT )
		g_messageRespDropPadding = static_cast<uint8>(block->GetInt("dropdownpadding"));

	if ( block->GetAttributeType( "eyewidth" )
			== ATTRIBUTE_TYPE_INT )
		g_messageEyeDropWidth = static_cast<uint8>(block->GetInt("eyewidth"));

	if ( block->GetAttributeType( "greatlibrary" )
			== ATTRIBUTE_TYPE_INT )
		g_messageEyeGreatPadding = static_cast<uint8>(block->GetInt("greatlibrary"));

	if ( block->GetAttributeType( "morex" )
			== ATTRIBUTE_TYPE_INT )
		g_messageMoreX = static_cast<uint16>(block->GetInt("morex"));

	if ( block->GetAttributeType( "morey" )
			== ATTRIBUTE_TYPE_INT )
		g_messageMoreY = static_cast<uint16>(block->GetInt("morey"));

	if ( block->GetAttributeType( "lessx" )
			== ATTRIBUTE_TYPE_INT )
		g_messageLessX = static_cast<uint16>(block->GetInt("lessx"));

	if ( block->GetAttributeType( "lessy" )
			== ATTRIBUTE_TYPE_INT )
		g_messageLessY = static_cast<uint16>(block->GetInt("lessy"));

	if ( block->GetAttributeType( "miconspacing" )
			== ATTRIBUTE_TYPE_INT )
		g_messageIconSpacing = static_cast<uint8>(block->GetInt("miconspacing"));

	if ( block->GetAttributeType( "miconheight" )
			== ATTRIBUTE_TYPE_INT )
		g_messageIconHeight = static_cast<uint8>(block->GetInt("miconheight"));

	if ( block->GetAttributeType( "miconwidth" )
			== ATTRIBUTE_TYPE_INT )
		g_messageIconWidth = static_cast<uint8>(block->GetInt("miconwidth"));

    sint32 distanceFromScreenBottom = (g_ScreenWidth < 1024) ? 270 : 300;

    g_messageReadPositionX = 16;
	g_messageReadPositionY = (uint16)(g_ScreenHeight - distanceFromScreenBottom);

	g_messageMaxVisible = static_cast<uint8>
        ((g_messageReadPositionY / (g_messageIconHeight + g_messageIconSpacing)) - 3);

	g_messageMoreX = g_messageReadPositionX;
	g_messageMoreY = g_messageReadPositionY -
						(g_messageIconHeight + g_messageIconSpacing) * g_messageMaxVisible;
	g_messageLessX = g_messageReadPositionX;
	g_messageLessY = g_messageReadPositionY + g_messageIconHeight + g_messageIconSpacing;

	return AUI_ERRCODE_OK;
}

MessageList *messagewin_InitializePlayerMessage( PLAYER_INDEX index )
{
	if ( messagewin_GetPlayerMessageList( index )) return nullptr;

	std::unique_ptr<MessageList> list(new MessageList(index));

	MessageList *listPtr = list.get();
	g_messageUserList.push_back(std::move(list));

	return listPtr;
}


MessageList *messagewin_GetPlayerMessageList( PLAYER_INDEX index )
{
	for (auto &userList : g_messageUserList)
    {
        if (userList->GetPlayer() == index)
        {
			return userList.get();
        }
	}

	return nullptr;
}














int messagewin_CreateMessage( Message data, BOOL bRecreate )
{
	if (data.IsHelpBox())
    {
		return 0;
	}

	controlpanel_Get()->AddMessage(data);

	MessageList *messagelist = messagewin_GetPlayerMessageList( data.GetOwner() );

	if ( !messagelist ) {

		messagelist = messagewin_InitializePlayerMessage( data.GetOwner( ));
		Assert( messagelist != nullptr );
		if ( messagelist == nullptr ) return -1;
	}

	AUI_ERRCODE errcode = messagelist->CreateMessage(data);
	Assert( errcode == AUI_ERRCODE_OK );
	if ( errcode != AUI_ERRCODE_OK ) return -1;

	if (!bRecreate)
    {
		MBCHAR const * wavName = data.AccessData()->GetMsgSound();
		if (wavName)
        {
			MBCHAR filename[_MAX_PATH];

			if (civpaths_Get()->FindFile(C3DIR_SOUNDS, wavName, filename))
            {
#if defined(WIN32)
			    PlaySound(filename, NULL, SND_ASYNC | SND_FILENAME);
#endif
            }
		}
	}

	return 1;
}


int messagewin_CreateModalMessage(Message data)
{
	messagemodal_CreateModalMessage(data);

	MBCHAR const *  wavName = data.AccessData()->GetMsgSound();
	if (wavName)
    {
		MBCHAR filename[ _MAX_PATH ];
		if (civpaths_Get()->FindFile( C3DIR_SOUNDS, wavName, filename))
        {
#if defined(WIN32)
		    PlaySound(filename, NULL, SND_ASYNC | SND_FILENAME);
#endif
        }
	}

	return 1;
}


int messagewin_PrepareDestroyWindow( MessageWindow *window )
{
	Assert( window != nullptr );
	if ( !window ) return -1;




	c3ui_Get()->AddDestructiveAction( new MessageCleanupAction( window, window->GetPlayer() ) );
	return 1;
}








int messagewin_FastKillWindow(MessageWindow *window)
{
	sint32			player = window->GetPlayer();

	messagewin_CleanupMessage( window );

	MessageList	* messagelist = messagewin_GetPlayerMessageList(player);

	if (messagelist)
    {
	    messagelist->CheckVisibleMessages();
    }

	return 1;
}













int messagewin_CleanupMessage( MessageWindow *window )
{
	return messagewin_CleanupMessage(window->GetIconWindow(), window);
}


int messagewin_CleanupMessage( MessageIconWindow *iconWindow )
{
	return messagewin_CleanupMessage(iconWindow, iconWindow->GetWindow());
}


int messagewin_CleanupMessage( MessageIconWindow *iconWindow,
							   MessageWindow *window )
{

	Assert ( !g_messageUserList.empty() );
	if ( g_messageUserList.empty() ) return -1;

	MessageList *messagelist = messagewin_GetPlayerMessageList( window->GetPlayer() );
	Assert( messagelist != nullptr );
	if ( messagelist == nullptr ) return -1;


	controlpanel_Get()->RemoveMessage(*window->GetMessage());

	messagelist->Remove( iconWindow, window );

	return 1;
}





void messagewin_Cleanup()
{
	messagewin_PurgeMessages();
}

void messagewin_PurgeMessages()
{
	messagewin_LessMessagesIcon( FALSE, TRUE );
	messagewin_MoreMessagesIcon( FALSE, TRUE );

	g_messageUserList.clear();
}


void messagewin_EndTurn( PLAYER_INDEX index )
{
	MessageList	* messagelist = messagewin_GetPlayerMessageList(index);
	if (messagelist)
    {
	    messagelist->HideVisibleWindows();
	    messagewin_MoreMessagesIcon( FALSE );
	    messagewin_LessMessagesIcon( FALSE );
    }
}


void messagewin_BeginTurn( PLAYER_INDEX index )
{
	if ( g_currentPlayerMessages != index ) {
		messagewin_EndTurn( g_currentPlayerMessages );
	}

	g_currentPlayerMessages = index;

	MessageList *messagelist = messagewin_GetPlayerMessageList( index );
	if ( !messagelist ) {
		messagelist = messagewin_InitializePlayerMessage( index );
		Assert( messagelist != nullptr );
		if ( messagelist == nullptr ) return;
	}

	messagelist->CheckVisibleMessages( );
}


int messagewin_MoreMessagesIcon( BOOL make, BOOL destroy )
{

return 1;
}

int messagewin_LessMessagesIcon( BOOL make, BOOL destroy )
{

return 1;
}

int messagewin_IsModalMessageDisplayed()
{
	return g_modalMessage != nullptr;
}
