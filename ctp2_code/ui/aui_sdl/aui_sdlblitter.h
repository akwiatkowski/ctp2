#ifndef __AUI_SDLBLITTER_H__
#define __AUI_SDLBLITTER_H__

#ifdef __AUI_USE_SDL__

#include "ui/aui_common/aui_blitter.h"

class aui_SDLBlitter : public aui_Blitter
{
public:

	aui_SDLBlitter() = default;
	~aui_SDLBlitter() override = default;

	RobustBltFunc Blt override;
	NakedBltFunc Blt16To16 override;
	NakedColorBltFunc ColorBlt16 override;
	NakedStretchBltFunc StretchBlt16To16 override;
	NakedColorStencilBltFunc ColorStencilBlt16 override;

protected:
};

typedef aui_SDLBlitter aui_NativeBlitter;

#endif

#endif
