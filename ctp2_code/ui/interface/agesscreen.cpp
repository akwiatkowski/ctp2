//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Age Screen
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
// - Starting and ending age selection screen now uses the age names from
//   gl_str.txt, Martin G�hmann.
// - Compatibility restored.
// - Memory leak repaired.
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
// - Added single-player start and end age. (11-Apr-2009 Maq)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/interface/agesscreen.h"

#include "AgeRecord.h"
#include "ui/aui_common/aui_stringtable.h"
#include "ui/aui_ctp2/c3_button.h"
#include "ui/aui_ctp2/c3_listitem.h"
#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_ctp2/ctp2_dropdown.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "gs/utility/Globals.h"                // allocated::...
#include <memory>
#include "ui/interface/spnewgamewindow.h"
#include "gs/database/StrDB.h"                  // stringdb_Get()
#include "gs/database/profileDB.h"              // profiledb_Get()
#if CTP2_ENABLE_NETWORKING
#include "ui/netshell/netshell.h"               // gamesetup_Get()
#endif

static std::unique_ptr<C3Window> s_agesScreen;

static std::unique_ptr<aui_Button>		s_back;
static std::unique_ptr<c3_Static>		s_name;
static std::unique_ptr<c3_Static>		s_start;
static std::unique_ptr<c3_Static>		s_end;
static std::unique_ptr<ctp2_DropDown>	s_startDropDown;
static std::unique_ptr<ctp2_DropDown>	s_endDropDown;


static sint32 s_numAges = 0;
static sint32 s_startAge = 0;
static sint32 s_endAge = s_numAges;
sint32 agesscreen_getStartAge( )
{
	return s_startAge;
}
sint32 agesscreen_getEndAge( )
{
	return s_endAge;
}






void agesscreen_setStartAge( sint32 index )
{
	Assert( index >= 0 && index < s_numAges );
	if ( index < 0 || index >= s_numAges )
		return;

	s_startDropDown->SetSelectedItem( index );

	s_startAge = index;
#if CTP2_ENABLE_NETWORKING
	gamesetup_Get().SetStartAge(static_cast<char>(index));
#endif
	profiledb_Get()->SetSPStartingAge(index);
}






void agesscreen_setEndAge( sint32 index )
{
	Assert( index >= 0 && index < s_numAges );
	if ( index < 0 || index >= s_numAges )
		return;

	s_endDropDown->SetSelectedItem( index );

	s_endAge = index;
#if CTP2_ENABLE_NETWORKING
	gamesetup_Get().SetEndAge(static_cast<char>(index));
#endif
	profiledb_Get()->SetSPEndingAge(index);
}





sint32	agesscreen_displayMyWindow(bool viewMode)
{
    sint32 retval = s_agesScreen ? 0 : agesscreen_Initialize();

	s_startDropDown->Enable( !viewMode );
	s_endDropDown->Enable( !viewMode );

	AUI_ERRCODE auiErr = c3ui_Get()->AddWindow(s_agesScreen.get());
	Assert(auiErr == AUI_ERRCODE_OK);

	return retval;
}

sint32 agesscreen_removeMyWindow(uint32 action)
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return 0;

	agesscreen_setStartAge( s_startDropDown->GetSelectedItem() );
	agesscreen_setEndAge( s_endDropDown->GetSelectedItem() );
	profiledb_Get()->SetSPStartingAge( s_startDropDown->GetSelectedItem() );
	profiledb_Get()->SetSPEndingAge( s_endDropDown->GetSelectedItem() );

	AUI_ERRCODE auiErr = c3ui_Get()->RemoveWindow(s_agesScreen->Id());
	Assert(auiErr == AUI_ERRCODE_OK);

	return 1;
}


