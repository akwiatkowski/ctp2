#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef _MESSAGEPOOL_H_
#define _MESSAGEPOOL_H_

class MessagePool;
typedef sint32 MESSAGE_TYPE;

#include "robot/aibackdoor/civarchive.h"	// CivArchive
#include "gs/gameobj/message.h"	// MessageData
#include "gs/gameobj/ObjPool.h"	// ObjPool
#include "gs/gameobj/Player.h"		// PLAYER_INDEX

#define k_HACK_RECONSTITUTED_CLASS 0x72adc754

class MessagePool : public ObjPool
{
	friend void to_json(nlohmann::json &j, MessagePool const &p);
	friend void from_json(nlohmann::json const &j, MessagePool &p);

public:
		MessageData* AccessMessage(const Message id)
			{
			return ((MessageData*)Access(id)) ;
			}

		MessageData* GetMessage(const Message id) const
			{
			return ((MessageData*)Get(id)) ;
			}

		MessagePool() ;
		MessagePool(CivArchive &archive) ;

		Message Create(PLAYER_INDEX owner, PLAYER_INDEX sender, MESSAGE_TYPE type, MBCHAR *msg) ;
		Message Create(PLAYER_INDEX owner, MBCHAR *msg) ;
		Message Create(PLAYER_INDEX owner, MessageData *copy);
		Message Recreate(PLAYER_INDEX owner, MBCHAR *msg, MBCHAR *title);
		Message ServerCreate();
		void Serialize(CivArchive &archive) ;
		void DoNetwork(MessageData *newData);

		void NotifySlicReload();
	} ;

// Lifecycle is split: gs/utility/gameinit.cpp handles fresh / archive
// construction and Cleanup; ctp/civapp.cpp re-allocates on SlicEngine
// reloads (delete-then-new).  Backing storage is file-scope `static`
// in gameinit.cpp; cross-TU writers go through messagepool_Set.
MessagePool * messagepool_Get();
void          messagepool_Set(MessagePool *p);

#endif
