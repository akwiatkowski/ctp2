//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Sprite with facings and shadows.
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
#include "gfx/spritesys/FacedSpriteWshadow.h"

#include "gfx/gfx_utils/tiffutils.h"
#include "gfx/gfx_utils/pixelutils.h"
#include "gfx/spritesys/spriteutils.h"
#include "ctp/ctp2_utils/c3errors.h"
#include "gfx/spritesys/screenmanager.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_surface.h"

#include "gfx/spritesys/Sprite.h"

#include "gs/fileio/Token.h"



FacedSpriteWshadow::FacedSpriteWshadow()
:   Sprite              (),
    m_hasShadow         (TRUE),
    m_shadowFrameCount  (0)
{
	for (size_t facing = 0; facing < k_NUM_FACINGS; ++facing)
	{
		m_frames[facing] = nullptr;
		m_framesSizes[facing] = nullptr;
		m_miniframes[facing] = nullptr;
		m_miniframesSizes[facing] = nullptr;
		m_shadowFrames[facing] = nullptr;
		m_shadowFramesSizes[facing] = nullptr;
		m_miniShadowFrames[facing] = nullptr;
		m_miniShadowFramesSizes[facing] = nullptr;
	}
	m_type = SPRITETYPE_FACEDWSHADOW;
}


FacedSpriteWshadow::~FacedSpriteWshadow()
{
	for (size_t facing = 0; facing < k_NUM_FACINGS; ++facing)
	{
        for (size_t i = 0; i < m_shadowFrameCount; ++i)
		{
            delete m_frames[facing][i];
            delete m_miniframes[facing][i];
            delete m_shadowFrames[facing][i];
            delete m_miniShadowFrames[facing][i];
		}

		delete [] m_frames[facing];
		delete [] m_framesSizes[facing];
		delete [] m_miniframes[facing];
		delete [] m_miniframesSizes[facing];
	        delete [] m_shadowFrames[facing];
		delete [] m_shadowFramesSizes[facing];
		delete [] m_miniShadowFrames[facing];
		delete [] m_miniShadowFramesSizes[facing];
	}
}

void FacedSpriteWshadow::Import(uint16 nframes, char *imageFiles[k_NUM_FACINGS][k_MAX_NAMES], char *shadowFiles[k_NUM_FACINGS][k_MAX_NAMES])
{
	AllocateFrameArrays(nframes);

	for (sint32 facing=0; facing < k_NUM_FACINGS; facing++)
	{
		for (sint32 i=0; i < nframes; i++)
		{
			char *tif = StripTIF2Mem(imageFiles[facing][i], &m_width, &m_height);

			if (tif)
			{
				char *minitif = nullptr;
				spriteutils_CreateQuarterSize((Pixel32 *)tif, m_width, m_height, (Pixel32 **)&minitif, TRUE);

				size_t dataSize = 0;
				Pixel16 *data =	spriteutils_RGB32ToEncoded((Pixel32 *)tif, m_width, m_height, &dataSize);
				SetFrameData(facing, i, data, dataSize);

				data = spriteutils_RGB32ToEncoded((Pixel32 *)minitif, m_width >> 1, m_height >> 1, &dataSize);
				SetMiniFrameData(facing, i, data, dataSize);

	            uint16		width;
                uint16      height;
				char *shadowTif = StripTIF2Mem(shadowFiles[facing][i], &width, &height);

				if (shadowTif)
				{
					data = spriteutils_RGB32ToEncoded((Pixel32 *)shadowTif, m_width, m_height, &dataSize);
					SetShadowFrameData(facing, i, data, dataSize);

				    char *minishadow = nullptr;
					spriteutils_CreateQuarterSize((Pixel32 *)shadowTif, m_width, m_height, (Pixel32 **)&minishadow, FALSE);

					if(minishadow)
					{
						data = spriteutils_RGB32ToEncoded((Pixel32 *)minishadow, m_width >> 1, m_height >> 1, &dataSize);
						SetMiniShadowFrameData(facing, i, data, dataSize);
						}

					free(shadowTif);
					}

				free(tif);
			}

			printf(".");
		}
	}
}




