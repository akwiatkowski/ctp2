//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  :
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
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Added check for clear queue actions from clients received after they lost
//   the city to another player (bug #26)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "net/general/net_action.h"

#include "gs/world/Cell.h"

#include "net/general/network.h"
#include "net/io/net_util.h"
#include "net/general/net_info.h"
#include "net/general/net_rand.h"
#include "net/general/net_terrain.h"
#include "net/general/net_traderoute.h"
#include "net/general/net_unit.h"
#include "gs/world/World.h"                      // world_Get()
#include "gs/gameobj/Player.h"                     // player_arr_Get()
#include "ui/aui_ctp2/SelItem.h"                    // selitem_Get()
#include "gs/gameobj/TradeOffer.h"
#include "gs/gameobj/Readiness.h"
#include "gs/gameobj/installation.h"
#include "gs/gameobj/TerrImprove.h"
#include "gs/gameobj/installationpool.h"
#include "gs/outcom/AICause.h"
#include "gs/gameobj/DiplomaticRequest.h"
#include "gs/gameobj/DiplomaticRequestPool.h"
#include "gs/gameobj/message.h"
#include "gs/gameobj/MessagePool.h"
#include "gs/gameobj/UnitData.h"
#include "gs/gameobj/citydata.h"
#include "gs/utility/TurnCnt.h"                    // g_turn
#include "gs/outcom/AICause.h"
#include "gs/gameobj/Advances.h"
#include "gs/gameobj/MaterialPool.h"
#include "gs/gameobj/TerrImprovePool.h"
#include "net/general/net_playerdata.h"
#include "gs/gameobj/UnitPool.h"                   // unitpool_Get()
#include "gs/gameobj/Order.h"
#include "gs/gameobj/ArmyPool.h"
#include "gfx/tilesys/tiledmap.h"                   // tiledmap_Get()
#include "ui/aui_ctp2/radarmap.h"                   // radar_map_Get()
#include "gs/gameobj/ArmyData.h"
#include "gs/gameobj/TradeOfferPool.h"             // tradeofferpool_Get()
#include "gs/gameobj/Agreement.h"
#include "gs/gameobj/AgreementPool.h"              // agreementpool_Get()
#include "AdvanceRecord.h"
#include "gs/gameobj/TradePool.h"
#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicObject.h"
#include "gs/utility/newturncount.h"
#include "gs/events/GameEventManager.h"
#include "gfx/spritesys/director.h"
#include "ai/diplomacy/Diplomat.h"
#include "ui/interface/battleviewwindow.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_common/aui_button.h"
#include "gs/utility/gstypes.h"                    // TERRAIN_TYPES


void battleview_ExitButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie );

const uint32 NetAction::m_args[NET_ACTION_NULL] = {
	2,
	0,
	1,
	2,
	3,
	2,
	7,
	1,
	7,
	1,

	1,
	1,
	2,
	5,
	4,
	1,
	1,
	1,
	1,
	1,

	1,
	1,
	2,
	1,
	4,
	2,
	2,
	1,
	1,
	1,

	3,
	4,
	4,
	2,
	1,
	4,
	1,
	4,
	4,
	3,

	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,

	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,

	4,
	1,
	1,
	4,
	4,
	4,
	1,
	1,
	1,
	1,

	1,
	2,
	2,
	2,
	2,
	2,
	2,
	2,
	2,
	2,

	2,
	1,
	1,
	1,
	4,
	3,
	1,
	2,
	1,
	0,

	3,
	4,
	1,
	1,
	1,
	4,
	3,
	0,
	1,
	1,

	1,
	1,
	4,
	2,
	1,
	2,
	4,
	1,
	0,
	1,

	4,
	3,
	2,
	2,
	1,
	3,
	1,
	1,
	2,
	5,

	5,
	5,
	1,
	2,
	0,
	4,
	1,
	1,
	1,
	2,

	1,
	3,
	4,
	1,
	2,
	3,
	1,
	5,
	1,
	0,

	3,
	2,
	1,
	1,
	1,
	1,
	1,
	2,
	1,
	1,

	1,
	1,
	1,
	3,
	3,
	0,
	1,
	3,
	1,
};

NetAction::NetAction(NET_ACTION action, ...)
:
	m_action    (action)
{
	va_list vl;
	uint32 i;
#ifdef _DEBUG
	char str[1024];
	snprintf(str, sizeof(str), "NetAction: action=%d ", m_action);
#endif
	if(m_action >= 0 && m_action < NET_ACTION_NULL && m_args[m_action] > 0) {
		va_start( vl, action );
		for(i = 0; i < m_args[m_action]; i++) {
			m_data[i] = va_arg( vl, uint32 );
#ifdef _DEBUG
			snprintf(str + strlen(str), sizeof(str) - strlen(str), "arg: %8d/0x%lx ", m_data[i], m_data[i]);
#endif
		}
		va_end(vl);
	}
#ifdef _DEBUG
	strcat(str, "\n");
	DPRINTF(k_DBG_NET, ("%s\n", str));
#endif

}

NetAction::NetAction()
:
    m_action    (NET_ACTION_NULL)
{
}

void NetAction::Packetize(uint8* buf, uint16& size)
{
	buf[0] = 'A';
	buf[1] = 'A';
	size = 2;

	PUSHSHORT(static_cast<uint16>(m_action));
	for(uint32 i = 0; i < m_args[m_action]; i++) {
		PUSHLONG(m_data[i]);
	}
}

