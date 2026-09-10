//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : The options window
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
// - Removed refferences to the civilisation database. (Aug 20th 2005 Martin Gühmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_surface.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_imagebase.h"
#include "ui/aui_common/aui_textbase.h"
#include "ui/aui_common/aui_textfield.h"
#include "ui/aui_common/aui_stringtable.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/c3_button.h"
#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/c3_listitem.h"
#include "ui/aui_ctp2/c3_dropdown.h"
#include "gs/database/StrDB.h"
#include "net/general/network.h"

#include "ui/interface/spnewgamewindow.h"
#include "ui/interface/optionswindow.h"












OptionsWindow::OptionsWindow( AUI_ERRCODE *retval, uint32 id, MBCHAR *ldlBlock, sint32 bpp,
							 AUI_WINDOW_TYPE type, bool bevel) :
c3_PopupWindow(retval,id,ldlBlock,bpp,type,bevel)
{
	m_graphics.reset(spNew_ctp2_Button(retval,ldlBlock,"GraphicsButton",optionsscreen_graphicsPress));
	m_sound.reset(spNew_ctp2_Button(retval,ldlBlock,"SoundButton",optionsscreen_soundPress));
	m_music.reset(spNew_ctp2_Button(retval,ldlBlock,"MusicButton",optionsscreen_musicPress));
	m_newgame.reset(spNew_ctp2_Button(retval,ldlBlock,"NewGameButton",optionsscreen_quitToShellPress));
	m_savegame.reset(spNew_ctp2_Button(retval,ldlBlock,"SaveGameButton",optionsscreen_savegamePress));
	m_loadgame.reset(spNew_ctp2_Button(retval,ldlBlock,"LoadGameButton",optionsscreen_loadgamePress));
	m_restart.reset(spNew_ctp2_Button(retval,ldlBlock,"RestartButton",optionsscreen_restartPress));
	m_gameplay.reset(spNew_ctp2_Button(retval,ldlBlock,"GamePlayButton",optionsscreen_gameplayPress));
	m_mapeditor.reset(spNew_ctp2_Button(retval,ldlBlock,"MapEditorButton",optionsscreen_mapeditorPress));
	m_keyboard.reset(spNew_ctp2_Button( retval, ldlBlock, "KeyboardButton", optionsscreen_keyboardPress ));
	m_quittoshell.reset(spNew_ctp2_Button( retval, ldlBlock, "QuitToShellButton", optionsscreen_quitPress ));

	m_configHeader.reset(spNew_c3_Static(retval,ldlBlock,"ConfigHeader"));
	m_gameHeader.reset(spNew_c3_Static(retval,ldlBlock,"GameHeader"));







	AddClose( optionsscreen_returnPress );

}

OptionsWindow::~OptionsWindow()
{
	// Every control is a unique_ptr member and releases itself.
}

sint32 OptionsWindow::EnableButtons( )
{

	m_newgame->Enable( TRUE );
	m_savegame->Enable( TRUE );

	m_restart->Enable( TRUE );
	m_mapeditor->Enable( TRUE );
	m_quittoshell->Enable( TRUE );

	return 0;
}

sint32 OptionsWindow::DisableButtons( )
{

	m_newgame->Enable( FALSE );
	m_savegame->Enable( FALSE );

	m_restart->Enable( FALSE );
	m_mapeditor->Enable( FALSE );
	m_quittoshell->Enable( FALSE );

	return 0;
}

void OptionsWindow::RemoveQuitToWindowsButton( )
{
	if ( m_quittoshell && GetChild( m_quittoshell->Id() ) )
		RemoveChild( m_quittoshell->Id() );
}

void OptionsWindow::AddQuitToWindowsButton( )
{
	if ( m_quittoshell && !GetChild( m_quittoshell->Id() ) )
		AddChild(m_quittoshell.get());
}
