#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef EFFECTSPRITEGROUP_H__
#define EFFECTSPRITEGROUP_H__

//----------------------------------------------------------------------------
// Library dependencies
//----------------------------------------------------------------------------

#include <windows.h>    // POINT

//----------------------------------------------------------------------------
// Export overview
//----------------------------------------------------------------------------

class EffectSpriteGroup;

enum EFFECTACTION {
	EFFECTACTION_NONE = -1,

	EFFECTACTION_PLAY,
	EFFECTACTION_FLASH,

	EFFECTACTION_MAX
};

//----------------------------------------------------------------------------
// Project dependencies
//----------------------------------------------------------------------------

#include <memory>               // std::unique_ptr

#include "gfx/spritesys/SpriteGroup.h"

class aui_Surface;
class ModernSpriteAtlas;

//----------------------------------------------------------------------------
// Class declarations
//----------------------------------------------------------------------------

class EffectSpriteGroup : public SpriteGroup
{
public:
	// Constructor and destructor are out-of-line so the
	// unique_ptr<ModernSpriteAtlas> member is created/destroyed where the
	// (forward-declared) type is complete.
	EffectSpriteGroup(GROUPTYPE type);
	~EffectSpriteGroup();

	void			Load(MBCHAR const * filename);
	void			Save(MBCHAR const * filename,unsigned int version_id, unsigned int compression_mode) override;

	void			LoadBasic(MBCHAR const * filename) override { Load(filename); };
	void			LoadFull (MBCHAR const * filename) override { Load(filename); };

	void			ExportScript(MBCHAR const * name);

	void			Draw(EFFECTACTION action, sint32 frame, sint32 drawX, sint32 drawY, sint32 SdrawX, sint32 SdrawY, sint32 facing, double scale, uint16 transparency, Pixel16 outlineColor, uint16 flags, BOOL specialDelayProcess, BOOL directionalAttack = FALSE);
	void			DrawDirect(aui_Surface *surf, EFFECTACTION action, sint32 frame, sint32 drawX, sint32 drawY, sint32 SdrawX, sint32 SdrawY, sint32 facing, double scale, uint16 transparency, Pixel16 outlineColor, uint16 flags, BOOL specialDelayProcess, BOOL directionalAttack = FALSE);
	bool			AddGpuSpriteQuad(EFFECTACTION action, sint32 frame, sint32 drawX, sint32 drawY, sint32 SdrawX, sint32 SdrawY, sint32 facing, double scale, uint16 transparency, Pixel16 outlineColor, uint16 flags, BOOL specialDelayProcess, BOOL directionalAttack = FALSE);

	sint32			Parse(uint16 id,GROUPTYPE group) override;
	POINT			GetHotPoint(EFFECTACTION action, sint32 facing);

private:
	// Modern-first atlas for the effect's PLAY frames (null when
	// CTP2_MODERN_SPRITES=0 or no generated manifest exists). The additive FLASH
	// overlay stays on the legacy path (the atlas cannot blend additively).
	std::unique_ptr<ModernSpriteAtlas> m_modernAtlas;
};

#endif
