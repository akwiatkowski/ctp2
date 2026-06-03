//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Victory movie window
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
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_button.h"

#include "ui/aui_ctp2/c3window.h"

#include "ui/interface/victorymoviewindow.h"
#include "ui/interface/victorymoviewin.h"
#include "gs/gameobj/GameOver.h"
#include "ui/interface/victorywin.h"
#include "ui/interface/victorywindow.h"
#include "ui/interface/infowin.h"
#include "ui/interface/EndgameWindow.h"

#include "gs/database/moviedb.h"
#include "gs/database/StrDB.h"
#include "AgeRecord.h"
#include "WonderRecord.h"

#include "ui/interface/screenutils.h"
#include "gs/gameobj/EndGame.h"

#include "gfx/spritesys/director.h"
extern MovieDB			*g_theVictoryMovieDB;

#include "sound/soundmanager.h"
extern SoundManager		*soundmgr_Get();

#include "ui/aui_ctp2/SelItem.h"
extern SelectedItem		*selitem_Get();

#include "gs/gameobj/Player.h"

VictoryMovieWindow		*g_victoryMovieWindow = nullptr;

static GAME_OVER		s_result;


void victorymoviewin_Initialize(SequenceWeakPtr seq)
{
	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;

	if (g_victoryMovieWindow == nullptr) {
		g_victoryMovieWindow = new VictoryMovieWindow(&errcode, aui_UniqueId(), "VictoryMovieWindow", 16);
		Assert(errcode == AUI_ERRCODE_OK);
		if (errcode != AUI_ERRCODE_OK)
			g_victoryMovieWindow = nullptr;
		Assert(g_victoryMovieWindow != nullptr);

	}

	g_victoryMovieWindow->SetSequence(seq);














}


void victorymoviewin_DisplayVictoryMovie(GAME_OVER reason)
{
	MBCHAR		*whichMovie;

	Assert(g_victoryMovieWindow != nullptr);
	if (g_victoryMovieWindow == nullptr) return;

	s_result = reason;

	switch (reason) {
	case GAME_OVER_LOST_OUT_OF_TIME :		whichMovie = "VICTORY_LOST_OUT_OF_TIME";
		break;
	case GAME_OVER_LOST_CONQUERED :			whichMovie = "VICTORY_LOST_CONQUERED";
		break;
	case GAME_OVER_LOST_DIPLOMACY :			whichMovie = "VICTORY_LOST_DIPLOMACY";
		break;
	case GAME_OVER_LOST_SCIENCE   :			whichMovie = "VICTORY_LOST_SCIENCE";
		break;
	case GAME_OVER_LOST_INEPT:				whichMovie = "VICTORY_LOST_INEPT";
		break;
	case GAME_OVER_LOST_SCENARIO:			whichMovie = "VICTORY_LOST_SCENARIO";
		break;
	case GAME_OVER_WON_SCENARIO:			whichMovie = "VICTORY_WON_SCENARIO";
		break;
	case GAME_OVER_WON_CONQUERED_WORLD :	whichMovie = "VICTORY_WON_CONQUERED_WORLD";
		break;
	case GAME_OVER_WON_DIPLOMACY :			whichMovie = "VICTORY_WON_DIPLOMACY";
		break;
	case GAME_OVER_WON_SCIENCE :			whichMovie = "VICTORY_WON_SCIENCE";
		break;

	case GAME_OVER_WON_OUT_OF_TIME:			whichMovie = "VICTORY_WON_CONQUERED_WORLD";
		break;

    default:
        {
            BOOL I_DONT_KNOW_WHAT_MOVIE_YOU_WANT_TO_SEE=0;
            Assert(I_DONT_KNOW_WHAT_MOVIE_YOU_WANT_TO_SEE);
            whichMovie = "VICTORY_LOST_INEPT";
        }
	}

	if (soundmgr_Get()) {
		soundmgr_Get()->TerminateAllSounds();
		soundmgr_Get()->TerminateMusic();
	}

	sint32 index = g_theVictoryMovieDB->FindTypeIndex(whichMovie);
	Assert(index >= 0);

	MBCHAR *filename = g_theVictoryMovieDB->GetMovieFilename(index);
	g_victoryMovieWindow->SetMovie(filename);

	AUI_ERRCODE		errcode;

	errcode = c3ui_Get()->AddWindow(g_victoryMovieWindow);
	Assert(errcode == AUI_ERRCODE_OK);

}


void victorymoviewin_Cleanup()
{
	SequenceWeakPtr	seq;

	if (g_victoryMovieWindow) {
		seq = g_victoryMovieWindow->GetSequence();

		c3ui_Get()->RemoveWindow(g_victoryMovieWindow->Id());

		delete g_victoryMovieWindow;
		g_victoryMovieWindow = nullptr;
	}

	director_Get()->ActionFinished(seq);
}


void victorymoviewin_MovieButtonCallback(aui_Control *control, uint32 action, uint32 data, void * cookie)
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	c3ui_Get()->AddAction(new CloseVictoryMovieAction);
}


void CloseVictoryMovieAction::Execute(aui_Control *control, uint32 action, uint32 data)
{

	victorymoviewin_Cleanup();


	sint32 type = 0;
	switch (s_result) {
	case GAME_OVER_LOST_OUT_OF_TIME :
	case GAME_OVER_LOST_CONQUERED :
	case GAME_OVER_LOST_SCIENCE :
	case GAME_OVER_LOST_DIPLOMACY :
	case GAME_OVER_LOST_INEPT:
	case GAME_OVER_LOST_SCENARIO:
		type = k_VICWIN_DEFEAT;
		break;
	case GAME_OVER_WON_OUT_OF_TIME:
	case GAME_OVER_WON_SCENARIO:
	case GAME_OVER_WON_CONQUERED_WORLD :
	case GAME_OVER_WON_SCIENCE:
	case GAME_OVER_WON_DIPLOMACY:
		type = k_VICWIN_VICTORY;
		break;
	default:
		Assert(0);
	}


	infowin_Cleanup();

	infowin_Initialize();

	victorywin_Initialize(type);

	victorywin_DisplayWindow(type);

	if (s_result == GAME_OVER_WON_SCIENCE) {
		EndGame *endGame = nullptr;

		sint32 p = selitem_Get()->GetVisiblePlayer();
		if (player_Get(p) != nullptr)
			endGame = player_Get(p)->m_endGame;


	}




}
