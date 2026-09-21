//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Sprite file handling
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
// __MAKESPR__
// __SPRITETEST__
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Crash prevention, small clean-ups.
// - Removed unused local variables. (Sep 9th 2005 Martin G�hmann)
// - Fixed crashes when zooming out and exiting the program.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gfx/spritesys/SpriteFile.h"

#include <memory>
#include <vector>

#include "gfx/gfx_utils/pixelutils.h"
#include "gfx/gfx_utils/tiffutils.h"
#include "gfx/spritesys/spriteutils.h"
#include "ctp/ctp2_utils/c3files.h"
#include "ctp/ctp2_utils/c3errors.h"

#include <array>
#include <filesystem>
#include <stdexcept>
#include "gfx/spritesys/Sprite.h"
#include "gfx/spritesys/FacedSprite.h"
#include "gfx/spritesys/FacedSpriteWshadow.h"
#include "gfx/spritesys/SpriteGroup.h"
#include "gfx/spritesys/UnitSpriteGroup.h"
#include "gfx/spritesys/GoodSpriteGroup.h"
#include "gfx/spritesys/CitySpriteGroup.h"

#include "gfx/spritesys/EffectSpriteGroup.h"
#include "gfx/spritesys/Anim.h"
#include "gs/fileio/CivPaths.h"               // civpaths_Get()
#include "gs/database/profileDB.h"              // profiledb_Get()

#ifdef __MAKESPR__
unsigned char g_compression_buff[COM_BUFF_SIZE];
#else
std::unique_ptr<unsigned char[]> g_compression_buff;
#endif

SpriteFile::SpriteFile(MBCHAR const * name)
:
    m_version           (k_SPRITEFILE_VERSION0),
	m_spr_compression   (SPRDATA_REGULAR),
	m_file              (nullptr)
{
	strlcpy(m_filename, name, sizeof(m_filename));
}

SpriteFile::~SpriteFile()
{
    if (m_file)
    {
        c3files_fclose(m_file);
    }
}

void SpriteFile::WriteSpriteData(Sprite *s)
{
	WriteData(static_cast<uint16>(s->GetType()));
	WriteData(static_cast<uint16>(s->GetWidth()));
	WriteData(static_cast<uint16>(s->GetHeight()));
	WriteData(static_cast<sint32>(s->GetHotPoint().x));
	WriteData(static_cast<sint32>(s->GetHotPoint().y));
	WriteData(static_cast<uint16>(s->GetFirstFrame()));
	WriteData(static_cast<uint16>(s->GetNumFrames()));

	long      frame_offset_pos = GetFilePos();

	if (s->GetNumFrames() > 800) {
		Assert(s->GetNumFrames() <= 800);
		return;
	}

	uint32		normal_ssizes[800];
	uint32		normal_msizes[800];
	uint16		i;
	for (i=0; i<s->GetNumFrames(); i++)
	{
	    normal_ssizes[i] = s->GetFrameDataSize(i);
	    normal_msizes[i] = s->GetMiniFrameDataSize(i);
	}

    WriteData((uint8 *)normal_ssizes,s->GetNumFrames()*sizeof(uint32));
    WriteData((uint8 *)normal_msizes,s->GetNumFrames()*sizeof(uint32));

	uint32		compressed_ssizes[800];
	for (i=0; i<s->GetNumFrames(); i++)
	{
		spriteutils_ConvertPixelFormatForFile(s->GetFrameData(i), s->GetWidth(), s->GetHeight(), s->GetFrameDataSize(i));

		size_t          size            = s->GetFrameDataSize(i);
		std::unique_ptr<uint8[]> CompressedData(CompressData(s->GetFrameData(i), size));
        size_t const    compressed_size = size;

	    if (m_version>k_SPRITEFILE_VERSION1)
		{
			size += sizeof(uint32);
			WriteData((uint32)normal_ssizes[i]);
		}

		WriteData(CompressedData.get(), compressed_size);
		compressed_ssizes[i] = compressed_size;

		// CompressedData is a unique_ptr, auto-freed
	}

	if(m_version>k_SPRITEFILE_VERSION1)
	{
	    long const    current_pos = GetFilePos();
	    SetFilePos(frame_offset_pos);
	    WriteData((uint8 *)compressed_ssizes,s->GetNumFrames()*sizeof(uint32));
	    SetFilePos(current_pos);
	}

	for (i=0; i<s->GetNumFrames(); i++)
	{
		spriteutils_ConvertPixelFormatForFile
            (s->GetMiniFrameData(i), s->GetWidth()/2, s->GetHeight()/2, s->GetMiniFrameDataSize(i));
		WriteData((uint8 *)s->GetMiniFrameData(i), s->GetMiniFrameDataSize(i));
	}
}

void SpriteFile::WriteFacedSpriteData(FacedSprite *s)
{
	uint16      num_frames = static_cast<uint16>(s->GetNumFrames());

	WriteData(static_cast<uint16>(s->GetType()));
	WriteData(static_cast<uint16>(s->GetWidth()));
	WriteData(static_cast<uint16>(s->GetHeight()));
	WriteData((uint8 *)s->GetHotPoints(), sizeof(POINT) * k_NUM_FACINGS);
	WriteData(static_cast<uint16>(s->GetFirstFrame()));
	WriteData(static_cast<uint16>(num_frames));

	long  		frame_offsets[k_MAX_FACINGS];
	uint32		normal_ssizes[k_MAX_FACINGS][128];
	uint32		normal_msizes[k_MAX_FACINGS][128];
	uint16	 i;
	uint16	 j;

	for (j=0; j<k_NUM_FACINGS; j++)
	{
		frame_offsets[j] = GetFilePos();

		for (i=0; i<num_frames; i++)
		{
			normal_ssizes[j][i] = s->GetFrameDataSize(j, i);
		    normal_msizes[j][i] = s->GetMiniFrameDataSize(j, i);
		}

		WriteData((uint8 *)normal_ssizes[j],sizeof(uint32) * s->GetNumFrames());
		WriteData((uint8 *)normal_msizes[j],sizeof(uint32) * s->GetNumFrames());
	}

	uint32		compressed_ssizes[k_MAX_FACINGS][128];
	for (j=0; j<k_NUM_FACINGS; j++)
	{
		for (i=0; i<num_frames; i++)
		{
			spriteutils_ConvertPixelFormatForFile(s->GetFrameData(j,i),
												s->GetWidth(), s->GetHeight(), normal_ssizes[j][i]);

			size_t  size            = normal_ssizes[j][i];
		    std::unique_ptr<uint8[]> CompressedData(CompressData(s->GetFrameData(j,i),size));
			size_t  compressed_size = size;

			if(m_version>k_SPRITEFILE_VERSION1)
			   WriteData((uint32)normal_ssizes[j][i]);

			WriteData(CompressedData.get(), compressed_size);

		    compressed_ssizes[j][i] = size;

			// CompressedData is a unique_ptr, auto-freed
		}

		for (i=0; i<num_frames; i++) {

			spriteutils_ConvertPixelFormatForFile(s->GetMiniFrameData(j,i),
												s->GetWidth()/2, s->GetHeight()/2, normal_msizes[j][i]);
			WriteData((uint8 *)s->GetMiniFrameData(j,i), normal_msizes[j][i]);
		}
	}

	if (m_version>k_SPRITEFILE_VERSION1)
	{
        long const    current_pos = GetFilePos();

        for (j=0; j<k_NUM_FACINGS; j++)
        {
            SetFilePos(frame_offsets[j]);
            WriteData((uint8 *)compressed_ssizes[j],num_frames*sizeof(uint32));
        }

        SetFilePos(current_pos);
	}
}

