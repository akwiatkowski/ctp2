// gs/core/audio_observer.cpp
// See audio_observer.h for the rationale. Thin dispatch layer:
// holds the registered Impl pointer and forwards every free function.

#include "ctp/c3.h"
#include "gs/core/audio_observer.h"

namespace audio_observer {

namespace {
    Impl *s_impl = nullptr;
}

void Register(Impl *impl) { s_impl = impl; }
Impl *Get()                { return s_impl; }

#define DISPATCH_VOID(method, ...) \
    do { if (s_impl) s_impl->method(__VA_ARGS__); } while (0)

void AddSound(sint32 type, uint32 associatedObject,
              sint32 soundID, sint32 x, sint32 y)
{
    DISPATCH_VOID(AddSound, type, associatedObject, soundID, x, y);
}
void AddGameSound(sint32 sound)                      { DISPATCH_VOID(AddGameSound, sound); }
void SetVolume(sint32 type, uint32 volume)           { DISPATCH_VOID(SetVolume, type, volume); }

void  SetMusicStyle(sint32 style)                    { DISPATCH_VOID(SetMusicStyle, style); }
sint32 GetMusicStyle()                               { return s_impl ? s_impl->GetMusicStyle() : 0; }
void  EnableMusic()                                  { DISPATCH_VOID(EnableMusic); }
void  DisableMusic()                                 { DISPATCH_VOID(DisableMusic); }
sint32 IsMusicEnabled()                              { return s_impl ? s_impl->IsMusicEnabled() : 0; }

void  SetAutoRepeat(sint32 autoRepeat)               { DISPATCH_VOID(SetAutoRepeat, autoRepeat); }
sint32 IsAutoRepeat()                                { return s_impl ? s_impl->IsAutoRepeat() : 0; }

#undef DISPATCH_VOID

} // namespace audio_observer
