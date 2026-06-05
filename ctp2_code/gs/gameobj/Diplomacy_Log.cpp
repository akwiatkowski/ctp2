#include "ctp/c3.h"
#include "gs/gameobj/Diplomacy_Log.h"

#include "gs/world/MapPoint.h"
#include "gs/gameobj/ArmyData.h"
#include "gs/gameobj/Order.h"

#include "gs/core/player_view.h"


#include "gs/gameobj/Player.h"

#include "gs/gameobj/AgreementTypes.h"
#include "gs/gameobj/DiplomaticTypes.h"

Diplomacy_Log::Diplomacy_Log()
{
    strlcpy(m_filename, "logs\\diplomacy_log.txt", sizeof(m_filename));
    m_player_bit_mask = 0xffffffff;
    FILE *fout = fopen(m_filename, "w");
    Assert(fout);
    if (!fout) return;

    fprintf (fout, "diplomacy log\n");
    fclose (fout);

    m_regard_stack_idx=0;
}

Diplomacy_Log::~Diplomacy_Log()
= default;

void Diplomacy_Log::LogAllPlayers()
{
    m_player_bit_mask = 0xffffffff;

}

void Diplomacy_Log::UnlogAllPlayers()
{
    m_player_bit_mask = 0;
}

void Diplomacy_Log::LogPlayer(const sint32 player_idx)
{
    m_player_bit_mask |= (1 << player_idx);

}

void Diplomacy_Log::UnlogPlayer(const sint32 player_idx)
{
    m_player_bit_mask &= ~(1 << player_idx);
}

BOOL Diplomacy_Log::IsPlayerLogged(const sint32 player_idx)
{

    if (player_idx == -1) return FALSE;

    Assert(-1 <= player_idx);
    Assert(player_idx < k_MAX_PLAYERS);
    if ((player_idx < -1) || (k_MAX_PLAYERS <= player_idx)) return FALSE;

    return (m_player_bit_mask & (1 << player_idx)) != 0;
}

void Diplomacy_Log::BeginRound(sint32 currentRound)
{
    if (m_player_bit_mask == 0)  return;

    FILE *fout = fopen(m_filename, "a");
    Assert(fout);
    if (!fout) return;

    fprintf (fout, "***************************ROUND %d ***********************\n", currentRound);
    fclose(fout);
}

void Diplomacy_Log::BeginTurn(sint32 currentRound)
{
    sint32 player_idx = player_view::CurPlayer();

    if (IsPlayerLogged(player_idx)) {
        FILE *fout = fopen(m_filename, "a");
        Assert(fout);
        if (!fout) return;

        fprintf (fout, "----------------Begin Turn %d:%d--------------------\n", currentRound, player_idx);

        if (Player::IsThisPlayerARobot(player_idx)) {

            sint32 i;


            fprintf (fout, "Player %d regards---\n", player_idx);

            for (i=0; i<k_MAX_PLAYERS; i++) {
                if (i == player_idx )continue;
                if (player_Get(i) == nullptr) continue;
				if (player_Get(i)->m_isDead) continue;


            }

            fprintf(fout, "\n");
        }
        fclose(fout);
    }
}

void Diplomacy_Log::EndTurn(sint32 currentRound)
{

    sint32 player_idx = player_view::CurPlayer();
    if (IsPlayerLogged(player_idx)) {

        FILE *fout = fopen(m_filename, "a");
        Assert(fout);
        if (!fout) return;

        if (Player::IsThisPlayerARobot(player_idx)) {

            sint32 i;

            fprintf (fout, "Player %d regards---\n", player_idx);

            for (i=0; i<k_MAX_PLAYERS; i++) {
                if (i == player_idx )continue;
                if (player_Get(i) == nullptr) continue;
				if (player_Get(i)->m_isDead) continue;


            }

            fprintf(fout, "\n");


        }

        fprintf (fout, ".....................End Turn %d:%d...................\n", currentRound, player_idx);

        fclose(fout);
    }
}

void Diplomacy_Log::LogStr(const sint32 player_idx, char *str)
{
    if (!IsPlayerLogged(player_idx)) return;

    FILE *fout= fopen(m_filename, "a");
    Assert(fout);
    if (!fout) return;

    fprintf (fout, "%s", str);
    fclose (fout);
}

