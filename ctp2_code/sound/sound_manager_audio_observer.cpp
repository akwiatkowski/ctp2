// sound/sound_manager_audio_observer.cpp
// 1:1 forwarder from audio_observer::Impl to g_soundManager.

#include "ctp/c3.h"
#include "sound/sound_manager_audio_observer.h"
#include "sound/soundmanager.h"     // SoundManager, SOUNDTYPE, MUSICSTYLE
#include "sound/gamesounds.h"        // GAMESOUNDS

extern SoundManager *g_soundManager;

void SoundManagerAudioObserver::AddSound(sint32 type, uint32 associatedObject,
                                         sint32 soundID, sint32 x, sint32 y)
{
    g_soundManager->AddSound((SOUNDTYPE)type, associatedObject, soundID, x, y);
}

void SoundManagerAudioObserver::AddGameSound(sint32 sound)
{
    g_soundManager->AddGameSound((GAMESOUNDS)sound);
}

void SoundManagerAudioObserver::SetVolume(sint32 type, uint32 volume)
{
    g_soundManager->SetVolume((SOUNDTYPE)type, volume);
}

void   SoundManagerAudioObserver::SetMusicStyle(sint32 style)  { g_soundManager->SetMusicStyle((MUSICSTYLE)style); }
sint32 SoundManagerAudioObserver::GetMusicStyle()              { return (sint32)g_soundManager->GetMusicStyle(); }
void   SoundManagerAudioObserver::EnableMusic()                { g_soundManager->EnableMusic(); }
void   SoundManagerAudioObserver::DisableMusic()               { g_soundManager->DisableMusic(); }
sint32 SoundManagerAudioObserver::IsMusicEnabled()             { return (sint32)g_soundManager->IsMusicEnabled(); }

void   SoundManagerAudioObserver::SetAutoRepeat(sint32 autoRepeat) { g_soundManager->SetAutoRepeat(autoRepeat ? TRUE : FALSE); }
sint32 SoundManagerAudioObserver::IsAutoRepeat()                   { return (sint32)g_soundManager->IsAutoRepeat(); }

namespace {
    SoundManagerAudioObserver s_soundManagerAudioObserver;
}

void RegisterSoundManagerAudioObserver()
{
    audio_observer::Register(&s_soundManagerAudioObserver);
}
