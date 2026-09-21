//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Sound property screen
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
#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_ctp2/c3_popupwindow.h"
#include "ui/aui_ctp2/c3_button.h"
#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/c3_listitem.h"
#include "ui/aui_ctp2/c3_dropdown.h"
#include "ui/aui_ctp2/c3slider.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_common/aui_uniqueid.h"

#include "ui/interface/spnewgamewindow.h"
#include "ui/interface/soundscreen.h"
#include "gs/database/profileDB.h"

#include "ui/aui_ctp2/keypress.h"


static std::unique_ptr<c3_PopupWindow> s_soundWindow;
static std::unique_ptr<C3Slider> s_sfx, s_music, s_voice;

static std::unique_ptr<c3_Static> s_sfxN, s_musicN, s_voiceN;
static std::unique_ptr<c3_Static> s_sfxmin, s_sfxmax, s_musicmin,
								  s_musicmax, s_voicemin, s_voicemax;





sint32	soundscreen_displayMyWindow()
{
	sint32 retval=0;
	if(!s_soundWindow) { retval = soundscreen_Initialize(); }

	AUI_ERRCODE auiErr;

	auiErr = c3ui_Get()->AddWindow(s_soundWindow.get());
	Assert( auiErr == AUI_ERRCODE_OK );
	keypress_RegisterHandler(s_soundWindow.get());

	return retval;
}
sint32 soundscreen_removeMyWindow(uint32 action)
{
	if ( action != (uint32)AUI_BUTTON_ACTION_EXECUTE ) return 0;

	AUI_ERRCODE auiErr;

	sint32		 sfx;
	sint32		 voice;
	sint32		 music;

	soundscreen_getValues(sfx, music, voice);

	profiledb_Get()->SetSFXVolume(sfx);
	profiledb_Get()->SetVoiceVolume(voice);
	profiledb_Get()->SetMusicVolume(music);

	auiErr = c3ui_Get()->RemoveWindow( s_soundWindow->Id() );
	Assert( auiErr == AUI_ERRCODE_OK );
	keypress_RemoveHandler(s_soundWindow.get());

	return 1;
}


AUI_ERRCODE soundscreen_Initialize( )
{
	AUI_ERRCODE errcode = AUI_ERRCODE_OK;
	MBCHAR		windowBlock[ k_AUI_LDL_MAXBLOCK + 1 ];

	if ( s_soundWindow ) return AUI_ERRCODE_OK;

	strlcpy(windowBlock, "SoundWindow", sizeof(windowBlock));
	s_soundWindow = std::make_unique<c3_PopupWindow>(
		&errcode,
		aui_UniqueId(),
		windowBlock,
		16,
		AUI_WINDOW_TYPE_FLOATING,
		false );
	Assert( AUI_NEWOK(s_soundWindow, errcode) );
	if ( !AUI_NEWOK(s_soundWindow, errcode) ) return errcode;

	s_soundWindow->SetStronglyModal(TRUE);

	s_sfx.reset(spNew_C3Slider(&errcode,windowBlock,"SFXSlider",soundscreen_sfxSlide));
	s_sfxN.reset(spNew_c3_Static(&errcode,windowBlock,"SFXName"));
	s_music.reset(spNew_C3Slider(&errcode,windowBlock,"MusicSlider",soundscreen_musicSlide));
	s_musicN.reset(spNew_c3_Static(&errcode,windowBlock,"MusicName"));
	s_voice.reset(spNew_C3Slider(&errcode,windowBlock,"VoiceSlider",soundscreen_voiceSlide));
	s_voiceN.reset(spNew_c3_Static(&errcode,windowBlock,"VoiceName"));

	s_sfxmin.reset(spNew_c3_Static(&errcode,windowBlock,"SFXMin"));
	s_sfxmax.reset(spNew_c3_Static(&errcode,windowBlock,"SFXMax"));
	s_musicmin.reset(spNew_c3_Static(&errcode,windowBlock,"MusicMin"));
	s_musicmax.reset(spNew_c3_Static(&errcode,windowBlock,"MusicMax"));
	s_voicemin.reset(spNew_c3_Static(&errcode,windowBlock,"VoiceMin"));
	s_voicemax.reset(spNew_c3_Static(&errcode,windowBlock,"VoiceMax"));

	s_sfx->SetValue(profiledb_Get()->GetSFXVolume(), 0);
	s_voice->SetValue(profiledb_Get()->GetVoiceVolume(), 0);
	s_music->SetValue(profiledb_Get()->GetMusicVolume(), 0);






	MBCHAR block[ k_AUI_LDL_MAXBLOCK + 1 ];
	snprintf(block, sizeof(block), "%s.%s", windowBlock, "Name" );
	s_soundWindow->AddTitle( block );
	s_soundWindow->AddClose( soundscreen_exitPress );

	errcode = aui_Ldl::SetupHeirarchyFromRoot( windowBlock );
	Assert( AUI_SUCCESS(errcode) );

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE soundscreen_Cleanup()
{
	if ( !s_soundWindow  ) return AUI_ERRCODE_OK;

	c3ui_Get()->RemoveWindow( s_soundWindow->Id() );
	keypress_RemoveHandler(s_soundWindow.get());

	// Same release order the mycleanup macro used.
	s_sfx.reset();
	s_sfxN.reset();
	s_music.reset();
	s_musicN.reset();
	s_voice.reset();
	s_voiceN.reset();

	s_sfxmin.reset();
	s_sfxmax.reset();
	s_musicmin.reset();
	s_musicmax.reset();
	s_voicemin.reset();
	s_voicemax.reset();

	s_soundWindow.reset();

	return AUI_ERRCODE_OK;
}




void soundscreen_exitPress(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	profiledb_Get()->Save();

	soundscreen_removeMyWindow(action);
}









void soundscreen_sfxSlide(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != AUI_RANGER_ACTION_VALUECHANGE ) return;
	profiledb_Get()->SetSFXVolume(s_sfx->GetValueX());

}
void soundscreen_musicSlide(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != AUI_RANGER_ACTION_VALUECHANGE ) return;
	profiledb_Get()->SetMusicVolume(s_music->GetValueX());

}
void soundscreen_voiceSlide(aui_Control *control, uint32 action, uint32 data, void *cookie )
{
	if ( action != AUI_RANGER_ACTION_VALUECHANGE ) return;
	profiledb_Get()->SetVoiceVolume(s_voice->GetValueX());

}






void soundscreen_getValues(sint32 &sfx, sint32 &music, sint32 &voice)
{

	sfx				= s_sfx->GetValueX();
	music			= s_music->GetValueX();
	voice			= s_voice->GetValueX();

}