void Diplomacy_Log::LogHostileAction(const UNIT_ORDER_TYPE order_type, const sint32 target_player,
    const sint32 target_id,  const sint32 owner, const uint32 unit_id, const MapPoint &pos)
{
    if (!IsPlayerLogged(target_player) && !IsPlayerLogged(owner)) return;

    char order_str[80];
    switch(order_type) {
    	case UNIT_ORDER_INVESTIGATE_CITY: strlcpy(order_str, "UNIT_ORDER_INVESTIGATE_CITY", sizeof(order_str)); break;
        case UNIT_ORDER_NULLIFY_WALLS: strlcpy(order_str, "UNIT_ORDER_NULLIFY_WALLS", sizeof(order_str)); break;
        case UNIT_ORDER_STEAL_TECHNOLOGY:strlcpy(order_str, "UNIT_ORDER_STEAL_TECHNOLOGY", sizeof(order_str)); break;
        case UNIT_ORDER_ASSASSINATE: strlcpy(order_str, "UNIT_ORDER_ASSASSINATE", sizeof(order_str)); break;
        case UNIT_ORDER_INVESTIGATE_READINESS:strlcpy(order_str, "UNIT_ORDER_INVESTIGATE_READINESS", sizeof(order_str)); break;
        case UNIT_ORDER_FRANCHISE: strlcpy(order_str, "UNIT_ORDER_FRANCHISE", sizeof(order_str)); break;
        case UNIT_ORDER_SUE_FRANCHISE: strlcpy(order_str, "UNIT_ORDER_SUE_FRANCHISE", sizeof(order_str)); break;
        case UNIT_ORDER_CAUSE_UNHAPPINESS: strlcpy(order_str, "UNIT_ORDER_CAUSE_UNHAPPINESS", sizeof(order_str)); break;
        case UNIT_ORDER_PLANT_NUKE: strlcpy(order_str, "UNIT_ORDER_PLANT_NUKE", sizeof(order_str)); break;
        case UNIT_ORDER_SLAVE_RAID: strlcpy(order_str, "UNIT_ORDER_SLAVE_RAID", sizeof(order_str)); break;
        case UNIT_ORDER_UNDERGROUND_RAILWAY: strlcpy(order_str, "case UNIT_ORDER_UNDERGROUND_RAILWAY", sizeof(order_str)); break;
        case UNIT_ORDER_INCITE_UPRISING: strlcpy(order_str, "UNIT_ORDER_INCITE_UPRISING", sizeof(order_str)); break;
        case UNIT_ORDER_BIO_INFECT: strlcpy(order_str, "UNIT_ORDER_BIO_INFECT", sizeof(order_str)); break;
        case UNIT_ORDER_NANO_INFECT: strlcpy(order_str, "UNIT_ORDER_NANO_INFECT", sizeof(order_str)); break;
        case UNIT_ORDER_CONVERT: strlcpy(order_str, "UNIT_ORDER_CONVERT", sizeof(order_str)); break;
        case UNIT_ORDER_INDULGENCE: strlcpy(order_str, "UNIT_ORDER_INDULGENCE", sizeof(order_str)); break;
        case UNIT_ORDER_SOOTHSAY: strlcpy(order_str, "UNIT_ORDER_SOOTHSAY", sizeof(order_str)); break;
        case UNIT_ORDER_INJOIN: strlcpy(order_str, "UNIT_ORDER_INJOIN", sizeof(order_str)); break;
        case UNIT_ORDER_INCITE_REVOLUTION: strlcpy(order_str, "UNIT_ORDER_INCITE_REVOLUTION", sizeof(order_str)); break;
        case UNIT_ORDER_CREATE_PARK: strlcpy(order_str, "UNIT_ORDER_CREATE_PARK", sizeof(order_str)); break;
        case UNIT_ORDER_BOMBARD: strlcpy(order_str, "UNIT_ORDER_BOMBARD", sizeof(order_str)); break;
        case UNIT_ORDER_EXPEL: strlcpy(order_str, "UNIT_ORDER_EXPEL", sizeof(order_str)); break;
        case UNIT_ORDER_ENSLAVE_SETTLER: strlcpy(order_str, "UNIT_ORDER_ENSLAVE_SETTLER", sizeof(order_str)); break;
        case UNIT_ORDER_FINISH_ATTACK: strlcpy(order_str, "UNIT_ORDER_FINISH_ATTACK", sizeof(order_str)); break;
        case UNIT_ORDER_SUE: strlcpy(order_str, "UNIT_ORDER_SUE", sizeof(order_str)); break;
        case UNIT_ORDER_PILLAGE: strlcpy(order_str, "UNIT_ORDER_PILLAGE", sizeof(order_str)); break;
        case UNIT_ORDER_INTERCEPT_TRADE: strlcpy(order_str, "UNIT_ORDER_INTERCEPT_TRADE", sizeof(order_str)); break;
        case UNIT_ORDER_INFORM_AI_CAPTURE_CITY: strlcpy(order_str, "UNIT_ORDER_INFORM_AI_CAPTURE_CITY", sizeof(order_str)); break;
        default:
            { BOOL unknown_order_type=0;
                Assert(unknown_order_type);
            }
            break;
    }

    FILE *fout = fopen(m_filename, "a");
    Assert(fout);
    if (!fout) return;

    fprintf (fout, "player %d did %s to player %d at %d %d\n",
        owner, order_str, target_player, pos.x, pos.y);

    fclose (fout);
}

