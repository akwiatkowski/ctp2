#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef GAMEINIT_H
#define GAMEINIT_H

struct HotseatPlayerSetup;

#include "os/include/ctp2_inttypes.h"  // sint32
#include "gs/utility/gstypes.h"        // k_MAX_PLAYERS
#include "gs/world/MapPoint.h"       // MapPoint

sint32 gameinit_InitializeGameFiles();
sint32 gameinit_Initialize(sint32 mWidth, sint32 mHeight);
void   gameinit_CleanupMessages();
void   gameinit_Cleanup();
sint32 gameinit_ResetForNetwork();
sint32 spriteEditor_Initialize(sint32 mWidth, sint32 mHeight);

void gameinit_SpewUnits(sint32 player, MapPoint &pos);

// g_startEmailGame / g_startHotseatGame demoted to file-scope `static`
// in gameinit.cpp.  Flags driven by the new-game / hotseat-list UI and
// consulted by the simulation core during turn setup.
BOOL gameinit_IsEmailGame();
void gameinit_SetEmailGame(BOOL v);
BOOL gameinit_IsHotseatGame();
void gameinit_SetHotseatGame(BOOL v);

struct HotseatPlayerSetup {
	sint32 civ;
	sint32 isHuman;
	MBCHAR *name;
	MBCHAR *email;
};
// Hotseat per-slot setup buffer.  Definition is file-scope `static`
// in gs/utility/gameinit.cpp; callers index via hs_player_setup_buf()
// and clear via hs_player_setup_Clear() (replaces the previous
// memset(g_hsPlayerSetup, 0, sizeof(g_hsPlayerSetup)) idiom).
HotseatPlayerSetup * hs_player_setup_buf();
void hs_player_setup_Clear();

#endif
