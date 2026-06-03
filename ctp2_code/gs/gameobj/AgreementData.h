//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Agreement data
// Id           : $Id$
//
//----------------------------------------------------------------------------
//
// Disclaimer
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
// This material has been developed at apolyton.net by the Apolyton CtP2
// Source Code Project. Contact the authors at ctp2source@apolyton.net.
//
//----------------------------------------------------------------------------
//
// Compiler flags
//
// - None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - None
//
//----------------------------------------------------------------------------

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __AGREEMENTDATA_H__
#define __AGREEMENTDATA_H__

#include "gs/gameobj/GameObj.h"
#include "gs/gameobj/Unit.h"
#include "gs/utility/gstypes.h"

#include <nlohmann/json.hpp>

class CivArchive;

typedef sint32 PLAYER_INDEX ;
typedef sint32 AdvanceType;

#define k_EXPIRATION_NEVER		(sint32)(-1)


#include "gs/gameobj/AgreementTypes.h"

class AgreementData : public GameObj
{
private:

	PLAYER_INDEX	m_owner,
		            m_recipient,
		            m_thirdParty ;

	AGREEMENT_TYPE	m_agreement ;

	sint32	m_round,
		    m_expires ;

	uint32  m_ownerPollution,
		    m_recipientPollution;

	BOOL    m_isBroken;





	Unit	m_targetCity ;




	void ExtractPlayer(sint32 indexId, sint32 memberId, MBCHAR *sExpanded) ;
	void Interpret(MBCHAR *msg, MBCHAR *sInterpreted) ;

	friend class NetAgreement ;
	friend class NetClientAgreement ;
	// JSON bridge — covers the scalar fields + m_targetCity.  The
	// intrusive linked-list pointers m_lesser/m_greater are OMITTED:
	// the AgreementPool's JSON bridge (future work) will flatten
	// agreements into a top-level array, sidestepping the recursive
	// binary serialisation pattern.
	friend void to_json(nlohmann::json &j, AgreementData const &a);
	friend void from_json(nlohmann::json const &j, AgreementData &a);

public:
	AgreementData(const ID id) ;
	AgreementData(const ID id,
				  PLAYER_INDEX sender,
				  PLAYER_INDEX recipient,
				  AGREEMENT_TYPE agreement,
				  sint32 currentRound) ;
	AgreementData(CivArchive &archive);

	void Init();

	AGREEMENT_TYPE GetAgreement() const { return (m_agreement) ; }

	PLAYER_INDEX GetOwner() const { return (m_owner) ; }

	PLAYER_INDEX GetRecipient() const { return (m_recipient) ; }

	PLAYER_INDEX GetThirdParty() const { return (m_thirdParty) ; }

	Unit GetTarget() const { return (m_targetCity) ; }

	sint32 GetTurns() const { return (m_expires) ; }

	sint32 GetStartTurn() const { return (m_round) ; }

	BOOL IsExpired() const { return (m_expires == 0) ; }

	BOOL DoesExpire() const { return (m_expires != k_EXPIRATION_NEVER) ; }

	void SetTarget(const Unit &city) ;
	void SetExpires(const sint32 turns);
	void SetThirdParty(const PLAYER_INDEX player) {
		Assert((player>=0) && (player<=k_MAX_PLAYERS)) ;
		m_thirdParty = player ;
	}

	void Expire() { m_expires = 0 ; }

	void MakeAgreement(const PLAYER_INDEX owner,
					   const PLAYER_INDEX recipient,
					   const AGREEMENT_TYPE agreement,
					   sint32 currentRound) ;
	void FulfillAgreement() ;
	sint32 DecrementTurns() ;

	void EndTurn() ;

	void ToString(MBCHAR *s) ;

	void Dump(const sint32 i) ;
	void Serialize(CivArchive &archive) ;

	BOOL IsBroken() const { return m_isBroken; }
	void Break();

	void RecipientIsViolating(PLAYER_INDEX curPlayer, BOOL forceBreak, sint32 currentRound);
	void OwnerIsViolating(PLAYER_INDEX curPlayer, sint32 currentRound);
	void BeginTurnOwner(sint32 currentRound);
	void BeginTurnRecipient(sint32 currentRound);
};

#endif