void Diplomacy_Log::LogRegard(const sint32 the_regarder, const sint32 the_regardee)
{
     if (!IsPlayerLogged(the_regarder) && !IsPlayerLogged(the_regardee)) return;

     if (!Player::IsThisPlayerARobot(the_regarder)) return;

     FILE *fout = fopen(m_filename, "a");
     Assert(fout);
     if (!fout) return;






     fclose (fout);
}

void Diplomacy_Log::LogMakeAgreement(const sint32 owner, const sint32 recipient,
     const sint32 thirdParty, const AGREEMENT_TYPE agreement)
{

    if (!IsPlayerLogged(owner) && !IsPlayerLogged(recipient) && !IsPlayerLogged(thirdParty)) return;

    FILE *fout = fopen(m_filename, "a");
    Assert(fout);
    if (!fout) return;

    char astr[80];
    switch (agreement) {
    case AGREEMENT_TYPE_DEMAND_ADVANCE: strlcpy(astr, "AGREEMENT_TYPE_DEMAND_ADVANCE", sizeof(astr)); break;
	case AGREEMENT_TYPE_DEMAND_CITY: strlcpy(astr, "AGREEMENT_TYPE_DEMAND_CITY", sizeof(astr));break;
	case AGREEMENT_TYPE_DEMAND_MAP: strlcpy(astr, "AGREEMENT_TYPE_DEMAND_MAP", sizeof(astr));break;
	case AGREEMENT_TYPE_DEMAND_GOLD: strlcpy(astr, "AGREEMENT_TYPE_DEMAND_GOLD", sizeof(astr));break;
	case AGREEMENT_TYPE_DEMAND_STOP_TRADE: strlcpy(astr, "AGREEMENT_TYPE_DEMAND_STOP_TRADE", sizeof(astr));break;
	case AGREEMENT_TYPE_DEMAND_LEAVE_OUR_LANDS: strlcpy(astr, "AGREEMENT_TYPE_DEMAND_LEAVE_OUR_LANDS", sizeof(astr)); break;
    case AGREEMENT_TYPE_REDUCE_POLLUTION: strlcpy(astr, "AGREEMENT_TYPE_REDUCE_POLLUTION", sizeof(astr)); break;
	case AGREEMENT_TYPE_CEASE_FIRE: strlcpy(astr, "AGREEMENT_TYPE_CEASE_FIRE", sizeof(astr));break;
    case AGREEMENT_TYPE_PACT_CAPTURE_CITY: strlcpy(astr, "AGREEMENT_TYPE_PACT_CAPTURE_CITY", sizeof(astr));break;
    case AGREEMENT_TYPE_PACT_END_POLLUTION: strlcpy(astr, "AGREEMENT_TYPE_PACT_END_POLLUTION", sizeof(astr));break;
    case AGREEMENT_TYPE_DEMAND_ATTACK_ENEMY: strlcpy(astr, "AGREEMENT_TYPE_DEMAND_ATTACK_ENEMY", sizeof(astr));break;
	case AGREEMENT_TYPE_OFFER_ADVANCE: strlcpy(astr, "AGREEMENT_TYPE_OFFER_ADVANCE", sizeof(astr));break;
	case AGREEMENT_TYPE_OFFER_CITY: strlcpy(astr, "AGREEMENT_TYPE_OFFER_CITY", sizeof(astr));break;
	case AGREEMENT_TYPE_OFFER_MAP: strlcpy(astr, "AGREEMENT_TYPE_OFFER_MAP", sizeof(astr));break;
	case AGREEMENT_TYPE_OFFER_GOLD: strlcpy(astr, "AGREEMENT_TYPE_OFFER_GOLD", sizeof(astr));break;
	case AGREEMENT_TYPE_EXCHANGE_ADVANCE: strlcpy(astr, "AGREEMENT_TYPE_EXCHANGE_ADVANCE", sizeof(astr));break;
	case AGREEMENT_TYPE_EXCHANGE_CITY: strlcpy(astr, "AGREEMENT_TYPE_EXCHANGE_CITY", sizeof(astr));break;
	case AGREEMENT_TYPE_EXCHANGE_MAP: strlcpy(astr, "AGREEMENT_TYPE_EXCHANGE_MAP", sizeof(astr));break;
	case AGREEMENT_TYPE_NO_PIRACY: strlcpy(astr, "AGREEMENT_TYPE_NO_PIRACY", sizeof(astr)); break;
    default :
        {
        sint32 unknown_agreement_type=0;
        Assert(unknown_agreement_type);
        }
        break;
    }

    if (-1 == thirdParty) {
        fprintf (fout, "player %d makes agreement with %d that %s\n", owner, recipient, astr);
    } else {
        fprintf (fout, "player %d makes agreement with %d that %s about\n", owner, recipient, astr);
    }

    fclose(fout);
}

