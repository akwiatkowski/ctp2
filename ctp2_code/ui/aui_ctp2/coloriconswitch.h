#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __COLORICONSWITCH_H__
#define __COLORICONSWITCH_H__


#include <string>

#include "ui/aui_ctp2/c3_switch.h"


class Picture;


class ColorIconSwitch : public c3_Switch
{
public:

	ColorIconSwitch(
		AUI_ERRCODE *retval,
		uint32 id,
		sint32 x,
		sint32 y,
		sint32 width,
		sint32 height,
		MBCHAR const *pattern,
		MBCHAR const *icon,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );

	ColorIconSwitch(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR const *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );

	virtual AUI_ERRCODE	InitCommon( MBCHAR const *ldlBlock, BOOL isLDL = FALSE);

	~ColorIconSwitch() override;

	AUI_ERRCODE Resize(sint32 width, sint32 height) override;
	void	ResizePictureRect();

	void SetIcon(MBCHAR const *name);
	void ShrinkToFit(BOOL fit) { m_shrinkToFit = fit; }

	const MBCHAR *GetFilename() { return m_filename.c_str(); }

	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

protected:
	std::string		m_filename;
	BOOL		m_shrinkToFit;
	RECT		m_pictureRect;
};

#endif
