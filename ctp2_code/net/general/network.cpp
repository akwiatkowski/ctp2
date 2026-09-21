//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Network framework
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
// - Generates debug information when set.
//
// _PLAYTEST
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Network object bookkeeping gets cleared when exiting a game (exiting and
//   rejoining occasionally left something lying around, resulting in a resync
//   not long after joining) (bug #30)
// - Updated the above to prevent an invalid second delete.
// - Feat tracking added.
// - Memory leaks repaired.
// - Replaced old civilisation database by new one. (Aug 20th 2005 Martin G�hmann)
// - Database in synchronicity check is now done on all databases. (Aug 25th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/world/Cell.h"

#include "net/io/net_types.h"
#include "net/general/network.h"
#include "net/io/net_io.h"
#include "net/io/net_anet.h"
#include "net/io/net_thread.h"
#include "net/general/net_packet.h"
#include "net/general/net_cell.h"
#include "net/io/net_util.h"
#include "net/general/net_unit.h"
#include "net/general/net_action.h"
#include "net/general/net_info.h"
#include "net/general/net_city.h"
#include "net/general/net_diff.h"
#include "net/general/net_player.h"
#include "net/general/net_traderoute.h"
#include "net/general/net_tradeoffer.h"
#include "net/general/net_rand.h"
#include "net/general/net_terrain.h"
#include "net/general/net_installation.h"
#include "net/general/net_ready.h"
#include "net/general/net_happy.h"
#include "net/general/net_report.h"
#include "net/general/net_order.h"
#include "net/general/net_agreement.h"
#include "net/general/net_civ.h"
#include "net/general/net_diplomacy.h"
#include "net/general/net_message.h"
#include "net/general/net_pollution.h"
#include "net/general/net_keys.h"
#include "net/general/net_gamesettings.h"
#include "net/general/net_army.h"
#include "net/general/net_playerdata.h"
#include "net/general/net_gameobj.h"
#include "net/general/net_chat.h"
#include "net/general/net_crc.h"
#include "net/general/net_wonder.h"
#include "net/general/net_achievement.h"
#include "net/general/net_vision.h"
#include "net/general/net_exclusions.h"
#include "net/general/net_research.h"
#include "net/general/net_guid.h"
#include "net/general/net_strengths.h"
#include "net/general/net_endgame.h"
#include "net/general/net_world.h"
#include "net/general/net_feat.h"

#ifdef _PLAYTEST
#include "net/general/net_cheat.h"
extern sint32 g_debugOwner;
#endif

#include "gs/gameobj/UnitData.h"
#include "gs/gameobj/player.h"
#include "gs/gameobj/XY_Coordinates.h"
#include "gs/world/World.h"
#include "gs/gameobj/UnitPool.h"
#include "gs/gameobj/citydata.h"
#include "gs/gameobj/TradeRouteData.h"
#include "gs/gameobj/Gold.h"
#include "robot/pathing/Path.h"
#include "gs/gameobj/Agreement.h"
#include "gs/gameobj/CivilisationPool.h"
#include "gs/gameobj/Civilisation.h"
#include "gs/gameobj/TradeOffer.h"
#include "gs/gameobj/TerrImprove.h"
#include "gs/gameobj/installation.h"
#include "gs/gameobj/DiplomaticRequest.h"
#include "gs/gameobj/DiplomaticRequestPool.h"
#include "gs/gameobj/message.h"
#include "gs/gameobj/TradePool.h"
#include "gs/utility/TurnCnt.h"
#include "gfx/tilesys/tiledmap.h"
#include "ui/aui_ctp2/radarmap.h"
#include "ui/interface/chatbox.h"
#include "gs/gameobj/ArmyData.h"
#include "gs/gameobj/ArmyPool.h"
#include "gs/gameobj/Order.h"
#include "gs/world/UnseenCell.h"
#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicObject.h"
#include "gs/gameobj/Exclusions.h"
#include "gs/database/profileDB.h"
#include "ctp/ctp2_utils/pointerlist.h"
#include "ui/aui_ctp2/c3_utilitydialogbox.h"
#include "ui/netshell/netfunc.h"
#include "ui/netshell/netshell.h"
#include "ctp/civapp.h"
#include "gs/database/StrDB.h"


#include "gs/gameobj/GameSettings.h"
#include "AgeRecord.h"
#include "CivilisationRecord.h"
#include "gs/events/GameEventManager.h"
#include "ai/ctpai.h"
#include "net/general/chatlist.h"
#include "sound/soundmanager.h"
#include <memory>
#include <vector>


#if !CTP2_ENABLE_NETWORKING
// Single-player UI code still asks whether the old lobby exists. Keep that
// compatibility query local without linking the legacy multiplayer shell.
NETFunc *netfunc_Get()
{
	return nullptr;
}
#endif
#include "sound/gamesounds.h"
#include "ui/interface/progresswindow.h"
extern ProgressWindow		*g_theProgressWindow;



#include "ui/aui_ctp2/SelItem.h"
#include "ctp/ctp2_rsrc/resource.h"
#include "gfx/spritesys/director.h"
#include "ctp/civ3_main.h"
#include "ui/interface/sci_advancescreen.h"
#include "ui/aui_ctp2/c3_utilitydialogbox.h"
#include "ui/aui_common/aui_button.h"
#ifdef _DEBUG
#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_surface.h"
#include "ui/aui_utils/primitives.h"
#endif

#include "ui/interface/controlpanelwindow.h"
#include "ui/interface/MainControlPanel.h"

#include "gs/utility/RandGen.h"
#include "gs/utility/stringutils.h"
#include "ui/interface/screenutils.h"
#include "ui/interface/battleviewwindow.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/interface/sci_advancescreen.h"
#include "ui/interface/dipwizard.h"
#include "ai/diplomacy/Diplomat.h"
#include "gs/gameobj/CTP2Combat.h"
#include "gs/gameobj/Strengths.h"


#define k_CHUNK_HEAD '>'
#define k_CHUNK_BODY 'C'


extern c3_UtilityPlayerListPopup *g_networkPlayersScreen;

void battleview_ExitButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie );

void network_AbortCallback( sint32 type )
{


	civapp_Get()->PostQuitToLobbyAction();

}

namespace
{

//----------------------------------------------------------------------------
//
// Name       : PacketManager
//
// Description: Simple automatic owner class to prevent memory leaks.
//
// Parameters : a_Packet	: packet to own
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : Assumption: a_Packet != NULL
//
//----------------------------------------------------------------------------
	class PacketManager
	{
	public:
		PacketManager(Packetizer * a_Packet)
		:	m_Packet	(a_Packet)
		{
			Assert(m_Packet);
			m_Packet->AddRef();
		};

		virtual ~PacketManager()
		{
			m_Packet->Release();
		};

	private:
		Packetizer * m_Packet;
	};

} // namespace

Network::Network() :
	m_state(NETSTATE_READY),
	m_pid(0),
	m_hostId(0),
	m_iAmHost(FALSE),
	m_iAmClient(FALSE),
	m_initialized(FALSE),
	m_processingNewPlayers(FALSE)
{
	m_noThread = FALSE;

#if CTP2_ENABLE_NETWORKING
	if(m_noThread) {
		m_netIO = std::make_unique<ActivNetIO>();
		m_netIO->Init(this);
	} else {
		m_netIO = std::make_unique<NetThread>();
		m_netIO->Init(this);
	}
#else
	// Single-player builds have no transport or worker thread.  The legacy
	// Network facade remains as an inactive compatibility shim for game code.
	m_netIO.reset();
#endif


	m_transport = 5;
	m_sessionIndex = -1;
	m_chatMask = 0xffffffff;
#ifdef _DEBUG
	InitPacketLog();
	m_displayPackets = FALSE;
#endif

	m_sentReadySignal = FALSE;
	m_deleting = FALSE;

	m_gameStyle = 0;
	m_setupMode = FALSE;

	m_newPlayerList = std::make_unique<PointerList<PlayerData>>();
	m_sessionList = std::make_unique<PointerList<SessionData>>();
	m_gameObjects = std::make_unique<NetGameObj>();
	m_deadUnitList = std::make_unique<NetHash>();

	m_launchFromNetFunc = FALSE;

	m_resetCityOwnerHackList = std::make_unique<DynamicArray<Unit>>();
	m_nsPlayerInfo = std::make_unique<PointerList<NSPlayerInfo>>();
	m_nsAIPlayerInfo = std::make_unique<PointerList<NSAIPlayerInfo>>();

	m_startingAge = 0;

	m_condensePopMoves = FALSE;
	m_enactedDiplomaticRequests = std::make_unique<DynamicArray<DiplomaticRequest>>();

#ifdef WIN32
	char exepath[_MAX_PATH];
	if(GetModuleFileName(NULL, exepath, _MAX_PATH) != 0) {
		char *lastbackslash = strrchr(exepath, FILE_SEPC);
		if(lastbackslash) {
			*lastbackslash = 0;
			SetCurrentDirectory(exepath);
		}
	}

	FILE *guidFile = fopen("nguid.ctp", "rb");
	if(!guidFile) {
		CoCreateGuid(&m_guid);
		guidFile = fopen("nguid.ctp", "wb");
		Assert(guidFile);
		if(guidFile) {
			fwrite(&m_guid, sizeof(m_guid), 1, guidFile);
			fclose(guidFile);
		}
	} else {
		sint32 r = fread(&m_guid, 1, sizeof(m_guid), guidFile);
		Assert(r == sizeof(m_guid));
		fclose(guidFile);
	}
#endif

	m_progress = -1;
	m_extraTimePerCity = 0;
	m_launchHost = FALSE;
	m_rememberExclusions.reset();
	m_teamsEnabled = FALSE;
	m_waitingOnResync = FALSE;
	m_wasAttached = FALSE;
	m_endTurnWhenClear = FALSE;
	m_dynamicJoin = FALSE;
	m_crcError = FALSE;
	m_sensitiveUIBlocked = false;

	m_chatList = std::make_unique<ChatList>();
}

Network::~Network()
{
	m_deleting = TRUE;

	m_netIO.reset();

	for (auto & i : m_playerData)
    {
		i.reset();
	}

	m_newPlayerList.reset();

	while (!m_sessionList->IsEmpty())
    {
	    std::unique_ptr<SessionData>(m_sessionList->RemoveHead());
	}
	m_sessionList.reset();

	m_gameObjects.reset();
	m_deadUnitList.reset();
	m_resetCityOwnerHackList.reset();

	if (m_nsPlayerInfo)
    {
		m_nsPlayerInfo->DeleteAll();
		m_nsPlayerInfo.reset();
	}

	if (m_nsAIPlayerInfo)
    {
		m_nsAIPlayerInfo->DeleteAll();
		m_nsAIPlayerInfo.reset();
	}

	m_enactedDiplomaticRequests.reset();
	m_rememberExclusions.reset();
	m_chatList.reset();
}


void
Network::Cleanup()
{
	uint16 i;

	c3_RemoveAbortMessage();

	if(m_netIO) {
		m_netIO->Reset();

	}

	for(i = 0; i < k_MAX_PLAYERS; i++) {
		m_playerData[i].reset();
	}

	while(!m_sessionList->IsEmpty()) {
		std::unique_ptr<SessionData>(m_sessionList->RemoveHead());
	}

	m_initialized = FALSE;
	m_pid = 0;
	m_hostId = 0;
	m_iAmHost = FALSE;
	m_iAmClient = FALSE;
	m_state = NETSTATE_READY;
	m_processingNewPlayers = FALSE;
	m_readyToStart = FALSE;
	m_crcError = FALSE;








	m_transport = 5;
	m_sessionIndex = -1;
	m_setupMode = FALSE;

	m_launchFromNetFunc = FALSE;

	if(m_nsPlayerInfo) {
		m_nsPlayerInfo->DeleteAll();
	}

	if(m_nsAIPlayerInfo) {
		m_nsAIPlayerInfo->DeleteAll();
	}

	m_startingAge = 0;

	m_enactedDiplomaticRequests->Clear();
	m_sentReadySignal = FALSE;
	m_launchHost = FALSE;
	m_teamsEnabled = FALSE;
	m_waitingOnResync = FALSE;

	m_wasAttached = FALSE;
	m_endTurnWhenClear = FALSE;

	m_dynamicJoin = FALSE;

	if(!exclusions_Get()) {
		exclusions_Set(std::make_unique<Exclusions>().release());
	}

	m_rememberExclusions.reset();

	if(g_networkPlayersScreen) {
		g_networkPlayersScreen->RemoveWindow();
	}

	if(m_newPlayerList) {
		m_newPlayerList->DeleteAll();
	}

	m_sensitiveUIBlocked = false;
}

void Network::SetLaunchFromNetFunc(BOOL fromSave)
{
#if !CTP2_ENABLE_NETWORKING
	(void)fromSave;
	return;
#else
	m_launchFromNetFunc = TRUE;
	m_fromSave = fromSave;
	m_readyToStart = FALSE;
	m_launchHost = NETFunc::IsHost();


	m_rememberExclusions.reset(exclusions_Get());
	exclusions_Set(nullptr);

	if(!m_noThread) {
		((NetThread *)m_netIO.get())->SetDP(netfunc_Get()->GetDP());
	}

	m_newPlayerList->DeleteAll();
#endif
}

void Network::InitFromNetFunc()
{
#if !CTP2_ENABLE_NETWORKING
	return;
#else
	m_initialized = TRUE;
	m_iAmHost = NETFunc::IsHost();
	m_iAmClient = !m_iAmHost;

	Assert(player_arr_Get() && player_Get(0));
	player_Get(0)->SetPlayerType(PLAYER_TYPE_ROBOT);

	m_battleViewOpenedTime = -1;
	m_battleViewOriginalEndTime = 0;

	if(m_fromSave) {


		sint32 i;
		sint32 numLegalSlots = 1;
		GUID zeroGuid;
		memset(&zeroGuid, 0, sizeof(GUID));
		for(i = 0; i < k_MAX_PLAYERS; i++) {
			if(player_Get(i) && memcmp(&zeroGuid, &player_Get(i)->m_networkGuid, sizeof(GUID))) {
				numLegalSlots++;
				OpenPlayer(i);
			} else if(player_Get(i)) {
				ClosePlayer(i);
			}
		}
		gamesetup_Get().SetSize(static_cast<sint16>(numLegalSlots));
		SetMaxPlayers(numLegalSlots);
		if(!exclusions_Get()) {

			exclusions_Set(m_rememberExclusions.release());
		}
	} else {
		if(m_rememberExclusions) {
			if(exclusions_Get()) {
				std::unique_ptr<Exclusions>{exclusions_Get()};
			}
			exclusions_Set(m_rememberExclusions.release());
		}
	}

	if(m_noThread) {
		((ActivNetIO *)m_netIO.get())->SetDP(netfunc_Get()->GetDP());
	} else {

	}

	if(!m_readyToStart) {
		if(m_iAmHost) {
			if(!m_launchHost) {

				civapp_Get()->PostQuitToLobbyAction();
				m_readyToStart = TRUE;
				return;
			}


			const char *str = stringdb_Get()->GetNameStr("NETWORK_WAITING_ON_PLAYERS");

			char nonConstStr[1024];
			if(str) {
				strlcpy(nonConstStr, str, sizeof(nonConstStr));
			} else {
				strlcpy(nonConstStr, "Waiting on players", sizeof(nonConstStr));
			}
			c3_AbortMessage(nonConstStr, k_UTILITY_ABORT, network_AbortCallback);
		} else if(!m_crcError) {
			const char *str = stringdb_Get()->GetNameStr("NETWORK_WAITING_FOR_DATA");
			char nonConstStr[1024];
			if(str) {
			strlcpy(nonConstStr, str, sizeof(nonConstStr));
		} else {
			strlcpy(nonConstStr, "Waiting on data", sizeof(nonConstStr));
			}
			c3_AbortMessage(nonConstStr, k_UTILITY_PROGRESS_ABORT, network_AbortCallback);
		}
	}
#endif
}

void Network::SetNSPlayerInfo(uint16 id,
                              char const *name,
                              int civ,
                              int group,
                              int civpoints,
                              int settlers)
{
	if(group > 0) {
		m_teamsEnabled = TRUE;
	}

	m_nsPlayerInfo->AddTail(std::make_unique<NSPlayerInfo>(id, name, civ, group, civpoints,
                                             settlers).release());
}

