//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Color definitions.
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
// - Added new colors above player 15 by Martin G�hmann
//
//----------------------------------------------------------------------------

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __COLORSET_H__
#define __COLORSET_H__

//----------------------------------------------------------------------------
// Library dependencies
//----------------------------------------------------------------------------

#include <vector>

//----------------------------------------------------------------------------
// Export overview
//----------------------------------------------------------------------------

class ColorSet;

#define k_MAX_COLOR_SET     100     // File name: Colors##.txt (00 - 99)

// COLOR enum lives in gs/core/color_types.h so gs/ and ai/ code can
// include it without depending on gfx/.
#include "gs/core/color_types.h"

extern ColorSet *	g_colorSet;

//----------------------------------------------------------------------------
// Project dependencies
//----------------------------------------------------------------------------

#include "gfx/gfx_utils/pixeltypes.h" // Pixel16

//----------------------------------------------------------------------------
// Class declarations
//----------------------------------------------------------------------------

class ColorSet
{
public:
	ColorSet();
	virtual ~ColorSet(void);

	Pixel16		GetColor(COLOR color) const;
	COLORREF	GetColorRef(COLOR color) const;
	Pixel16		GetPlayerColor(sint32 playerNum) const;
	COLOR		ComputePlayerColor(sint32 playerNum) const;

	Pixel16		GetDarkColor(COLOR color) const;
	Pixel16		GetDarkPlayerColor(sint32 playerNum) const;
	COLORREF	GetDarkColorRef(COLOR color) const;
	Pixel16		GetLightColor(COLOR color) const;
	Pixel16		GetLightPlayerColor(sint32 playerNum) const;
	COLORREF	GetLightColorRef(COLOR color) const;

	static void	Initialize(uint32 fileNumber = 0);
    static void Cleanup(void);

private:
    void        Import(uint32 fileNumber = 0);

	std::vector<Pixel16>    m_colors;
};

#endif
