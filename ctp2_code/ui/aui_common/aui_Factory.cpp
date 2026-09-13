/// \file aui_Factory.cpp
/// \brief Factory for native aui_* Instance creation
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Factory for native aui_* Instance creation
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
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/aui_common/aui_Factory.h"


#include "ui/aui_sdl/aui_sdlsurface.h"
#include "ui/aui_sdl/aui_sdlmouse.h"
#include "ui/aui_sdl/aui_sdlkeyboard.h"

#include "ui/aui_ctp2/c3ui.h"   // C3UI


aui_Surface *
aui_Factory::new_Surface(AUI_ERRCODE &retval,
                         const sint32 &width,
                         const sint32 &height,
                         void *data,
                         const BOOL &isPrimary,
                         const BOOL &useVideoMemory,
                         const BOOL &takeOwnership,
                         sint32 bpp
                        )
{
	// bpp == 0 -> follow the global display depth (historic behaviour).
	sint32 const surfaceBpp = (bpp > 0) ? bpp : c3ui_Get()->BitsPerPixel();

	aui_SDLSurface *surface = nullptr;

	surface = new aui_SDLSurface(&retval, width, height, surfaceBpp, C3UI::DD(),
	                             isPrimary, useVideoMemory, takeOwnership);
	Assert( AUI_NEWOK(surface, retval) );

	return surface;
}

aui_Mouse *
aui_Factory::new_Mouse(AUI_ERRCODE &retval,
                       MBCHAR      *ldlBlock,
                       const BOOL  &useExclusiveMode
                      )
{
	aui_SDLMouse *mouse = nullptr;

	mouse = new aui_SDLMouse(&retval, ldlBlock, useExclusiveMode);
	Assert( AUI_NEWOK(mouse, retval) );

	return mouse;
}

aui_Keyboard *
aui_Factory::new_Keyboard(AUI_ERRCODE &retval)
{
	aui_SDLKeyboard *keyboard = nullptr;

	keyboard = new aui_SDLKeyboard(&retval);
	Assert( AUI_NEWOK(keyboard, retval) );

	return keyboard;
}