void Network::SetNSAIPlayerInfo(int civ,
                                int group,
                                int civpoints,
                                int settlers)
{
	m_nsAIPlayerInfo->AddTail(std::make_unique<NSAIPlayerInfo>(civ, group, civpoints, settlers).release());
}

NSPlayerInfo *Network::GetNSPlayerInfo(sint32 index)
{
	Assert(index >= 0);
	Assert(index < m_nsPlayerInfo->GetCount());
	if(index < 0 || index >= m_nsPlayerInfo->GetCount()) {
		return nullptr;
	}
	sint32 c = 0;
	PointerList<NSPlayerInfo>::Walker walk(m_nsPlayerInfo.get());
	while(walk.IsValid()) {
		if(c == index) {
			return walk.GetObj();
		}
		walk.Next();
		c++;
	}
	Assert(FALSE);
	return nullptr;
}

NSPlayerInfo *Network::GetNSPlayerInfoByID(uint16 id)
{
	PointerList<NSPlayerInfo>::Walker walk(m_nsPlayerInfo.get());
	while(walk.IsValid()) {
		if(id == walk.GetObj()->m_id)
			return walk.GetObj();
		walk.Next();
	}
	return nullptr;
}

NSAIPlayerInfo *Network::GetNSAIPlayerInfo(sint32 index)
{
	Assert(index >= 0);
	Assert(index < m_nsAIPlayerInfo->GetCount());
	if(index < 0 || index >= m_nsAIPlayerInfo->GetCount())
		return nullptr;

	sint32 i = 0;
	PointerList<NSAIPlayerInfo>::Walker walk(m_nsAIPlayerInfo.get());
	while(walk.IsValid()) {
		if(i == index) {
			return walk.GetObj();
		}
		walk.Next();
		i++;
	}
	Assert(FALSE);
	return nullptr;
}

void
Network::Process()
{
	NET_ERR err;

	if(m_launchFromNetFunc) {
		InitFromNetFunc();
		m_launchFromNetFunc = FALSE;
	}

	m_chatList->RemoveExpired();

	if(!m_initialized)
		return;

	DoResetCityOwnerHack();







	ProcessSends();

	switch(m_state) {
	case NETSTATE_JOINING:
		Join(m_sessionIndex);
		m_state = NETSTATE_READY;
		break;
	case NETSTATE_SHOWSESSIONS:
		break;
	default:
		break;
	}

	if(!m_sentReadySignal && m_netIO->ReadyForData()) {
		if(m_hostId == 0) {
			m_netIO->GetHostId(m_hostId);
		}

		if(m_hostId == 0) {
			Assert(m_iAmClient);
		} else {
			uint8 buf[512];
			uint16 size;
			PacketizerPtr guid = make_packetizer<NetGuid>(GetGuid());
			guid->Packetize(buf, size);
			NET_ERR err = m_netIO->Send(m_hostId, TRUE, buf, size);
			Assert(err == NET_ERR_OK);

			PacketizerPtr report = make_packetizer<NetReport>(NET_REPORT_READY_FOR_DATA);
			report->Packetize(buf, size);
			err = m_netIO->Send(m_hostId, TRUE, buf, size);

			Assert(err == NET_ERR_OK);

			m_sentReadySignal = TRUE;
		}
	}




	if(m_netIO) {
		err = m_netIO->Idle();
		Assert(err == NET_ERR_OK);
		if(err != NET_ERR_OK)
			return;
	}

	static time_t battleEndedTime = -1;

	if(BattleViewWindow *bvw = battleviewwindow_Get(); bvw && c3ui_Get()->GetWindow(bvw->Id()) && (!combat_Get() || combat_Get()->IsDone())) {
		if(battleEndedTime < 0) {
			battleEndedTime = time(nullptr);
		} else if(battleEndedTime + 30 < time(nullptr)) {
			battleview_ExitButtonActionCallback(nullptr, AUI_BUTTON_ACTION_EXECUTE, 0, nullptr);
			battleEndedTime = -1;
		}
	} else {
		battleEndedTime = -1;
	}

	if(m_gameStyle & (k_GAME_STYLE_SPEED | k_GAME_STYLE_SPEED_CITIES)) {

		time_t timeNow = time(nullptr);

		bool diplomacyShouldPause = false;
		if(!DipWizard::CanInitiateRightNow() && IsMyTurn()) {

			sint32 p;
			for(p = 0; p < k_MAX_PLAYERS; p++) {
				if(!player_Get(p) || p == selitem_Get()->GetVisiblePlayer())
					continue;
				if(Diplomat::GetDiplomat(GetPlayerIndex()).GetReceiverHasInitiative(p)) {

					diplomacyShouldPause = true;
					break;
				}
			}
		}

		BattleViewWindow *bvw = battleviewwindow_Get();
		if((c3ui_Get() &&
		   (bvw && c3ui_Get()->GetWindow(bvw->Id()))) ||
		   (diplomacyShouldPause)) {
			if(m_battleViewOpenedTime < 0) {
				m_battleViewOpenedTime = timeNow;
			m_battleViewOriginalEndTime = m_turnEndsAt;
			}
			m_turnEndsAt = m_battleViewOriginalEndTime + (timeNow - m_battleViewOpenedTime);
		} else {
			m_battleViewOpenedTime = -1;
		}

		if(selitem_Get()) {
			if(selitem_Get()->GetCurPlayer() ==
			   selitem_Get()->GetVisiblePlayer() &&
			   IsMyTurn() && (!m_iAmHost || m_readyToStart)) {
				if(timeNow >= m_turnEndsAt) {
					if(sci_advancescreen_isOnScreen()) {
						sci_advancescreen_removeMyWindow(AUI_BUTTON_ACTION_EXECUTE);
					}
					director_Get()->AddEndTurn();

				}
			}
		}
	}

	if(m_gameStyle & k_GAME_STYLE_TOTAL_TIME) {
		if (IsMyTurn() &&
		    (m_totalTimeUsed + static_cast<sint32>(time(nullptr) - m_turnStartedAt) > m_totalStartTime)) {

			player_Get(selitem_Get()->GetCurPlayer())->
				GameOver(GAME_OVER_LOST_OUT_OF_TIME, -1);
		}
	}
}

void Network::ProcessSends()
{
	NET_ERR err;
	BOOL isBusy;
	for(sint32 pl = 0; pl < k_MAX_PLAYERS; pl++) {
		if(!player_Get(pl)) continue;
		if(m_playerData[pl] && !m_playerData[pl]->m_frozen && m_playerData[pl]->m_ready) {
			PointerList<Packetizer>* packetList = m_playerData[pl]->m_packetList.get();

			isBusy = FALSE;

			while(m_playerData[pl] && !packetList->IsEmpty() && !isBusy) {
				uint8 buf[8192];
				uint8 *sbuf;
				sint32 size;
				Packetizer* packet = packetList->RemoveHead();
				if(packet->m_packetbuf.empty()) {
					Assert(FALSE);
					uint16 psize;
					packet->Packetize(buf, psize);
					size = (sint32)psize;
					sbuf = buf;
				} else {
					sbuf = packet->m_packetbuf.data();
					size = packet->m_packetsize;
				}

				if(packet->ShouldSendCompressed()) {
					err = m_netIO->SendCompressed(m_playerData[pl]->m_id,
											packet->m_reliability,
											sbuf, size);
				} else {
					err = m_netIO->Send(m_playerData[pl]->m_id,
										packet->m_reliability,
										sbuf, size);
				}
				switch(err) {
					case NET_ERR_OK:
						if(packet->m_unitId != 0) {
							m_playerData[pl]->m_unitHash.Remove(
								packet->m_unitId);
						}

#ifdef _DEBUG
						LogSentPacket(sbuf[0], sbuf[1], static_cast<uint16>(size));
#endif
						break;
					case NET_ERR_WOULDBLOCK:

						isBusy = TRUE;
						if (m_playerData[pl])
						{
							// Reinsert for retry
							packet->AddRef();
							packetList->AddHead(packet);
						}
						break;
					case NET_ERR_INVALIDADDR:
						RemovePlayer(m_playerData[pl]->m_id);
						break;
					default:
						Assert(FALSE);
						break;
				}
				packet->Release();
			}

			if(!m_playerData[pl])
				continue;

#ifdef _DEBUG
			if(isBusy) {
				m_blockedPackets++;
			}
#endif
		}
	}
}


void Network::Init()
{
	NET_ERR err;
	if(!m_initialized) {
		m_transport = 5;


		err = m_netIO->EnumTransports();
		if(err == NET_ERR_OK) {
			err = m_netIO->SetTransport(m_transport);
		}

		if(err == NET_ERR_OK) {
			if(!profiledb_Get()->UseIPX()) {

				err = m_netIO->SetLobby(const_cast<char *>("california12.activision.com"));
			}
		}
		if(err == NET_ERR_OK) {
			m_initialized = TRUE;
		}
	}
}

void
Network::Host()
{
}

void
Network::EnumSessions()
{
}

void Network::Join(sint32 index )
{
}

void
Network::EnumTransport(NET_ERR result,
                       sint32 index,
                       const char* transname,
                       void* transdata)
{
	DPRINTF(k_DBG_NET, ("Transport %d: %s\n", index, transname));

	if(profiledb_Get()->UseIPX()) {
		if(strstr(transname, "wipx2d.dll")) {
			m_transport = index;
		}
	} else {
		if(strstr(transname, "winets2d.dll")) {

			m_transport = index;
		}
	}
}

void
Network::EnumSession(NET_ERR result,
                     sint32 index,
                     const char* sessionName,
                     void* sessionData)
{
	if(result == NET_ERR_OK) {
		DPRINTF(k_DBG_NET, ("Session %d: %s\n", index, sessionName));
		SessionData* sessionData = std::make_unique<SessionData>(index, sessionName).release();
		m_sessionList->AddTail(sessionData);
		if(strcmp(sessionName, m_sessionName) == 0) {
			m_sessionIndex = index;
		}
	} else if(result == NET_ERR_NOMORESESSIONS) {
		m_state = NETSTATE_SHOWSESSIONS;
	}
}

void Network::SessionReady(NET_ERR result,
						   void* session_data)
{
	DPRINTF(k_DBG_NET, ("Session Ready\n"));
	m_netIO->GetMyId(m_pid);
	if(m_iAmHost) {
		ProcessNewPlayer(m_pid);
	}
}

Packetizer*
Network::GetHandler(uint8* buf,
					uint16 size)
{
	Packetizer *handler = nullptr;
	switch(MAKE_CIV3_ID(buf[0], buf[1])) {
		case k_PACKET_CELL_ID:			handler = std::make_unique<NetCellData>().release(); break;
		case k_PACKET_CELL_LIST_ID:		handler = std::make_unique<NetCellList>().release(); break;
		case k_PACKET_UNIT_ID:			handler = std::make_unique<NetUnit>().release(); break;
		case k_PACKET_ACTION_ID:		handler = std::make_unique<NetAction>().release(); break;
		case k_PACKET_INFO_ID:			handler = std::make_unique<NetInfo>().release(); break;
		case k_PACKET_CITY_ID:			handler = std::make_unique<NetCity>().release(); break;
		case k_PACKET_DIFFICULTY_ID:    handler = std::make_unique<NetDifficulty>().release(); break;
		case k_PACKET_PLAYER_ID:        handler = std::make_unique<NetPlayer>().release(); break;
		case k_PACKET_TRADE_ROUTE_ID:   handler = std::make_unique<NetTradeRoute>().release(); break;
		case k_PACKET_TRADE_OFFER_ID:   handler = std::make_unique<NetTradeOffer>().release(); break;
		case k_PACKET_RAND_ID:          handler = std::make_unique<NetRand>().release(); break;
		case k_PACKET_TERRAIN_ID:       handler = std::make_unique<NetTerrainImprovement>().release(); break;
		case k_PACKET_INSTALLATION_ID:  handler = std::make_unique<NetInstallation>().release(); break;
		case k_PACKET_CHAT_ID:          handler = std::make_unique<NetChat>().release(); break;
		case k_PACKET_READINESS_ID:     handler = std::make_unique<NetReadiness>().release(); break;
		case k_PACKET_HAPPY_ID:         handler = std::make_unique<NetHappy>().release(); break;
		case k_PACKET_PLAYER_HAPPY_ID:  handler = std::make_unique<NetPlayerHappy>().release(); break;
		case k_PACKET_REPORT_ID:        handler = std::make_unique<NetReport>().release(); break;
		case k_PACKET_UNIT_MOVE_ID:     handler = std::make_unique<NetUnitMove>().release(); break;
		case k_PACKET_UNIT_ORDER_ID:    handler = std::make_unique<NetOrder>().release(); break;
		case k_PACKET_AGREEMENT_ID:     handler = std::make_unique<NetAgreement>().release(); break;
		case k_PACKET_CIVILIZATION_ID:  handler = std::make_unique<NetCivilization>().release(); break;
		case k_PACKET_CITY_NAME_ID:     handler = std::make_unique<NetCityName>().release(); break;
		case k_PACKET_DIP_PROPOSAL_ID:  handler = std::make_unique<NetDipProposal>().release(); break;
		case k_PACKET_DIP_RESPONSE_ID:  handler = std::make_unique<NetDipResponse>().release(); break;
		case k_PACKET_MESSAGE_ID:       handler = std::make_unique<NetMessage>().release(); break;
		case k_PACKET_CITY2_ID:         handler = std::make_unique<NetCity2>().release(); break;
		case k_PACKET_POLLUTION_ID:     handler = std::make_unique<NetPollution>().release(); break;
		case k_PACKET_CITY_BQ_ID:       handler = std::make_unique<NetCityBuildQueue>().release(); break;
		case k_PACKET_KEYS_ID:          handler = std::make_unique<NetKeys>().release(); break;
		case k_PACKET_GAME_SETTINGS_ID: handler = std::make_unique<NetGameSettings>().release(); break;
		case k_PACKET_NEW_ARMY_ID:      handler = std::make_unique<NetNewArmy>().release(); break;
		case k_PACKET_REMOVE_ARMY_ID:   handler = std::make_unique<NetRemoveArmy>().release(); break;
		case k_PACKET_CRC_ID:           handler = std::make_unique<NetCRC>().release(); break;
		case k_PACKET_ARMY_ID:          handler = std::make_unique<NetArmy>().release(); break;
		case k_PACKET_WONDER_TRACKER_ID: handler= std::make_unique<NetWonderTracker>().release(); break;
		case k_PACKET_ACHIEVEMENT_TRACKER_ID: handler= std::make_unique<NetAchievementTracker>().release(); break;
		case k_PACKET_VISION_ID:        handler = std::make_unique<NetVision>().release(); break;
		case k_PACKET_UNSEEN_CELL_ID:   handler = std::make_unique<NetUnseenCell>().release(); break;
		case k_PACKET_EXCLUSIONS_ID:    handler = std::make_unique<NetExclusions>().release(); break;
		case k_PACKET_RESOURCES_ID:     handler = std::make_unique<NetCityResources>().release(); break;
		case k_PACKET_UNIT_HP_ID:       handler = std::make_unique<NetUnitHP>().release(); break;
		case k_PACKET_CELL_UNIT_ORDER_ID: handler = std::make_unique<NetCellUnitOrder>().release(); break;
		case k_PACKET_ADD_PLAYER_ID:    handler = std::make_unique<NetAddPlayer>().release(); break;
		case k_PACKET_RESEARCH_ID:      handler = std::make_unique<NetResearch>().release(); break;
		case k_PACKET_GUID_ID:          handler = std::make_unique<NetGuid>().release(); break;
		case k_PACKET_STRENGTH_ID:      handler = std::make_unique<NetStrengths>().release(); break;
		case k_PACKET_FULL_STRENGTHS_ID: handler = std::make_unique<NetFullStrengths>().release(); break;
		case k_PACKET_NET_INFO_MESSAGE_ID: handler = std::make_unique<NetInfoMessage>().release(); break;
		case k_PACKET_ENDGAME_ID:       handler = std::make_unique<NetEndGame>().release(); break;
		case k_PACKET_WORMHOLE_ID:      handler = std::make_unique<NetWormhole>().release(); break;
		case k_PACKET_SET_PLAYER_GUID_ID: handler = std::make_unique<NetSetPlayerGuid>().release(); break;
		case k_PACKET_SET_LEADER_NAME_ID: handler = std::make_unique<NetSetLeaderName>().release(); break;
		case k_PACKET_WORLD_ID:           handler = std::make_unique<NetWorld>().release(); break;
		case k_PACKET_DIP_AGREEMENT_MATRIX_ID: handler = std::make_unique<NetAgreementMatrix>().release(); break;
		case k_PACKET_GROUP_REQUEST_ID: handler = std::make_unique<NetGroupRequest>().release(); break;
		case k_PACKET_UNGROUP_REQUEST_ID: handler = std::make_unique<NetUngroupRequest>().release(); break;
		case k_PACKET_SCORES_ID:          handler = std::make_unique<NetScores>().release(); break;

		case k_PACKET_FEAT_TRACKER_ID:	handler = std::make_unique<NetFeatTracker>().release(); break;

#ifdef _PLAYTEST
		case k_PACKET_CHEAT_ID:         handler = std::make_unique<NetCheat>().release(); break;
#endif
	}
	if(handler) {
		handler->AddRef();
	}
#ifdef _DEBUG
	LogPacket(buf[0], buf[1], size);
#endif
	return handler;
}

