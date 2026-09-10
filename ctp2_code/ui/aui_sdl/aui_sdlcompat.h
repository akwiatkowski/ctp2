#pragma once

#define SDL_ENABLE_OLD_NAMES
#include <SDL3/SDL.h>

#define CTP2_SDL_WINDOW_SHOWN 0
#define CTP2_SDL_WINDOW_ALLOW_HIGHDPI SDL_WINDOW_HIGH_PIXEL_DENSITY

using CTP2_SDL_Mutex = SDL_Mutex;
using CTP2_SDL_Condition = SDL_Condition;

inline SDL_Window *CTP2_SDL_CreateWindow(
	char const *title,
	int width,
	int height,
	Uint64 flags)
{
	return SDL_CreateWindow(title, width, height, flags);
}

inline SDL_Renderer *CTP2_SDL_CreateRenderer(SDL_Window *window, bool software)
{
	SDL_Renderer *renderer = SDL_CreateRenderer(window, software ? "software" : nullptr);
	if (renderer && !software)
	{
		SDL_SetRenderVSync(renderer, 1);
	}
	return renderer;
}

inline bool CTP2_SDL_RenderTexture(SDL_Renderer *renderer, SDL_Texture *texture)
{
	return SDL_RenderTexture(renderer, texture, nullptr, nullptr);
}

// P11 Stage 2 F (smooth camera): copy the whole texture into a destination
// rectangle (offset + scale) instead of filling the render target. Used to
// pan/zoom the world + fog layers on the GPU while the UI layer stays full
// screen. SDL3 wants an SDL_FRect dest; SDL2 an SDL_Rect.
inline bool CTP2_SDL_RenderTextureDst(SDL_Renderer *renderer, SDL_Texture *texture,
                                      float x, float y, float w, float h)
{
	SDL_FRect dst = { x, y, w, h };
	return SDL_RenderTexture(renderer, texture, nullptr, &dst);
}

// P11 Stage 3 G1 (terrain quads): copy a sub-rectangle of a source texture
// (an atlas slot) into a destination rectangle of the current render target.
// Used to draw each visible terrain cell from the tile atlas into the world
// texture. SDL3 wants SDL_FRect src/dst; SDL2 SDL_Rect.
inline bool CTP2_SDL_RenderTextureSrcDst(SDL_Renderer *renderer, SDL_Texture *texture,
                                         int sx, int sy, int sw, int sh,
                                         float dx, float dy, float dw, float dh)
{
	SDL_FRect src = { (float)sx, (float)sy, (float)sw, (float)sh };
	SDL_FRect dst = { dx, dy, dw, dh };
	return SDL_RenderTexture(renderer, texture, &src, &dst);
}

