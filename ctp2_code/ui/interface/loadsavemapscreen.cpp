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
// None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Prevented memory leak report at the end of the program.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include <memory>
#include "ui/interface/loadsavemapwindow.h"

#include "ctp/civ3_main.h"
#include "ctp/civapp.h"
#include "gs/fileio/gamefile.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_stringtable.h"
#include "ui/aui_common/aui_textfield.h"
#include "ui/netshell/ns_gamesetup.h"

#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/c3_button.h"
#include "ui/aui_ctp2/c3_dropdown.h"
#include "ui/aui_ctp2/c3_listitem.h"
#include "ui/aui_ctp2/c3_listbox.h"

#include "ui/interface/initialplaywindow.h"
#include "ui/interface/spnewgamewindow.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "ui/netshell/netshell.h"               // gamesetup_Get()

extern std::unique_ptr<SPNewGameWindow> g_spNewGameWindow;

// Owned by s_loadSaveMapWindowOwner; g_loadSaveMapWindow stays raw because it
// is extern'd in loadsavemapwindow.cpp and spnewgamewindow.cpp.
static std::unique_ptr<LoadSaveMapWindow> s_loadSaveMapWindowOwner;
LoadSaveMapWindow				*g_loadSaveMapWindow = nullptr;


sint32	loadsavemapscreen_displayMyWindow(uint32 type)
{
	AUI_ERRCODE const   retval = loadsavemapscreen_Initialize();

	if (g_loadSaveMapWindow)
    {
	    g_loadSaveMapWindow->CleanUpSaveMapInfo();
		g_loadSaveMapWindow->SetType(type);

        Assert(c3ui_Get());
		c3ui_Get()->AddWindow(g_loadSaveMapWindow);
        c3ui_Get()->RegisterCleanup(&loadsavemapscreen_Cleanup);
	}

	return static_cast<sint32>(retval);
}
sint32 loadsavemapscreen_removeMyWindow(uint32 action)
{
	if (action != (uint32)AUI_BUTTON_ACTION_EXECUTE) return 0;

	AUI_ERRCODE auiErr = c3ui_Get()->RemoveWindow(g_loadSaveMapWindow->Id());
	Assert(auiErr == AUI_ERRCODE_OK);

	return 1;
}


AUI_ERRCODE loadsavemapscreen_Initialize( aui_Control::ControlActionCallback *callback )
{
	if ( g_loadSaveMapWindow ) return AUI_ERRCODE_OK;

	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		windowBlock[k_AUI_LDL_MAXBLOCK + 1];
	strlcpy(windowBlock, "LoadSaveMapWindow", sizeof(windowBlock));

	s_loadSaveMapWindowOwner = std::make_unique<LoadSaveMapWindow>(&errcode, aui_UniqueId(), windowBlock, 16 , AUI_WINDOW_TYPE_FLOATING);
	g_loadSaveMapWindow = s_loadSaveMapWindowOwner.get();
	Assert( AUI_NEWOK(g_loadSaveMapWindow, errcode) );
	if ( !AUI_NEWOK(g_loadSaveMapWindow, errcode) ) return errcode;

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );

	switch ( g_loadSaveMapWindow->GetType() )
	{
	case LSMS_LOAD_GAMEMAP:
		g_loadSaveMapWindow->GetOkButton()->Enable( FALSE );
		break;

	default:
		g_loadSaveMapWindow->GetOkButton()->Enable( TRUE );
		break;
	}

	g_loadSaveMapWindow->GetDeleteButton()->Enable( FALSE );
	g_loadSaveMapWindow->GetListOne()->GetHeader()->Enable( FALSE );
	g_loadSaveMapWindow->GetListTwo()->GetHeader()->Enable( FALSE );

	if ( callback )
		g_loadSaveMapWindow->GetOkButton()->SetActionFuncAndCookie(
			callback, nullptr );

	return AUI_ERRCODE_OK;
}




void loadsavemapscreen_Cleanup()
{
    if (c3ui_Get() && g_loadSaveMapWindow)
    {
        c3ui_Get()->RemoveWindow(g_loadSaveMapWindow->Id());
    }

    s_loadSaveMapWindowOwner.reset();
    g_loadSaveMapWindow = nullptr;
}




void loadsavemapscreen_LoadGameMap()
{
	GameMapInfo	*gameMapInfo = g_loadSaveMapWindow->GetGameMapInfo();
	SaveMapInfo	*saveMapInfo = g_loadSaveMapWindow->GetSaveMapInfo();

	Assert(gameMapInfo);
	if (!gameMapInfo) return;

	Assert(saveMapInfo);
	if (!saveMapInfo) return;

	MBCHAR		path[_MAX_PATH];

	snprintf(path, sizeof(path), "%s" FILE_SEP "%s", gameMapInfo->path, saveMapInfo->fileName);


}