void Network::PacketReady(sint32 from,
                          uint8* buf,
                          sint32 size)
{
	if(m_deleting)
		return;
	if(buf[0] == k_CHUNK_HEAD &&
	   buf[1] == k_CHUNK_BODY) {

		DechunkList(from, &buf[2], size - 2);
	} else {
		Packetizer* handler = GetHandler(buf, static_cast<sint16>(size));
		Assert(handler != nullptr);
		if(handler) {
			handler->Unpacketize((uint16)from, buf, static_cast<sint16>(size));
			handler->Release();
		}

	}
}

void Network::AddPlayer(uint16 id,
                        char* name)
{
	if(m_iAmHost) {
		QueuePacketToAll(std::make_unique<NetAddPlayer>(id, name).release());
	}

	for(auto & i : m_playerData) {
		if(i && i->m_id == id) {
			DPRINTF(k_DBG_NET, ("AddPlayer(%d) but already have that player.\n",
			                    id));
			return;
		}
	}

	PointerList<PlayerData>::Walker walk(m_newPlayerList.get());
	while(walk.IsValid()) {
		if(walk.GetObj()->m_id == id) {
			// (Bug fix: format promised three conversions but only `id` and
			// `name` were supplied — the %d for the existing player's number
			// read whatever was next in the varargs. Supply the existing
			// player's id explicitly.)
			DPRINTF(k_DBG_NET, ("AddPlayer(%d), but player %d (%s) is already in the new player list\n",
			                    id, walk.GetObj()->m_id, name));
			return;
		}
		walk.Next();
	}
	DPRINTF(k_DBG_NET, ("Adding player %s (id=%d)\n", name, id));
	m_newPlayerList->AddTail(std::make_unique<PlayerData>(name, (uint16)id).release());
}

void Network::RemovePlayer(uint16 id)
{
	DPRINTF(k_DBG_NET, ("Removing player %d\n", id));

	if(id == m_pid) {
		SessionLost();
		// removing object bookkeeping
		m_gameObjects = std::make_unique<NetGameObj>();
	}

	if(m_deleting)
		return;

	sint32 index = IdToIndex(id);

	if(index < 0) {

		PointerList<PlayerData>::Walker walk(m_newPlayerList.get());
		while(walk.IsValid()) {
			if(walk.GetObj()->m_id == id) {
				walk.Remove();
				if(m_iAmHost && m_newPlayerList->IsEmpty() && !m_readyToStart) {
					ResetTurnEndsAt();
					c3_RemoveAbortMessage();
					SetReadyToStart(TRUE);
				}
				return;
			}
			walk.Next();
		}

		Assert(FALSE);
		return;
	}

	if(m_playerData[index]) {
		std::string name = m_playerData[index]->m_name;

		m_playerData[index].reset();

		if(m_iAmHost && player_Get(index) && !player_Get(index)->m_isDead) {
			SendLeftMessage(name.c_str(), index);
		}

	}

	if(index == selitem_Get()->GetCurPlayer()) {
		m_enactedDiplomaticRequests->Clear();
	}

	if(m_iAmHost) {

		if(player_Get(index)) {
			player_Get(index)->SetPlayerType(PLAYER_TYPE_ROBOT);
			if(index == selitem_Get()->GetCurPlayer()) {
				director_Get()->AddEndTurn();
			}

			SetRobotName(index);
		}
		if(player_Get(index) && !player_Get(index)->m_isDead)
			OpenPlayer(index);
		else
			ClosePlayer(index);

		SetMaxPlayers(CountOpenSlots() + CountTakenSlots());
	} else {

		if(player_Get(index)) {
			player_Get(index)->m_openForNetwork = TRUE;
		}
	}
	if(g_networkPlayersScreen) {
		g_networkPlayersScreen->UpdateData();
	}

}

void Network::SetToHost()
{
	if(!m_readyToStart) {
		civapp_Get()->PostQuitToLobbyAction();
		m_readyToStart = TRUE;
		return;
	}

	m_iAmHost = TRUE;
	m_iAmClient = FALSE;
	if(!m_deleting) {
		if(player_Get(m_playerIndex)) {
			player_Get(m_playerIndex)->SetPlayerType(PLAYER_TYPE_HUMAN);
			if(player_Get(selitem_Get()->GetCurPlayer())->IsHuman()) {
				SetMyTurn(TRUE);
			}
		}

		turn_Get()->NotifyBecameHost();

		sint32 p;
		for(p = 0; p < k_MAX_PLAYERS; p++) {
			if(p == m_playerIndex)
				continue;
			if(!player_Get(p))
				continue;

			if(!m_playerData[p]) {

				player_Get(p)->SetPlayerType(PLAYER_TYPE_ROBOT);
				SetRobotName(p);
			} else {


				player_Get(p)->SetPlayerType(PLAYER_TYPE_NETWORK);
				Resync(p);
			}
		}

		SendNewHostMessage(const_cast<MBCHAR *>(m_playerData[m_playerIndex]->m_name.c_str()),
						   m_playerIndex);
		SetMaxPlayers(CountOpenSlots() + CountTakenSlots());

		if(!player_Get(selitem_Get()->GetCurPlayer())
		||  player_Get(selitem_Get()->GetCurPlayer())->IsRobot()
		){
			DPRINTF(k_DBG_GAMESTATE, ("Set to host, cur player (%d) is robot, adding EndTurn\n", selitem_Get()->GetCurPlayer()));
			director_Get()->AddEndTurn();
		}
	}
}

void Network::ChangeHost(uint16 id)
{
	if(m_hostId != 0) {

		m_readyToStart = FALSE;
	}
	m_hostId = id;
	const char *str = stringdb_Get()->GetNameStr("NETWORK_WAITING_FOR_DATA");
	char nonConstStr[1024];
	if(str) {
		strlcpy(nonConstStr, str, sizeof(nonConstStr));
	} else {
		strlcpy(nonConstStr, "Waiting on data", sizeof(nonConstStr));
	}
	c3_AbortMessage(nonConstStr, k_UTILITY_PROGRESS_ABORT, network_AbortCallback );
}

void Network::SessionLost()
{
	if(slicengine_Get() && m_readyToStart) {
		auto so = std::make_unique<SlicObject>("355SessionLost");
		so->AddRecipient(m_playerIndex);
		slicengine_Get()->Execute(std::move(so));
	} else {
		civapp_Get()->PostQuitToLobbyAction();
	}
}

bool Network::ReadyForPackets()
{
	if(gevmanager_Get()->EventsPending())
		return false;
	else
		return true;
}

void Network::SetReady(uint16 id)
{
	sint32 i;
	sint32 n;

	sint32 index = IdToIndex(id);
	if(index < 0)
		return;

	Assert(m_playerData[index]);
	if(!m_playerData[index])
		return;

	PlayerData *player = m_playerData[index].get();
	m_playerData[index]->m_ready = TRUE;

	MapPoint* size = world_Get()->GetSize();

	QueuePacket(player->m_id, std::make_unique<NetCRC>().release());

	QueuePacket(player->m_id, std::make_unique<NetGameSettings>(size->x, size->y,
						      profiledb_Get()->GetNPlayers(),
						      m_gameStyle,
						      m_unitMovesPerSlice,
						      m_totalStartTime,
						      m_turnStartTime,
						      m_extraTimePerCity).release());

	NetInfo* netInfo = std::make_unique<NetInfo>(NET_INFO_CODE_PLAYER_INDEX,
				       index, player->m_id).release();
	QueuePacket(player->m_id, netInfo);

	SetupPlayerFromNSPlayerInfo(player->m_id, index);

	for(i = 0; i < k_MAX_PLAYERS; i++) {
		if(!player_Get(i)) continue;
		if(m_playerData[i] && i != index) {
			NetInfo* netInfo2 = std::make_unique<NetInfo>(NET_INFO_CODE_PLAYER_INDEX,
											i, m_playerData[i]->m_id).release();
			QueuePacket(player->m_id, netInfo2);
		}
	}

	if(player->m_id == m_pid) {
		m_playerIndex = index;

		if(director_Get()) {
			director_Get()->NextPlayer();
		}
		if(tiledmap_Get()) {
			tiledmap_Get()->NextPlayer();
			tiledmap_Get()->CopyVision();
			tiledmap_Get()->InvalidateMix();
			tiledmap_Get()->InvalidateMap();
			tiledmap_Get()->Refresh();
		}
		if(radar_map_Get()) {
			radar_map_Get()->Update();
		}

		MainControlPanel::UpdateCityList();

		return;
	}

#define PROGRESS(x) { QueuePacket(player->m_id, std::make_unique<NetInfo>(NET_INFO_CODE_PROGRESS, x).release()); }
#define CPROGRESS(x) { chunkPackets.AddTail(std::make_unique<NetInfo>(NET_INFO_CODE_PROGRESS, x).release()); }

#define k_CELL_LIST_CELL_SIZE 6

	PROGRESS(0);

	NetCellList* cellList = nullptr;

	double percentMap = 0;

	PointerList<Packetizer> chunkPackets;

	sint32 x;
	sint32 y;
	for(x = 0; x < size->x; x++) {
		for(y = 0; y < size->y; y++) {
			if(!cellList) {
				cellList = std::make_unique<NetCellList>(x,y).release();
			}
			cellList->m_cells++;


			if(cellList->m_cells * k_CELL_LIST_CELL_SIZE >= 220) {
				chunkPackets.AddTail(cellList);

				cellList = nullptr;
			}
		}
		if(x < size->x - 1) {
			percentMap = double(x) / double(size->x);
			CPROGRESS(sint32(double(50) * percentMap));
		}
	}
	if(cellList && cellList->m_cells > 0) {
		chunkPackets.AddTail(cellList);

	}

	ChunkList(player->m_id, &chunkPackets);
	Assert(!chunkPackets.GetHead());

	QueuePacket(player->m_id, std::make_unique<NetInfo>(NET_INFO_CODE_MAP_DONE, 0).release());
	PROGRESS(50);

	uint8 p;
	for(p = 0; p < k_MAX_PLAYERS; p++) {
		if(!player_Get(p)) continue;
		chunkPackets.AddTail(std::make_unique<NetPlayer>(player_Get(p)).release());
		chunkPackets.AddTail(std::make_unique<NetResearch>(player_Get(p)->m_advances.get()).release());
		chunkPackets.AddTail(std::make_unique<NetDifficulty>(player_Get(p)->GetDifficulty()).release());
		Assert(civilisationpool_Get()->IsValid(*player_Get(p)->m_civilisation));
		if(civilisationpool_Get()->IsValid(*player_Get(p)->m_civilisation)) {
			chunkPackets.AddTail(std::make_unique<NetCivilization>(player_Get(p)->m_civilisation->AccessData()).release());
		}
		sint32 r;
		sint32 n = player_Get(p)->m_strengths->m_strengthRecords[0].Num();
		for(r = 0; r < n; r += 100) {
			chunkPackets.AddTail(std::make_unique<NetFullStrengths>(p, r, ((r + 99) < n) ? (r+99) : (n - 1)).release());
		}
	}

	ChunkList(player->m_id, &chunkPackets);
	Assert(!chunkPackets.GetHead());

	PROGRESS(55);
	QueuePacket(player->m_id, std::make_unique<NetInfo>(NET_INFO_CODE_START_UNITS, 0).release());

	sint32 numPlayers = 0;
	for(p = 0; p < k_MAX_PLAYERS; p++) {
		if(player_Get(p)) {
			numPlayers++;
		}
	}
	double percentPerPlayer = 1.0 / double(numPlayers);
	double playerPercent = 0;

	for(p = 0; p < k_MAX_PLAYERS; p++) {
		if(!player_Get(p)) continue;

		chunkPackets.AddTail(std::make_unique<NetSetPlayerGuid>(p).release());

		UnitDynamicArray *unitList = player_Get(p)->GetAllCitiesList();
		for(n = 0; n < unitList->Num(); n++) {
			UnitData * unitData =
                unitpool_Get()->GetUnit(unitList->Get(n).m_id);

			chunkPackets.AddTail(std::make_unique<NetUnit>(unitData).release());

			chunkPackets.AddTail(std::make_unique<NetCity>(unitData, TRUE).release());
			chunkPackets.AddTail(std::make_unique<NetCityName>(unitData->GetCityData()).release());
			chunkPackets.AddTail(std::make_unique<NetCity2>(unitData->GetCityData(), TRUE).release());
			chunkPackets.AddTail(std::make_unique<NetCityBuildQueue>(unitData->GetCityData()).release());
			chunkPackets.AddTail(std::make_unique<NetHappy>(unitList->Get(n),
									 unitData->GetCityData()->GetHappy(),
									 TRUE).release());





		}

		unitList = player_Get(p)->GetAllUnitList();
		for(n = 0; n < unitList->Num(); n++) {
			chunkPackets.AddTail(std::make_unique<NetUnit>(unitpool_Get()->GetUnit(unitList->Get(n).m_id)).release());
		}

		for(n = 0; n < player_Get(p)->m_all_armies->Num(); n++) {
			Army army = player_Get(p)->m_all_armies->Access(n);
			chunkPackets.AddTail(std::make_unique<NetArmy>(armypool_Get()->AccessArmy(army)).release());

			chunkPackets.AddTail(std::make_unique<NetInfo>(NET_INFO_CODE_ADD_ARMY,
												  p,
												  CAUSE_NEW_ARMY_INITIAL,
												  player_Get(p)->m_all_armies->Access(n)).release());

			sint32 m;
			for(m = 0; m < army.NumOrders(); m++) {
				const Order *order = army.GetOrder(m);
				Assert(order);
				if(order) {
					chunkPackets.AddTail(std::make_unique<NetOrder>(p,
														   army,
														   order->m_order,
														   order->m_path.get(),
														   order->m_point,
														   order->m_argument,
														   order->m_eventType).release());
				}
			}
		}


		UnitDynamicArray* traderList = player_Get(p)->GetTradersList();
		for(n = 0; n < traderList->Num(); n++) {
			UnitData * unitData =
                unitpool_Get()->GetUnit(traderList->Get(n).m_id);
			chunkPackets.AddTail(std::make_unique<NetUnit>(unitData).release());
		}

		n = player_Get(p)->m_terrainImprovements->Num();
		for(i = 0; i < n; i++) {
			chunkPackets.AddTail(std::make_unique<NetTerrainImprovement>(player_Get(p)->m_terrainImprovements->Access(i).AccessData()).release());
		}

		n = player_Get(p)->m_allInstallations->Num();
		for(i = 0; i < n; i++) {
			chunkPackets.AddTail(std::make_unique<NetInstallation>(player_Get(p)->m_allInstallations->Access(i).AccessData()).release());
		}

		chunkPackets.AddTail(std::make_unique<NetInfo>(NET_INFO_CODE_GOLD,
											  p, player_Get(p)->m_gold->GetLevel()).release());

		chunkPackets.AddTail(std::make_unique<NetReadiness>(player_Get(p)->m_readiness.get()).release());

		chunkPackets.AddTail(std::make_unique<NetPlayerHappy>((uint8)p, player_Get(p)->m_global_happiness.get(), TRUE).release());

		chunkPackets.AddTail(std::make_unique<NetCivilization>(player_Get(p)->m_civilisation->AccessData()).release());





		sint32 y;
		for(y = 0; y < world_Get()->GetYHeight(); y += k_VISION_STEP) {
			chunkPackets.AddTail(std::make_unique<NetVision>(p, static_cast<sint16>(y), k_VISION_STEP).release());
		}
		static DynamicArray<UnseenCellCarton> array;
		player_Get(p)->m_vision->GetUnseenCellList(array);
		n = array.Num();
		for(i = 0; i < n; i++) {
			chunkPackets.AddTail(std::make_unique<NetUnseenCell>(array[i].m_unseenCell,
                                                               p).release());
		}

		chunkPackets.AddTail(std::make_unique<NetEndGame>(p).release());

		playerPercent += percentPerPlayer;
		CPROGRESS(55 + static_cast<uint32>(playerPercent * 30));
		ChunkList(player->m_id, &chunkPackets);
		Assert(!chunkPackets.GetHead());
	}

	PROGRESS(85);

	chunkPackets.AddTail(std::make_unique<NetWormhole>().release());

	chunkPackets.AddTail(std::make_unique<NetPollution>().release());

	chunkPackets.AddTail(std::make_unique<NetWonderTracker>().release());
	chunkPackets.AddTail(std::make_unique<NetAchievementTracker>().release());
	chunkPackets.AddTail(std::make_unique<NetFeatTracker>().release());
	chunkPackets.AddTail(std::make_unique<NetExclusions>().release());

	chunkPackets.AddTail(std::make_unique<NetWorld>().release());
	n = tradepool_Get()->m_all_routes->Num();
	for(i = 0; i < n; i++) {
		chunkPackets.AddTail(std::make_unique<NetTradeRoute>(tradepool_Get()->m_all_routes->Access(i).AccessData(), false).release());
	}

	PROGRESS(90);

	for(x = 0; x < world_Get()->GetXWidth(); x++) {
		for(y = 0; y < world_Get()->GetYHeight(); y++) {
			if(world_Get()->GetCell(x, y)->GetNumUnits() >= 2) {
				chunkPackets.AddTail(std::make_unique<NetCellUnitOrder>(x, y).release());
			}
		}
	}

	chunkPackets.AddTail(std::make_unique<NetInfo>(NET_INFO_CODE_END_UNITS,
										  unitpool_Get()->HackGetKey(),
										  armypool_Get()->HackGetKey()).release());

	PROGRESS(95);

	chunkPackets.AddTail(std::make_unique<NetAgreementMatrix>().release());

	chunkPackets.AddTail(std::make_unique<NetRand>().release());

	chunkPackets.AddTail(std::make_unique<NetKeys>().release());
	chunkPackets.AddTail(std::make_unique<NetInfo>(NET_INFO_CODE_YEAR,
                                         turn_Get()->GetRound(),
                                         turn_Get()->GetYear()).release());

	ChunkList(player->m_id, &chunkPackets);
	Assert(!chunkPackets.GetHead());

	if(m_setupMode) {
		sint32 index = IdToIndex(player->m_id);
		MapPoint center = player_Get(index)->m_setupCenter;
		QueuePacket(player->m_id, std::make_unique<NetInfo>(NET_INFO_CODE_SET_SETUP_MODE,
	                                              m_setupMode).release());
		QueuePacket(player->m_id, std::make_unique<NetInfo>(NET_INFO_CODE_SET_SETUP_AREA,
						      index,
						      center.x, center.y,
						      player_Get(index)->m_setupRadius).release());
		QueuePacket(player->m_id, std::make_unique<NetInfo>(NET_INFO_CODE_POWER_POINTS,
						      index,
						      player_Get(index)->m_powerPoints).release());
	}

	PROGRESS(100);

	SendJoinedMessage(const_cast<MBCHAR *>(player->m_name.c_str()), index);
	QueuePacket(player->m_id, std::make_unique<NetInfoMessage>(NET_MSG_PLAYER_JOINED,
							     const_cast<MBCHAR *>(m_playerData[m_playerIndex]->m_name.c_str()),
							     m_playerIndex).release());

	QueuePacket(player->m_id, std::make_unique<NetInfo>(NET_INFO_CODE_SET_TURN,
					      selitem_Get()->GetCurPlayer()).release());

	if(index == selitem_Get()->GetCurPlayer()) {
		player->m_ackBeginTurn = TRUE;
	}

	if(m_readyToStart) {
		QueuePacket(player->m_id, std::make_unique<NetInfo>(NET_INFO_CODE_ALL_PLAYERS_READY).release());
	}
}

