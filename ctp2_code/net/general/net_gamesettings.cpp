#include "ctp/c3.h"
#include "net/general/net_gamesettings.h"
#include "net/io/net_util.h"
#include "net/general/network.h"

#include "gs/gameobj/XY_Coordinates.h"
#include "gs/world/World.h"
#include "gs/gameobj/Player.h"
#include "gfx/tilesys/tiledmap.h"
#include "gs/utility/UnitDynArr.h"
#include "gs/utility/QuadTree.h"
#include "ui/aui_ctp2/SelItem.h"
#include "ui/aui_ctp2/background.h"
#include "ui/aui_ctp2/radarmap.h"
#include "gs/gameobj/installationtree.h"
#include "ui/interface/radarwindow.h"
#include "ui/aui_ctp2/c3ui.h"
#include "gs/utility/gameinit.h"

#include "gs/gameobj/GameSettings.h"

#include "ui/interface/controlpanelwindow.h"

extern Background			*g_background;

extern ControlPanelWindow	*g_controlPanel;

NetGameSettings::NetGameSettings(sint32 x, sint32 y,
								 sint32 numPlayers,
								 uint8 gameStyle,
								 sint32 movesPerSlice,
								 time_t totalTime,
								 time_t turnTime,
								 time_t cityTime)
{
	m_x = x;
	m_y = y;
	m_numPlayers = numPlayers;
	m_gameStyle = gameStyle;
	m_movesPerSlice = movesPerSlice;
	m_totalTime = totalTime;
	m_turnTime = turnTime;
	m_cityTime = cityTime;
}

void NetGameSettings::Packetize(uint8 *buf, uint16 &size)
{
	size = 0;
	PUSHID(k_PACKET_GAME_SETTINGS_ID);

	PUSHLONG(m_x);
	PUSHLONG(m_y);
	PUSHLONG(m_numPlayers);
	PUSHBYTE(m_gameStyle);
	PUSHLONG(m_movesPerSlice);
	PUSHLONG(m_totalTime);
	PUSHLONG(m_turnTime);
	PUSHLONG(m_cityTime);
	uint32 playerMask = 0;
	for(sint32 i = 0; i < k_MAX_PLAYERS; i++) {
		if(player_Get(i))
			playerMask |= (1 << i);
	}
	PUSHLONG(playerMask);

	PUSHLONG(gamesettings_Get()->m_difficulty);
	PUSHLONG(gamesettings_Get()->m_risk);
	PUSHLONG(gamesettings_Get()->m_alienEndGame);
	PUSHLONG(gamesettings_Get()->m_pollution);

	PUSHLONG(world_Get()->m_isYwrap);
	PUSHLONG(world_Get()->m_isXwrap);
}

void NetGameSettings::Unpacketize(uint16 id, uint8 *buf, uint16 size)
{
	uint16 packid;
	uint16 pos = 0;
	uint32 playerMask;

	PULLID(packid);
	Assert(packid == k_PACKET_GAME_SETTINGS_ID);

	PULLLONG(m_x);
	PULLLONG(m_y);
	PULLLONG(m_numPlayers);
	PULLBYTE(m_gameStyle);
	PULLLONG(m_movesPerSlice);
	PULLLONG(m_totalTime);
	PULLLONG(m_turnTime);
	PULLLONG(m_cityTime);
	PULLLONG(playerMask);

	PULLLONG(gamesettings_Get()->m_difficulty);
	PULLLONG(gamesettings_Get()->m_risk);
	PULLLONG(gamesettings_Get()->m_alienEndGame);
	PULLLONG(gamesettings_Get()->m_pollution);

	BOOL isYwrap, isXwrap;
	PULLLONG(isYwrap);
	PULLLONG(isXwrap);

	Assert(pos == size);

	if(g_controlPanel) {

	}

	tiledmap_Get()->CopyVision();

	for(sint32 p = 0; p < k_MAX_PLAYERS; p++) {
		if(player_Get(p)) {
			player_Get(p)->m_all_armies->FastKillList();
			player_Get(p)->GetAllUnitList()->FastKillList();
			player_Get(p)->GetAllCitiesList()->FastKillList();
			player_Get(p)->GetTradersList()->FastKillList();
			player_Get(p)->m_vision->Clear();
		}
	}
	unit_tree_Get()->Clear();
	installation_tree_Get()->Clear();
	delete unit_tree_Get();
	unit_tree_Set(NULL);
	delete installation_tree_Get();
	installation_tree_Set(NULL);

	g_network.ClearDeadUnits();













	world_Get()->Reset(sint16(m_x), sint16(m_y), isYwrap, isXwrap);

	sint32 i;
	for(i = 0; i < k_MAX_PLAYERS; i++) {
		if(player_Get(i)) {
			delete player_Get(i);
		}
	}
	delete [] player_arr_Get();

	player_arr_Set(new Player *[k_MAX_PLAYERS]);
	for(i = 0; i < k_MAX_PLAYERS; i++) {
		player_arr_Get()[i] = NULL;
	}

	for(i = 0; i < k_MAX_PLAYERS; i++) {
		if(playerMask & (1 << i)) {
			player_arr_Get()[i] = new Player(i, 0, PLAYER_TYPE_HUMAN);
		}
	}

	delete selitem_Get();
	selitem_Set(new SelectedItem(m_numPlayers));

	unit_tree_Set(new QuadTree<Unit>((sint16)world_Get()->GetXWidth(),
									   (sint16)world_Get()->GetYHeight(),
									   world_Get()->IsYwrap()));
	installation_tree_Set(new InstallationQuadTree((sint16)world_Get()->GetXWidth(),
													 (sint16)world_Get()->GetYHeight(),
													 world_Get()->IsYwrap()));

	g_network.SetStyleFromServer(m_gameStyle, m_movesPerSlice, m_totalTime, m_turnTime, m_cityTime);

	tiledmap_Get()->CopyVision();

	gameinit_ResetForNetwork();

	g_network.SetLoop(TRUE);
}
