//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description	: Wonder movie pop-up window
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
// - Memory leak repaired.
// - Initialized local variables. (Sep 9th 2005 Martin Gühmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "ui/aui_common/aui_moviebutton.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_movie.h"

#include "ui/aui_ctp2/c3window.h"
#include "ui/aui_ctp2/pattern.h"
#include "ui/aui_ctp2/patternbase.h"
#include "ui/aui_ctp2/c3_static.h"

#include "ui/interface/wondermoviewindow.h"
#include "ui/interface/wondermoviewin.h"
#include "ui/aui_ctp2/ctp2_hypertextbox.h"


WonderMovieWindow::WonderMovieWindow(
									 AUI_ERRCODE *retval,
									 uint32 id,
									 MBCHAR *ldlBlock,
									 sint32 bpp,
									 AUI_WINDOW_TYPE type )
									 :
C3Window(retval, id, ldlBlock, bpp, type)
{
	*retval = InitCommonLdl(ldlBlock);
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}


WonderMovieWindow::WonderMovieWindow(
									 AUI_ERRCODE *retval,
									 uint32 id,
									 sint32 x,
									 sint32 y,
									 sint32 width,
									 sint32 height,
									 sint32 bpp,
									 MBCHAR *pattern,
									 AUI_WINDOW_TYPE type)
									 :
C3Window( retval, id, x, y, width, height, bpp, pattern, type )
{
	*retval = InitCommon();
	Assert(AUI_SUCCESS(*retval));
	if (!AUI_SUCCESS(*retval)) return;
}


WonderMovieWindow::~WonderMovieWindow()
{
}




AUI_ERRCODE WonderMovieWindow::InitCommonLdl(MBCHAR *ldlBlock)
{
	MBCHAR			buttonBlock[k_AUI_LDL_MAXBLOCK+1];
	AUI_ERRCODE		errcode = AUI_ERRCODE_OK;

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "MovieButton");
	m_movieButton = std::make_unique<aui_MovieButton>(&errcode, aui_UniqueId(), buttonBlock, wondermoviewin_MovieButtonCallback);
	Assert(errcode == AUI_ERRCODE_OK);
	if (errcode != AUI_ERRCODE_OK) return AUI_ERRCODE_LOADFAILED;
	AddControl(m_movieButton.get());

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "WonderName");
	m_wonderName = std::make_unique<c3_Static>(&errcode, aui_UniqueId(), buttonBlock);
	Assert(errcode == AUI_ERRCODE_OK);
	if (errcode != AUI_ERRCODE_OK) return AUI_ERRCODE_LOADFAILED;
	AddControl(m_wonderName.get());

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "TopBorder");
	m_topBorder = std::make_unique<c3_Static>(&errcode, aui_UniqueId(), buttonBlock);
	Assert(errcode == AUI_ERRCODE_OK);
	if (errcode != AUI_ERRCODE_OK) return AUI_ERRCODE_LOADFAILED;
	AddControl(m_topBorder.get());

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "LeftBorder");
	m_leftBorder = std::make_unique<c3_Static>(&errcode, aui_UniqueId(), buttonBlock);
	Assert(errcode == AUI_ERRCODE_OK);
	if (errcode != AUI_ERRCODE_OK) return AUI_ERRCODE_LOADFAILED;
	AddControl(m_leftBorder.get());

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "RightBorder");
	m_rightBorder = std::make_unique<c3_Static>(&errcode, aui_UniqueId(), buttonBlock);
	Assert(errcode == AUI_ERRCODE_OK);
	if (errcode != AUI_ERRCODE_OK) return AUI_ERRCODE_LOADFAILED;
	AddControl(m_rightBorder.get());

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "BottomBorder");
	m_bottomBorder = std::make_unique<c3_Static>(&errcode, aui_UniqueId(), buttonBlock);
	Assert(errcode == AUI_ERRCODE_OK);
	if (errcode != AUI_ERRCODE_OK) return AUI_ERRCODE_LOADFAILED;
	AddControl(m_bottomBorder.get());

	snprintf(buttonBlock, sizeof(buttonBlock), "%s.%s", ldlBlock, "Text");
	m_textBox = std::make_unique<ctp2_HyperTextBox>(&errcode, aui_UniqueId(), buttonBlock);
	Assert(errcode == AUI_ERRCODE_OK);
	if(errcode != AUI_ERRCODE_OK) return AUI_ERRCODE_LOADFAILED;
	AddControl(m_textBox.get());

	return InitCommon();
}

AUI_ERRCODE WonderMovieWindow::InitCommon()
{
	return C3Window::InitCommon();
}


void WonderMovieWindow::SetMovie(const MBCHAR *filename)
{
	if (!m_movieButton) return;

	m_movieButton->SetMovie(filename);
	m_movieButton->SetFlags(k_AUI_MOVIE_PLAYFLAG_PLAYANDHOLD);
}


void WonderMovieWindow::SetWonderName(MBCHAR *name)
{
	if (m_wonderName)
		m_wonderName->SetText(name);
}

void WonderMovieWindow::SetText(const MBCHAR *text)
{
	if(m_textBox)
		m_textBox->SetHyperText(text);
}


AUI_ERRCODE WonderMovieWindow::Idle()
{

	return AUI_ERRCODE_OK;
}


AUI_ERRCODE WonderMovieWindow::DrawThis( aui_Surface *surface, sint32 x, sint32 y )
{

	if ( IsHidden() ) return AUI_ERRCODE_OK;

	if (m_pattern) {
		RECT rect = { 0, 0, m_width, m_height };
		m_pattern->Draw( m_surface.get(), &rect );

		m_dirtyList->AddRect( &rect );
	}

	return AUI_ERRCODE_OK;
}
