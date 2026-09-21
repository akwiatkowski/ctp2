#include "ctp/c3.h"
#include <memory>
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/player.h"
#include "gs/utility/safety.h"          // safe_player
#include "gs/gameobj/MessageData.h"
#include "gs/gameobj/message.h"
#include "gs/gameobj/MessagePool.h"

#include "net/general/network.h"
#include "gs/utility/Globals.h"
#include "gs/utility/TurnCnt.h"









MessagePool::MessagePool() : ObjPool(k_BIT_GAME_OBJ_TYPE_MESSAGE)
	{
	}









Message MessagePool::Create(PLAYER_INDEX owner, PLAYER_INDEX sender, MESSAGE_TYPE type, MBCHAR *msg)
	{
	Message newRequest(NewKey(k_BIT_GAME_OBJ_TYPE_MESSAGE)) ;
	auto newData = std::make_unique<MessageData>(newRequest, owner, sender, type, msg, turn_Get() ? turn_Get()->GetYear() : 0) ;
	MessageData *newDataPtr = newData.get();
	Insert(newData.release()) ;

	if(Player * p = safe_player(owner))
		p->AddMessage(newRequest) ;
	DoNetwork(newDataPtr);

	return (newRequest) ;
	}












Message MessagePool::Create(PLAYER_INDEX owner, MBCHAR *msg)
	{
	Message newMessage(NewKey(k_BIT_GAME_OBJ_TYPE_MESSAGE)) ;
	auto newData = std::make_unique<MessageData>(newMessage, owner, PLAYER_INDEX_INVALID, 0, msg, turn_Get() ? turn_Get()->GetYear() : 0) ;
	MessageData *newDataPtr = newData.get();
	Insert(newData.release()) ;

	if(Player * p = safe_player(owner))
		p->AddMessage(newMessage) ;

	DoNetwork(newDataPtr);

	return (newMessage) ;
	}

Message MessagePool::Recreate(PLAYER_INDEX owner, MBCHAR *msg, MBCHAR *title)
	{
	Message newMessage(NewKey(k_BIT_GAME_OBJ_TYPE_MESSAGE)) ;
	auto newData = std::make_unique<MessageData>(newMessage, owner, PLAYER_INDEX_INVALID, 0, msg, turn_Get() ? turn_Get()->GetYear() : 0) ;
	MessageData *newDataPtr = newData.get();
	Insert(newData.release()) ;

	if(title)
		newDataPtr->SetTitle(title);

	if(Player * p = safe_player(owner))
		p->AddMessage(newMessage) ;

	DoNetwork(newDataPtr);

	return (newMessage) ;
	}

Message MessagePool::Create(PLAYER_INDEX owner, MessageData *copy)
{
	Message newMessage(NewKey(k_BIT_GAME_OBJ_TYPE_MESSAGE));
	auto newData = std::make_unique<MessageData>(newMessage, copy);
	newData->SetOwner(owner);
	Insert(newData.release());
	if(Player * p = safe_player(owner))
		p->AddMessage(newMessage);




	return newMessage;
}











Message MessagePool::ServerCreate()
{
	Message newRequest(NewKey(k_BIT_GAME_OBJ_TYPE_MESSAGE));
	auto newData = std::make_unique<MessageData>(newRequest, turn_Get() ? turn_Get()->GetYear() : 0);
	Insert(newData.release());
	return newRequest;
}

void MessagePool::DoNetwork(MessageData *newData)
{

}

void MessagePool::NotifySlicReload()
{
	sint32 i;
	for(i = 0; i < k_OBJ_POOL_TABLE_SIZE; i++) {
		if(m_table[i]) {
			((MessageData *)(m_table[i]))->NotifySlicReload();
		}
	}
}
