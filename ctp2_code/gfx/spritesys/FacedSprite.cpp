//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Sprite with facings.
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
// None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Added separate counters in Sprite-derived classes to prevent crashes.
// - Added Get*Size() methods to increase portability (_msize=windows api)
// - Added size argument to Set*Data methods for increasing portability
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gfx/spritesys/FacedSprite.h"

#include "gfx/gfx_utils/tiffutils.h"
#include "gfx/gfx_utils/pixelutils.h"
#include "gfx/spritesys/spriteutils.h"
#include "gfx/spritesys/screenmanager.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_surface.h"

#include "gfx/spritesys/Sprite.h"
#include "gfx/tilesys/tiledmap.h"

#include "gs/fileio/Token.h"



FacedSprite::FacedSprite()
:   Sprite              (),
    m_facedFrameCount   (0)
{
	for (size_t facing = 0; facing < k_NUM_FACINGS; ++facing)
	{
		m_frames[facing]        = nullptr;
		m_framesSizes[facing]   = nullptr;
		m_miniframes[facing]    = nullptr;
		m_miniframesSizes[facing] = nullptr;
	}
	m_type = SPRITETYPE_FACED;
}


FacedSprite::~FacedSprite()
{
	for (size_t facing = 0; facing < k_NUM_FACINGS; ++facing)
	{
		for (size_t i = 0; i < m_facedFrameCount; ++i)
	        {
			if ((m_frames[facing] != nullptr) && (m_frames[facing][i] != nullptr)) {
				delete m_frames[facing][i];
				m_frames[facing][i] = nullptr;
			}
			if ((m_miniframes[facing] != nullptr) && (m_miniframes[facing][i] != nullptr)) {
				delete m_miniframes[facing][i];
				m_miniframes[facing][i] = nullptr;
			}
		}

		if (m_frames[facing] != nullptr) {
			delete [] m_frames[facing];
			m_frames[facing] = nullptr;
		}
		if (m_framesSizes[facing] != nullptr) {
			delete [] m_framesSizes[facing];
			m_framesSizes[facing] = nullptr;
		}
		if (m_miniframes[facing] != nullptr) {
			delete [] m_miniframes[facing];
			m_miniframes[facing] = nullptr;
		}
		if (m_miniframesSizes[facing] != nullptr) {
			delete [] m_miniframesSizes[facing];
			m_miniframesSizes[facing] = nullptr;
		}
	}
}


void FacedSprite::Import(size_t nframes, char *imageFiles[k_NUM_FACINGS][k_MAX_NAMES], char *shadowFiles[k_NUM_FACINGS][k_MAX_NAMES])
{
    AllocateFrameArrays(nframes);

	for (sint32 facing=0; facing < k_NUM_FACINGS; facing++)
	{
		for (uint16 i=0; i < nframes; i++)
		{
			char ext[_MAX_DIR];

			Pixel16 *   data        = nullptr;
			size_t      dataSize    = 0;
			Pixel32 *   image       = nullptr;
			Pixel32 *   miniimage	= nullptr;
			Pixel32 *   shadow		= nullptr;
			Pixel32 *   minishadow	= nullptr;
			char *      fname       = imageFiles[facing][i];

			_splitpath(fname,nullptr,nullptr,nullptr,ext);

			if (strstr(strupr(ext),"TIF"))
				ImportTIFF(i,imageFiles[facing],&image);
			else
				if (strstr(strupr(ext),"TGA"))
					ImportTGA(i,imageFiles[facing],&image);
				else
					printf("Unknown image file \"%s\"\n",fname);

			fname = shadowFiles[facing][i];

			_splitpath(fname,nullptr,nullptr,nullptr,ext);

			if (strstr(strupr(ext),"TIF"))
				ImportTIFF(i,shadowFiles[facing],&shadow);
			else
				if (strstr(strupr(ext),"TGA"))
					ImportTGA(i,shadowFiles[facing],&shadow);

			if (image)
			{
				spriteutils_CreateQuarterSize(image, m_width, m_height,&miniimage, TRUE);

				data = spriteutils_RGB32ToEncoded(image,shadow, m_width, m_height, &dataSize);
				SetFrameData(facing, i, data, dataSize);

				if (shadow)
					spriteutils_CreateQuarterSize(shadow, m_width, m_height,&minishadow, FALSE);

				data = spriteutils_RGB32ToEncoded(miniimage, minishadow, m_width >> 1, m_height >> 1, &dataSize);
				SetMiniFrameData(facing, i, data, dataSize);
			}
			else
			{

				printf("Could not locate %s.  Aborting.\n\n", imageFiles[facing][i]);
				exit(-1);
			}

			delete [] image;
			delete [] shadow;
			delete [] miniimage;
			delete [] minishadow;

			printf(".");
		}
	}
}

