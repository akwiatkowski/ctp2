#ifndef EVENT_TRACKER_H
#define EVENT_TRACKER_H

#include "ctp/ctp2_utils/pointerlist.h"

#include <nlohmann/json.hpp>

void trackerevent_Initialize();
void trackerevent_Cleanup();

class CivArchive;

enum EVENT_TYPE
{
	EVENT_TYPE_NONE = -1,
	EVENT_TYPE_WONDER,
	EVENT_TYPE_FEAT,
	EVENT_TYPE_AGES,
	EVENT_TYPE_MAX,
};

struct EventData
{
	EVENT_TYPE m_type;
	sint32 m_playerNum;
	sint32 m_turn;
	sint32 m_dbIndex;
};

inline void to_json(nlohmann::json &j, EventData const &e)
{
	j = nlohmann::json{
		{"type",       static_cast<sint32>(e.m_type)},
		{"player_num", e.m_playerNum},
		{"turn",       e.m_turn},
		{"db_index",   e.m_dbIndex},
	};
}

inline void from_json(nlohmann::json const &j, EventData &e)
{
	e.m_type = static_cast<EVENT_TYPE>(j.at("type").get<sint32>());
	j.at("player_num").get_to(e.m_playerNum);
	j.at("turn")      .get_to(e.m_turn);
	j.at("db_index")  .get_to(e.m_dbIndex);
}

class EventTracker
{
public:
	EventTracker();
	EventTracker(CivArchive &archive);
	~EventTracker();
	void AddEvent(EVENT_TYPE type, sint32 playerNum, sint32 turn, sint32 dbIndex);
	void ResetList();
	EventData *GetEvents(BOOL Reset);
	void Serialize(CivArchive &archive);
	int GetEventCount();
public:
	PointerList<EventData> *m_dataList;

	// JSON bridge — mirrors EventTracker::Serialize.  Persists every
	// EventData in m_dataList.
	friend void to_json(nlohmann::json &j, EventTracker const &t);
	friend void from_json(nlohmann::json const &j, EventTracker &t);
};

inline void to_json(nlohmann::json &j, EventTracker const &t)
{
	nlohmann::json events = nlohmann::json::array();
	if (t.m_dataList)
	{
		PointerList<EventData>::Walker w(t.m_dataList);
		while (w.IsValid())
		{
			events.push_back(*w.GetObj());
			w.Next();
		}
	}
	j = nlohmann::json{{"events", std::move(events)}};
}

inline void from_json(nlohmann::json const &j, EventTracker &t)
{
	if (!t.m_dataList) t.m_dataList = new PointerList<EventData>;
	t.m_dataList->DeleteAll();
	for (auto const &entry : j.at("events"))
	{
		EventData *e = new EventData;
		entry.get_to(*e);
		t.m_dataList->AddTail(e);
	}
}

// g_eventTracker demoted to file-scope `static` in gameinit.cpp.
// External callers go through eventtracker_Get() (returns NULL before
// the game state is loaded).
EventTracker * eventtracker_Get(void);

#endif
