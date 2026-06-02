#include "ctp/c3.h"

#include "ai/diplomacy/DStateEvent.h"

#include "gs/gameobj/Events.h"
#include "gs/events/GameEventUser.h"
#include "gs/gameobj/Unit.h"
#include "gs/database/StrDB.h"
#include "gs/events/GameEventManager.h"

#include "gs/outcom/AICause.h"
#include "ai/diplomacy/Diplomat.h"
#include "ai/mapanalysis/mapanalysis.h"
#include "ai/diplomacy/AgreementMatrix.h"
#include "DiplomacyRecord.h"

#include "gs/gameobj/Player.h"







STDEHANDLER(InitDStateEvent)
{
	PLAYER_INDEX playerId;
	PLAYER_INDEX foreignerId;

	if (!args->GetPlayer(0, playerId))
		return GEV_HD_Continue;

	if (!args->GetPlayer(1, foreignerId))
		return GEV_HD_Continue;

	Diplomat & diplomat = Diplomat::GetDiplomat(playerId);
	AiState state;

	g_theDiplomacyDB->GetNamedItem("DIPLOMACY_DEFAULT",state.dbIndex);
	state.priority = 0;

	diplomat.SetDiplomaticState(foreignerId, state);

	return GEV_HD_Continue;
}







STDEHANDLER(NextDStateEvent)
{
	PLAYER_INDEX playerId;
	PLAYER_INDEX foreignerId;

	if (!args->GetPlayer(0, playerId))
		return GEV_HD_Continue;

	if (!args->GetPlayer(1, foreignerId))
		return GEV_HD_Continue;

	Diplomat & diplomat = Diplomat::GetDiplomat(playerId);

	diplomat.SetDiplomaticState(foreignerId, diplomat.GetBestDiplomaticState());
	return GEV_HD_Continue;
}








STDEHANDLER(Default_NextDStateEvent)
{
	PLAYER_INDEX playerId;
	PLAYER_INDEX foreignerId;

	if (!args->GetPlayer(0, playerId))
		return GEV_HD_Continue;

	if (!args->GetPlayer(1, foreignerId))
		return GEV_HD_Continue;

	AiState state;
	state.priority = 1;
	g_theDiplomacyDB->GetNamedItem("DIPLOMACY_DEFAULT",state.dbIndex);

	Diplomat & diplomat = Diplomat::GetDiplomat(playerId);
	diplomat.ConsiderDiplomaticState(foreignerId, state);

	return GEV_HD_Continue;
}


STDEHANDLER(ProvokeWar_NextDStateEvent)
{
	PLAYER_INDEX playerId;
	PLAYER_INDEX foreignerId;

	if (!args->GetPlayer(0, playerId))
		return GEV_HD_Continue;

	if (!args->GetPlayer(1, foreignerId))
		return GEV_HD_Continue;

	Player * foreigner_ptr = player_Get(foreignerId);
	Player * player_ptr = player_Get(playerId);

	if (foreigner_ptr == NULL || player_ptr == NULL)
		return GEV_HD_Continue;

	Diplomat & diplomat = Diplomat::GetDiplomat(playerId);

	if (!diplomat.DesireWarWith(foreignerId))
		return GEV_HD_Continue;

	const PersonalityRecord * personality = diplomat.GetPersonality();

	ai::Regard regard = diplomat.GetEffectiveRegard(foreignerId);

	if ( !personality->GetConquestAgressive() &&
		 (!personality->GetAlignmentEvil() || !personality->GetDiscoveryMilitary()) )
	{
		return GEV_HD_Continue;
	}




	if ( !personality->GetTrustworthinessChaotic() )
	{

		if ( regard >= COLDWAR_REGARD && personality->GetAlignmentGood())
		{
			return GEV_HD_Continue;
		}

		if ( regard >= NEUTRAL_REGARD && personality->GetAlignmentNeutral())
		{
			return GEV_HD_Continue;
		}

		DIPLOMATIC_STRENGTH player_strength =
			player_ptr->GetRelativeStrength(foreignerId);


		if (player_strength <= DIPLOMATIC_STRENGTH_VERY_WEAK)
			return GEV_HD_Continue;
	}

	AiState state;
	state.priority = 75;
	g_theDiplomacyDB->GetNamedItem("DIPLOMACY_PROVOKE_WAR",state.dbIndex);

	stringdb_Get()->GetStringID("SPY_PROVOKE_WAR_DS",state.spyStrId);
	stringdb_Get()->GetStringID("ADVICE_PROVOKE_WAR_DS",state.adviceStrId );
	stringdb_Get()->GetStringID("NEWS_PROVOKE_WAR_DS",state.newsStrId);

	diplomat.ConsiderDiplomaticState(foreignerId, state);

	return GEV_HD_Continue;
}


