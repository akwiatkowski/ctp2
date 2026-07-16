#if defined(HAVE_PRAGMA_ONCE)
#pragma once
#endif

#ifndef UNITSPRITEGROUP_H__
#define UNITSPRITEGROUP_H__

//----------------------------------------------------------------------------
// Library dependencies
//----------------------------------------------------------------------------

#include <windows.h>          // BOOL, POINT
#include <memory>             // std::unique_ptr

//----------------------------------------------------------------------------
// Export overview
//----------------------------------------------------------------------------

class UnitSpriteGroup;

enum UNITACTION
{
	UNITACTION_FACE_OFF = -4,
	UNITACTION_MORPH = -3,
	UNITACTION_FAKE_DEATH = -2,
	UNITACTION_NONE = -1,

	UNITACTION_MOVE,
	UNITACTION_ATTACK,
	UNITACTION_IDLE,
	UNITACTION_VICTORY,
	UNITACTION_WORK,

	UNITACTION_MAX
};

//----------------------------------------------------------------------------
// Project dependencies
//----------------------------------------------------------------------------

#include "gfx/spritesys/Action.h"         // GAME_ACTION
#include "os/include/ctp2_inttypes.h"  // sint32, uint16
#include "gfx/gfx_utils/pixeltypes.h"		// Pixel16
#include "gfx/spritesys/SpriteGroup.h"	// SpriteGroup, GROUPTYPE

class aui_Surface;

//----------------------------------------------------------------------------
// Class declarations
//----------------------------------------------------------------------------

class ModernSpriteAtlas;

class UnitSpriteGroup : public SpriteGroup
{
public:
	UnitSpriteGroup(GROUPTYPE type);
	~UnitSpriteGroup() override;

	void			DeallocateStorage() override;
	void			DeallocateFullLoadAnims() override;

	void			LoadBasic(MBCHAR const * filename) override;
	void			LoadIndexed(MBCHAR const * filename, GAME_ACTION index) override;
	void			LoadFull(MBCHAR const * filename) override;

	// nameSize = capacity of `name` in chars (the write is bounded by it).
	bool			GetImageFileName(MBCHAR * name, size_t nameSize, char * format, ...);

	void			Save(MBCHAR const * filename, unsigned int version_id, unsigned int compression_mode) override;

	void			Draw(UNITACTION action, sint32 frame, sint32 drawX, sint32 drawY,
						   sint32 facing, double scale, uint16 transparency, Pixel16 outlineColor, uint16 flags, BOOL specialDelayProcess, BOOL directionalAttack);

	void			DrawText(sint32 x, sint32 y, MBCHAR const * s) override;
	void			DrawDirect(aui_Surface *surf, UNITACTION action, sint32 frame, sint32 drawX, sint32 drawY,
							   sint32 facing, double scale, uint16 transparency, Pixel16 outlineColor, uint16 flags,
							   BOOL specialDelayProcess,
							   BOOL directionalAttack);

	POINT *         GetShieldPoints(UNITACTION action) { return m_shieldPoints[action]; }











	uint16			GetNumFirePointsWork() { return m_numFirePointsWork; }
	void			SetNumFirePointsWork(uint16 num) { m_numFirePointsWork = num; }

	sint32			Parse(uint16 id,GROUPTYPE type) override;
	void			ExportScript(MBCHAR const * name);

	POINT			GetHotPoint(UNITACTION action, sint32 facing);
	void			SetHotPoint(UNITACTION action, sint32 facing,POINT pt);

	BOOL			HitTest(POINT mousePt, UNITACTION action, sint32 frame, sint32 drawX, sint32 drawY,
						   sint32 facing, double scale, uint16 transparency, Pixel16 outlineColor, uint16 flags, BOOL specialDelayProcess, BOOL directionalAttack);

private:


	uint16			m_numFirePointsWork;
	POINT			m_firePointsWork[k_NUM_FIREPOINTS][k_NUM_FACINGS];

	POINT			m_moveOffsets[k_NUM_FACINGS];

	POINT			m_shieldPoints[UNITACTION_MAX][k_NUM_FACINGS];

	// Modern-first atlas (P11 B1), populated on load when CTP2_MODERN_SPRITES
	// is set and a generated manifest exists; else null and the legacy RLE
	// sprites are drawn. Owned; freed in the out-of-line destructor.
	std::unique_ptr<ModernSpriteAtlas> m_modernAtlas;
};

#endif
