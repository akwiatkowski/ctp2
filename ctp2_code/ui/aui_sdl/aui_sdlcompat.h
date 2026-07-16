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

// P11 Stage 2 F (smooth camera): copy the whole texture into a destination
// rectangle (offset + scale) instead of filling the render target. Used to
// pan/zoom the world + fog layers on the GPU while the UI layer stays full
// screen. SDL3 wants an SDL_FRect dest; SDL2 an SDL_Rect.
inline bool CTP2_SDL_RenderTextureDst(SDL_Renderer *renderer, SDL_Texture *texture,
                                      float x, float y, float w, float h)
{
#if defined(CTP2_USE_SDL3)
	SDL_FRect dst = { x, y, w, h };
	return SDL_RenderTexture(renderer, texture, nullptr, &dst);
#else
	SDL_Rect dst = { (int)x, (int)y, (int)w, (int)h };
	return SDL_RenderCopy(renderer, texture, nullptr, &dst) == 0;
#endif
}

// P11 Stage 3 G1 (terrain quads): copy a sub-rectangle of a source texture
// (an atlas slot) into a destination rectangle of the current render target.
// Used to draw each visible terrain cell from the tile atlas into the world
// texture. SDL3 wants SDL_FRect src/dst; SDL2 SDL_Rect.
inline bool CTP2_SDL_RenderTextureSrcDst(SDL_Renderer *renderer, SDL_Texture *texture,
                                         int sx, int sy, int sw, int sh,
                                         float dx, float dy, float dw, float dh)
{
#if defined(CTP2_USE_SDL3)
	SDL_FRect src = { (float)sx, (float)sy, (float)sw, (float)sh };
	SDL_FRect dst = { dx, dy, dw, dh };
	return SDL_RenderTexture(renderer, texture, &src, &dst);
#else
	SDL_Rect src = { sx, sy, sw, sh };
	SDL_Rect dst = { (int)dx, (int)dy, (int)dw, (int)dh };
	return SDL_RenderCopy(renderer, texture, &src, &dst) == 0;
#endif
}

// Texture pixel dimensions. SDL2 returns ints via SDL_QueryTexture; SDL3 returns
// floats via SDL_GetTextureSize.
inline void CTP2_SDL_QueryTextureSize(SDL_Texture *texture, int &w, int &h)
{
	w = 0; h = 0;
#if defined(CTP2_USE_SDL3)
	float fw = 0.0f, fh = 0.0f;
	SDL_GetTextureSize(texture, &fw, &fh);
	w = (int)fw; h = (int)fh;
#else
	SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
#endif
}

// P11 2a (buttery pan, ADR-001): copy a FLOAT sub-rectangle of a source texture
// into a destination rectangle. Same as CTP2_SDL_RenderTextureSrcDst but the
// source rect is float, so the world viewport can be windowed into the oversized
// world texture with sub-pixel precision (smooth pan) on SDL3. SDL2 rounds the
// source to ints (SDL_Rect), so its pan is pixel- rather than sub-pixel-smooth.
inline bool CTP2_SDL_RenderTextureWindow(SDL_Renderer *renderer, SDL_Texture *texture,
                                         float sx, float sy, float sw, float sh,
                                         float dx, float dy, float dw, float dh)
{
#if defined(CTP2_USE_SDL3)
	SDL_FRect src = { sx, sy, sw, sh };
	SDL_FRect dst = { dx, dy, dw, dh };
	return SDL_RenderTexture(renderer, texture, &src, &dst);
#else
	SDL_Rect src = { (int)(sx + 0.5f), (int)(sy + 0.5f), (int)(sw + 0.5f), (int)(sh + 0.5f) };
	SDL_Rect dst = { (int)dx, (int)dy, (int)dw, (int)dh };
	return SDL_RenderCopy(renderer, texture, &src, &dst) == 0;
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

inline SDL_Surface *CTP2_SDL_CreateRGBSurface(
	int width,
	int height,
	int depth,
	Uint32 rmask,
	Uint32 gmask,
	Uint32 bmask,
	Uint32 amask)
{
#if defined(CTP2_USE_SDL3)
	SDL_PixelFormat const format = SDL_GetPixelFormatForMasks(
		depth, rmask, gmask, bmask, amask);
	return format == SDL_PIXELFORMAT_UNKNOWN ? nullptr : SDL_CreateSurface(width, height, format);
#else
	return SDL_CreateRGBSurface(0, width, height, depth, rmask, gmask, bmask, amask);
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

inline bool CTP2_SDL_SetColorKey(SDL_Surface *surface, bool enabled, Uint32 key)
{
#if defined(CTP2_USE_SDL3)
	return SDL_SetColorKey(surface, enabled, key);
#else
	return SDL_SetColorKey(surface, enabled ? SDL_TRUE : SDL_FALSE, key) == 0;
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

inline Uint32 CTP2_SDL_MapRGB(SDL_Surface const *surface, Uint8 r, Uint8 g, Uint8 b)
{
#if defined(CTP2_USE_SDL3)
	SDL_PixelFormatDetails const *details = SDL_GetPixelFormatDetails(surface->format);
	return SDL_MapRGB(details, nullptr, r, g, b);
#else
	return SDL_MapRGB(surface->format, r, g, b);
#endif
}

inline void CTP2_SDL_LockMutex(CTP2_SDL_Mutex *mutex)
{
	SDL_LockMutex(mutex);
}

inline bool CTP2_SDL_LockMutexChecked(CTP2_SDL_Mutex *mutex)
{
#if defined(CTP2_USE_SDL3)
	SDL_LockMutex(mutex);
	return true;
#else
	return SDL_LockMutex(mutex) == 0;
#endif
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

inline bool CTP2_SDL_IsKeyDown(SDL_Event const &event)
{
	return event.type == SDL_KEYDOWN;
}

inline bool CTP2_SDL_IsKeyDown(SDL_KeyboardEvent const &event)
{
#if defined(CTP2_USE_SDL3)
	return event.down;
#else
	return event.state == SDL_PRESSED;
#endif
}

inline bool CTP2_SDL_IsMouseButtonDown(SDL_Event const &event)
{
	return event.type == SDL_MOUSEBUTTONDOWN;
}

// Touch finger id — the struct field casing differs (SDL2 fingerId,
// SDL3 fingerID). Event TYPE constants (SDL_FINGERDOWN etc.) come from
// SDL_ENABLE_OLD_NAMES and need no wrapper.
inline Sint64 CTP2_SDL_FingerId(SDL_TouchFingerEvent const &event)
{
#if defined(CTP2_USE_SDL3)
	return static_cast<Sint64>(event.fingerID);
#else
	return event.fingerId;
#endif
}

// Last event type in the contiguous finger range, for SDL_PeepEvents.
// SDL3 appended FINGER_CANCELED (a lifted-by-the-OS finger) after MOTION;
// SDL2's range ends at MOTION.
#if defined(CTP2_USE_SDL3)
#define CTP2_SDL_FINGER_RANGE_LAST SDL_EVENT_FINGER_CANCELED
#else
#define CTP2_SDL_FINGER_RANGE_LAST SDL_FINGERMOTION
#endif

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

inline void CTP2_SDL_HideCursor()
{
#if defined(CTP2_USE_SDL3)
	SDL_HideCursor();
#else
	SDL_ShowCursor(SDL_DISABLE);
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
