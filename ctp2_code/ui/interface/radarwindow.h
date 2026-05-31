#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef ___BMH_RADAR_WINDOW_HEADER
#define ___BMH_RADAR_WINDOW_HEADER

class ctp2_Window;

sint32	radarwindow_Initialize();

void	radarwindow_Display();

sint32	radarwindow_Cleanup();

void    radarwindow_Hide();
void    radarwindow_Show();
void    radarwindow_Toggle();

// g_radarWindow demoted to file-scope `static` in radarwindow.cpp.
// External callers go through radarwindow_Get() (returns NULL when the
// radar window has not been initialized).
ctp2_Window * radarwindow_Get(void);

#endif
