#include "ctp/c3.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/Gold.h"
#include "gs/gameobj/Advances.h"
#include "gs/gameobj/AgreementData.h"
#include "gs/gameobj/Agreement.h"
#include "gs/gameobj/AgreementPool.h"
#include "gs/gameobj/Player.h"
#include "gs/gameobj/pollution.h"
#include "gs/outcom/AICause.h"
#include "gs/utility/AgreementDynArr.h"
#include "gs/utility/Globals.h"
#include "gs/utility/TurnCnt.h"
#include "gs/fileio/gamefile.h"

AgreementPool::AgreementPool() : ObjPool(k_BIT_GAME_OBJ_TYPE_AGREEMENT)
	{
	}









Agreement AgreementPool::Create(PLAYER_INDEX owner, PLAYER_INDEX recipient, AGREEMENT_TYPE request)
	{
	AgreementData* newData;
	Agreement newAgreement(NewKey(k_BIT_GAME_OBJ_TYPE_AGREEMENT));

	newData = new AgreementData(newAgreement, owner, recipient, request, turn_Get()->GetRound()) ;
	Insert(newData) ;

	if(player_Get(owner))
		player_Get(owner)->AddAgreement(newAgreement) ;

	if(player_Get(recipient))
		player_Get(recipient)->AddAgreement(newAgreement) ;

	return (newAgreement) ;
	}












void AgreementPool::EndRound()
	{
	sint32	i ;

	AgreementDynamicArray	expired ;

	for(i = 0; i < k_OBJ_POOL_TABLE_SIZE; i++)
		{
		if(m_table[i])
			{
			AgreementData	*agreeData = (AgreementData *)(m_table[i]) ;

			if (agreeData->IsExpired())
				{




				expired.Insert(m_table[i]->m_id) ;
				}

			agreeData->DecrementTurns() ;
			}

		}

    expired.KillList() ;
	}

AgreementData *AgreementPool::AccessAgreement(const Agreement id)
{
	return ((AgreementData*)Access((const ID)id)) ;
}

AgreementData *AgreementPool::GetAgreement(const Agreement id) const
{
	return ((AgreementData*)Get((const ID)id)) ;
}
