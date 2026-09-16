#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __C3THUMB_H__
#define __C3THUMB_H__

#include "ui/aui_common/aui_thumb.h"
#include "ui/aui_ctp2/patternbase.h"
#include "ui/aui_common/aui_soundbase.h"

class C3Thumb : public aui_Thumb, public PatternBase
{
public:

	C3Thumb(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR const *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	C3Thumb(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		MBCHAR const *pattern,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	~C3Thumb() override = default;

	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

	AUI_ERRCODE	PlaySound( AUI_SOUNDBASE_SOUND sound );
};

#endif
