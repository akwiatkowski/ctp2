#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef _DIPLOMATICREQUESTPOOL_H_
#define _DIPLOMATICREQUESTPOOL_H_

#include "gs/gameobj/ObjPool.h"

#include "gs/gameobj/DiplomaticRequest.h"
#include <nlohmann/json.hpp>

enum REQUEST_TYPE ;

class DiplomaticRequestData;
class MessageDynamicArray ;

class DiplomaticRequestPool : public ObjPool
	{













	public:
		DiplomaticRequestData* AccessDiplomaticRequest(const DiplomaticRequest id)
			{
			return ((DiplomaticRequestData*)Access(id)) ;
			}

		DiplomaticRequestData* GetDiplomaticRequest(const DiplomaticRequest id) const
			{
			return ((DiplomaticRequestData*)Get(id)) ;
			}

		DiplomaticRequestPool() ;
		DiplomaticRequestPool(CivArchive &archive) ;

		DiplomaticRequest Create(PLAYER_INDEX owner, PLAYER_INDEX recipient, REQUEST_TYPE request) ;
		DiplomaticRequestData *CreateData();

		void EndTurn(const PLAYER_INDEX sender) ;
		void EndTurn(DiplomaticRequestData *top, const PLAYER_INDEX sender,
					 MessageDynamicArray &msgExpired);
		void Serialize(CivArchive &archive) override ;

		friend void to_json(nlohmann::json &j, DiplomaticRequestPool const &p);
		friend void from_json(nlohmann::json const &j, DiplomaticRequestPool &p);
	} ;

// Lifecycle (new / archive-load / Cleanup) lives in
// gs/utility/gameinit.cpp; the variable is now file-scope `static`
// there.  External readers go through diplomaticrequestpool_Get().
DiplomaticRequestPool * diplomaticrequestpool_Get();
void                    diplomaticrequestpool_Set(DiplomaticRequestPool *p);
#else

class DiplomaticRequestPool ;
DiplomaticRequestPool * diplomaticrequestpool_Get();
void                    diplomaticrequestpool_Set(DiplomaticRequestPool *p);

#endif
