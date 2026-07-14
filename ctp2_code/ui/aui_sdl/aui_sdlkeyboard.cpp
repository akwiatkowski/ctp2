#include "os/include/ctp2_config.h"
#include "ctp/c3.h"

#if defined(__AUI_USE_SDL__)

#include "ui/aui_sdl/aui_sdlcompat.h"
#include "ui/aui_sdl/aui_sdlkeyboard.h"
#include "ui/aui_common/aui_ui.h"
#include "ui/aui_ctp2/c3ui.h"


#include "ctp/civapp.h"

// We have to have a secondary keyboard event queue from which
// we extract events here.  This is because keyboard events are
// handled in two places.  This queue is filled by the main
// game loop in CivMain in civ3_main.cpp
static std::queue<SDL_Event> g_secondaryKeyboardEventQueue;
// Then we need a mutex to allow us to thread-safely access the queue
// (Actually I don't think we do, now I understand the main game loop
// better, but I'll leave it in here to be on the safe side).
// Lifecycle managed via aui_sdlkbd_InitQueueMutex / DestroyQueueMutex
// from civ3_main.cpp.
static SDL_mutex* g_secondaryKeyboardEventQueueMutex = nullptr;

void aui_sdlkbd_InitQueueMutex()
{
	g_secondaryKeyboardEventQueueMutex = SDL_CreateMutex();
}

void aui_sdlkbd_DestroyQueueMutex()
{
	SDL_DestroyMutex(g_secondaryKeyboardEventQueueMutex);
	g_secondaryKeyboardEventQueueMutex = nullptr;
}

void aui_sdlkbd_PushQueueEvent(SDL_Event const & event)
{
	if (!g_secondaryKeyboardEventQueueMutex) return;
	if (-1 == SDL_LockMutex(g_secondaryKeyboardEventQueueMutex)) {
		fprintf(stderr, "[aui_sdlkbd_PushQueueEvent] SDL_LockMutex failed: %s\n",
		        SDL_GetError());
		return;
	}
	g_secondaryKeyboardEventQueue.push(event);
	SDL_UnlockMutex(g_secondaryKeyboardEventQueueMutex);
}

bool aui_sdlkbd_TryPopQueueEvent(SDL_Event & event)
{
	if (!g_secondaryKeyboardEventQueueMutex) return false;
	if (-1 == SDL_LockMutex(g_secondaryKeyboardEventQueueMutex)) {
		fprintf(stderr, "[aui_sdlkbd_TryPopQueueEvent] SDL_LockMutex failed: %s\n",
		        SDL_GetError());
		return false;
	}
	bool gotEvent = false;
	if (!g_secondaryKeyboardEventQueue.empty()) {
		event = g_secondaryKeyboardEventQueue.front();
		g_secondaryKeyboardEventQueue.pop();
		gotEvent = true;
	}
	SDL_UnlockMutex(g_secondaryKeyboardEventQueueMutex);
	return gotEvent;
}

aui_SDLKeyboard::aui_SDLKeyboard(
	AUI_ERRCODE *retval )
	:
	aui_Input(),
	aui_Keyboard(),
	aui_SDLInput( retval, FALSE )
{
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = createSDLKeyboard();
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}

AUI_ERRCODE aui_SDLKeyboard::createSDLKeyboard( )
{
	// TODO: SDL_Init()
	return AUI_ERRCODE_OK;
}

