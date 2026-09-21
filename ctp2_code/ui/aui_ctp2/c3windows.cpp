//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : The civilization 3 base window
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

#include <memory>

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_ctp2/c3ui.h"

#include "ui/aui_ctp2/statuswindow.h"
#include "ui/aui_ctp2/tipwindow.h"
#include "ui/interface/controlpanelwindow.h"
#include "ui/interface/debugwindow.h"
#include "ui/aui_ctp2/c3_popupwindow.h"
#include "ui/interface/radarwindow.h"

#include "ui/aui_ctp2/textbutton.h"
#include "ui/aui_ctp2/iconbutton.h"

#include "ui/aui_ctp2/c3thumb.h"
#include "ui/aui_ctp2/c3slider.h"
#include "ui/aui_ctp2/c3scroller.h"
#include "ui/aui_ctp2/c3spinner.h"

#include "ui/aui_ctp2/checkbox.h"
#include "ui/aui_ctp2/textswitch.h"

#include "ui/aui_ctp2/textradio.h"
#include "ui/aui_ctp2/radiogroup.h"

#include "ui/aui_ctp2/texttab.h"
#include "ui/aui_common/aui_tabgroup.h"

#include "ui/aui_common/aui_item.h"
#include "ui/aui_ctp2/c3listbox.h"
#include "ui/aui_ctp2/textbox.h"

#include "ui/aui_ctp2/c3dropdown.h"

#include "ui/aui_ctp2/c3textfield.h"

#include "gfx/tilesys/tiledmap.h"

#include "ui/aui_ctp2/c3windows.h"

#include "gs/fileio/CivPaths.h"
#include "gfx/gfx_utils/videoutils.h"

#include "ui/interface/workwin.h"
#include "ui/interface/statswindow.h"
#include "ui/interface/workwindow.h"

#include "ui/interface/backgroundwin.h"
#include "ui/interface/workwin.h"

#include "ui/aui_ctp2/thumbnailmap.h"

#include "gs/gameobj/player.h"
#include "gs/gameobj/ID.h"
#include "ui/aui_ctp2/SelItem.h"

#include "ui/interface/screenutils.h"
#include "gs/gameobj/GameSettings.h"

#include "ui/aui_ctp2/c3_utilitydialogbox.h"


extern sint32 g_ScreenWidth;
extern sint32 g_ScreenHeight;

extern C3Window				*g_turnWindow;
extern C3Window				*g_statsWindow;


extern sint32				g_god;

extern RECT g_backgroundViewport;

// The window globals are extern'd raw pointers in other TUs (controlpanel.cpp,
// c3debug.cpp, ...); ownership lives in these file-static unique_ptrs and the
// globals alias them.
static std::unique_ptr<DebugWindow>	s_debugWindowOwner;
DebugWindow					*g_debugWindow;





static std::unique_ptr<C3Window>	s_testWindowOwner;
static std::unique_ptr<C3Window>	s_standardWindowOwner;
static std::unique_ptr<C3Window>	s_floatingWindowOwner;
C3Window		*g_testWindow = nullptr;
C3Window		*g_standardWindow = nullptr;
C3Window		*g_floatingWindow = nullptr;

// Owned here; the g_testWindow/g_standardWindow/g_floatingWindow globals above
// stay raw because they are extern'd in controlpanel.cpp.
static std::unique_ptr<TipWindow>	g_tipWindow;
static std::unique_ptr<TipWindow>	g_thumbTipWindow;


static std::unique_ptr<TextTab>		g_happyTab;
static std::unique_ptr<IconButton>	g_iconButton;






void DebugExitButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	AUI_ERRCODE auiErr;

	auiErr = c3ui_Get()->RemoveWindow( g_debugWindow->Id() );
	Assert( auiErr == AUI_ERRCODE_OK );
	if ( auiErr != AUI_ERRCODE_OK ) return;
}

void DebugApplyButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	TextBox *textbox;
	C3TextField *textfield;

	memcpy( &textbox, cookie, sizeof( TextBox * ) );
	memcpy( &textfield, (MBCHAR *)cookie + sizeof( TextBox * ), sizeof( C3TextField * ) );

	static MBCHAR text[ 100 ];

	textfield->GetFieldText( text, 100 );


	if ( strcmp( text, "clear" ) == 0 )
		textbox->SetText( "" );

	else
		textbox->AppendText( text );
}

void TestWindowButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	AUI_ERRCODE auiErr;

	auiErr = c3ui_Get()->RemoveWindow( k_ID_WINDOW_TEST );
	Assert( auiErr == AUI_ERRCODE_OK );
	if ( auiErr != AUI_ERRCODE_OK ) return;

	auiErr = c3ui_Get()->RemoveWindow( k_ID_WINDOW_STANDARD );
	Assert( auiErr == AUI_ERRCODE_OK );
	if ( auiErr != AUI_ERRCODE_OK ) return;

	auiErr = c3ui_Get()->RemoveWindow( k_ID_WINDOW_FLOATING );
	Assert( auiErr == AUI_ERRCODE_OK );
	if ( auiErr != AUI_ERRCODE_OK ) return;




}






