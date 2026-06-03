//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  :
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
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Corrected strange access of non-static members from static data.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "ui/aui_common/aui_stringtable.h"
#include "ui/netshell/netshell.h"
#include "ui/netshell/ns_gplayer.h"

#include "ui/netshell/ns_tribes.h"
#include "ui/netshell/ns_playersetup.h"


ns_GPlayer::ns_GPlayer(NETFunc::Player * player)
:	ns_Object<NETFunc::Player, ns_GPlayer>(player)
{
	list.emplace_back(ICON,		&m_host);
	list.emplace_back(ICON,		&m_launched);
	list.emplace_back(STRING,	&m_name);
	list.emplace_back(INT,		&m_ping);
	list.emplace_back(STRING,	&m_tribe);
	list.emplace_back(INT,		&m_civpoints);
	list.emplace_back(INT,		&m_pwpoints);
};


void ns_GPlayer::Update( NETFunc::Player *player ) {
	SetMine(player->IsMe());
	m_launched = player->IsReadyToLaunch() ? netshell_Get()->GetTrueBmp() : nullptr;
	m_host = player->IsHost() ? netshell_Get()->GetTrueBmp() : nullptr;
	m_name = player->GetName();
	m_ping = player->GetLatency();


	nf_PlayerSetup ps( player );
	m_tribe = nstribes_Get()->GetStrings()->GetString( ps.GetTribe() );
	m_civpoints = ps.GetCivPoints();
	m_pwpoints = ps.GetPwPoints();
}
