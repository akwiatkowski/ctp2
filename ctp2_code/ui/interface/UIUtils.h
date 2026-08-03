#include "ui/aui_ctp2/c3window.h"

#define TestControl(p) { Assert(AUI_NEWOK(p, errcode)) ; if (!AUI_NEWOK(p, errcode)) return (FALSE) ; }
// Deletes the control and clears the pointer. Named DeleteControl because that
// is what it does -- it was RemoveControl, which reads like a detach from a
// parent and hid a delete behind it. Members that own their control should use
// std::unique_ptr and reset() instead of this.
//
// Do NOT reuse this name for a method. aui_Window::RemoveControl(uint32) and
// AttractWindow::RemoveControl(MBCHAR *) detach a child without freeing it, and
// they keep that name deliberately. A function-like macro expands wherever the
// name is followed by "(", so a method sharing it breaks in any translation
// unit that includes this header first.
#define DeleteControl(p) { if (p) delete p ; p = NULL ; }

extern void BlockPush(MBCHAR *path, MBCHAR *addition) ;
extern void BlockPop(MBCHAR *path) ;

void ui_TruncateString( aui_Control *control, MBCHAR *str );

MBCHAR *uiutils_ChooseLdl(MBCHAR *firstChoice, MBCHAR *fallback);




MBCHAR *uiutils_AppendBlock(MBCHAR *destString, MBCHAR *srcString1, MBCHAR *srcString2);