void Network::SyncRand()
{
	if(m_iAmHost) {
		QueuePacketToAll(std::make_unique<NetRand>().release());
	}
}

void Network::SyncRand(sint32 index)
{
	if(m_iAmHost) {
		QueuePacket(IndexToId(index), std::make_unique<NetRand>().release());
	}
}

void
Network::Enqueue(UnitData* unit)
{
	if(m_iAmHost) {
		NetUnit* netUnit = std::make_unique<NetUnit>(unit).release();
		QueuePacketToAll(netUnit);
	}
}

void Network::Enqueue(UnitData *unit, Unit useActor)
{
	if(m_iAmHost) {
		NetUnit *netUnit = std::make_unique<NetUnit>(unit, useActor).release();
		QueuePacketToAll(netUnit);
	}
}

void
Network::MoveUnit(UnitData *data, const MapPoint &pnt)
{
	if(m_iAmHost) {
		Block(data->GetOwner());
		QueuePacketToAll(std::make_unique<NetUnitMove>(Unit(data->m_id), pnt).release());
		Unblock(data->GetOwner());
	}
}

void
Network::Enqueue(UnitData* unit, CityData* city, BOOL isInitial)
{
	if(m_iAmHost) {
		NetCity* netCity = std::make_unique<NetCity>(unit, isInitial).release();
		QueuePacketToAll(netCity);

		NetCity2* netCity2 = std::make_unique<NetCity2>(city, static_cast<uint8>(isInitial)).release();
		QueuePacketToAll(netCity2);

		Unit u(unit->m_id);
		QueuePacketToAll(std::make_unique<NetHappy>(u, city->GetHappy(), isInitial).release());

		Block(city->GetOwner());
		QueuePacketToAll(std::make_unique<NetCityBuildQueue>(city).release());
		Unblock(city->GetOwner());
	}
}

void Network::SendCityName(CityData *city)
{
	if(m_iAmHost) {
		QueuePacketToAll(std::make_unique<NetCityName>(city).release());
	} else {
		QueuePacket(m_hostId, std::make_unique<NetCityName>(city).release());
	}
}

void
Network::AddNewUnit(sint32 owner, Unit u)
{
	if(m_playerData[owner] && player_Get(owner)->IsNetwork()) {
		m_playerData[owner]->m_createdUnits.Insert(u);
	}
}

void
Network::Enqueue(Cell* cell,
				 sint32 x, sint32 y)
{
	if(m_iAmHost) {

		NetCellData* cellData = std::make_unique<NetCellData>(cell, x, y).release();
		QueuePacketToAll(cellData);
	}
}

void
Network::Enqueue(TradeRouteData* tradeRoute)
{
	if(m_iAmHost) {
		NetTradeRoute* netTradeRoute = std::make_unique<NetTradeRoute>(tradeRoute, true).release();
		QueuePacketToAll(netTradeRoute);
	}
}

void
Network::Enqueue(TradeOfferData* offer)
{
	if(m_iAmHost) {
		NetTradeOffer* netTradeOffer = std::make_unique<NetTradeOffer>(offer).release();
		QueuePacketToAll(netTradeOffer);
	}
}

void
Network::Enqueue(NetInfo* netInfo)
{
	QueuePacketToAll(netInfo);
}

void
Network::Enqueue(TerrainImprovementData *data)
{
	if(m_iAmHost) {
		QueuePacketToAll(std::make_unique<NetTerrainImprovement>(data).release());
	}
}

void
Network::Enqueue(InstallationData *data)
{
	if(m_iAmHost) {
		QueuePacketToAll(std::make_unique<NetInstallation>(data).release());
	}
}

void
Network::Enqueue(Gold *gold)
{
	if(m_iAmHost) {
		QueuePacketToAll(std::make_unique<NetInfo>(NET_INFO_CODE_GOLD,
									 gold->GetOwner(), gold->GetLevel()).release());
	}
}

void
Network::Enqueue(MilitaryReadiness *readiness)
{
	if(m_iAmHost) {
		QueuePacketToAll(std::make_unique<NetReadiness>(readiness).release());
	}
}

void
Network::Enqueue(uint8 owner, PlayerHappiness *hap)
{
	if(m_iAmHost) {
		QueuePacketToAll(std::make_unique<NetPlayerHappy>(owner, hap, FALSE).release());
	}
}

void
Network::Enqueue(AgreementData *data)
{
	if(m_iAmHost) {
		QueuePacketToAll(std::make_unique<NetAgreement>(data).release());
	}
}

void
Network::MakeAgreement(Agreement &a)
{
	if(m_iAmClient) {
		QueuePacket(m_hostId, std::make_unique<NetClientAgreement>(a.AccessData()).release());
	}
}

void
Network::Enqueue(CivilisationData *data)
{
	if(m_iAmHost) {
		QueuePacketToAll(std::make_unique<NetCivilization>(data).release());
	}
}

void
Network::Enqueue(DiplomaticRequestData *data)
{
}

void Network::SendDiplomaticRequest(DiplomaticRequestData *data)
{
}

void
Network::Enqueue(MessageData *data)
{
	if(m_iAmHost) {
		QueuePacketToAll(std::make_unique<NetMessage>(data).release());
	}
}

void
Network::EnqueuePollution()
{
	if(m_iAmHost) {
		QueuePacketToAll(std::make_unique<NetPollution>().release());
	}
}

void
Network::Enqueue(NetOrder *order)
{
	PacketManager	l_AutoRelease(order);

	if(m_iAmHost) {
		QueuePacketToAll(order);
	}
}

void
Network::Enqueue(ArmyData *armyData)
{
	if(m_iAmHost) {
		QueuePacketToAll(std::make_unique<NetArmy>(armyData).release());
	}
}

void
Network::AddNewArmy(sint32 owner, const Army &army)
{
	if(m_playerData[owner] && player_Get(owner)->IsNetwork()) {
		m_playerData[owner]->m_createdArmies.Insert(army);
	}
}

void
Network::Enqueue(CityData *cd)
{
	if(m_iAmHost) {
		QueuePacketToAll(std::make_unique<NetCityBuildQueue>(cd).release());
	}
}

void
Network::SendBuildQueue(CityData *cd)
{
	if(m_hostId == 0) {
		m_netIO->GetHostId(m_hostId);
	}

	QueuePacket(m_hostId, std::make_unique<NetCityBuildQueue>(cd).release());
}

void
Network::SendMessage(MessageData *data)
{
	if(m_hostId == 0) {
		m_netIO->GetHostId(m_hostId);
	}
	QueuePacket(m_hostId, std::make_unique<NetMessage>(data).release());
}

#ifdef _PLAYTEST
void
Network::SendCheat(NetCheat *netCheat)
{
	if(m_hostId == 0) {
		m_netIO->GetHostId(m_hostId);
	}
	QueuePacket(m_hostId, netCheat);
}
#endif

void
Network::SendAction(NetAction* netAction)
{
	if(m_hostId == 0) {
		m_netIO->GetHostId(m_hostId);
	}

	QueuePacket(m_hostId, netAction);
}

void
Network::SendActionBookmark(NetAction* netAction)
{
	if(m_hostId == 0) {
		m_netIO->GetHostId(m_hostId);
	}

	QueuePacketBookmark(m_hostId, netAction);
}

void
Network::SendOrder(sint32 owner, const Army &army, UNIT_ORDER_TYPE o,
		   Path *a_path, const MapPoint &point, sint32 arg,
		   GAME_EVENT event)
{
	if(m_hostId == 0) {
		m_netIO->GetHostId(m_hostId);
	}

	QueuePacket(m_hostId, std::make_unique<NetOrder>(owner, army,
					   o, a_path, point,
					   arg, event).release());
}

void Network::SendToServer(Packetizer *packet)
{
	if(m_hostId == 0) {
		m_netIO->GetHostId(m_hostId);
	}
	QueuePacket(m_hostId, packet);
}

void
Network::QueuePacket(uint16 id,
		     Packetizer* packet)
{
	PacketManager l_AutoRelease(packet);

	if(m_iAmClient && m_waitingOnResync)
		return;

	sint32 index = IdToIndex(id);
	Assert(m_playerData[index]);
	if(!m_playerData[index])
		return;


	packet->m_destination = (sint32)id;

	if(m_playerData[index]->m_blocked > 0)
		return;


	if(packet->m_unitId == 0 ||
	   !m_playerData[index]->m_unitHash.IsPresent(packet->m_unitId)) {

		m_playerData[index]->m_packetList->AddTail(packet);
		packet->PacketizeAndSave();

		if(packet->m_unitId != 0) {
			m_playerData[index]->m_unitHash.Add(packet->m_unitId);
		}
		packet->AddRef();
	}
	ProcessSends();
}

void
Network::QueuePacketBookmark(uint16 id,
							 Packetizer* packet)
{
	PacketManager	l_AutoRelease(packet);
	sint32 index = IdToIndex(id);
	Assert(m_playerData[index]);





	if(m_playerData[index]->m_blocked > 0)
		return;

	if(m_playerData[index]->m_frozen <= 0) {
		BOOL NetworkProgrammerSmokingCrackPleaseIgnore = FALSE;
		Assert(NetworkProgrammerSmokingCrackPleaseIgnore);
		QueuePacket(id, packet);
		return;
	}


	packet->m_destination = (sint32)id;


	if(packet->m_unitId == 0 ||
	   !m_playerData[index]->m_unitHash.IsPresent(packet->m_unitId)) {

		m_playerData[index]->m_packetList->InsertAt(m_playerData[index]->m_bookmarks->GetTail(), packet);
		packet->PacketizeAndSave();

		if(packet->m_unitId != 0) {
			m_playerData[index]->m_unitHash.Add(packet->m_unitId);
		}
		packet->AddRef();
	}
}

void
Network::QueuePacketToAll(Packetizer* packet)
{
	PacketManager	l_AutoRelease(packet);

	if(!player_arr_Get()) {

		return;
	}

	for(sint32 pl = 0; pl < k_MAX_PLAYERS; pl++) {
		if(!player_Get(pl)) continue;

		if(m_playerData[pl] &&
		   player_Get(pl)->IsNetwork()) {





			if(m_playerData[pl]->m_blocked > 0)
				continue;


			if(packet->m_unitId == 0 ||
			   !m_playerData[pl]->m_unitHash.IsPresent(packet->m_unitId)) {

				m_playerData[pl]->m_packetList->AddTail(packet);
				packet->PacketizeAndSave();
				if(packet->m_unitId != 0) {

					m_playerData[pl]->m_unitHash.Add(packet->m_unitId);
				}
				packet->AddRef();
			}
		}
	}
	ProcessSends();

}

void Network::Freeze(uint16 id)
{
	if(id == 0xffff) {
		Assert(IsClient());
		id = m_hostId;
	}
	m_playerData[IdToIndex(id)]->m_frozen++;
}

void Network::Unfreeze(uint16 id)
{
	if(id == 0xffff) {
		Assert(IsClient());
		id = m_hostId;
	}
	DPRINTF(k_DBG_NET, ("Unbookmarking %d\n", IdToIndex(id)));
	m_playerData[IdToIndex(id)]->m_frozen--;
	m_playerData[IdToIndex(id)]->m_bookmarks->RemoveTail();
	Assert(m_playerData[IdToIndex(id)]->m_frozen >= 0);
}

void Network::Bookmark(uint16 id)
{
	if(id == 0xffff) {
		Assert(IsClient());
		id = m_hostId;
	}
	DPRINTF(k_DBG_NET, ("Bookmarking %d\n", IdToIndex(id)));

	m_playerData[IdToIndex(id)]->m_frozen++;
	m_playerData[IdToIndex(id)]->m_bookmarks->AddTail(
		m_playerData[IdToIndex(id)]->m_packetList->GetTailNode());
}

void Network::Block(sint32 index)
{

	if(m_playerData[index])
		m_playerData[index]->m_blocked++;
}

void Network::Unblock(sint32 index)
{

	if(m_playerData[index]) {
		m_playerData[index]->m_blocked--;
		Assert(m_playerData[index]->m_blocked >= 0);
	}
}


sint32
Network::IdToIndex(uint16 id)
{
	for(auto & i : m_playerData) {
		if(i && i->m_id == id) {
			return i->m_index;
		}
	}

#ifdef _DEBUG
	PointerList<PlayerData>::Walker walk(m_newPlayerList.get());
	while(walk.IsValid()) {
		if(walk.GetObj()->m_id == id) {
			return -1;
		}
		walk.Next();
	}
	Assert(FALSE);
#endif
	return -1;
}


