#include "ctp/c3.h"
#include "gs/events/GameEventHook.h"
#include "gs/gameobj/Events.h"
#include "gs/gameobj/EventTracker.h"
#include "gs/gameobj/Unit.h"
#include "gs/events/GameEventArgList.h"
#include "gs/events/GameEventManager.h"
#include "gs/utility/TurnCnt.h"
#include "gs/gameobj/UnitData.h"

EventTracker::EventTracker()
{
	m_dataList=new PointerList<EventData>;
}


EventTracker::~EventTracker()
{
	m_dataList->DeleteAll();
	delete m_dataList;
}

void EventTracker::AddEvent(EVENT_TYPE type, sint32 playerNum, sint32 turn, sint32 dbIndex)
{
	EventData *newdata=new EventData;
	newdata->m_type=type;
	newdata->m_playerNum=playerNum;
	newdata->m_turn=turn;
	newdata->m_dbIndex=dbIndex;
	m_dataList->AddTail(newdata);
}

void EventTracker::ResetList()
{
}

EventData *EventTracker::GetEvents(BOOL Reset)
{
	static PointerList<EventData>::PointerListNode *curDataPtr;
	if(Reset)
	{
		curDataPtr=m_dataList->GetHeadNode();
	}
	else if(curDataPtr)
	{
		curDataPtr = curDataPtr->GetNext();
	}

	if(curDataPtr)
	{
		return curDataPtr->GetObj();
	}
	else
	{
		return nullptr;
	}
}


int EventTracker::GetEventCount()
{
	return m_dataList->GetCount();
}

STDEHANDLER(TrackCreateWonderEvent)
{
	Unit c;
	sint32 wonder;
	if(!args->GetCity(0, c)) return GEV_HD_Continue;
	if(!args->GetInt(0, wonder)) return GEV_HD_Continue;

	eventtracker_Get()->AddEvent(EVENT_TYPE_WONDER,c->GetOwner(),turn_Get()->GetSessionRound(),wonder);

	return GEV_HD_Continue;
}

void trackerevent_Initialize()
{
	gevmanager_Get()->AddCallback(GEV_CreateWonder, GEV_PRI_Post, &s_TrackCreateWonderEvent);
}

void trackerevent_Cleanup()
{
}