void SpriteFile::WriteFacedSpriteWshadowData(FacedSpriteWshadow *s)
{
	WriteData(static_cast<uint16>(s->GetType()));
	WriteData(static_cast<uint16>(s->GetWidth()));
	WriteData(static_cast<uint16>(s->GetHeight()));
	WriteData((uint8 *)s->GetHotPoints(), sizeof(POINT) * k_NUM_FACINGS);
	WriteData(static_cast<uint16>(s->GetFirstFrame()));
	WriteData(static_cast<uint16>(s->GetNumFrames()));
	WriteData(static_cast<uint16>(s->GetHasShadow()));

	uint16	 i;
	uint16	 j;

	for (j=0; j<k_NUM_FACINGS; j++)
	{
		for (i=0; i<s->GetNumFrames(); i++)
		{
			size_t size;
			if(s->GetFrameData(j, i) == nullptr)
			{
				size = 0;
			}
			else
			{
				size = s->GetFrameDataSize(j, i);
			}
			WriteData((uint32)size);
		}


		for (i=0; i<s->GetNumFrames(); i++)
		{
			size_t size;
			if(s->GetMiniFrameData(j, i) == nullptr)
			{
				size = 0;
			}
			else
			{
				size = s->GetMiniFrameDataSize(j, i);
			}
			WriteData((uint32)size);
		}

		if(s->GetHasShadow())
		{

			for (i=0; i<s->GetNumFrames(); i++)
			{
				size_t size;
				if(s->GetShadowFrameData(j, i) == nullptr)
				{
					size = 0;
				}
				else
				{
					size = s->GetShadowFrameDataSize(j, i);
				}
				WriteData((uint32)size);
			}

			for (i=0; i<s->GetNumFrames(); i++)
			{
				size_t size;
					if(s->GetMiniShadowFrameData(j, i) == nullptr)
					{
						size = 0;
					}
					else
					{
						size = s->GetMiniShadowFrameDataSize(j, i);
					}
				WriteData((uint32)size);
			}
		}
	}

	for (j=0; j<k_NUM_FACINGS; j++)
	{

		for (i=0; i<s->GetNumFrames(); i++)
		{
			size_t size;
			if(s->GetFrameData(j, i) != nullptr)
			{
				size = s->GetFrameDataSize(j, i);
				WriteData((uint8 *)s->GetFrameData(j,i), size);
			}
		}

		for (i=0; i<s->GetNumFrames(); i++)
		{
			size_t size;
			if(s->GetMiniFrameData(j, i) != nullptr)
			{
				size = s->GetMiniFrameDataSize(j, i);
				WriteData((uint8 *)s->GetMiniFrameData(j,i), size);
			}
		}
		if(s->GetHasShadow())
		{

			for (i=0; i<s->GetNumFrames(); i++)
			{
				if (s->GetShadowFrameData(j, i) != nullptr)
				{
					WriteData((uint8 *)s->GetShadowFrameData(j,i),
                              s->GetShadowFrameDataSize(j, i)
                             );
				}
			}

			for (i=0; i<s->GetNumFrames(); i++)
			{
				if (s->GetMiniShadowFrameData(j, i) != nullptr)
				{
					WriteData((uint8 *)s->GetMiniShadowFrameData(j,i),
                              s->GetMiniShadowFrameDataSize(j, i)
                             );
				}
			}
		}
	}
}

void SpriteFile::WriteAnimData(Anim *a)
{
	WriteData(static_cast<uint16>(a->GetType()));
	WriteData(static_cast<uint16>(a->GetNumFrames()));
	WriteData(a->GetPlaybackTime());
	WriteData(a->GetDelay());
	WriteData((uint8 *)a->GetFrames(), sizeof(uint16) * a->GetNumFrames());
	WriteData((uint8 *)a->GetDeltas(), sizeof(POINT) * a->GetNumFrames());
	WriteData((uint8 *)a->GetTransparencies(), sizeof(uint16) * a->GetNumFrames());
}




namespace {
constexpr size_t kMaxSpriteBytes = 128 * 1024 * 1024;
constexpr size_t kMaxFrameBytes = 64 * 1024 * 1024;
constexpr uint16 kMaxSpriteFrames = 800; // Legacy size-table capacity.

void RequireSprite(bool condition, char const *message)
{
    if (!condition) throw std::runtime_error(message);
}

void ValidateFrame(Pixel16 const *data, size_t bytes, int width, int height)
{
    size_t words = bytes / sizeof(Pixel16);
    RequireSprite(bytes % sizeof(Pixel16) == 0 && words >= size_t(height) + 1,
                  "truncated sprite row table");
    for (int y = 0; y < height; ++y) {
        auto offset = data[1 + y];
        if (offset == k_EMPTY_TABLE_ENTRY) continue;
        size_t pos = 1 + height + size_t(offset);
        RequireSprite(pos < words, "sprite row offset outside frame");
        auto tag = data[pos++] & 0x0fff;
        int x = 0;
        while ((tag & 0xf000) == 0) {
            auto op = (tag >> 8) & 0xf;
            size_t length = tag & 0xff;
            size_t payload = 0;
            switch (op) {
                case k_CHROMAKEY_RUN_ID: case k_SHADOW_RUN_ID: x += length; break;
                case k_COPY_RUN_ID: x += length; payload = length; break;
                case k_FEATHERED_RUN_ID: ++x; payload = 1; break;
                default: throw std::runtime_error("invalid sprite run opcode");
            }
            RequireSprite(x <= width && payload < words - pos,
                          "sprite run exceeds row or frame");
            pos += payload;
            tag = data[pos++];
        }
    }
}
}

