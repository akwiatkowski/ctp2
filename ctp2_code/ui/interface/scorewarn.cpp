//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Cheat editor score warning window.
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

#include <vector>
#include <memory>
#include "ui/interface/scorewarn.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_action.h"
#include "ui/aui_common/aui_control.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_ctp2/c3_popupwindow.h"
#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/ctp2_button.h"
#include "ui/aui_ctp2/ctp2_hypertextbox.h"
#include "gs/gameobj/GameSettings.h"
#include "ui/aui_ctp2/keypress.h"
#include "ui/interface/optionswindow.h"
#include "gs/database/profileDB.h"
#include "ui/interface/scenarioeditor.h"
#include "ui/aui_ctp2/SelItem.h"
#include "gs/database/StrDB.h"
#include "ui/interface/UIUtils.h"

extern BOOL         g_launchIntoCheatMode;

std::unique_ptr<c3_PopupWindow>	g_scorewarn;

static std::unique_ptr<c3_Static>	s_message;

void scorewarn_OkButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	disclaimer_Initialize(scorewarn_AcceptWarningCallback);
}


void scorewarn_AcceptWarningCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if(optionsscreen_removeMyWindow(action)) {
		AUI_ERRCODE auiErr = c3ui_Get()->RemoveWindow( g_scorewarn->Id() );
		keypress_RemoveHandler(g_scorewarn.get());
		Assert( auiErr == AUI_ERRCODE_OK );
		if ( auiErr != AUI_ERRCODE_OK ) return;

		if ( selitem_Get() ) {
			selitem_Get()->Deselect( selitem_Get()->GetVisiblePlayer() );
		}

		ScenarioEditor::Display();

		gamesettings_Get()->SetKeepScore( FALSE );
	}
}

void scorewarn_CancelButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	AUI_ERRCODE auiErr = c3ui_Get()->RemoveWindow( g_scorewarn->Id() );
	keypress_RemoveHandler(g_scorewarn.get());
	Assert( auiErr == AUI_ERRCODE_OK );
	if ( auiErr != AUI_ERRCODE_OK ) return;

	gamesettings_Get()->SetKeepScore( TRUE );
}

sint32 scorewarn_Initialize( )
{
	if (g_scorewarn) return 0;

	MBCHAR			windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	strlcpy(windowBlock, "Scorewarn", sizeof(windowBlock));

	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;
	g_scorewarn = std::make_unique<c3_PopupWindow>(
		&errcode,
		aui_UniqueId(),
		windowBlock,
		16,
		AUI_WINDOW_TYPE_FLOATING);
	Assert(AUI_NEWOK(g_scorewarn, errcode));
	if (!AUI_NEWOK(g_scorewarn, errcode)) return -1;

	g_scorewarn->SetStronglyModal( TRUE );

	g_scorewarn->AddOk( scorewarn_OkButtonActionCallback, nullptr, "c3_PopupOk" );
	g_scorewarn->AddCancel( scorewarn_CancelButtonActionCallback );

	MBCHAR			buttonBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", windowBlock, "Message");
	s_message = std::make_unique<c3_Static>(&errcode,	aui_UniqueId(),	buttonBlock);
	Assert(AUI_NEWOK(s_message, errcode));
	if (!AUI_NEWOK(s_message, errcode)) return -1;

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );
	if ( !AUI_SUCCESS(errcode) ) return -1;

	return 0;
}

void scorewarn_Cleanup()
{
	if (g_scorewarn)
    {
    	c3ui_Get()->RemoveWindow( g_scorewarn->Id() );
	    keypress_RemoveHandler(g_scorewarn.get());
        s_message.reset();
    	g_scorewarn.reset();
    }
}

static std::unique_ptr<c3_PopupWindow>	s_disclaimerWindow;
static std::unique_ptr<c3_Static>		s_disclaimerLabel;
static std::unique_ptr<ctp2_Button>		s_disclaimerAcceptButton;
static std::unique_ptr<ctp2_Button>		s_disclaimerDeclineButton;
static aui_Control::ControlActionCallback *s_disclaimerCallback = nullptr;
static std::unique_ptr<ctp2_HyperTextBox>	s_disclaimerTextBox;

void DisclaimerCloseAction::Execute(aui_Control *control, uint32 action, uint32 data)
{
	disclaimer_Cleanup();
}

