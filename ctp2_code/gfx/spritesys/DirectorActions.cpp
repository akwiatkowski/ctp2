//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Director queue actions
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
// - Repaired memory leak
//
//----------------------------------------------------------------------------
//
/// \file   DirectorActions.cpp
/// \brief  Action types for processing by the director

#include "ctp/c3.h"
#include "gfx/spritesys/directoractions.h"

#include "gfx/spritesys/EffectActor.h"        // EffectActor destructor

DQActionMoveProjectile::DQActionMoveProjectile()
:
    pmove_oldPos        (),
    pmove_newPos        (),
    projectile_path     (0)
{
}

// pshooting_actor / ptarget_actor are weak references; end_projectile is a
// unique_ptr that frees the effect actor unless ownership was already handed
// to the director (see dh_projectileMove, which release()s it).
DQActionMoveProjectile::~DQActionMoveProjectile() = default;