void Diplomacy_Log::LogBrokenAgreement(const sint32 owner, const sint32 recipient,
     const sint32 thirdParty, const AGREEMENT_TYPE agreement)
{
    if (!IsPlayerLogged(owner) && !IsPlayerLogged(recipient) && !IsPlayerLogged(thirdParty)) return;

    FILE *fout = fopen(m_filename, "a");
    Assert(fout);
    if (!fout) return;

    char astr[80];
    switch (agreement) {
    case AGREEMENT_TYPE_DEMAND_ADVANCE: strlcpy(astr, "AGREEMENT_TYPE_DEMAND_ADVANCE", sizeof(astr)); break;
	case AGREEMENT_TYPE_DEMAND_CITY: strlcpy(astr, "AGREEMENT_TYPE_DEMAND_CITY", sizeof(astr));break;
	case AGREEMENT_TYPE_DEMAND_MAP: strlcpy(astr, "AGREEMENT_TYPE_DEMAND_MAP", sizeof(astr));break;
	case AGREEMENT_TYPE_DEMAND_GOLD: strlcpy(astr, "AGREEMENT_TYPE_DEMAND_GOLD", sizeof(astr));break;
	case AGREEMENT_TYPE_DEMAND_STOP_TRADE: strlcpy(astr, "AGREEMENT_TYPE_DEMAND_STOP_TRADE", sizeof(astr));break;
	case AGREEMENT_TYPE_DEMAND_LEAVE_OUR_LANDS: strlcpy(astr, "AGREEMENT_TYPE_DEMAND_LEAVE_OUR_LANDS", sizeof(astr)); break;
    case AGREEMENT_TYPE_REDUCE_POLLUTION: strlcpy(astr, "AGREEMENT_TYPE_REDUCE_POLLUTION", sizeof(astr)); break;
	case AGREEMENT_TYPE_CEASE_FIRE: strlcpy(astr, "AGREEMENT_TYPE_CEASE_FIRE", sizeof(astr));break;
    case AGREEMENT_TYPE_PACT_CAPTURE_CITY: strlcpy(astr, "AGREEMENT_TYPE_PACT_CAPTURE_CITY", sizeof(astr));break;
    case AGREEMENT_TYPE_PACT_END_POLLUTION: strlcpy(astr, "AGREEMENT_TYPE_PACT_END_POLLUTION", sizeof(astr));break;
    case AGREEMENT_TYPE_DEMAND_ATTACK_ENEMY: strlcpy(astr, "AGREEMENT_TYPE_DEMAND_ATTACK_ENEMY", sizeof(astr));break;
	case AGREEMENT_TYPE_OFFER_ADVANCE: strlcpy(astr, "AGREEMENT_TYPE_OFFER_ADVANCE", sizeof(astr));break;
	case AGREEMENT_TYPE_OFFER_CITY: strlcpy(astr, "AGREEMENT_TYPE_OFFER_CITY", sizeof(astr));break;
	case AGREEMENT_TYPE_OFFER_MAP: strlcpy(astr, "AGREEMENT_TYPE_OFFER_MAP", sizeof(astr));break;
	case AGREEMENT_TYPE_OFFER_GOLD: strlcpy(astr, "AGREEMENT_TYPE_OFFER_GOLD", sizeof(astr));break;
	case AGREEMENT_TYPE_EXCHANGE_ADVANCE: strlcpy(astr, "AGREEMENT_TYPE_EXCHANGE_ADVANCE", sizeof(astr));break;
	case AGREEMENT_TYPE_EXCHANGE_CITY: strlcpy(astr, "AGREEMENT_TYPE_EXCHANGE_CITY", sizeof(astr));break;
	case AGREEMENT_TYPE_EXCHANGE_MAP: strlcpy(astr, "AGREEMENT_TYPE_EXCHANGE_MAP", sizeof(astr));break;
	case AGREEMENT_TYPE_NO_PIRACY: strlcpy(astr, "AGREEMENT_TYPE_NO_PIRACY", sizeof(astr)); break;
    default :
        {
        sint32 unknown_agreement_type=0;
        Assert(unknown_agreement_type);
        }
        break;
    }

    if (-1 == thirdParty) {
        fprintf (fout, "sombody? has broken player %d agreement with %d that %s\n", owner, recipient, astr);
    } else {
        fprintf (fout, "sombody? has broken player %d agreement with %d that %s about\n", owner, recipient, astr);
    }

    fclose(fout);
}