//----------------------------------------------------------------------------
//
// Name       : FacedSprite::Draw
//
// Description: Draw a sprite
//
// Parameters : drawX, drawY    : position
//              facing          : facing
//              scale           : magnification (zoom level)
//              transparency    :
//              outlineColor    :
//              flags
//
// Globals    : tiledmap_Get()
//
// Returns    : -
//
// Remark(s)  : Refactored a bit to better understand what is going on.
//              But the problem wasn't here after all.
//
//----------------------------------------------------------------------------
void FacedSprite::Draw(sint32 drawX, sint32 drawY, sint32 facing, double scale, sint16 transparency, Pixel16 outlineColor, uint16 flags)
{
	SetSurface();

    bool const      isReversed  = facing >= k_NUM_FACINGS;
    size_t          facingIndex = isReversed ? k_MAX_FACINGS - facing : facing;
    Pixel16 *       frame       = (scale == tiledmap_Get()->GetZoomScale(k_ZOOM_SMALLEST))
                                  ? m_miniframes[facingIndex][m_currentFrame]
                                  : m_frames[facingIndex][m_currentFrame];

    if (!frame)
    {
        return;
    }

	if (isReversed)
    {
		drawX -= (sint32)((double)(m_width - m_hotPoints[facingIndex].x) * scale);
	    drawY -= (sint32)((double)m_hotPoints[facingIndex].y * scale);
	}
    else
    {
		drawX -= (sint32)((double)m_hotPoints[facingIndex].x * scale);
	    drawY -= (sint32)((double)m_hotPoints[facingIndex].y * scale);
	}

	if (scale == tiledmap_Get()->GetZoomScale(k_ZOOM_LARGEST))
    {
		if (isReversed)
        {
			(this->*_DrawLowReversed)(frame, drawX, drawY, m_width, m_height, transparency, outlineColor, flags);
		}
        else
        {
			(this->*_DrawLow)(frame, drawX, drawY, m_width, m_height, transparency, outlineColor, flags);
        }
	}
    else if (scale == tiledmap_Get()->GetZoomScale(k_ZOOM_SMALLEST))
    {
		if (isReversed)
        {
			(this->*_DrawLowReversed)(frame, drawX, drawY, m_width>>1, m_height>>1, transparency, outlineColor, flags);
        }
		else
        {
			(this->*_DrawLow)(frame, drawX, drawY, m_width>>1, m_height>>1, transparency, outlineColor, flags);
        }
	}
    else
    {
		sint32 const    destWidth   = (sint32) (m_width * scale);
		sint32 const    destHeight  = (sint32) (m_height * scale);

		(this->*_DrawScaledLow)(frame, drawX, drawY, destWidth, destHeight,
								transparency, outlineColor, flags, isReversed
                               );
	}
}

