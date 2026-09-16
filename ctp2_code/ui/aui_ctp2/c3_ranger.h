#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __C3_RANGER_H__
#define __C3_RANGER_H__

#include "ui/aui_common/aui_ranger.h"
#include "ui/aui_ctp2/patternbase.h"

#define k_C3_RANGER_DEFAULTPATTERN		"pattern.tga"

class aui_Surface;
class aui_Static;

class c3_Ranger : public aui_Ranger, public PatternBase
{
public:

	c3_Ranger(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR const *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	c3_Ranger(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		AUI_RANGER_TYPE type,
		AUI_RANGER_ORIENTATION orientation,
		MBCHAR const *pattern,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	~c3_Ranger() override;

protected:
	c3_Ranger() : aui_Ranger() {}
	AUI_ERRCODE InitCommonLdl( MBCHAR const *ldlBlock );
	AUI_ERRCODE InitCommon( );
	AUI_ERRCODE CreateButtonsAndThumb( MBCHAR const *ldlBlock );

public:
	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

protected:
	AUI_ERRCODE RepositionButtons( ) override;

	aui_Static *m_arrows[ 4 ];
};

#endif
