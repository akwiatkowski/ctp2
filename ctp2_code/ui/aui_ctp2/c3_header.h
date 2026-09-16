#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __C3_HEADER_H__
#define __C3_HEADER_H__

#include "ui/aui_ctp2/c3_listbox.h"
#include "ui/aui_common/aui_header.h"

class c3_Header : public aui_Header
{
public:

	c3_Header(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR const *ldlBlock );
	c3_Header(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height );
	~c3_Header() override;

protected:
	c3_Header() : aui_Header() {}
	AUI_ERRCODE InitCommonLdl( MBCHAR const *ldlBlock );
	AUI_ERRCODE InitCommon( );
	AUI_ERRCODE CreateSwitches( MBCHAR const *ldlBlock = nullptr );
};

#endif
