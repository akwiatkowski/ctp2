//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Matrix of all diplomatic agreements between players
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
// _DEBUG
// - Generate debug version when set.
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Input checks corrected, preventing a crash to desktop.
// - Made some sizes unsigned, to allow more than 28 players.
// - Standardised min/max usage.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ai/diplomacy/AgreementMatrix.h"

#include <vector>

#include "robot/aibackdoor/civarchive.h"
#include "gs/gameobj/Player.h"

#include "ai/diplomacy/diplomacyutil.h"
#include "DiplomacyProposalRecord.h"
#include "gs/utility/TurnCnt.h"
#include "ai/ctpai.h"
#include "gs/utility/MoveFlags.h"
#include "ai/diplomacy/Diplomat.h"
#include "gs/core/game_observer.h"

ai::Agreement       AgreementMatrix::s_badAgreement;
AgreementMatrix     AgreementMatrix::s_agreements;

AgreementMatrix::AgreementMatrix()
:
	m_agreements    (),
	m_maxPlayers    (0)
{
}

void AgreementMatrix::Resize(const PLAYER_INDEX & newMaxPlayers)
{
	// Just make sure that we can rely on testing against m_maxPlayers to have
	// a valid index in player_Get.
	Assert(newMaxPlayers <= k_MAX_PLAYERS);
	m_maxPlayers = std::min<sint16>(static_cast<sint16>(newMaxPlayers), k_MAX_PLAYERS);

	AgreementVector	old_agreements;
	m_agreements.swap(old_agreements);

	if (m_maxPlayers > 0)
    {
        m_agreements.resize(m_maxPlayers * m_maxPlayers * PROPOSAL_MAX, s_badAgreement);

		for (auto & old_agreement : old_agreements)
		{
			PLAYER_INDEX const	senderId	= old_agreement.senderId;
			PLAYER_INDEX const	receiverId	= old_agreement.receiverId;

			if (senderId >= 0 && senderId < m_maxPlayers &&
				receiverId >= 0 && receiverId < m_maxPlayers &&
				player_Get(senderId) && !player_Get(senderId)->IsDead() &&
				player_Get(receiverId) && !player_Get(receiverId)->IsDead())
			{

				SetAgreement( old_agreement );
			}
		}
	}
}



const ai::Agreement & AgreementMatrix::GetAgreement( const PLAYER_INDEX sender_player,
													 const PLAYER_INDEX receiver_player,
													 const PROPOSAL_TYPE type ) const
{
	size_t const index = AgreementIndex(sender_player, receiver_player, type);

#ifdef _DEBUG
	Assert(index < m_agreements.size());
	ai::Agreement agreement = m_agreements[ index ];
	if (agreement != AgreementMatrix::s_badAgreement)
	{
		if ((agreement.receiverId != receiver_player && agreement.receiverId != sender_player) ||
		    (agreement.senderId != sender_player && agreement.senderId != receiver_player))
		{

			return AgreementMatrix::s_badAgreement;
		}
	}
#endif _DEBUG

	if (index >= m_agreements.size())
		return AgreementMatrix::s_badAgreement;

	return m_agreements[ index ];
}

sint16 AgreementMatrix::GetAgreementDuration( const PLAYER_INDEX sender_player,
											  const PLAYER_INDEX receiver_player,
											  const PROPOSAL_TYPE type ) const
{
	const ai::Agreement & agreement = GetAgreement(sender_player, receiver_player, type);

	if (agreement.start == -1 || agreement.end != -1)
		return -1;

	return (turn_Get()->GetSessionRound() - agreement.start);
}

