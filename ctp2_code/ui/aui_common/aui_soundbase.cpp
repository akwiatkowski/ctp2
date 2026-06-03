#include "ctp/c3.h"
#include "ui/aui_common/aui_soundbase.h"

#include "ui/aui_common/aui_ui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_sound.h"

#include "sound/soundmanager.h"
#include "sound/gamesounds.h"

#include "ui/ldl/ldl_data.hpp"

extern SoundManager		*soundmgr_Get();

MBCHAR *aui_SoundBase::m_soundLdlKeywords[ AUI_SOUNDBASE_SOUND_LAST ] =
{
	"activatesound",
	"deactivatesound",
	"engagesound",
	"disengagesound",
	"executesound",
	"tipsound"
};


aui_SoundBase::aui_SoundBase( MBCHAR *ldlBlock )
{
	InitCommonLdl( ldlBlock );
}


aui_SoundBase::aui_SoundBase( MBCHAR **soundNames )
{
	InitCommon( soundNames );
}


AUI_ERRCODE aui_SoundBase::InitCommonLdl( MBCHAR *ldlBlock )
{
    ldl_datablock * block = aui_Ldl::FindDataBlock(ldlBlock);
	Assert( block != nullptr );
	if ( !block ) return AUI_ERRCODE_LDLFINDDATABLOCKFAILED;

	MBCHAR *soundNames[ AUI_SOUNDBASE_SOUND_LAST ];
	for ( sint32 i = 0; i < AUI_SOUNDBASE_SOUND_LAST; i++ )
		soundNames[ i ] = block->GetString( m_soundLdlKeywords[ i ] );

	AUI_ERRCODE errcode = InitCommon( soundNames );
	Assert( AUI_SUCCESS(errcode) );
	return errcode;
}


AUI_ERRCODE aui_SoundBase::InitCommon( MBCHAR **soundNames )
{
	memset( m_sounds, 0, sizeof( m_sounds ) );

	for ( sint32 i = 0; i < AUI_SOUNDBASE_SOUND_LAST; i++ )
		SetSound( (AUI_SOUNDBASE_SOUND)i, soundNames ? soundNames[ i ] : nullptr );

	return AUI_ERRCODE_OK;
}


aui_SoundBase::~aui_SoundBase()
{
	for ( sint32 i = 0; i < AUI_SOUNDBASE_SOUND_LAST; i++ )
	{
		if ( m_sounds[ i ] )
		{
			aui_ui_Get()->UnloadSound( m_sounds[ i ] );
			m_sounds[ i ] = nullptr;
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
	MBCHAR *soundName )
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

		break;
	}

	return AUI_ERRCODE_OK;
}