void disclaimer_AcceptButtonActionCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if (action != AUI_BUTTON_ACTION_EXECUTE) return;

	c3ui_Get()->AddAction(std::make_unique<DisclaimerCloseAction>().release());

	if (s_disclaimerCallback)
		s_disclaimerCallback(control, action, data, cookie);
}

void disclaimer_DeclineButtonActionCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if (action != AUI_BUTTON_ACTION_EXECUTE) return;




	g_launchIntoCheatMode = FALSE;

	c3ui_Get()->AddAction(std::make_unique<DisclaimerCloseAction>().release());
}

sint32 disclaimer_Initialize(aui_Control::ControlActionCallback *callback)
{
	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;

	if (s_disclaimerWindow) {
		errcode = c3ui_Get()->AddWindow( s_disclaimerWindow.get() );

		Assert( errcode == AUI_ERRCODE_OK );
		if ( errcode != AUI_ERRCODE_OK ) return -1;

		s_disclaimerCallback = callback;

		return 0;
	}

	MBCHAR			windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	strlcpy(windowBlock, "DisclaimerScreen", sizeof(windowBlock));

	s_disclaimerWindow = std::make_unique<c3_PopupWindow>(
		&errcode,
		aui_UniqueId(),
		windowBlock,
		16,
		AUI_WINDOW_TYPE_FLOATING);
	Assert(AUI_NEWOK(s_disclaimerWindow, errcode));
	if (!AUI_NEWOK(s_disclaimerWindow, errcode)) return -1;

	s_disclaimerWindow->SetStronglyModal(TRUE);




	MBCHAR			buttonBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", windowBlock, "TitleLabel");
	s_disclaimerLabel = std::make_unique<c3_Static>(&errcode, aui_UniqueId(), buttonBlock);
	Assert(AUI_NEWOK(s_disclaimerLabel, errcode));
	if (!AUI_NEWOK(s_disclaimerLabel, errcode)) return -1;




	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", windowBlock, "AgreeButton" );
	s_disclaimerAcceptButton = std::make_unique<ctp2_Button>( &errcode, aui_UniqueId(), buttonBlock,
		disclaimer_AcceptButtonActionCallback);
	Assert( AUI_NEWOK(s_disclaimerAcceptButton, errcode) );
	if ( !AUI_NEWOK(s_disclaimerAcceptButton, errcode) ) return -1;





	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", windowBlock, "DisagreeButton" );
	s_disclaimerDeclineButton = std::make_unique<ctp2_Button>( &errcode, aui_UniqueId(), buttonBlock,
		disclaimer_DeclineButtonActionCallback);
	Assert( AUI_NEWOK(s_disclaimerDeclineButton, errcode) );
	if ( !AUI_NEWOK(s_disclaimerDeclineButton, errcode) ) return -1;




	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", windowBlock, "DisclaimerText");
	s_disclaimerTextBox = std::make_unique<ctp2_HyperTextBox>(&errcode, aui_UniqueId(), buttonBlock, nullptr, nullptr);
	Assert( AUI_NEWOK(s_disclaimerTextBox, errcode) );
	if ( !AUI_NEWOK(s_disclaimerTextBox, errcode) ) return -1;

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );
	if ( !AUI_SUCCESS(errcode) ) return -1;




	std::vector<MBCHAR>	message;
	sint32		filesize = 0;

	FILE *f = fopen("disclaimer.txt", "rb");

	if (!f)
		goto Error;

	if (fseek(f, 0, SEEK_END) == 0) {
		filesize = ftell(f);
	} else {
		goto Error;
	}

	fclose(f);

	message.resize(filesize + 1);

	f = fopen("disclaimer.txt", "rb");
	if (!f)
		goto Error;

	c3files_fread( message.data(), 1, filesize, f );

	fclose(f);

	s_disclaimerTextBox->SetHyperText(message.data());

	s_disclaimerCallback = callback;

	errcode = c3ui_Get()->AddWindow( s_disclaimerWindow.get() );

	Assert( errcode == AUI_ERRCODE_OK );
	if ( errcode != AUI_ERRCODE_OK ) return -1;

	return 0;

Error:
	disclaimer_Cleanup();
	return -1;
}


void disclaimer_Cleanup()
{
	if (s_disclaimerWindow)
    {
	    c3ui_Get()->RemoveWindow( s_disclaimerWindow->Id() );

	    s_disclaimerLabel.reset();
	    s_disclaimerAcceptButton.reset();
	    s_disclaimerDeclineButton.reset();
	    s_disclaimerTextBox.reset();

	    s_disclaimerWindow.reset();
    }
}