inline bool CTP2_SDL_RenderTextureSrcDstFlip(SDL_Renderer *renderer, SDL_Texture *texture,
                                             int sx, int sy, int sw, int sh,
                                             float dx, float dy, float dw, float dh,
                                             bool mirror)
{
	SDL_FRect src = { (float)sx, (float)sy, (float)sw, (float)sh };
	SDL_FRect dst = { dx, dy, dw, dh };
	return SDL_RenderTextureRotated(renderer, texture, &src, &dst, 0.0, nullptr,
		mirror ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
}

// Texture pixel dimensions. SDL2 returns ints via SDL_QueryTexture; SDL3 returns
// floats via SDL_GetTextureSize.
inline void CTP2_SDL_QueryTextureSize(SDL_Texture *texture, int &w, int &h)
{
	w = 0; h = 0;
	float fw = 0.0f, fh = 0.0f;
	SDL_GetTextureSize(texture, &fw, &fh);
	w = (int)fw; h = (int)fh;
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
	SDL_FRect src = { sx, sy, sw, sh };
	SDL_FRect dst = { dx, dy, dw, dh };
	return SDL_RenderTexture(renderer, texture, &src, &dst);
}

inline bool CTP2_SDL_UpdateTexture(
	SDL_Texture *texture,
	SDL_Rect const *rect,
	void const *pixels,
	int pitch)
{
	return SDL_UpdateTexture(texture, rect, pixels, pitch);
}

// Linear sampling -- the right choice when MINIFYING, where nearest aliases
// badly and shimmers as the source rect slides during a pan.
inline bool CTP2_SDL_SetTextureLinear(SDL_Texture *texture)
{
	return SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
}

inline bool CTP2_SDL_SetTextureNearest(SDL_Texture *texture)
{
	return SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
}

inline bool CTP2_SDL_GetTextureSize(SDL_Texture *texture, int *width, int *height)
{
	float w = 0.0f;
	float h = 0.0f;
	if (!SDL_GetTextureSize(texture, &w, &h))
	{
		return false;
	}
	*width = static_cast<int>(w);
	*height = static_cast<int>(h);
	return true;
}

inline bool CTP2_SDL_SetRenderTarget(SDL_Renderer *renderer, SDL_Texture *target)
{
	return SDL_SetRenderTarget(renderer, target);
}

inline SDL_Surface *CTP2_SDL_CreateARGB8888Surface(int width, int height)
{
	return SDL_CreateSurface(width, height, SDL_PIXELFORMAT_ARGB8888);
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
	SDL_PixelFormat const format = SDL_GetPixelFormatForMasks(
		depth, rmask, gmask, bmask, amask);
	return format == SDL_PIXELFORMAT_UNKNOWN ? nullptr : SDL_CreateSurface(width, height, format);
}

inline bool CTP2_SDL_SaveBMP(SDL_Surface *surface, char const *path)
{
	return SDL_SaveBMP(surface, path);
}

inline bool CTP2_SDL_SetColorKey(SDL_Surface *surface, bool enabled, Uint32 key)
{
	return SDL_SetColorKey(surface, enabled, key);
}

inline void CTP2_SDL_DestroySurface(SDL_Surface *surface)
{
	SDL_DestroySurface(surface);
}

inline SDL_Surface *CTP2_SDL_ConvertSurfaceFormat(SDL_Surface *surface, Uint32 format)
{
	return SDL_ConvertSurface(surface, static_cast<SDL_PixelFormat>(format));
}

inline int CTP2_SDL_SurfaceBitsPerPixel(SDL_Surface const *surface)
{
	SDL_PixelFormatDetails const *details = SDL_GetPixelFormatDetails(surface->format);
	return details ? details->bits_per_pixel : SDL_BITSPERPIXEL(surface->format);
}

inline int CTP2_SDL_SurfaceBytesPerPixel(SDL_Surface const *surface)
{
	SDL_PixelFormatDetails const *details = SDL_GetPixelFormatDetails(surface->format);
	return details ? details->bytes_per_pixel : SDL_BYTESPERPIXEL(surface->format);
}

inline Uint32 CTP2_SDL_SurfaceRMask(SDL_Surface const *surface)
{
	SDL_PixelFormatDetails const *details = SDL_GetPixelFormatDetails(surface->format);
	return details ? details->Rmask : 0;
}

inline Uint32 CTP2_SDL_SurfaceGMask(SDL_Surface const *surface)
{
	SDL_PixelFormatDetails const *details = SDL_GetPixelFormatDetails(surface->format);
	return details ? details->Gmask : 0;
}

inline Uint32 CTP2_SDL_SurfaceBMask(SDL_Surface const *surface)
{
	SDL_PixelFormatDetails const *details = SDL_GetPixelFormatDetails(surface->format);
	return details ? details->Bmask : 0;
}

inline Uint8 CTP2_SDL_SurfaceRShift(SDL_Surface const *surface)
{
	SDL_PixelFormatDetails const *details = SDL_GetPixelFormatDetails(surface->format);
	return details ? details->Rshift : 0;
}

inline Uint8 CTP2_SDL_SurfaceGShift(SDL_Surface const *surface)
{
	SDL_PixelFormatDetails const *details = SDL_GetPixelFormatDetails(surface->format);
	return details ? details->Gshift : 0;
}

inline Uint8 CTP2_SDL_SurfaceBShift(SDL_Surface const *surface)
{
	SDL_PixelFormatDetails const *details = SDL_GetPixelFormatDetails(surface->format);
	return details ? details->Bshift : 0;
}

inline Uint32 CTP2_SDL_MapRGB(SDL_Surface const *surface, Uint8 r, Uint8 g, Uint8 b)
{
	SDL_PixelFormatDetails const *details = SDL_GetPixelFormatDetails(surface->format);
	return SDL_MapRGB(details, nullptr, r, g, b);
}

inline void CTP2_SDL_LockMutex(CTP2_SDL_Mutex *mutex)
{
	SDL_LockMutex(mutex);
}

inline bool CTP2_SDL_LockMutexChecked(CTP2_SDL_Mutex *mutex)
{
	SDL_LockMutex(mutex);
	return true;
}

inline void CTP2_SDL_UnlockMutex(CTP2_SDL_Mutex *mutex)
{
	SDL_UnlockMutex(mutex);
}

inline CTP2_SDL_Condition *CTP2_SDL_CreateCondition()
{
	return SDL_CreateCondition();
}

inline void CTP2_SDL_DestroyCondition(CTP2_SDL_Condition *condition)
{
	SDL_DestroyCondition(condition);
}

inline void CTP2_SDL_WaitCondition(
	CTP2_SDL_Condition *condition,
	CTP2_SDL_Mutex *mutex)
{
	SDL_WaitCondition(condition, mutex);
}

inline void CTP2_SDL_SignalCondition(CTP2_SDL_Condition *condition)
{
	SDL_SignalCondition(condition);
}
// Text input (city names, chat): SDL2 takes no window, SDL3 takes one.
// ASCII keeps flowing through KEYDOWN macros; TEXTINPUT carries the rest.
inline void CTP2_SDL_StartTextInput(SDL_Window *window)
{
	SDL_StartTextInput(window);
}

inline void CTP2_SDL_StopTextInput(SDL_Window *window)
{
	SDL_StopTextInput(window);
}

inline SDL_Keycode CTP2_SDL_GetKeycode(SDL_KeyboardEvent const &event)
{
	return event.key;
}

inline SDL_Keymod CTP2_SDL_GetKeymod(SDL_KeyboardEvent const &event)
{
	return event.mod;
}

inline bool CTP2_SDL_IsKeyDown(SDL_Event const &event)
{
	return event.type == SDL_KEYDOWN;
}

inline bool CTP2_SDL_IsKeyDown(SDL_KeyboardEvent const &event)
{
	return event.down;
}

inline bool CTP2_SDL_IsMouseButtonDown(SDL_Event const &event)
{
	return event.type == SDL_MOUSEBUTTONDOWN;
}

inline void CTP2_SDL_ShowCursor()
{
	SDL_ShowCursor();
}

// Wrap an existing ARGB8888 pixel buffer in an SDL_Surface (no copy; the
// buffer must outlive the surface). The two APIs order arguments differently.
inline SDL_Surface *CTP2_SDL_CreateARGBSurfaceFrom(
	Uint32 *pixels, int width, int height, int pitch)
{
	return SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_ARGB8888,
	                             pixels, pitch);
}