void AgreementMatrix::SetAgreement( const ai::Agreement & agreement )
{
	if (agreement == AgreementMatrix::s_badAgreement)
		return;

	size_t const firstAgreementIndex =
        AgreementIndex(agreement.senderId,
                       agreement.receiverId,
                       agreement.proposal.first_type
                      );

	Assert(firstAgreementIndex < m_agreements.size());
	m_agreements[firstAgreementIndex] = agreement;

#ifdef _DEBUG
	if (agreement != GetAgreement(agreement.senderId, agreement.receiverId, agreement.proposal.first_type))
	{
		Assert(0);
	}
#endif

	PROPOSAL_TYPE   reciprocalType    = agreement.proposal.first_type;
	sint32          reciprocalDBIndex;

	if (diplomacyutil_GetRecord(agreement.proposal.first_type)->GetReciprocalIndex(reciprocalDBIndex)) {
		reciprocalType = diplomacyutil_GetProposalType(reciprocalDBIndex);
	}

	size_t const    firstReciprocalIndex =
        AgreementIndex(agreement.receiverId,
                       agreement.senderId,
                       reciprocalType
                      );

	Assert(firstReciprocalIndex < m_agreements.size());
	m_agreements[firstReciprocalIndex] = agreement;

#ifdef _DEBUG
	if (agreement != GetAgreement(agreement.receiverId, agreement.senderId, reciprocalType))
	{
		Assert(0);
	}
#endif

	if (agreement.proposal.second_type != PROPOSAL_NONE)
	{
		size_t const secondAgreementIndex =
            AgreementIndex(agreement.senderId,
                           agreement.receiverId,
                           agreement.proposal.second_type
                           );

		Assert(secondAgreementIndex < m_agreements.size());
		m_agreements[secondAgreementIndex] = agreement;

#ifdef _DEBUG
		if (agreement != GetAgreement(agreement.senderId, agreement.receiverId, agreement.proposal.second_type))
		{
			Assert(0);
		}
#endif

		if(diplomacyutil_GetRecord(agreement.proposal.second_type)->GetReciprocalIndex(reciprocalDBIndex)) {
			reciprocalType = diplomacyutil_GetProposalType(reciprocalDBIndex);
		}
		else
		{
			reciprocalType = agreement.proposal.second_type;
		}

		size_t const secondReciprocalIndex =
            AgreementIndex(agreement.receiverId,
                           agreement.senderId,
                           reciprocalType
                          );

		Assert(secondReciprocalIndex < m_agreements.size());
		m_agreements[secondReciprocalIndex] = agreement;

#ifdef _DEBUG
		if (agreement != GetAgreement(agreement.receiverId, agreement.senderId, reciprocalType))
		{
			Assert(0);
		}
#endif

	}
	if (gameobservers_Get()) gameobservers_Get()->NotifyRadarMapUpdate(0);
}




//----------------------------------------------------------------------------
//
// Name       : AgreementMatrix::HasAgreement
//
// Description: Test whether 2 players have the indicated agreement.
//
// Parameters : sender_player	: initiator of the agreement
//				receiver_player	: responder to the agreement
//				type			: type of the agreement
//
// Globals    : player_Get		: all players in the game
//
// Returns    : -
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
bool AgreementMatrix::HasAgreement(const PLAYER_INDEX & sender_player,
								   const PLAYER_INDEX & receiver_player,
								   const PROPOSAL_TYPE type) const
{
	Assert(sender_player < m_maxPlayers);
	Assert(sender_player >= 0);

	if ((sender_player >= m_maxPlayers)  || (sender_player < 0))
	{
		return false;
	}

	Assert(receiver_player < m_maxPlayers);
	Assert(receiver_player >= 0);

	if ((receiver_player >= m_maxPlayers) || (receiver_player < 0))
	{
		return false;
	}

	Player *player_ptr = player_Get(sender_player);
	Assert(player_ptr != nullptr);

	if (player_ptr == nullptr)
		return false;

	sint32 round = turn_Get()->GetSessionRound();

	const ai::Agreement & agreement = GetAgreement(sender_player, receiver_player, type);

	if (agreement.start == -1)
		return false;

	if (agreement.end > round || agreement.end == -1)
		return true;

	return false;
}

bool AgreementMatrix::HasAgreement(const PLAYER_INDEX & sender_player,
								   const PROPOSAL_TYPE type) const
{
	for (PLAYER_INDEX foreignerId = 0; foreignerId < CtpAi::s_maxPlayers; foreignerId++)
	{
		if (HasAgreement(sender_player, foreignerId, type))
			return true;
	}
	return false;
}

void AgreementMatrix::CancelAgreement(const PLAYER_INDEX & sender_player,
									  const PLAYER_INDEX & receiver_player,
									  const PROPOSAL_TYPE type )
{
	ai::Agreement agreement =
		GetAgreement(sender_player, receiver_player, type);

	agreement.end = turn_Get()->GetSessionRound();

	SetAgreement(agreement);

	agreement =
		GetAgreement(receiver_player, sender_player, type);

	agreement.end = turn_Get()->GetSessionRound();

	SetAgreement(agreement);
}

