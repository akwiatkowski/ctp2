#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __AGREEMENTPOOL_H__
#define __AGREEMENTPOOL_H__

class AgreementPool;

#include "gs/gameobj/ObjPool.h"
#include <nlohmann/json.hpp>

class AgreementData;
class Agreement;

#include "gs/gameobj/AgreementTypes.h"

class AgreementPool : public ObjPool
{
public:
	AgreementData* AccessAgreement(const Agreement id);
	AgreementData* GetAgreement(const Agreement id) const;

	AgreementPool();

	Agreement Create(PLAYER_INDEX owner, PLAYER_INDEX recipient, AGREEMENT_TYPE request);
	void EndRound();

	friend void to_json(nlohmann::json &j, AgreementPool const &p);
	friend void from_json(nlohmann::json const &j, AgreementPool &p);
};

// Lifecycle in gs/utility/gameinit.cpp; backing static there.  External
// readers go through agreementpool_Get().
AgreementPool * agreementpool_Get();
void            agreementpool_Set(AgreementPool *p);

#endif
