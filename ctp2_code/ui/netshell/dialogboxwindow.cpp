//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Multiplayer dialog box window
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
#include <memory>
#include "ui/netshell/dialogboxwindow.h"

#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_static.h"
#include "ui/aui_common/aui_progressbar.h"
#include "ui/aui_common/aui_screen.h"
#include "ui/aui_common/aui_button.h"

#include "ui/netshell/netshell.h"
#include "ui/netshell/ns_chatbox.h"
#include "ui/netshell/ns_customlistbox.h"

#include "ui/aui_ctp2/c3_button.h"

#include "ui/interface/spnewgamewindow.h"

#include "ui/ldl/ldl_file.hpp"

DialogBoxWindow::DialogBoxWindow(
	AUI_ERRCODE *retval,
	MBCHAR *ldlBlock,
	aui_Action **actions )
:
	ns_Window	(retval,
				 aui_UniqueId(),
				 ldlBlock,
				 0,
				 AUI_WINDOW_TYPE_FLOATING
				),
	m_numButtons	(0)
{
	if ( !AUI_SUCCESS(*retval) ) return;
	*retval = InitCommon();
	if ( !AUI_SUCCESS(*retval) ) return;
	*retval = CreateControls( ldlBlock, actions );
}

AUI_ERRCODE DialogBoxWindow::InitCommon( )
{
	m_controls = std::make_unique<aui_Control *[]>( m_numControls = CONTROL_MAX );

	return AUI_ERRCODE_OK;
}

AUI_ERRCODE DialogBoxWindow::CreateControls(
	MBCHAR *ldlBlock,
	aui_Action **actions )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;

	Assert( ldlBlock != nullptr );
	if ( !ldlBlock ) return AUI_ERRCODE_INVALIDPARAM;

	aui_Control *control;
	static MBCHAR block[ k_AUI_LDL_MAXBLOCK + 1 ];

	snprintf(block, sizeof(block), "%s.titlestatictext", ldlBlock );
	if (aui_Ldl::GetLdl()->FindDataBlock( block ) )
	{
		control = std::make_unique<aui_Static>(
			&errcode,
			aui_UniqueId(),
			block ).release();
		Assert( AUI_NEWOK(control,errcode) );
		if ( !AUI_NEWOK(control,errcode) ) return errcode;
	}
	else
		control = nullptr;
	m_controls[ CONTROL_TITLESTATICTEXT ] = control;

	snprintf(block, sizeof(block), "%s.titlebox", ldlBlock );
    if (aui_Ldl::GetLdl()->FindDataBlock( block ) )
	{
		control = std::make_unique<aui_Static>(
			&errcode,
			aui_UniqueId(),
			block ).release();
		Assert( AUI_NEWOK(control,errcode) );
		if ( !AUI_NEWOK(control,errcode) ) return errcode;
	}
	else
		control = nullptr;
	m_controls[ CONTROL_TITLEBOX ] = control;

	snprintf(block, sizeof(block), "%s.lefttopcorner", ldlBlock );
    if (aui_Ldl::GetLdl()->FindDataBlock( block ) )
	{
		control = std::make_unique<aui_Static>(
			&errcode,
			aui_UniqueId(),
			block ).release();
		Assert( AUI_NEWOK(control,errcode) );
		if ( !AUI_NEWOK(control,errcode) ) return errcode;
	}
	else
		control = nullptr;
	m_controls[ CONTROL_LEFTTOPCORNER ] = control;

	snprintf(block, sizeof(block), "%s.righttopcorner", ldlBlock );
    if (aui_Ldl::GetLdl()->FindDataBlock( block ) )
	{
		control = std::make_unique<aui_Static>(
			&errcode,
			aui_UniqueId(),
			block ).release();
		Assert( AUI_NEWOK(control,errcode) );
		if ( !AUI_NEWOK(control,errcode) ) return errcode;
	}
	else
		control = nullptr;
	m_controls[ CONTROL_RIGHTTOPCORNER ] = control;

	snprintf(block, sizeof(block), "%s.leftbottomcorner", ldlBlock );
    if (aui_Ldl::GetLdl()->FindDataBlock( block ) )
	{
		control = std::make_unique<aui_Static>(
			&errcode,
			aui_UniqueId(),
			block ).release();
		Assert( AUI_NEWOK(control,errcode) );
		if ( !AUI_NEWOK(control,errcode) ) return errcode;
	}
	else
		control = nullptr;
	m_controls[ CONTROL_LEFTBOTTOMCORNER ] = control;

	snprintf(block, sizeof(block), "%s.rightbottomcorner", ldlBlock );
    if (aui_Ldl::GetLdl()->FindDataBlock( block ) )
	{
		control = std::make_unique<aui_Static>(
			&errcode,
			aui_UniqueId(),
			block ).release();
		Assert( AUI_NEWOK(control,errcode) );
		if ( !AUI_NEWOK(control,errcode) ) return errcode;
	}
	else
		control = nullptr;
	m_controls[ CONTROL_RIGHTBOTTOMCORNER ] = control;

	snprintf(block, sizeof(block), "%s.leftedge", ldlBlock );
    if (aui_Ldl::GetLdl()->FindDataBlock( block ) )
	{
		control = std::make_unique<aui_Static>(
			&errcode,
			aui_UniqueId(),
			block ).release();
		Assert( AUI_NEWOK(control,errcode) );
		if ( !AUI_NEWOK(control,errcode) ) return errcode;
	}
	else
		control = nullptr;
	m_controls[ CONTROL_LEFTEDGE ] = control;

	snprintf(block, sizeof(block), "%s.rightedge", ldlBlock );
    if (aui_Ldl::GetLdl()->FindDataBlock( block ) )
	{
		control = std::make_unique<aui_Static>(
			&errcode,
			aui_UniqueId(),
			block ).release();
		Assert( AUI_NEWOK(control,errcode) );
		if ( !AUI_NEWOK(control,errcode) ) return errcode;
	}
	else
		control = nullptr;
	m_controls[ CONTROL_RIGHTEDGE ] = control;

	snprintf(block, sizeof(block), "%s.topedge", ldlBlock );
    if (aui_Ldl::GetLdl()->FindDataBlock( block ) )
	{
		control = std::make_unique<aui_Static>(
			&errcode,
			aui_UniqueId(),
			block ).release();
		Assert( AUI_NEWOK(control,errcode) );
		if ( !AUI_NEWOK(control,errcode) ) return errcode;
	}
	else
		control = nullptr;
	m_controls[ CONTROL_TOPEDGE ] = control;

	snprintf(block, sizeof(block), "%s.bottomedge", ldlBlock );
    if (aui_Ldl::GetLdl()->FindDataBlock( block ) )
	{
		control = std::make_unique<aui_Static>(
			&errcode,
			aui_UniqueId(),
			block ).release();
		Assert( AUI_NEWOK(control,errcode) );
		if ( !AUI_NEWOK(control,errcode) ) return errcode;
	}
	else
		control = nullptr;
	m_controls[ CONTROL_BOTTOMEDGE ] = control;

	snprintf(block, sizeof(block), "%s.descriptionstatictext", ldlBlock );
    if (aui_Ldl::GetLdl()->FindDataBlock( block ) )
	{
		control = std::make_unique<aui_Static>(
			&errcode,
			aui_UniqueId(),
			block ).release();
		Assert( AUI_NEWOK(control,errcode) );
		if ( !AUI_NEWOK(control,errcode) ) return errcode;
	}
	else
		control = nullptr;
	m_controls[ CONTROL_DESCRIPTIONSTATICTEXT ] = control;

	snprintf(block, sizeof(block), "%s.progressbar", ldlBlock );
    if (aui_Ldl::GetLdl()->FindDataBlock( block ) )
	{
		control = std::make_unique<aui_ProgressBar>(
			&errcode,
			aui_UniqueId(),
			block ).release();
		Assert( AUI_NEWOK(control,errcode) );
		if ( !AUI_NEWOK(control,errcode) ) return errcode;
	}
	else
		control = nullptr;
	m_controls[ CONTROL_PROGRESSBAR ] = control;

	do
	{
		snprintf(block, sizeof(block), "%s.button%d", ldlBlock, m_numButtons );

        if ( !aui_Ldl::GetLdl()->FindDataBlock( block ) )
			break;

		m_numButtons++;

	} while ( true );

	if ( m_numButtons )
	{
		m_buttons.resize( m_numButtons );
		Assert( !m_buttons.empty() );
		if ( m_buttons.empty() ) return AUI_ERRCODE_MEMALLOCFAILED;

		for ( sint32 i = 0; i < m_numButtons; i++ )
		{

			snprintf(block, sizeof(block), "button%d", i );

			m_buttons[ i ].reset( spNew_ctp2_Button(
				&errcode,
				ldlBlock,
				block,
				nullptr) );

			Assert( AUI_NEWOK(m_buttons[i],errcode) );
			if ( !AUI_NEWOK(m_buttons[i],errcode) )
				return AUI_ERRCODE_MEMALLOCFAILED;

			if ( actions )
				m_buttons[ i ]->SetAction( actions[ i ] );
		}
	}

	aui_Ldl::SetupHeirarchyFromRoot( ldlBlock );

	SetStronglyModal( TRUE );

	return AUI_ERRCODE_OK;
}

