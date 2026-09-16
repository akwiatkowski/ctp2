#include "ctp/c3.h"
#include "ui/aui_ctp2/iconbutton.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_ctp2/pattern.h"
#include "ui/aui_ctp2/icon.h"
#include "ui/aui_common/aui_window.h"
#include "ui/aui_common/aui_surface.h"
#include "ui/aui_common/aui_ldl.h"

#include "gfx/gfx_utils/pixelutils.h"
#include "gfx/gfx_utils/colorset.h"       // colorset_Get()
#include "gs/fileio/CivPaths.h"       // civpaths_Get()
#include "ui/aui_utils/primitives.h"

#include "ui/ldl/ldl_data.hpp"

#include "ui/aui_ctp2/c3ui.h"

IconButton::IconButton(
	AUI_ERRCODE *retval,
	uint32 id,
	sint32 x,
	sint32 y,
	sint32 width,
	sint32 height,
	MBCHAR const *pattern,
	MBCHAR const *icon,
	uint16 color,
	ControlActionCallback *ActionFunc,
	void *cookie )
:
	aui_ImageBase( (sint32)0 ),
	aui_TextBase(nullptr),
	aui_Button( retval, id, x, y, width, height, ActionFunc, cookie ),
	PatternBase( pattern ),
	m_color(color )
{
	InitCommon(icon);

}

IconButton::IconButton(
	AUI_ERRCODE *retval,
	uint32 id,
	MBCHAR const *ldlBlock,
	ControlActionCallback *ActionFunc,
	void *cookie )
	:
	aui_ImageBase( ldlBlock ),
	aui_TextBase(ldlBlock, (MBCHAR *)nullptr),
	aui_Button( retval, id, ldlBlock, ActionFunc, cookie ),
	PatternBase( ldlBlock, (MBCHAR *)nullptr )
{
	InitCommon(ldlBlock, TRUE);

}

AUI_ERRCODE IconButton::InitCommon( MBCHAR const *ldlBlock, BOOL isLDL)
{
	MBCHAR const		*name;

	if (isLDL) {
		ldl_datablock * block = aui_Ldl::FindDataBlock(ldlBlock);
		Assert( block != nullptr );
		if ( !block ) return AUI_ERRCODE_LDLFINDDATABLOCKFAILED;


		name = block->GetString( "icon" );
		Assert( name != nullptr );
	} else {
		name = ldlBlock;
	}

	MBCHAR path[_MAX_PATH];
	if (civpaths_Get()->FindFile(C3DIR_ICONS, name, path)) {
		m_filename = path;
		m_icon = c3ui_Get()->LoadIcon(m_filename.c_str());
	} else {
		m_filename.clear();
		m_icon = nullptr;
	}

	return AUI_ERRCODE_OK;
}

IconButton::~IconButton()
{
	// m_filename is std::string, auto-freed
}

AUI_ERRCODE IconButton::DrawThis( aui_Surface *surface, sint32 x, sint32 y )
{

	if ( IsHidden() ) return AUI_ERRCODE_OK;

	if ( !surface ) surface = m_window->TheSurface();

	RECT rect = { 0, 0, m_width, m_height };
	OffsetRect( &rect, m_x + x, m_y + y );
	ToWindow( &rect );

	RECT offsetRect = rect;

	offsetRect.left += 1;
	offsetRect.top += 1;

	m_pattern->Draw( surface, &rect );

	if ( IsDown() )
	{
		primitives_BevelRect16( surface, &rect, 1, 1, 16, 16 );
	}
	else
	{
		primitives_BevelRect16( surface, &rect, 1, 0, 16, 16 );
	}

	if ( IsActive() )
	{
		m_icon->Draw( surface, &offsetRect, colorset_Get()->GetColor(COLOR_BUTTON_TEXT_DROP));
		m_icon->Draw( surface, &rect, colorset_Get()->GetColor(COLOR_BUTTON_TEXT_HILITE));
	}
	else
	{
		m_icon->Draw( surface, &offsetRect, colorset_Get()->GetColor(COLOR_BUTTON_TEXT_DROP));
		m_icon->Draw( surface, &rect, colorset_Get()->GetColor(COLOR_BUTTON_TEXT_PLAIN));
	}

	if ( surface == m_window->TheSurface() )
		m_window->AddDirtyRect( &rect );

	return AUI_ERRCODE_OK;
}