void FacedSpriteWshadow::Import(uint16 nframes, char *imageFiles[k_NUM_FACINGS][k_MAX_NAMES])
{
    AllocateFrameArrays(nframes);

	for (sint32 facing=0; facing < k_NUM_FACINGS; facing++)
	{
		for (sint32 i=0; i < nframes; i++)
		{
			char *tif = StripTIF2Mem(imageFiles[facing][i], &m_width, &m_height);

			if (tif)
			{
				char *minitif = nullptr;
				spriteutils_CreateQuarterSize((Pixel32 *)tif, m_width, m_height, (Pixel32 **)&minitif, TRUE);

				size_t dataSize = 0;
				Pixel16 * data = spriteutils_RGB32ToEncoded((Pixel32 *)tif, m_width, m_height, &dataSize);
				SetFrameData(facing, i, data, dataSize);
				data = spriteutils_RGB32ToEncoded((Pixel32 *)minitif, m_width >> 1, m_height >> 1, &dataSize);
				SetMiniFrameData(facing, i, data, dataSize);

				free(tif);
			}

			printf(".");
		}
	}
}

void FacedSpriteWshadow::Draw(sint32 drawX, sint32 drawY, sint32 facing,
					   double scale, sint16 transparency, Pixel16 outlineColor, uint16 flags)
{
	SetSurface();

	if (facing < 5)
	{
		drawX -= (sint32)((double)m_hotPoints[facing].x * scale);
		drawY -= (sint32)((double)m_hotPoints[facing].y * scale);
	}
	else
	{
		drawX -=  (m_width - (sint32)((double)m_hotPoints[k_MAX_FACINGS - facing].x * scale));
		drawY -= (sint32)((double)m_hotPoints[k_MAX_FACINGS - facing].y * scale);
	}

	if(m_frames[facing][m_currentFrame] == nullptr)
	{
		facing = k_SPRITE_BASEFACING;
	}
	Assert(m_frames[facing][m_currentFrame] != nullptr);
	if(m_frames[facing][m_currentFrame] == nullptr)
	{
		c3errors_ErrorDialog("FacedSpriteWshadow", "base sprite facing is missing or invalid for Draw");
		return;
	}




	if (drawX > screenmanager_Get()->GetSurfWidth() - (m_width*scale) || drawX < 0) return;
	if (drawY > screenmanager_Get()->GetSurfHeight() - (m_height*scale) || drawY < 0) return;

	if (scale == 1.0)
	{
		if (facing < 5)
		{
			(this->*_DrawLow)((Pixel16 *)m_frames[facing][m_currentFrame], drawX, drawY, m_width, m_height, transparency, outlineColor, flags);

		}
		else
			(this->*_DrawLowReversed)((Pixel16 *)m_frames[k_MAX_FACINGS - facing][m_currentFrame], drawX, drawY, m_width, m_height, transparency, outlineColor, flags);
	}
	else
	{
		if (scale == 0.5)
		{
			if (facing < 5)
				(this->*_DrawLow)((Pixel16 *)m_miniframes[facing][m_currentFrame], drawX, drawY, m_width>>1, m_height>>1, transparency, outlineColor, flags);
			else
				(this->*_DrawLowReversed)((Pixel16 *)m_miniframes[k_MAX_FACINGS - facing][m_currentFrame], drawX, drawY, m_width>>1, m_height>>1, transparency, outlineColor, flags);
		}
		else
		{

			sint32 destWidth = (sint32)(m_width * scale);
			sint32 destHeight = (sint32)(m_height * scale);

			if (facing < 5)
			{
				(this->*_DrawScaledLow)((Pixel16 *)m_frames[facing][m_currentFrame], drawX, drawY, destWidth, destHeight,
									transparency, outlineColor, flags, FALSE);
			}
			else
			{
				(this->*_DrawScaledLow)((Pixel16 *)m_frames[k_MAX_FACINGS - facing][m_currentFrame], drawX, drawY, destWidth, destHeight,
									transparency, outlineColor, flags, TRUE);
			}
		}
	}
}