void loadsavemapscreen_SaveGameMap()
{
	SaveMapInfo		*saveMapInfo = g_loadSaveMapWindow->GetSaveMapInfoToSave();

	Assert( saveMapInfo != nullptr );
	if ( !saveMapInfo ) return;

	if (!g_loadSaveMapWindow->GetGameMapName(saveMapInfo->gameMapName)) return;
	if (!g_loadSaveMapWindow->GetSaveMapName(saveMapInfo->fileName)) return;
	if (!g_loadSaveMapWindow->GetNote(saveMapInfo->note)) return;

	MBCHAR	path[_MAX_PATH];
	MBCHAR	fullPath[_MAX_PATH];

	if (!civpaths_Get()->GetSavePath(C3SAVEDIR_MAP, path)) return;

	snprintf(fullPath, sizeof(fullPath), "%s" FILE_SEP "%s", path, saveMapInfo->gameMapName);

	if (!c3files_PathIsValid(fullPath)) {
		if (!c3files_CreateDirectory(fullPath)) {
			Assert(FALSE);


			return;
		}
	}

	snprintf(saveMapInfo->pathName, sizeof(saveMapInfo->pathName), "%s" FILE_SEP "%s", fullPath, saveMapInfo->fileName);

	g_loadSaveMapWindow->GetRadarMap(saveMapInfo);

	GameMapFile::SaveGameMap(saveMapInfo->pathName, saveMapInfo);
}





void loadsavemapscreen_executePress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{


	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	if(loadsavemapscreen_removeMyWindow(action)) {
		switch(g_loadSaveMapWindow->GetType()) {
			case LSMS_LOAD_GAMEMAP:		loadsavemapscreen_LoadGameMap();			break;
			case LSMS_SAVE_GAMEMAP:		loadsavemapscreen_SaveGameMap();			break;
			default:
				Assert(0);
				break;
		}
	}

	spnewgamescreen_update();
}


void loadsavemapscreen_backPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;





	loadsavemapscreen_removeMyWindow(action) ;

	if ( g_spNewGameWindow ) g_spNewGameWindow->m_useCustomMap = false;
	spnewgamescreen_update();
}


void loadsavemapscreen_deletePress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	GameMapInfo	*gameMapInfo = g_loadSaveMapWindow->GetGameMapInfo();
	SaveMapInfo	*saveMapInfo = g_loadSaveMapWindow->GetSaveMapInfo();

	Assert(gameMapInfo);
	if (!gameMapInfo) return;

	Assert(saveMapInfo);
	if (!saveMapInfo) return;

	MBCHAR		path[_MAX_PATH];

	snprintf(path, sizeof(path), "%s" FILE_SEP "%s", gameMapInfo->path, saveMapInfo->fileName);
#ifdef WIN32
	if ( DeleteFile( path ) )
#else // WIN32
	if (!unlink(path))
#endif // WIN32
	{


		sint32 one = g_loadSaveMapWindow->GetListOne()->GetSelectedItemIndex();
		sint32 two = g_loadSaveMapWindow->GetListTwo()->GetSelectedItemIndex();

		g_loadSaveMapWindow->SetType( g_loadSaveMapWindow->GetType() );

		g_loadSaveMapWindow->GetListOne()->SelectItem( one );

		if ( two && two == g_loadSaveMapWindow->GetListTwo()->NumItems() ) --two;
		g_loadSaveMapWindow->GetListTwo()->SelectItem( two );








	}
	else
	{
		Assert( "Couldn't delete file." == nullptr );
	}
}


void loadsavemapscreen_ListOneHandler(aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_LISTBOX_ACTION_SELECT ) return;

	c3_ListBox	*list = (c3_ListBox *)control;
	if (list == nullptr) return;

	LSMGameMapsListItem *item = (LSMGameMapsListItem *)list->GetSelectedItem();
	if (item == nullptr)
	{
		g_loadSaveMapWindow->SetGameMapInfo(nullptr);

		g_loadSaveMapWindow->SetType( g_loadSaveMapWindow->GetType() );
	}
	else
	{

		g_loadSaveMapWindow->SetGameMapInfo(item->GetGameMapInfo());
	}

	if ( !g_loadSaveMapWindow->GetListTwo()->GetSelectedItem() )
	{
		switch ( g_loadSaveMapWindow->GetType() )
		{
		case LSMS_LOAD_GAMEMAP:
			g_loadSaveMapWindow->GetOkButton()->Enable( FALSE );
			break;

		default:
			g_loadSaveMapWindow->GetOkButton()->Enable( TRUE );
			break;
		}

		g_loadSaveMapWindow->GetDeleteButton()->Enable( FALSE );
	}
}


void loadsavemapscreen_ListTwoHandler(aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_LISTBOX_ACTION_SELECT ) return;

	c3_ListBox	*list = (c3_ListBox *)control;
	if (list == nullptr) return;

	LSMSaveMapsListItem *item = (LSMSaveMapsListItem *)list->GetSelectedItem();
	if (item == nullptr)
	{
		switch ( g_loadSaveMapWindow->GetType() )
		{
		case LSMS_LOAD_GAMEMAP:
			g_loadSaveMapWindow->GetOkButton()->Enable( FALSE );
			break;

		default:
			g_loadSaveMapWindow->GetOkButton()->Enable( TRUE );
			break;
		}

		g_loadSaveMapWindow->SetSaveMapInfo(nullptr);

		g_loadSaveMapWindow->SetType( g_loadSaveMapWindow->GetType() );

		g_loadSaveMapWindow->GetDeleteButton()->Enable( FALSE );
	}
	else
	{
		SaveMapInfo	*info = item->GetSaveMapInfo();
		if (info == nullptr) return;

		g_loadSaveMapWindow->SetSaveMapInfo(info);

		g_loadSaveMapWindow->GetOkButton()->Enable( TRUE );
		g_loadSaveMapWindow->GetDeleteButton()->Enable( TRUE );
	}
}