void NetAction::Unpacketize(uint16 id, uint8* buf, uint16 size)
{
	Assert(buf[0] == 'A' && buf[1] == 'A');
	sint32 pos = 2;

	Assert(g_network.IsHost());

	PULLSHORTTYPE(m_action, NET_ACTION);
	if(m_action < 0 || m_action >= NET_ACTION_NULL) return;
	for(uint32 i = 0; i < m_args[m_action]; i++) {
		PULLLONG(m_data[i]);
	}

	sint32 index = g_network.IdToIndex(id);

	if(g_network.SentResync(index)) {


		return;
	}

	switch(m_action) {
		case NET_ACTION_MOVE_UNIT:
		{

			DPRINTF(k_DBG_NET, ("NetAction: Player %d moving army %d(%d units), dir: %d\n",
								index, m_data[0],
								player_Get(index)->GetAllArmiesList()->Access(m_data[0]).Num(),
								m_data[1]));
			break;
		}
		case NET_ACTION_END_TURN:
		{
			int curPlayerIndex = g_network.IdToIndex(id);

			if(curPlayerIndex != selitem_Get()->GetCurPlayer()) {
				if(turn_Get()->SimultaneousMode()) {
					if(player_Get(curPlayerIndex))
						player_Get(curPlayerIndex)->EndTurnSoon();
				}
				break;
			} else {
				if(g_network.ShouldAckBeginTurn()) {
					DPRINTF(k_DBG_NET, ("NET_ACTION_END_TURN, %d\n", curPlayerIndex));

					Assert(g_network.m_playerData[index]->m_createdArmies.Num() == 0);
					Assert(g_network.m_playerData[index]->m_createdUnits.Num() == 0);

					if(BattleViewWindow *bvw = battleviewwindow_Get(); bvw && c3ui_Get()->GetWindow(bvw->Id())) {
						battleview_ExitButtonActionCallback(nullptr, AUI_BUTTON_ACTION_EXECUTE, 0, nullptr);
					}

					director_Get()->AddEndTurn();








				} else {
					g_network.SetEndTurnWhenClear();
				}
			}
			break;
		}
		case NET_ACTION_SETTLE:
		{
			DPRINTF(k_DBG_NET, ("Server: settling unit %lx, client created city %lx\n", m_data[0], 0xdeadbeef));
			Unit unit(m_data[0]);
			if(!unit.IsValid()) {
				g_network.Resync(index);
				break;
			}

			if(selitem_Get()->GetCurPlayer() == index) {
				g_network.Bookmark(id);
				unit.Settle();












				g_network.Unfreeze(id);
			} else {
			}
			break;
		}
		case NET_ACTION_BUILD:
			DPRINTF(k_DBG_NET, ("Server: Building unit type %d at city %d\n", m_data[0], m_data[1]));
			if(player_Get(index)) {
				Unit unit = Unit(m_data[1]);
				player_Get(index)->BuildUnit(m_data[0], unit);
			}
			break;
		case NET_ACTION_TAX_RATES:
		{
			double s,g,l;
			s = (double)m_data[0] / 100000.;
			g = (double)m_data[1] / 100000.;
			l = (double)m_data[2] / 100000.;

			DPRINTF(k_DBG_NET, ("Server: Setting tax rates for player %d to %lf, %lf, %lf\n",
								index, s, g, l));
			if(player_Get(index))
				player_Get(index)->SetTaxes(s);
			break;
		}
		case NET_ACTION_BUILD_IMP:
			DPRINTF(k_DBG_NET, ("Server: Building improvement %d at city %d\n",
								m_data[0], m_data[1]));
			if(player_Get(index)) {
				Unit unit(m_data[1]);
				player_Get(index)->BuildImprovement(m_data[0], unit);
			}
			break;
		case NET_ACTION_CREATE_TRADE_ROUTE:
		{
			DPRINTF(k_DBG_NET, ("Server: Player %d TR: src: %d, ty: %d, rsrc: %d, dest: %d\n",
								index,
								m_data[0], m_data[1], m_data[2], m_data[3]));

			TradeRoute route;
			if(player_Get(index)) {
				g_network.Block(index);
				route =
					player_Get(index)->CreateTradeRoute(
					    Unit(m_data[0]),
						ROUTE_TYPE(m_data[1]),
						m_data[2],
						Unit(m_data[3]),
						m_data[5],
						m_data[6]);
				g_network.Unblock(index);
				Assert(route.IsValid());
			}

			if((uint32)route != m_data[4]) {
				g_network.QueuePacket(id, new NetInfo(NET_INFO_CODE_NAK_OBJECT,
													  m_data[4], (uint32)route));
				TradeRoute otherRoute(m_data[4]);
				if(tradepool_Get()->IsValid(otherRoute))
					g_network.QueuePacket(id, new NetTradeRoute(otherRoute.AccessData(), true));

				if (route.IsValid()) {
					g_network.QueuePacket(id, new NetTradeRoute(route.AccessData(), true));
				}
			} else {
				g_network.QueuePacket(id, new NetInfo(NET_INFO_CODE_ACK_OBJECT,
													  m_data[4]));
			}
			break;
		}
		case NET_ACTION_CANCEL_TRADE_ROUTE:
		{
			DPRINTF(k_DBG_NET, ("Server: Cancelling trade route %d\n",
								m_data[0]));
			TradeRoute route(m_data[0]);


			Assert((route.GetSource().GetOwner() == index) ||
				   (route.GetDestination().GetOwner() == index));
			if(player_Get(index))
				player_Get(index)->CancelTradeRoute(route);
			break;
		}

		case NET_ACTION_CREATE_TRADE_OFFER:
		{
			DPRINTF(k_DBG_NET, ("Server: new trade offer: pl=%d, city=%d, ofty=%d, ofrsrc=%d, askty=%d, askrsrc=%d\n",
								index,
								m_data[0], m_data[1], m_data[2],
								m_data[3], m_data[4]));
			if(player_Get(index)) {
				g_network.Bookmark(id);
				TradeOffer offer = player_Get(index)->CreateTradeOffer(
				    Unit(m_data[0]),
					ROUTE_TYPE(m_data[1]),
					m_data[2],
					ROUTE_TYPE(m_data[3]),
					m_data[4],
					Unit(m_data[5])
					);
				if((uint32)offer != m_data[6]) {
					g_network.QueuePacketBookmark(id, new NetInfo(NET_INFO_CODE_NAK_OBJECT,
																  m_data[6], (uint32)offer));
				} else {
					g_network.QueuePacketBookmark(id, new NetInfo(NET_INFO_CODE_ACK_OBJECT,
																  m_data[6]));
				}
				g_network.Unfreeze(id);
			}
			break;
		}
		case NET_ACTION_WITHDRAW_TRADE_OFFER:
		{
			DPRINTF(k_DBG_NET, ("Server: Withdrawing trade offer %d\n",
								m_data[0]));
			if(player_Get(index)) {
				if(tradeofferpool_Get()->IsValid(TradeOffer(m_data[0])))
				   player_Get(index)->WithdrawTradeOffer(TradeOffer(m_data[0]));
			}
			break;
		}
		case NET_ACTION_CREATED_UNIT:
		{

			DPRINTF(k_DBG_NET, ("Server: client %d created Unit %lx\n",
								index, m_data[0]));

			if(!g_network.m_playerData[index])
				break;

			PlayerData *pd = g_network.m_playerData[index];
			Assert(pd->m_createdUnits.Num() > 0);
			if(pd->m_createdUnits.Num() <= 0) {
				g_network.Resync(index);
				break;
			}


			if(pd->m_createdUnits[0] == Unit(m_data[0])) {
				g_network.QueuePacket(id,
									  new NetInfo(NET_INFO_CODE_ACK_OBJECT,
												  m_data[0]));
			} else {
				g_network.QueuePacket(id,
									  new NetInfo(NET_INFO_CODE_NAK_OBJECT,
												  m_data[0], (uint32)pd->m_createdUnits[0]));
			}

			pd->m_createdUnits.DelIndex(0);

			break;
		}
		case NET_ACTION_SET_READINESS:
		{
			DPRINTF(k_DBG_NET, ("Server: Player %d setting readiness to %d\n",
								index, m_data[0]));
			if(player_Get(index)) {
				player_Get(index)->SetReadinessLevel((READINESS_LEVEL)m_data[0], m_data[1] != 0);
			}

			break;
		}
		case NET_ACTION_TERRAIN_IMPROVEMENT:
		{

			DPRINTF(k_DBG_NET, ("Server: Player %d creating terrain improvement\n", index));
			MapPoint pnt(m_data[1], m_data[2]);
			if(player_Get(index)) {
				g_network.Block(index);
				TerrainImprovement imp = player_Get(index)->CreateImprovement(
				    (TERRAIN_IMPROVEMENT)m_data[0],
					pnt, (TERRAIN_TYPES)m_data[3]);
				g_network.Unblock(index);
				if((uint32)imp != m_data[4]) {
					TerrainImprovement oops(m_data[4]);
					g_network.QueuePacket(id, new NetInfo(
														  NET_INFO_CODE_NAK_OBJECT, m_data[4], (uint32)imp));


					if(terrimprovepool_Get()->IsValid(oops)) {
						g_network.QueuePacket(id,
											  new NetTerrainImprovement(oops.AccessData()));
					}

					if(terrimprovepool_Get()->IsValid(imp)) {

						g_network.QueuePacket(id,
											  new NetTerrainImprovement(imp.AccessData()));
					}
				} else {
					g_network.QueuePacket(id, new NetInfo(
														  NET_INFO_CODE_ACK_OBJECT, m_data[4]));
				}
			}
			break;
		}
		case NET_ACTION_CREATE_INSTALLATION:
		{

			DPRINTF(k_DBG_NET, ("Server: Player %d creating installation id %lx, type %d at %d,%d\n",
								index, m_data[3], m_data[0],
								m_data[1], m_data[2]));
			MapPoint pnt(m_data[1], m_data[2]);
			Installation inst(m_data[3]);
			MapPoint rpnt;
			if(!installationpool_Get()->IsValid(inst)) {
				g_network.QueuePacket(id, new NetInfo(
					NET_INFO_CODE_NAK_OBJECT, m_data[3], 0));
				return;
			}

			inst.GetPos(rpnt);
			if(inst.GetOwner() == index && rpnt == pnt && inst.GetType() == (sint32)m_data[0]) {
				g_network.QueuePacket(id, new NetInfo(
					NET_INFO_CODE_ACK_OBJECT, m_data[3]));
			} else {
				g_network.QueuePacket(id, new NetInfo(
					NET_INFO_CODE_NAK_OBJECT, m_data[3], (uint32)inst));
			}
			break;
		}
		case NET_ACTION_ENTRENCH:
		{

			DPRINTF(k_DBG_NET, ("Server: Player %d entrenching unit %lx\n", index, m_data[0]));
			if(player_Get(index))
				player_Get(index)->Entrench(m_data[0]);
			break;
		}
		case NET_ACTION_DETRENCH:
		{

			DPRINTF(k_DBG_NET, ("Server: Player %d detrenching unit %lx\n", index, m_data[0]));
			if(player_Get(index))
				player_Get(index)->Detrench(m_data[0]);
			break;
		}
		case NET_ACTION_SLEEP:
		{

			DPRINTF(k_DBG_NET, ("Server: Player %d sleeping unit %lx\n", index, m_data[0]));
			if(player_Get(index))
				player_Get(index)->Sleep(m_data[0]);
			break;
		}
		case NET_ACTION_WAKEUP:
		{

			DPRINTF(k_DBG_NET, ("Server: Player %d waking unit %lx\n", index, m_data[0]));
			if(player_Get(index))
				player_Get(index)->WakeUp(m_data[0]);
			break;
		}
		case NET_ACTION_SET_MATERIALS_TAX:
		{
			DPRINTF(k_DBG_NET, ("Server: Player %d setting materials tax to %d\n", index, m_data[0]));
			double tax = (double)(((double)m_data[0] + 0.0001)/100.0);
			if(tax > 1) tax = 1;
			if(player_Get(index))
				player_Get(index)->SetMaterialsTax(tax);
			break;
		}
		case NET_ACTION_FOLLOW_PATH:
		{
			DPRINTF(k_DBG_NET, ("Server: Player %d army %d executing path\n",
								index, m_data[0]));
			Assert(FALSE);
			break;
		}
		case NET_ACTION_FOLLOW_PATROL:
		{
			DPRINTF(k_DBG_NET, ("Server: Player %d army %d executing path\n",
								index, m_data[0]));
			Assert(FALSE);
			break;
		}
		case NET_ACTION_DEL_TAIL_ORDER:
		{
			DPRINTF(k_DBG_NET, ("Server: Player %d army %d deleting tail order type %d\n",
								index, m_data[1], m_data[0]));
			Assert(FALSE);
			break;
		}
		case NET_ACTION_POP_TO_CITY:
			DPRINTF(k_DBG_NET, ("Server: Player %d sending pop %lx to city\n",
								index, m_data[0]));
			break;
		case NET_ACTION_POP_TO_FIELD:
		{
			DPRINTF(k_DBG_NET, ("Server: Player %d putting pop %lx in field at %d,%d,%d\n",
								index, m_data[3], m_data[0], m_data[1], m_data[2]));
			break;
		}
		case NET_ACTION_BUILD_WONDER:
		{
			DPRINTF(k_DBG_NET, ("Server: Player %d building wonder %d in city %lx\n",
								index, m_data[1], m_data[0]));
			if(unitpool_Get()->IsValid(Unit(m_data[0])))
				Unit(m_data[0]).BuildWonder(m_data[1]);
			break;
		}
		case NET_ACTION_INTERCEPT_TRADE:
		{

			DPRINTF(k_DBG_NET, ("Server: Player %d intercepting trade with army %d\n",
								index, m_data[0]));
			if(player_Get(index))
				player_Get(index)->InterceptTrade(m_data[0]);
			break;
		}
		case NET_ACTION_WORKDAY_LEVEL:
			DPRINTF(k_DBG_NET, ("Player %d set workday to %d\n", index, m_data[0]));
			if(player_Get(index)) {
				player_Get(index)->SetWorkdayLevel(m_data[0]);
				g_network.Block(index);
				g_network.QueuePacketToAll(new NetInfo(NET_INFO_CODE_WORKDAY_LEVEL,
													   index, m_data[0]));
				g_network.Unblock(index);
			}
			break;
		case NET_ACTION_WAGES_LEVEL:
			DPRINTF(k_DBG_NET, ("Player %d set wages to %d\n", index, m_data[0]));
			if(player_Get(index)) {
				player_Get(index)->SetWagesLevel(m_data[0]);
				g_network.Block(index);
				g_network.QueuePacketToAll(new NetInfo(NET_INFO_CODE_WAGES_LEVEL,
													   index, m_data[0]));
				g_network.Unblock(index);
			}
			break;
		case NET_ACTION_RATIONS_LEVEL:
			DPRINTF(k_DBG_NET, ("Player %d set rations to %d\n", index, m_data[0]));
			if(player_Get(index)) {
				player_Get(index)->SetRationsLevel(m_data[0]);
				g_network.Block(index);
				g_network.QueuePacketToAll(new NetInfo(NET_INFO_CODE_RATIONS_LEVEL,
													   index, m_data[0]));
				g_network.Unblock(index);
			}
			break;
		case NET_ACTION_CREATED_CIV:
			Assert(player_Get(m_data[0]) != nullptr);
			if(player_Get(m_data[0]) != nullptr)
				break;

			if(player_Get(m_data[0])) {
				Assert(player_Get(m_data[0])->IsRobot());
			}
			break;
		case NET_ACTION_CREATE_DIP_REQUEST:
		{
			DPRINTF(k_DBG_NET, ("Server: Player %d created diplomatic request %lx to player %d, type %d\n",
								m_data[0], m_data[3], m_data[1], m_data[2]));
			g_network.Bookmark(id);
			DiplomaticRequest req = diplomaticrequestpool_Get()->Create(
							   m_data[0], m_data[1], REQUEST_TYPE(m_data[2]));
			if(req != DiplomaticRequest(m_data[3])) {
				g_network.QueuePacketBookmark(id, new NetInfo(
							  NET_INFO_CODE_NAK_OBJECT, m_data[3], (uint32)req));
			} else {
				g_network.QueuePacketBookmark(id, new NetInfo(
							  NET_INFO_CODE_ACK_OBJECT, m_data[3]));
			}
			g_network.Unfreeze(id);
			break;
		}

		case NET_ACTION_UNLOAD_TRANSPORT:
		{

			break;
		}
		case NET_ACTION_GROUP_ARMY:
		{
			Army army(m_data[0]);
			Unit unit(m_data[1]);
			DPRINTF(k_DBG_NET, ("Server: client grouping unit 0x%lx into army 0x%lx\n", unit.m_id, army.m_id));
			if(!army.IsValid() || !unit.IsValid()) {
				g_network.Resync(index);
				break;
			}
			army->GroupUnit(unit);
			break;
		}
		case NET_ACTION_UNGROUP_ARMY:
		{
			Assert(FALSE);
			break;
		}
		case NET_ACTION_PARADROP:
		{
			Assert(FALSE);
			break;
		}
		case NET_ACTION_SPACE_LAUNCH:
		{
			break;
		}
		case NET_ACTION_INVESTIGATE_CITY:
		{
			Assert(index == selitem_Get()->GetCurPlayer());
			if(index != selitem_Get()->GetCurPlayer()) {

				return;
			}

			break;
		}
		case NET_ACTION_NULLIFY_WALLS:
		{
			Assert(index == selitem_Get()->GetCurPlayer());
			if(index != selitem_Get()->GetCurPlayer()) {

				return;
			}
			break;
		}
		case NET_ACTION_STEAL_TECHNOLOGY:
		{
			Assert(index == selitem_Get()->GetCurPlayer());
			if(index != selitem_Get()->GetCurPlayer()) {

				return;
			}
			Unit u(m_data[0]);
			Unit city(m_data[1]);
			sint32 advance = sint32(m_data[2]);
			if(!unitpool_Get()->IsValid(u)) {
				g_network.Resync(index);
				break;
			}
			if(!unitpool_Get()->IsValid(city)) {
				g_network.Resync(index);
				break;
			}

			ORDER_RESULT res = u.StealTechnology(city, advance);
			if(res != ORDER_RESULT_ILLEGAL) {
				sint32 ordindex = orderinfo_MapAt(UNIT_ORDER_STEAL_TECHNOLOGY);
				OrderInfo const *oi = nullptr;
				if(ordindex >= 0 && ordindex < orderinfo_Num()) {
					oi = &orderinfo_Get(ordindex);
				}

				if(oi && oi->m_goldCost > 0) {
					player_Get(index)->m_gold->SubGold(oi->m_goldCost);
				}

				if(unitpool_Get()->IsValid(u)) {
					u.SetFlag(k_UDF_USED_SPECIAL_ACTION_THIS_TURN);
					if(oi && oi->m_moveCost > 0) {
						bool out_of_fuel;
						u.DeductMoveCost(oi->m_moveCost, out_of_fuel);
					}
					u.ClearFlag(k_UDF_FIRST_MOVE);
				}
			}
			break;
		}
		case NET_ACTION_INCITE_REVOLUTION:
		{
			Assert(index == selitem_Get()->GetCurPlayer());
			if(index != selitem_Get()->GetCurPlayer()) {

				return;
			}
			break;
		}
		case NET_ACTION_ASSASSINATE_RULER:
		{
			Assert(index == selitem_Get()->GetCurPlayer());
			if(index != selitem_Get()->GetCurPlayer()) {

				return;
			}

			break;
		}
		case NET_ACTION_INVESTIGATE_READINESS:
		{
			Assert(index == selitem_Get()->GetCurPlayer());
			if(index != selitem_Get()->GetCurPlayer()) {

				return;
			}

			break;
		}
		case NET_ACTION_BOMBARD:
		{
			Assert(FALSE);
			break;
		}
		case NET_ACTION_SUE:
		{
			Assert(FALSE);
			break;
		}
		case NET_ACTION_SUE_FRANCHISE:
		{
			Assert(FALSE);
			break;
		}
		case NET_ACTION_EXPEL:
		{
			Assert(FALSE);
			break;
		}
		case NET_ACTION_CAUSE_UNHAPPINESS:
		{
			Assert(FALSE);
			break;
		}
		case NET_ACTION_PLANT_NUKE:
		{
			Assert(index == selitem_Get()->GetCurPlayer());
			if(index != selitem_Get()->GetCurPlayer()) {

				return;
			}

			break;
		}
		case NET_ACTION_SLAVE_RAID:
		{
			Assert(index == selitem_Get()->GetCurPlayer());
			if(index != selitem_Get()->GetCurPlayer()) {

				return;
			}
			break;
		}
		case NET_ACTION_ENSLAVE_SETTLER:
		{
			Assert(FALSE);
			break;
		}
		case NET_ACTION_UNDERGROUND_RAILWAY:
		{
			Assert(index == selitem_Get()->GetCurPlayer());
			if(index != selitem_Get()->GetCurPlayer()) {

				return;
			}
			break;
		}
		case NET_ACTION_INCITE_UPRISING:
		{
			Assert(FALSE);
			break;
		}
		case NET_ACTION_BIO_INFECT:
		{
			Assert(FALSE);
			break;
		}
		case NET_ACTION_NANO_INFECT:
		{
			Assert(FALSE);
			break;
		}
		case NET_ACTION_CONVERT_CITY:
		{
			Assert(index == selitem_Get()->GetCurPlayer());
			if(index != selitem_Get()->GetCurPlayer()) {

				return;
			}
			break;
		}
		case NET_ACTION_REFORM_CITY:
		{
			Assert(index == selitem_Get()->GetCurPlayer());
			if(index != selitem_Get()->GetCurPlayer()) {

				return;
			}
			break;
		}
		case NET_ACTION_SOOTHSAY:
		{
			Assert(index == selitem_Get()->GetCurPlayer());
			if(index != selitem_Get()->GetCurPlayer()) {

				return;
			}
			break;
		}
		case NET_ACTION_DEFUSE_MINES:
		{
			Assert(index == selitem_Get()->GetCurPlayer());
			if(index != selitem_Get()->GetCurPlayer()) {

				return;
			}
			break;
		}
		case NET_ACTION_FRANCHISE:
		{
			Assert(index == selitem_Get()->GetCurPlayer());
			if(index != selitem_Get()->GetCurPlayer()) {

				return;
			}
			break;
		}

		case NET_ACTION_INDULGENCE_SALE:
		{
			Assert(index == selitem_Get()->GetCurPlayer());
			if(index != selitem_Get()->GetCurPlayer()) {

				return;
			}
			break;
		}
		case NET_ACTION_CLOAK:
		{
			Assert(FALSE);
			break;
		}
		case NET_ACTION_UNCLOAK:
		{
			Assert(FALSE);
			break;
		}
		case NET_ACTION_RUSTLE:
		{
			Assert(index == selitem_Get()->GetCurPlayer());
			if(index != selitem_Get()->GetCurPlayer()) {

				return;
			}
			break;
		}
		case NET_ACTION_CREATE_PARK:
		{
			Assert(index == selitem_Get()->GetCurPlayer());
			if(index != selitem_Get()->GetCurPlayer()) {

				return;
			}
			break;
		}
		case NET_ACTION_CREATE_RIFT:
		{
			Assert(index == selitem_Get()->GetCurPlayer());
			if(index != selitem_Get()->GetCurPlayer()) {

				return;
			}
			break;
		}

		case NET_ACTION_REJECT_MESSAGE:
		{
			Message message(m_data[0]);
			Assert(messagepool_Get()->IsValid(message));
			if(!messagepool_Get()->IsValid(message))
				return;
			Assert(index == message.GetOwner());
			if(index != message.GetOwner())
				return;
			g_network.Block(index);
			message.Reject();
			message.Kill();
			g_network.Unblock(index);
			break;
		}
		case NET_ACTION_ACCEPT_MESSAGE:
		{
			Message message(m_data[0]);
			Assert(messagepool_Get()->IsValid(message));
			if(!messagepool_Get()->IsValid(message))
				return;
			Assert(index == message.GetOwner());
			if(index != message.GetOwner())
				return;
			g_network.Block(index);
			message.Accept();
			message.Kill();
			g_network.Unblock(index);
			break;
		}


		case NET_ACTION_SET_THIRD_PARTY:
		{
			DiplomaticRequest request(m_data[0]);
			Assert(diplomaticrequestpool_Get()->IsValid(request));
			if(!diplomaticrequestpool_Get()->IsValid(request)) {
				return;
			}

			DiplomaticRequestData *requestData = request.AccessData();
			requestData->m_thirdParty = PLAYER_INDEX(m_data[1]);
			break;
		}

		case NET_ACTION_SET_RESPONSE:
		{
			DiplomaticRequest request(m_data[0]);
			Assert(diplomaticrequestpool_Get()->IsValid(request));
			if(!diplomaticrequestpool_Get()->IsValid(request)) {
				return;
			}

			DiplomaticRequestData *requestData = request.AccessData();
			requestData->m_response = REQUEST_RESPONSE_TYPE(m_data[1]);
			break;
		}

		case NET_ACTION_SET_REWARD_GOLD:
		{

			Assert(FALSE) ;
			break;
		}

		case NET_ACTION_SET_REWARD_CITY:
		{

			Assert(FALSE) ;
			break;
		}
		case NET_ACTION_SET_REWARD_ADVANCE:
		{

			Assert(FALSE) ;
			break;
		}
		case NET_ACTION_SET_TARGET_CITY:
		{
			DiplomaticRequest request(m_data[0]);
			Assert(diplomaticrequestpool_Get()->IsValid(request));
			if(!diplomaticrequestpool_Get()->IsValid(request)) {
				return;
			}

			DiplomaticRequestData *requestData = request.AccessData();
			requestData->m_targetCity = Unit(m_data[1]);
			break;
		}
		case NET_ACTION_SET_REQUEST_GOLD:
		{
			DiplomaticRequest request(m_data[0]);
			Assert(diplomaticrequestpool_Get()->IsValid(request));
			if(!diplomaticrequestpool_Get()->IsValid(request)) {
				return;
			}

			DiplomaticRequestData *requestData = request.AccessData();
			requestData->m_amount.SetLevel(m_data[1]);
			break;
		}
		case NET_ACTION_SET_ADVANCE:
		{
			DiplomaticRequest request(m_data[0]);
			Assert(diplomaticrequestpool_Get()->IsValid(request));
			if(!diplomaticrequestpool_Get()->IsValid(request)) {
				return;
			}

			DiplomaticRequestData *requestData = request.AccessData();
			requestData->m_advance = AdvanceType(m_data[1]);
			break;
		}
		case NET_ACTION_SET_WANTED_ADVANCE:
		{
			DiplomaticRequest request(m_data[0]);
			Assert(diplomaticrequestpool_Get()->IsValid(request));
			if(!diplomaticrequestpool_Get()->IsValid(request)) {
				return;
			}

			DiplomaticRequestData *requestData = request.AccessData();
			requestData->m_reciprocalAdvance = AdvanceType(m_data[1]);
			break;
		}

		case NET_ACTION_SET_WANTED_CITY:
		{
			DiplomaticRequest request(m_data[0]);
			Assert(diplomaticrequestpool_Get()->IsValid(request));
			if(!diplomaticrequestpool_Get()->IsValid(request)) {
				return;
			}

			DiplomaticRequestData *requestData = request.AccessData();
			requestData->m_reciprocalCity = Unit(m_data[1]);
			break;
		}
		case NET_ACTION_PILLAGE:
		{
			Assert(FALSE);
			break;
		}
		case NET_ACTION_RESEARCH:
		{
			if(player_Get(index)) {
				g_network.Block(index);
				player_Get(index)->SetResearching(m_data[0]);
				g_network.Unblock(index);
			}
			break;
		}
		case NET_ACTION_KILLED_PLAYER:
		{
			Assert(player_Get(m_data[0]) == nullptr || player_Get(m_data[0])->m_isDead);
			break;
		}
		case NET_ACTION_INJOIN:
		{
			Assert(FALSE);
			break;
		}
		case NET_ACTION_CHANGE_BUILD:
		{
			if(player_Get(index)) {
				Unit unit(m_data[0]);
				player_Get(index)->ChangeCurrentlyBuildingItem(unit, m_data[1], m_data[2]);
			}
			break;
		}
		case NET_ACTION_USE_SPACE_LADDER:
		{

			break;
		}
		case NET_ACTION_END_SLICE:
		{
			int curPlayerIndex = g_network.IdToIndex(id);

			Assert(curPlayerIndex == selitem_Get()->GetCurPlayer());
			if(curPlayerIndex != selitem_Get()->GetCurPlayer())
				break;

			turn_Get()->EndThisSliceBeginNewSlice();
			break;
		}
		case NET_ACTION_DO_FRONT_ORDER:
		case NET_ACTION_DO_SETTLE_ORDER:
		{
			DPRINTF(k_DBG_NET, ("DO_FRONT_ORDER: %d,%d,%d\n", m_data[0], m_data[1], m_data[2]));
			if(m_action == NET_ACTION_DO_SETTLE_ORDER) {
				DPRINTF(k_DBG_NET, ("DO_SETTLE_ORDER: %lx", m_data[3]));
			}

			break;
		}

		case NET_ACTION_SET_GOVERNMENT:
			DPRINTF(k_DBG_NET, ("Client %d set government to %d\n", index, m_data[0]));
			if(player_Get(index)) {
				player_Get(index)->SetGovernmentType(m_data[0]);
			}
			break;
		case NET_ACTION_ENACT_REQUEST:
		{
			DiplomaticRequest req(m_data[0]);

			if(!diplomaticrequestpool_Get()->IsValid(req)) {
				g_network.QueuePacket(id, new NetInfo(NET_INFO_CODE_NAK_ENACT,
													  m_data[0]));
				break;
			}
			Assert(req.GetRecipient() == index);
			if(req.GetRecipient() != index) {
				g_network.Resync(index);
				break;
			}
			req.Enact(selitem_Get()->GetCurPlayer() == index &&
					  g_network.CurrentPlayerAckedBeginTurn());

			break;
		}

		case NET_ACTION_REJECT_REQUEST:
		{
			DiplomaticRequest req(m_data[0]);

			if(!diplomaticrequestpool_Get()->IsValid(req))
				break;
			Assert(req.GetRecipient() == index);
			if(req.GetRecipient() != index) {
				g_network.Resync(index);
				break;
			}
			g_network.Block(index);
			req.Reject();
			req.Kill();
			g_network.Unblock(index);
			break;
		}
		case NET_ACTION_CREATE_UNIT_CHEAT:
		{
#ifdef _DEBUG
			MapPoint pnt(m_data[1], m_data[2]);
			if(player_Get(index)) {
				player_Get(index)->CreateUnit(m_data[0], pnt, Unit(m_data[3]),
											FALSE, CAUSE_NEW_ARMY_CHEAT);
			}
#endif
			break;
		}
		case NET_ACTION_CREATE_CITY_CHEAT:
		{
#ifdef _DEBUG
			MapPoint pnt(m_data[1], m_data[2]);
			if(player_Get(index)) {
				player_Get(index)->CreateCity(m_data[0], pnt,
											CAUSE_NEW_CITY_CHEAT,
											nullptr, -1);
			}
#endif
			break;
		}
		case NET_ACTION_DONE_SETTING_UP:
			g_network.SignalSetupDone(index);
			break;

		case NET_ACTION_ADVANCE_CHEAT:
#ifdef _DEBUG
			if(player_Get(index))
				player_Get(index)->m_advances->GiveAdvance(m_data[0], CAUSE_SCI_CHEAT, TRUE);
#endif
			break;

		case NET_ACTION_TAKE_ADVANCE_CHEAT:
#ifdef _DEBUG
			if(player_Get(index))
				player_Get(index)->m_advances->TakeAdvance(m_data[0]);
#endif
			break;
		case NET_ACTION_CHEAT_ADD_MATERIALS:
#ifdef _DEBUG
			if(player_Get(index))
				player_Get(index)->m_materialPool->CheatAddMaterials(m_data[0]);
#endif
			break;
		case NET_ACTION_CHEAT_SUB_MATERIALS:
#ifdef _DEBUG
			if(player_Get(index))
				player_Get(index)->m_materialPool->CheatSubtractMaterials(m_data[0]);
#endif
			break;
		case NET_ACTION_CHEAT_CREATE_TERRAIN_IMPROVEMENT:
		{
#ifdef _DEBUG
			MapPoint pnt(m_data[1], m_data[2]);
			Assert(g_network.CanStillSetup(index));
			if(!g_network.CanStillSetup(index))
				return;
			if(player_Get(index))
				player_Get(index)->CreateImprovement((TERRAIN_IMPROVEMENT)m_data[0], pnt,
												   m_data[3]);
#endif
			break;
		}
		case NET_ACTION_SELL_UNITS:
		{
			MapPoint pnt(m_data[0], m_data[1]);
			Assert(g_network.CanStillSetup(index));
			if(!g_network.CanStillSetup(index))
				return;
			if(player_Get(index))
				player_Get(index)->TradeUnitsForPoints(pnt);
			break;
		}

		case NET_ACTION_SELL_ONE_UNIT:
		{
			Unit unit(m_data[0]);
			Assert(g_network.CanStillSetup(index));
			if(!g_network.CanStillSetup(index))
				return;
			if(player_Get(index))
				player_Get(index)->TradeUnitForPoints(unit);
			break;
		}

		case NET_ACTION_SELL_IMPROVEMENTS:
		{
			MapPoint pnt(m_data[0], m_data[1]);
			Assert(g_network.CanStillSetup(index));
			if(!g_network.CanStillSetup(index))
				return;
			if(player_Get(index))
				player_Get(index)->TradeImprovementsForPoints(pnt);
			break;
		}
		case NET_ACTION_AIRLIFT:
		{
			Assert(FALSE);
			break;
		}
		case NET_ACTION_BUY_FRONT:
		{
			Unit city(m_data[0]);
			if(unitpool_Get()->IsValid(city)) {
				Assert(index == city.GetOwner());
				if(index == city.GetOwner()) {
					city.BuyFront();
				}
			}
			break;
		}
		case NET_ACTION_ACK_BEGIN_TURN:
		{
			Assert(index == selitem_Get()->GetCurPlayer());
			if(index == selitem_Get()->GetCurPlayer()) {
				g_network.AckBeginTurn(index);
			}
			break;
		}

		case NET_ACTION_CREATED_CITY:
		{





				UnitDynamicArray *cityList = g_network.GetCreatedCities(index);
				if(cityList) {
					Assert(cityList->Num() > 0);
					if(cityList->Num() > 0 && uint32(cityList->Get(0)) == m_data[0]) {
						g_network.QueuePacket(id, new NetInfo(NET_INFO_CODE_ACK_OBJECT,
															  m_data[0]));
						cityList->DelIndex(0);
					} else if(cityList->Num() > 0){
						g_network.QueuePacket(id, new NetInfo(NET_INFO_CODE_NAK_OBJECT,
															  m_data[0],
															  uint32(cityList->Get(0))));
						cityList->DelIndex(0);
					} else {
						g_network.QueuePacket(id, new NetInfo(NET_INFO_CODE_NAK_OBJECT,
															  m_data[0], 0));
					}
				}

			break;
		}
		case NET_ACTION_SELL_BUILDING:
		{
			DPRINTF(k_DBG_NET, ("Server: Client %d selling building %d in city %lx\n",
								index, m_data[1], m_data[0]));
			Unit city(m_data[0]);
			if(unitpool_Get()->IsValid(city)) {
				city.GetData()->GetCityData()->SellBuilding(m_data[1]);
			}
			break;
		}
		case NET_ACTION_ACTUALLY_SET_GOVERNMENT:
		{
			DPRINTF(k_DBG_NET, ("Server: Client %d actually set government to %d\n", index, m_data[0]));
			if(player_Get(index)) {
				player_Get(index)->ActuallySetGovernment(m_data[0]);
			}
			break;
		}
		case NET_ACTION_REMOVE_BUILD_ITEM:
		{
			DPRINTF(k_DBG_NET, ("Server: Client %d removed build item %d from city %lx\n",
								index, m_data[1], m_data[0]));
			Unit city(m_data[0]);
			Assert(unitpool_Get()->IsValid(city));
			if(unitpool_Get()->IsValid(city)) {
				city.AccessData()->GetCityData()->GetBuildQueue()->RemoveNodeByIndex(m_data[1],
																					 (CAUSE_REMOVE_BUILD_ITEM)m_data[2]);
			}
			break;
		}
		case NET_ACTION_CLEAR_ORDERS:
		{
			DPRINTF(k_DBG_NET, ("Server: Client %d cleared orders for army %lx\n",
								index, m_data[0]));
			Army army(m_data[0]);

			if(armypool_Get()->IsValid(army)) {
				army.ClearOrders();
			} else {
				DPRINTF(k_DBG_NET, ("Invalid army %lx\n", m_data[0]));
			}
			break;
		}

		case NET_ACTION_EXECUTE_ORDERS:
		{
			DPRINTF(k_DBG_NET, ("Server: Client %d's army %lx executing orders\n",
								index, m_data[0]));
			Army army(m_data[0]);

			if(armypool_Get()->IsValid(army)) {
				gevmanager_Get()->Pause();
				BOOL res = army->ExecuteOrders();
				Assert(res);
				if(!res) {
					g_network.Resync(index);
				}
				gevmanager_Get()->Resume();
			} else {
				DPRINTF(k_DBG_NET, ("Invalid army %lx\n", m_data[0]));
			}
			break;
		}
		case NET_ACTION_CREATED_ARMY:
		{
			DPRINTF(k_DBG_NET, ("Server: client %d created Army %lx, cause %d\n",
								index, m_data[0], m_data[1]));

			CAUSE_NEW_ARMY cause = (CAUSE_NEW_ARMY)m_data[1];
			PlayerData *pd = g_network.m_playerData[index];
			Assert((pd && pd->m_createdArmies.Num() > 0) || (cause == CAUSE_NEW_ARMY_UNGROUPING) || (cause == CAUSE_NEW_ARMY_GROUPING));
			if((!pd || pd->m_createdArmies.Num() <= 0) && (cause != CAUSE_NEW_ARMY_UNGROUPING) && (cause != CAUSE_NEW_ARMY_GROUPING)) {
				g_network.Resync(index);
				break;
			}

			if(cause == CAUSE_NEW_ARMY_UNGROUPING || cause == CAUSE_NEW_ARMY_GROUPING) {
				player_Get(index)->GetNewArmy(cause);
			}

			if(pd->m_createdArmies[0] == Army(m_data[0])) {
				g_network.QueuePacket(id, new NetInfo(NET_INFO_CODE_ACK_OBJECT,
													  m_data[0]));
			} else {
				c3errors_ErrorDialog("NET TESTING", "NAK: Army 0x%lx should be 0x%lx",
					m_data[0], pd->m_createdArmies[0].m_id);
				g_network.QueuePacket(id,
									  new NetInfo(NET_INFO_CODE_NAK_OBJECT,
												  m_data[0], (uint32)pd->m_createdArmies[0]));
			}
			pd->m_createdArmies.DelIndex(0);

			break;
		}
		case NET_ACTION_SEND_TRADE_BID:
		{
			DPRINTF(k_DBG_NET, ("Server: Trade Bid: from: %d, fc: %lx, res: %d, tc: %lx, price: %d",
								m_data[0], m_data[1], m_data[2], m_data[3], m_data[4]));
			Assert(player_Get(m_data[0]));
			if(player_Get(m_data[0])) {
				Unit fromCity(m_data[1]);
				Unit toCity(m_data[3]);
				if(unitpool_Get()->IsValid(fromCity) &&
				   unitpool_Get()->IsValid(toCity))
					player_Get(m_data[0])->SendTradeBid(fromCity, m_data[2],
													  toCity, m_data[4]);
			}
			break;
		}

		case NET_ACTION_ACCEPT_TRADE_BID:
		{
			DPRINTF(k_DBG_NET, ("Server: Accept Trade Bid: from: %d, fc: %lx, res: %d, tc: %lx, price: %d",
								m_data[0], m_data[1], m_data[2], m_data[3], m_data[4]));
			Assert(player_Get(m_data[0]));
			if(player_Get(m_data[0])) {
				Unit fromCity(m_data[1]);
				Unit toCity(m_data[3]);
				if(unitpool_Get()->IsValid(fromCity) &&
				   unitpool_Get()->IsValid(toCity))
					player_Get(m_data[0])->AcceptTradeBid(fromCity, m_data[2],
														toCity, m_data[4]);
			}
			break;
		}
		case NET_ACTION_REJECT_TRADE_BID:
		{
			DPRINTF(k_DBG_NET, ("Server: Reject Trade Bid: from: %d, fc: %lx, res: %d, tc: %lx, price: %d",
								m_data[0], m_data[1], m_data[2], m_data[3], m_data[4]));
			Assert(player_Get(m_data[0]));
			if(player_Get(m_data[0])) {
				Unit fromCity(m_data[1]);
				Unit toCity(m_data[3]);
				if(unitpool_Get()->IsValid(fromCity) &&
				   unitpool_Get()->IsValid(toCity))
					player_Get(m_data[0])->RejectTradeBid(fromCity, m_data[2],
														toCity, m_data[4]);
			}
			break;
		}
		case NET_ACTION_CREATE_OWN_ARMY:
		{
			DPRINTF(k_DBG_NET, ("Server: Client %d creating new army for unit %lx\n",
								index, m_data[0]));
			Unit unit(m_data[0]);
			Assert(unitpool_Get()->IsValid(unit));
			if(unitpool_Get()->IsValid(unit)) {
				unit.AccessData()->CreateOwnArmy();
			}
			break;
		}
		case NET_ACTION_SEND_SLAVE_TO:
		{
			DPRINTF(k_DBG_NET, ("Server: Client %d sent slave from %lx to %lx\n",
								m_data[0], m_data[1]));
			Unit fromCity(m_data[0]);
			Unit toCity(m_data[1]);
			Assert(unitpool_Get()->IsValid(fromCity));
			Assert(unitpool_Get()->IsValid(toCity));
			if(unitpool_Get()->IsValid(fromCity) &&
			   unitpool_Get()->IsValid(toCity)) {
				fromCity.SendSlaveTo(toCity);
			}
			break;
		}
		case NET_ACTION_NAK_BEGIN_TURN:
		{
			DPRINTF(k_DBG_NET, ("Client %d NAK'ed begin turn, trying again\n", index));
			Assert(selitem_Get()->GetCurPlayer() == index);
			if(selitem_Get()->GetCurPlayer() == index) {
				g_network.QueuePacket(id, new NetInfo(NET_INFO_CODE_BEGIN_TURN,
													  index));
			}
			break;
		}
		case NET_ACTION_CLEAR_ORDERS_EXCEPT_GROUP:
		{
			Assert(FALSE);
			break;
		}
		case NET_ACTION_BUILD_INFRASTRUCTURE:
		{
			DPRINTF(k_DBG_NET, ("City %lx building infrastructure\n", m_data[0]));
			Unit city(m_data[0]);
			Assert(unitpool_Get()->IsValid(city));
			if(unitpool_Get()->IsValid(city)) {
				Assert(city.GetOwner() == index);
				if(city.GetOwner() == index)
					city.BuildInfrastructure();
			}
			break;
		}
		case NET_ACTION_BUILD_CAPITALIZATION:
		{
			DPRINTF(k_DBG_NET, ("City %lx building capitalization\n", m_data[0]));
			Unit city(m_data[0]);
			Assert(unitpool_Get()->IsValid(city));
			if(unitpool_Get()->IsValid(city)) {
				Assert(city.GetOwner() == index);
				if(city.GetOwner() == index)
					city.BuildCapitalization();
			}
			break;
		}
		case NET_ACTION_BUILD_END_GAME:
		{
			DPRINTF(k_DBG_NET, ("City %lx building end game object %d\n", m_data[0], m_data[1]));
			Unit city(m_data[0]);
			Assert(unitpool_Get()->IsValid(city));
			if(unitpool_Get()->IsValid(city)) {
				city.BuildEndGame(m_data[1]);
			}
			break;
		}
		case NET_ACTION_DISBAND_CITY:
		{
			DPRINTF(k_DBG_NET, ("City %lx disbanded by client %d\n", m_data[0], index));
			Unit city(m_data[0]);
			Assert(unitpool_Get()->IsValid(city));
			if(unitpool_Get()->IsValid(city)) {
				gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_DisbandCity,
									   GEA_City, city.m_id,
									   GEA_End);
			}
			break;
		}
		case NET_ACTION_TAKE_TRADE_OFFER:
		{
			DPRINTF(k_DBG_NET, ("Client %d takes trade offer %lx\n", index, m_data[0]));
			TradeOffer offer(m_data[0]);
			if(tradeofferpool_Get()->IsValid(offer)) {
				if(player_Get(index)) {
					Unit unit1(m_data[1]);
					Unit unit2(m_data[2]);
					player_Get(index)->AcceptTradeOffer(offer, unit1, unit2);
				}
			}
			break;
		}
		case NET_ACTION_ACCEPT_TRADE_OFFER:
		{
			DPRINTF(k_DBG_NET, ("Client %d accepted trade offer %lx\n",
								index, m_data[0]));
			TradeOffer offer(m_data[0]);
			if(tradeofferpool_Get()->IsValid(offer)) {
				if(player_Get(index)) {
					if(offer.Accept(m_data[1],
									Unit(m_data[2]),
									Unit(m_data[3]))) {
						if(tradeofferpool_Get()->IsValid(offer)) {
							offer.Kill();
						}
					}
				}
			}
			break;
		}
		case NET_ACTION_KILL_AGREEMENT:
		{
			DPRINTF(k_DBG_NET, ("Client %d is killing agreement %lx\n",
								index, m_data[0]));
			Agreement agreement(m_data[0]);
			if(agreementpool_Get()->IsValid(agreement)) {
				agreement.Kill();
			}
			break;
		}
		case NET_ACTION_BREAK_ALLIANCE:
			DPRINTF(k_DBG_NET, ("Client %d breaking alliances with %d\n",
								index, m_data[1]));
			Assert(index == (sint32)m_data[0]);
			if(index == (sint32)m_data[0]) {
				if(player_Get(index)) {
					player_Get(index)->BreakAlliance(m_data[1]);
				}
			}
			break;
		case NET_ACTION_BREAK_CEASE_FIRE:
			DPRINTF(k_DBG_NET, ("Client %d breaking cease fires with %d\n",
								index, m_data[1], m_data[2]));
			Assert(index == (sint32)m_data[0]);
			if(index == (sint32)m_data[0]) {
				if(player_Get(index)) {
					player_Get(index)->BreakCeaseFire(m_data[1], m_data[2] != 0);
				}
			}
			break;

		case NET_ACTION_CLEAR_QUEUE:
		{
			DPRINTF(k_DBG_NET, ("Client %d clearing queue for city %lx\n",
								index, m_data[0]));
			Unit city(m_data[0]);
			if(unitpool_Get()->IsValid(city) && city->GetOwner() == index) {
				city.GetData()->GetCityData()->GetBuildQueue()->Clear();
			}
			break;
		}
		case NET_ACTION_REQUEST_RESYNC:
		{
			DPRINTF(k_DBG_NET, ("Client %d requests resync, reason %d\n", index, m_data[0]));
			g_network.Resync(index);
			break;
		}
		case NET_ACTION_ACK_RESYNC:
		{
			DPRINTF(k_DBG_NET, ("Client %d acks resync\n", index));
			g_network.AckResync(index);
			break;
		}

		case NET_ACTION_VERIFY_POS:
		{
			DPRINTF(k_DBG_NET, ("Client %d claims Army %lx is at (%d,%d)\n",
								index, m_data[0], m_data[1], m_data[2]));
			Army army(m_data[0]);
			Assert(armypool_Get()->IsValid(army));

			if(armypool_Get()->IsValid(army)) {
				MapPoint apos;
				army.GetPos(apos);
				Assert(apos.x == (sint16)m_data[1]);
				Assert(apos.y == (sint16)m_data[2]);
				if(apos.x != (sint16)m_data[1] ||
					apos.y != (sint16)m_data[2]) {
					DPRINTF(k_DBG_NET, ("It's actually at (%d,%d)\n",apos.x, apos.y));
					g_network.Resync(index);
				}
			} else {
				DPRINTF(k_DBG_NET, ("But it isn't even a valid army\n"));
				g_network.Resync(index);
			}
			break;
		}
		case NET_ACTION_SET_DIP_STATE:
		{
			DPRINTF(k_DBG_NET, ("Player %d's dip state for %d is now %d\n",
								index, m_data[0], m_data[1]));
			if(player_Get(index)) {
				player_Get(index)->SetDiplomaticState(m_data[0],
													(DIPLOMATIC_STATE)m_data[1]);
			}
			break;
		}
		case NET_ACTION_KILL_ALL_TRADE_ROUTES:
		{
			DPRINTF(k_DBG_NET, ("Player %d killing all trade routes at %lx\n",
								index, m_data[0]));
			Unit city(m_data[0]);
			if(unitpool_Get()->IsValid(city)) {
				Assert(index == city.GetOwner());
				if(index == city.GetOwner()) {
					g_network.Block(index);
					city.GetData()->GetCityData()->KillAllTradeRoutes();
					g_network.Unblock(index);
				}
			}
			break;
		}
		case NET_ACTION_ACK_ENACT:
		{
			DPRINTF(k_DBG_NET, ("Player %d acked enact of request %lx\n",
								index, m_data[0]));
			DiplomaticRequest req(m_data[0]);
			g_network.RemoveEnact(req);
			break;
		}
		case NET_ACTION_ACK_REMOVE_ILLEGAL:
		{
			DPRINTF(k_DBG_NET, ("Client %d acknowledges RemoveIllegalItems at %lx\n", index, m_data[0]));
			Unit city(m_data[0]);
			if(unitpool_Get()->IsValid(city)) {


				if(city.GetOwner() == index) {
					city.GetData()->GetCityData()->GetBuildQueue()->RemoveIllegalItems(TRUE);
				}
			}
			break;
		}
		case NET_ACTION_CLEAR_QUEUE_EXCEPT_HEAD:
		{
			DPRINTF(k_DBG_NET, ("Client %d clearing queue except head for city %lx\n",
								index, m_data[0]));
			Unit city(m_data[0]);

			if(unitpool_Get()->IsValid(city)) {
				city.GetData()->GetCityData()->GetBuildQueue()->ClearAllButHead();
			}
			break;
		}
		case NET_ACTION_VIOLATE_AGREEMENT:
		{
			DPRINTF(k_DBG_NET, ("Client %d claims it's in violation of %lx\n",
								m_data[0]));
			Agreement ag(m_data[0]);
			if(agreementpool_Get()->IsValid(ag)) {
				Assert(ag.GetRecipient() == index);
				if(ag.GetRecipient() == index) {
					ag.AccessData()->RecipientIsViolating(ag.GetOwner(), TRUE, turn_Get()->GetRound());
				} else {
					g_network.Resync(index);
				}
			}
			break;
		}
		case NET_ACTION_OFFER_REJECTED_MESSAGE:
		{
			DPRINTF(k_DBG_NET, ("Client %d says offer %d/%d rejected\n",
								index, m_data[0], m_data[1]));
			g_network.QueuePacket(g_network.IndexToId(m_data[0]),
								  new NetInfo(NET_INFO_CODE_OFFER_REJECTED_MESSAGE,
											  m_data[0], m_data[1]));
			if(player_Get(m_data[0]) &&
			   player_Get(m_data[1])) {
				SlicObject *so = new SlicObject("91OfferRejected");
				so->AddRecipient(m_data[0]);
				so->AddCivilisation(m_data[1]);
				slicengine_Get()->Execute(so);
			}
			break;
		}
		case NET_ACTION_FINISH_BUILDING:
		{
			DPRINTF(k_DBG_NET, ("Finish building from %d at %lx\n",
								index, m_data[0]));
			Unit city(m_data[0]);
			if(unitpool_Get()->IsValid(city)) {
				if(city.GetOwner() == index) {
					city.GetData()->GetCityData()->FinishBuilding();
				}
			}
			break;
		}
		case NET_ACTION_FREE_SLAVES:
		{
			DPRINTF(k_DBG_NET, ("Client %d frees slaves at %lx\n",
								index, m_data[0]));
			Unit city(m_data[0]);
			if(unitpool_Get()->IsValid(city)) {
				if(city.GetOwner() == index) {
					if(!city.GetData()->GetCityData()->CapturedThisTurn()) {
						g_network.Resync(index);
					} else {
						city.FreeSlaves();
					}
				}
			}
			break;
		}
		case NET_ACTION_TRANSPORT_SELECT:
		{
			DPRINTF(k_DBG_NET, ("Client %d selected %lx for unloading\n",
								index, m_data[0]));
			Unit unit(m_data[0]);
			if(unitpool_Get()->IsValid(unit)) {
				unit.SetFlag(k_UDF_TEMP_TRANSPORT_SELECT);
			}
			break;
		}
		case NET_ACTION_TRANSPORT_UNSELECT:
		{
			DPRINTF(k_DBG_NET, ("Client %d unselected %lx for unloading\n",
								index, m_data[0]));
			Unit unit(m_data[0]);
			if(unitpool_Get()->IsValid(unit)) {
				unit.ClearFlag(k_UDF_TEMP_TRANSPORT_SELECT);
			}
			break;
		}
		case NET_ACTION_RESET_ROUTE:
		{
			DPRINTF(k_DBG_NET, ("Client %d reset route %lx\n", index, m_data[0]));
			TradeRoute route(m_data[0]);

			if(!tradepool_Get()->IsValid(route)) {
				g_network.Resync(index);
			} else {
				ROUTE_TYPE	routeType;
				sint32		resIndex = 0;

				Unit sourceCity = route.GetSource();
				if(!unitpool_Get()->IsValid(sourceCity)) {
					g_network.Resync(index);
					return;
				}

				sint32 sourceOwner = sourceCity.GetOwner();
				route.GetSourceResource(routeType, resIndex);

				Unit destCity = route.GetDestination();
				if(!unitpool_Get()->IsValid(destCity)) {
					g_network.Resync(index);
					return;
				}

				sint32 destOwner = destCity.GetOwner();
				sint32 resValue = route.GetGoldInReturn();

				route.Kill(CAUSE_KILL_TRADE_ROUTE_RESET);

				player_Get(sourceOwner)->CreateTradeRoute(sourceCity, ROUTE_TYPE_RESOURCE, resIndex, destCity, destOwner, resValue);
			}
			break;
		}
		case NET_ACTION_SET_MAYOR:
		{
			DPRINTF(k_DBG_NET, ("Client %d set city 0x%lx mayor to %d,%d\n", index, m_data[0], m_data[1], m_data[2]));
			Unit city(m_data[0]);
			if(!city.IsValid()) {
				g_network.Resync(index);
			} else {
				city.CD()->SetUseGovernor(m_data[2] != 0);
				city.CD()->SetBuildListSequenceIndex(m_data[1]);
			}
			break;
		}
		case NET_ACTION_SET_SPECIALISTS:
		{
			DPRINTF(k_DBG_NET, ("Client %d set specialist type %d for city 0x%lx to %d",
								index, m_data[1], m_data[0], m_data[2]));
			Unit city(m_data[0]);
			if(!city.IsValid()) {
				g_network.Resync(index);
			} else {
				sint32 oldValue = city.CD()->SpecialistCount((POP_TYPE)m_data[1]);
				city.CD()->ChangeSpecialists((POP_TYPE)m_data[1], (sint32)m_data[2] - oldValue);
			}
			break;
		}
		case NET_ACTION_START_MOVE_PHASE:
		{
			DPRINTF(k_DBG_NET, ("Client %d ready for move phase\n", index));
			gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
								   GEV_StartMovePhase,
								   GEA_Player, index,
								   GEA_End);
			break;
		}
		case NET_ACTION_DECLARE_WAR:
		{
			DPRINTF(k_DBG_NET, ("Client %d declaring war on %d\n", index, m_data[0]));
			Diplomat::GetDiplomat(index).DeclareWar(m_data[0]);
			break;
		}
		case NET_ACTION_REQUEST_TRADE_ROUTE:
		{
			Unit src(m_data[1]);
			Unit dest(m_data[2]);
			DPRINTF(k_DBG_NET, ("Client %d wants a trade route from %lx(%d) to %lx(%d) carrying %d\n",
								index, m_data[1], src.IsValid(), m_data[2], dest.IsValid(), m_data[0]));
			if(!player_Get(index)) {
				g_network.Resync(index);
				break;
			}

			if(!src.IsValid() || !dest.IsValid()) {
				g_network.Resync(index);
				break;
			}

			player_Get(index)->CreateTradeRoute(src, ROUTE_TYPE_RESOURCE,
											  m_data[0], dest, index, 0);
			break;
		}
		case NET_ACTION_DISBAND_UNIT:
		{
			Unit u(m_data[0]);
			if(!u.IsValid()) {
				g_network.Resync(index);
				break;
			}

			if(u.GetOwner() != index) {
				g_network.Resync(index);
				break;
			}

			gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_DisbandUnit,
								   GEA_Unit, m_data[0],
								   GEA_End);
			break;
		}
		default:
			Assert(FALSE);
			break;
	}
}
