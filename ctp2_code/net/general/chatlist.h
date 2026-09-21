#ifndef xCHAT_LIST_H__
#define xCHAT_LIST_H__

#include <memory>
#include <string>

#include "ctp/ctp2_utils/pointerlist.h"

#define k_CHAT_TEXT_TIME 15

class ChatText
{
  public:
	std::string m_text;
	sint32 m_sender;
	time_t m_timeAdded;

	ChatText(sint32 sender, const char *text)
		: m_text(text), m_sender(sender), m_timeAdded(time(nullptr))
	{}

	~ChatText() = default;
};

class ChatList
{
  public:
	PointerList<ChatText> m_list;

	ChatList() = default;

	~ChatList() {
		m_list.DeleteAll();
	}

	void AddLine(sint32 sender, const char *text) {
		m_list.AddHead(std::make_unique<ChatText>(sender, text).release());
	}

	void RemoveExpired() {
		while(m_list.GetTail() && (m_list.GetTail()->m_timeAdded + k_CHAT_TEXT_TIME < time(nullptr))) {
			std::unique_ptr<ChatText>(m_list.RemoveTail());
		}
	}
};

#endif
