#include "ctp/c3.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_blitter.h"
#include "ui/aui_common/aui_surface.h"

#include "ui/aui_ctp2/c3ui.h"
#include "gfx/gfx_utils/pixelutils.h"

#include "ui/aui_ctp2/pattern.h"


Pattern::Pattern(
	AUI_ERRCODE *retval,
	MBCHAR const * filename,
	MBCHAR const * lightFilename,
	MBCHAR const * darkFilename )
:
	aui_Image( retval, filename )
{
	m_lightImage = std::make_unique<aui_Image>( retval, lightFilename );
	m_darkImage = std::make_unique<aui_Image>( retval, darkFilename );
}

Pattern::Pattern(
	AUI_ERRCODE *retval,
	MBCHAR const *filename )
:
	aui_Image( retval, filename ),
	m_lightImage( nullptr ),
	m_darkImage( nullptr )
{
}

Pattern::~Pattern( ) = default;

AUI_ERRCODE Pattern::Draw( aui_Surface *pDestSurf, RECT *pDestRect )
{
	if (!m_surface) return AUI_ERRCODE_OK;

	RECT rect = { 0, 0, m_surface->Width(), m_surface->Height() };
	return c3ui_Get()->TheBlitter()->TileBlt(
		pDestSurf,
		pDestRect,
		m_surface.get(),
		&rect,
		0,
		0,
		k_AUI_BLITTER_FLAG_COPY );
}

AUI_ERRCODE Pattern::Draw( aui_Surface *pDestSurf, RECT *pDestRect, RECT *pSrcRect )
{
	if (!m_surface) return AUI_ERRCODE_OK;

	return c3ui_Get()->TheBlitter()->TileBlt(
		pDestSurf,
		pDestRect,
		m_surface.get(),
		pSrcRect,
		0,
		0,
		k_AUI_BLITTER_FLAG_COPY );
}

AUI_ERRCODE Pattern::DrawDither( aui_Surface *pDestSurf, RECT *pDestRect, BOOL flag )
{
	RECT rect = { 0, 0, m_surface->Width(), m_surface->Height() };
	return c3ui_Get()->TheBlitter()->TileBlt(
		pDestSurf,
		pDestRect,
		m_surface.get(),
		&rect,
		0,
		0,
		k_AUI_BLITTER_FLAG_COPY );
}

AUI_ERRCODE Pattern::DrawDither(
	aui_Surface *pDestSurf,
	RECT *pDestRect,
	BOOL flag,
	sint32 lighten,
	sint32 darken
	)
{
	RECT rect = { 0, 0, m_surface->Width(), m_surface->Height() };
	return c3ui_Get()->TheBlitter()->TileBlt(
		pDestSurf,
		pDestRect,
		m_surface.get(),
		&rect,
		0,
		0,
		k_AUI_BLITTER_FLAG_COPY );
}
