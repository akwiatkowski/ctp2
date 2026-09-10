//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Common sprite handling
// Id           : $Id$
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
// - None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Fixed memory leaks.
//
//----------------------------------------------------------------------------

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef SPRITEGROUP_H__
#define SPRITEGROUP_H__

//----------------------------------------------------------------------------
// Library dependencies
//----------------------------------------------------------------------------

#include <windows.h>          // BOOL, FILE

//----------------------------------------------------------------------------
// Export overview
//----------------------------------------------------------------------------

#define k_NUM_FIREPOINTS		8

class SpriteGroup;

enum GROUPTYPE
{
	GROUPTYPE_GROUP,

	GROUPTYPE_UNIT,
	GROUPTYPE_PROJECTILE,
	GROUPTYPE_EFFECT,
	GROUPTYPE_CITY,
	GROUPTYPE_GOOD,

	GROUPTYPE_MAX
};

enum LOADTYPE {
	LOADTYPE_NONE = -1,

	LOADTYPE_BASIC,
	LOADTYPE_FULL,
	LOADTYPE_INDEXED,

	LOADTYPE_MAX
};

//----------------------------------------------------------------------------
// Project dependencies
//----------------------------------------------------------------------------

#include <memory>

#include "gfx/spritesys/Action.h"
#include "os/include/ctp2_inttypes.h"	// sint32, uint16
#include "gfx/spritesys/Anim.h"
#include "gfx/spritesys/FacedSprite.h"
#include "gs/fileio/Token.h"

class Anim;
class aui_Surface;
class Sprite;

//----------------------------------------------------------------------------
// Class declarations
//----------------------------------------------------------------------------

class SpriteGroup
{
public:
	SpriteGroup(GROUPTYPE type);
	virtual ~SpriteGroup();

	virtual void	LoadBasic   (MBCHAR const * filename) {};
	virtual void	LoadFull    (MBCHAR const * filename) {};
	virtual void	LoadIndexed (MBCHAR const * filename, GAME_ACTION action) {};

	virtual void	Save(MBCHAR const * filename, unsigned int version_id, unsigned int compression_mode){};
	virtual sint32	Parse(uint16 id,GROUPTYPE group){ return FALSE;};

	virtual void	DeallocateStorage();
	virtual void	DeallocateFullLoadAnims();

	virtual void	Draw(sint32 drawX, sint32 drawY, sint32 facing, double scale,
					  uint16 transparency, Pixel16 outlineColor, uint16 flags);
	virtual void	DrawText(sint32 x, sint32 y, MBCHAR const * s);

	virtual void	AddRef();
	virtual void	Release();

	virtual void	AddFullLoadRef();
	virtual void	ReleaseFullLoad();

	sint32			GetRefCount() const { return m_usageRefCount; }
	sint32			GetFullLoadRefCount() const { return m_fullLoadRefCount; }

	GROUPTYPE		GetType() const { return m_type; }

	LOADTYPE		GetLoadType() const { return m_loadType; }
	void			SetLoadType(LOADTYPE type) { m_loadType = type; }

	Sprite *        GetGroupSprite(GAME_ACTION action) const { return ((action >= 0) && (action < ACTION_MAX)) ? m_sprites[action].get() : nullptr; }
	void			SetGroupSprite (GAME_ACTION action, Sprite *sprite) { if ((action >= 0) && (action < ACTION_MAX)) AdoptSlot(m_sprites[action], sprite); }

	Anim *          GetGroupAnim(uint32 action) const { return (action < ACTION_MAX) ? m_anims[action].get() : nullptr; }
	void			SetGroupAnim (GAME_ACTION action, Anim *anim) { if ((action >= 0) && (action < ACTION_MAX)) AdoptSlot(m_anims[action], anim); }

	// Takes int (not GAME_ACTION) so callers passing UNITACTION_NONE (-1) or
	// any other out-of-range value don't trigger an enum-load UBSan hit at
	// entry.  Bounds-check then index is safe with an int.
	Anim *          GetAnim(int action) const { return ((action >= 0) && (action < ACTION_MAX)) ? m_anims[action].get() : nullptr; }

	sint32			GetWidth() const { return m_width; };
	sint32			GetHeight() const { return m_height; };

	size_t			GetNumFrames(GAME_ACTION action) const;
    virtual void   	ExportSpriteGroup(FILE *file,GAME_ACTION action,TOKEN_TYPES main_token,TOKEN_TYPES sub_token,BOOL sub_value=FALSE);

	bool			HasDirectional() const { return m_hasDirectional; }
	void			SetHasDirectional(bool val) { m_hasDirectional = val; }

	bool			HasDeath() const { return m_hasDeath; }
	void			SetHasDeath(bool val) { m_hasDeath = val; }




private:

	GROUPTYPE		m_type;

protected:
	sint32			m_width, m_height;

	sint32			m_usageRefCount;
	sint32			m_fullLoadRefCount;

	LOADTYPE		m_loadType;

	// The group owns one sprite and one animation per action.
	std::unique_ptr<Sprite>	m_sprites[ACTION_MAX];
	std::unique_ptr<Anim>	m_anims  [ACTION_MAX];

	// The sprite loader reads the current slot, hands it to a reader that fills
	// it IN PLACE (allocating only when the slot is empty), then writes it back.
	// So the incoming pointer is usually the one already held, and a bare
	// reset() would destroy the object it had just been handed. Guarding the
	// self-assignment keeps that path correct while still releasing a genuine
	// replacement — which the previous raw-pointer setter silently leaked.
	template <typename T>
	static void AdoptSlot(std::unique_ptr<T> & slot, T * incoming)
	{
		if (slot.get() != incoming)
			slot.reset(incoming);
	}

	bool			m_hasDeath;
	bool			m_hasDirectional;
};

#endif