void FacedSpriteWshadow::DrawDirect(aui_Surface *surf, sint32 drawX, sint32 drawY, sint32 facing,
					   double scale, sint16 transparency, Pixel16 outlineColor, uint16 flags)
{
	LockSurface(surf);

	if (facing < 5)
	{
		drawX -= (sint32)((double)m_hotPoints[facing].x * scale);
		drawY -= (sint32)((double)m_hotPoints[facing].y * scale);
	}
	else
	{
		drawX -=  (m_width - (sint32)((double)m_hotPoints[k_MAX_FACINGS - facing].x * scale));
		drawY -= (sint32)((double)m_hotPoints[k_MAX_FACINGS - facing].y * scale);
	}

	if(m_frames[facing][m_currentFrame] == nullptr)
	{
		facing = k_SPRITE_BASEFACING;
	}
	Assert(m_frames[facing][m_currentFrame] != nullptr);
	if(m_frames[facing][m_currentFrame] == nullptr)
	{
		c3errors_ErrorDialog("FacedSpriteWshadow", "base sprite facing is missing or invalid for Draw");
		UnlockSurface();
		return;
	}




	if (drawX > surf->Width() - (m_width*scale) || drawX < 0) {
		UnlockSurface();
		return;
	}
	if (drawY > surf->Height() - (m_height*scale) || drawY < 0) {
		UnlockSurface();
		return;
	}

	if (scale == 1.0)
	{
		if (facing < 5)
		{
			(this->*_DrawLow)((Pixel16 *)m_frames[facing][m_currentFrame], drawX, drawY, m_width, m_height, transparency, outlineColor, flags);

		}
		else {
			(this->*_DrawLowReversed)((Pixel16 *)m_frames[k_MAX_FACINGS - facing][m_currentFrame], drawX, drawY, m_width, m_height, transparency, outlineColor, flags);
		}
	}
	else
	{
		if (scale == 0.5)
		{
			if (facing < 5)
				(this->*_DrawLow)((Pixel16 *)m_miniframes[facing][m_currentFrame], drawX, drawY, m_width>>1, m_height>>1, transparency, outlineColor, flags);
			else {
				(this->*_DrawLowReversed)((Pixel16 *)m_miniframes[k_MAX_FACINGS - facing][m_currentFrame], drawX, drawY, m_width>>1, m_height>>1, transparency, outlineColor, flags);
			}
		}
		else
		{

			sint32 destWidth = (sint32)(m_width * scale);
			sint32 destHeight = (sint32)(m_height * scale);

			if (facing < 5)
			{
				(this->*_DrawScaledLow)((Pixel16 *)m_frames[facing][m_currentFrame], drawX, drawY, destWidth, destHeight,
									transparency, outlineColor, flags, FALSE);
			}
			else
			{
				(this->*_DrawScaledLow)((Pixel16 *)m_frames[k_MAX_FACINGS - facing][m_currentFrame], drawX, drawY, destWidth, destHeight,
									transparency, outlineColor, flags, TRUE);
			}
		}
	}
	UnlockSurface();

}