Pixel16 *SpriteFile::ReadFrame(int width, int height, uint32 size, uint32 &actual, bool compressed)
{
    actual = size;
    if (compressed) ReadData(&actual, sizeof(actual));
    RequireSprite(actual <= kMaxFrameBytes && actual % sizeof(Pixel16) == 0,
                  "invalid decoded sprite frame size");
    RequireSprite(size <= kMaxFrameBytes, "sprite frame is too large");
    CheckReadSize(size);
    RequireSprite(m_decodedBytes <= kMaxSpriteBytes - actual, "decoded sprite exceeds size limit");
    m_decodedBytes += actual;
    if (size == 0 && actual == 0) return nullptr;
    auto pixels = std::make_unique<Pixel16[]>(actual / sizeof(Pixel16));
    if (compressed) {
        std::vector<uint8> input(size);
        ReadData(input.data(), size);
        std::unique_ptr<uint8[]> decoded(DeCompressData(input.data(), size, actual));
        std::memcpy(pixels.get(), decoded.get(), actual);
    } else {
        ReadData(pixels.get(), size);
    }
    ValidateFrame(pixels.get(), actual, width, height);
    spriteutils_ConvertPixelFormat(pixels.get(), width, height, actual);
    return pixels.release();
}

void SpriteFile::ReadFrames(Sprite *s, bool faced, bool shadow, bool basic, bool skip)
{
    uint16 width, height, first, frames;
    ReadData(&width, sizeof(width));
    ReadData(&height, sizeof(height));
    RequireSprite(width > 0 && height > 0 && width <= 8192 && height <= 8192,
                  "invalid sprite dimensions");
    s->SetWidth(width);
    s->SetHeight(height);
    auto *f = faced && !shadow ? static_cast<FacedSprite *>(s) : nullptr;
    auto *sh = shadow ? static_cast<FacedSpriteWshadow *>(s) : nullptr;
    if (faced) {
        POINT points[k_NUM_FACINGS];
        ReadData(points, sizeof(points));
        if (shadow) sh->SetHotPoints(points); else f->SetHotPoints(points);
    } else {
        sint32 x, y;
        ReadData(&x, sizeof(x));
        ReadData(&y, sizeof(y));
        s->SetHotPoint(x, y);
    }
    ReadData(&first, sizeof(first));
    ReadData(&frames, sizeof(frames));
    RequireSprite(frames > 0 && frames <= kMaxSpriteFrames, "invalid sprite frame count");
    s->SetFirstFrame(first);
    if (!skip) s->AllocateFrameArrays(basic ? 1 : frames);
    uint16 hasShadow = 0;
    if (shadow) {
        ReadData(&hasShadow, sizeof(hasShadow));
        RequireSprite(hasShadow <= 1, "invalid sprite shadow flag");
        sh->SetHasShadow(hasShadow);
    }
    int facings = faced ? k_NUM_FACINGS : 1;
    int layers = hasShadow ? 4 : 2;
    // Tables and payloads are interleaved by facing, then normal/mini/shadow.
    std::array<std::array<std::array<uint32, kMaxSpriteFrames>, 4>, k_NUM_FACINGS> sizes{};
    for (int face = 0; face < facings; ++face)
        for (int layer = 0; layer < layers; ++layer)
            ReadData(sizes[face][layer].data(), sizeof(uint32) * frames);
    for (int face = 0; face < facings; ++face) {
        for (int layer = 0; layer < layers; ++layer) {
            for (uint16 frame = 0; frame < frames; ++frame) {
                uint32 size = sizes[face][layer][frame];
                bool compressed = !shadow && layer == 0 && m_version > k_SPRITEFILE_VERSION1;
                if (skip || (basic && frame != 0)) {
                    RequireSprite(size <= kMaxFrameBytes, "sprite frame is too large");
                    SetFilePos(GetFilePos() + size + (compressed ? sizeof(uint32) : 0));
                    continue;
                }
                uint32 actual;
                bool mini = layer % 2 != 0;
                Pixel16 *data = ReadFrame(mini ? width / 2 : width, mini ? height / 2 : height,
                                          size, actual, compressed);
                if (shadow) {
                    switch (layer) {
                        case 0: sh->SetFrameData(face, frame, data, actual); break;
                        case 1: sh->SetMiniFrameData(face, frame, data, actual); break;
                        case 2: sh->SetShadowFrameData(face, frame, data, actual); break;
                        case 3: sh->SetMiniShadowFrameData(face, frame, data, actual); break;
                    }
                } else if (faced) {
                    if (mini) f->SetMiniFrameData(face, frame, data, actual);
                    else f->SetFrameData(face, frame, data, actual);
                } else {
                    if (mini) s->SetMiniFrameData(frame, data, actual);
                    else s->SetFrameData(frame, data, actual);
                }
            }
        }
    }
}

void SpriteFile::ReadSpriteDataBasic(Sprite *s) { ReadFrames(s, false, false, true); }
void SpriteFile::ReadSpriteDataFull(Sprite *s) { ReadFrames(s, false, false, false); }
void SpriteFile::SkipSpriteData() { Sprite s; ReadFrames(&s, false, false, false, true); }
void SpriteFile::ReadFacedSpriteDataBasic(FacedSprite *s) { ReadFrames(s, true, false, true); }
void SpriteFile::ReadFacedSpriteDataFull(FacedSprite *s) { ReadFrames(s, true, false, false); }
void SpriteFile::SkipFacedSpriteData() { FacedSprite s; ReadFrames(&s, true, false, false, true); }
void SpriteFile::ReadFacedSpriteWshadowData(FacedSpriteWshadow *s) { ReadFrames(s, true, true, false); }

void SpriteFile::ReadSpriteDataGeneralBasic(Sprite **sprite) { ReadGeneral(sprite, true); }
void SpriteFile::ReadSpriteDataGeneralFull(Sprite **sprite) { ReadGeneral(sprite, false); }

void SpriteFile::ReadGeneral(Sprite **sprite, bool basic)
{
    uint16 type;
    ReadData(&type, sizeof(type));
    RequireSprite(type == SPRITETYPE_NORMAL || type == SPRITETYPE_FACED, "invalid sprite type");
    std::unique_ptr<Sprite> parsed = type == SPRITETYPE_NORMAL
        ? std::unique_ptr<Sprite>(std::make_unique<Sprite>())
        : std::unique_ptr<Sprite>(std::make_unique<FacedSprite>());
    parsed->SetType(type);
    ReadFrames(parsed.get(), type == SPRITETYPE_FACED, false, basic);
    *sprite = parsed.release(); // Group's setter releases the prior owned sprite.
}

void SpriteFile::SkipSpriteDataGeneral()
{
	uint16		data16;
	ReadData(&data16, sizeof(data16));

	if ((SPRITETYPE)data16 == SPRITETYPE_NORMAL)
	{
		SkipSpriteData();
	}
	else if ((SPRITETYPE)data16 == SPRITETYPE_FACED)
	{
		SkipFacedSpriteData();
	}
	else
		throw std::runtime_error("invalid sprite type");

}

void SpriteFile::ReadSpriteDataGeneral(FacedSpriteWshadow **sprite)
{
    uint16 type;
    ReadData(&type, sizeof(type));
    RequireSprite(type == SPRITETYPE_FACEDWSHADOW, "invalid shadow sprite type");
    auto parsed = std::make_unique<FacedSpriteWshadow>();
    parsed->SetType(type);
    ReadFacedSpriteWshadowData(parsed.get());
    *sprite = parsed.release();
}

