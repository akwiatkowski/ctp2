#pragma once

#if defined(CTP2_USE_SDL3)
#define SDL_ENABLE_OLD_NAMES
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

#if defined(CTP2_USE_SDL3)
using CTP2_SDL_Mutex = SDL_Mutex;
using CTP2_SDL_Condition = SDL_Condition;
#else
using CTP2_SDL_Mutex = SDL_mutex;
using CTP2_SDL_Condition = SDL_cond;
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

inline bool CTP2_SDL_UpdateTexture(
	SDL_Texture *texture,
	SDL_Rect const *rect,
	void const *pixels,
	int pitch)
{
#if defined(CTP2_USE_SDL3)
	return SDL_UpdateTexture(texture, rect, pixels, pitch);
#else
	return SDL_UpdateTexture(texture, rect, pixels, pitch) == 0;
#endif
}

inline bool CTP2_SDL_GetTextureSize(SDL_Texture *texture, int *width, int *height)
{
#if defined(CTP2_USE_SDL3)
	float w = 0.0f;
	float h = 0.0f;
	if (!SDL_GetTextureSize(texture, &w, &h))
	{
		return false;
	}
	*width = static_cast<int>(w);
	*height = static_cast<int>(h);
	return true;
#else
	return SDL_QueryTexture(texture, nullptr, nullptr, width, height) == 0;
#endif
}

inline bool CTP2_SDL_SetRenderTarget(SDL_Renderer *renderer, SDL_Texture *target)
{
#if defined(CTP2_USE_SDL3)
	return SDL_SetRenderTarget(renderer, target);
#else
	return SDL_SetRenderTarget(renderer, target) == 0;
#endif
}

inline SDL_Surface *CTP2_SDL_CreateARGB8888Surface(int width, int height)
{
#if defined(CTP2_USE_SDL3)
	return SDL_CreateSurface(width, height, SDL_PIXELFORMAT_ARGB8888);
#else
	return SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);
#endif
}

inline bool CTP2_SDL_SaveBMP(SDL_Surface *surface, char const *path)
{
#if defined(CTP2_USE_SDL3)
	return SDL_SaveBMP(surface, path);
#else
	return SDL_SaveBMP(surface, path) == 0;
#endif
}

inline void CTP2_SDL_DestroySurface(SDL_Surface *surface)
{
#if defined(CTP2_USE_SDL3)
	SDL_DestroySurface(surface);
#else
	SDL_FreeSurface(surface);
#endif
}

inline SDL_Surface *CTP2_SDL_ConvertSurfaceFormat(SDL_Surface *surface, Uint32 format)
{
#if defined(CTP2_USE_SDL3)
	return SDL_ConvertSurface(surface, static_cast<SDL_PixelFormat>(format));
#else
	return SDL_ConvertSurfaceFormat(surface, format, 0);
#endif
}

inline int CTP2_SDL_SurfaceBitsPerPixel(SDL_Surface const *surface)
{
#if defined(CTP2_USE_SDL3)
	SDL_PixelFormatDetails const *details = SDL_GetPixelFormatDetails(surface->format);
	return details ? details->bits_per_pixel : SDL_BITSPERPIXEL(surface->format);
#else
	return surface->format->BitsPerPixel;
#endif
}

inline int CTP2_SDL_SurfaceBytesPerPixel(SDL_Surface const *surface)
{
#if defined(CTP2_USE_SDL3)
	SDL_PixelFormatDetails const *details = SDL_GetPixelFormatDetails(surface->format);
	return details ? details->bytes_per_pixel : SDL_BYTESPERPIXEL(surface->format);
#else
	return surface->format->BytesPerPixel;
#endif
}

inline Uint32 CTP2_SDL_SurfaceRMask(SDL_Surface const *surface)
{
#if defined(CTP2_USE_SDL3)
	SDL_PixelFormatDetails const *details = SDL_GetPixelFormatDetails(surface->format);
	return details ? details->Rmask : 0;
#else
	return surface->format->Rmask;
#endif
}

inline Uint32 CTP2_SDL_SurfaceGMask(SDL_Surface const *surface)
{
#if defined(CTP2_USE_SDL3)
	SDL_PixelFormatDetails const *details = SDL_GetPixelFormatDetails(surface->format);
	return details ? details->Gmask : 0;
#else
	return surface->format->Gmask;
#endif
}