uint16
Network::IndexToId(sint32 index)
{
	for(auto & i : m_playerData) {
		if(i && i->m_index == index) {
			return i->m_id;
		}
	}
	Assert(FALSE);
	return 0xffff;
}

void Network::SetPlayerIndex(sint32 index, uint16 id)
{
	if(id == m_pid) {
		m_playerIndex = index;
	}
	m_totalTimeUsed = 0;

	PointerList<PlayerData>::Walker walk(m_newPlayerList.get());
	while(walk.IsValid()) {
		PlayerData* pd = (PlayerData*)walk.GetObj();
		if(pd->m_id == id) {
			pd->m_index = index;
			m_playerData[index].reset(pd);
			m_playerData[index]->m_ready = TRUE;
			walk.Remove();
			break;
		}
		walk.Next();
	}

	if(!m_playerData[index]) {
		AddPlayer(id, const_cast<char *>("anotherclient"));
		m_playerData[index].reset(m_newPlayerList->RemoveTail());
		m_playerData[index]->m_ready = TRUE;
	}
	m_playerData[index]->m_id = id;




}







BOOL
Network::DeadUnit(sint32 unitId)
{
	return m_deadUnitList->IsPresent(unitId);
}

void
Network::AddDeadUnit(sint32 unitId)
{
	Assert(!m_deadUnitList->IsPresent(unitId));
	if(!m_deadUnitList->IsPresent(unitId)) {
		m_deadUnitList->Add(unitId);
	}
}

void
Network::RemoveDeadUnit(sint32 unitId)
{
	m_deadUnitList->Remove(unitId);
}

void
Network::ClearDeadUnits()
{
	m_deadUnitList->Clear();
}

void Network::AddCreatedObject(GameObj *obj)
{
	m_gameObjects->AddCreated(obj);
}

void Network::HandleObjectACK(uint32 id)
{
	m_gameObjects->ACKObject(id);
}

void Network::HandleObjectNAK(uint32 myId, uint32 realId)
{
	m_gameObjects->NAKObject(myId, realId);
}

void Network::CheckReceivedObject(uint32 id)
{
	m_gameObjects->CheckReceived(id);
}

sint32 Network::FindEmptySlot(PlayerData *player, uint16 id)
{
	sint32 newslot = -1;

	NSPlayerInfo *nspi = GetNSPlayerInfoByID(id);

	sint32 p;
	for(p = 1; p < k_MAX_PLAYERS; p++) {
		if(player_Get(p) && player_Get(p)->m_networkId == id &&
		   !m_playerData[p]) {
			return p;
		}

		if(nspi && player_Get(p) &&
		   player_Get(p)->m_civilisation->GetCivilisation() == nspi->m_civ &&
		   !m_playerData[p]) {
			return p;
		}
	}

	GUID zeroGuid;
	memset((uint8 *)&zeroGuid, 0, sizeof(zeroGuid));

	for(p = 1; p < k_MAX_PLAYERS; p++) {
		if(!player_Get(p)) continue;

		if(!m_playerData[p]) {
			if(newslot < 0 || player_Get(p)->IsRobot()) {
					newslot = p;
			}




			if(profiledb_Get()->NoHumanPlayersOnHost() && player->m_id == m_pid &&
			   newslot >= 0 && player_Get(newslot)->IsRobot()) {
				break;
			}
			if(   player_Get(newslot)
			&& (  player_Get(newslot)->IsHuman()
			||  ( player_Get(newslot)->m_openForNetwork
			&&    memcmp(&player_Get(newslot)->m_networkGuid, &zeroGuid, sizeof(GUID)) == 0
			    )
			   )
			  )
			{
				break;
			}
		}
	}
	if (newslot < 0 || newslot >= k_MAX_PLAYERS || !player_Get(newslot))
		return -1;

	return newslot;
}

sint32 Network::FindOldSlot(PlayerData *player, uint16 id)
{
	sint32 p;

	GUID zeroGuid;
	memset((uint8 *)&zeroGuid, 0, sizeof(zeroGuid));

	for(p = 1; p < k_MAX_PLAYERS; p++) {
		if(!player_Get(p))
			continue;
		if(!player_Get(p)->m_openForNetwork)
			continue;
		if(memcmp(&player_Get(p)->m_networkGuid, &zeroGuid, sizeof(GUID)) == 0)
			continue;

		if(memcmp(&player_Get(p)->m_networkGuid, &player->m_guid, sizeof(GUID)) == 0) {
			if(m_playerData[p]) {

				return -1;
			}
			return p;
		}
	}
	return -1;
}

void
Network::ProcessNewPlayer(uint16 id)
{
	m_processingNewPlayers = TRUE;
	sint32 newslot = -1;
	BOOL found = FALSE;

	PointerList<PlayerData>::Walker walk(m_newPlayerList.get());

	while(walk.IsValid() && !found) {
		PlayerData* player = walk.GetObj();
		if(player->m_id != id) {
			walk.Next();
			continue;
		}
		NSPlayerInfo *nspi = GetNSPlayerInfoByID(player->m_id);
		if(nspi)
			player->m_group = nspi->m_group;

		GUID zeroGuid;
		memset((uint8 *)&zeroGuid, 0, sizeof(zeroGuid));

		if(id == m_pid) {
			player->m_guid = m_guid;
		}

		newslot = FindOldSlot(player, id);
		if(newslot < 0) {
			newslot = FindEmptySlot(player, id);
		}

		Assert(newslot >= 0);
		if(newslot < 0) {
			m_netIO->KickPlayer(id);
			return;
		}

		if(memcmp(&player_Get(newslot)->m_networkGuid, &zeroGuid, sizeof(GUID)) &&
		   memcmp(&player_Get(newslot)->m_networkGuid, &player->m_guid, sizeof(GUID))) {

			SendWrongPlayerJoinedMessage(const_cast<MBCHAR *>(player->m_name.c_str()), newslot);
		} else {
			player_Get(newslot)->m_networkGuid = player->m_guid;
			player_Get(newslot)->m_networkGroup = player->m_group;
			ClosePlayer(newslot);
		}
		found = TRUE;


		m_playerData[newslot].reset(player);

		if(m_iAmHost) {
			SetMaxPlayers(CountOpenSlots() + CountTakenSlots());
		}


		Assert(!player_Get(newslot)->IsNetwork());


//		sint32 oldVisPlayer = selitem_Get()->GetVisiblePlayer();
		if(player->m_id != m_pid) {
			player_Get(newslot)->SetPlayerType(PLAYER_TYPE_NETWORK);
		} else {
			m_playerIndex = newslot;
			player_Get(m_playerIndex)->m_networkId = m_pid;

			if(newslot == selitem_Get()->GetCurPlayer()) {
				SetMyTurn(TRUE);
			}
			if(director_Get()) {
				director_Get()->NextPlayer();
			}
			if(tiledmap_Get()) {
				tiledmap_Get()->NextPlayer();
				tiledmap_Get()->CopyVision();
				tiledmap_Get()->InvalidateMix();
				tiledmap_Get()->InvalidateMap();
				tiledmap_Get()->Refresh();
			}
			if(radar_map_Get()) {
				radar_map_Get()->Update();
			}
		}

		player->m_index = newslot;

		NetInfo* netInfo = std::make_unique<NetInfo>(NET_INFO_CODE_PLAYER_INDEX,
									   newslot, player->m_id).release();
		QueuePacketToAll(netInfo);
		QueuePacketToAll(std::make_unique<NetSetPlayerGuid>(newslot).release());
		if(!player->m_name.empty()) {
			player_Get(newslot)->m_civilisation->AccessData()->SetLeaderName(player->m_name.c_str());
			QueuePacketToAll(std::make_unique<NetSetLeaderName>(newslot).release());
			if(g_networkPlayersScreen) {
				g_networkPlayersScreen->UpdateData();
			}
		}
		if(player->m_id == m_pid) {
			SetupPlayerFromNSPlayerInfo(m_pid, m_playerIndex);
		}
		walk.Remove();

		SetReady(id);
		if(!m_readyToStart) {
			if(m_newPlayerList->IsEmpty()) {
				ResetTurnEndsAt();

				c3_RemoveAbortMessage();
				SetReadyToStart(TRUE);
			}
		}
	}
	m_processingNewPlayers = FALSE;
	Assert(found && newslot >= 0);

	if(g_networkPlayersScreen) {
		g_networkPlayersScreen->UpdateData();
	}
}

void Network::AddChatText(const MBCHAR *str, sint32 len, uint8 from, BOOL priv)
{


	if(!priv)
		snprintf(m_chatStr, sizeof(m_chatStr), "[%s] ",
				((from == 0) ? stringdb_Get()->GetNameStr("NETWORK_SENDER_SYSTEM") :
				 (player_Get(from) ? (player_Get(from)->m_civilisation->GetLeaderName()) : ".")));
	else
		snprintf(m_chatStr, sizeof(m_chatStr), "[P] (%s) ", ((from == 0) ?
										 (stringdb_Get()->GetNameStr("NETWORK_SENDER_SYSTEM")) :
										 (player_Get(from) ?
										  (player_Get(from)->m_civilisation->GetLeaderName()) :
										  ("."))));

	strncat(m_chatStr, str, sizeof(m_chatStr) - strlen(m_chatStr) - 1);

	if (chatbox_Get()) {
		MBCHAR *c;

		for(c = m_chatStr; *c; c++) {
			if(*c == '<')
				*c = '(';
			else if(*c == '>')
				*c = ')';
		}

		chatbox_Get()->AddLine(from, m_chatStr);
	}

	m_chatList->AddLine((sint32)from, m_chatStr);
}

void Network::SendChatText(MBCHAR *str, sint32 len)
{
	if(str[0] == '/') {
		if(stricmp(str, "/rules") == 0) {
			char buf[1024];
			snprintf(buf, sizeof(buf), "Difficulty: %d", gamesettings_Get()->GetDifficulty());
			chatbox_Get()->AddLine(m_playerIndex, buf);

			snprintf(buf, sizeof(buf), "Risk: %d", gamesettings_Get()->GetRisk());
			chatbox_Get()->AddLine(m_playerIndex, buf);

			snprintf(buf, sizeof(buf), "Pollution: %s", gamesettings_Get()->GetPollution() ? "On" : "Off");
			chatbox_Get()->AddLine(m_playerIndex, buf);

			snprintf(buf, sizeof(buf), "Bloodlust: %s", gamesettings_Get()->GetAlienEndGame() ? "Off" : "On");
			chatbox_Get()->AddLine(m_playerIndex, buf);

			if(gamesettings_Get()->GetStartingAge() > 0 || gamesettings_Get()->GetEndingAge() < g_theAgeDB->NumRecords()) {
				snprintf(buf, sizeof(buf), "Starting Age: %d", gamesettings_Get()->GetStartingAge());
				chatbox_Get()->AddLine(m_playerIndex, buf);

				snprintf(buf, sizeof(buf), "Ending Age: %d", gamesettings_Get()->GetEndingAge());
				chatbox_Get()->AddLine(m_playerIndex, buf);
			}

			if(TeamsEnabled()) {
				snprintf(buf, sizeof(buf), "Teammates: ");
				sint32 i;
				for(i = 1; i < k_MAX_PLAYERS; i++) {
					if(i == m_playerIndex)
						continue;
					if(!player_Get(i))
						continue;
					if(player_Get(i)->m_networkGroup == player_Get(m_playerIndex)->m_networkGroup) {
						char civname[1024];
						player_Get(i)->m_civilisation->GetSingularCivName(civname);
						snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s(%s)  ",
								player_Get(i)->m_civilisation->GetLeaderName(),
								civname);
					}
				}
				chatbox_Get()->AddLine(m_playerIndex, buf);
			}
		} else if(strnicmp(str, "/msg", 4) == 0) {
			char destination[256];
			sint32 dest = -1;
			sint32 d = 0;
			char *c = &str[4];
			while(*c && isspace(*c))
				c++;

			if(*c) {
				while(!isspace(*c) && *c) {
					destination[d] = *c;
					c++; d++;
				}
				destination[d] = 0;
				if(*c) {
					if(!isdigit(destination[0])) {
						sint32 p;
						for(p = 0; p < k_MAX_PLAYERS; p++) {
							if(!player_Get(p))
								continue;

							char name[256];
							const char *ln;
							char *n;

							for(n = &name[0], ln = player_Get(p)->m_civilisation->GetLeaderName();
							*ln && !isspace(*ln); ln++, n++)
								*n = *ln;
							*n = 0;

							if(player_Get(p) &&
								stricmp(destination, name) == 0) {
								dest = p;
								break;
							}
						}
					} else {
						dest = atoi(destination);
					}
					if(dest > 0 && dest < k_MAX_PLAYERS && player_Get(dest)) {
						NetChat *chatPacket = std::make_unique<NetChat>(1 << dest, c, len - (c - str)).release();
						if(network_Get().IsHost()) {
							QueuePacket(IndexToId(dest), chatPacket);
						} else {
							QueuePacket(m_hostId, chatPacket);
						}
					}
				}
			}
		}

		return;
	}


	AddChatText(str, len, static_cast<uint8>(selitem_Get()->GetVisiblePlayer()), FALSE);

	PacketizerPtr chatPacket = make_packetizer<NetChat>(m_chatMask, str, (sint16)len);
	if (IsActive()) {

		if(network_Get().IsHost()) {
			for(sint32 p = 0; p < k_MAX_PLAYERS; p++) {
				if(!player_Get(p)) continue;
				if(m_chatMask & (1 << p) && !player_Get(p)->IsRobot() &&
				   m_playerData[p]) {
					QueuePacket(IndexToId(p), chatPacket.get());
				}
			}
		} else {
			QueuePacket(m_hostId, chatPacket.get());
		}
    }
}

void Network::AddCivilization(sint32 index, PLAYER_TYPE pt, sint32 civ)
{
	if(m_iAmHost) {
		QueuePacketToAll(std::make_unique<NetInfo>(NET_INFO_CODE_NEW_CIVILIZATION,
									 index, pt, civ).release());
	} else {
		QueuePacket(m_hostId, std::make_unique<NetAction>(NET_ACTION_CREATED_CIV,
											index, pt, civ).release());
	}
}

void Network::KillPlayer(sint32 p, GAME_OVER reason, sint32 data)
{
	if(m_iAmHost) {
		QueuePacketToAll(std::make_unique<NetInfo>(NET_INFO_CODE_KILL_PLAYER, p,
									 reason, data).release());
		ClosePlayer(p);
	} else {
		QueuePacket(m_hostId, std::make_unique<NetAction>(NET_ACTION_KILLED_PLAYER, p).release());
	}
}

void Network::GetSliceFor(sint32 player)
{
	if(turn_Get()->SimultaneousMode() && IsHost()) {
		if(selitem_Get()->GetCurPlayer() != player) {
			if(!player_Get(selitem_Get()->GetCurPlayer())->IsNetwork()) {
				turn_Get()->SetSliceTo(player);
			} else {
				turn_Get()->QueueSliceFor(player);
			}
		}
	}
}

#ifdef _DEBUG
#define k_LEFT_EDGE 200
#define k_TOP_EDGE 80
#define k_TEXT_SPACING 16

void Network::DisplayChat(aui_Surface *surf)
{
	if(m_displayPackets) {
		MBCHAR buf[256];
		uint32 totalCount=0;
		uint32 totalBytes=0;
		uint32 totalSent = 0;
		uint32 totalSentBytes = 0;

		for(sint32 i = 0; i < k_NUM_PACKET_TYPES; i++) {
			snprintf(buf, sizeof(buf), "%c%c : Rx: %d/%d   Tx: %d/%d",
					m_packetName[i][0], m_packetName[i][1],
					m_packetCounter[i], m_packetBytes[i],
					m_sentPacketCounter[i], m_sentPacketBytes[i]);
			primitives_DrawText(surf, k_LEFT_EDGE,
								k_TOP_EDGE + ((i+1) * k_TEXT_SPACING),
								buf, 0, false);
			totalCount += m_packetCounter[i];
			totalBytes += m_packetBytes[i];
			totalSent += m_sentPacketCounter[i];
			totalSentBytes += m_sentPacketBytes[i];
		}
		snprintf(buf, sizeof(buf), "Total: Rx: %d/%d, Tx: %d/%d [%d blocked]",
				totalCount, totalBytes,
				totalSent, totalSentBytes,
				m_blockedPackets);
		primitives_DrawText(surf, k_LEFT_EDGE,
							k_TOP_EDGE + ((k_NUM_PACKET_TYPES+1) * k_TEXT_SPACING),
							buf, 0, false);
	}

	primitives_DrawText(surf, k_LEFT_EDGE, k_TOP_EDGE, m_chatStr, 0, false);
}
#endif