void Diplomacy_Log::Request2String(REQUEST_TYPE request, char astr[80])
{
    switch (request) {
    // TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
    case REQUEST_TYPE_GREETING: strcpy (astr, "REQUEST_TYPE_GREETING"); break;
	// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
	case REQUEST_TYPE_DEMAND_ADVANCE: strcpy (astr, "REQUEST_TYPE_DEMAND_ADVANCE"); break;
	// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
	case REQUEST_TYPE_DEMAND_CITY: strcpy (astr, "REQUEST_TYPE_DEMAND_CITY"); break;
	// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
	case REQUEST_TYPE_DEMAND_MAP: strcpy (astr, "REQUEST_TYPE_DEMAND_MAP"); break;
	// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
	case REQUEST_TYPE_DEMAND_GOLD: strcpy (astr, "REQUEST_TYPE_DEMAND_GOLD"); break;
	// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
	case REQUEST_TYPE_DEMAND_STOP_TRADE: strcpy (astr, "REQUEST_TYPE_DEMAND_STOP_TRADE"); break;
	// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
	case REQUEST_TYPE_DEMAND_ATTACK_ENEMY: strcpy (astr, "REQUEST_TYPE_DEMAND_ATTACK_ENEMY"); break;
	// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
	case REQUEST_TYPE_DEMAND_LEAVE_OUR_LANDS: strcpy (astr, "REQUEST_TYPE_DEMAND_LEAVE_OUR_LANDS"); break;
	// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
	case REQUEST_TYPE_DEMAND_REDUCE_POLLUTION: strcpy (astr, "REQUEST_TYPE_DEMAND_REDUCE_POLLUTION"); break;
	// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
	case REQUEST_TYPE_OFFER_ADVANCE: strcpy (astr, "REQUEST_TYPE_OFFER_ADVANCE"); break;
	// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
	case REQUEST_TYPE_OFFER_CITY: strcpy (astr, "REQUEST_TYPE_OFFER_CITY"); break;
	// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
	case REQUEST_TYPE_OFFER_MAP: strcpy (astr, "REQUEST_TYPE_OFFER_MAP"); break;
	// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
	case REQUEST_TYPE_OFFER_GOLD: strcpy (astr, "REQUEST_TYPE_OFFER_GOLD"); break;
	// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
	case REQUEST_TYPE_OFFER_CEASE_FIRE: strcpy (astr, "REQUEST_TYPE_OFFER_CEASE_FIRE"); break;
	// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
	case REQUEST_TYPE_OFFER_PERMANENT_ALLIANCE: strcpy (astr, "REQUEST_TYPE_OFFER_PERMANENT_ALLIANCE"); break;
	// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
	case REQUEST_TYPE_OFFER_PACT_CAPTURE_CITY: strcpy (astr, "REQUEST_TYPE_OFFER_PACT_CAPTURE_CITY"); break;
    // TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
    case REQUEST_TYPE_OFFER_PACT_END_POLLUTION: strcpy (astr, "REQUEST_TYPE_OFFER_PACT_END_POLLUTION"); break;
	// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
	case REQUEST_TYPE_EXCHANGE_ADVANCE: strcpy (astr, "REQUEST_TYPE_EXCHANGE_ADVANCE"); break;
    // TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
    case REQUEST_TYPE_EXCHANGE_CITY: strcpy (astr, "REQUEST_TYPE_EXCHANGE_CITY"); break;
    // TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
    case REQUEST_TYPE_EXCHANGE_MAP: strcpy (astr, "REQUEST_TYPE_EXCHANGE_MAP"); break;
    // TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
    case REQUEST_TYPE_DEMAND_NO_PIRACY: strcpy (astr, "REQUEST_TYPE_DEMAND_NO_PIRACY"); break;
    default:
        {
            sint32 unknown_request=0;
            Assert(unknown_request);
        }
    }
}

