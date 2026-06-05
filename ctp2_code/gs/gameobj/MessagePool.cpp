#include "ctp/c3.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/Player.h"
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
	MessageData* newData;

	Message newRequest(NewKey(k_BIT_GAME_OBJ_TYPE_MESSAGE));

	newData = new MessageData(newRequest, owner, sender, type, msg, turn_Get() ? turn_Get()->GetYear() : 0) ;
	Insert(newData) ;

	player_Get(owner)->AddMessage(newRequest) ;
	DoNetwork(newData);

	return (newRequest) ;
	}












Message MessagePool::Create(PLAYER_INDEX owner, MBCHAR *msg)
	{
	MessageData* newData;

	Message newMessage(NewKey(k_BIT_GAME_OBJ_TYPE_MESSAGE));

	newData = new MessageData(newMessage, owner, PLAYER_INDEX_INVALID, 0, msg, turn_Get() ? turn_Get()->GetYear() : 0) ;
	Insert(newData) ;

	player_Get(owner)->AddMessage(newMessage) ;

	DoNetwork(newData);

	return (newMessage) ;
	}

Message MessagePool::Recreate(PLAYER_INDEX owner, MBCHAR *msg, MBCHAR *title)
	{
	MessageData* newData;

	Message newMessage(NewKey(k_BIT_GAME_OBJ_TYPE_MESSAGE));

	newData = new MessageData(newMessage, owner, PLAYER_INDEX_INVALID, 0, msg, turn_Get() ? turn_Get()->GetYear() : 0) ;
	Insert(newData) ;

	if(title)
		newData->SetTitle(title);

	player_Get(owner)->AddMessage(newMessage) ;

	DoNetwork(newData);

	return (newMessage) ;
	}

Message MessagePool::Create(PLAYER_INDEX owner, MessageData *copy)
{
	MessageData *newData;
	Message newMessage(NewKey(k_BIT_GAME_OBJ_TYPE_MESSAGE));
	newData = new MessageData(newMessage, copy);
	newData->SetOwner(owner);
	Insert(newData);
	player_Get(owner)->AddMessage(newMessage);




	return newMessage;
}











Message MessagePool::ServerCreate()
{
	MessageData *newData;
	Message newRequest(NewKey(k_BIT_GAME_OBJ_TYPE_MESSAGE));
	newData = new MessageData(newRequest, turn_Get() ? turn_Get()->GetYear() : 0);
	Insert(newData);
	return newRequest;
}

void MessagePool::DoNetwork(MessageData *newData)
{

	#if 0   // Unreachable
    if(network_Get().IsClient()) {
		Assert(newData->GetSender() == network_Get().GetPlayerIndex() ||
			   newData->GetSender() == PLAYER_INDEX_INVALID);
		if(newData->GetSender() == network_Get().GetPlayerIndex() ||
			newData->GetSender() == PLAYER_INDEX_INVALID) {
			network_Get().AddCreatedObject(newData);
			network_Get().SendMessage(newData);
		}
	} else if(network_Get().IsHost()) {
		if(newData->GetSender() != PLAYER_INDEX_INVALID) {
			network_Get().Block(newData->GetSender());
		}
		network_Get().Enqueue(newData);
		if(newData->GetSender() != PLAYER_INDEX_INVALID) {
			network_Get().Unblock(newData->GetSender());
		}
	}
#endif
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
