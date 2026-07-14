//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : General declarations
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
// _DEBUG
// - Generate debug version
//
// _MSC_VER
// - Use Microsoft C++ extensions when set.
//
// USE_SDL
// - USE SDL for sound, cdrom, ... TODO
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - #pragmas commented out
// - includes fixed for case sensitive filesystems
// - sdl sound and cdrom
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "sound/civsound.h"
#include "SoundRecord.h"
#include "gs/fileio/prjfile.h"

#if defined(USE_SDL)
#include "ui/aui_sdl/aui_sdlcompat.h"
#endif

extern ProjectFile  *g_SoundPF;

CivSound::CivSound(const uint32 &associatedObject, const sint32 &soundID)
#if !defined(USE_SDL)
  : m_hAudio(0),
#else
  : m_Audio(nullptr), m_Channel(-1),
#endif
    m_associatedObject(associatedObject)

{
    const char *fname;
	if(soundID < 0)
		fname = nullptr;
	else
		fname = g_theSoundDB->Get(soundID)->GetValue();

	m_soundID = soundID;
    m_isPlaying = FALSE;
    m_isLooping = FALSE;

    if (nullptr == fname) {
        m_soundFilename[0] = 0;
        m_dataptr = nullptr;
        m_datasize = 0;
        return;
    }

    strlcpy(m_soundFilename, fname, sizeof(m_soundFilename));

    size_t      l_dataSize = 0;
    m_dataptr   = g_SoundPF->getData(m_soundFilename, l_dataSize);
    m_datasize  = static_cast<sint32>(l_dataSize);

#if !defined(USE_SDL)
	m_hAudio = AIL_quick_load_mem(m_dataptr, m_datasize);
#else
    // Use Mix_QuickLoad_WAV to avoid SDL2_mixer 2.8.x double-free bug in
    // Mix_LoadWAV_RW. The WAV files are already in the mixer format
    // (22050 Hz, 16-bit mono) so no conversion is needed.
    // Mix_QuickLoad_WAV sets chunk->allocated=0, so Mix_FreeChunk only
    // frees the Mix_Chunk struct, not the audio buffer. The audio buffer
    // is managed by ProjectFile (freed via freeData in the destructor).
    //
    // Guard against missing/empty sound assets: ProjectFile returns NULL or
    // a too-small buffer when the .wav isn't packaged or is corrupt, and
    // Mix_QuickLoad_WAV unconditionally reads the RIFF header (44 bytes).
    // SEGV on null+0xf observed during autoplay around turn 272.
    if (m_dataptr && m_datasize >= 44) {
        m_Audio = Mix_QuickLoad_WAV((Uint8 *) m_dataptr);
    } else {
        m_Audio = nullptr;
    }
#endif
}

CivSound::~CivSound()
{
#if !defined(USE_SDL)
    if (m_hAudio) {
        AIL_quick_unload(m_hAudio);
	}
#else
	if (m_Audio) {
		Mix_FreeChunk(m_Audio);
	}
#endif

    if (m_dataptr) {
        g_SoundPF->freeData(m_dataptr);
	}
}

const uint32
CivSound::GetAssociatedObject() const
{
	return m_associatedObject;
}

#if defined(USE_SDL)
Mix_Chunk *
CivSound::GetAudio() const
{
	return m_Audio;
}

const int
CivSound::GetChannel() const
{
    return m_Channel;
}

#else
HAUDIO
CivSound::GetHAudio() const
{
	return m_hAudio;
}
#endif

MBCHAR
*CivSound::GetSoundFilename()
{
	return m_soundFilename;
}

const sint32
CivSound::GetSoundID() const
{
	return m_soundID;
}

const sint32
CivSound::GetVolume()
{
	return m_volume;
}

const BOOL
CivSound::IsLooping()
{
	return m_isLooping;
}

const BOOL
CivSound::IsPlaying() const
{
	return m_isPlaying;
}

#if defined(USE_SDL)
void
CivSound::SetChannel(const int &channel)
{
    m_Channel = channel;
}
#endif

void
CivSound::SetIsLooping(const BOOL &looping)
{
	m_isLooping = looping;
}

void
CivSound::SetIsPlaying(const BOOL &is)
{
	m_isPlaying = is;
}

void
CivSound::SetVolume(const sint32 &volume)
{
#if !defined(USE_SDL)
    if (0 == m_hAudio) {
#else
    if (nullptr == m_Audio) {
#endif
        return;
    }

	// Assume max volume is 10...
	sint32 scaledVolume = (sint32)((double)volume * 12.7);

#if defined(USE_SDL)
	if (scaledVolume > MIX_MAX_VOLUME) {
		Mix_VolumeChunk(m_Audio, MIX_MAX_VOLUME);
	} else {
		Mix_VolumeChunk(m_Audio, (Uint8) scaledVolume);
	}
#else
	AIL_quick_set_volume(m_hAudio, scaledVolume, 64);
#endif
	m_volume = volume;
}
