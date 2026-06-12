//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Turn display
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
// - None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Crash prevented.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/interface/TurnYearStatus.h"

#include <sstream>
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_ctp2/ctp2_button.h"
#include "gs/utility/TurnCnt.h"
#include "gs/database/StrDB.h"
#include "ui/aui_ctp2/ctp2_Static.h"
#include "ui/aui_ctp2/SelItem.h"
#include "gs/gameobj/Player.h"
#include "ui/aui_utils/primitives.h"
#include "gfx/gfx_utils/colorset.h"           // colorset_Get()
#include "gs/fileio/gamefile.h"           // is_scenario_Get()
#include "gs/fileio/CivPaths.h"           // civpaths_Get()

sTurnLengthOverride *TurnYearStatus::s_pTurnLengthOverride    = nullptr;
uint32               TurnYearStatus::s_turnLengthOverrideSize = 0;
bool                 TurnYearStatus::s_useCustomYear          = false;

const MBCHAR *TurnYearStatus::GetCurrentYear()
{
	sint32 currentYear = turn_Get()->GetSessionYear();

	sint32 round       = player_Get(selitem_Get()->GetVisiblePlayer()) ?
	                     player_Get(selitem_Get()->GetVisiblePlayer())->m_current_round :
	                     turn_Get()->GetSessionRound();

	return TurnYearStatus::GetYearString(currentYear, round);
}

const MBCHAR *TurnYearStatus::GetYearString(sint32 currentYear, sint32 round)
{
	static MBCHAR buf[1024];

	if(s_useCustomYear && s_pTurnLengthOverride)
	{
		if(round >= 0)
		{
			if((unsigned) round >= s_turnLengthOverrideSize)
			{
				round = s_turnLengthOverrideSize - 1;
			}
			strlcpy(buf, s_pTurnLengthOverride[round].text, sizeof(buf));
		}
		else
		{
			buf[0] = 0;
		}
	}
	else
	{
		if(currentYear == 0)
		{
			currentYear = 1;
		}

		AUI_ERRCODE         errcode         = AUI_ERRCODE_OK;
		aui_StringTable *   table           = new aui_StringTable(&errcode, "YearStrings");
		sint32 const        yearStringIndex = (currentYear < 0) ? 0 : 1;
		snprintf(buf, sizeof(buf), "%ld%s", abs(currentYear), table->GetString(yearStringIndex));
		delete table;
	}

	return buf;

#if 0

	std::stringstream yearString;

	if (s_useCustomYear && g_pTurnLengthOverride)
	{
		uint32 round = turn_Get()->GetSessionRound();
		if (round > s_turnLengthOverrideSize)
		{
			round = s_turnLengthOverrideSize;
		}

		yearString << s_pTurnLengthOverride[round].text << std::ends;
	}
	else
	{

		yearString << abs(currentYear) << " "
			<< ((currentYear < 0) ?
			stringdb_Get()->GetNameStr("str_tbl_ldl_BC") :
			stringdb_Get()->GetNameStr("str_tbl_ldl_AD"))
			<< std::ends;
	}

	return(yearString.str());
#endif
}

const MBCHAR *TurnYearStatus::GetCurrentRound()
{
	static MBCHAR buf[1024];
	sint32 round = player_Get(selitem_Get()->GetVisiblePlayer()) ?
	                   player_Get(selitem_Get()->GetVisiblePlayer())->m_current_round :
	                   turn_Get()->GetSessionRound();
	snprintf(buf, sizeof(buf), "%d %s", round, stringdb_Get()->GetNameStr("str_ldl_Turns"));
	return buf;
#if 0

	std::stringstream roundString;
	roundString << turn_Get()->GetSessionRound() << " "
		<< stringdb_Get()->GetNameStr("str_ldl_Turns")
		<< std::ends;

	return(roundString.str());
#endif
}

void TurnYearStatus::BuildTurnLengthOverride()
{
	if (is_scenario_Get() || (scenario_name_buf() && *scenario_name_buf()))
	{
		MBCHAR overridePath[_MAX_PATH];
		snprintf(overridePath, sizeof(overridePath), "%s%s%s", civpaths_Get()->GetCurScenarioPath(), FILE_SEP, "turnlength.txt");

		s_useCustomYear = false;

		FILE *fp = fopen(overridePath, "r");
		if (fp)
		{
			sint32 count = 0;

			while (!feof(fp))
			{
				char dummy[256];
				if (fscanf(fp, "%[^'\n']\n", dummy))
				{
					count++;
				}
				else
				{
					break;
				}
			}

			if (count)
			{
				MBCHAR dummy[1024];
				s_pTurnLengthOverride = new sTurnLengthOverride[count];
				s_turnLengthOverrideSize = count;
				rewind(fp);
				for (int i = 0; i < count; i++)
				{
					fscanf(fp, "%d,%[^'\n']\n", &(s_pTurnLengthOverride[i].turn), &dummy);
					memset(s_pTurnLengthOverride[i].text, 0, 32);
					strlcpy(s_pTurnLengthOverride[i].text, dummy, sizeof(s_pTurnLengthOverride[i].text));
				}
			}

			s_useCustomYear = true;

			fclose(fp);
		}
	}
	else
	{
		CleanupTurnLengthOverride();
	}
}

