//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Good sprite handling
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

#if defined(HAVE_PRAGMA_ONCE)
#pragma once
#endif

#ifndef GOODSPRITEGROUP_H__
#define GOODSPRITEGROUP_H__

//----------------------------------------------------------------------------
// Library dependencies
//----------------------------------------------------------------------------

#include <windows.h>          // POINT

//----------------------------------------------------------------------------
// Export overview
//----------------------------------------------------------------------------

class GoodSpriteGroup;

enum GOODACTION {
	GOODACTION_NONE = -1,

	GOODACTION_IDLE,

	GOODACTION_MAX
};

#define k_NOT_GOOD		-1

//----------------------------------------------------------------------------
// Project dependencies
//----------------------------------------------------------------------------

#include <memory>                        // std::unique_ptr

#include "os/include/ctp2_inttypes.h"    // sint32, uint16
#include "gfx/gfx_utils/pixeltypes.h"		// Pixel16
#include "gfx/spritesys/SpriteGroup.h"	// SpriteGroup, GROUPTYPE

class aui_Surface;
class ModernSpriteAtlas;

//----------------------------------------------------------------------------
// Class declarations
//----------------------------------------------------------------------------

class GoodSpriteGroup : public SpriteGroup
{
public:
	// Constructor and destructor are out-of-line so the
	// unique_ptr<ModernSpriteAtlas> member is created/destroyed where the
	// (forward-declared) type is complete.
	GoodSpriteGroup(GROUPTYPE type);
	~GoodSpriteGroup();

	void			DeallocateStorage() override;
	void			DeallocateFullLoadAnims() override;

	void			LoadBasic(MBCHAR const * filename) override;
	void			LoadFull(MBCHAR const * filename) override;

	void			Save(MBCHAR const * filename, unsigned int version_id, unsigned int compression_mode) override;

	void			ExportScript(MBCHAR const * name);

	void			Draw(GOODACTION action, sint32 frame, sint32 drawX, sint32 drawY,
						   sint32 facing, double scale, uint16 transparency, Pixel16 outlineColor, uint16 flags);
	void			DrawDirect(aui_Surface *surf, GOODACTION action, sint32 frame, sint32 drawX, sint32 drawY,
						   sint32 facing, double scale, uint16 transparency, Pixel16 outlineColor, uint16 flags);
	bool			AddGpuSpriteQuad(GOODACTION action, sint32 frame, sint32 drawX, sint32 drawY,
						   sint32 facing, double scale, Pixel16 outlineColor, uint16 flags);
	// Why the last AddGpuSpriteQuad declined, for the frame-incomplete report.
	static char const * GpuFallbackReason() { return s_gpuFallbackReason; }
	static char const * s_gpuFallbackReason;

	void			DrawText(sint32 x, sint32 y, MBCHAR const * s) override;

	POINT			GetHotPoint(GOODACTION action);


	sint32			Parse(uint16 id,GROUPTYPE group) override;

private:
	// Modern-first atlas for this good sprite (null unless CTP2_MODERN_SPRITES
	// is set and a generated manifest exists). Goods are non-faced.
	std::unique_ptr<ModernSpriteAtlas> m_modernAtlas;
};

#endif
