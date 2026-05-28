//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Public color type definitions (extracted from gfx/ headers)
//
// This header provides the COLOR enum without dragging in the rest of
// the gfx system.  Mirrors the audio_types.h pattern used by
// gs/core/audio_observer.h.
//
// The companion header `gfx/gfx_utils/colorset.h` now `#includes` this
// file so the UI side gets the same enum without a duplicate definition.
//
//----------------------------------------------------------------------------

#if defined(HAVE_PRAGMA_ONCE)
#pragma once
#endif

#ifndef COLOR_TYPES_H__
#define COLOR_TYPES_H__

enum COLOR {
	COLOR_BLACK,
	COLOR_WHITE,

	COLOR_RED,
	COLOR_GREEN,
	COLOR_BLUE,

	COLOR_YELLOW,
	COLOR_ORANGE,
	COLOR_PURPLE,
	COLOR_DARK_GREEN,

	COLOR_PLAYER1,
	COLOR_PLAYER2,
	COLOR_PLAYER3,
	COLOR_PLAYER4,
	COLOR_PLAYER5,
	COLOR_PLAYER6,
	COLOR_PLAYER7,
	COLOR_PLAYER8,
	COLOR_PLAYER9,
	COLOR_PLAYER10,
	COLOR_PLAYER11,
	COLOR_PLAYER12,
	COLOR_PLAYER13,
	COLOR_PLAYER14,
	COLOR_PLAYER15,
	COLOR_PLAYER16,
	COLOR_PLAYER17,

	// Added by Martin G�hmann to support more players with colors
	COLOR_PLAYER18,
	COLOR_PLAYER19,
	COLOR_PLAYER20,
	COLOR_PLAYER21,
	COLOR_PLAYER22,
	COLOR_PLAYER23,
	COLOR_PLAYER24,
	COLOR_PLAYER25,
	COLOR_PLAYER26,
	COLOR_PLAYER27,
	COLOR_PLAYER28,
	COLOR_PLAYER29,
	COLOR_PLAYER30,
	COLOR_PLAYER31,
	COLOR_PLAYER32,
	COLOR_PLAYER33,

	COLOR_TERRAIN_0,
	COLOR_TERRAIN_1,
	COLOR_TERRAIN_2,
	COLOR_TERRAIN_3,
	COLOR_TERRAIN_4,
	COLOR_TERRAIN_5,
	COLOR_TERRAIN_6,
	COLOR_TERRAIN_7,
	COLOR_TERRAIN_8,
	COLOR_TERRAIN_9,
	COLOR_TERRAIN_10,
	COLOR_TERRAIN_11,
	COLOR_TERRAIN_12,
	COLOR_TERRAIN_13,
	COLOR_TERRAIN_14,
	COLOR_TERRAIN_15,
	COLOR_TERRAIN_16,
	COLOR_TERRAIN_17,
	COLOR_TERRAIN_18,
    COLOR_TERRAIN_19,
    COLOR_TERRAIN_20,
    COLOR_TERRAIN_21,
    COLOR_TERRAIN_22,

	COLOR_SELECT_0,
	COLOR_SELECT_1,
	COLOR_SELECT_2,
	COLOR_SELECT_3,

	COLOR_BUTTON_TEXT_DROP,
	COLOR_BUTTON_TEXT_PLAIN,
	COLOR_BUTTON_TEXT_HILITE,

	COLOR_UI_BOX,
	COLOR_GRAY,

	COLOR_MAX
};

#endif
