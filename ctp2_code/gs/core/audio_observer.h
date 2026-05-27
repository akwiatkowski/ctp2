//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Engine-side notifications to the audio layer
//
//----------------------------------------------------------------------------
//
// Game-state code (`gs/`) and AI code (`ai/`) historically called
// `g_soundManager->X()` directly to play sound effects, switch music
// tracks, and read music settings.  SoundManager lives in `sound/` — that
// is the wrong direction in the layered architecture, since gs/ should
// not depend on sound/.
//
// This header exposes the same surface as a set of free functions that
// fan out to a registered `Impl` callback.  The UI build registers a
// `SoundManagerAudioObserver` (in `sound/sound_manager_audio_observer.cpp`)
// that forwards to `g_soundManager`; the headless build leaves the
// observer unregistered and every call becomes a no-op.
//
// Migration pattern at the call site:
//   before:  if (g_soundManager) g_soundManager->AddSound(SOUNDTYPE_SFX, ...);
//   after:   audio_observer::AddSound((sint32)SOUNDTYPE_SFX, ...);
//
// Mirrors `gs/core/render_observer.h`.
//
//----------------------------------------------------------------------------

#pragma once

#include "ctp2_inttypes.h"

namespace audio_observer {

// --- The Impl interface ---
// Concrete implementations live in:
//   - sound/sound_manager_audio_observer.cpp (UI build, forwards to g_soundManager)
//   - test fixtures (record-and-replay spies, no-op stubs)
// Headless does not register an Impl; the free functions below short-circuit.
class Impl
{
public:
    virtual ~Impl() = default;

    // Effects — soundID is a string-table sound enum.  x/y default to 0
    // (used by 3D-positional audio).  Type maps to SOUNDTYPE / GAMESOUNDS
    // — passed as sint32 so this header doesn't pull in soundmanager.h.
    virtual void AddSound(sint32 type, uint32 associatedObject,
                          sint32 soundID, sint32 x, sint32 y) = 0;
    virtual void AddGameSound(sint32 sound) = 0;

    // Volume per type.
    virtual void SetVolume(sint32 type, uint32 volume) = 0;

    // Music control.
    virtual void  SetMusicStyle(sint32 style) = 0;
    virtual sint32 GetMusicStyle()             = 0;   // returns MUSICSTYLE enum value
    virtual void  EnableMusic()  = 0;
    virtual void  DisableMusic() = 0;
    virtual sint32 IsMusicEnabled() = 0;              // BOOL (0 / non-zero)

    // Autorepeat toggle.
    virtual void  SetAutoRepeat(sint32 autoRepeat) = 0;
    virtual sint32 IsAutoRepeat() = 0;
};

// --- Registration ---
void Register(Impl *impl);
Impl *Get();

// --- Free-function fan-outs ---
// Each function null-checks Get() and forwards.  Non-void functions return
// a safe default when no Impl is registered.

void AddSound(sint32 type, uint32 associatedObject,
              sint32 soundID, sint32 x = 0, sint32 y = 0);
void AddGameSound(sint32 sound);
void SetVolume(sint32 type, uint32 volume);

void  SetMusicStyle(sint32 style);
sint32 GetMusicStyle();      // returns 0 (MUSICSTYLE_NONE) in headless
void  EnableMusic();
void  DisableMusic();
sint32 IsMusicEnabled();     // returns 0 (FALSE) in headless

void  SetAutoRepeat(sint32 autoRepeat);
sint32 IsAutoRepeat();       // returns 0 (FALSE) in headless

} // namespace audio_observer
