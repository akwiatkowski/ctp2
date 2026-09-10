#pragma once

#define SDL_ENABLE_OLD_NAMES
#include <SDL3_mixer/SDL_mixer.h>

using Mix_Chunk = MIX_Audio;

inline MIX_Mixer *g_ctp2_sdl3Mixer = nullptr;
inline MIX_Track *g_ctp2_sdl3Tracks[32] = {};

#ifndef SDL_INIT_NOPARACHUTE
#define SDL_INIT_NOPARACHUTE 0

inline int Mix_OpenAudio(int frequency, int format, int channels, int chunksize)
{
	(void)chunksize;
	if (!MIX_Init())
	{
		return -1;
	}

	SDL_AudioSpec spec = {};
	spec.freq = frequency;
	spec.format = static_cast<SDL_AudioFormat>(format);
	spec.channels = channels;
	g_ctp2_sdl3Mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
	return g_ctp2_sdl3Mixer ? 0 : -1;
}

inline void Mix_CloseAudio()
{
	for (MIX_Track *&track : g_ctp2_sdl3Tracks)
	{
		if (track)
		{
			MIX_DestroyTrack(track);
			track = nullptr;
		}
	}
	MIX_DestroyMixer(g_ctp2_sdl3Mixer);
	g_ctp2_sdl3Mixer = nullptr;
	MIX_Quit();
}

inline Mix_Chunk *CTP2_Mix_QuickLoadWAV(Uint8 *mem, size_t size)
{
	return MIX_LoadAudioNoCopy(g_ctp2_sdl3Mixer, mem, size, false);
}

inline void Mix_FreeChunk(Mix_Chunk *chunk)
{
	MIX_DestroyAudio(chunk);
}

inline int Mix_PlayChannel(int channel, Mix_Chunk *chunk, int loops)
{
	if (!g_ctp2_sdl3Mixer || !chunk)
	{
		return -1;
	}

	if (channel < 0)
	{
		for (int i = 0; i < 32; ++i)
		{
			if (!g_ctp2_sdl3Tracks[i] || MIX_GetTrackRemaining(g_ctp2_sdl3Tracks[i]) == 0)
			{
				channel = i;
				break;
			}
		}
	}

	if (channel < 0 || channel >= 32)
	{
		return -1;
	}

	MIX_Track *&track = g_ctp2_sdl3Tracks[channel];
	if (!track)
	{
		track = MIX_CreateTrack(g_ctp2_sdl3Mixer);
	}
	if (!track || !MIX_SetTrackAudio(track, chunk))
	{
		return -1;
	}

	SDL_PropertiesID props = SDL_CreateProperties();
	if (props)
	{
		SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, loops);
	}
	bool const ok = MIX_PlayTrack(track, props);
	if (props)
	{
		SDL_DestroyProperties(props);
	}
	return ok ? channel : -1;
}

inline int Mix_Playing(int channel)
{
	if (channel < 0 || channel >= 32 || !g_ctp2_sdl3Tracks[channel])
	{
		return 0;
	}
	return MIX_GetTrackRemaining(g_ctp2_sdl3Tracks[channel]) != 0;
}

inline int Mix_HaltChannel(int channel)
{
	if (channel < 0 || channel >= 32 || !g_ctp2_sdl3Tracks[channel])
	{
		return 0;
	}
	return MIX_StopTrack(g_ctp2_sdl3Tracks[channel], 0) ? 0 : -1;
}

inline int Mix_VolumeChunk(Mix_Chunk *chunk, int volume)
{
	(void)chunk;
	return volume;
}

#define AUDIO_S16SYS SDL_AUDIO_S16
#define MIX_MAX_VOLUME 128
#else
#include <SDL2/SDL_mixer.h>

inline Mix_Chunk *CTP2_Mix_QuickLoadWAV(Uint8 *mem, size_t size)
{
	(void)size;
	return Mix_QuickLoad_WAV(mem);
}
#endif
