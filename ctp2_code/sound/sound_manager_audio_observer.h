// sound/sound_manager_audio_observer.h
// Adapter bridging `audio_observer::Impl` (gs/core) to `soundmgr_Get()`
// (sound/soundmanager.h).  Lives in sound/ — depends on both layers.

#pragma once

#include "gs/core/audio_observer.h"

class SoundManagerAudioObserver : public audio_observer::Impl
{
public:
    void   AddSound(sint32 type, uint32 associatedObject,
                    sint32 soundID, sint32 x, sint32 y)  override;
    void   AddGameSound(sint32 sound)                    override;
    void   SetVolume(sint32 type, uint32 volume)         override;

    void   SetMusicStyle(sint32 style)                   override;
    sint32 GetMusicStyle()                               override;
    void   EnableMusic()                                 override;
    void   DisableMusic()                                override;
    sint32 IsMusicEnabled()                              override;

    void   SetAutoRepeat(sint32 autoRepeat)              override;
    sint32 IsAutoRepeat()                                override;
};

// Convenience: instantiate the singleton adapter and register it.
void RegisterSoundManagerAudioObserver();