inline Uint32 CTP2_SDL_SurfaceBMask(SDL_Surface const *surface)
{
#if defined(CTP2_USE_SDL3)
	SDL_PixelFormatDetails const *details = SDL_GetPixelFormatDetails(surface->format);
	return details ? details->Bmask : 0;
#else
	return surface->format->Bmask;
#endif
}

inline Uint8 CTP2_SDL_SurfaceRShift(SDL_Surface const *surface)
{
#if defined(CTP2_USE_SDL3)
	SDL_PixelFormatDetails const *details = SDL_GetPixelFormatDetails(surface->format);
	return details ? details->Rshift : 0;
#else
	return surface->format->Rshift;
#endif
}

inline Uint8 CTP2_SDL_SurfaceGShift(SDL_Surface const *surface)
{
#if defined(CTP2_USE_SDL3)
	SDL_PixelFormatDetails const *details = SDL_GetPixelFormatDetails(surface->format);
	return details ? details->Gshift : 0;
#else
	return surface->format->Gshift;
#endif
}

inline Uint8 CTP2_SDL_SurfaceBShift(SDL_Surface const *surface)
{
#if defined(CTP2_USE_SDL3)
	SDL_PixelFormatDetails const *details = SDL_GetPixelFormatDetails(surface->format);
	return details ? details->Bshift : 0;
#else
	return surface->format->Bshift;
#endif
}

inline void CTP2_SDL_LockMutex(CTP2_SDL_Mutex *mutex)
{
	SDL_LockMutex(mutex);
}

inline void CTP2_SDL_UnlockMutex(CTP2_SDL_Mutex *mutex)
{
	SDL_UnlockMutex(mutex);
}

inline CTP2_SDL_Condition *CTP2_SDL_CreateCondition()
{
#if defined(CTP2_USE_SDL3)
	return SDL_CreateCondition();
#else
	return SDL_CreateCond();
#endif
}

inline void CTP2_SDL_DestroyCondition(CTP2_SDL_Condition *condition)
{
#if defined(CTP2_USE_SDL3)
	SDL_DestroyCondition(condition);
#else
	SDL_DestroyCond(condition);
#endif
}

inline void CTP2_SDL_WaitCondition(
	CTP2_SDL_Condition *condition,
	CTP2_SDL_Mutex *mutex)
{
#if defined(CTP2_USE_SDL3)
	SDL_WaitCondition(condition, mutex);
#else
	SDL_CondWait(condition, mutex);
#endif
}

inline void CTP2_SDL_SignalCondition(CTP2_SDL_Condition *condition)
{
#if defined(CTP2_USE_SDL3)
	SDL_SignalCondition(condition);
#else
	SDL_CondSignal(condition);
#endif
}

inline SDL_Keycode CTP2_SDL_GetKeycode(SDL_KeyboardEvent const &event)
{
#if defined(CTP2_USE_SDL3)
	return event.key;
#else
	return event.keysym.sym;
#endif
}

inline SDL_Keymod CTP2_SDL_GetKeymod(SDL_KeyboardEvent const &event)
{
#if defined(CTP2_USE_SDL3)
	return event.mod;
#else
	return static_cast<SDL_Keymod>(event.keysym.mod);
#endif
}

inline Uint32 CTP2_SDL_GetMouseState(int *x, int *y)
{
#if defined(CTP2_USE_SDL3)
	float fx = 0.0f;
	float fy = 0.0f;
	Uint32 const state = SDL_GetMouseState(&fx, &fy);
	*x = static_cast<int>(fx);
	*y = static_cast<int>(fy);
	return state;
#else
	return SDL_GetMouseState(x, y);
#endif
}

inline bool CTP2_SDL_SaveRendererPixels(
	SDL_Renderer *renderer,
	char const *path,
	int width,
	int height)
{
#if defined(CTP2_USE_SDL3)
	SDL_Surface *shot = SDL_RenderReadPixels(renderer, nullptr);
	if (!shot)
	{
		return false;
	}
	bool const ok = CTP2_SDL_SaveBMP(shot, path);
	CTP2_SDL_DestroySurface(shot);
	return ok;
#else
	SDL_Surface *shot = CTP2_SDL_CreateARGB8888Surface(width, height);
	if (!shot)
	{
		return false;
	}
	bool const ok = SDL_RenderReadPixels(renderer, nullptr,
		SDL_PIXELFORMAT_ARGB8888, shot->pixels, shot->pitch) == 0
		&& CTP2_SDL_SaveBMP(shot, path);
	CTP2_SDL_DestroySurface(shot);
	return ok;
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
