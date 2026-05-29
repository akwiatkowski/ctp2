//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Engine-side bridge for raw text-on-surface drawing
//
//----------------------------------------------------------------------------
//
// DataCheck::draw_crc / draw_time used to call ui/aui_utils/primitives.h's
// primitives_DrawText() to overlay CRC checksums and timing values onto a
// debug surface.  That include was severed when DataCheck.cpp was migrated
// off ui/ (commit cbc995ea); the calls became // TODO(orchestrator) lines.
//
// This namespace mirrors the primitives_DrawText surface as a free function
// behind a Register/Get pointer.  The UI build registers an adapter that
// forwards to primitives_DrawText (which is __AUI_USE_DIRECTX__-gated and
// is currently a no-op on Mac builds — restoring proper SDL_ttf text
// rendering is a separate piece of work).  Headless leaves the adapter
// unregistered and the call short-circuits.
//
// Migration pattern at the call site:
//   before:  primitives_DrawText(surf, x, y, (MBCHAR*)str, 0, 0);
//   after:   text_observer::DrawText(surf, x, y, str, 0, false);
//
//----------------------------------------------------------------------------

#pragma once

#include "ctp2_inttypes.h"

// Forward decl only — aui_Surface lives in ui/aui_common/aui_surface.h,
// which gs/ must not include.  Call sites pass the pointer through opaquely.
class aui_Surface;

namespace text_observer {

// Mirrors ui/aui_utils/primitives.h::primitives_DrawText.
//   color: a COLORREF (uint32 in the os/nowin32 typedef).
//   bg:    if true the call sets transparent background mode before drawing.
using DrawTextFn = void (*)(aui_Surface *surf, sint32 x, sint32 y,
                            const char *str, uint32 color, bool bg);

void Register(DrawTextFn fn);
DrawTextFn Get();   // NULL in headless / pre-registration

// Free function — null-checks Get() and forwards.  No-op when no adapter
// is registered.  Does NOT assert on a NULL surface (the caller's
// debug-overlay code may legitimately be invoked outside an active draw
// context, in which case skipping the draw is the right thing).
void DrawText(aui_Surface *surf, sint32 x, sint32 y,
              const char *str, uint32 color, bool bg);

} // namespace text_observer
