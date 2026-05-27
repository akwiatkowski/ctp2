// test/cpp/test_audio_observer.cpp
// Spy contract tests for audio_observer fan-out interface.
//
// Verifies register/unregister, dispatch of every fan-out method onto a
// recording Impl, and safe-default returns when no Impl is registered.
// Runs without engine initialisation (< 1 ms).

#include "ctp/c3.h"
#include "doctest.h"
#include "gs/core/audio_observer.h"

namespace {

struct RecordingSpy : audio_observer::Impl {
    // AddSound
    int addSoundCalls = 0;
    sint32 lastSoundType = -1;
    uint32 lastSoundObj  = 0;
    sint32 lastSoundId   = -1;
    sint32 lastSoundX    = -1;
    sint32 lastSoundY    = -1;
    void AddSound(sint32 t, uint32 o, sint32 id, sint32 x, sint32 y) override {
        ++addSoundCalls;
        lastSoundType = t;
        lastSoundObj  = o;
        lastSoundId   = id;
        lastSoundX    = x;
        lastSoundY    = y;
    }

    // AddGameSound
    int addGameSoundCalls = 0;
    sint32 lastGameSound = -1;
    void AddGameSound(sint32 sound) override {
        ++addGameSoundCalls;
        lastGameSound = sound;
    }

    // SetVolume
    int setVolumeCalls = 0;
    sint32 lastVolumeType = -1;
    uint32 lastVolume = 0;
    void SetVolume(sint32 type, uint32 volume) override {
        ++setVolumeCalls;
        lastVolumeType = type;
        lastVolume = volume;
    }

    // Music control
    int setMusicStyleCalls = 0;
    sint32 lastMusicStyle = -1;
    void SetMusicStyle(sint32 style) override {
        ++setMusicStyleCalls;
        lastMusicStyle = style;
    }

    sint32 getMusicStyleReturn = 42;
    sint32 GetMusicStyle() override { return getMusicStyleReturn; }

    int enableMusicCalls = 0;
    void EnableMusic() override { ++enableMusicCalls; }

    int disableMusicCalls = 0;
    void DisableMusic() override { ++disableMusicCalls; }

    sint32 isMusicEnabledReturn = 1;
    sint32 IsMusicEnabled() override { return isMusicEnabledReturn; }

    // AutoRepeat
    int setAutoRepeatCalls = 0;
    sint32 lastAutoRepeat = -1;
    void SetAutoRepeat(sint32 autoRepeat) override {
        ++setAutoRepeatCalls;
        lastAutoRepeat = autoRepeat;
    }

    sint32 isAutoRepeatReturn = 1;
    sint32 IsAutoRepeat() override { return isAutoRepeatReturn; }
};

struct ScopedSpy {
    audio_observer::Impl *prev;
    explicit ScopedSpy(audio_observer::Impl *s) {
        prev = audio_observer::Get();
        audio_observer::Register(s);
    }
    ~ScopedSpy() { audio_observer::Register(prev); }
};

} // anonymous namespace

TEST_CASE("audio_observer::Get returns null when no Impl is registered") {
    audio_observer::Impl *prev = audio_observer::Get();
    audio_observer::Register(nullptr);
    CHECK(audio_observer::Get() == nullptr);
    audio_observer::Register(prev);
}

TEST_CASE("audio_observer::Register installs the Impl; Get returns it") {
    RecordingSpy spy;
    audio_observer::Impl *prev = audio_observer::Get();
    audio_observer::Register(&spy);
    CHECK(audio_observer::Get() == &spy);
    audio_observer::Register(prev);
}

TEST_CASE("audio_observer::AddSound dispatches all five args to Impl") {
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    audio_observer::AddSound(1, 2, 3, 4, 5);
    CHECK(spy.addSoundCalls == 1);
    CHECK(spy.lastSoundType == 1);
    CHECK(spy.lastSoundObj  == 2u);
    CHECK(spy.lastSoundId   == 3);
    CHECK(spy.lastSoundX    == 4);
    CHECK(spy.lastSoundY    == 5);
}

TEST_CASE("audio_observer::AddGameSound dispatches the sound id") {
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    audio_observer::AddGameSound(99);
    CHECK(spy.addGameSoundCalls == 1);
    CHECK(spy.lastGameSound == 99);
}

TEST_CASE("audio_observer::SetVolume dispatches type + volume") {
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    audio_observer::SetVolume(7, 128);
    CHECK(spy.setVolumeCalls == 1);
    CHECK(spy.lastVolumeType == 7);
    CHECK(spy.lastVolume == 128);
}

TEST_CASE("audio_observer::SetMusicStyle dispatches the style") {
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    audio_observer::SetMusicStyle(3);
    CHECK(spy.setMusicStyleCalls == 1);
    CHECK(spy.lastMusicStyle == 3);
}

TEST_CASE("audio_observer::GetMusicStyle returns 0 when no Impl is registered") {
    audio_observer::Impl *prev = audio_observer::Get();
    audio_observer::Register(nullptr);
    CHECK(audio_observer::GetMusicStyle() == 0);
    audio_observer::Register(prev);
}

TEST_CASE("audio_observer::GetMusicStyle returns the Impl's value when registered") {
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    CHECK(audio_observer::GetMusicStyle() == 42);
}

TEST_CASE("audio_observer::EnableMusic dispatches to Impl") {
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    audio_observer::EnableMusic();
    CHECK(spy.enableMusicCalls == 1);
}

TEST_CASE("audio_observer::DisableMusic dispatches to Impl") {
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    audio_observer::DisableMusic();
    CHECK(spy.disableMusicCalls == 1);
}

TEST_CASE("audio_observer::IsMusicEnabled returns 0 when no Impl is registered") {
    audio_observer::Impl *prev = audio_observer::Get();
    audio_observer::Register(nullptr);
    CHECK(audio_observer::IsMusicEnabled() == 0);
    audio_observer::Register(prev);
}

TEST_CASE("audio_observer::IsMusicEnabled returns the Impl's value when registered") {
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    CHECK(audio_observer::IsMusicEnabled() == 1);
}

TEST_CASE("audio_observer::SetAutoRepeat dispatches to Impl") {
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    audio_observer::SetAutoRepeat(1);
    CHECK(spy.setAutoRepeatCalls == 1);
    CHECK(spy.lastAutoRepeat == 1);
}

TEST_CASE("audio_observer::IsAutoRepeat returns 0 when no Impl is registered") {
    audio_observer::Impl *prev = audio_observer::Get();
    audio_observer::Register(nullptr);
    CHECK(audio_observer::IsAutoRepeat() == 0);
    audio_observer::Register(prev);
}

TEST_CASE("audio_observer::IsAutoRepeat returns the Impl's value when registered") {
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    CHECK(audio_observer::IsAutoRepeat() == 1);
}