BOOL FacedSprite::HitTest(POINT mousePt, sint32 drawX, sint32 drawY, sint32 facing, double scale, sint16 transparency,
						Pixel16 outlineColor, uint16 flags)
{
	if (facing < 5) {
		drawX -= (sint32)((double)m_hotPoints[facing].x * scale);
		drawY -= (sint32)((double)m_hotPoints[facing].y * scale);
	} else {
		drawX -=  (sint32)((double)(m_width - m_hotPoints[k_MAX_FACINGS - facing].x) * scale);
		drawY -= (sint32)((double)m_hotPoints[k_MAX_FACINGS - facing].y * scale);
	}


	if (scale == tiledmap_Get()->GetZoomScale(k_ZOOM_LARGEST)) {
		if (facing < 5) {
			return HitTestLow(mousePt, (Pixel16 *)m_frames[facing][m_currentFrame], drawX, drawY, m_width, m_height, transparency, outlineColor, flags);
		} else
			return HitTestLowReversed(mousePt, (Pixel16 *)m_frames[k_MAX_FACINGS - facing][m_currentFrame], drawX, drawY, m_width, m_height, transparency, outlineColor, flags);
	} else {
		if (scale == tiledmap_Get()->GetZoomScale(k_ZOOM_SMALLEST)) {
			if (facing < 5)
				return HitTestLow(mousePt, (Pixel16 *)m_miniframes[facing][m_currentFrame], drawX, drawY, m_width>>1, m_height>>1, transparency, outlineColor, flags);
			else
				return HitTestLowReversed(mousePt, (Pixel16 *)m_miniframes[k_MAX_FACINGS - facing][m_currentFrame], drawX, drawY, m_width>>1, m_height>>1, transparency, outlineColor, flags);
		} else {

			sint32 destWidth = (sint32)(m_width * scale);
			sint32 destHeight = (sint32)(m_height * scale);

			if (facing < 5) {
				return HitTestScaledLow(mousePt, (Pixel16 *)m_frames[facing][m_currentFrame], drawX, drawY, destWidth, destHeight,
									transparency, outlineColor, flags, FALSE);
			} else {
				return HitTestScaledLow(mousePt, (Pixel16 *)m_frames[k_MAX_FACINGS - facing][m_currentFrame], drawX, drawY, destWidth, destHeight,
									transparency, outlineColor, flags, TRUE);
			}
		}
	}
}

Pixel16 *FacedSprite::GetFrameData(uint16 facing, uint16 frame)
{
	Assert(facing < k_NUM_FACINGS);
	Assert(frame < m_facedFrameCount);
	Assert(m_frames[facing] != nullptr);

	return m_frames[facing][frame];
}

size_t FacedSprite::GetFrameDataSize(uint16 facing, uint16 frame)
{
	Assert(facing < k_NUM_FACINGS);
	Assert(frame < m_facedFrameCount);
	Assert(m_framesSizes[facing] != nullptr);
#ifdef _WINDOWS
	Assert(m_framesSizes[facing][frame] == _msize(GetFrameData(facing, frame)));

	return _msize(GetFrameData(facing, frame));
#else
	return m_framesSizes[facing][frame];
#endif
}

Pixel16 *FacedSprite::GetMiniFrameData(uint16 facing, uint16 frame)
{
	Assert(facing < k_NUM_FACINGS);
	Assert(frame < m_facedFrameCount);
	Assert(m_miniframes[facing] != nullptr);

	return m_miniframes[facing][frame];
}

size_t FacedSprite::GetMiniFrameDataSize(uint16 facing, uint16 frame)
{
	Assert(facing < k_NUM_FACINGS);
	Assert(frame < m_facedFrameCount);
	Assert(m_miniframesSizes[facing] != nullptr);
#ifdef _WINDOWS
	Assert(m_miniframesSizes[facing][frame] = _msize(GetMiniFrameData(facing, frame)));

	return _msize(GetMiniFrameData(facing, frame));
#else
	return m_miniframesSizes[facing][frame];
#endif
}

void FacedSprite::SetFrameData(uint16 facing, uint16 frame, Pixel16 *data, size_t size)
{
	Assert(facing < k_NUM_FACINGS);
	Assert(frame < m_facedFrameCount);
	Assert(m_frames[facing] != nullptr);
	Assert(m_framesSizes[facing] != nullptr);
#ifdef _WINDOWS
//	Assert((((data == NULL) && (size = 0)) || ((data != NULL) && (_msize(data) == size))));
#endif

	m_frames[facing][frame] = data;
	m_framesSizes[facing][frame] = size;
}

