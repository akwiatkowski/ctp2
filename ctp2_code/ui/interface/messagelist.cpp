#include "ctp/c3.h"
#include "ui/interface/messagelist.h"

#include "ui/aui_ctp2/SelItem.h"        // selitem_Get()
#include "gs/database/profileDB.h"      // profiledb_Get()

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
#include "ui/interface/messagewin.h"

extern uint16			g_messageReadPositionY;
extern uint8			g_messageMaxVisible;
extern uint8			g_messageIconHeight;
extern uint8			g_messageIconSpacing;

MessageList::MessageList(PLAYER_INDEX player)
:
	m_player    (player)
{
}

MessageList::~MessageList( )
{
	for (auto &iconEntry : m_iconList) {
		MessageIconWindow *iconWindow = iconEntry.get();
		if (iconWindow) {
			MessageWindow *window = iconWindow->GetWindow();

			if (window) {
				if ( c3ui_Get()->GetWindow( iconWindow->Id( )))
					c3ui_Get()->RemoveWindow( iconWindow->Id( ));

				if ( c3ui_Get()->GetWindow( window->Id( )))
					window->ShowWindow( FALSE );

				delete window;
			}

			delete iconWindow;
		}
	}
}

AUI_ERRCODE MessageList::CreateMessage( Message data )
{
	AUI_ERRCODE			errcode = AUI_ERRCODE_OK;
	MBCHAR				windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	std::unique_ptr<MessageIconWindow> createdIcon(new MessageIconWindow( &errcode,
										 aui_UniqueId(),
										 "MessageIconWindow",
										 data,
										 16,
										 this ));
	Assert( AUI_NEWOK( createdIcon, errcode ));
	if ( !AUI_NEWOK( createdIcon, errcode )) return AUI_ERRCODE_MEMALLOCFAILED;

	MessageIconWindow *mIconWindow = createdIcon.get();
	m_iconList.push_back(std::move(createdIcon));

	strlcpy( windowBlock, "StandardMessageWindow", sizeof(windowBlock) );

	std::unique_ptr<MessageWindow> createdWindow(new MessageWindow( &errcode, aui_UniqueId(), windowBlock,
								 16, data, mIconWindow ));
	Assert( AUI_NEWOK( createdWindow, errcode ));
	if ( !AUI_NEWOK( createdWindow, errcode )) return AUI_ERRCODE_MEMALLOCFAILED;

	mIconWindow->SetWindow( createdWindow.get() );
	// Ownership of the window passes to this list; it is released ahead of
	// the icon window in the destructor and in Remove().
	createdWindow.release();

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );
	if ( !AUI_SUCCESS(errcode) ) return AUI_ERRCODE_LDLFINDDATABLOCKFAILED;

	CheckVisibleMessages( );

	return AUI_ERRCODE_OK;

}

void MessageList::HideVisibleWindows( )
{
	if ( m_iconList.empty() ) return;

	for (auto &iconEntry : m_iconList)
    {
		MessageIconWindow * iconWindow = iconEntry.get();
		if (iconWindow)
        {
			MessageWindow * window = iconWindow->GetWindow();

			iconWindow->SetCurrentIconButton( nullptr );

			if ( c3ui_Get()->GetWindow( iconWindow->Id( )))
				c3ui_Get()->RemoveWindow( iconWindow->Id( ));

			if (window)
				if ( c3ui_Get()->GetWindow( window->Id( )))
					window->ShowWindow( FALSE );
		}
	}

}