DialogBoxWindow::~DialogBoxWindow() = default;

DialogBoxWindow *DialogBoxWindow::PopUp(
	MBCHAR *ldlBlock,
	aui_Action **actions )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;

	auto dbw = std::make_unique<DialogBoxWindow>(
		&errcode,
		ldlBlock,
		actions );
	Assert( AUI_NEWOK(dbw,errcode) );
	if ( !AUI_NEWOK(dbw,errcode) )
	{
		dbw.reset();
	}

	aui_Screen *screen = netshell_Get()->GetCurrentScreen();
	if ( screen )
		screen->AddWindow( dbw.get(), TRUE );

	return dbw.release();
}

void DialogBoxWindow::PopDown( DialogBoxWindow *dbw, aui_Button *button )
{
	Assert( dbw != nullptr );
	if ( !dbw ) return;

	aui_Screen *screen = netshell_Get()->GetCurrentScreen();
	if ( screen )
		screen->RemoveWindow( dbw->Id() );

	if ( button && button->GetAction() )
		button->GetAction()->Execute(
			button,
			AUI_BUTTON_ACTION_EXECUTE,
			0 );

	aui_ui_Get()->AddAction( std::make_unique<SafeDeleteAction>( dbw ).release());
}

DialogBoxWindow::SafeDeleteAction::~SafeDeleteAction() = default;

void DialogBoxWindow::SafeDeleteAction::Execute(
	aui_Control *control,
	uint32 action,
	uint32 data )
{
	m_dbw.reset();
}
