#ifndef __aui_sdl__aui_sdlkeyboard_h__
#define __aui_sdl__aui_sdlkeyboard_h__ 1

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#include "os/include/ctp2_config.h"

#if defined(__AUI_USE_SDL__)

#include <queue>

#include "ui/aui_common/aui_keyboard.h"
#include "ui/aui_sdl/aui_sdlinput.h"

// Secondary keyboard event queue API.  The queue and its mutex used to
// be exposed as raw globals; they now live as file-scope statics in
// aui_sdlkeyboard.cpp.  The main thread (civ3_main.cpp / civapp.cpp)
// peeps SDL keyboard events and pushes them via PushQueueEvent; the
// SDL keyboard input device drains them via TryPopQueueEvent on its
// own poll cycle.  Init/Destroy bracket the mutex lifetime.
void aui_sdlkbd_InitQueueMutex(void);
void aui_sdlkbd_DestroyQueueMutex(void);
void aui_sdlkbd_PushQueueEvent(SDL_Event const & event);
bool aui_sdlkbd_TryPopQueueEvent(SDL_Event & event);

class aui_SDLKeyboard : public aui_Keyboard, public aui_SDLInput {
public:
	aui_SDLKeyboard(AUI_ERRCODE *retval);
	virtual ~aui_SDLKeyboard() {}

protected:
	aui_SDLKeyboard() {}
	AUI_ERRCODE createSDLKeyboard();
	void convertSDLKeyboardEvent(SDL_KeyboardEvent &sdlevent,
	                             aui_KeyboardEvent &auievent);
	uint32 convertSDLKey(SDL_Keysym keysym);

public:
	virtual AUI_ERRCODE Acquire();
	virtual AUI_ERRCODE Unacquire();
	virtual AUI_ERRCODE GetInput();
};

typedef aui_SDLKeyboard aui_NativeKeyboard;

#endif // defined(__AUI_USE_SDL__)

#endif
