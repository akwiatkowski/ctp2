//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Engine-side bridge for raw text-on-surface drawing
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/core/text_observer.h"

namespace text_observer {

namespace {
DrawTextFn s_drawText = nullptr;
}

void Register(DrawTextFn fn)
{
    s_drawText = fn;
}

DrawTextFn Get()
{
    return s_drawText;
}

void DrawText(aui_Surface *surf, sint32 x, sint32 y,
              const char *str, uint32 color, bool bg)
{
    if (s_drawText) {
        s_drawText(surf, x, y, str, color, bg);
    }
}

} // namespace text_observer
