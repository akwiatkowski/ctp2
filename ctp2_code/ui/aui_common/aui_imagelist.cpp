//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : User interface image list handling
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
// - Unload image info when not found.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/aui_common/aui_ui.h"
#include "ui/aui_common/aui_blitter.h"
#include "ui/aui_common/aui_surface.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_imagelist.h"

aui_ImageList::aui_ImageListInfo::~aui_ImageListInfo()
{

	if(m_image) {
		aui_ui_Get()->UnloadImage(m_image);
		m_image = nullptr;
	}

	// m_imageName is std::string, auto-freed
}

void aui_ImageList::aui_ImageListInfo::Load()
{

	if(m_image)
		return;

	if(!m_imageName.empty()) {

		m_image = aui_ui_Get()->LoadImage(m_imageName.c_str());
		Assert(m_image);

		if(m_image) {
			m_image->SetChromakey(m_chromaRed,m_chromaGreen,m_chromaBlue);
			m_imageName.clear();
		}
		else
		{
			aui_ui_Get()->UnloadImage(m_imageName.c_str());
		}
	}
}

aui_ImageList::aui_ImageList(sint32 numStates, sint32 numImages,
							 bool loadOnDemand) :
m_numStates(numStates),
m_numImages(numImages),
m_loadOnDemand(loadOnDemand)
{

	Assert(m_numStates);
	Assert(m_numImages);

	m_images = new aui_ImageListInfo *[m_numStates];

	for(sint32 i = 0; i < m_numStates; i++) {
		m_images[i] = new aui_ImageListInfo[m_numImages];
	}

	m_currentState = 0;
}

aui_ImageList::~aui_ImageList()
{

	for(sint32 i = 0; i < m_numStates; i++) {
		delete [] m_images[i];
		m_images[i] = nullptr;
	}

	delete [] m_images;
	m_images = nullptr;
}




void aui_ImageList::SetImage(sint32 state, sint32 imageIndex, AUI_IMAGEBASE_BLTTYPE bltType,
							 AUI_IMAGEBASE_BLTFLAG bltFlag, RECT *rect,
							 const MBCHAR *imageFileName, sint32 chromaRed,
							 sint32 chromaGreen, sint32 chromaBlue)
{

	if(!VerifyRange(state, imageIndex))
		return;

	aui_ImageListInfo *info = &m_images[state][imageIndex];

	info->m_bltType = bltType;
	info->m_bltFlag = bltFlag;
	info->m_rect.left = rect->left;
	info->m_rect.right = rect->right;
	info->m_rect.top = rect->top;
	info->m_rect.bottom = rect->bottom;
	info->m_chromaRed = chromaRed;
	info->m_chromaGreen = chromaGreen;
	info->m_chromaBlue = chromaBlue;

	ExchangeImage(state, imageIndex, imageFileName);
}

void aui_ImageList::ExchangeImage(sint32 state, sint32 imageIndex,
								  const MBCHAR *imageFileName)
{

	if(!VerifyRange(state, imageIndex))
		return;

	aui_ImageListInfo *info = &m_images[state][imageIndex];

	aui_Image *oldImage = info->m_image;
	info->m_image = nullptr;

	info->m_imageName.clear();

	if(!imageFileName) {
		if(oldImage)
			aui_ui_Get()->UnloadImage(oldImage);

		return;
	}

	if(m_loadOnDemand) {

		info->m_imageName = imageFileName;
	} else {

		aui_Image *theImage = aui_ui_Get()->LoadImage(const_cast<char*>(imageFileName));
		if(theImage) {

			if(info->m_rect.right < 0)
				info->m_rect.right = info->m_rect.left +
				theImage->TheSurface()->Width();
			if(info->m_rect.bottom < 0)
				info->m_rect.bottom = info->m_rect.top +
				theImage->TheSurface()->Height();

		info->m_image = theImage;

		info->m_image->SetChromakey(info->m_chromaRed,
			info->m_chromaGreen, info->m_chromaBlue);
		}
		else
		{
			aui_ui_Get()->UnloadImage(const_cast<MBCHAR *>(imageFileName));
		}
	}
	if(oldImage)
		aui_ui_Get()->UnloadImage(oldImage);
}