void FacedSpriteWshadow::DirectionalDraw(sint32 drawX, sint32 drawY, sint32 facing,
					   double scale, sint16 transparency, Pixel16 outlineColor, uint16 flags)
{
	SetSurface();

	if (facing < 5)
	{
		drawX -= (sint32)((double)m_hotPoints[facing].x * scale);
		drawY -= (sint32)((double)m_hotPoints[facing].y * scale);
	}
	else
	{
		drawX -=  (m_width - (sint32)((double)m_hotPoints[k_MAX_FACINGS - facing].x * scale));
		drawY -= (sint32)((double)m_hotPoints[k_MAX_FACINGS - facing].y * scale);
	}

	if(m_frames[facing][m_currentFrame] == nullptr)
	{
		facing = k_SPRITE_BASEFACING;
	}

	Assert(m_frames[facing][m_currentFrame] != nullptr);
	if(m_frames[facing][m_currentFrame] == nullptr)
	{
		c3errors_ErrorDialog("FacedSpriteWshadow", "base sprite facing is missing or invalid for Directional Draw");
		return;
	}


	if (drawX > screenmanager_Get()->GetSurfWidth() - (m_width*scale) || drawX < 0) return;
	if (drawY > screenmanager_Get()->GetSurfHeight() - (m_height*scale) || drawY < 0) return;

	if (scale == 1.0)
	{
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
	}
	else
	{
		if (scale == 0.5)
		{
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
		}
		else
		{

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




void FacedSpriteWshadow::DrawShadow(sint32 drawX, sint32 drawY, sint32 facing,
					   double scale, sint16 transparency, Pixel16 outlineColor, uint16 flags)
{
	SetSurface();

	if (facing < 5)
	{
		drawX -= (sint32)((double)m_hotPoints[facing].x * scale);
		drawY -= (sint32)((double)m_hotPoints[facing].y * scale);
	}
	else
	{
		drawX -=  (m_width - (sint32)((double)m_hotPoints[k_MAX_FACINGS - facing].x * scale));
		drawY -= (sint32)((double)m_hotPoints[k_MAX_FACINGS - facing].y * scale);
	}

	if(m_shadowFrames[facing][m_currentFrame] == nullptr)
	{
		facing = k_SPRITE_BASEFACING;
	}
	if(m_shadowFrames[facing][m_currentFrame] == nullptr)
	{

		return;
	}


	if (drawX > screenmanager_Get()->GetSurfWidth() - (m_width*scale) || drawX < 0) return;
	if (drawY > screenmanager_Get()->GetSurfHeight() - (m_height*scale) || drawY < 0) return;

	if (scale == 1.0)
	{
		if (facing < 5)
		{
			(this->*_DrawLow)((Pixel16 *)m_shadowFrames[facing][m_currentFrame], drawX, drawY, m_width, m_height, transparency, outlineColor, flags);

		}
		else
			(this->*_DrawLowReversed)((Pixel16 *)m_shadowFrames[k_MAX_FACINGS - facing][m_currentFrame], drawX, drawY, m_width, m_height, transparency, outlineColor, flags);
	}
	else
	{
		if (scale == 0.5)
		{
			if (facing < 5)
				(this->*_DrawLow)((Pixel16 *)m_miniShadowFrames[facing][m_currentFrame], drawX, drawY, m_width>>1, m_height>>1, transparency, outlineColor, flags);
			else
				(this->*_DrawLowReversed)((Pixel16 *)m_miniShadowFrames[k_MAX_FACINGS - facing][m_currentFrame], drawX, drawY, m_width>>1, m_height>>1, transparency, outlineColor, flags);
		}
		else
		{

			sint32 destWidth = (sint32)(m_width * scale);
			sint32 destHeight = (sint32)(m_height * scale);

			if (facing < 5)
			{
				(this->*_DrawScaledLow)((Pixel16 *)m_shadowFrames[facing][m_currentFrame], drawX, drawY, destWidth, destHeight,
									transparency, outlineColor, flags, FALSE);
			}
			else
			{
				(this->*_DrawScaledLow)((Pixel16 *)m_shadowFrames[k_MAX_FACINGS - facing][m_currentFrame], drawX, drawY, destWidth, destHeight,
									transparency, outlineColor, flags, TRUE);
			}
		}
	}
}




void FacedSpriteWshadow::DrawFlashEffect(sint32 drawX, sint32 drawY, sint32 facing,
					   double scale, sint16 transparency, Pixel16 outlineColor, uint16 flags)
{
	SetSurface();

	if (facing < 5)
	{
		drawX -= (sint32)((double)m_hotPoints[facing].x * scale);
		drawY -= (sint32)((double)m_hotPoints[facing].y * scale);
	}
	else
	{
		drawX -=  (m_width - (sint32)((double)m_hotPoints[k_MAX_FACINGS - facing].x * scale));
		drawY -= (sint32)((double)m_hotPoints[k_MAX_FACINGS - facing].y * scale);
	}

	if(m_frames[facing][m_currentFrame] == nullptr)
	{
		facing = k_SPRITE_BASEFACING;
	}

	Assert(m_frames[facing][m_currentFrame] != nullptr);
	if(m_frames[facing][m_currentFrame] == nullptr)
	{
		c3errors_ErrorDialog("FacedSpriteWshadow", "base sprite facing is missing or invalid Draw Flash Effect");
		return;
	}


	if (drawX > screenmanager_Get()->GetSurfWidth() - (m_width*scale) || drawX < 0) return;
	if (drawY > screenmanager_Get()->GetSurfHeight() - (m_height*scale) || drawY < 0) return;

	if (scale == 1.0)
	{
		if (facing < 5)
		{
			(this->*_DrawFlashLow)((Pixel16 *)m_frames[facing][m_currentFrame], drawX, drawY, m_width, m_height, transparency, outlineColor, flags);
		}
		else
		{
			(this->*_DrawFlashLowReversed)((Pixel16 *)m_frames[k_MAX_FACINGS - facing][m_currentFrame], drawX, drawY, m_width, m_height, transparency, outlineColor, flags);
		}
	}
	else
	{
		if (scale == 0.5)
		{
			if (facing < 5)
			{
				(this->*_DrawFlashLow)((Pixel16 *)m_miniframes[facing][m_currentFrame], drawX, drawY, m_width>>1, m_height>>1, transparency, outlineColor, flags);
			}
			else
			{
				(this->*_DrawFlashLowReversed)((Pixel16 *)m_miniframes[k_MAX_FACINGS - facing][m_currentFrame], drawX, drawY, m_width>>1, m_height>>1, transparency, outlineColor, flags);
			}
		}
		else
		{

			sint32 destWidth = (sint32)(m_width * scale);
			sint32 destHeight = (sint32)(m_height * scale);

			if (facing < 5)
			{
				(this->*_DrawFlashScaledLow)((Pixel16 *)m_frames[facing][m_currentFrame], drawX, drawY, destWidth, destHeight,
					transparency, outlineColor, flags, FALSE);
			}
			else
			{
				(this->*_DrawFlashScaledLow)((Pixel16 *)m_frames[k_MAX_FACINGS - facing][m_currentFrame], drawX, drawY, destWidth, destHeight,
					transparency, outlineColor, flags, TRUE);
			}
		}
	}
}

Pixel16 * FacedSpriteWshadow::GetFrameData(uint16 facing, uint16 frame)
{
	Assert(facing < k_NUM_FACINGS);
	Assert(frame < m_shadowFrameCount);
	Assert(m_frames[facing] != nullptr);

	return m_frames[facing][frame];
}

size_t FacedSpriteWshadow::GetFrameDataSize(uint16 facing, uint16 frame)
{
	Assert(facing < k_NUM_FACINGS);
	Assert(frame < m_shadowFrameCount);
	Assert(m_frames[facing] != nullptr);
	Assert(m_framesSizes[facing] != nullptr);
#ifdef _WINDOWS
	Assert(m_framesSizes[facing][frame] == _msize(GetFrameData(facing, frame)));

	return _msize(GetFrameData(facing, frame));
#else
	return m_framesSizes[facing][frame];
#endif
}

Pixel16 * FacedSpriteWshadow::GetMiniFrameData(uint16 facing, uint16 frame)
{
	Assert(facing < k_NUM_FACINGS);
	Assert(frame < m_shadowFrameCount);
	Assert(m_miniframes[facing] != nullptr);

	return m_miniframes[facing][frame];
}

size_t FacedSpriteWshadow::GetMiniFrameDataSize(uint16 facing, uint16 frame)
{
	Assert(facing < k_NUM_FACINGS);
	Assert(frame < m_shadowFrameCount);
	Assert(m_miniframesSizes[facing] != nullptr);
#ifdef _WINDOWS
	Assert(m_miniframesSizes[facing][frame] == _msize(GetMiniFrameData(facing, frame)));

	return _msize(GetMiniFrameData(facing,frame));
#else
	return m_miniframesSizes[facing][frame];
#endif
}

void FacedSpriteWshadow::SetFrameData(uint16 facing, uint16 frame, Pixel16 *data, size_t size)
{
	Assert(facing < k_NUM_FACINGS);
	Assert(frame < m_shadowFrameCount);
	Assert(m_frames[facing] != nullptr);
	Assert(m_framesSizes[facing] != nullptr);
#ifdef _WINDOWS
	Assert((((data == NULL) && (size = 0)) || ((data != NULL) && (_msize(data) == size))));
#endif
	m_frames[facing][frame] = data;
	m_framesSizes[facing][frame] = size;
}

void FacedSpriteWshadow::SetMiniFrameData(uint16 facing, uint16 frame, Pixel16 *data, size_t size)
{
	Assert(facing < k_NUM_FACINGS);
	Assert(frame < m_shadowFrameCount);
	Assert(m_miniframes[facing] != nullptr);
	Assert(m_miniframesSizes[facing] != nullptr);
#ifdef _WINDOWS
	Assert((((data == NULL) && (size = 0)) || ((data != NULL) && (_msize(data) == size))));
#endif

	m_miniframes[facing][frame] = data;
	m_miniframesSizes[facing][frame] = size;
}

Pixel16 * FacedSpriteWshadow::GetShadowFrameData(uint16 facing, uint16 frame)
{
	Assert(facing < k_NUM_FACINGS);
	Assert(frame < m_shadowFrameCount);
	Assert(m_shadowFrames[facing] != nullptr);

	return m_shadowFrames[facing][frame];
}

size_t FacedSpriteWshadow::GetShadowFrameDataSize(uint16 facing, uint16 frame)
{
	Assert(facing < k_NUM_FACINGS);
	Assert(frame < m_shadowFrameCount);
	Assert(m_shadowFramesSizes[facing] != nullptr);
#ifdef _WINDOWS
	Assert(m_shadowFramesSizes[facing][frame] == _msize(GetShadowFrameData(facing, frame)));

	return _msize(GetShadowFrameData(facing, frame));
#else
	return m_shadowFramesSizes[facing][frame];
#endif
}

Pixel16 * FacedSpriteWshadow::GetMiniShadowFrameData(uint16 facing, uint16 frame)
{
	Assert(facing < k_NUM_FACINGS);
	Assert(frame < m_shadowFrameCount);
	Assert(m_miniShadowFrames[facing] != nullptr);

	return m_miniShadowFrames[facing][frame];
}

size_t FacedSpriteWshadow::GetMiniShadowFrameDataSize(uint16 facing, uint16 frame)
{
	Assert(facing < k_NUM_FACINGS);
	Assert(frame < m_shadowFrameCount);
	Assert(m_miniShadowFramesSizes[facing] != nullptr);
#ifdef _WINDOWS
	Assert(m_miniShadowFramesSizes[facing][frame] == _msize(GetMiniShadowFrameData(facing, frame)));

	return _msize(GetMiniShadowFrameData(facing, frame));
#else
	return m_miniShadowFramesSizes[facing][frame];
#endif
}

void FacedSpriteWshadow::SetShadowFrameData(uint16 facing, uint16 frame, Pixel16 *data, size_t size)
{
	Assert(facing < k_NUM_FACINGS);
	Assert(frame < m_shadowFrameCount);
	Assert(m_shadowFrames[facing] != nullptr);
	Assert(m_shadowFramesSizes[facing] != nullptr);
#ifdef _WINDOWS
	Assert((((data == NULL) && (size = 0)) || ((data != NULL) && (_msize(data) == size))));
#endif
	m_shadowFrames[facing][frame] = data;
	m_shadowFramesSizes[facing][frame] = size;
}

void FacedSpriteWshadow::SetMiniShadowFrameData(uint16 facing, uint16 frame, Pixel16 *data, size_t size)
{
	Assert(facing < k_NUM_FACINGS);
	Assert(frame < m_shadowFrameCount);
	Assert(m_miniShadowFrames[facing] != nullptr);
	Assert(m_miniShadowFramesSizes[facing] != nullptr);
#ifdef _WINDOWS
	Assert((((data == NULL) && (size = 0)) || ((data != NULL) && (_msize(data) == size))));
#endif
	m_miniShadowFrames[facing][frame] = data;
	m_miniShadowFramesSizes[facing][frame] = size;
}

void FacedSpriteWshadow::DirectionalDrawShadow(sint32 drawX, sint32 drawY, sint32 facing,
					   double scale, sint16 transparency, Pixel16 outlineColor, uint16 flags)
{
	SetSurface();

	if (facing < 5)
	{
		drawX -= (sint32)((double)m_hotPoints[facing].x * scale);
		drawY -= (sint32)((double)m_hotPoints[facing].y * scale);
	}
	else
	{
		drawX -=  (m_width - (sint32)((double)m_hotPoints[k_MAX_FACINGS - facing].x * scale));
		drawY -= (sint32)((double)m_hotPoints[k_MAX_FACINGS - facing].y * scale);
	}

	if(m_shadowFrames[facing][m_currentFrame] == nullptr)
	{
		facing = k_SPRITE_BASEFACING;
	}

	if(m_shadowFrames[facing][m_currentFrame] == nullptr)
	{
		return;
	}


	if (drawX > screenmanager_Get()->GetSurfWidth() - (m_width*scale) || drawX < 0) return;
	if (drawY > screenmanager_Get()->GetSurfHeight() - (m_height*scale) || drawY < 0) return;

	if (scale == 1.0)
	{
		if (facing < 4 && facing > 0)
		{
			(this->*_DrawLow)((Pixel16 *)m_shadowFrames[facing][m_currentFrame], drawX, drawY, m_width, m_height, transparency, outlineColor, flags);

		}
		else if (facing == 4 || facing == 0)
		{
			(this->*_DrawLowReversed)((Pixel16 *)m_shadowFrames[facing][m_currentFrame], drawX, drawY, m_width, m_height, transparency, outlineColor, flags);
		}
		else
		{
			(this->*_DrawLowReversed)((Pixel16 *)m_shadowFrames[k_MAX_FACINGS - facing][m_currentFrame], drawX, drawY, m_width, m_height, transparency, outlineColor, flags);
		}
	}
	else
	{
		if (scale == 0.5)
		{
			if (facing < 4 && facing > 0)
			{
				(this->*_DrawLow)((Pixel16 *)m_miniShadowFrames[facing][m_currentFrame], drawX, drawY, m_width>>1, m_height>>1, transparency, outlineColor, flags);
			}
			else if (facing == 4 || facing == 0)
			{
				(this->*_DrawLowReversed)((Pixel16 *)m_miniShadowFrames[facing][m_currentFrame], drawX, drawY, m_width>>1, m_height>>1, transparency, outlineColor, flags);
			}
			else
			{
				(this->*_DrawLowReversed)((Pixel16 *)m_miniShadowFrames[facing][m_currentFrame], drawX, drawY, m_width>>1, m_height>>1, transparency, outlineColor, flags);
			}
		}
		else
		{

			sint32 destWidth = (sint32)(m_width * scale);
			sint32 destHeight = (sint32)(m_height * scale);

			if (facing < 4 && facing > 0)
			{
				(this->*_DrawScaledLow)((Pixel16 *)m_shadowFrames[facing][m_currentFrame], drawX, drawY, destWidth, destHeight,
									transparency, outlineColor, flags, FALSE);
			}
			else if(facing == 4 || facing == 0)
			{
				(this->*_DrawScaledLow)((Pixel16 *)m_shadowFrames[facing][m_currentFrame], drawX, drawY, destWidth, destHeight,
									transparency, outlineColor, flags, TRUE);
			}
			else
			{
				(this->*_DrawScaledLow)((Pixel16 *)m_shadowFrames[k_MAX_FACINGS - facing][m_currentFrame], drawX, drawY, destWidth, destHeight,
									transparency, outlineColor, flags, TRUE);
			}
		}
	}
}


sint32 FacedSpriteWshadow::ParseFromTokens(Token *theToken)
{
	sint32		tmp;
	sint32		i;

	if (!token_ParseAnOpenBraceNext(theToken)) return FALSE;

	if (!token_ParseValNext(theToken, TOKEN_SPRITE_NUM_FRAMES, tmp)) return FALSE;
	m_numFrames = (uint16)tmp;

	if (!token_ParseValNext(theToken, TOKEN_SPRITE_FIRST_FRAME, tmp)) return FALSE;
	m_firstFrame = (uint16)tmp;

	if (!token_ParseValNext(theToken, TOKEN_SPRITE_WIDTH, tmp)) return FALSE;
	m_width = (uint16)tmp;

	if (!token_ParseValNext(theToken, TOKEN_SPRITE_HEIGHT, tmp)) return FALSE;
	m_height = (uint16)tmp;

	if (!token_ParseKeywordNext(theToken, TOKEN_SPRITE_HOT_POINTS)) return FALSE;
	for (i=0; i<k_NUM_FACINGS; i++)
	{
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
// Name       : FacedSpriteWshadow::AllocateFrameArrays
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
void FacedSpriteWshadow::AllocateFrameArrays(size_t count)
{
    // Two-phase sprite loads (Basic then Full) reuse the same object.  Free
    // any previously allocated buffers + arrays before reallocating.  The old
    // Assert was a no-op in release builds and silently leaked.
    for (size_t facing = 0; facing < k_NUM_FACINGS; ++facing)
    {
        for (size_t i = 0; i < m_shadowFrameCount; ++i)
        {
            if (m_frames[facing])           delete m_frames[facing][i];
            if (m_miniframes[facing])       delete m_miniframes[facing][i];
            if (m_shadowFrames[facing])     delete m_shadowFrames[facing][i];
            if (m_miniShadowFrames[facing]) delete m_miniShadowFrames[facing][i];
        }
        delete [] m_frames[facing];                m_frames[facing] = nullptr;
        delete [] m_framesSizes[facing];           m_framesSizes[facing] = nullptr;
        delete [] m_miniframes[facing];            m_miniframes[facing] = nullptr;
        delete [] m_miniframesSizes[facing];       m_miniframesSizes[facing] = nullptr;
        delete [] m_shadowFrames[facing];          m_shadowFrames[facing] = nullptr;
        delete [] m_shadowFramesSizes[facing];     m_shadowFramesSizes[facing] = nullptr;
        delete [] m_miniShadowFrames[facing];      m_miniShadowFrames[facing] = nullptr;
        delete [] m_miniShadowFramesSizes[facing]; m_miniShadowFramesSizes[facing] = nullptr;
    }
    m_shadowFrameCount = 0;

	for (size_t facing = 0; facing < k_NUM_FACINGS; ++facing)
	{
		m_frames[facing]                = new Pixel16 * [count];
		m_framesSizes[facing]           = new size_t    [count];
		m_miniframes[facing]            = new Pixel16 * [count];
		m_miniframesSizes[facing]       = new size_t    [count];
		m_shadowFrames[facing]          = new Pixel16 * [count];
		m_shadowFramesSizes[facing]     = new size_t    [count];
		m_miniShadowFrames[facing]      = new Pixel16 * [count];
		m_miniShadowFramesSizes[facing] = new size_t    [count];

		for (size_t i = 0; i < count; ++i)
		{
			m_frames[facing][i]                = nullptr;
			m_framesSizes[facing][i]           = 0;
			m_miniframes[facing][i]            = nullptr;
			m_miniframesSizes[facing][i]       = 0;
			m_shadowFrames[facing][i]          = nullptr;
			m_shadowFramesSizes[facing][i]     = 0;
			m_miniShadowFrames[facing][i]      = nullptr;
			m_miniShadowFramesSizes[facing][i] = 0;
		}
	}

	m_shadowFrameCount  = count;
}