void TabCallbackSad( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
}

void TabCallbackIndifferent( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
}

void TabCallbackHappy( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
}






void CheckboxCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	aui_TabGroup *tabGroup = (aui_TabGroup *)cookie;
	if ( !tabGroup ) return;

	AUI_ERRCODE errcode = AUI_ERRCODE_OK;


	switch ( action )
	{
	case AUI_SWITCH_ACTION_ON:

		if ( !g_iconButton )
		{
			sint32 controlX = 50;
			sint32 controlY = 50;

			g_iconButton = std::make_unique<IconButton>(
				&errcode,
				k_ID_ICONBUTTON_TESTWINDOW,
				controlX,
				controlY,
				50, 50,
				k_PatternName,
				k_IconName,
				0xf2ed );
			if ( !g_iconButton ) return;

		}


		if ( !g_happyTab )
		{

			g_happyTab = std::make_unique<TextTab>(
				&errcode,
				k_ID_TAB_HAPPY,
				0,
				0,
				0,
				0,
				0,
				0,
				k_PatternName,
				"Happy",
				TabCallbackHappy );
			if ( !g_happyTab ) return;

			errcode = g_happyTab->AddPaneControl( g_iconButton.get() );
			Assert( errcode == AUI_ERRCODE_OK );
			if ( errcode != AUI_ERRCODE_OK ) return;
		}

		errcode = tabGroup->AddTab( g_happyTab.get() );
		Assert( errcode == AUI_ERRCODE_OK );
		if ( errcode != AUI_ERRCODE_OK ) return;

		break;

	case AUI_SWITCH_ACTION_OFF:
		if ( g_happyTab )
		{
			errcode = tabGroup->RemoveTab( k_ID_TAB_HAPPY );
			Assert( errcode == AUI_ERRCODE_OK );
			if ( errcode != AUI_ERRCODE_OK ) return;














		}
		break;
	}
}

void DraggableCheckboxCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	aui_Window *window = control->GetParentWindow();
	if ( !window ) return;

	switch ( action )
	{
	case AUI_SWITCH_ACTION_ON:
		window->SetDraggable( TRUE );
		break;

	case AUI_SWITCH_ACTION_OFF:
		window->SetDraggable( FALSE );
		break;
	}
}

void TransparentCheckboxCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	aui_Window *window = control->GetParentWindow();
	if ( !window ) return;

	switch ( action )
	{
	case AUI_SWITCH_ACTION_ON:
		window->SetTransparent( TRUE );
		break;

	case AUI_SWITCH_ACTION_OFF:
		window->SetTransparent( FALSE );
		break;
	}
}






void ModalRadioCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	aui_Window *window = control->GetParentWindow();
	if ( !window ) return;

	switch ( control->Id() )
	{
	case k_ID_RADIO_WEAKLYMODAL:
		switch ( action )
		{
		case AUI_SWITCH_ACTION_ON:
			window->SetWeaklyModal( TRUE );
			break;

		case AUI_SWITCH_ACTION_OFF:
			window->SetWeaklyModal( FALSE );
			break;
		}
		break;

	case k_ID_RADIO_STRONGLYMODAL:
		switch ( action )
		{
		case AUI_SWITCH_ACTION_ON:
			window->SetStronglyModal( TRUE );
			break;

		case AUI_SWITCH_ACTION_OFF:
			window->SetStronglyModal( FALSE );
			break;
		}
		break;
	}
}








