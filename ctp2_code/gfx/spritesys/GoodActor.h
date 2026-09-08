//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Handling of good animations on screen
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
// None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
//----------------------------------------------------------------------------
///
/// \file   GoodActor.h
/// \brief  Handling of good animations on screen

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef GOODACTOR_H__
#define GOODACTOR_H__

//----------------------------------------------------------------------------
//
// Library imports
//
//----------------------------------------------------------------------------

// none

//----------------------------------------------------------------------------
//
// Exported names
//
//----------------------------------------------------------------------------

class GoodActor;

//----------------------------------------------------------------------------
//
// Project imports
//
//----------------------------------------------------------------------------
#include <deque>
#include <memory>

#include "gfx/spritesys/Actor.h"              // Actor
#include "os/include/ctp2_inttypes.h"      // sint32, uint16
#include "gfx/spritesys/GoodSpriteGroup.h"    // GOODACTION
#include "gfx/spritesys/SpriteGroup.h"        // LOADTYPE
#include "gs/world/World.h"              // MapPoint

class Action;
class Anim;
class aui_Surface;

//----------------------------------------------------------------------------
//
// Declarations
//
//----------------------------------------------------------------------------

class GoodActor : public Actor
{
public:
    GoodActor(sint32 index, MapPoint const & pos);
    GoodActor(GoodActor const & rhs);

    virtual ~GoodActor();
    GoodActor & operator = (GoodActor const & rhs);

	void	Process() override;

	void			PositionActor(MapPoint &pos);
  void			AddAction(ActionPtr actionObj) override;
	void			GetNextAction();
	void			AddIdle();

	std::unique_ptr<Anim>	CreateAnim(GOODACTION action);

	void			DrawSelectionBrackets();
	bool			Draw(bool fogged = false);
	void			DrawDirect(aui_Surface *surf, sint32 x, sint32 y, double scale);
	bool			AddGpuSpriteQuad(sint32 x, sint32 y, double scale, bool fogged);

	void			DrawText(sint32 x, sint32 y, MBCHAR const * goodText);

	bool			IsAnimating() const;

	MapPoint		GetPos() const { return m_pos; }
	void			SetPos(MapPoint const & pos) { m_pos = pos; }
	uint16			GetWidth() const;
	uint16			GetHeight() const;
	POINT			GetHotpoint() const;
	void			GetBoundingRect(RECT *rect) const;

	LOADTYPE		GetLoadType() const { return m_loadType; }
	void			SetLoadType(LOADTYPE type) { m_loadType = type; }
	void			FullLoad();
	void			DumpFullLoad();

protected:
	sint32				m_facing;
	sint32				m_frame;
	uint16				m_transparency;
	sint32              m_index;
	MapPoint			m_pos;
	GoodSpriteGroup	*   m_goodSpriteGroup;
	GOODACTION			m_curGoodAction;
	LOADTYPE			m_loadType;
};

#endif
