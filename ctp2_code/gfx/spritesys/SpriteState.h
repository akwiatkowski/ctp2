#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __SPRITE_STATE_H__
#define __SPRITE_STATE_H__

// Forward decl + SpriteStatePtr typedef live in gs/core/sprite_state_fwd.h
// so gs/ headers can use the typedef without depending on gfx/.  This
// header re-uses that declaration to keep a single canonical definition.
#include "gs/core/sprite_state_fwd.h"

class CivArchive ;

class SpriteState {

protected:

	sint32 m_index;










public:

	SpriteState(sint32 index) { m_index = index; };
	SpriteState(CivArchive &archive) ;

	sint32		GetIndex(void) { return m_index; };
	void		SetIndex(sint32 index) { m_index = index; };

	void Serialize(CivArchive &archive) ;

};

// SpriteStatePtr typedef is now defined in gs/core/sprite_state_fwd.h
// (included above).  This re-declaration was here historically but is
// now redundant.

#endif