int c3windows_MakeTestWindow( BOOL make )
{

	static std::unique_ptr<TextButton>		button;
	static std::unique_ptr<TextTab>		sadTab;
	static std::unique_ptr<Checkbox>		checkbox;
	static std::unique_ptr<Checkbox>		dragcheckbox;
	static std::unique_ptr<Checkbox>		transparentcheckbox;
	static std::unique_ptr<TextTab>		indifferentTab;
	static std::unique_ptr<TextRadio>		stronglyModalRadio;
	static std::unique_ptr<TextRadio>		weaklyModalRadio;
	static std::unique_ptr<RadioGroup>	modalRadioGroup;
	static std::unique_ptr<aui_TabGroup>	moodyTabGroup;
	static std::unique_ptr<C3Spinner>	spinner;

	if ( make )
	{
		if ( g_testWindow ) return 0;

		AUI_ERRCODE errcode = AUI_ERRCODE_OK;


		MBCHAR windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

		{

			snprintf(windowBlock, sizeof(windowBlock), "mywindow" );

			s_testWindowOwner = std::make_unique<C3Window>(
				&errcode,
				k_ID_WINDOW_TEST,
				windowBlock,
				16 );
			g_testWindow = s_testWindowOwner.get();
			Assert( AUI_NEWOK(g_testWindow,errcode) );
			if ( !AUI_NEWOK(g_testWindow,errcode) ) return -1;




			MBCHAR controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

			{

				snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "mybutton" );

				button = std::make_unique<TextButton>(
					&errcode,
					k_ID_BUTTON_TESTWINDOW,
					controlBlock,
					TestWindowButtonActionCallback );
				Assert( AUI_NEWOK(button,errcode) );
				if ( !AUI_NEWOK(button,errcode) ) return -1;






				c3windows_MakeTipWindow();

				button->SetTipWindow( g_tipWindow.get() );

			}

			{

				snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "myspinner" );

				spinner = std::make_unique<C3Spinner>(
					&errcode,
					k_ID_SPINNER,
					controlBlock );
				Assert( AUI_NEWOK(spinner,errcode) );
				if ( !AUI_NEWOK(spinner,errcode) ) return -1;

			}

			{

				snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "mytabgroup" );

				moodyTabGroup = std::make_unique<aui_TabGroup>(
					&errcode,
					k_ID_TABGROUP_MOODY,
					controlBlock );
				Assert( AUI_NEWOK(moodyTabGroup,errcode) );
				if ( !AUI_NEWOK(moodyTabGroup,errcode) ) return -1;

				moodyTabGroup->SetDrawMask( k_AUI_REGION_DRAWFLAG_UPDATE );




				MBCHAR tabBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

				{

					snprintf(tabBlock, sizeof(tabBlock), "%s.%s", controlBlock, "sadtab" );

					sadTab = std::make_unique<TextTab>(
						&errcode,
						k_ID_TAB_SAD,
						tabBlock,
						TabCallbackSad );
					Assert( AUI_NEWOK(sadTab,errcode) );
					if ( !AUI_NEWOK(sadTab,errcode) ) return -1;




					MBCHAR checkboxBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

					{

						snprintf(checkboxBlock, sizeof(checkboxBlock), "%s.pane.%s", tabBlock, "checkbox" );

						checkbox = std::make_unique<Checkbox>(
							&errcode,
							k_ID_CHECKBOX_TESTWINDOW,
							checkboxBlock,
							CheckboxCallback,
							moodyTabGroup.get() );
						Assert( AUI_NEWOK(checkbox,errcode) );
						if ( !AUI_NEWOK(checkbox,errcode) ) return -1;

					}

					{

						snprintf(checkboxBlock, sizeof(checkboxBlock), "%s.pane.%s", tabBlock, "dragcheckbox" );

						dragcheckbox = std::make_unique<Checkbox>(
							&errcode,
							k_ID_DRAGCHECKBOX_TESTWINDOW,
							checkboxBlock,
							DraggableCheckboxCallback );
						Assert( AUI_NEWOK(dragcheckbox,errcode) );
						if ( !AUI_NEWOK(dragcheckbox,errcode) ) return -1;

					}

					{

						snprintf(checkboxBlock, sizeof(checkboxBlock), "%s.pane.%s", tabBlock, "transparentcheckbox" );

						transparentcheckbox = std::make_unique<Checkbox>(
							&errcode,
							k_ID_TRANSPARENTCHECKBOX_TESTWINDOW,
							checkboxBlock,
							TransparentCheckboxCallback );
						Assert( AUI_NEWOK(transparentcheckbox,errcode) );
						if ( !AUI_NEWOK(transparentcheckbox,errcode) ) return -1;

					}

				}

				{

					snprintf(tabBlock, sizeof(tabBlock), "%s.%s", controlBlock, "indifferenttab" );

					indifferentTab = std::make_unique<TextTab>(
						&errcode,
						k_ID_TAB_INDIFFERENT,
						tabBlock,
						TabCallbackIndifferent );
					Assert( AUI_NEWOK(indifferentTab,errcode) );
					if ( !AUI_NEWOK(indifferentTab,errcode) ) return -1;




					MBCHAR radiogroupBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

					{

						snprintf(radiogroupBlock, sizeof(radiogroupBlock), "%s.pane.%s", tabBlock, "modalradiogroup" );

						modalRadioGroup = std::make_unique<RadioGroup>(
							&errcode,
							k_ID_RADIOGROUP_MODAL,
							radiogroupBlock );
						Assert( AUI_NEWOK(modalRadioGroup,errcode) );
						if ( !AUI_NEWOK(modalRadioGroup,errcode) ) return -1;




						MBCHAR radioBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

						{

							snprintf(radioBlock, sizeof(radioBlock), "%s.%s", radiogroupBlock, "stronglymodalradio" );

							stronglyModalRadio = std::make_unique<TextRadio>(
								&errcode,
								k_ID_RADIO_STRONGLYMODAL,
								radioBlock,
								ModalRadioCallback );
							Assert( AUI_NEWOK(stronglyModalRadio,errcode) );
							if ( !AUI_NEWOK(stronglyModalRadio,errcode) ) return -1;

						}

						{

							snprintf(radioBlock, sizeof(radioBlock), "%s.%s", radiogroupBlock, "weaklymodalradio" );

							weaklyModalRadio = std::make_unique<TextRadio>(
								&errcode,
								k_ID_RADIO_WEAKLYMODAL,
								radioBlock,
								ModalRadioCallback );
							Assert( AUI_NEWOK(weaklyModalRadio,errcode) );
							if ( !AUI_NEWOK(weaklyModalRadio,errcode) ) return -1;

						}

					}

				}

			}

		}

		errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
		Assert( AUI_SUCCESS(errcode) );
		if ( !AUI_SUCCESS(errcode) ) return -1;
	}
	else
	{
		if ( !g_testWindow ) return 0;

		button.reset();
		sadTab.reset();
		checkbox.reset();
		dragcheckbox.reset();
		transparentcheckbox.reset();
		indifferentTab.reset();
		stronglyModalRadio.reset();
		weaklyModalRadio.reset();
		modalRadioGroup.reset();
		moodyTabGroup.reset();
		spinner.reset();

		c3windows_MakeTipWindow( FALSE );

		if ( g_testWindow )
		{
			c3ui_Get()->RemoveWindow( g_testWindow->Id() );
			s_testWindowOwner.reset();
			g_testWindow = nullptr;
		}
	}

	return 0;
}