void FacedSprite::SetMiniFrameData(uint16 facing, uint16 frame, Pixel16 *data, size_t size)
{
	Assert(facing < k_NUM_FACINGS);
	Assert(frame < m_facedFrameCount);
	Assert(m_miniframes[facing] != nullptr);
	Assert(m_miniframesSizes[facing] != nullptr);
#ifdef _WINDOWS
//	Assert((((data == NULL) && (size = 0)) || ((data != NULL) && (_msize(data) == size))));
#endif

	m_miniframes[facing][frame] = data;
	m_miniframesSizes[facing][frame] = size;
}

void FacedSprite::DrawDirect(aui_Surface *surf, sint32 drawX, sint32 drawY, sint32 facing,
					   double scale, sint16 transparency, Pixel16 outlineColor, uint16 flags)
{
	BOOL wasNull = FALSE;

	if (surf == nullptr) {
		SetSurface();
		surf = m_surface;
		wasNull = TRUE;
	} else
		LockSurface(surf);

	if (facing < 5) {
		drawX -= (sint32)((double)m_hotPoints[facing].x * scale);
		drawY -= (sint32)((double)m_hotPoints[facing].y * scale);
	} else {
		drawX -=  (sint32)((double)(m_width - m_hotPoints[k_MAX_FACINGS - facing].x) * scale);
		drawY -= (sint32)((double)m_hotPoints[k_MAX_FACINGS - facing].y * scale);
	}


	if (drawX > surf->Width() - 1 || drawX < -(m_width*scale)) {
		UnlockSurface();
		return;
	}

	if (drawY > surf->Height() - 1 || drawY < -(m_height*scale))
	{
		UnlockSurface();
		return;
	}

	if (scale == tiledmap_Get()->GetZoomScale(k_ZOOM_LARGEST))
	{
		if (facing < 5) {
			(this->*_DrawLow)((Pixel16 *)m_frames[facing][m_currentFrame], drawX, drawY, m_width, m_height, transparency, outlineColor, flags);

		} else {
			(this->*_DrawLowReversed)((Pixel16 *)m_frames[k_MAX_FACINGS - facing][m_currentFrame], drawX, drawY, m_width, m_height, transparency, outlineColor, flags);
		}
	} else {
		if (scale == tiledmap_Get()->GetZoomScale(k_ZOOM_SMALLEST)) {
			if (facing < 5) {
				(this->*_DrawLow)((Pixel16 *)m_miniframes[facing][m_currentFrame], drawX, drawY, m_width>>1, m_height>>1, transparency, outlineColor, flags);
			} else {
				(this->*_DrawLowReversed)((Pixel16 *)m_miniframes[k_MAX_FACINGS - facing][m_currentFrame], drawX, drawY, m_width>>1, m_height>>1, transparency, outlineColor, flags);
			}
		} else {

			sint32 destWidth = (sint32)(m_width * scale);
			sint32 destHeight = (sint32)(m_height * scale);

			if (facing < 5) {
				(this->*_DrawScaledLow)((Pixel16 *)m_frames[facing][m_currentFrame], drawX, drawY, destWidth, destHeight,
									transparency, outlineColor, flags, FALSE);
			} else {
				(this->*_DrawScaledLow)((Pixel16 *)m_frames[k_MAX_FACINGS - facing][m_currentFrame], drawX, drawY, destWidth, destHeight,
									transparency, outlineColor, flags, TRUE);
			}
		}
	}

	if (surf != nullptr && !wasNull)
		UnlockSurface();
}