#ifdef _DEBUG
static sint32 GetPacketLogIndex(uint16 id)
{
	sint32 idx;
	switch(id) {
		case k_PACKET_CELL_ID: idx=0; break;
		case k_PACKET_CELL_LIST_ID: idx=1; break;
		case k_PACKET_UNIT_ID: idx=2; break;
		case k_PACKET_ACTION_ID: idx=3; break;
		case k_PACKET_INFO_ID: idx=4; break;
		case k_PACKET_CITY_ID: idx=5; break;
		case k_PACKET_POP_ID: idx=6; break;
		case k_PACKET_DIFFICULTY_ID: idx=7; break;
		case k_PACKET_PLAYER_ID: idx=8; break;
		case k_PACKET_TRADE_ROUTE_ID: idx=9; break;
		case k_PACKET_TRADE_OFFER_ID: idx=10; break;
		case k_PACKET_RAND_ID: idx=11; break;
		case k_PACKET_TERRAIN_ID: idx=12; break;
		case k_PACKET_INSTALLATION_ID: idx=13; break;
		case k_PACKET_CHAT_ID: idx=14; break;
		case k_PACKET_READINESS_ID: idx=15; break;
		case k_PACKET_HAPPY_ID: idx=16; break;
		case k_PACKET_PLAYER_HAPPY_ID: idx=17; break;
		case k_PACKET_REPORT_ID: idx = 18; break;
		case k_PACKET_UNIT_MOVE_ID: idx = 19; break;
		case k_PACKET_UNIT_ORDER_ID: idx = 20; break;
		case k_PACKET_AGREEMENT_ID: idx = 21; break;
		case k_PACKET_CIVILIZATION_ID: idx = 22; break;
		case k_PACKET_CITY_NAME_ID: idx = 23; break;
		case k_PACKET_DIP_PROPOSAL_ID: idx = 24; break;
		case k_PACKET_MESSAGE_ID: idx = 25; break;
		case k_PACKET_CITY2_ID: idx = 26; break;
		case k_PACKET_POLLUTION_ID: idx = 27; break;
		case k_PACKET_CITY_BQ_ID: idx = 28; break;
		case k_PACKET_KEYS_ID: idx = 29; break;
		case k_PACKET_GAME_SETTINGS_ID: idx = 30; break;
		case k_PACKET_NEW_ARMY_ID: idx = 31; break;
		case k_PACKET_REMOVE_ARMY_ID: idx = 32; break;
		case k_PACKET_DIP_RESPONSE_ID: idx = 33; break;
		default: idx = k_NUM_PACKET_TYPES - 1;
	}
	return idx;
}

void Network::LogPacket(uint8 c1, uint8 c2, uint16 size)
{
	sint32 idx;

	idx = GetPacketLogIndex(MAKE_CIV3_ID(c1, c2));
	m_packetCounter[idx]++;
	m_packetBytes[idx] += size;

}

void Network::LogSentPacket(uint8 c1, uint8 c2, uint16 size)
{
	sint32 idx;

	idx = GetPacketLogIndex(MAKE_CIV3_ID(c1, c2));
	m_sentPacketCounter[idx]++;
	m_sentPacketBytes[idx]+=size;
}

void Network::InitPacketLog()
{
	m_blockedPackets = 0;
	for(sint32 i = 0; i < k_NUM_PACKET_TYPES; i++) {
		m_packetCounter[i] = 0;
		m_packetBytes[i] = 0;
		m_sentPacketCounter[i] = 0;
		m_sentPacketBytes[i] = 0;

		switch(i) {
			case 0:
				m_packetName[i][0] = k_PACKET_CELL_ID >> 8;
				m_packetName[i][1] = k_PACKET_CELL_ID & 0xff; break;
			case 1:
				m_packetName[i][0] = k_PACKET_CELL_LIST_ID >> 8;
				m_packetName[i][1] = k_PACKET_CELL_LIST_ID & 0xff; break;
			case 2:
				m_packetName[i][0] = k_PACKET_UNIT_ID >> 8;
				m_packetName[i][1] = k_PACKET_UNIT_ID & 0xff; break;
			case 3:
				m_packetName[i][0] = k_PACKET_ACTION_ID >> 8;
				m_packetName[i][1] = k_PACKET_ACTION_ID & 0xff; break;
			case 4:
				m_packetName[i][0] = k_PACKET_INFO_ID >> 8;
				m_packetName[i][1] = k_PACKET_INFO_ID & 0xff; break;
			case 5:
				m_packetName[i][0] = k_PACKET_CITY_ID >> 8;
				m_packetName[i][1] = k_PACKET_CITY_ID & 0xff; break;
			case 6:
				m_packetName[i][0] = k_PACKET_POP_ID >> 8;
				m_packetName[i][1] = k_PACKET_POP_ID & 0xff; break;
			case 7:
				m_packetName[i][0] = k_PACKET_DIFFICULTY_ID >> 8;
				m_packetName[i][1] = k_PACKET_DIFFICULTY_ID & 0xff; break;
			case 8:
				m_packetName[i][0] = k_PACKET_PLAYER_ID >> 8;
				m_packetName[i][1] = k_PACKET_PLAYER_ID & 0xff; break;
			case 9:
				m_packetName[i][0] = k_PACKET_TRADE_ROUTE_ID >> 8;
				m_packetName[i][1] = k_PACKET_TRADE_ROUTE_ID & 0xff; break;
			case 10:
				m_packetName[i][0] = k_PACKET_TRADE_OFFER_ID >> 8;
				m_packetName[i][1] = k_PACKET_TRADE_OFFER_ID & 0xff; break;
			case 11:
				m_packetName[i][0] = k_PACKET_RAND_ID >> 8;
				m_packetName[i][1] = k_PACKET_RAND_ID & 0xff; break;
			case 12:
				m_packetName[i][0] = k_PACKET_TERRAIN_ID >> 8;
				m_packetName[i][1] = k_PACKET_TERRAIN_ID & 0xff; break;
			case 13:
				m_packetName[i][0] = k_PACKET_INSTALLATION_ID >> 8;
				m_packetName[i][1] = k_PACKET_INSTALLATION_ID & 0xff; break;
			case 14:
				m_packetName[i][0] = k_PACKET_CHAT_ID >> 8;
				m_packetName[i][1] = k_PACKET_CHAT_ID & 0xff; break;
			case 15:
				m_packetName[i][0] = k_PACKET_READINESS_ID >> 8;
				m_packetName[i][1] = k_PACKET_READINESS_ID & 0xff; break;
			case 16:
				m_packetName[i][0] = k_PACKET_HAPPY_ID >> 8;
				m_packetName[i][1] = k_PACKET_HAPPY_ID & 0xff; break;
			case 17:
				m_packetName[i][0] = k_PACKET_PLAYER_HAPPY_ID >> 8;
				m_packetName[i][1] = k_PACKET_PLAYER_HAPPY_ID & 0xff; break;
			case 18:
				m_packetName[i][0] = k_PACKET_REPORT_ID >> 8;
				m_packetName[i][1] = k_PACKET_REPORT_ID & 0xff; break;
			case 19:
				m_packetName[i][0] = k_PACKET_UNIT_MOVE_ID >> 8;
				m_packetName[i][1] = k_PACKET_UNIT_MOVE_ID & 0xff; break;
			case 20:
				m_packetName[i][0] = k_PACKET_UNIT_ORDER_ID >> 8;
				m_packetName[i][1] = k_PACKET_UNIT_ORDER_ID & 0xff; break;
			case 21:
				m_packetName[i][0] = k_PACKET_AGREEMENT_ID >> 8;
				m_packetName[i][1] = k_PACKET_AGREEMENT_ID & 0xff; break;
			case 22:
				m_packetName[i][0] = k_PACKET_CIVILIZATION_ID >> 8;
				m_packetName[i][1] = k_PACKET_CIVILIZATION_ID & 0xff; break;
			case 23:
				m_packetName[i][0] = k_PACKET_CITY_NAME_ID >> 8;
				m_packetName[i][1] = k_PACKET_CITY_NAME_ID & 0xff; break;
			case 24:
				m_packetName[i][0] = k_PACKET_DIP_PROPOSAL_ID >> 8;
				m_packetName[i][1] = k_PACKET_DIP_PROPOSAL_ID & 0xff; break;
			case 25:
				m_packetName[i][0] = k_PACKET_MESSAGE_ID >> 8;
				m_packetName[i][1] = k_PACKET_MESSAGE_ID & 0xff; break;
			case 26:
				m_packetName[i][0] = k_PACKET_CITY2_ID >> 8;
				m_packetName[i][1] = k_PACKET_CITY2_ID & 0xff; break;
			case 27:
				m_packetName[i][0] = k_PACKET_POLLUTION_ID >> 8;
				m_packetName[i][1] = k_PACKET_POLLUTION_ID & 0xff; break;
			case 28:
				m_packetName[i][0] = k_PACKET_CITY_BQ_ID >> 8;
				m_packetName[i][1] = k_PACKET_CITY_BQ_ID & 0xff; break;
			case 29:
				m_packetName[i][0] = k_PACKET_KEYS_ID >> 8;
				m_packetName[i][1] = k_PACKET_KEYS_ID & 0xff; break;
			case 30:
				m_packetName[i][0] = k_PACKET_GAME_SETTINGS_ID >> 8;
				m_packetName[i][1] = k_PACKET_GAME_SETTINGS_ID & 0xff; break;
			case 31:
				m_packetName[i][0] = k_PACKET_NEW_ARMY_ID >> 8;
				m_packetName[i][1] = k_PACKET_NEW_ARMY_ID & 0xff; break;
			case 32:
				m_packetName[i][0] = k_PACKET_REMOVE_ARMY_ID >> 8;
				m_packetName[i][1] = k_PACKET_REMOVE_ARMY_ID & 0xff; break;
			case 33:
				m_packetName[i][0] = k_PACKET_DIP_RESPONSE_ID >> 8;
				m_packetName[i][1] = k_PACKET_DIP_RESPONSE_ID & 0xff; break;
			default:
				m_packetName[i][0] = '?';
				m_packetName[i][0] = '?';
				break;
		}
	}
}

void Network::TogglePacketLog()
{
	m_displayPackets = !m_displayPackets;
#if defined(_PLAYTEST)
	if(m_displayPackets) {
		g_debugOwner = k_DEBUG_OWNER_NETWORK_CHAT;
	}
#endif
}

#endif

PlayerData::PlayerData(char* name, uint16 id) :
	m_name(name ? name : ""),
	m_id(id),
	m_index(-1),
	m_frozen(FALSE),
	m_ready(FALSE),
	m_blocked(0),
	m_ackBeginTurn(FALSE)
{
	m_bookmarks = std::make_unique<PointerList<PointerList<Packetizer>::PointerListNode>>();
	m_packetList = std::make_unique<PointerList<Packetizer>>();
	m_createdCities = std::make_unique<UnitDynamicArray>();
	memset(&m_guid, 0, sizeof(GUID));

	m_sentResync = FALSE;
};

PlayerData::~PlayerData()
{
	Packetizer* packet;
	while(!m_packetList->IsEmpty()) {
		packet = m_packetList->RemoveHead();
		packet->Release();
	}

	m_bookmarks.reset();
	m_packetList.reset();
	m_createdCities.reset();
}

uint8 Network::GetGameStyle() const
{
	return m_gameStyle;
}

BOOL Network::IsClassicStyle() const
{
	return m_gameStyle == 0;
}

BOOL Network::IsUnitMovesStyle() const
{
	return (m_gameStyle & k_GAME_STYLE_UNIT_MOVES) != 0;
}

BOOL Network::IsSpeedStyle() const
{
	return (m_gameStyle & k_GAME_STYLE_SPEED) != 0;
}

BOOL Network::IsTimedStyle() const
{
	return (m_gameStyle & k_GAME_STYLE_TOTAL_TIME) != 0;
}

sint32 Network::GetUnitMovesPerSlice() const
{
	return m_unitMovesPerSlice;
}

sint32 Network::GetUnitMovesUsed() const
{
	return m_unitMovesUsed;
}

time_t Network::GetTotalStartTime() const
{
    return m_totalStartTime;
}

time_t Network::GetTotalTimeUsed() const
{
    return m_totalTimeUsed;
}

time_t Network::GetTurnStartTime() const
{
    return m_turnStartTime;
}

time_t Network::GetTurnStartedAt() const
{
    return static_cast<sint32>(m_turnStartedAt);
}

time_t Network::GetTurnEndsAt() const
{
	return m_turnEndsAt;
}

time_t Network::GetBonusTime() const
{
	return m_bonusTime;
}

void Network::SetClassicStyle(BOOL fromServer)
{
	if(!IsActive() || (m_iAmHost || fromServer)) {
		m_gameStyle = 0;
		if(m_iAmHost) {
			Enqueue(std::make_unique<NetInfo>(NET_INFO_CODE_CLASSIC_STYLE).release());
		}
	}
}

void Network::SetUnitMovesStyle(BOOL on, sint32 unitMovesPerSlice,
								BOOL fromServer)
{
	if(!IsActive() || (m_iAmHost || fromServer)) {
		m_unitMovesPerSlice = unitMovesPerSlice;
		if(on) {
			m_gameStyle |= k_GAME_STYLE_UNIT_MOVES;
		} else {
			m_gameStyle &= ~(k_GAME_STYLE_UNIT_MOVES);
		}
		if(m_iAmHost) {
			Enqueue(std::make_unique<NetInfo>(NET_INFO_CODE_UNIT_MOVES_STYLE, on, unitMovesPerSlice).release());
		}
	}
}

void Network::SetSpeedStyle(BOOL on, sint32 timePerTurn,
							BOOL fromServer, sint32 timePerCity)
{
	if(!IsActive() || (m_iAmHost || fromServer)) {
		m_turnStartTime = timePerTurn;
		if(on) {
			uint32 oldStyle = m_gameStyle;
			m_gameStyle |= k_GAME_STYLE_SPEED;
			if(timePerCity > 0) {
				m_gameStyle |= k_GAME_STYLE_SPEED_CITIES;
				m_extraTimePerCity = timePerCity;
			} else {
				m_extraTimePerCity = 0;
				m_gameStyle &= ~(k_GAME_STYLE_SPEED_CITIES);
			}
			if(oldStyle != m_gameStyle) {
				m_turnStartedAt = time(nullptr);
				ResetTurnEndsAt();
			}
		} else {
			m_gameStyle &= ~(k_GAME_STYLE_SPEED | k_GAME_STYLE_SPEED_CITIES);
		}

		if(m_iAmHost) {
			Enqueue(std::make_unique<NetInfo>(NET_INFO_CODE_SPEED_STYLE, on, timePerTurn, timePerCity).release());
		}
	}
}

void Network::SetTimedStyle(BOOL on, sint32 timePerGame,
							BOOL fromServer)
{
	if(!IsActive() || (m_iAmHost || fromServer)) {
		m_totalStartTime = timePerGame;
		if(on) {
			if(!(m_gameStyle & k_GAME_STYLE_TOTAL_TIME))
				m_totalTimeUsed = 0;

			m_gameStyle |= k_GAME_STYLE_TOTAL_TIME;
		} else {
			m_gameStyle &= ~(k_GAME_STYLE_TOTAL_TIME);
		}
		if(m_iAmHost) {
			Enqueue(std::make_unique<NetInfo>(NET_INFO_CODE_TIMED_STYLE, on, timePerGame).release());
		}
	}
}

void Network::SetSimultaneousStyle(BOOL on, BOOL fromServer)
{
	if(!IsActive() || (m_iAmHost || fromServer)) {
		if(on) {
			m_gameStyle = k_GAME_STYLE_SIMULTANEOUS;
		} else {
			m_gameStyle &= ~(k_GAME_STYLE_SIMULTANEOUS);
		}

		if(m_iAmHost) {
			Enqueue(std::make_unique<NetInfo>(NET_INFO_CODE_SIMULTANEOUS_STYLE, on).release());
		}
	}
}