int c3windows_MakeStandardWindow( BOOL make )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;

	static std::unique_ptr<C3Slider> slider;
	static std::unique_ptr<C3DropDown> dropdown;
	static std::unique_ptr<TextSwitch> i1;
	static std::unique_ptr<TextSwitch> i2;
	static std::unique_ptr<TextSwitch> i3;
	static std::unique_ptr<TextSwitch> i4;
	static std::unique_ptr<TextSwitch> i5;
	static std::unique_ptr<TextSwitch> i6;

	if ( make )
	{
		if ( g_standardWindow ) return 0;





		sint32 windowWidth = 3 * g_ScreenWidth / 8;
		sint32 windowHeight = 2 * g_ScreenHeight / 6;
		sint32 windowX = ( g_ScreenWidth - windowWidth ) / 3;
		sint32 windowY = ( g_ScreenHeight - windowHeight ) / 3;

		s_standardWindowOwner = std::make_unique<C3Window>(
			&errcode,
			k_ID_WINDOW_STANDARD,
			windowX, windowY, windowWidth, windowHeight,
			16,
			k_PatternName );
		g_standardWindow = s_standardWindowOwner.get();
		Assert( g_standardWindow != nullptr );
		if ( !g_standardWindow ) return -1;





		sint32 controlWidth = 2 * windowWidth / 3;
		sint32 controlHeight = windowHeight / 4;
		sint32 controlX = windowWidth / 6;
		sint32 controlY = windowHeight / 6;

		slider = std::make_unique<C3Slider>(
			&errcode,
			k_ID_SLIDER,
			controlX,
			controlY,
			controlWidth,
			controlHeight,
			FALSE,
			k_PatternName );
		if ( !slider ) return -3;

		errcode = slider->SetMaximum( 3, 0 );
		Assert( errcode == AUI_ERRCODE_OK );
		if ( errcode != AUI_ERRCODE_OK ) return -3;

		errcode = slider->SetPage( 1, 1 );
		Assert( errcode == AUI_ERRCODE_OK );
		if ( errcode != AUI_ERRCODE_OK ) return -3;

		errcode = slider->UseRigidThumb( TRUE );
		Assert( errcode == AUI_ERRCODE_OK );
		if ( errcode != AUI_ERRCODE_OK ) return -3;

		errcode = slider->UseQuantizedDragging( TRUE );
		Assert( errcode == AUI_ERRCODE_OK );
		if ( errcode != AUI_ERRCODE_OK ) return -3;





		windowWidth = 200;
		windowHeight = 40;
		windowX = 0;
		windowY = 0;

		g_thumbTipWindow = std::make_unique<TipWindow>(
			&errcode,
			k_ID_WINDOW_TIP,
			windowX, windowY, windowWidth, windowHeight,
			16,
			k_PatternName,
			"I am a Thumb control." );
		Assert( g_thumbTipWindow != nullptr );
		if ( !g_thumbTipWindow ) return -1;

		C3Thumb *thumb = (C3Thumb *)slider->GetThumb();
		if ( thumb )
			thumb->SetTipWindow( g_thumbTipWindow.get() );






		controlWidth = 2 * g_standardWindow->Width() / 3;
		controlHeight = g_standardWindow->Height() / 8;
		controlX = g_standardWindow->Width() / 6;
		controlY = g_standardWindow->Height() - 2 * controlHeight;

		dropdown = std::make_unique<C3DropDown>(
			&errcode,
			aui_UniqueId(),
			controlX,
			controlY,
			controlWidth,
			controlHeight,
			k_PatternName,
			50,
			5 * ( controlHeight - 4 ) );
		if ( !dropdown ) return -3;


		i1 = std::make_unique<TextSwitch>(
			&errcode,
			aui_UniqueId(),
			0, 0,
			controlWidth - 30, controlHeight - 4,
			k_PatternName,
			"yes" );
		if ( !i1 ) return -1;
		i2 = std::make_unique<TextSwitch>(
			&errcode,
			aui_UniqueId(),
			0, 0,
			controlWidth - 40, controlHeight - 4,
			k_PatternName,
			"no" );
		if ( !i2 ) return -2;
		i3 = std::make_unique<TextSwitch>(
			&errcode,
			aui_UniqueId(),
			0, 0,
			controlWidth - 50, controlHeight - 4,
			k_PatternName,
			"i mean it" );
		if ( !i3 ) return -3;
		i4 = std::make_unique<TextSwitch>(
			&errcode,
			aui_UniqueId(),
			0, 0,
			controlWidth - 30, controlHeight - 4,
			k_PatternName,
			"anybody wanna peanut" );
		if ( !i4 ) return -4;
		i5 = std::make_unique<TextSwitch>(
			&errcode,
			aui_UniqueId(),
			0, 0,
			controlWidth - 25, controlHeight - 4,
			k_PatternName,
			"yes ma'am" );
		if ( !i5 ) return -5;
		i6 = std::make_unique<TextSwitch>(
			&errcode,
			aui_UniqueId(),
			0, 0,
			controlWidth - 35, controlHeight - 4,
			k_PatternName,
			"yes sir" );
		if ( !i6 ) return -6;
		dropdown->AddItem( (aui_Item *)i1.get() );
		dropdown->AddItem( (aui_Item *)i2.get() );
		dropdown->AddItem( (aui_Item *)i3.get() );
		dropdown->AddItem( (aui_Item *)i4.get() );
		dropdown->AddItem( (aui_Item *)i5.get() );
		dropdown->AddItem( (aui_Item *)i6.get() );






		errcode = g_standardWindow->AddControl( slider.get() );
		Assert( errcode == AUI_ERRCODE_OK );
		if ( errcode != AUI_ERRCODE_OK ) return -4;
		errcode = g_standardWindow->AddControl( dropdown.get() );
		Assert( errcode == AUI_ERRCODE_OK );
		if ( errcode != AUI_ERRCODE_OK ) return -4;

		g_standardWindow->SetDraggable( TRUE );
	}
	else
	{
		if ( !g_standardWindow ) return 0;

		slider.reset();
		g_thumbTipWindow.reset();
		dropdown.reset();
		i1.reset();
		i2.reset();
		i3.reset();
		i4.reset();
		i5.reset();
		i6.reset();

		c3ui_Get()->RemoveWindow( g_standardWindow->Id() );
		s_standardWindowOwner.reset();
		g_standardWindow = nullptr;
	}

	return 0;
}

