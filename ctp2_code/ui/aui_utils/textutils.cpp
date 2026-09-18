//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Text handling utilities
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
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Ambiguous fabs calls resolved.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/aui_utils/textutils.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_surface.h"

#include "ui/aui_utils/primitives.h"
#include "gfx/gfx_utils/colorset.h"               // colorset_Get()
#include "gs/fileio/CivPaths.h"               // civpaths_Get()

static HFONT		g_hFont;

HFONT hfont_Get()
{
	return g_hFont;
}

#define k_FONT_FILE_NAME	"ArialBd.ttf"
#define k_FONT_FACE_NAME	"Arial Bold"


void textutils_Initialize()
{
	MBCHAR		path[_MAX_PATH];

	civpaths_Get()->FindFile(C3DIR_FONTS, k_FONT_FILE_NAME, path);
	textutils_AddFont(path);
}

void textutils_Cleanup()
{
}







sint32 textutils_GetWidth(
		aui_Surface *pDirectSurface,
		const MBCHAR *pString
		)
{
	return 0;
}







sint32 textutils_GetHeight(
		aui_Surface *pDirectSurface,
		const MBCHAR *pString
		)
{
	return 0;
}

sint32 textutils_GetFontHeight(
		aui_Surface *surface,
		uint32 size
		)
{
	return 0;
}

HFONT textutils_GetFont(
		aui_Surface *surface,
		uint32 size
		)
{
	return textutils_CreateFont(surface, k_FONT_FACE_NAME, size * 10, 0, 0, TRUE);
}








RECT textutils_GetBounds(
		aui_Surface *pDirectSurface,
		const MBCHAR *pString
		)
{

	RECT rect = {0,0,0,0};

	return rect;
}







RECT textutils_CenterText(
		aui_Surface *pDirectSurface,
		const MBCHAR *pString,
		RECT *pDestRect
		)
{
	RECT center = {0,0,0,0};

	Assert(pDirectSurface);
	if (pDirectSurface==nullptr) return center;
	Assert(pString);
	if (pString==nullptr) return center;
	Assert(pDestRect);
	if (pDestRect==nullptr) return center;

	RECT bounds = textutils_GetBounds(pDirectSurface,pString);

	OffsetRect(&bounds,
				pDestRect->left + ((pDestRect->right-pDestRect->left)/2 - (bounds.right-bounds.left)/2),
				pDestRect->top + ((pDestRect->bottom-pDestRect->top)/2 - (bounds.bottom-bounds.top)/2));

	return bounds;

}







void textutils_AddFont(
		const MBCHAR *szFileName
		)
{
}







void textutils_RemoveFont(
		const MBCHAR *szFileName
		)
{
}







HFONT textutils_CreateFont(
		aui_Surface *pDirectSurface,
		const MBCHAR *szFaceName,
		sint32 iDeciPtHeight,
		sint32 iDeciPtWidth,
		sint32 iAttributes,

		BOOL fLogRes
		)
{
	return nullptr;
}







void textutils_SelectFont(
		HFONT hFont
		)
{
	Assert(hFont);
	if (hFont==nullptr) return;

	if (g_hFont)
		textutils_DeleteFont(g_hFont);

	g_hFont = hFont;
}







void textutils_DeleteFont(
		HFONT hFont
		)
{
	Assert(hFont);
	if (hFont==nullptr) return;
	// hfont_t has no definition anywhere: textutils_CreateFont is a stub
	// that only ever returns nullptr, so there is nothing to free. The old
	// `delete hFont` on the incomplete type was UB (-Wdelete-incomplete).
	(void)hFont;
}




void textutils_DropString(aui_Surface *surface, const MBCHAR *text, sint32 x, sint32 y, sint32 size, COLOR color, sint32 font)
{
	HFONT		tempFont;
	COLORREF	colorRef = colorset_Get()->GetColorRef(color);

	tempFont = textutils_CreateFont(surface, k_FONT_FACE_NAME, size * 10, 0, 0, TRUE);
	Assert(tempFont);
	if (!tempFont) return;

	textutils_SelectFont(tempFont);
	primitives_DropText(surface, x, y, text, colorRef, 1);
	textutils_DeleteFont(tempFont);
}




void textutils_ColoredDropString(aui_Surface *surface, const MBCHAR *text, sint32 x, sint32 y, sint32 size, COLOR textColor, COLOR dropColor, sint32 font)
{
	HFONT		tempFont;
	COLORREF	colorRefText = colorset_Get()->GetColorRef(textColor);
	COLORREF	colorRefDrop = colorset_Get()->GetColorRef(dropColor);

	tempFont = textutils_CreateFont(surface, k_FONT_FACE_NAME, size * 10, 0, 0, TRUE);
	Assert(tempFont);
	if (!tempFont) return;

	textutils_SelectFont(tempFont);
	primitives_ColoredDropText(surface, x, y, text, colorRefText, colorRefDrop, 1);
	textutils_DeleteFont(tempFont);
}




void textutils_CenteredDropString(aui_Surface *surface, const MBCHAR *text, RECT *destRect, sint32 size, COLOR color, sint32 font)
{
	HFONT		tempFont;
	COLORREF	colorRef = colorset_Get()->GetColorRef(color);

	tempFont = textutils_CreateFont(surface, k_FONT_FACE_NAME, size * 10, 0, 0, TRUE);
	Assert(tempFont);
	if (!tempFont) return;


	textutils_SelectFont(tempFont);

	textutils_CenterText(surface, text, destRect);

	primitives_DropTextCentered(surface, destRect, text, colorRef, 1);

	textutils_DeleteFont(tempFont);
}




void textutils_CenteredColoredDropString(aui_Surface *surface, const MBCHAR *text, RECT *destRect, sint32 size, COLOR textColor, COLOR dropColor,sint32 font)
{
	HFONT		tempFont;
	COLORREF	colorRefText = colorset_Get()->GetColorRef(textColor);
	COLORREF	colorRefDrop = colorset_Get()->GetColorRef(dropColor);

	tempFont = textutils_CreateFont(surface, k_FONT_FACE_NAME, size * 10, 0, 0, TRUE);
	Assert(tempFont);
	if (!tempFont) return;


	textutils_SelectFont(tempFont);

	textutils_CenterText(surface, text, destRect);

	primitives_ColoredDropTextCentered(surface, destRect, text, colorRefText, colorRefDrop, 1);

	textutils_DeleteFont(tempFont);
}

void textutils_SizedBoundedString(aui_Surface *surface, const MBCHAR *text, RECT *destRect, sint32 size, COLOR color, sint32 font)
{
	HFONT		tempFont;
	COLORREF	colorRef = colorset_Get()->GetColorRef(color);

	tempFont = textutils_CreateFont(surface, k_FONT_FACE_NAME, size * 10, 0, 0, TRUE);
	Assert(tempFont);
	if (!tempFont) return;


	textutils_SelectFont(tempFont);

	textutils_CenterText(surface, text, destRect);

	primitives_DrawBoundedText(surface, destRect, text, colorRef, 1);

	textutils_DeleteFont(tempFont);
}







void textutils_TestFonts(
		aui_Surface *pDirectSurface
		)
{
}
