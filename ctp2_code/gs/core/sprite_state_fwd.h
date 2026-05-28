//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Forward declaration + shared_ptr typedef for SpriteState
//
// gs/ headers that store SpriteStatePtr members or expose Get/Set
// accessors only need the forward declaration + the shared_ptr typedef,
// not the full SpriteState class definition.  Including this header
// instead of `gfx/spritesys/SpriteState.h` keeps gs/ headers off the
// gfx/ dependency.
//
// gfx/spritesys/SpriteState.h itself #includes this header so the
// typedef has a single canonical definition (avoids ODR conflict).
//
//----------------------------------------------------------------------------

#pragma once

#include <memory>   // std::shared_ptr

class SpriteState;
typedef std::shared_ptr<SpriteState> SpriteStatePtr;
