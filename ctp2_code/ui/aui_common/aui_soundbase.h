#ifndef __AUI_SOUNDBASE_H__
#define __AUI_SOUNDBASE_H__

class aui_Sound;


enum AUI_SOUNDBASE_SOUND
{
	AUI_SOUNDBASE_SOUND_FIRST = 0,
	AUI_SOUNDBASE_SOUND_ACTIVATE = 0,
	AUI_SOUNDBASE_SOUND_DEACTIVATE,
	AUI_SOUNDBASE_SOUND_ENGAGE,
	AUI_SOUNDBASE_SOUND_DISENGAGE,
	AUI_SOUNDBASE_SOUND_EXECUTE,
	AUI_SOUNDBASE_SOUND_TIP,
	AUI_SOUNDBASE_SOUND_LAST
};


class aui_SoundBase
{
public:

	aui_SoundBase( MBCHAR const *ldlBlock );
	aui_SoundBase( MBCHAR const **soundNames );
	virtual ~aui_SoundBase();

protected:
	aui_SoundBase() {}
	AUI_ERRCODE InitCommonLdl( MBCHAR const *ldlBlock );
	AUI_ERRCODE InitCommon( MBCHAR const **soundNames );

public:
	aui_Sound	*GetSound( AUI_SOUNDBASE_SOUND sound ) const;
	aui_Sound	*SetSound( AUI_SOUNDBASE_SOUND sound, MBCHAR const *soundName );

	AUI_ERRCODE	PlaySound( AUI_SOUNDBASE_SOUND sound );

protected:
	aui_Sound		*m_sounds[ AUI_SOUNDBASE_SOUND_LAST ];
	static MBCHAR	*m_soundLdlKeywords[ AUI_SOUNDBASE_SOUND_LAST ];
};

#endif
