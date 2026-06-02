#include "ctp/c3.h"

#include "ui/interface/EndTurnButton.h"

#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_ctp2/ctp2_button.h"
#include "gs/utility/newturncount.h"
#include "ui/aui_ctp2/SelItem.h"

#include "ui/interface/AttractWindow.h"
#include "gfx/spritesys/director.h"

EndTurnButton::EndTurnButton(MBCHAR *ldlBlock) :
m_endTurn(static_cast<ctp2_Button*>(aui_Ldl::GetObject(ldlBlock, "TurnButton")))
{

	Assert(m_endTurn);

	m_endTurn->SetActionFuncAndCookie(EndTurnButtonActionCallback, this);

	m_endTurn->Enable(false);
}


void EndTurnButton::UpdatePlayer(PLAYER_INDEX player)
{

	if(selitem_Get()->GetVisiblePlayer() == player)
		m_endTurn->Enable(true);
	else
		m_endTurn->Enable(false);

	attractwindow_Get()->RemoveRegion(m_endTurn);
}

void EndTurnButton::EndTurnButtonActionCallback(aui_Control *control, uint32 action,
												uint32 data, void *cookie)
{

	if(action != static_cast<uint32>(AUI_BUTTON_ACTION_EXECUTE))
		return;







	DPRINTF(k_DBG_GAMESTATE, ("Button end turn, %d\n", selitem_Get()->GetCurPlayer()));
	if((selitem_Get()->GetCurPlayer() != selitem_Get()->GetVisiblePlayer())) {
		DPRINTF(k_DBG_GAMESTATE, ("But not my turn!\n"));
		return;
	}

	selitem_Get()->RegisterManualEndTurn();
	director_Get()->AddEndTurn();
}