AUI_ERRCODE aui_SDLKeyboard::GetInput( )
{
	SDL_Event event;
	if (!aui_sdlkbd_TryPopQueueEvent(event)) {
		return AUI_ERRCODE_NOINPUT;
	}

	m_data.time = SDL_GetTicks();
	switch (event.type) {
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			switch (event.key.keysym.sym) {
				case SDLK_LSHIFT:
					if (c3ui_Get()->TheMouse()) {
						if (event.key.state & SDL_PRESSED) {
							c3ui_Get()->TheMouse()->SetFlags(c3ui_Get()->TheMouse()->GetFlags() | k_MOUSE_EVENT_FLAG_LSHIFT);
						} else {
							c3ui_Get()->TheMouse()->SetFlags(c3ui_Get()->TheMouse()->GetFlags() & ~k_MOUSE_EVENT_FLAG_LSHIFT);
						}
					}
					return AUI_ERRCODE_OK;
				case SDLK_RSHIFT:
					if (c3ui_Get()->TheMouse()) {
						if (event.key.state & SDL_PRESSED) {
							c3ui_Get()->TheMouse()->SetFlags(c3ui_Get()->TheMouse()->GetFlags() | k_MOUSE_EVENT_FLAG_RSHIFT);
						} else {
							c3ui_Get()->TheMouse()->SetFlags(c3ui_Get()->TheMouse()->GetFlags() & ~k_MOUSE_EVENT_FLAG_RSHIFT);
						}
					}
					return AUI_ERRCODE_OK;
				case SDLK_LCTRL:
					if (c3ui_Get()->TheMouse()) {
						if (event.key.state & SDL_PRESSED) {
							c3ui_Get()->TheMouse()->SetFlags(c3ui_Get()->TheMouse()->GetFlags() | k_MOUSE_EVENT_FLAG_LCONTROL);
						} else {
							c3ui_Get()->TheMouse()->SetFlags(c3ui_Get()->TheMouse()->GetFlags() & ~k_MOUSE_EVENT_FLAG_LCONTROL);
						}
					}
					return AUI_ERRCODE_OK;
				case SDLK_RCTRL:
					if (c3ui_Get()->TheMouse()) {
						if (event.key.state & SDL_PRESSED) {
							c3ui_Get()->TheMouse()->SetFlags(c3ui_Get()->TheMouse()->GetFlags() | k_MOUSE_EVENT_FLAG_RCONTROL);
						} else {
							c3ui_Get()->TheMouse()->SetFlags(c3ui_Get()->TheMouse()->GetFlags() & ~k_MOUSE_EVENT_FLAG_RCONTROL);
						}
					}
					return AUI_ERRCODE_OK;
				case SDLK_RETURN:
#ifdef _DEBUG
					extern BOOL commandMode;
					if (commandMode)
						return AUI_ERRCODE_OK;
#endif
					break;
				case SDLK_UP:
				case SDLK_DOWN:
				case SDLK_LEFT:
				case SDLK_RIGHT:
					if (event.key.state & SDL_PRESSED) {
						civapp_Get()->BeginKeyboardScrolling(convertSDLKey(event.key.keysym));
					} else {
						civapp_Get()->StopKeyboardScrolling(convertSDLKey(event.key.keysym));
					}
					break;
			}
			convertSDLKeyboardEvent(event.key, m_data);
			break;
		default:
			Assert(true);
	}

	return AUI_ERRCODE_OK;
}

void aui_SDLKeyboard::convertSDLKeyboardEvent(SDL_KeyboardEvent &sdlevent,
                                      aui_KeyboardEvent &auievent)
{
	auievent.down = (sdlevent.state & SDL_PRESSED) ? TRUE : FALSE;
	auievent.key = convertSDLKey(sdlevent.keysym);
}

uint32 aui_SDLKeyboard::convertSDLKey(SDL_Keysym keysym)
{
	switch (keysym.sym) {
		case SDLK_ESCAPE:
			return AUI_KEYBOARD_KEY_ESCAPE;
		case SDLK_RETURN:
			return AUI_KEYBOARD_KEY_RETURN;
		case SDLK_SPACE:
			return AUI_KEYBOARD_KEY_SPACE;
		case SDLK_TAB:
			return AUI_KEYBOARD_KEY_TAB;
		case SDLK_UP:
			return AUI_KEYBOARD_KEY_UPARROW;
		case SDLK_DOWN:
			return AUI_KEYBOARD_KEY_DOWNARROW;
		case SDLK_LEFT:
			return AUI_KEYBOARD_KEY_LEFTARROW;
		case SDLK_RIGHT:
			return AUI_KEYBOARD_KEY_RIGHTARROW;
	}
	// SDL2 removed keysym.unicode; use sym for ASCII range
	if (keysym.sym >= 0 && keysym.sym < 128) {
		return keysym.sym;
	}
	return AUI_KEYBOARD_KEY_INVALID;
}

AUI_ERRCODE aui_SDLKeyboard::Acquire()
{
	return AUI_ERRCODE_OK;
}

AUI_ERRCODE aui_SDLKeyboard::Unacquire()
{
	return AUI_ERRCODE_OK;
}

#endif // defined(__AUI_USE_SDL__)