void SpriteFile::ReadAnimDataBasic(Anim *a)
{
	ReadAnimDataFull(a);

	a->SetPlaybackTime(a->GetPlaybackTime() / a->GetNumFrames());
	a->SetNumFrames(1);
	a->ResizeFrames(1);
	uint16 *    u = a->GetFrames();
	u[0] = 0;
}

void SpriteFile::ReadAnimDataFull(Anim *a)
{
	uint16		data16;
	ReadData(&data16, sizeof(data16));
	a->SetType(data16);

	ReadData(&data16, sizeof(data16));
	a->SetNumFrames(data16);

	ReadData(&data16, sizeof(data16));
	a->SetPlaybackTime(data16);

	ReadData(&data16, sizeof(data16));
	a->SetDelay(data16);

    uint16 numFrames = a->GetNumFrames();
    RequireSprite(numFrames > 0 && numFrames <= kMaxSpriteFrames, "invalid animation frame count");

	// Always (re)allocate buffers sized for the just-read numFrames.
	// Why: the Anim may have been partially populated by an earlier
	// ReadAnimDataBasic call whose numFrames differed from this one's.
	// Reusing the old buffer caused a heap-buffer-overflow in fread.
	a->ResizeFrames(numFrames);
	ReadData(a->GetFrames(), sizeof(uint16) * numFrames);

	a->ResizeDeltas(numFrames);
	ReadData(a->GetDeltas(), sizeof(POINT) * numFrames);

	a->ResizeTransparencies(numFrames);
	ReadData(a->GetTransparencies(), sizeof(uint16) * numFrames);
}

void SpriteFile::SkipAnimData()
{
	uint16		data16;
	ReadData(&data16, sizeof(data16));

	uint16		numFrames;
	ReadData(&numFrames, sizeof(numFrames));
	RequireSprite(numFrames > 0 && numFrames <= kMaxSpriteFrames, "invalid animation frame count");

	ReadData(&data16, sizeof(data16));
	ReadData(&data16, sizeof(data16));

	SetFilePos(GetFilePos() + sizeof(uint16) * numFrames);
	SetFilePos(GetFilePos() + sizeof(POINT) * numFrames);
	SetFilePos(GetFilePos() + sizeof(uint16) * numFrames);
}

SPRITEFILEERR SpriteFile::Create(SPRITEFILETYPE type,unsigned version,unsigned compression_mode)
{
    m_reading = false;
	MBCHAR			path[_MAX_PATH];

#if defined(__MAKESPR__) || defined(__SPRITETEST__)
	strlcpy(path, m_filename, sizeof(path));
#else
	MBCHAR fullPath[_MAX_PATH];
	civpaths_Get()->GetSpecificPath(C3DIR_SPRITES, fullPath, FALSE);
	snprintf(path, sizeof(path), "%s%s%s", fullPath, FILE_SEP, m_filename);
#endif

    if (m_file)
    {
        c3files_fclose(m_file);
    }
	m_file = c3files_fopen(C3DIR_DIRECT, path, "wb");
	Assert(m_file != nullptr);

	if (m_file == nullptr)
		return SPRITEFILEERR_NOCREATE;

	SPRITEFILEERR	err     = WriteData(static_cast<uint32>(k_SPRITEFILE_TAG));
	Assert(err == SPRITEFILEERR_OK);

	m_version = version;
	err  = WriteData(static_cast<uint32>(version));
	Assert(err == SPRITEFILEERR_OK);

	m_spr_compression = compression_mode;

	if (m_version>k_SPRITEFILE_VERSION1)
	{
		err  = WriteData(static_cast<uint32>(m_spr_compression));
		Assert(err == SPRITEFILEERR_OK);
	}

	err = WriteData(static_cast<uint32>(type));
	Assert(err == SPRITEFILEERR_OK);

	return SPRITEFILEERR_OK;
}

SPRITEFILEERR SpriteFile::Write(Sprite *s, Anim *anim)
{

	WriteSpriteData(s);
	WriteAnimData(anim);

	return SPRITEFILEERR_OK;
}

SPRITEFILEERR SpriteFile::Write(FacedSprite *s, Anim *anim)
{

	WriteFacedSpriteData(s);
	WriteAnimData(anim);

	return SPRITEFILEERR_OK;
}

SPRITEFILEERR SpriteFile::Write(FacedSpriteWshadow *s, Anim *anim)
{

	WriteFacedSpriteWshadowData(s);
	WriteAnimData(anim);

	return SPRITEFILEERR_OK;
}

SPRITEFILEERR SpriteFile::Write(SpriteGroup *s, Anim *anim)
{
	return SPRITEFILEERR_OK;
}




SPRITEFILEERR
SpriteFile::Write_v13(UnitSpriteGroup *s)
{
	long const    start_of_offsets = GetFilePos();

	uint16		i;
	for (i=0; i<UNITACTION_MAX; i++)
		WriteData((uint32)0);

	uint32		offset[UNITACTION_MAX];
	for (i=0; i<UNITACTION_MAX; i++)
	{
		Sprite	* sprite = s->GetGroupSprite((GAME_ACTION) i);

		if (sprite)
		{
			WriteData((uint32)TRUE);

			offset[i] = static_cast<size_t>(GetFilePos());

			switch(sprite->GetType())
			{
			case	SPRITETYPE_NORMAL:
					WriteSpriteData(sprite);
					break;
			case	SPRITETYPE_FACED:
					WriteFacedSpriteData((FacedSprite *)sprite);
					break;
			default:
					c3errors_ErrorDialog("SpriteFile", "\"%s\": Bad Sprite Type %d at index %d.",
										 m_filename,sprite->GetType(),i);
			}
			WriteAnimData(s->GetGroupAnim((UNITACTION)i));
		}
		else
		{
			WriteData((uint32)FALSE);
			offset[i] = static_cast<uint32>(-1);
		}
	}

	WriteData((uint16)0);
    WriteData((uint16)0);

	POINT		NoData;
	for(i=0;i<k_NUM_FACINGS;i++)
		WriteData((uint8 *)&NoData, sizeof(POINT));

	for (i=0; i<UNITACTION_MAX; i++)
		WriteData((uint8 *)s->GetShieldPoints((UNITACTION)i), sizeof(POINT) * k_NUM_FACINGS);

	WriteData((uint16)s->HasDeath());
	WriteData((uint16)s->HasDirectional());

	SetFilePos(start_of_offsets);

	for (i=0; i<UNITACTION_MAX; i++)
		WriteData(offset[i]);

	return SPRITEFILEERR_OK;
}