void AgreementMatrix::BreakAgreements(const PLAYER_INDEX & sender_player, const PLAYER_INDEX & foreign_player)
{
	Diplomat & foreign_diplomat = Diplomat::GetDiplomat(foreign_player);

	if (HasAgreement(sender_player, foreign_player, PROPOSAL_TREATY_PEACE))
	{
		CancelAgreement(sender_player, foreign_player, PROPOSAL_TREATY_PEACE);
		foreign_diplomat.LogViolationEvent(sender_player, PROPOSAL_TREATY_PEACE);
	}

	if (HasAgreement(sender_player, foreign_player, PROPOSAL_TREATY_TRADE_PACT))
	{
		CancelAgreement(sender_player, foreign_player, PROPOSAL_TREATY_TRADE_PACT);
		foreign_diplomat.LogViolationEvent(sender_player, PROPOSAL_TREATY_TRADE_PACT);
	}

	if (HasAgreement(sender_player, foreign_player, PROPOSAL_TREATY_RESEARCH_PACT))
	{
		CancelAgreement(sender_player, foreign_player, PROPOSAL_TREATY_RESEARCH_PACT);
		foreign_diplomat.LogViolationEvent(sender_player, PROPOSAL_TREATY_RESEARCH_PACT);
	}

	if (HasAgreement(sender_player, foreign_player, PROPOSAL_TREATY_MILITARY_PACT))
	{
		CancelAgreement(sender_player, foreign_player, PROPOSAL_TREATY_MILITARY_PACT);
		foreign_diplomat.LogViolationEvent(sender_player, PROPOSAL_TREATY_MILITARY_PACT);
	}

	if (HasAgreement(sender_player, foreign_player, PROPOSAL_TREATY_ALLIANCE))
	{
		CancelAgreement(sender_player, foreign_player, PROPOSAL_TREATY_ALLIANCE);
		foreign_diplomat.LogViolationEvent(sender_player, PROPOSAL_TREATY_ALLIANCE);
	}
}

sint32 AgreementMatrix::TurnsSinceLastWar(const PLAYER_INDEX & player,
										  const PLAYER_INDEX & foreigner) const
{

	if (HasAgreement(player, foreigner, PROPOSAL_TREATY_DECLARE_WAR))
		return 0;

	const ai::Agreement & last_war =
		GetAgreement(player, foreigner, PROPOSAL_TREATY_DECLARE_WAR);

	if (last_war.start == -1)
		return -1;

	return (turn_Get()->GetSessionRound() - last_war.end);
}

sint32 AgreementMatrix::TurnsAtWar(const PLAYER_INDEX & player,
											const PLAYER_INDEX & foreigner) const
{

	if (HasAgreement(player, foreigner, PROPOSAL_TREATY_PEACE))
		return -1;

	if (HasAgreement(player, foreigner, PROPOSAL_TREATY_CEASEFIRE))
		return -1;

	const ai::Agreement & last_war =
		GetAgreement(player, foreigner, PROPOSAL_TREATY_DECLARE_WAR);

	if (!HasAgreement(player, foreigner, PROPOSAL_TREATY_DECLARE_WAR))
		return -1;

	if (last_war.start == -1)
		return -1;


	Assert(last_war.end == -1);

	return (turn_Get()->GetSessionRound() - last_war.start);
}

void AgreementMatrix::SetAgreementFast(size_t index, const ai::Agreement &agreement)
{
	Assert(index < m_agreements.size());
	if (index < m_agreements.size())
    {
		m_agreements[index] = agreement;
	}
}

void AgreementMatrix::ClearAgreementsInvolving(const PLAYER_INDEX playerId)
{
	for (auto & m_agreement : m_agreements)
	{
		if ((m_agreement.senderId == playerId) ||
			(m_agreement.receiverId == playerId))
			m_agreement = s_badAgreement;
	}
}

void AgreementMatrix::Cleanup()
{
	AgreementVector().swap(s_agreements.m_agreements);
}