int c3windows_MakeTipWindow( BOOL make )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;

	if ( make )
	{
		if ( g_tipWindow ) return 0;





		sint32 windowWidth = 200;
		sint32 windowHeight = 30;
		sint32 windowX = 0;
		sint32 windowY = 0;

		g_tipWindow = std::make_unique<TipWindow>(
			&errcode,
			k_ID_WINDOW_TIP,
			windowX, windowY, windowWidth, windowHeight,
			16,
			k_PatternName,
			"Press this button" );
		Assert( g_tipWindow != nullptr );
		if ( !g_tipWindow ) return -1;
	}
	else
	{
		if ( !g_tipWindow ) return 0;

		g_tipWindow.reset();
	}

	return 0;
}

int c3windows_MakeFloatingWindow( BOOL make )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;

	static std::unique_ptr<C3ListBox> listbox;
	static std::unique_ptr<TextSwitch> items[ 14 ];
	static std::unique_ptr<TextSwitch> subItems[ 14 ][ 5 ];
	static std::unique_ptr<TextSwitch> h1;
	static std::unique_ptr<TextSwitch> h2;
	static std::unique_ptr<TextSwitch> h3;
	static std::unique_ptr<TextSwitch> h4;
	static std::unique_ptr<TextSwitch> h5;
	static std::unique_ptr<TextSwitch> h6;

	if ( make )
	{
		if ( g_floatingWindow ) return 0;




		sint32 windowWidth = 600;
		sint32 windowHeight = 300;
		sint32 windowX = 10;
		sint32 windowY = 10;

		s_floatingWindowOwner = std::make_unique<C3Window>(
			&errcode,
			k_ID_WINDOW_FLOATING,
			windowX, windowY, windowWidth, windowHeight,
			16,
			k_PatternName,
			AUI_WINDOW_TYPE_FLOATING );
		g_floatingWindow = s_floatingWindowOwner.get();
		Assert( g_floatingWindow != nullptr );
		if ( !g_floatingWindow ) return -1;

		g_floatingWindow->SetDraggable( TRUE );






		sint32 controlWidth = 2 * windowWidth / 3;
		sint32 controlHeight = 2 * windowHeight / 3;
		sint32 controlX = windowWidth / 6;
		sint32 controlY = windowHeight / 6;

		listbox = std::make_unique<C3ListBox>(
			&errcode,
			k_ID_LISTBOX,
			controlX,
			controlY,
			controlWidth,
			controlHeight,
			k_PatternName );
		if ( !listbox ) return -3;




		sint32 itemHeight = 30 + 2 * (rand()%5);

		for ( sint32 i = 0; i < 14; i++ )
		{
			static char s[ 50 ];
			snprintf(s, sizeof(s), "row=%d col=%d", i, 0 );

			items[ i ] = std::make_unique<TextSwitch>(
				&errcode,
				aui_UniqueId(),
				0, 0,
				140 + 4 * (rand()%10), itemHeight,
				k_PatternName,
				s );
			if ( !items[ i ] ) return -i * 100;

			for ( sint32 j = 0; j < 5; j++ )
			{
				snprintf(s, sizeof(s), "row=%d col=%d", i, j+1 );

				subItems[ i ][ j ] = std::make_unique<TextSwitch>(
					&errcode,
					aui_UniqueId(),
					0, 0,
					140 + 4 * (rand()%10), itemHeight,
					k_PatternName,
					s );
				if ( !subItems[ i ][ j ] ) return -j * 1000;

				items[ i ]->AddChild( subItems[ i ][ j ].get() );
			}

			listbox->AddItem( (aui_Item *)items[ i ].get() );
		}




		h1 = std::make_unique<TextSwitch>(
			&errcode,
			aui_UniqueId(),
			0, 0,
			50, 25,
			k_PatternName,
			"name" );
		if ( !h1 ) return -1;
		h2 = std::make_unique<TextSwitch>(
			&errcode,
			aui_UniqueId(),
			0, 0,
			50, 25,
			k_PatternName,
			"date" );
		if ( !h2 ) return -2;
		h3 = std::make_unique<TextSwitch>(
			&errcode,
			aui_UniqueId(),
			0, 0,
			50, 25,
			k_PatternName,
			"stuff" );
		if ( !h3 ) return -3;
		h4 = std::make_unique<TextSwitch>(
			&errcode,
			aui_UniqueId(),
			0, 0,
			50, 25,
			k_PatternName,
			"thing" );
		if ( !h4 ) return -4;
		h5 = std::make_unique<TextSwitch>(
			&errcode,
			aui_UniqueId(),
			0, 0,
			50, 25,
			k_PatternName,
			"rate" );
		if ( !h5 ) return -5;
		h6 = std::make_unique<TextSwitch>(
			&errcode,
			aui_UniqueId(),
			0, 0,
			50, 25,
			k_PatternName,
			"creamescense" );
		if ( !h6 ) return -6;
		listbox->AddHeaderSwitch( h1.get() );
		listbox->AddHeaderSwitch( h2.get() );
		listbox->AddHeaderSwitch( h3.get() );
		listbox->AddHeaderSwitch( h4.get() );
		listbox->AddHeaderSwitch( h5.get() );
		listbox->AddHeaderSwitch( h6.get() );






		errcode = g_floatingWindow->AddControl( listbox.get() );
		Assert( errcode == AUI_ERRCODE_OK );
		if ( errcode != AUI_ERRCODE_OK ) return -4;

	}
	else
	{
		if ( !g_floatingWindow ) return 0;

		listbox.reset();
		for ( sint32 i = 0; i < 14; i++ )
		{
			for ( sint32 j = 0; j < 5; j++ )
			{
				subItems[ i ][ j ].reset();
			}
			items[ i ].reset();
		}
		// h1-h6 were leaked before; the header switches are owned here too.
		h1.reset();
		h2.reset();
		h3.reset();
		h4.reset();
		h5.reset();
		h6.reset();
		c3ui_Get()->RemoveWindow( g_floatingWindow->Id() );
		s_floatingWindowOwner.reset();
		g_floatingWindow = nullptr;
	}

	return 0;
}

void ControlWindowButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	int err;
	AUI_ERRCODE auiErr;

	err = c3windows_MakeTestWindow();
	Assert( err == 0 );
	if ( err != 0 ) return;

	err = c3windows_MakeStandardWindow();
	Assert( err == 0 );
	if ( err != 0 ) return;

	err = c3windows_MakeFloatingWindow();
	Assert( err == 0 );
	if ( err != 0 ) return;

	auiErr = c3ui_Get()->AddWindow( g_floatingWindow );
	Assert( auiErr == AUI_ERRCODE_OK );
	if ( auiErr != AUI_ERRCODE_OK ) return;
	auiErr = c3ui_Get()->AddWindow( g_testWindow );
	Assert( auiErr == AUI_ERRCODE_OK );
	if ( auiErr != AUI_ERRCODE_OK ) return;
	auiErr = c3ui_Get()->AddWindow( g_standardWindow );
	Assert( auiErr == AUI_ERRCODE_OK );
	if ( auiErr != AUI_ERRCODE_OK ) return;
}

void KnowledgeButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	auto popup = std::make_unique<c3_ExpelPopup>(nullptr);
	popup->DisplayWindow();
}

void DebugButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	AUI_ERRCODE auiErr;

	auiErr = c3ui_Get()->AddWindow( g_debugWindow );
	Assert( auiErr == AUI_ERRCODE_OK );
	if ( auiErr != AUI_ERRCODE_OK ) return;
}

