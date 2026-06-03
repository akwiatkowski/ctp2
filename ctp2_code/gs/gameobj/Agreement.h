#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __AGREEMENT_H__
#define __AGREEMENT_H__

#include "gs/gameobj/ID.h"
#include "gs/gameobj/AgreementData.h"

class Unit;

class Agreement : public ID
{
public:
	Agreement() : ID() { return ; }
	Agreement(sint32 val) : ID(val) { return ; }
	Agreement(uint32 val) : ID(val) { return ; }

	void KillAgreement() ;
	void Kill() { KillAgreement() ; } ;
	void RemoveAllReferences() ;

	const AgreementData* GetData() const ;
	AgreementData* AccessData() ;

	void Castrate() {} ;
	PLAYER_INDEX GetOwner() const { return (GetData()->GetOwner()) ; }
	Unit GetTarget() { return (GetData()->GetTarget()) ; }
	PLAYER_INDEX GetRecipient() const { return (GetData()->GetRecipient()) ; }
	PLAYER_INDEX GetThirdParty() const { return (GetData()->GetThirdParty()) ; }
	AGREEMENT_TYPE GetAgreement() const { return (GetData()->GetAgreement()) ; }

	void SetExpires(const sint32 turns) { AccessData()->SetExpires(turns) ; }
	BOOL IsExpired() const { return (GetData()->IsExpired()) ; }
	BOOL DoesExpire() const { return (GetData()->DoesExpire()) ; }
	sint32 GetTurns() const { return (GetData()->GetTurns()) ; }




	void SetTarget(const Unit &city) { AccessData()->SetTarget(city) ; }
	void SetThirdParty(const PLAYER_INDEX player) { AccessData()->SetThirdParty(player) ; }

	void ToString(MBCHAR *s) { AccessData()->ToString(s) ; }

	void Break();
	BOOL IsBroken() const;

	void BeginTurnOwner();
	void BeginTurnRecipient();

	void Dump(const sint32 i) { AccessData()->Dump(i) ; }
};

#endif
