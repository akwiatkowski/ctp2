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

#include "gs/core/game_observer.h"   // gameobservers_Get()

#include "gs/slic/SlicButton.h"
#include "gs/gameobj/DiplomaticRequestPool.h"










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

	if (player_arr_Get() && player_Get(GetOwner()))
		player_Get(GetOwner())->RemoveMessageReferences(*this) ;

	AccessData()->KillMessageWindow();
	messagepool_Get()->Del(*this) ;
	}









const MessageData* Message::GetData() const
	{
	return (messagepool_Get()->GetMessage(*this)) ;
	}









MessageData* Message::AccessData()
	{
	return (messagepool_Get()->AccessMessage(*this)) ;
	}











void Message::Show()
{
	if (!AccessData()) return;
	if (gameobservers_Get()) gameobservers_Get()->NotifyMessageShow(*this);
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

void Message::SetDuration(sint32 duration, sint32 currentRound)
{
	AccessData()->SetDuration(duration, currentRound);
}

sint32 Message::GetExpiration() const
{
	return GetData()->GetExpiration();
}

void Message::MinimizeMessage()
{
	if (gameobservers_Get()) gameobservers_Get()->NotifyMessageMinimize(*this);
}
