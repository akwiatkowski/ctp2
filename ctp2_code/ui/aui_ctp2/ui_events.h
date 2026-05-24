//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Registration of UI-side game event hooks
//
//----------------------------------------------------------------------------
//
// UI windows (DiplomacyWindow, ArmyManagerWindow, ...) install hooks into the
// game-event system to refresh themselves when the underlying simulation
// changes.  These registrations used to live in gs/gameobj/Events.cpp, which
// pulled the game-state core into a dependency on every UI header.  They
// belong in the UI subsystem instead — only the UI-enabled build calls them.
//
//----------------------------------------------------------------------------

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __UI_EVENTS_H__
#define __UI_EVENTS_H__

void ui_events_Initialize();
void ui_events_Cleanup();

#endif