SPRITEFILEERR
SpriteFile::Write_v20(UnitSpriteGroup *s)
{
	long const    start_of_offsets = GetFilePos();

	int  			offset[ACTION_MAX+1];
	std::fill(offset, offset + ACTION_MAX + 1, -1);

	WriteData((uint8*)offset, sizeof(int) * (ACTION_MAX+1));

	size_t			i;
	for (i=0;i<ACTION_MAX; i++)
	{
		Sprite *    sprite = s->GetGroupSprite((GAME_ACTION)i);

		if (sprite)
		{
			offset[i] = static_cast<size_t>(GetFilePos());

			switch(sprite->GetType())
			{
			case	SPRITETYPE_NORMAL:
					WriteSpriteData(sprite);
					break;
			case	SPRITETYPE_FACED:
					WriteFacedSpriteData((FacedSprite *)sprite);
					break;
			default:
				c3errors_ErrorDialog("SpriteFile", "\"%s\": Bad Sprite Type %d at index %zu.",
									 m_filename,sprite->GetType(),i);
			}

			WriteAnimData(s->GetGroupAnim((UNITACTION)i));
		}
	}

	offset[ACTION_MAX] = static_cast<size_t>(GetFilePos());

	for (i=0; i<UNITACTION_MAX; i++)
		WriteData((uint8 *)s->GetShieldPoints((UNITACTION)i), sizeof(POINT) * k_NUM_FACINGS);

	WriteData((uint16)s->HasDeath());
	WriteData((uint16)s->HasDirectional());

	SetFilePos(start_of_offsets);

	WriteData((uint8*)offset, sizeof(int) * (ACTION_MAX+1));

	return SPRITEFILEERR_OK;
}




SPRITEFILEERR SpriteFile::Write(UnitSpriteGroup *s)
{
	switch(m_version)
	{
	case	k_SPRITEFILE_VERSION1:
	case	k_SPRITEFILE_VERSION2:
			return Write_v20(s);

	case	k_SPRITEFILE_VERSION0:
	default:
			return Write_v13(s);
	}
}








































































SPRITEFILEERR SpriteFile::Write(EffectSpriteGroup *s)
{
	Anim		*anim;

	Sprite *    sprite = s->GetGroupSprite((GAME_ACTION)EFFECTACTION_PLAY);
	WriteData((uint32)(sprite != nullptr));
	if (sprite != nullptr)
	{
		WriteSpriteData(sprite);
		anim = s->GetGroupAnim((GAME_ACTION)EFFECTACTION_PLAY);
		WriteData((uint32)(anim != nullptr));
		if (anim != nullptr)
		{
			WriteAnimData(anim);
		}
	}

	sprite = s->GetGroupSprite((GAME_ACTION)EFFECTACTION_FLASH);
	WriteData((uint32)(sprite != nullptr));
	if (sprite != nullptr)
	{
		WriteSpriteData(sprite);
		anim = s->GetGroupAnim((GAME_ACTION)EFFECTACTION_FLASH);
		WriteData((uint32)(anim != nullptr));
		if (anim != nullptr)
		{
			WriteAnimData(anim);
		}
	}

	return SPRITEFILEERR_OK;
}

SPRITEFILEERR SpriteFile::Write(GoodSpriteGroup *s)
{
	uint16		i;
	uint32		offset[GOODACTION_MAX];

	for (i=0; i<GOODACTION_MAX; i++)
	{
		WriteData((uint32)0);
	}

	for (i=0; i<GOODACTION_MAX; i++)
	{
		Sprite *    sprite = s->GetGroupSprite((GAME_ACTION)i);
		if (sprite != nullptr)
		{

			WriteData((uint32)TRUE);

			offset[i] = static_cast<size_t>(GetFilePos());

			WriteSpriteData(sprite);
			WriteAnimData(s->GetGroupAnim((GAME_ACTION)i));

		}
		else
		{

			WriteData((uint32)FALSE);
			offset[i] = static_cast<uint32>(-1);
		}

	}

	SetFilePos(k_SPRITEFILE_HEADER_SIZE + (m_version == k_SPRITEFILE_VERSION2 ? sizeof(uint32) : 0));

	for (i=0; i<GOODACTION_MAX; i++)
	{
		WriteData(offset[i]);
	}

	return SPRITEFILEERR_OK;
}

SPRITEFILEERR SpriteFile::Write(CitySpriteGroup *s, Anim *anim)
{
	return SPRITEFILEERR_OK;
}

SPRITEFILEERR SpriteFile::CloseWrite()
{
	if (m_file)
    {
        c3files_fclose(m_file);
        m_file = nullptr;
    }

	return SPRITEFILEERR_OK;
}


SPRITEFILEERR SpriteFile::Open(SPRITEFILETYPE *type) try
{
    if (m_file) c3files_fclose(m_file);
    m_file = std::filesystem::path(m_filename).is_absolute()
        ? std::fopen(m_filename, "rb")
        : c3files_fopen(C3DIR_SPRITES, m_filename, "rb");
    if (!m_file) return SPRITEFILEERR_NOOPEN;
    m_reading = true;
    m_decodedBytes = 0;
    RequireSprite(std::fseek(m_file, 0, SEEK_END) == 0, "cannot size sprite file");
    m_fileSize = GetFilePos();
    RequireSprite(m_fileSize >= 12 && size_t(m_fileSize) <= kMaxSpriteBytes, "invalid sprite file size");
    SetFilePos(0);
    uint32 data;
    ReadData(&data, sizeof(data));
    RequireSprite(data == k_SPRITEFILE_TAG, "invalid sprite file tag");
    ReadData(&m_version, sizeof(m_version));
    RequireSprite(m_version == k_SPRITEFILE_VERSION0 || m_version == k_SPRITEFILE_VERSION1
                  || m_version == k_SPRITEFILE_VERSION2, "unsupported sprite file version");
    m_spr_compression = SPRDATA_REGULAR;
    if (m_version == k_SPRITEFILE_VERSION2) {
        ReadData(&data, sizeof(data));
        RequireSprite(data < SPRDATA_MAX, "invalid sprite compression mode");
        // Shipping v2 files use LZW1, including the copy-mode header.
        m_spr_compression = SPRDATA_LZW1;
    }
    ReadData(&data, sizeof(data));
    // The old v2 goods writer sought to byte 12 (the v1 header size),
    // overwriting type with the first sprite offset, 24. GG023 ships this
    // layout. Its body still uses the normal bounded goods parser.
    if (m_version == k_SPRITEFILE_VERSION2 && data == 24) data = SPRITEFILETYPE_GOOD;
    RequireSprite(data < SPRITEFILETYPE_MAX, "invalid sprite file type");
    *type = static_cast<SPRITEFILETYPE>(data);
    return SPRITEFILEERR_OK;
}
catch (std::exception const &error) {
    std::fprintf(stderr, "[sprite] %s: %s\n", m_filename, error.what());
    CloseRead();
    return SPRITEFILEERR_READERR;
}