static std::unique_ptr<aui_Window>	s_thumbWindow;
static std::unique_ptr<ThumbnailMap>	s_thumbnail;

void ResourceButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;


















}
void CheatButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;


	gamesettings_Get()->SetKeepScore( FALSE );

}

void DiplomacyButtonActionCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

}

void CityViewButtonActionCallback( aui_Control *control, uint32 action, uint32 data, void *cookie )
{

	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return;

	bool This_Code_Is_Really_Really_Old = false;
	Assert(This_Code_Is_Really_Really_Old);

}

#define k_STATUS_WINDOW_HEIGHT		30

int c3windows_MakeStatusWindow( BOOL make )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;

	static std::unique_ptr<TextButton>	button;
	static std::unique_ptr<TextButton>	debugButton;
	static std::unique_ptr<TextButton>	resourceButton;
	static std::unique_ptr<TextButton>	cheatButton;
	static std::unique_ptr<TextButton>	knowledgeButton;
	static std::unique_ptr<TextButton>	diplomacyButton;

	if ( make )
	{
		if ( statuswindow_Get() ) return 0;

		sint32 windowWidth = g_ScreenWidth;
		sint32 windowHeight = k_STATUS_WINDOW_HEIGHT;
		sint32 windowX = 0;
		sint32 windowY = 27;

		statuswindow_Set(std::make_unique<StatusWindow>(
			&errcode,
			k_ID_WINDOW_STATUS,
			windowX, windowY, windowWidth, windowHeight,
			16,
			k_PatternName ).release());
		Assert( statuswindow_Get() != nullptr );
		if ( !statuswindow_Get() ) return -1;
		statuswindow_Get()->SetDraggable( TRUE );

		sint32 controlWidth = 60;
		sint32 controlHeight = 20;
		sint32 controlX = statuswindow_Get()->Width() / 2;
		sint32 controlY = 5;

		knowledgeButton = std::make_unique<TextButton>(
			&errcode,
			aui_UniqueId(),
			controlX,
			controlY,
			controlWidth,
			controlHeight,
			k_PatternName,
			"Science",
			KnowledgeButtonActionCallback );
		if ( !knowledgeButton ) return -16;

		controlX += controlWidth + 6;

		button = std::make_unique<TextButton>(
			&errcode,
			k_ID_BUTTON_CONTROLWINDOW,
			controlX,
			controlY,
			controlWidth,
			controlHeight,
			k_PatternName,
			"UI",
			ControlWindowButtonActionCallback );
		if ( !button ) return -16;

		controlX += controlWidth + 6;

		debugButton = std::make_unique<TextButton>(
			&errcode,
			aui_UniqueId(),
			controlX,
			controlY,
			controlWidth,
			controlHeight,
			k_PatternName,
			"Dbg",
			DebugButtonActionCallback );
		if ( !debugButton ) return -16;

			controlX += controlWidth + 6;

		resourceButton = std::make_unique<TextButton>(
			&errcode,
			aui_UniqueId(),
			controlX,
			controlY,
			controlWidth,
			controlHeight,
			k_PatternName,
			"FLI",
			ResourceButtonActionCallback );
		if ( !resourceButton ) return -16;

		controlX += controlWidth + 6;

		cheatButton = std::make_unique<TextButton>(
			&errcode,
			aui_UniqueId(),
			controlX,
			controlY,
			controlWidth,
			controlHeight,
			k_PatternName,
			"Cheat",
			CheatButtonActionCallback );
		if ( !cheatButton ) return -16;

		controlX += controlWidth + 6;














		controlX += controlWidth + 6;
		diplomacyButton = std::make_unique<TextButton>(
			&errcode,
			aui_UniqueId(),
			controlX,
			controlY,
			controlWidth,
			controlHeight,
			k_PatternName,
			"Throne",
			DiplomacyButtonActionCallback );
		if ( !diplomacyButton ) return -16;

		controlX += controlWidth + 6;




		errcode = statuswindow_Get()->AddControl( button.get() );
		Assert( errcode == AUI_ERRCODE_OK );
		if ( errcode != AUI_ERRCODE_OK ) return -15;
		errcode = statuswindow_Get()->AddControl( debugButton.get() );
		Assert( errcode == AUI_ERRCODE_OK );
		if ( errcode != AUI_ERRCODE_OK ) return -15;
		errcode = statuswindow_Get()->AddControl( resourceButton.get() );
		Assert( errcode == AUI_ERRCODE_OK );
		if ( errcode != AUI_ERRCODE_OK ) return -15;
		errcode = statuswindow_Get()->AddControl( cheatButton.get() );
		Assert( errcode == AUI_ERRCODE_OK );
		if ( errcode != AUI_ERRCODE_OK ) return -15;


		errcode = statuswindow_Get()->AddControl( diplomacyButton.get() );
		Assert( errcode == AUI_ERRCODE_OK );
		if ( errcode != AUI_ERRCODE_OK ) return -15;
		errcode = statuswindow_Get()->AddControl( knowledgeButton.get() );
		Assert( errcode == AUI_ERRCODE_OK );
		if ( errcode != AUI_ERRCODE_OK ) return -15;































	}
	else
	{
		if ( !statuswindow_Get() ) return 0;

		c3ui_Get()->RemoveWindow( statuswindow_Get()->Id() );

		button.reset();
		debugButton.reset();
		resourceButton.reset();
		cheatButton.reset();
		diplomacyButton.reset();
		knowledgeButton.reset();

		s_thumbWindow.reset();
		s_thumbnail.reset();

		std::unique_ptr<StatusWindow>{statuswindow_Get()};
		statuswindow_Set(nullptr);
	}

	return 0;
}

