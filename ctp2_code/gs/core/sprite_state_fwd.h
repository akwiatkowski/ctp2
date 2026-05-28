//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Forward declarations + shared_ptr typedefs for sprite system
//
// gs/ headers that store SpriteStatePtr / UnitActorPtr members or expose
// Get/Set accessors only need the forward declaration + the shared_ptr
// typedef, not the full class definitions.  Including this header
// instead of `gfx/spritesys/SpriteState.h` or `gfx/spritesys/UnitActor.h`
// keeps gs/ headers off the gfx/ dependency.
//
// gfx/spritesys/SpriteState.h and gfx/spritesys/UnitActor.h re-include
// this header so the typedefs have a single canonical definition
// (avoids ODR conflicts).
//
//----------------------------------------------------------------------------

#pragma once

#include <memory>   // std::shared_ptr

class SpriteState;
class UnitActor;

typedef std::shared_ptr<SpriteState> SpriteStatePtr;
typedef std::shared_ptr<UnitActor>   UnitActorPtr;