void TurnYearStatus::CleanupTurnLengthOverride()
{
	s_useCustomYear = false;
	delete [] s_pTurnLengthOverride;
	s_pTurnLengthOverride = nullptr;
}

TurnYearStatus::TurnYearStatus(MBCHAR *ldlBlock)
:   m_turnYearStatus (static_cast<ctp2_Button*>(aui_Ldl::GetObject(ldlBlock, "TurnYearStatus"))),
    m_dougsProgress  (static_cast<ctp2_Static*>(aui_Ldl::GetObject(ldlBlock, "DougsProgressBar"))),
    m_displayType    (DISPLAY_YEAR)
{
	Assert(m_turnYearStatus);

	m_turnYearStatus->SetActionFuncAndCookie(TurnYearStatusActionCallback, this);

	m_dougsProgress->SetDrawCallbackAndCookie(DrawDougsProgress, this);
}

void TurnYearStatus::UpdatePlayer(PLAYER_INDEX player)
{
	Update();
}

void TurnYearStatus::Update()
{

	switch(m_displayType)
	{
		case DISPLAY_YEAR:
			if (s_useCustomYear && s_pTurnLengthOverride)
			{
				uint32 round = turn_Get()->GetSessionRound();
				if (round > s_turnLengthOverrideSize)
				{
					round = s_turnLengthOverrideSize;
				}

				m_turnYearStatus->SetText(s_pTurnLengthOverride[round].text);
			}
			else
			{
				m_turnYearStatus->SetText(GetCurrentYear());
			}
			break;
		case DISPLAY_TURN:

			m_turnYearStatus->SetText(GetCurrentRound());
			break;
		default:
			Assert(false);
			break;
	}
	m_dougsProgress->ShouldDraw(TRUE);
}

void TurnYearStatus::TurnYearStatusActionCallback(aui_Control *control, uint32 action,
												  uint32 data, void *cookie)
{

	if(action != static_cast<uint32>(AUI_BUTTON_ACTION_EXECUTE))
		return;


	TurnYearStatus *turnYearStatus = static_cast<TurnYearStatus*>(cookie);

	turnYearStatus->m_displayType = static_cast<DisplayType>(
		(turnYearStatus->m_displayType + 1) % NUMBER_OF_DISPLAY_TYPES);

	turnYearStatus->Update();
}

AUI_ERRCODE TurnYearStatus::DrawDougsProgress(ctp2_Static *control,
											  aui_Surface *surface,
											  RECT &rect,
											  void *cookie)
{

	if(selitem_Get() == nullptr)
		return AUI_ERRCODE_OK;

	if (nullptr == player_arr_Get())
	{
		return AUI_ERRCODE_OK;
	}

	if(!player_Get(selitem_Get()->GetVisiblePlayer())) {
		return AUI_ERRCODE_OK;
	}

	primitives_PaintRect16(surface, &rect, colorset_Get()->GetColor(COLOR_BLACK));
	if(selitem_Get()->GetVisiblePlayer() != selitem_Get()->GetCurPlayer()) {
		sint32 p;

		sint32 alive = 0;
		sint32 progress = 0;

		sint32 startp = selitem_Get()->GetVisiblePlayer() + 1;
		if(startp >= k_MAX_PLAYERS)
			startp = 0;
		for(p = startp; p != selitem_Get()->GetVisiblePlayer(); p++) {
			if(player_Get(p)) {
				alive++;
			}
			if(p == selitem_Get()->GetCurPlayer()) {
				progress = alive;
			}
			if(p == k_MAX_PLAYERS - 1) {

				p = -1;
			}
		}

		if(alive > 0) {
			RECT tmp = rect;
			sint32 width = tmp.right - tmp.left;
			sint32 displayWidth = (width * progress) / alive;
			if(displayWidth > width) {
				displayWidth = width;
			}
			tmp.right = tmp.left + displayWidth;
			primitives_PaintRect16(surface, &tmp, colorset_Get()->GetPlayerColor(selitem_Get()->GetCurPlayer()));
		}
	}
	return AUI_ERRCODE_OK;
}