void FacedSprite::DirectionalDraw(sint32 drawX, sint32 drawY, sint32 facing,
					   double scale, sint16 transparency, Pixel16 outlineColor, uint16 flags)
{
	SetSurface();

	if (facing < 5) {
		drawX -= (sint32)((double)m_hotPoints[facing].x * scale);
		drawY -= (sint32)((double)m_hotPoints[facing].y * scale);
	} else {
		drawX -=  (m_width - (sint32)((double)(m_width - m_hotPoints[k_MAX_FACINGS - facing].x) * scale));
		drawY -= (sint32)((double)m_hotPoints[k_MAX_FACINGS - facing].y * scale);
	}


	if (drawX > screenmanager_Get()->GetSurfWidth() - (m_width*scale) || drawX < 0) return;
	if (drawY > screenmanager_Get()->GetSurfHeight() - (m_height*scale) || drawY < 0) return;

	if (scale == tiledmap_Get()->GetZoomScale(k_ZOOM_LARGEST)) {
		if (facing < 4 && facing > 0)
		{
			(this->*_DrawLow)((Pixel16 *)m_frames[facing][m_currentFrame], drawX, drawY, m_width, m_height, transparency, outlineColor, flags);

		}
		else if (facing == 4 || facing == 0)
		{
			(this->*_DrawLowReversed)((Pixel16 *)m_frames[facing][m_currentFrame], drawX, drawY, m_width, m_height, transparency, outlineColor, flags);
		}
		else
		{
			(this->*_DrawLowReversed)((Pixel16 *)m_frames[k_MAX_FACINGS - facing][m_currentFrame], drawX, drawY, m_width, m_height, transparency, outlineColor, flags);
		}
	} else {
		if (scale == tiledmap_Get()->GetZoomScale(k_ZOOM_SMALLEST)) {
			if (facing < 4 && facing > 0)
			{
				(this->*_DrawLow)((Pixel16 *)m_miniframes[facing][m_currentFrame], drawX, drawY, m_width>>1, m_height>>1, transparency, outlineColor, flags);
			}
			else if (facing == 4 || facing == 0)
			{
				(this->*_DrawLowReversed)((Pixel16 *)m_miniframes[facing][m_currentFrame], drawX, drawY, m_width>>1, m_height>>1, transparency, outlineColor, flags);
			}
			else
			{
				(this->*_DrawLowReversed)((Pixel16 *)m_miniframes[facing][m_currentFrame], drawX, drawY, m_width>>1, m_height>>1, transparency, outlineColor, flags);
			}
		} else {

			sint32 destWidth = (sint32)(m_width * scale);
			sint32 destHeight = (sint32)(m_height * scale);

			if (facing < 4 && facing > 0)
			{
				(this->*_DrawScaledLow)((Pixel16 *)m_frames[facing][m_currentFrame], drawX, drawY, destWidth, destHeight,
									transparency, outlineColor, flags, FALSE);
			}
			else if(facing == 4 || facing == 0)
			{
				(this->*_DrawScaledLow)((Pixel16 *)m_frames[facing][m_currentFrame], drawX, drawY, destWidth, destHeight,
									transparency, outlineColor, flags, TRUE);
			}
			else
			{
				(this->*_DrawScaledLow)((Pixel16 *)m_frames[k_MAX_FACINGS - facing][m_currentFrame], drawX, drawY, destWidth, destHeight,
									transparency, outlineColor, flags, TRUE);
			}
		}
	}
}


sint32 FacedSprite::ParseFromTokens(Token *theToken)
{
	sint32		tmp;
	sint32		i;

	if (!token_ParseAnOpenBraceNext(theToken)) return FALSE;

	if (!token_ParseValNext(theToken, TOKEN_SPRITE_NUM_FRAMES, tmp)) return FALSE;
	m_facedFrameCount = tmp;

	if (!token_ParseValNext(theToken, TOKEN_SPRITE_FIRST_FRAME, tmp)) return FALSE;
	m_firstFrame = (uint16)tmp;

	if (!token_ParseValNext(theToken, TOKEN_SPRITE_WIDTH, tmp)) return FALSE;
	m_width = (uint16)tmp;

	if (!token_ParseValNext(theToken, TOKEN_SPRITE_HEIGHT, tmp)) return FALSE;
	m_height = (uint16)tmp;

	if (!token_ParseKeywordNext(theToken, TOKEN_SPRITE_HOT_POINTS)) return FALSE;
	for (i=0; i<k_NUM_FACINGS; i++) {
		POINT		p;

		if (theToken->Next() == TOKEN_NUMBER) theToken->GetNumber(tmp);
		else return FALSE;

		p.x = tmp;

		if (theToken->Next() == TOKEN_NUMBER) theToken->GetNumber(tmp);
		else return FALSE;

		p.y = tmp;

		m_hotPoints[i] = p;
	}

	if (!token_ParseAnCloseBraceNext(theToken)) return FALSE;

	return TRUE;
}