STDEHANDLER(MakeFriend_NextDStateEvent)
{
	PLAYER_INDEX playerId;
	PLAYER_INDEX foreignerId;

	if (!args->GetPlayer(0, playerId))
		return GEV_HD_Continue;

	if (!args->GetPlayer(1, foreignerId))
		return GEV_HD_Continue;

	Player * foreigner_ptr = player_Get(foreignerId);
	Player * player_ptr = player_Get(playerId);

	if (foreigner_ptr == NULL || player_ptr == NULL)
		return GEV_HD_Continue;

	Diplomat & diplomat = Diplomat::GetDiplomat(playerId);

	const PersonalityRecord * personality = diplomat.GetPersonality();

//	ai::Regard regard = diplomat.GetEffectiveRegard(foreignerId);

	bool personality_match = (personality->GetConquestPassive() &&
			personality->GetAlignmentGood() &&
			personality->GetTrustworthinessLawful());

	if (!personality_match && personality->GetConquestPassive())
	{
		personality_match = (personality->GetDiscoveryDiplomatic() ||
			 personality->GetDiscoveryEconomic());
	}


	if (!personality_match && personality->GetTrustworthinessChaotic())
	{
		DIPLOMATIC_STRENGTH player_strength =
			player_ptr->GetRelativeStrength(foreignerId);


		personality_match = (personality->GetAlignmentGood() &&
			(player_strength >= DIPLOMATIC_STRENGTH_VERY_STRONG));
	}

	if (!personality_match)
	{
		return GEV_HD_Continue;
	}

	AiState state;
	state.priority = 80;
	g_theDiplomacyDB->GetNamedItem("DIPLOMACY_MAKE_FRIEND",state.dbIndex);

	stringdb_Get()->GetStringID("SPY_PROVOKE_WAR_DS",state.spyStrId);
	stringdb_Get()->GetStringID("ADVICE_PROVOKE_WAR_DS",state.adviceStrId );
	stringdb_Get()->GetStringID("NEWS_PROVOKE_WAR_DS",state.newsStrId);

	diplomat.ConsiderDiplomaticState(foreignerId, state);

	return GEV_HD_Continue;
}

void DiplomaticStateEventCallbacks::AddCallbacks()
{
	gevmanager_Get()->AddCallback(GEV_InitDiplomaticState,
							  GEV_PRI_Primary,
							  &s_InitDStateEvent);

	gevmanager_Get()->AddCallback(GEV_NextDiplomaticState,
							  GEV_PRI_Primary,
							  &s_NextDStateEvent);

	gevmanager_Get()->AddCallback(GEV_NextDiplomaticState,
							  GEV_PRI_Pre,
							  &s_Default_NextDStateEvent);

	gevmanager_Get()->AddCallback(GEV_NextDiplomaticState,
							  GEV_PRI_Pre,
							  &s_ProvokeWar_NextDStateEvent);

	gevmanager_Get()->AddCallback(GEV_NextDiplomaticState,
							  GEV_PRI_Pre,
							  &s_MakeFriend_NextDStateEvent);
}