// Touch finger id — the struct field casing differs (SDL2 fingerId,
// SDL3 fingerID). Event TYPE constants (SDL_FINGERDOWN etc.) come from
// SDL_ENABLE_OLD_NAMES and need no wrapper.
inline Sint64 CTP2_SDL_FingerId(SDL_TouchFingerEvent const &event)
{
	return static_cast<Sint64>(event.fingerID);
}

// Last event type in the contiguous finger range, for SDL_PeepEvents.
// SDL3 appended FINGER_CANCELED (a lifted-by-the-OS finger) after MOTION;
// SDL2's range ends at MOTION.
#define CTP2_SDL_FINGER_RANGE_LAST SDL_EVENT_FINGER_CANCELED

inline Uint32 CTP2_SDL_GetMouseState(int *x, int *y)
{
	float fx = 0.0f;
	float fy = 0.0f;
	Uint32 const state = SDL_GetMouseState(&fx, &fy);
	*x = static_cast<int>(fx);
	*y = static_cast<int>(fy);
	return state;
}

inline void CTP2_SDL_HideCursor()
{
	SDL_HideCursor();
}

// Fill an integer rect on the current target. SDL3 took SDL_RenderFillRect to
// float rects; this keeps call sites in integer pixel space.
inline bool CTP2_SDL_RenderFillRectI(SDL_Renderer *renderer, SDL_Rect const *rect)
{
	SDL_FRect const fr = { (float)rect->x, (float)rect->y,
	                       (float)rect->w, (float)rect->h };
	return SDL_RenderFillRect(renderer, &fr);
}

// Read a single ARGB pixel back from the current render target. Reading one
// pixel rather than the whole target matters when the target is large — the
// P13 whole-map texture is ~133MB, and a full readback to prove one value would
// allocate all of it.
inline bool CTP2_SDL_RenderReadPixelARGB(SDL_Renderer *renderer, int x, int y,
                                         uint32 *out)
{
	SDL_Rect const rect = { x, y, 1, 1 };
	SDL_Surface *px = SDL_RenderReadPixels(renderer, &rect);
	if (!px)
	{
		return false;
	}
	SDL_Surface *argb = SDL_ConvertSurface(px, SDL_PIXELFORMAT_ARGB8888);
	bool ok = false;
	if (argb && argb->pixels)
	{
		*out = *static_cast<uint32 *>(argb->pixels);
		ok = true;
	}
	if (argb) CTP2_SDL_DestroySurface(argb);
	CTP2_SDL_DestroySurface(px);
	return ok;
}

inline bool CTP2_SDL_SaveRendererPixels(
	SDL_Renderer *renderer,
	char const *path,
	int width,
	int height)
{
	SDL_Surface *shot = SDL_RenderReadPixels(renderer, nullptr);
	if (!shot)
	{
		return false;
	}
	bool const ok = CTP2_SDL_SaveBMP(shot, path);
	CTP2_SDL_DestroySurface(shot);
	return ok;
}

inline bool CTP2_SDL_SetRenderLogicalSize(SDL_Renderer *renderer, int width, int height)
{
	return SDL_SetRenderLogicalPresentation(
		renderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
}

inline bool CTP2_SDL_RenderWindowToLogical(
	SDL_Renderer *renderer,
	float windowX,
	float windowY,
	float *logicalX,
	float *logicalY)
{
	return SDL_RenderCoordinatesFromWindow(renderer, windowX, windowY, logicalX, logicalY);
}
// Backing-store scale for HiDPI (ctp2-078): logical points vs physical
// pixels. SDL3 reports it directly; SDL2 derives it from drawable size.
inline float CTP2_SDL_GetWindowDisplayScale(SDL_Window *window)
{
	float scale = SDL_GetWindowDisplayScale(window);
	return (scale > 0.0f) ? scale : 1.0f;
}
