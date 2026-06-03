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
#include "ui/netshell/ns_game.h"


ns_Game::ns_Game(NETFunc::Game * game)
:	ns_Object<NETFunc::Game, ns_Game>(game)
{
	list.emplace_back(ICON,		&m_launched);
	list.emplace_back(STRING,	&m_name);
	list.emplace_back(ICON,		&m_locked);
	list.emplace_back(ICON,		&m_closed);
	list.emplace_back(INT,		&m_players);
};

void ns_Game::Update( NETFunc::Game *game ) {
	SetMine(game->IsCurrentSession());
	m_locked = strlen(game->GetPassword()) ? netshell_Get()->GetTrueBmp() : nullptr;
	m_closed = (game->IsClosed() || game->IsHostile() || game->GetFree() == 0) ? netshell_Get()->GetTrueBmp() : nullptr;
	m_launched = game->IsLaunched() ? netshell_Get()->GetTrueBmp() : nullptr;
	m_name = game->GetName();
	m_players = game->GetPlayers();

}