SPRITEFILEERR SpriteFile::Read(Sprite **s, Anim **anim) try
{
    uint32 offsets[2];
    ReadData(offsets, sizeof(offsets));
    auto sprite = std::make_unique<Sprite>();
    ReadSpriteDataFull(sprite.get());
    auto animation = std::make_unique<Anim>();
    ReadAnimDataFull(animation.get());
    *s = sprite.release();
    *anim = animation.release();
    return SPRITEFILEERR_OK;
}
catch (std::exception const &error) {
    std::fprintf(stderr, "[sprite] %s: %s\n", m_filename, error.what());
    return SPRITEFILEERR_READERR;
}

SPRITEFILEERR SpriteFile::Read(FacedSprite **s, Anim **anim) try
{
    uint32 offsets[2];
    ReadData(offsets, sizeof(offsets));
    auto sprite = std::make_unique<FacedSprite>();
    ReadFacedSpriteDataFull(sprite.get());
    auto animation = std::make_unique<Anim>();
    ReadAnimDataFull(animation.get());
    *s = sprite.release();
    *anim = animation.release();
    return SPRITEFILEERR_OK;
}
catch (std::exception const &error) {
    std::fprintf(stderr, "[sprite] %s: %s\n", m_filename, error.what());
    return SPRITEFILEERR_READERR;
}

SPRITEFILEERR SpriteFile::Read(FacedSpriteWshadow **s, Anim **anim) try
{
    uint32 offsets[2];
    ReadData(offsets, sizeof(offsets));
    auto sprite = std::make_unique<FacedSpriteWshadow>();
    ReadFacedSpriteWshadowData(sprite.get());
    auto animation = std::make_unique<Anim>();
    ReadAnimDataFull(animation.get());
    *s = sprite.release();
    *anim = animation.release();
    return SPRITEFILEERR_OK;
}
catch (std::exception const &error) {
    std::fprintf(stderr, "[sprite] %s: %s\n", m_filename, error.what());
    return SPRITEFILEERR_READERR;
}


SPRITEFILEERR SpriteFile::Read(SpriteGroup **, Anim **) { return SPRITEFILEERR_READERR; }
SPRITEFILEERR SpriteFile::Read(CitySpriteGroup **, Anim **) { return SPRITEFILEERR_READERR; }

void SpriteFile::ReadGroupSprite(SpriteGroup *group, GAME_ACTION action, bool basic)
{
    Sprite *sprite = nullptr;
    ReadGeneral(&sprite, basic);
    group->SetGroupSprite(action, sprite);
}

void SpriteFile::ReadGroupAnim(SpriteGroup *group, GAME_ACTION action, bool basic)
{
    auto animation = std::make_unique<Anim>();
    if (basic) ReadAnimDataBasic(animation.get()); else ReadAnimDataFull(animation.get());
    group->SetGroupAnim(action, animation.release());
}

void SpriteFile::ReadUnitMetadata(UnitSpriteGroup *group, bool legacy)
{
    uint16 data;
    POINT points[k_NUM_FACINGS];
    if (legacy) {
        for (int list = 0; list < 2; ++list) {
            ReadData(&data, sizeof(data));
            for (int i = 0; i < k_NUM_FIREPOINTS; ++i) ReadData(points, sizeof(points));
        }
        ReadData(points, sizeof(points));
    }
    for (int i = 0; i < UNITACTION_MAX; ++i)
        ReadData(group->GetShieldPoints(static_cast<UNITACTION>(i)), sizeof(points));
    ReadData(&data, sizeof(data));
    group->SetHasDeath(data != 0);
    ReadData(&data, sizeof(data));
    group->SetHasDirectional(data != 0);
}

void SpriteFile::ReadUnitGroup(UnitSpriteGroup *group, bool basic, int selected)
{
    RequireSprite(selected >= -1 && selected < ACTION_MAX, "invalid unit action index");
    bool legacy = m_version == k_SPRITEFILE_VERSION0;
    sint32 offsets[ACTION_MAX + 1];
    // UNITACTION_MAX (legacy v0 files, 5 unit actions) vs ACTION_MAX
    // (GAME_ACTION count, current format) are distinct enums; the sprite
    // file just stores a raw action count, so compare as plain int.
    int count = legacy ? static_cast<int>(UNITACTION_MAX) : static_cast<int>(ACTION_MAX);
    ReadData(offsets, sizeof(sint32) * (legacy ? count : count + 1));
    if (selected == -1) {
        group->DeallocateStorage();
        group->DeallocateFullLoadAnims();
    }
    for (int i = 0; i < count; ++i) {
        uint32 present = offsets[i] > 0;
        if (legacy) ReadData(&present, sizeof(present));
        if (!present) continue;
        bool keep = selected >= 0 ? i == selected : !basic || i == UNITACTION_MOVE || i == UNITACTION_IDLE;
        if (!keep) {
            if (legacy) { SkipSpriteDataGeneral(); SkipAnimData(); }
            continue;
        }
        if (!legacy) SetFilePos(offsets[i]);
        bool oneFrame = basic;
#ifndef __MAKESPR__
        if (i == UNITACTION_IDLE && profiledb_Get() && profiledb_Get()->IsUnitAnim()) oneFrame = false;
#endif
        auto action = static_cast<GAME_ACTION>(i);
        ReadGroupSprite(group, action, oneFrame);
        ReadGroupAnim(group, action, oneFrame);
    }
    if (!legacy) SetFilePos(offsets[ACTION_MAX]);
    ReadUnitMetadata(group, legacy);
}

void SpriteFile::ReadGoodGroup(GoodSpriteGroup *group, bool basic)
{
    uint32 offsets[GOODACTION_MAX];
    ReadData(offsets, sizeof(offsets));
    group->DeallocateStorage();
    group->DeallocateFullLoadAnims();
    for (int i = 0; i < GOODACTION_MAX; ++i) {
        uint32 present;
        ReadData(&present, sizeof(present));
        if (!present) continue;
        auto action = static_cast<GAME_ACTION>(i);
        ReadGroupSprite(group, action, basic);
        ReadGroupAnim(group, action, basic);
    }
}

SPRITEFILEERR SpriteFile::ReadBasic(UnitSpriteGroup *s) try
{
    ReadUnitGroup(s, true, -1);
    return SPRITEFILEERR_OK;
}
catch (std::exception const &error) {
    std::fprintf(stderr, "[sprite] %s: %s\n", m_filename, error.what());
    s->DeallocateStorage();
    s->DeallocateFullLoadAnims();
    return SPRITEFILEERR_READERR;
}

SPRITEFILEERR SpriteFile::ReadFull(UnitSpriteGroup *s) try
{
    ReadUnitGroup(s, false, -1);
    return SPRITEFILEERR_OK;
}
catch (std::exception const &error) {
    std::fprintf(stderr, "[sprite] %s: %s\n", m_filename, error.what());
    s->DeallocateStorage();
    s->DeallocateFullLoadAnims();
    return SPRITEFILEERR_READERR;
}

