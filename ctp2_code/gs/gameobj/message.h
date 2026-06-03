#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __MESSAGE_H__
#define __MESSAGE_H__

class Message;

#include "gs/gameobj/Advances.h"       // AdvanceType
#include "gs/gameobj/ID.h"             // ID
#include "gs/gameobj/MessageData.h"    // MESSAGE_RESPONSE_TYPE, MESSAGE_TYPE
#include "gs/gameobj/Player.h"         // PLAYER_INDEX
#include <nlohmann/json.hpp>

class Message : public ID
{
public:
		Message() : ID() { }
		Message(sint32 val) : ID(val) { }
		Message(uint32 val) : ID(val) { }

		void KillMessage() ;
		void Kill() { KillMessage() ; }
		void MessageRead() { KillMessage() ; }

		void RemoveAllReferences() ;

		void MinimizeMessage();
		void Minimize() {MinimizeMessage();}

		const MessageData* GetData() const ;
		MessageData* AccessData() ;

		void Castrate() {} ;
		PLAYER_INDEX GetOwner() const { return (GetData()->GetOwner()) ; }
		PLAYER_INDEX GetSender() const { return (GetData()->GetSender()) ; }
		MESSAGE_TYPE GetMsgType() const { return (GetData()->GetMsgType()) ; }
		MBCHAR *GetText() { return (AccessData()->GetMsgText()) ; }
		void ToString(MBCHAR *s) { AccessData()->ToString(s) ; }
		MESSAGE_RESPONSE_TYPE Reject() { return (AccessData()->Reject()) ; }
		MESSAGE_RESPONSE_TYPE Accept() { return (AccessData()->Accept()) ; }
		void Dump(const sint32 i) { AccessData()->Dump(i) ; }
		void Show();

		void SetSelectedAdvance(AdvanceType advance);
		AdvanceType GetSelectedAdvance() const;

		void SetDuration(sint32 duration, sint32 currentRound);
		sint32 GetExpiration() const;

		void SetIsHelpBox() { AccessData()->SetIsHelpBox(); }
		BOOL IsHelpBox() const { return GetData()->IsHelpBox(); }

		void SetIsAlertBox() { AccessData()->SetIsAlertBox(); }
		BOOL IsAlertBox() const { return GetData()->IsAlertBox(); }

		void SetIsInstantMessage() { AccessData()->SetIsInstantMessage(); }
		BOOL IsInstantMessage() const { return GetData()->IsInstantMessage(); }

		BOOL IsRead() const { return GetData()->IsRead(); }
		void SetRead() { ((MessageData*)GetData())->SetRead(); }

		BOOL IsDiplomaticResponse() const { return GetData()->IsDiplomaticResponse(); }

		void SetClass(sint32 mclass) { AccessData()->SetClass(mclass); }
		sint32 GetClass() const { return GetData()->GetClass(); }

		void SetUseDirector() { AccessData()->SetUseDirector(); }
		BOOL UseDirector() const { return GetData()->UseDirector(); }
};

// JSON bridge — Message is a pure ID-derived handle, serialise as uint32.
inline void to_json(nlohmann::json &j, Message const &m)
{
    j = m.m_id;
}

inline void from_json(nlohmann::json const &j, Message &m)
{
    m.m_id = j.get<uint32>();
}

#endif
