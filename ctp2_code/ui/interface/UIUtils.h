#include "ui/aui_ctp2/c3window.h"

#define TestControl(p) { Assert(AUI_NEWOK(p, errcode)) ; if (!AUI_NEWOK(p, errcode)) return (FALSE) ; }
// Deletes the control and clears the pointer. Named DeleteControl because that
// is what it does -- it was RemoveControl, which reads like a detach from a
// parent and hid a delete behind it. Members that own their control should use
// std::unique_ptr and reset() instead of this.
#define DeleteControl(p) { if (p) delete p ; p = NULL ; }

extern void BlockPush(MBCHAR *path, MBCHAR *addition) ;
extern void BlockPop(MBCHAR *path) ;

void ui_TruncateString( aui_Control *control, MBCHAR *str );

MBCHAR *uiutils_ChooseLdl(MBCHAR *firstChoice, MBCHAR *fallback);




MBCHAR *uiutils_AppendBlock(MBCHAR *destString, MBCHAR *srcString1, MBCHAR *srcString2);
