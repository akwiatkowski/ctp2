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

#include "ui/netshell/netshell.h"
#include "ui/netshell/ns_player.h"

ns_Player::ns_Player(NETFunc::Player * player)
:	ns_Object<NETFunc::Player, ns_Player>(player)
{
	list.emplace_back(ICON,		&m_mute);
	list.emplace_back(STRING,	&m_name);
	list.emplace_back(INT,		&m_ping);
};

void ns_Player::Update( NETFunc::Player *player ) {
	SetMine(player->IsMe());
	m_mute = player->IsMuted() ? netshell_Get()->GetTrueBmp() : nullptr;
	m_name = player->GetName();
	m_ping = player->GetLatency();
}
