#pragma once

#if defined(CTP2_USE_SDL3)
#include <SDL3/SDL.h>
#else
#include <SDL2/SDL.h>
#endif

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
