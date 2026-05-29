//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : UI-side adapter forwarding text_observer::DrawText to
//                primitives_DrawText.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/core/text_observer.h"
#include "ui/aui_utils/primitives.h"

namespace {

void UIDrawText(aui_Surface *surf, sint32 x, sint32 y,
                const char *str, uint32 color, bool bg)
{
    // primitives_DrawText is itself __AUI_USE_DIRECTX__-gated, so on Mac /
    // SDL builds this remains a no-op until proper cross-platform text
    // rendering is implemented in the primitives layer.
    primitives_DrawText(surf, x, y, str, static_cast<COLORREF>(color), bg);
}

} // namespace

void RegisterTextObserverAdapter()
{
    text_observer::Register(&UIDrawText);
}

void UnregisterTextObserverAdapter()
{
    text_observer::Register(nullptr);
}
