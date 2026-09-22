#include "os/include/ctp2_config.h"
#include "ctp/c3.h"

#ifdef __AUI_USE_SDL__

#include "ui/aui_sdl/aui_sdlmouse.h"

aui_SDLMouse::aui_SDLMouse(
   AUI_ERRCODE *retval,
   MBCHAR *ldlBlock,
   BOOL useExclusiveMode)
   :
   aui_Input(),
   aui_Mouse(retval, ldlBlock),
   aui_SDLInput(retval, useExclusiveMode)
{
   Assert(AUI_SUCCESS(*retval));
   if (!AUI_SUCCESS(*retval)) return;
}

aui_SDLMouse::~aui_SDLMouse()
= default;

AUI_ERRCODE
aui_SDLMouse::GetInput()
{
   m_data.time = SDL_GetTicks();
   if (m_syntheticInput)
      return AUI_ERRCODE_NOINPUT;
   bool haveMoves = false;

   for ( sint32 numInputs = 200; numInputs; numInputs-- ) {
      SDL_Event od;
      // check for one of the mouse events
      // SDL2: SDL_PeepEvents uses minType/maxType instead of event masks
      // NB: upper bound is SDL_MOUSEBUTTONUP, NOT SDL_MOUSEWHEEL — the mouse
      // path must NOT consume wheel events (it has no wheel handler and would
      // drop them); wheel is left for the main loop, which drives the P11 F
      // smooth-camera zoom (civ3_main.cpp SDLMessageHandler).
      int numElements =
         SDL_PeepEvents(&od, 1, SDL_GETEVENT,
          		SDL_MOUSEMOTION, SDL_MOUSEBUTTONUP);
      if (0 > numElements) {
         fprintf(stderr, "Mouse PeepEvents failed: %s\n", SDL_GetError());
         return AUI_ERRCODE_GETDEVICEDATAFAILED;
      }
      if (0 == numElements) {
         if(haveMoves)
            return AUI_ERRCODE_OK;
         else
            return AUI_ERRCODE_NOINPUT;
      }
      switch (od.type) {
      case SDL_MOUSEMOTION:
         {
            // Map window coords -> game logical coords so input stays correct
            // when the window is resized or HiDPI-scaled (renderer logical size
            // = game res). No-op (returns raw coords) when window is 1:1.
            SDL_Window *win = SDL_GetWindowFromID(od.motion.windowID);
            SDL_Renderer *ren = win ? SDL_GetRenderer(win) : nullptr;
            if (ren) {
               float lx = 0.0f, ly = 0.0f;
               CTP2_SDL_RenderWindowToLogical(ren, od.motion.x, od.motion.y, &lx, &ly);
               m_data.position.x = (sint32)lx;
               m_data.position.y = (sint32)ly;
            } else {
               m_data.position.x = od.motion.x;
               m_data.position.y = od.motion.y;
            }
         }
         m_data.lbutton = !!(od.motion.state & SDL_BUTTON_LMASK);
         m_data.rbutton = !!(od.motion.state & SDL_BUTTON_RMASK);
         static int motionLogCount = 0;
         if (++motionLogCount <= 20) {
            fprintf(stderr, "[MOUSE-IN] SDL motion: (%d, %d) state=%d\n",
                    static_cast<int>(od.motion.x),
                    static_cast<int>(od.motion.y),
                    od.motion.state);
         }
         break;
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
         {
            SDL_Window *win = SDL_GetWindowFromID(od.button.windowID);
            SDL_Renderer *ren = win ? SDL_GetRenderer(win) : nullptr;
            if (ren) {
               float lx = 0.0f, ly = 0.0f;
               CTP2_SDL_RenderWindowToLogical(ren, od.button.x, od.button.y, &lx, &ly);
               m_data.position.x = (sint32)lx;
               m_data.position.y = (sint32)ly;
            } else {
               m_data.position.x = od.button.x;
               m_data.position.y = od.button.y;
            }
         }
         if (od.button.button == SDL_BUTTON_LEFT) {
            m_data.lbutton = CTP2_SDL_IsMouseButtonDown(od);
         } else if (od.button.button == SDL_BUTTON_RIGHT) {
            m_data.rbutton = CTP2_SDL_IsMouseButtonDown(od);
         }
         break;
      default:
         printf("event not handeled: %d\n", od.type);
         continue;
      }
      return AUI_ERRCODE_OK;
   }
   return AUI_ERRCODE_OK;
}

#endif
