#pragma once

#if defined(CTP2_USE_SDL3)
#include <SDL3/SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#if defined(CTP2_USE_SDL3)
#define CTP2_SDL_WINDOW_SHOWN 0
#define CTP2_SDL_WINDOW_ALLOW_HIGHDPI SDL_WINDOW_HIGH_PIXEL_DENSITY
#else
#define CTP2_SDL_WINDOW_SHOWN SDL_WINDOW_SHOWN
#define CTP2_SDL_WINDOW_ALLOW_HIGHDPI SDL_WINDOW_ALLOW_HIGHDPI
#endif

inline SDL_Window *CTP2_SDL_CreateWindow(
	char const *title,
	int width,
	int height,
	Uint64 flags)
{
#if defined(CTP2_USE_SDL3)
	return SDL_CreateWindow(title, width, height, flags);
#else
	return SDL_CreateWindow(
		title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags);
#endif
}

inline SDL_Renderer *CTP2_SDL_CreateRenderer(SDL_Window *window, bool software)
{
#if defined(CTP2_USE_SDL3)
	SDL_Renderer *renderer = SDL_CreateRenderer(window, software ? "software" : nullptr);
	if (renderer && !software)
	{
		SDL_SetRenderVSync(renderer, 1);
	}
	return renderer;
#else
	Uint32 const flags = software
		? SDL_RENDERER_SOFTWARE
		: SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
	return SDL_CreateRenderer(window, -1, flags);
#endif
}

inline bool CTP2_SDL_RenderTexture(SDL_Renderer *renderer, SDL_Texture *texture)
{
#if defined(CTP2_USE_SDL3)
	return SDL_RenderTexture(renderer, texture, nullptr, nullptr);
#else
	return SDL_RenderCopy(renderer, texture, nullptr, nullptr) == 0;
#endif
}

inline bool CTP2_SDL_SetRenderLogicalSize(SDL_Renderer *renderer, int width, int height)
{
#if defined(CTP2_USE_SDL3)
	return SDL_SetRenderLogicalPresentation(
		renderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
#else
	return SDL_RenderSetLogicalSize(renderer, width, height) == 0;
#endif
}

inline bool CTP2_SDL_RenderWindowToLogical(
	SDL_Renderer *renderer,
	float windowX,
	float windowY,
	float *logicalX,
	float *logicalY)
{
#if defined(CTP2_USE_SDL3)
	return SDL_RenderCoordinatesFromWindow(renderer, windowX, windowY, logicalX, logicalY);
#else
	SDL_RenderWindowToLogical(renderer, windowX, windowY, logicalX, logicalY);
	return true;
#endif
}
