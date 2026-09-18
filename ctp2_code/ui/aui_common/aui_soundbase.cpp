#include "ctp/c3.h"
#include "ui/aui_common/aui_soundbase.h"

#include "ui/aui_common/aui_ui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_sound.h"

#include "sound/soundmanager.h"
#include "sound/gamesounds.h"

#include "ui/ldl/ldl_data.hpp"



MBCHAR *aui_SoundBase::m_soundLdlKeywords[ AUI_SOUNDBASE_SOUND_LAST ] =
{
	// const_cast required: member is declared MBCHAR* in aui_soundbase.h (not owned here)
	const_cast<MBCHAR *>("activatesound"),
	const_cast<MBCHAR *>("deactivatesound"),
	const_cast<MBCHAR *>("engagesound"),
	const_cast<MBCHAR *>("disengagesound"),
	const_cast<MBCHAR *>("executesound"),
	const_cast<MBCHAR *>("tipsound")
};


aui_SoundBase::aui_SoundBase( MBCHAR const *ldlBlock )
{
	InitCommonLdl( ldlBlock );
}


aui_SoundBase::aui_SoundBase( MBCHAR const **soundNames )
{
	InitCommon( soundNames );
}


AUI_ERRCODE aui_SoundBase::InitCommonLdl( MBCHAR const *ldlBlock )
{
    ldl_datablock * block = aui_Ldl::FindDataBlock(ldlBlock);
	Assert( block != nullptr );
	if ( !block ) return AUI_ERRCODE_LDLFINDDATABLOCKFAILED;

	MBCHAR const *soundNames[ AUI_SOUNDBASE_SOUND_LAST ];
	for ( sint32 i = 0; i < AUI_SOUNDBASE_SOUND_LAST; i++ )
		soundNames[ i ] = block->GetString( m_soundLdlKeywords[ i ] );

	AUI_ERRCODE errcode = InitCommon( soundNames );
	Assert( AUI_SUCCESS(errcode) );
	return errcode;
}


AUI_ERRCODE aui_SoundBase::InitCommon( MBCHAR const **soundNames )
{
	memset( m_sounds, 0, sizeof( m_sounds ) );

	for ( sint32 i = 0; i < AUI_SOUNDBASE_SOUND_LAST; i++ )
		SetSound( (AUI_SOUNDBASE_SOUND)i, soundNames ? soundNames[ i ] : nullptr );

	return AUI_ERRCODE_OK;
}


aui_SoundBase::~aui_SoundBase()
{
	for (auto & m_sound : m_sounds)
	{
		if ( m_sound )
		{
			aui_ui_Get()->UnloadSound( m_sound );
			m_sound = nullptr;
		}
	}
}


aui_Sound *aui_SoundBase::GetSound( AUI_SOUNDBASE_SOUND sound ) const
{
	if ( sound < 0 || sound >= AUI_SOUNDBASE_SOUND_LAST )
		return nullptr;

	return m_sounds[ sound ];
}


aui_Sound *aui_SoundBase::SetSound(
	AUI_SOUNDBASE_SOUND sound,
	MBCHAR const *soundName )
{
	aui_Sound *prevSound = GetSound( sound );

	if ( soundName && aui_ui_Get()->TheAudioManager()->UsingAudio() )
	{
		m_sounds[ sound ] = aui_ui_Get()->LoadSound( soundName );
		Assert( m_sounds[ sound ] != nullptr );
		if ( !m_sounds[ sound ] )
		{
			m_sounds[ sound ] = prevSound;
			return nullptr;
		}
	}
	else
		m_sounds[ sound ] = nullptr;

	if ( prevSound ) aui_ui_Get()->UnloadSound( prevSound );

	return prevSound;
}


AUI_ERRCODE aui_SoundBase::PlaySound( AUI_SOUNDBASE_SOUND sound )
{






	switch (sound) {


	case AUI_SOUNDBASE_SOUND_EXECUTE:
		soundmgr_Get()->AddSound(SOUNDTYPE_SFX, 0,
				gamesounds_GetGameSoundID(GAMESOUNDS_BUTTONCLICK), 0, 0);
		break;
	case AUI_SOUNDBASE_SOUND_ENGAGE:
	case AUI_SOUNDBASE_SOUND_TIP:
	// Sentinel and unused kinds have no stock sound mapping.
	case AUI_SOUNDBASE_SOUND_FIRST:  // == ACTIVATE
	case AUI_SOUNDBASE_SOUND_DEACTIVATE:
	case AUI_SOUNDBASE_SOUND_DISENGAGE:
	case AUI_SOUNDBASE_SOUND_LAST:
		break;
	}

	return AUI_ERRCODE_OK;
}