aui_Image *aui_ImageList::GetImage(sint32 state, sint32 imageIndex)
{

	if(!VerifyRange(state, imageIndex))
		return(nullptr);

	aui_ImageListInfo *info = &m_images[state][imageIndex];

	if(!info->m_image) {

		if(info->m_imageName.empty())
			return nullptr;

		info->Load();
		Assert(info->m_image);
	}

	return info->m_image;
}

aui_ImageList::aui_ImageListInfo *aui_ImageList::GetImageInfo(sint32 state, sint32 imageIndex)
{

	if(!VerifyRange(state, imageIndex))
		return(nullptr);

	aui_ImageListInfo *info = &m_images[state][imageIndex];

	if(!info->m_image) {
		if(!info->m_imageName.empty()) {
			info->Load();
			Assert(info->m_image);
		}
	}

	return info;
}


AUI_ERRCODE aui_ImageList::DrawImages(aui_Surface *destSurf, RECT *destRect)
{

	AUI_ERRCODE err = AUI_ERRCODE_OK;

	for(sint32 imageIndex = 0; imageIndex < m_numImages; imageIndex++) {

		aui_ImageListInfo *info = GetImageInfo( m_currentState, imageIndex);
		if(!info || !info->m_image)
			continue;

		aui_Surface *srcSurf = info->m_image->TheSurface();

		RECT srcRect = { 0, 0, srcSurf->Width(), srcSurf->Height() };

		RECT subDestRect = {
			info->m_rect.left + destRect->left,
			info->m_rect.top + destRect->top,
			info->m_rect.right + destRect->left,
			info->m_rect.bottom + destRect->top
		};

		uint32 flag = 0;
		switch ( info->m_bltFlag )
		{
			default:
			case AUI_IMAGEBASE_BLTFLAG_COPY:
				flag |= k_AUI_BLITTER_FLAG_COPY;
				break;
			case AUI_IMAGEBASE_BLTFLAG_CHROMAKEY:
				flag |= k_AUI_BLITTER_FLAG_CHROMAKEY;
				break;
			case AUI_IMAGEBASE_BLTFLAG_BLEND:
				flag |= k_AUI_BLITTER_FLAG_BLEND;
				break;
		}


		switch(info->m_bltType) {
			default:
			case AUI_IMAGEBASE_BLTTYPE_COPY:
				err = aui_ui_Get()->TheBlitter()->Blt(destSurf, subDestRect.left,
					subDestRect.top, srcSurf, &srcRect, flag);
				break;

			case AUI_IMAGEBASE_BLTTYPE_STRETCH:
				err = aui_ui_Get()->TheBlitter()->StretchBlt(destSurf, &subDestRect,
					srcSurf, &srcRect, flag);
				break;
			case AUI_IMAGEBASE_BLTTYPE_TILE:
				err = aui_ui_Get()->TheBlitter()->TileBlt(destSurf, &subDestRect,
					srcSurf, &srcRect, 0, 0, flag );
				break;
		}
	}

	Assert(err == AUI_ERRCODE_OK);
	return(err);
}

AUI_ERRCODE aui_ImageList::SetSize(sint32 state, sint32 imageIndex, RECT *size)
{

	aui_ImageListInfo *info = GetImageInfo(state, imageIndex);
	if(!info)
		return AUI_ERRCODE_INVALIDPARAM;

	info->m_rect.left = size->left;
	info->m_rect.right = size->right;
	info->m_rect.top = size->top;
	info->m_rect.bottom = size->bottom;

	return AUI_ERRCODE_OK;
}

RECT *aui_ImageList::GetSize(sint32 state, sint32 imageIndex)
{

	aui_ImageListInfo *info = GetImageInfo(state, imageIndex);
	if(!info)
		return nullptr;

	return(&info->m_rect);
}
