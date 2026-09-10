#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __BACKGROUNDWIN_H__
#define __BACKGROUNDWIN_H__

class Background;

sint32      backgroundWin_Initialize(bool fullscreen = false);
void        backgroundWin_Cleanup();
AUI_ERRCODE background_render_map_only(Background *bg);
AUI_ERRCODE background_draw_handler(LPVOID bg);

Background * background_Get();
void         background_Set(Background *p);

#endif
