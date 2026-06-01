#include "ctp/c3.h"

#include "robot/aibackdoor/civarchive.h"
#include "gs/gameobj/Player.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/DiplomaticRequestData.h"
#include "gs/gameobj/DiplomaticRequest.h"
#include "gs/gameobj/MessageData.h"
#include "gs/gameobj/message.h"
#include "gs/gameobj/MessagePool.h"
#include "net/general/network.h"
#include "net/general/net_info.h"

#include "gs/core/game_observer.h"   // g_gameObservers

#include "gs/slic/SlicButton.h"
#include "gs/gameobj/DiplomaticRequestPool.h"

extern	Player	**g_player ;

extern	MessagePool	*g_theMessagePool ;








void Message::KillMessage()
	{
	Message	tmp(*this) ;
	tmp.RemoveAllReferences() ;
	}









void Message::RemoveAllReferences()
	{
	SlicButton *closeEvent = AccessData()->GetCloseEvent();
	if(closeEvent) {
		closeEvent->Callback();
	}

	if (g_player && g_player[GetOwner()])
		g_player[GetOwner()]->RemoveMessageReferences(*this) ;

	AccessData()->KillMessageWindow();
	g_theMessagePool->Del(*this) ;
	}









const MessageData* Message::GetData() const
	{
	return (g_theMessagePool->GetMessage(*this)) ;
	}









MessageData* Message::AccessData()
	{
	return (g_theMessagePool->AccessMessage(*this)) ;
	}











void Message::Show()
{
	if (!AccessData()) return;
	if (g_gameObservers) g_gameObservers->NotifyMessageShow(*this);
	SetRead();
}

void Message::SetSelectedAdvance(AdvanceType adv)
{
	AccessData()->SetSelectedAdvance(adv);
}

AdvanceType Message::GetSelectedAdvance() const
{
	return GetData()->GetSelectedAdvance();
}

void Message::SetDuration(sint32 duration)
{
	AccessData()->SetDuration(duration);
}

sint32 Message::GetExpiration() const
{
	return GetData()->GetExpiration();
}

void Message::MinimizeMessage()
{
	if (g_gameObservers) g_gameObservers->NotifyMessageMinimize(*this);
}