void Network::SetCarryoverStyle(BOOL on, BOOL fromServer)
{
	if(!IsActive() || (m_iAmHost || fromServer)) {
		if(on) {
			if(!(m_gameStyle & k_GAME_STYLE_CARRYOVER)) {
				m_gameStyle |= k_GAME_STYLE_CARRYOVER;
			}
		} else {
			m_gameStyle &= ~(k_GAME_STYLE_CARRYOVER);
		}

		m_bonusTime = 0;
		if(m_iAmHost) {
			Enqueue(std::make_unique<NetInfo>(NET_INFO_CODE_CARRYOVER_STYLE, on).release());
		}
	}
}

void Network::SetStyleFromServer(uint8 gameStyle,
								 sint32 movesPerSlice,
								 time_t totalTime,
								 time_t turnTime,
								 time_t cityTime)
{
	m_gameStyle = gameStyle;
	m_unitMovesPerSlice = movesPerSlice;
	m_totalStartTime = totalTime;
	m_turnStartTime = turnTime;
	m_extraTimePerCity = cityTime;

	m_totalTimeUsed = 0;
}

void Network::UnitsMoved(sint32 count)
{
	if(!(m_gameStyle & k_GAME_STYLE_UNIT_MOVES))
		return;

	m_unitMovesUsed += count;
	if(m_unitMovesUsed >= m_unitMovesPerSlice && IsMyTurn()) {
		turn_Get()->EndThisSliceBeginNewSlice();
	}
}

void Network::SetMyTurn(BOOL turn)
{
	if(turn) {
		if(m_gameStyle & k_GAME_STYLE_UNIT_MOVES) {
			m_unitMovesUsed = 0;
		}

		m_turnStartedAt = time(nullptr);
		if(m_gameStyle & (k_GAME_STYLE_SPEED | k_GAME_STYLE_SPEED_CITIES)) {
			ResetTurnEndsAt();
		}
		if(g_theProgressWindow) {
			ProgressWindow::EndProgress(g_theProgressWindow);
		}
		if(!m_isMyTurn) {
		}

	} else {
		if(m_isMyTurn) {

			sint32 timeUsed = static_cast<sint32>(time(nullptr) - m_turnStartedAt);
			m_totalTimeUsed += timeUsed;
			if((m_gameStyle & k_GAME_STYLE_SPEED) &&
			   (m_gameStyle & k_GAME_STYLE_CARRYOVER)) {
				m_bonusTime = m_turnStartTime - timeUsed;
				if(m_bonusTime < 0)
					m_bonusTime = 0;
			}
		}
	}
	m_isMyTurn = turn;




}

BOOL Network::IsLocalPlayer(sint32 index)
{
	if(index == m_playerIndex)
		return TRUE;

	if(m_iAmClient)
		return FALSE;

	if(!player_Get(index))
		return FALSE;

	if(player_Get(index)->IsNetwork())
		return FALSE;

	return TRUE;
}

void Network::TurnSync()
{
	if(IsLocalPlayer(selitem_Get()->GetCurPlayer())) {
		SetMyTurn(TRUE);

		m_turnStartedAt = time(nullptr);
		if(m_gameStyle & (k_GAME_STYLE_SPEED | k_GAME_STYLE_SPEED_CITIES)) {
			ResetTurnEndsAt();
		}

		DPRINTF(k_DBG_NET, ("Adding finish begin turn for player %d.  Rand call count: %d\n",
							selitem_Get()->GetCurPlayer(), rand_ptr()->CallCount()));
		gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_FinishBeginTurn,
							   GEA_Player, selitem_Get()->GetCurPlayer(),
							   GEA_End);
		if(player_Get(selitem_Get()->GetCurPlayer())->IsRobot())
		{
			CtpAi::BeginMapAnalysis(selitem_Get()->GetCurPlayer());
			CtpAi::BeginTurn(selitem_Get()->GetCurPlayer());
		}
	}
}

void Network::EnterSetupMode()
{
	Assert(turn_Get()->GetRound() == 0);
	if(turn_Get()->GetRound() != 0)
		return;

	for(sint32 i = 0; i < k_MAX_PLAYERS; i++) {
		if(player_Get(i)) {
			if(player_Get(i)->IsRobot()) {
				player_Get(i)->m_doneSettingUp = TRUE;
			} else {
				player_Get(i)->m_doneSettingUp = FALSE;
				if(m_iAmHost) {
					Assert(player_Get(i)->m_all_armies->Num() > 0);

					if(player_Get(i)->m_all_armies->Num() > 0) {
						MapPoint pos;
						player_Get(i)->m_all_armies->Access(0).GetPos(pos);
						SetSetupArea(i, pos, profiledb_Get()->SetupRadius());
					}
					SetPowerPoints(i, profiledb_Get()->PowerPoints());
				}
			}
		}
	}

	if(m_iAmHost) {
		Enqueue(std::make_unique<NetInfo>(NET_INFO_CODE_SET_SETUP_MODE, TRUE).release());
	}
	m_setupMode = TRUE;
}

void Network::ExitSetupMode()
{
	if(m_iAmHost) {
		Enqueue(std::make_unique<NetInfo>(NET_INFO_CODE_SET_SETUP_MODE, FALSE).release());
	}
	m_setupMode = FALSE;
}

void Network::SignalSetupDone(PLAYER_INDEX player)
{
	player_Get(player)->m_doneSettingUp = TRUE;

	if(m_iAmClient) {
		SendAction(std::make_unique<NetAction>(NET_ACTION_DONE_SETTING_UP).release());
		return;
	}

	sint32 count = 0;
	sint32 active = 0;

	for(sint32 i = 0; i < k_MAX_PLAYERS; i++) {
		if(player_Get(i)) {
			active++;
			if(!player_Get(i)->IsNetwork()) {
				player_Get(i)->m_doneSettingUp = TRUE;
			}
			if(player_Get(i)->m_doneSettingUp)
				count++;
		}
	}
	if(count >= active)
		ExitSetupMode();
}

BOOL Network::SetupMode() const
{
	return m_setupMode;
}

void Network::SetSetupArea(PLAYER_INDEX player, const MapPoint &center,
						   sint32 radius)
{
	player_Get(player)->m_setupCenter = center;
	player_Get(player)->m_setupRadius = radius;

	player_Get(player)->AddUnitVision(center, radius);

	player_Get(player)->OwnExploredArea();

	player_Get(player)->RemoveUnitVision(center, radius);

	if(m_iAmHost)
	{
		Enqueue(std::make_unique<NetInfo>(NET_INFO_CODE_SET_SETUP_AREA,
							player, center.x, center.y,
							radius).release());
	}
}

BOOL Network::IsInSetupArea(PLAYER_INDEX player, const MapPoint &pnt) const
{
	sint32 dist = UnitData::GetDistance(pnt,
										player_Get(player)->m_setupCenter,
										player_Get(player)->m_setupRadius);
	double rplus = double(player_Get(player)->m_setupRadius) + 0.5;
	if(dist > (rplus * rplus)) {
		return FALSE;
	} else {
		return TRUE;
	}
}

void Network::SetPowerPoints(PLAYER_INDEX player, sint32 points)
{
	player_Get(player)->m_powerPoints = points;
	if(m_iAmHost) {
		Enqueue(std::make_unique<NetInfo>(NET_INFO_CODE_POWER_POINTS,
							player, points).release());
	}
}

BOOL Network::CanStillSetup(PLAYER_INDEX index)
{
	return !player_Get(index)->m_doneSettingUp;
}

BOOL Network::CurrentPlayerAckedBeginTurn()
{
	Assert(m_playerData[selitem_Get()->GetCurPlayer()]);
	if(!m_playerData[selitem_Get()->GetCurPlayer()])
		return FALSE;

	return m_playerData[selitem_Get()->GetCurPlayer()]->m_ackBeginTurn;
}

void Network::BeginTurn(PLAYER_INDEX index)
{
	if(m_playerData[index]) {
		m_playerData[index]->m_ackBeginTurn = FALSE;
	}
}

void Network::AckBeginTurn(PLAYER_INDEX index)
{
	Assert(m_playerData[index]);
	if(m_playerData[index]) {
		m_playerData[index]->m_ackBeginTurn = TRUE;
	}




	gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
		GEV_BeginTurn,
		GEA_Player, index,
		GEA_Int, player_Get(index)->m_current_round + 1,
		GEA_End);

}

void Network::AddCreatedCity(PLAYER_INDEX owner, Unit &city)
{
	if(m_playerData[owner]) {
		m_playerData[owner]->m_createdCities->Insert(city);
	}
}

UnitDynamicArray *Network::GetCreatedCities(PLAYER_INDEX owner)
{
	Assert(m_playerData[owner]);
	if(!m_playerData[owner])
		return nullptr;

	return m_playerData[owner]->m_createdCities.get();
}

void Network::AddResetCityOwnerHack(const Unit &unit)
{
	m_resetCityOwnerHackList->Insert(unit);
}

void Network::DoResetCityOwnerHack()
{
	sint32 i;
	sint32 n = m_resetCityOwnerHackList->Num();

	for(i = 0; i < n; i++) {
		Unit u = m_resetCityOwnerHackList->Access(i);
		if(unitpool_Get()->IsValid(u)) {
			Enqueue(u.AccessData());
			Enqueue(u.AccessData(),
					u.AccessData()->GetCityData(),
					TRUE);
		}
	}

	for(i = 0; i < n; i++) {
		m_resetCityOwnerHackList->DelIndex(0);
	}
}

void Network::SetupPlayerFromNSPlayerInfo(uint16 id, sint32 index)
{
	Player *p = player_Get(index);
	Assert(p);
	if(p) {
		NSPlayerInfo *nspi = nullptr;
		PointerList<NSPlayerInfo>::Walker walk(m_nsPlayerInfo.get());
		while(walk.IsValid()) {
			if(walk.GetObj()->m_id == id) {
				nspi = walk.GetObj();
				break;
			}
			walk.Next();
		}

		if(!nspi)
			return;
		if(!m_fromSave) {
		}
	}
}

sint32 Network::GetNumHumanPlayers()
{
	return m_nsPlayerInfo->GetCount();
}

uint32 Network::GetHumanMask()
{
	uint32 mask = 0;
	sint32 i;
	sint32 n = m_nsPlayerInfo->GetCount();
	if(profiledb_Get()->NoHumanPlayersOnHost())
		n--;

	for(i = 1; i < k_MAX_PLAYERS; i++) {
		if(player_Get(i) && player_Get(i)->m_networkId != 0) {
			mask |= (1 << i);
			n--;
		}
	}
	if(n > 0) {
		for(i = 1; i < k_MAX_PLAYERS; i++) {
			if(player_Get(i) && !(mask & (1 << i))) {
				mask |= (1 << i);
				n--;
				if(n <= 0)
					break;
			}
		}
	}

	return mask;
}

void Network::SetCondensePopMoves(BOOL on)
{
	m_condensePopMoves = on;
}

BOOL Network::ShouldAckBeginTurn()
{

	if(m_enactedDiplomaticRequests->Num() > 0)
		return FALSE;
	if(m_iAmClient) {
		if(m_waitingOnResync)
			return FALSE;
	}

	return TRUE;
}

void Network::AddEnact(DiplomaticRequest &req)
{
	m_enactedDiplomaticRequests->Insert(req);
}

void Network::RemoveEnact(DiplomaticRequest &req)
{
	if(m_iAmClient) {






		m_enactedDiplomaticRequests->Del(req);
	} else {
		Assert(m_enactedDiplomaticRequests->Access(0).m_id == req.m_id);
		if(m_enactedDiplomaticRequests->Access(0).m_id != req.m_id) {
			while(m_enactedDiplomaticRequests->Num() > 0) {
				m_enactedDiplomaticRequests->Access(0).Enact(TRUE);
				m_enactedDiplomaticRequests->DelIndex(0);
			}
			Resync(selitem_Get()->GetCurPlayer());
			return;
		}
		if(diplomaticrequestpool_Get()->IsValid(req)) {

			m_enactedDiplomaticRequests->Access(0).Enact(TRUE);
		}
		m_enactedDiplomaticRequests->DelIndex(0);

		if(m_enactedDiplomaticRequests->Num() < 1 &&
		   m_endTurnWhenClear) {
			turn_Get()->EndThisTurnBeginNewTurn(TRUE);
		}
	}
}

void Network::SetGuid(uint16 id, GUID *guid)
{
	BOOL found = FALSE;
	PointerList<PlayerData>::Walker walk(m_newPlayerList.get());
	while(walk.IsValid() && !found) {
		if(walk.GetObj()->m_id == id) {
			walk.GetObj()->m_guid = *guid;
			found = TRUE;
		}
		walk.Next();
	}
	Assert(found);
}

void Network::KickPlayer(sint32 player)
{
	if(m_iAmHost && m_playerData[player]) {
		m_netIO->KickPlayer(m_playerData[player]->m_id);
	}
}

sint32 Network::GetProgress()
{
	return m_progress;
}

void Network::ResetGuid(sint32 player)
{
	if(m_playerData[player] && player_Get(player)) {
		player_Get(player)->m_networkGuid = m_playerData[player]->m_guid;
		QueuePacketToAll(std::make_unique<NetSetPlayerGuid>(player).release());
	}
}

sint32 Network::CountOpenSlots()
{
	sint32 i;
	sint32 count = 0;
	for(i = 1; i < k_MAX_PLAYERS; i++) {
		if(player_Get(i) && player_Get(i)->m_openForNetwork)
			count++;
	}
	return count;
}

sint32 Network::CountTakenSlots()
{
	sint32 i;
	sint32 count = 0;
	for(i = 1; i < k_MAX_PLAYERS; i++) {
		if(m_playerData[i]) {
			count++;
		}
	}
	return count;
}

void Network::OpenPlayer(sint32 player)
{
	if(!m_iAmHost)
		return;

	if(!player_Get(player) || player_Get(player)->m_isDead) {
		return;
	}
	player_Get(player)->m_openForNetwork = TRUE;
	SetMaxPlayers(CountOpenSlots() + CountTakenSlots());
}

void Network::ClosePlayer(sint32 player)
{
	if(!m_iAmHost)
		return;

	if(!player_Get(player)) {
		return;
	}
	player_Get(player)->m_openForNetwork = FALSE;
	SetMaxPlayers(CountOpenSlots() + CountTakenSlots());
}

void Network::ResetTurnEndsAt()
{
	m_turnEndsAt = time(nullptr) + m_turnStartTime + m_bonusTime;
	if((m_gameStyle & k_GAME_STYLE_SPEED_CITIES) && player_arr_Get() && player_Get(m_playerIndex)) {
		m_turnEndsAt += player_Get(m_playerIndex)->m_all_cities->Num() *
			m_extraTimePerCity;
	}
}

void Network::SetAllPlayersReady()
{
	if(!m_readyToStart && !m_crcError) {
		if(tiledmap_Get()) {
			tiledmap_Get()->CopyVision();
		}
		NewTurnCount::ClientStartNewYear();

		c3_RemoveAbortMessage();
		SetReadyToStart(TRUE);
	}
}

void Network::SetProgress(sint32 progress)
{
	if(m_crcError)
		return;

	m_progress = progress;
	c3_AbortUpdateData(nullptr, (progress > 100 ? 100 : progress) );
	if(m_progress >= 100) {
		const char *str = stringdb_Get()->GetNameStr("NETWORK_WAITING_ON_PLAYERS");

		char nonConstStr[1024];
		if(str) {
			strlcpy(nonConstStr, str, sizeof(nonConstStr));
	} else {
			strlcpy(nonConstStr, "Waiting on players", sizeof(nonConstStr));
		}
		c3_AbortUpdateData(nonConstStr, 100);
	} else {
		c3_AbortUpdateData(nullptr, progress);

		civapp_Get()->ProcessGraphicsCallback();
	}
}

