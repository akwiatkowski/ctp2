//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : The intro movie window
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
// - Initialized local variables. (Sep 9th 2005 Martin Gühmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_button.h"

#include "ui/aui_ctp2/c3window.h"

#include "ui/interface/IntroMovieWindow.h"
#include "ui/interface/IntroMovieWin.h"

#include "gs/database/moviedb.h"
#include "gs/database/StrDB.h"
#include "AgeRecord.h"
#include "WonderRecord.h"

#include "ui/interface/initialplaywindow.h"
#include "sound/soundmanager.h"

#include "gs/gameobj/wonderutil.h"

extern MovieDB			*g_theVictoryMovieDB;


IntroMovieWindow		*g_introMovieWindow = nullptr;


void intromoviewin_Initialize()
{
	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;

	if (g_introMovieWindow == nullptr) {
		g_introMovieWindow = new IntroMovieWindow(&errcode, aui_UniqueId(), "IntroMovieWindow", 16);
		Assert(errcode == AUI_ERRCODE_OK);
		if (errcode != AUI_ERRCODE_OK)
			g_introMovieWindow = nullptr;
		Assert(g_introMovieWindow != nullptr);
	}
}


void intromoviewin_DisplayIntroMovie()
{
	MBCHAR		*whichMovie;

	Assert(g_introMovieWindow != nullptr);
	if (g_introMovieWindow == nullptr) return;

	whichMovie = "VICTORY_INTRO";

	sint32 index = g_theVictoryMovieDB->FindTypeIndex(whichMovie);
	Assert(index >= 0);

	MBCHAR *filename = g_theVictoryMovieDB->GetMovieFilename(index);
	g_introMovieWindow->SetMovie(filename);

	AUI_ERRCODE		errcode;

	errcode = c3ui_Get()->AddWindow(g_introMovieWindow);
	Assert(errcode == AUI_ERRCODE_OK);

}


void intromoviewin_Cleanup()
{
	if (g_introMovieWindow) {

		c3ui_Get()->RemoveWindow(g_introMovieWindow->Id());

		delete g_introMovieWindow;
		g_introMovieWindow = nullptr;
	}
}


void intromoviewin_MovieButtonCallback(aui_Control *control, uint32 action, uint32 data, void * cookie)
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	c3ui_Get()->AddAction(new CloseIntroMovieAction);
}


void CloseIntroMovieAction::Execute(aui_Control *control, uint32 action, uint32 data)
{

	intromoviewin_Cleanup();

	AUI_ERRCODE errcode;

	if (soundmgr_Get()) {
		soundmgr_Get()->EnableMusic();
		soundmgr_Get()->PickNextTrack();
		soundmgr_Get()->StartMusic();
	}

	errcode = initialplayscreen_Initialize();
	Assert(errcode == AUI_ERRCODE_OK);

	initialplayscreen_displayMyWindow();

}
