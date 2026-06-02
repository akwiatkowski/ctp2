// sound/sound_manager_audio_observer.cpp
// 1:1 forwarder from audio_observer::Impl to soundmgr_Get().

#include "ctp/c3.h"
#include "sound/sound_manager_audio_observer.h"
#include "sound/soundmanager.h"     // SoundManager, SOUNDTYPE, MUSICSTYLE
#include "sound/gamesounds.h"        // GAMESOUNDS

void SoundManagerAudioObserver::AddSound(sint32 type, uint32 associatedObject,
                                         sint32 soundID, sint32 x, sint32 y)
{
    soundmgr_Get()->AddSound((SOUNDTYPE)type, associatedObject, soundID, x, y);
}

void SoundManagerAudioObserver::AddGameSound(sint32 sound)
{
    soundmgr_Get()->AddGameSound((GAMESOUNDS)sound);
}

void SoundManagerAudioObserver::SetVolume(sint32 type, uint32 volume)
{
    soundmgr_Get()->SetVolume((SOUNDTYPE)type, volume);
}

void   SoundManagerAudioObserver::SetMusicStyle(sint32 style)  { soundmgr_Get()->SetMusicStyle((MUSICSTYLE)style); }
sint32 SoundManagerAudioObserver::GetMusicStyle()              { return (sint32)soundmgr_Get()->GetMusicStyle(); }
void   SoundManagerAudioObserver::EnableMusic()                { soundmgr_Get()->EnableMusic(); }
void   SoundManagerAudioObserver::DisableMusic()               { soundmgr_Get()->DisableMusic(); }
sint32 SoundManagerAudioObserver::IsMusicEnabled()             { return (sint32)soundmgr_Get()->IsMusicEnabled(); }

void   SoundManagerAudioObserver::SetAutoRepeat(sint32 autoRepeat) { soundmgr_Get()->SetAutoRepeat(autoRepeat ? TRUE : FALSE); }
sint32 SoundManagerAudioObserver::IsAutoRepeat()                   { return (sint32)soundmgr_Get()->IsAutoRepeat(); }

namespace {
    SoundManagerAudioObserver s_soundManagerAudioObserver;
}

void RegisterSoundManagerAudioObserver()
{
    audio_observer::Register(&s_soundManagerAudioObserver);
}