void Network::SetReadyToStart(BOOL ready)
{
	m_readyToStart = ready;
	if(ready) {
		director_Get()->AddCopyVision();
		tiledmap_Get()->InvalidateMix();
		tiledmap_Get()->InvalidateMap();
		tiledmap_Get()->Refresh();
		radar_map_Get()->Update();

		if(player_Get(m_playerIndex)->m_first_city) {
			MapPoint pos;
			if(player_Get(m_playerIndex)->m_all_armies->Num() > 0) {
				player_Get(m_playerIndex)->m_all_armies->Access(0).GetPos(pos);
				director_Get()->AddCenterMap(pos);
			}
		}

		if(m_wasAttached) {

			player_Get(m_playerIndex)->SetPlayerType(PLAYER_TYPE_ROBOT);
		}

		if(m_iAmHost) {

			sint32 i;
			for(i = 0; i < k_MAX_PLAYERS; i++) {
				if(!player_Get(i)) continue;
				if(i == m_playerIndex) continue;
				if(!m_playerData[i] && !player_Get(i)->IsRobot()) {
					SendLeftMessage(player_Get(i)->m_civilisation->GetLeaderName(), i);




					player_Get(i)->SetPlayerType(PLAYER_TYPE_ROBOT);
					SetRobotName(i);
					OpenPlayer(i);
				} else if(player_Get(i)->IsRobot()) {
					if(m_dynamicJoin) {
						OpenPlayer(i);
					} else {
						ClosePlayer(i);
					}
				}
				if(i == selitem_Get()->GetCurPlayer() && player_Get(i)->IsRobot()) {

					director_Get()->AddEndTurn();
				}
				QueuePacketToAll(std::make_unique<NetSetPlayerGuid>(i).release());
			}




			QueuePacketToAll(std::make_unique<NetInfo>(NET_INFO_CODE_ALL_PLAYERS_READY).release());
		}

		MainControlPanel::UpdatePlayer(selitem_Get()->GetCurPlayer());
	}
}

void Network::SendJoinedMessage(MBCHAR *name, sint32 player)
{
	if(slicengine_Get() && player != m_playerIndex && name) {
		auto so = std::make_unique<SlicObject>("351NetworkPlayerJoined");
		so->AddAction(name);
		so->AddCivilisation(player);
		so->AddRecipient(m_playerIndex);

		MBCHAR interp[k_MAX_NAME_LEN];
		stringutils_Interpret(stringdb_Get()->GetNameStr("NETWORK_PLAYER_JOINED"), *so, interp);
		AddChatText(interp, strlen(interp), 0, FALSE);

		slicengine_Get()->Execute(std::move(so));

	}

	if(m_iAmHost) {
		QueuePacketToAll(std::make_unique<NetInfoMessage>(NET_MSG_PLAYER_JOINED,
										name, player).release());
	}
}

void Network::SendWrongPlayerJoinedMessage(MBCHAR *name, sint32 player)
{
	if(slicengine_Get()) {
		auto so = std::make_unique<SlicObject>("352DifferentPlayerJoined");
		so->AddAction(name);
		so->AddCivilisation(player);
		so->AddRecipient(m_playerIndex);
		slicengine_Get()->Execute(std::move(so));
	}
}

void Network::SendLeftMessage(const MBCHAR *name, sint32 player)
{
	if(slicengine_Get()) {
		auto so = std::make_unique<SlicObject>("350NetworkPlayerLeft");
		so->AddAction(name);

		so->AddRecipient(m_playerIndex);

		MBCHAR interp[k_MAX_NAME_LEN];
		stringutils_Interpret(stringdb_Get()->GetNameStr("NETWORK_PLAYER_LEFT"), *so, interp);
		AddChatText(interp, strlen(interp), 0, FALSE);

		slicengine_Get()->Execute(std::move(so));
	}

	if(m_iAmHost) {
		QueuePacketToAll(std::make_unique<NetInfoMessage>(NET_MSG_PLAYER_LEFT,
										name, player).release());
	}
}

void Network::SendNewHostMessage(MBCHAR *name, sint32 player)
{
	if(slicengine_Get()) {
		auto so = std::make_unique<SlicObject>(m_iAmHost ? "353YouAreNowHost" : "354NewHost");
		so->AddAction(name);
		so->AddRecipient(m_playerIndex);

		MBCHAR interp[k_MAX_NAME_LEN];
		stringutils_Interpret(stringdb_Get()->GetNameStr("NETWORK_YOU_ARE_NOW_HOST"), *so, interp);
		AddChatText(interp, strlen(interp), 0, FALSE);

		slicengine_Get()->Execute(std::move(so));
	}

	if(m_iAmHost) {
		QueuePacketToAll(std::make_unique<NetInfoMessage>(NET_MSG_NEW_HOST,
											name, player).release());
	}
}

void Network::Resync(sint32 playerIndex)
{
	if(!m_playerData[playerIndex]) {
		return;
	}

	if(m_playerData[playerIndex]->m_sentResync)
		return;

	uint16 id = IndexToId(playerIndex);
	if(id != 0xffff) {
		m_playerData[playerIndex]->m_sentResync = TRUE;




		m_playerData[playerIndex]->m_createdUnits.Clear();
		m_playerData[playerIndex]->m_createdArmies.Clear();
		m_playerData[playerIndex]->m_createdCities->Clear();


		if(selitem_Get()->GetCurPlayer() == playerIndex &&
		   !m_playerData[playerIndex]->m_ackBeginTurn) {


			gevmanager_Get()->AddEvent(GEV_INSERT_Tail,
				GEV_BeginTurn,
				GEA_Player, playerIndex,
				GEA_Int, player_Get(playerIndex)->m_current_round,
				GEA_End);
		}

		QueuePacket(id, std::make_unique<NetInfo>(NET_INFO_CODE_RESYNC).release());
		SetReady(id);
	}
#ifdef WIN32
	DPRINTF(k_DBG_NET, ("Resync stack trace: %s\n", c3debug_StackTrace()));
#endif
}

void Network::StartResync()
{
	Assert(m_iAmClient);
	if(!m_iAmClient)
		return;

	close_AllScreens();

	if(director_Get()) {
		director_Get()->CatchUp();
	}

	if(tiledmap_Get()) {
		tiledmap_Get()->QuickBlackBackGround(nullptr);
	}

	if(sci_advancescreen_isOnScreen()) {
		sci_advancescreen_removeMyWindow(AUI_BUTTON_ACTION_EXECUTE);
	}


	if(player_Get(m_playerIndex)->IsRobot())
	{
		m_wasAttached = TRUE;
	}

	m_readyToStart = FALSE;
	m_waitingOnResync = FALSE;

	if(m_gameObjects) {





		m_gameObjects = std::make_unique<NetGameObj>();
	}

	ClearDeadUnits();

	DPRINTF(k_DBG_NET, ("Acknowledging resync\n"));
	QueuePacket(m_hostId, std::make_unique<NetReport>(NET_REPORT_ACK_RESYNC).release());

	const char *str = stringdb_Get()->GetNameStr("NETWORK_RESYNCING");
	char nonConstStr[1024];
	if(str) {
		strlcpy(nonConstStr, str, sizeof(nonConstStr));
	} else {
		strlcpy(nonConstStr, "Resyncing", sizeof(nonConstStr));
	}
	c3_AbortMessage(nonConstStr, k_UTILITY_PROGRESS_ABORT, network_AbortCallback );

	if(gevmanager_Get())
		gevmanager_Get()->NotifyResync();

	if(director_Get())
		director_Get()->NotifyResync();

	CtpAi::Initialize();
}

void Network::AckResync(sint32 index)
{
	if(!m_playerData[index])
		return;

	m_playerData[index]->m_sentResync = FALSE;
}

void Network::RequestResync(RESYNC_REASON reason)
{
	Assert(m_iAmClient);
	if(!m_iAmClient)
		return;

	if(m_waitingOnResync)
		return;

#ifdef WIN32
	DPRINTF(k_DBG_NET, ("RequestResync(%d) stack trace: %s\n", reason, c3debug_StackTrace()));
#endif

	m_readyToStart = FALSE;
	const char *str = stringdb_Get()->GetNameStr("NETWORK_RESYNCING");
	char nonConstStr[1024];
	if(str) {
		strlcpy(nonConstStr, str, sizeof(nonConstStr));
	} else {
		strlcpy(nonConstStr, "Resyncing", sizeof(nonConstStr));
	}
	c3_AbortMessage(nonConstStr, k_UTILITY_PROGRESS_ABORT, network_AbortCallback);

	SendAction(std::make_unique<NetAction>(NET_ACTION_REQUEST_RESYNC, reason).release());

	m_waitingOnResync = TRUE;
}

BOOL Network::SentResync(sint32 playerindex)
{
	Assert(m_iAmHost);
	if(!m_iAmHost)
		return FALSE;

	if(!m_playerData[playerindex])
		return FALSE;

	return m_playerData[playerindex]->m_sentResync;
}

uint32 Network::PackedPos(const MapPoint &pnt)
{
	return ((pnt.x & 0x7fff) << 16) |
		(pnt.y & 0xffff);
}

void Network::UnpackedPos(uint32 p, MapPoint &pnt)
{
	pnt.x = static_cast<sint16>((p & 0x7fff0000) >> 16);
	pnt.y = static_cast<sint16>(p & 0x0000ffff);
}

void network_PlayerListCallback(sint32 player, sint32 val, sint32 action)
{
	if(val) {
		switch(action) {
			case PLAYER_ACTION_KICK:
				if(player != network_Get().GetPlayerIndex())
					network_Get().KickPlayer(player);
				break;
			case PLAYER_ACTION_OPEN:

					network_Get().OpenPlayer(player);

				break;
			case PLAYER_ACTION_CLOSE:

					network_Get().ClosePlayer(player);

				break;
		}
	}
	if(g_networkPlayersScreen) {
		g_networkPlayersScreen->UpdateData();
	}
}

MBCHAR *Network::GetStatusString(sint32 player)
{
	static MBCHAR strbuf[1024];

	if(!player_arr_Get() || !player_Get(player))
		return nullptr;
	if(player_Get(player)->IsHuman()) {
		strlcpy(strbuf, stringdb_Get()->GetNameStr("NETWORK_PLAYER_STATUS_HUMAN"), sizeof(strbuf));
	} else if(player_Get(player)->IsNetwork()) {
		strlcpy(strbuf, stringdb_Get()->GetNameStr("NETWORK_PLAYER_STATUS_CONNECTED"), sizeof(strbuf));
	} else {
		if(player_Get(player)->m_openForNetwork) {
			strlcpy(strbuf, stringdb_Get()->GetNameStr("NETWORK_PLAYER_STATUS_AI_OPEN"), sizeof(strbuf));
		} else {
			strlcpy(strbuf, stringdb_Get()->GetNameStr("NETWORK_PLAYER_STATUS_AI_CLOSED"), sizeof(strbuf));
		}
	}
	return strbuf;
}

void Network::SetDynamicJoin(BOOL on)
{
	m_dynamicJoin = on;
	if(m_readyToStart) {
		if(m_netIO) {
			if(m_dynamicJoin) {
				SetMaxPlayers(CountOpenSlots() + CountTakenSlots());
			} else {
				SetMaxPlayers(CountTakenSlots());
			}
		}
	}
}

void Network::ChunkList(uint16 id, PointerList<Packetizer> * a_List)
{
	Assert(a_List);
	if(!a_List)
		return;

	std::vector<uint8> mapBuf(a_List->GetCount() * 258 + 16384);

	mapBuf[0] = k_CHUNK_HEAD;
	mapBuf[1] = k_CHUNK_BODY;
	sint32 size = 2;

	while(a_List->GetHead()) {
		uint16 len = 0;

		std::unique_ptr<Packetizer> packet(a_List->RemoveHead());
		// Packetize writes an unknown length (bounded by k_MAX_PACKET_LEN)
		// before it reports it, so guarantee room for the largest possible
		// packet before the write. The reactive realloc below keeps the buffer
		// sized for subsequent packets, but only enforced the invariant via an
		// Assert that is compiled out in release builds.
		const sint32 k_MAX_PACKET_LEN = 16384;
		if(size + 2 + k_MAX_PACKET_LEN > static_cast<sint32>(mapBuf.size())) {
			mapBuf.resize(size + 2 + k_MAX_PACKET_LEN);
		}

		packet->Packetize(&mapBuf[size + 2], len);
		Assert(len < k_MAX_PACKET_LEN);

		putshort(&mapBuf[size], len);
		size += len + 2;

		if(len >= 256) {

			mapBuf.resize(mapBuf.size() + len - 256);
		}


	}

	Packetizer *chunk = std::make_unique<Packetizer>(mapBuf.data(), size).release();
	QueuePacket(id, chunk);

}

void Network::DechunkList(sint32 from, uint8 *buf, sint32 len)
{

	sint32 pos;
	pos = 0;
	while(pos < len) {
		sint32 packLen;
		packLen = getshort(&buf[pos]); pos += 2;
		PacketReady(from, &buf[pos], packLen);
		if(!m_initialized)
			return;
		pos += packLen;
	}
	Assert(pos == len);
}

void Network::SetCRCError()
{
	m_crcError = TRUE;
}

void Network::SetMaxPlayers(sint32 maxPlayers)
{
	m_netIO->SetMaxPlayers(static_cast<uint16>(maxPlayers), CountOpenSlots() > 0);
}

void Network::SetRobotName(sint32 player)
{
	if(!player_Get(player))
		return;

	Civilisation *civ = player_Get(player)->m_civilisation.get();
	StringId strId;
	if(civ->GetGender() == GENDER_MALE) {
		strId = g_theCivilisationDB->Get(civ->GetCivilisation())->GetLeaderNameMale();
	} else {
		strId = g_theCivilisationDB->Get(civ->GetCivilisation())->GetLeaderNameFemale();
	}
	civ->AccessData()->SetLeaderName(stringdb_Get()->GetNameStr(strId));

	QueuePacketToAll(std::make_unique<NetSetLeaderName>(player).release());
	if(g_networkPlayersScreen) {
		g_networkPlayersScreen->UpdateData();
	}
}

void Network::SendCity(CityData *cd)
{
	SendToServer(std::make_unique<NetCity>(cd->GetHomeCity().AccessData(), TRUE).release());
	SendToServer(std::make_unique<NetCity2>(cd, TRUE).release());
	SendToServer(std::make_unique<NetCityBuildQueue>(cd).release());
	SendToServer(std::make_unique<NetHappy>(cd->GetHomeCity(),
							  cd->GetHappy(), TRUE).release());
}

void Network::NotifyDiplomacyResponse(Response &response, sint32 p1, sint32 p2)
{
	if(!network_Get().IsActive()) return;

	if(IsHost()) {
		QueuePacketToAll(std::make_unique<NetDipResponse>(response, p1, p2).release());
	} else if(IsLocalPlayer(p1)) {
		SendToServer(std::make_unique<NetDipResponse>(response, p1, p2).release());
	}
}

void Network::NotifyDiplomacyThreatRejected(Response &response, const Response &sender_response, sint32 p1, sint32 p2)
{
}

void Network::SendGroupRequest(const CellUnitList &units, const Army &army)
{
	SendToServer(std::make_unique<NetGroupRequest>(units, army).release());
}

void Network::SendUngroupRequest(const Army &army, const CellUnitList &units)
{
	SendToServer(std::make_unique<NetUngroupRequest>(army, units).release());
}

void network_VerifyGameData()
{

	sint32 p;
	sint32 u;
	for(p = 0; p < k_MAX_PLAYERS; p++) {
		if(!player_Get(p)) continue;

		for(u = 0; u < player_Get(p)->m_all_units->Num(); u++) {
			if(!player_Get(p)->m_all_units->Access(u).IsValid()) {
				network_Get().RequestResync(RESYNC_INVALID_UNIT);
				return;
			}
		}

		sint32 a;
		for(a = 0; a < player_Get(p)->m_all_armies->Num(); a++) {
			if(!player_Get(p)->m_all_armies->Access(a).IsValid()) {
				network_Get().RequestResync(RESYNC_INVALID_ARMY_OTHER);
				return;
			}
		}

		sint32 c;
		for(c = 0; c < player_Get(p)->m_all_cities->Num(); c++) {
			if(!player_Get(p)->m_all_cities->Access(c).IsValid()) {
				network_Get().RequestResync(RESYNC_INVALID_UNIT);
				return;
			}
		}
	}

	sint32 x;
	sint32 y;
	for(x = 0; x < world_Get()->GetXWidth(); x++) {
		for(y = 0; y < world_Get()->GetYHeight(); y++) {
			Cell *cell = world_Get()->GetCell(x, y);
			for(u = 0; u < cell->GetNumUnits(); u++) {
				if(!cell->AccessUnit(u).IsValid()) {
					network_Get().RequestResync(RESYNC_INVALID_UNIT);
					return;
				}
				if(cell->GetCity().m_id != 0 && !cell->GetCity().IsValid()) {
					network_Get().RequestResync(RESYNC_INVALID_UNIT);
					return;
				}
			}
		}
	}
}
