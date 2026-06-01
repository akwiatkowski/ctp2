#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef GAMEINIT_H
#define GAMEINIT_H

struct HotseatPlayerSetup;

#include "robot/aibackdoor/civarchive.h"     // CivArchive
#include "os/include/ctp2_inttypes.h"  // sint32
#include "gs/utility/gstypes.h"        // k_MAX_PLAYERS
#include "gs/world/MapPoint.h"       // MapPoint

sint32 gameinit_InitializeGameFiles(void);
sint32 gameinit_Initialize(sint32 mWidth, sint32 mHeight, CivArchive *archive);
void   gameinit_CleanupMessages(void);
void   gameinit_Cleanup(void);
sint32 gameinit_ResetForNetwork();
sint32 spriteEditor_Initialize(sint32 mWidth, sint32 mHeight);

void gameinit_SpewUnits(sint32 player, MapPoint &pos);

// g_startEmailGame / g_startHotseatGame demoted to file-scope `static`
// in gameinit.cpp.  Flags driven by the new-game / hotseat-list UI and
// consulted by the simulation core during turn setup.
BOOL gameinit_IsEmailGame(void);
void gameinit_SetEmailGame(BOOL v);
BOOL gameinit_IsHotseatGame(void);
void gameinit_SetHotseatGame(BOOL v);

struct HotseatPlayerSetup {
	sint32 civ;
	sint32 isHuman;
	MBCHAR *name;
	MBCHAR *email;
};
extern HotseatPlayerSetup g_hsPlayerSetup[k_MAX_PLAYERS];

#endif
