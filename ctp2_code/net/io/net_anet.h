#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef _ANETIO_H_
#define _ANETIO_H_

#include "net/io/net_io.h"
#include "libs/anet/h/anet.h"
#include "net/io/net_array.h"
#include "ctp/ctp2_utils/pointerlist.h"

#define CIV3_SPECIES 1504

enum SESSION_STATE {
	SESSION_STATE_READY,
	SESSION_STATE_CREATE_PLAYER
};

enum ANET_STATE {
	ANET_STATE_READY,
	ANET_STATE_CONTACTING_LOBBY,
};

class AnetPlayerData {
public:
	AnetPlayerData(dpid_t id, char* name) :
		m_id(id),
		m_name(name)
	{
	};

	dpid_t m_id;
	char* m_name;
};

class ActivNetIO : public NetIO
{
public:
	ActivNetIO();
	~ActivNetIO() override;

	void SetDP(dp_t *dp);

	NET_ERR EnumTransports() override;
	NET_ERR SetTransport(sint32 trans_id) override;
	NET_ERR Host(char* sessionName) override;
	NET_ERR EnumSessions() override;
	NET_ERR Join(sint32 index) override;
	NET_ERR GetMyId(uint16& id) override;
	NET_ERR GetHostId(uint16& id) override;
	NET_ERR EnumPlayers() override;
	NET_ERR Send(uint16 id, sint32 flags, uint8* buf, sint32 len) override;
	NET_ERR Idle() override;
	NET_ERR SetName(char* name) override;
	NET_ERR SetLobby(char* serverName) override;
	NET_ERR SetMaxPlayers(uint16 players, bool lock) override;
	NET_ERR KickPlayer(uint16 player) override;
	NET_ERR Reset() override;

	BOOL    ReadyForData() override;
	BOOL    IsHost() const { return m_isHost; }

private:
	NetArray m_transports;
	NetArray m_sessions;

	PointerList<AnetPlayerData> m_playerList;

	dp_t* m_dp;
	dpid_t m_pid;
	dpid_t m_hostId;
	sint32 m_isHost;
	sint32 m_broadcastAddMessage;
	time_t m_broadcastAddMessageTime;

	dp_session_t m_session;

	SESSION_STATE m_sessionState;
	ANET_STATE m_state;

	char* m_name;

	BOOL m_got_end_players;

	void TransportCallback(const dp_transport_t* fname,
					   const comm_driverInfo_t* description);
	friend void dp_PASCAL anet_EnumTransportsCallback(const dp_transport_t *,
									const comm_driverInfo_t *,
									void* context);

	sint32 SessionCallback(dp_session_t* sDesc,
						  long * pTimeout,
						  long flags);
	friend int dp_PASCAL anet_EnumSessionsCallback(dp_session_t *sDesc,
									 long * pTimeout,
									 long flags,
									 void* context);

	sint32 SessionReadyCallback(dp_session_t* ps,
							   long * pTimeout,
							   long flags);
	friend int dp_PASCAL anet_CreateSessionCallback(dp_session_t *ps,
							 long *pTimeout,
							 long flags,
							 void* context);

	void PlayerReady(dpid_t id, dp_char_t *name, sint32 flags);
	friend void dp_PASCAL anet_PlayerReadyCallback(dpid_t id, dp_char_t *name,
							long flags, void *context);

	void PlayerCallback(dpid_t id, dp_char_t *name,	long flags);
	friend void dp_PASCAL anet_EnumPlayers(dpid_t id,
						dp_char_t *name,
						long flags,
						void *context);
};

#endif
