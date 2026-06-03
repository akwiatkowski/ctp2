//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : CTP2 drop down menu
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
// -None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - ForceSelect can now be disabled. (Feb 4th 2007 Martin Gühmann)
//
//----------------------------------------------------------------------------

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __CTP2_DROPDOWN_H__
#define __CTP2_DROPDOWN_H__

#include "ui/aui_common/aui_dropdown.h"
#include "ui/aui_ctp2/patternbase.h"

class ctp2_ListItem;

class ctp2_DropDown : public aui_DropDown, public PatternBase
{
public:

	ctp2_DropDown(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	ctp2_DropDown(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		MBCHAR *pattern,
		sint32 buttonSize = 0,
		sint32 windowSize = 0,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	virtual ~ctp2_DropDown() = default;

	void	Clear();

	virtual AUI_ERRCODE Draw(aui_Surface *surface, sint32 x, sint32 y);

	virtual AUI_ERRCODE DrawThis(
						aui_Surface *surface = nullptr,
						sint32 x = 0,
						sint32 y = 0 );


	AUI_ERRCODE		AddItem(ctp2_ListItem *item);


	void			BuildListStart();


	void			BuildListEnd();
	void			SetForceSelect(bool forceSelect);

protected:
	ctp2_DropDown() : aui_DropDown() {}
	AUI_ERRCODE		InitCommonLdl( MBCHAR *ldlBlock );
	AUI_ERRCODE		InitCommon( sint32 buttonSize, sint32 windowSize );
	AUI_ERRCODE		CreateComponents( MBCHAR *ldlBlock = nullptr );

protected:
	virtual AUI_ERRCODE	RepositionButton( );
	virtual AUI_ERRCODE	RepositionListBoxWindow( );
};

#endif