SPRITEFILEERR SpriteFile::ReadIndexed(UnitSpriteGroup *s, GAME_ACTION action) try
{
    ReadUnitGroup(s, false, action);
    return SPRITEFILEERR_OK;
}
catch (std::exception const &error) {
    std::fprintf(stderr, "[sprite] %s: %s\n", m_filename, error.what());
    s->DeallocateStorage();
    s->DeallocateFullLoadAnims();
    return SPRITEFILEERR_READERR;
}

SPRITEFILEERR SpriteFile::ReadBasic(GoodSpriteGroup *s) try
{
    ReadGoodGroup(s, true);
    return SPRITEFILEERR_OK;
}
catch (std::exception const &error) {
    std::fprintf(stderr, "[sprite] %s: %s\n", m_filename, error.what());
    s->DeallocateStorage();
    s->DeallocateFullLoadAnims();
    return SPRITEFILEERR_READERR;
}

SPRITEFILEERR SpriteFile::ReadFull(GoodSpriteGroup *s) try
{
    ReadGoodGroup(s, false);
    return SPRITEFILEERR_OK;
}
catch (std::exception const &error) {
    std::fprintf(stderr, "[sprite] %s: %s\n", m_filename, error.what());
    s->DeallocateStorage();
    s->DeallocateFullLoadAnims();
    return SPRITEFILEERR_READERR;
}

SPRITEFILEERR SpriteFile::ReadIndexed(GoodSpriteGroup *s, GAME_ACTION action) try
{
    // action is GAME_ACTION (shared sprite-file action index) while
    // GOODACTION_MAX bounds the goods enum; the file format intentionally
    // reuses one action index space, so bound-check as plain int.
    RequireSprite(action >= 0 && action < static_cast<int>(GOODACTION_MAX), "invalid goods action index");
    ReadGoodGroup(s, false);
    return SPRITEFILEERR_OK;
}
catch (std::exception const &error) {
    std::fprintf(stderr, "[sprite] %s: %s\n", m_filename, error.what());
    s->DeallocateStorage();
    s->DeallocateFullLoadAnims();
    return SPRITEFILEERR_READERR;
}

SPRITEFILEERR SpriteFile::Read(EffectSpriteGroup *s) try
{
    s->DeallocateStorage();
    s->DeallocateFullLoadAnims();
    for (auto action : {EFFECTACTION_PLAY, EFFECTACTION_FLASH}) {
        uint32 present;
        ReadData(&present, sizeof(present));
        if (!present) continue;
        ReadGroupSprite(s, static_cast<GAME_ACTION>(action), false);
        ReadData(&present, sizeof(present));
        if (present) ReadGroupAnim(s, static_cast<GAME_ACTION>(action), false);
    }
    return SPRITEFILEERR_OK;
}
catch (std::exception const &error) {
    std::fprintf(stderr, "[sprite] %s: %s\n", m_filename, error.what());
    s->DeallocateStorage();
    s->DeallocateFullLoadAnims();
    return SPRITEFILEERR_READERR;
}

SPRITEFILEERR SpriteFile::CloseRead()
{
	if (m_file)
    {
        c3files_fclose(m_file);
        m_file = nullptr;
    }

	return SPRITEFILEERR_OK;
}

SPRITEFILEERR SpriteFile::WriteData(uint8 *data, size_t bytes)
{
	size_t	countWritten = c3files_fwrite(data, 1, bytes, m_file);
	Assert(countWritten == bytes);

    return (countWritten == bytes) ? SPRITEFILEERR_OK : SPRITEFILEERR_WRITEERR;
}

SPRITEFILEERR SpriteFile::WriteData(sint16 data)
{
	size_t  countWritten = c3files_fwrite(&data, 1, 2, m_file);
	Assert(countWritten == 2);

    return (countWritten == 2) ? SPRITEFILEERR_OK : SPRITEFILEERR_WRITEERR;
}

SPRITEFILEERR SpriteFile::WriteData(uint16 data)
{
	size_t	countWritten = c3files_fwrite(&data, 1, 2, m_file);
	Assert(countWritten == 2);

    return (countWritten == 2) ? SPRITEFILEERR_OK : SPRITEFILEERR_WRITEERR;
}

SPRITEFILEERR SpriteFile::WriteData(sint32 data)
{
	size_t	countWritten = c3files_fwrite(&data, 1, 4, m_file);
	Assert(countWritten == 4);

    return (countWritten == 4) ? SPRITEFILEERR_OK : SPRITEFILEERR_WRITEERR;
}

SPRITEFILEERR SpriteFile::WriteData(uint32 data)
{
	size_t	countWritten = c3files_fwrite(&data, 1, 4, m_file);
	Assert(countWritten == 4);

	return (countWritten == 4) ? SPRITEFILEERR_OK : SPRITEFILEERR_WRITEERR;
}

void SpriteFile::CheckReadSize(size_t bytes)
{
    long pos = GetFilePos();
    RequireSprite(m_file && pos >= 0 && pos <= m_fileSize
                  && bytes <= size_t(m_fileSize - pos), "truncated sprite data");
}

SPRITEFILEERR SpriteFile::ReadData(void *data, size_t bytes)
{
    CheckReadSize(bytes);
    RequireSprite(c3files_fread(data, 1, bytes, m_file) == bytes, "cannot read sprite data");
    return SPRITEFILEERR_OK;
}

/**
 * Return the current file position as a plain 'long' offset.
 *
 * @return Byte offset from the start of the file.
 *
 * @note fpos_t is a struct on Linux (with a __pos member) but a scalar on
 *       Windows and macOS.  The #ifdef branches extract the offset correctly
 *       for each platform.  Missing the macOS branch caused UBSan
 *       "Missing return" because the function fell through without returning.
 */
long SpriteFile::GetFilePos()
{
    RequireSprite(m_file != nullptr, "sprite file is not open");
    long pos = std::ftell(m_file);
    RequireSprite(pos >= 0, "cannot query sprite position");
    return pos;
}

void SpriteFile::SetFilePos(long pos)
{
    RequireSprite(m_file && pos >= 0 && (!m_reading || pos <= m_fileSize), "invalid sprite offset");
    RequireSprite(std::fseek(m_file, pos, SEEK_SET) == 0, "cannot seek sprite file");
}




uint8 *
SpriteFile::CompressData  (void *Data, size_t &DataLen)
{
	uint8 *ReturnVal=nullptr;

	switch(m_spr_compression)
	{

	case	SPRDATA_REGULAR:
			ReturnVal = CompressData_Default(Data,DataLen);
			break;

	case	SPRDATA_LZW1:
			ReturnVal = CompressData_LZW1(Data,DataLen);
			break;

	default:

		DataLen = 0;
		c3errors_ErrorDialog("SpriteFile: ", "Bad Compression Mode %d",m_spr_compression);
	};

	return ReturnVal;
}

