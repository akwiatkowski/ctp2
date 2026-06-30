//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : General declarations
//
//----------------------------------------------------------------------------
//
// Disclaimer
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
// This material has been developed at apolyton.net by the Apolyton CtP2
// Source Code Project. Contact the authors at ctp2source@apolyton.net.
//
//----------------------------------------------------------------------------
//
// Compiler flags
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - #pragmas commented out
// - includes fixed for case sensitive filesystems.
// - Merged DoFinalCleanup versions, removed some unused exports.
//
//----------------------------------------------------------------------------

#if defined(HAVE_PRAGMA_ONCE)
#pragma once
#endif

#ifndef __CIV3_MAIN_H__
#define __CIV3_MAIN_H__

#define k_TICKS_PER_GENERIC_FRAME	100
#define k_TICKS_PER_SCROLL_FRAME	50

class aui_Surface;

int ui_Initialize();
int ui_Process();
bool ui_CheckForScroll();

sint32 sharedsurface_Initialize( );
void   sharedsurface_Cleanup( );

int sprite_Initialize();
int sprite_Update(aui_Surface *surf);
void sprite_Cleanup();

int tile_Initialize(BOOL isRestoring);
void tile_Cleanup();

int WINAPI main_filehelper_GetOS();

int main_Restart();
int main_RestoreGame(const MBCHAR *filename);

void main_HideTaskBar();
void main_RestoreTaskBar();

int radar_Initialize();

void DoFinalCleanup(int exitCode = -1);

BOOL ExitGame();

#endif