AUI_ERRCODE agesscreen_Initialize( aui_Control::ControlActionCallback *callback )
{
	if ( s_agesScreen ) return AUI_ERRCODE_OK;

	MBCHAR		windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	strlcpy(windowBlock, "agesscreen", sizeof(windowBlock));

	AUI_ERRCODE errcode = AUI_ERRCODE_OK;

	s_agesScreen = std::make_unique<C3Window>(
		&errcode,
		aui_UniqueId(),
		windowBlock,
		16 );
	Assert( AUI_NEWOK(s_agesScreen, errcode) );
	if ( !AUI_NEWOK(s_agesScreen, errcode) ) return errcode;

	s_back = std::make_unique<aui_Button>(
		&errcode,
		aui_UniqueId(),
		"agesscreen.closebutton" );
	Assert( AUI_NEWOK(s_back,errcode) );
	if ( !AUI_NEWOK(s_back,errcode) ) return errcode;

	if ( !callback ) callback = agesscreen_backPress;
	s_back->SetActionFuncAndCookie(callback, nullptr);

	s_name.reset(spNew_c3_Static(&errcode,windowBlock,"NameStatic"));
	s_start.reset(spNew_c3_Static(&errcode,windowBlock,"StartStatic"));
	s_end.reset(spNew_c3_Static(&errcode,windowBlock,"EndStatic"));

	MBCHAR		controlBlock[ k_AUI_LDL_MAXBLOCK + 1 ];
	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "StartDropDown" );

	s_startDropDown = std::make_unique<ctp2_DropDown>(
		&errcode,
		aui_UniqueId(),
		controlBlock,
		agesscreen_startDropDownCallback,
		nullptr );
	Assert( AUI_NEWOK(s_startDropDown, errcode) );
	if ( !AUI_NEWOK(s_startDropDown, errcode) ) return errcode;

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", windowBlock, "EndDropDown" );

	s_endDropDown = std::make_unique<ctp2_DropDown>(
		&errcode,
		aui_UniqueId(),
		controlBlock,
		agesscreen_endDropDownCallback,
		nullptr );
	Assert( AUI_NEWOK(s_endDropDown, errcode) );
	if ( !AUI_NEWOK(s_endDropDown, errcode) ) return errcode;

	aui_StringTable	startagestrings(&errcode, "strings.startagestrings");
	s_numAges = g_theAgeDB->NumRecords();
	bool const		isLdlUsable = s_numAges == startagestrings.GetNumStrings();
	for (sint32 i = 0; i < s_numAges; i++)
	{
//Added by Martin G�hmann so that no *.ldl needs to be edited
//anymore when new ages are added.
		MBCHAR const *	ageId	= g_theAgeDB->GetNameStr(i);
		MBCHAR const *	name	= g_theAgeDB->Get(i)->GetNameText();

		if (isLdlUsable && (0 == strcmp(ageId, name)))
		{
			// Age name not defined in gl_str.txt: use the ldl file text.
			name = startagestrings.GetString(i);
		}
		else if (!name)
		{
			name = ageId;
		}






		{
			auto item = std::make_unique<SingleListItem>(
				&errcode,
				name,
				i,
				"listitems.ageitem" );
			Assert( AUI_NEWOK(item,errcode) );
			if ( !AUI_NEWOK(item,errcode) ) return AUI_ERRCODE_MEMALLOCFAILED;

			s_startDropDown->AddItem( (ctp2_ListItem *)item.release() );
		}







		{

			auto item = std::make_unique<SingleListItem>(
				&errcode,
				name,
				i,
				"listitems.ageitem" );
			Assert( AUI_NEWOK(item,errcode) );
			if ( !AUI_NEWOK(item,errcode) ) return AUI_ERRCODE_MEMALLOCFAILED;

			s_endDropDown->AddItem( (ctp2_ListItem *)item.release() );
		}
	}

	agesscreen_setStartAge(0);
	agesscreen_setEndAge(s_numAges - 1);

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );

	if (s_back) {
		s_back->Move( s_agesScreen->Width() - s_back->Width() - 14, s_agesScreen->Height() - s_back->Height() - 17);
	}

	return AUI_ERRCODE_OK;
}


//----------------------------------------------------------------------------
//
// Name       : agesscreen_Cleanup
//
// Description: Release the memory of all static data from this file.
//
// Parameters : -
//
// Globals    : All static variables.
//
// Returns    : -
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
void agesscreen_Cleanup()
{
	if (s_startDropDown)
	{
		s_startDropDown->Clear();
        s_startDropDown.reset();
	}
	if (s_endDropDown)
	{
		s_endDropDown->Clear();
        s_endDropDown.reset();
	}

    s_name.reset();
	s_start.reset();
	s_end.reset();
	s_back.reset();

	if (s_agesScreen)
	{
		c3ui_Get()->RemoveWindow(s_agesScreen->Id());
		s_agesScreen.reset();
	}
}





void agesscreen_startDropDownCallback(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void *cookie )
{
	if ( action != (uint32)AUI_DROPDOWN_ACTION_SELECT ) return;




	if ( s_startDropDown->GetSelectedItem() >
		 s_endDropDown->GetSelectedItem() )
		s_endDropDown->SetSelectedItem(
			s_startDropDown->GetSelectedItem() );

}
void agesscreen_endDropDownCallback(
	aui_Control *control,
	uint32 action,
	uint32 data,
	void *cookie )
{
	if ( action != (uint32)AUI_DROPDOWN_ACTION_SELECT ) return;




	if ( s_endDropDown->GetSelectedItem() <
		 s_startDropDown->GetSelectedItem() )
		s_startDropDown->SetSelectedItem(
			s_endDropDown->GetSelectedItem() );

}
void agesscreen_backPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{


	agesscreen_removeMyWindow(action);
}