uint8 *
SpriteFile::DeCompressData(void *Data, size_t CompressedLen, size_t ActualLen)
{
	uint8 *ReturnVal = nullptr;

	switch(m_spr_compression)
	{

	case	SPRDATA_REGULAR:
			ReturnVal = DeCompressData_Default(Data,CompressedLen,ActualLen);
			break;

	case	SPRDATA_LZW1:
			ReturnVal = DeCompressData_LZW1(Data,CompressedLen,ActualLen);
			break;

	default:

		throw std::runtime_error("invalid sprite compression mode");
	};

	return ReturnVal;
}




uint8 *
SpriteFile::CompressData_Default  (void *Data, size_t &DataLen)
{
	auto ReturnVal = std::make_unique<uint8[]>(DataLen);

	memcpy(ReturnVal.get(),Data,DataLen);

  return ReturnVal.release();
}

uint8 *
SpriteFile::DeCompressData_Default(void *Data, size_t CompressedLen, size_t ActualLen)
{
    RequireSprite(Data && ActualLen <= kMaxFrameBytes && CompressedLen == ActualLen,
                  "invalid raw sprite length");
    auto result = std::make_unique<uint8[]>(ActualLen);
    std::memcpy(result.get(), Data, ActualLen);
    return result.release();
}

uint8 *
SpriteFile::CompressData_LZW1(void *Data, size_t &DataLen)
{
 uint8  *p_src_first=(uint8 *)Data;
 uint8  *p_dst_first=(uint8 *)g_compression_buff.get();

 size_t  src_len=DataLen;
 uint32     p_dst_len=COM_BUFF_SIZE;

 uint8 *p_src=p_src_first;
 uint8 *p_dst=p_dst_first;
 uint8 *p_src_post=p_src_first+src_len;
 uint8 *p_dst_post=p_dst_first+src_len;
 uint8 *p_src_max1=p_src_post-LZW1_ITEMMAX;
 uint8 *p_src_max16=p_src_post-16*LZW1_ITEMMAX;
 uint8 *hash[4096];
 uint8 *p_control;
 uint16 control=0;
 uint16 control_bits=0;

 *p_dst=LZW1_FLAG_COMPRESS;
 p_dst+=LZW1_FLAG_BYTES;
 p_control=p_dst;
 p_dst+=2;

 while (true)
 {
	uint8 *p;
	uint8 *s;
	uint16 unroll=16;
	uint16 len;
	uint32 index;
	uint32 offset;

	if (p_dst>p_dst_post)
		goto overrun;

    if (p_src>p_src_max16)
    {
		unroll=1;
		if (p_src>p_src_max1)
        {
			if (p_src==p_src_post)
				break;
			goto literal;
		}
	}

begin_unrolled_loop:

    index=((40543*((((p_src[0]<<4)^p_src[1])<<4)^p_src[2]))>>4) & 0xFFF;

	p=hash[index];
	hash[index]=s=p_src;
	offset=s-p;

	if ((offset>4095) || (p<p_src_first) || (offset==0) || LZW1_PS || LZW1_PS || LZW1_PS)
    {
literal:
	 	*p_dst++=*p_src++;
	 	control>>=1;
	 	control_bits++;
	}
    else
    {
	   LZW1_PS || LZW1_PS || LZW1_PS || LZW1_PS || LZW1_PS || LZW1_PS || LZW1_PS ||
       LZW1_PS || LZW1_PS || LZW1_PS || LZW1_PS || LZW1_PS || LZW1_PS || s++;
	   len = static_cast<uint16>(s - p_src - 1);
       *p_dst++=(unsigned char)(((offset&0xF00)>>4)+(len-1));
	   *p_dst++=(unsigned char)(offset&0xFF);
       p_src+=len;
	   control=(control>>1)|0x8000; control_bits++;
	}


	if (--unroll)
	   goto begin_unrolled_loop;

	if (control_bits==16)
	{
		*p_control++ = static_cast<uint8>(control&0xFF);
		*p_control++ = static_cast<uint8>(control>>8);
		p_control    = p_dst;
		p_dst += 2;
		control=control_bits=0;
	}
 }

 control>>=16-control_bits;
 *p_control++ = static_cast<uint8>(control&0xFF);
 *p_control++ = static_cast<uint8>(control>>8);

 if (p_control==p_dst)
	 p_dst-=2;

 p_dst_len=p_dst-p_dst_first;

 goto end_of_compression;

overrun: memcpy(p_dst_first+LZW1_FLAG_BYTES,p_src_first,src_len);

   *p_dst_first=LZW1_FLAG_COPY;
	p_dst_len=src_len+LZW1_FLAG_BYTES;

end_of_compression:

    auto retval = std::make_unique<uint8[]>(p_dst_len);
    DataLen = p_dst_len;
    memcpy(retval.get(), g_compression_buff.get(), DataLen);

    return retval.release();
}


uint8 *
SpriteFile::DeCompressData_LZW1(void *Data, size_t CompressedLen, size_t ActualLen)
{
    RequireSprite(Data && CompressedLen >= LZW1_FLAG_BYTES && ActualLen <= kMaxFrameBytes,
                  "invalid LZW1 sprite header or length");
    auto const *input = static_cast<uint8 const *>(Data);
    RequireSprite(input[0] == LZW1_FLAG_COPY || input[0] == LZW1_FLAG_COMPRESS,
                  "invalid LZW1 compression flag");
    auto output = std::make_unique<uint8[]>(ActualLen);
    if (input[0] == LZW1_FLAG_COPY) {
        RequireSprite(CompressedLen - LZW1_FLAG_BYTES == ActualLen, "LZW1 copy length mismatch");
        std::memcpy(output.get(), input + LZW1_FLAG_BYTES, ActualLen);
        return output.release();
    }
    size_t src = LZW1_FLAG_BYTES, dst = 0;
    uint16 control = 0;
    unsigned bits = 0;
    while (src < CompressedLen) {
        if (bits == 0) {
            RequireSprite(CompressedLen - src >= 2, "truncated LZW1 control word");
            control = input[src] | (uint16(input[src + 1]) << 8);
            src += 2;
            bits = 16;
        }
        if (control & 1) {
            RequireSprite(CompressedLen - src >= 2, "truncated LZW1 reference");
            size_t offset = ((input[src] & 0xf0) << 4) | input[src + 1];
            size_t length = 1 + (input[src] & 0xf);
            src += 2;
            RequireSprite(offset > 0 && offset <= dst && length <= ActualLen - dst,
                          "LZW1 reference outside output");
            while (length--) { output[dst] = output[dst - offset]; ++dst; }
        } else {
            RequireSprite(src < CompressedLen && dst < ActualLen, "LZW1 literal outside output");
            output[dst++] = input[src++];
        }
        control >>= 1;
        --bits;
    }
    RequireSprite(dst == ActualLen, "LZW1 decoded length mismatch");
    return output.release();
}