int c3windows_MakeDebugWindow( BOOL make )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;

	static std::unique_ptr<TextBox> textbox;
	static std::unique_ptr<TextButton> exitButton;

	if ( make )
	{
		if ( g_debugWindow ) return 0;

		sint32 windowWidth = g_ScreenWidth / 3;
		sint32 windowHeight = g_ScreenHeight / 2;
		sint32 windowX = 0;
		sint32 windowY = g_ScreenHeight - windowHeight;

		s_debugWindowOwner = std::make_unique<DebugWindow>(
			&errcode,
			aui_UniqueId(),
			windowX, windowY, windowWidth, windowHeight,
			16,
			const_cast<MBCHAR *>(k_PatternName),
			AUI_WINDOW_TYPE_FLOATING );
		g_debugWindow = s_debugWindowOwner.get();
		Assert( g_debugWindow != nullptr );
		if ( !g_debugWindow ) return -1;

		g_debugWindow->SetDynamic(FALSE);

		g_debugWindow->GrabRegion()->Move( 0, 0 );
		g_debugWindow->GrabRegion()->Resize( windowWidth, 20 );
		g_debugWindow->SetDraggable( TRUE );

		textbox = std::make_unique<TextBox>(
			&errcode,
			aui_UniqueId(),
			2,
			12,
			windowWidth - 30,
			windowHeight - 24,
			k_PatternName, nullptr, nullptr, nullptr);
		if ( !textbox ) return -3;


		exitButton = std::make_unique<TextButton>(
			&errcode,
			aui_UniqueId(),
			windowWidth - 12,
			2,
			10,
			10,
			k_PatternName,
			"X",
			DebugExitButtonActionCallback,
			textbox.get() );
		if ( !exitButton ) return -3;

		errcode = g_debugWindow->AddControl( textbox.get() );
		Assert( errcode == AUI_ERRCODE_OK );
		if ( errcode != AUI_ERRCODE_OK ) return -4;

		errcode = g_debugWindow->AddControl( exitButton.get() );
		Assert( errcode == AUI_ERRCODE_OK );
		if ( errcode != AUI_ERRCODE_OK ) return -4;


		g_debugWindow->SetTextBox(textbox.get());

	}
	else
	{
		if ( !g_debugWindow ) return 0;

		c3ui_Get()->RemoveWindow( g_debugWindow->Id() );

		textbox.reset();

		exitButton.reset();

		s_debugWindowOwner.reset();
		g_debugWindow = nullptr;
	}

	return 0;
}

void c3windows_Cleanup( )
{
	c3windows_MakeDebugWindow( FALSE );
	workwin_Cleanup();

	c3windows_MakeTestWindow( FALSE );
	c3windows_MakeFloatingWindow( FALSE );
	c3windows_MakeStandardWindow( FALSE );
	c3windows_MakeStatusWindow( FALSE );

	controlpanelwindow_Cleanup();


	radarwindow_Cleanup();
	backgroundWin_Cleanup();

	g_iconButton.reset();
	g_happyTab.reset();

	c3ui_Get()->UnloadIcon( k_IconName );
}
