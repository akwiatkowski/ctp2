#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __C3IMAGEFORMATS_H__
#define __C3IMAGEFORMATS_H__

#include "ui/aui_common/aui_image.h"

class aui_Surface;

class TargaImageFormat : public aui_ImageFormat
{
public:
	TargaImageFormat() = default;
	~TargaImageFormat() override = default;

	AUI_ERRCODE LoadRIM(MBCHAR const *filename, aui_Image *image);
	AUI_ERRCODE Load(MBCHAR const * filename, aui_Image *image) override;
};


class TiffImageFormat : public aui_ImageFormat
{
public:

	TiffImageFormat() = default;
	~TiffImageFormat() override = default;

	AUI_ERRCODE	Load(MBCHAR const *filename, aui_Image *image ) override;
};

#endif