//----------------------------------------------------------------------------
//
// Name       : FacedSprite::AllocateFrameArrays
//
// Description: Allocate memory for the faced sprites.
//
// Parameters : count   : number of sprites per facing to reserve
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : Assumption: Memory has not been allocated before.
//
//----------------------------------------------------------------------------
void FacedSprite::AllocateFrameArrays(size_t count)
{
    // Two-phase sprite loads (Basic then Full) reuse the same FacedSprite
    // object.  Mirror the destructor: free any previously allocated per-frame
    // buffers and per-facing arrays before reallocating.  The old Assert was
    // a no-op in release builds and silently leaked.
    for (size_t facing = 0; facing < k_NUM_FACINGS; ++facing)
    {
        for (size_t i = 0; i < m_facedFrameCount; ++i)
        {
            if (m_frames[facing] && m_frames[facing][i])
            {
                delete m_frames[facing][i];
                m_frames[facing][i] = nullptr;
            }
            if (m_miniframes[facing] && m_miniframes[facing][i])
            {
                delete m_miniframes[facing][i];
                m_miniframes[facing][i] = nullptr;
            }
        }
        delete [] m_frames[facing];          m_frames[facing] = nullptr;
        delete [] m_framesSizes[facing];     m_framesSizes[facing] = nullptr;
        delete [] m_miniframes[facing];      m_miniframes[facing] = nullptr;
        delete [] m_miniframesSizes[facing]; m_miniframesSizes[facing] = nullptr;
    }
    m_facedFrameCount = 0;

	for (size_t facing = 0; facing < k_NUM_FACINGS; ++facing)
    {
		m_frames[facing]          = new Pixel16 * [count];
		m_framesSizes[facing]     = new size_t    [count];
		m_miniframes[facing]      = new Pixel16 * [count];
		m_miniframesSizes[facing] = new size_t    [count];

        for (size_t i = 0; i < count; ++i)
        {
            m_frames[facing][i]          = nullptr;
            m_framesSizes[facing][i]     = 0;
            m_miniframes[facing][i]      = nullptr;
            m_miniframesSizes[facing][i] = 0;
        }
	}

    m_facedFrameCount   = count;
}

void FacedSprite::Export(FILE *file)
{
	extern TokenData	g_allTokens[];
	sint32				i;

	fprintf(file, "\t{\n");

	fprintf(file, "\t\t%s\t%zu\n", g_allTokens[TOKEN_SPRITE_NUM_FRAMES].keyword, m_facedFrameCount);

	fprintf(file, "\t\t%s\t%d\n", g_allTokens[TOKEN_SPRITE_FIRST_FRAME].keyword, m_firstFrame);

	fprintf(file, "\t\t%s\t%d\n", g_allTokens[TOKEN_SPRITE_WIDTH].keyword, m_width);

	fprintf(file, "\t\t%s\t%d\n", g_allTokens[TOKEN_SPRITE_HEIGHT].keyword, m_height);

	fprintf(file, "\t\t%s\n", g_allTokens[TOKEN_SPRITE_HOT_POINTS].keyword);
	for (i=0; i<k_NUM_FACINGS; i++) {
		fprintf(file, "\t\t\t%d %d\n", m_hotPoints[i].x, m_hotPoints[i].y);
	}

	fprintf(file, "\t}\n\n");

	return;
}