void MessageList::CheckVisibleMessages( )
{




#if 0   // CTP1?
	uint32 minCount = m_offset;
	uint32 maxCount = ( m_offset + g_messageMaxVisible );
	uint32 count = 0;
	MessageIconWindow *iconWindow;

	if ( !selitem_Get() || m_player != selitem_Get()->GetVisiblePlayer( )) {
		HideVisibleWindows();
		return;
	}

	ListPos position = m_iconList->GetHeadPosition();

	for ( uint32 i = m_iconList->L(); i; i-- ) {
		iconWindow = m_iconList->GetNext( position );

		if (( count < maxCount ) && ( count >= minCount )) {
			if ( !c3ui_Get()->GetWindow( iconWindow->Id( )))
				if ( iconWindow->CheckShowWindow() ) {
					c3ui_Get()->AddWindow( iconWindow );
					if ( !BOUNCE_IT )

						iconWindow->Move( iconWindow->X(), ( g_messageReadPositionY -
								(( g_messageIconHeight + g_messageIconSpacing ) *
								 ( count - minCount ))));

				}
		} else {
			if ( c3ui_Get()->GetWindow( iconWindow->Id( )))
				c3ui_Get()->RemoveWindow( iconWindow->Id( ));
		}
		count++;
	}

	if (( count <= minCount ) && ( m_offset > 0 )) {
		ChangeOffset( -1, SHOW_MESSAGE_OFFSET_RELATIVE );
		return;
	}

	if ( count > maxCount )
		messagewin_MoreMessagesIcon( TRUE );
	else
		messagewin_MoreMessagesIcon( FALSE );

	if ( m_offset > 0 )
		messagewin_LessMessagesIcon( TRUE );
	else
		messagewin_LessMessagesIcon( FALSE );

#endif

}




void MessageList::CheckMaxMessages( )
{

	uint32 count = m_iconList.size();
	uint32 maxCount = ( m_offset + g_messageMaxVisible );

	if ( count > maxCount )
		messagewin_MoreMessagesIcon( TRUE );
	else
		messagewin_MoreMessagesIcon( FALSE );

}


void MessageList::ChangeOffset( sint32 offset, int flag )
{
	if ( flag == SHOW_MESSAGE_OFFSET_ABSOLUTE )
		m_offset = offset;
	else
		m_offset = m_offset + offset;

	Assert( m_offset >= 0 );
	if ( m_offset < 0 ) {
		ChangeOffset( 0, SHOW_MESSAGE_OFFSET_ABSOLUTE );
		return;
	}

	uint32 minCount = m_offset;
	uint32 maxCount = ( m_offset + g_messageMaxVisible );
	uint32 count = 0;

	for (auto &iconEntry : m_iconList) {
		MessageIconWindow *iconWindow = iconEntry.get();

		if (iconWindow) {
			if ( count >= minCount ) {
				if ( count <= maxCount ) {
					if ( !c3ui_Get()->GetWindow( iconWindow->Id( )))
						c3ui_Get()->AddWindow( iconWindow );

					if ( iconWindow->Animating( ))
						iconWindow->SetupAnimation( count - minCount );
					else
						iconWindow->Move( iconWindow->X(), ( g_messageReadPositionY -
								(( g_messageIconHeight + g_messageIconSpacing ) *
								 ( count - minCount ))));
				} else {
					iconWindow->SetupAnimation( count - minCount );
				}
			} else {
				if ( c3ui_Get()->GetWindow( iconWindow->Id( ))) {
					c3ui_Get()->RemoveWindow( iconWindow->Id( ));
					iconWindow->StopAnimation();
				}
			}
		}
		count++;
	}

	CheckVisibleMessages();
}

void MessageList::Remove( MessageIconWindow *iconWindow,
						  MessageWindow *window )
{

	if ( c3ui_Get()->GetWindow( iconWindow->Id( )))
		c3ui_Get()->RemoveWindow( iconWindow->Id( ));
	if ( c3ui_Get()->GetWindow( window->Id( )))
		window->ShowWindow(FALSE);

	auto found = std::find_if(m_iconList.begin(), m_iconList.end(),
		[iconWindow](std::unique_ptr<MessageIconWindow> const &entry)
		{ return entry.get() == iconWindow; });
	Assert( found != m_iconList.end() );
	if (found == m_iconList.end()) return;

	m_iconList.erase(found);

	if ( iconWindow == iconWindow->GetCurrentMessageIconWindow())
		iconWindow->SetCurrentIconButton( nullptr );

	uint32 count = 0;
	for (auto &iconEntry : m_iconList) {
		MessageIconWindow *iw = iconEntry.get();
		if (( count >= m_offset ) &&
			( count < ( m_offset + g_messageMaxVisible ))) {
			iw->SetupAnimation( count - m_offset );
		}
		count++;
	}

	delete window;
	delete iconWindow;

	CheckVisibleMessages();
}