void Diplomacy_Log::LogRequestCreated(const sint32 owner, const sint32 recipient,
    const REQUEST_TYPE request)
{
    if (!IsPlayerLogged(owner) && !IsPlayerLogged(recipient)) return;

    FILE *fout = fopen(m_filename, "a");
    Assert(fout);
    if (!fout) return;

    char astr[80];
    Request2String(request, astr);

    fprintf (fout, "player %d requests that player %d that %s\n", owner, recipient, astr);

    fclose (fout);
}

void Diplomacy_Log::LogTone(const sint32 sender, const sint32 reciever, const ATTITUDE_TYPE attitude)
{

    if (!IsPlayerLogged(sender) && !IsPlayerLogged(reciever)) return;

    char astr[80];

    switch (attitude) {
    case ATTITUDE_TYPE_STRONG_HOSTILE: strlcpy(astr, "ATTITUDE_TYPE_STRONG_HOSTILE", sizeof(astr)); break;
	case ATTITUDE_TYPE_WEAK_HOSTILE:  strlcpy(astr, "ATTITUDE_TYPE_WEAK_HOSTILE", sizeof(astr)); break;
	case ATTITUDE_TYPE_NEUTRAL:  strlcpy(astr, "ATTITUDE_TYPE_NEUTRAL", sizeof(astr)); break;
	case ATTITUDE_TYPE_WEAK_FRIENDLY:  strlcpy(astr, "ATTITUDE_TYPE_WEAK_FRIENDLY", sizeof(astr)); break;
	case ATTITUDE_TYPE_STRONG_FRIENDLY:  strlcpy(astr, "ATTITUDE_TYPE_STRONG_FRIENDLY", sizeof(astr)); break;
    default:
        {
            sint32 unknown_attitude=0;
            Assert(unknown_attitude);
        }
    }

    FILE *fout = fopen(m_filename, "a");
    Assert(fout);
    if (!fout) return;

    fprintf (fout, "player's %d tone to player %d is %s\n", sender, reciever, astr);

    fclose (fout);

}

void Diplomacy_Log::LogEnact(const sint32 sender, const sint32 reciever, const REQUEST_TYPE request)
{

    if (!IsPlayerLogged(sender) && !IsPlayerLogged(reciever)) return;

    char astr[80];

    FILE *fout = fopen(m_filename, "a");
    Assert(fout);
    if (!fout) return;

    Request2String(request, astr);

    fprintf (fout, "player's %d accepts player %d request of %s\n", reciever, sender, astr);

    fclose (fout);
}

void Diplomacy_Log::LogReject(const sint32 sender, const sint32 reciever, const REQUEST_TYPE request)
{

    if (!IsPlayerLogged(sender) && !IsPlayerLogged(reciever)) return;

    char astr[80];

    FILE *fout = fopen(m_filename, "a");
    Assert(fout);
    if (!fout) return;

    Request2String(request, astr);

    fprintf (fout, "player's %d rejects player %d request of %s\n", reciever, sender, astr);

    fclose (fout);

}

void Diplomacy_Log::PushRegardReqest(const sint32 regarder, const sint32 regardee)
{


    if (k_REGARD_STACK_MAX <= m_regard_stack_idx)
        return;

    m_regard_stack[m_regard_stack_idx][0] = regarder;
    m_regard_stack[m_regard_stack_idx][1] = regardee;
    m_regard_stack_idx++;
}

void Diplomacy_Log::PopRegardRequest()
{

    if (m_regard_stack_idx <= 0)
        return;

    m_regard_stack_idx--;
    LogRegard(m_regard_stack[m_regard_stack_idx][0],
        m_regard_stack[m_regard_stack_idx][1]);

}
