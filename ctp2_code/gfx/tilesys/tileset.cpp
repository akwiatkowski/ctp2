//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Tile set
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
// HAVE_UNISTD_H
// HAVE_SYS_STAT_H
// HAVE_SYS_TYPES_H
// LINUX
// WIN32
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Added new map icon database. (3-Mar-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include <memory>
#include <vector>

#include "ctp/c3.h"
#include "gfx/tilesys/tileset.h"

#include "gs/world/World.h"

#include "ctp/ctp2_utils/c3errors.h"

#include "gfx/gfx_utils/pixelutils.h"
#include "gfx/tilesys/tileutils.h"
#include "gfx/gfx_utils/tiffutils.h"
#include "gfx/tilesys/BaseTile.h"

#include "gfx/tilesys/tiledmap.h"   // tiledmap_Get

#include "gs/fileio/CivPaths.h"   // civpaths_Get()
#include "gfx/gfx_utils/rimutils.h"
#include "MapIconRecord.h"

#include "gs/fileio/prjfile.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <fcntl.h>
#ifdef LINUX
#include <sys/mman.h>
#endif

extern ProjectFile *    g_ImageMapPF;
namespace
{
    uint8 const     DIRECTION_INVALID   = static_cast<uint8>(-1);   // 255

    char const      TILESETFILE_555[]   = "gtset555.til";
    char const      TILESETFILE_565[]   = "gtset565.til";

    char const *    TileSetFile()
    {
        return (is_565_Get()) ? TILESETFILE_565 : TILESETFILE_555;
    }
}

TileSet::TileSet()
:
	m_numTransforms         (0),
    m_numRiverTransforms    (0),
	m_tileSetData           (nullptr),
	m_numMegaTiles          (0),
    m_quick                 (false),
	m_mapped                (false),
#ifdef WIN32
	m_fileHandle            (INVALID_HANDLE_VALUE),
	m_mappedFileHandle      (INVALID_HANDLE_VALUE)
#else
	m_fd                    (-1),
	m_MMapSize              (0)
#endif
{
	sint32		 i;
	sint32		 j;
	sint32		 k;

	// Data from the tile file
	for (i=0; i<TERRAIN_MAX; i++) {
		for (j=0; j<TERRAIN_MAX; j++) {
			for(k=0; k<k_TRANSITIONS_PER_TILE; k++) {
				m_transitions[i][j][k] = nullptr;
			}
		}
	}

	// m_baseTiles is an array of unique_ptr, default-null

	// Data from the tile file
	for (i=0; i<k_MAX_IMPROVEMENTS; i++) {
		m_improvementData[i] = nullptr;
	}

	// Data from the tile file
	for (i=0; i<k_MAX_MEGATILES; i++) {
		for (j=0; j<k_MAX_MEGATILE_STEPS; j++) {
			m_megaTileData[i][j].direction = 0;
			m_megaTileData[i][j].terrainType = 0;
			m_megaTileData[i][j].tileNum = 0;
		}
		m_megaTileLengths[i] = 0;
	}
}

TileSet::~TileSet()
{
	Cleanup();
}

// ToDo: Merge the comon parts of CleanupQuick and CleanupMapped

void TileSet::CleanupQuick()
{
	m_transforms.clear();
	m_numTransforms = 0;

	m_riverTransforms.clear();

	m_riverData.clear();

	sint32 i;

	for (i=0; i<k_MAX_BASE_TILES; i++)
	{
		m_baseTiles[i].reset();
	}

	// m_mapIcons is a vector of unique_ptr, auto-freed by clear()
	m_mapIcons.clear();
	m_mapIconDimensions.clear();

	m_tileSetDataOwner.reset();   // heap path only; mmap path uses CleanupMapped
	m_tileSetData = nullptr;
}

void TileSet::CleanupMapped()
{
	m_transforms.clear();
	m_numTransforms = 0;

	m_riverTransforms.clear();

	m_riverData.clear();

	sint32 i;

	for (i=0; i<k_MAX_BASE_TILES; i++)
	{
		m_baseTiles[i].reset();
	}

	// m_mapIcons is a vector of unique_ptr, auto-freed by clear()
	m_mapIcons.clear();
	m_mapIconDimensions.clear();

#ifdef WIN32
	UnmapViewOfFile(m_tileSetData);

	CloseHandle(m_mappedFileHandle);
	CloseHandle(m_fileHandle);
#else
	munmap(m_tileSetData, m_MMapSize);
	close(m_fd);
#endif
}

void TileSet::Cleanup()
{
	if (m_mapped)
    {
		CleanupMapped();
    }
	else if (m_quick)
    {
		CleanupQuick();
    }
	else
    {
	    sint32		 i;
	    sint32		 j;
	    sint32		 k;

		m_transforms.clear();
		m_transformOwners.clear();
		m_numTransforms = 0;


		m_riverTransforms.clear();
		m_riverTransformOwners.clear();
		m_riverData.clear();
		m_riverDataOwners.clear();
		m_numRiverTransforms = 0;

		for (i=0; i<TERRAIN_MAX; i++) {
			for (j=0; j<TERRAIN_MAX; j++) {
				for(k=0; k < k_TRANSITIONS_PER_TILE; k++) {
					m_transitions[i][j][k] = nullptr;
				}
			}
		}

		for (i=0; i<k_MAX_BASE_TILES; i++)
        {
			m_baseTiles[i].reset();
		}
	}
}

void TileSet::LoadBaseTiles(FILE *file)
{
	uint32			baseTileCount;
	c3files_fread(&baseTileCount, 1, sizeof(baseTileCount), file);

	for (uint32 i = 0; i < baseTileCount; ++i)
	{
		auto baseTile = std::make_unique<BaseTile>();
		baseTile->Read(file);
		sint32 const tileNum = baseTile->GetTileNum();
		if (tileNum < 0 || tileNum >= k_MAX_BASE_TILES) {
			continue;
		}
		m_baseTiles[tileNum] = std::move(baseTile);
	}
}


void TileSet::LoadTransitions(FILE *file)
{
	uint32		transitionCount = 0;
	uint32		transitionSize = 0;
	size_t		count;
    uint32      i;

	count = c3files_fread(&transitionCount, 1, sizeof(transitionCount), file);
	if (count != sizeof(transitionCount)) goto Error;

	count = c3files_fread(&transitionSize, 1, sizeof(transitionSize), file);
	if (count != sizeof(transitionSize)) goto Error;

	sint16	 from;
	sint16	 to;

	for (i = 0; i < transitionCount; ++i)
    {
		count = c3files_fread(&from, 1, sizeof(from), file);
		if (count != sizeof(from)) goto Error;
		count = c3files_fread(&to, 1, sizeof(to), file);
		if (count != sizeof(to)) goto Error;

		for (size_t k = 0; k < k_TRANSITIONS_PER_TILE; ++k)
        {
	        auto xData = std::make_unique<Pixel16[]>(transitionSize/2);
			count = c3files_fread(xData.get(), 1, transitionSize, file);
			if (count != transitionSize) goto Error;

			m_transitions[from][to][k] = xData.release();
		}
	}

	return;

Error:

	c3errors_FatalDialog("TileSet", "Could not load Transitions");
}

void TileSet::LoadTransforms(FILE *file)
{
	if (file)
    {
	    uint16		numTransforms;
		c3files_fread(&numTransforms, 1, sizeof(numTransforms), file);

		m_transforms.resize(numTransforms);
		m_numTransforms = numTransforms;

		for (uint16 i = 0; i < numTransforms; ++i)
        {
	        auto transform = std::make_unique<sint16[]>(k_TRANSFORM_SIZE);
			c3files_fread(transform.get(), 1, sizeof(sint16)*k_TRANSFORM_SIZE, file);
			m_transforms[i] = transform.get();
			m_transformOwners.push_back(std::move(transform));
		}
	}
}

void TileSet::LoadRiverTransforms(FILE *file)
{
	if (file)
    {
	    uint16		numRiverTransforms;
		c3files_fread(&numRiverTransforms, 1, sizeof(numRiverTransforms), file);

		if (numRiverTransforms > 0) {
			m_riverTransforms.resize(numRiverTransforms);
			m_numRiverTransforms = numRiverTransforms;
			m_riverData.resize(numRiverTransforms);

			for (uint16 i = 0; i < numRiverTransforms; ++i)
            {
	            auto transform = std::make_unique<sint16[]>(k_RIVER_TRANSFORM_SIZE);
				c3files_fread(transform.get(), 1, sizeof(sint16)*k_RIVER_TRANSFORM_SIZE, file);
				m_riverTransforms[i] = transform.get();
				m_riverTransformOwners.push_back(std::move(transform));

	            uint32		len = 0;
				c3files_fread(&len, 1, sizeof(uint32), file);

				if (len > 0)
                {
                	auto riverData = std::make_unique<Pixel16[]>(len/2);
					c3files_fread(riverData.get(), 1, len, file);
					m_riverData[i] = riverData.get();
					m_riverDataOwners.push_back(std::move(riverData));
				}
                else
                {
					m_riverData[i] = nullptr;
				}
			}
		}
	}
}

void TileSet::LoadImprovements(FILE *file)
{
	if (file)
    {
	    uint16		numImprovements;
		c3files_fread(&numImprovements, 1, sizeof(numImprovements), file);

		for (uint16 i = 0; i < numImprovements; ++i)
        {
			uint16		impNum;
			c3files_fread(&impNum, 1, sizeof(uint16), file);

	        uint32		len;
			c3files_fread(&len, 1, sizeof(uint32), file);

			if (len > 0)
            {
				auto impData = std::make_unique<Pixel16[]>(len/2);
				c3files_fread(impData.get(), 1, len, file);
				m_improvementData[impNum] = impData.release();
			}
            else
            {
				m_improvementData[impNum] = nullptr;
			}
		}
	}
}

void TileSet::LoadMegaTiles(FILE *file)
{
	if (file)
    {
		c3files_fread(&m_numMegaTiles, 1, sizeof(m_numMegaTiles), file);

		for (uint16 i = 0; i < m_numMegaTiles; ++i)
        {
			uint16			megaLen;
			c3files_fread(&megaLen, 1, sizeof(uint16), file);
			m_megaTileLengths[i] = megaLen;

			if (megaLen > 0)
            {
				c3files_fread(&m_megaTileData[i], 1, megaLen * sizeof(MegaTileStep), file);
			}
		}
	}
}

uint8 TileSet::ReverseDirection(sint32 dir)
{
	switch (dir) {
	case k_MEGATILE_DIRECTION_X	: return k_MEGATILE_DIRECTION_X;
	case k_MEGATILE_DIRECTION_N : return k_MEGATILE_DIRECTION_S;
	case k_MEGATILE_DIRECTION_E	: return k_MEGATILE_DIRECTION_W;
	case k_MEGATILE_DIRECTION_S	: return k_MEGATILE_DIRECTION_N;
	case k_MEGATILE_DIRECTION_W	: return k_MEGATILE_DIRECTION_E;
	default:
		Assert(FALSE);
		break;
	}

	return DIRECTION_INVALID;
}

void TileSet::LoadMapIcons()
{
	MBCHAR		name[_MAX_PATH];
	MBCHAR		path[_MAX_PATH];
	uint16		 width;
	uint16		 height;
	uint32		len;
	std::unique_ptr<Pixel16[]>	tga;
	std::unique_ptr<Pixel16[]>	data;

	m_mapIcons.clear();
	m_mapIcons.resize(g_theMapIconDB->NumRecords());
	m_mapIconDimensions.resize(g_theMapIconDB->NumRecords());
	for (sint32 i = 0; i < g_theMapIconDB->NumRecords(); ++i)
	{

		strlcpy(name, g_theMapIconDB->Get(i)->GetValue(), sizeof(name));

		if (civpaths_Get()->FindFile(C3DIR_PICTURES, name, path, TRUE, FALSE) == nullptr) {

			snprintf(path, sizeof(path), "%s", name);
			char * lastDot = strrchr(path, '.');
			if (lastDot)
			{
				++lastDot;
				snprintf(lastDot, sizeof(lastDot), "rim");
			}
			else
			{
				snprintf(path, sizeof(path), "%s.rim", path);
			}

			size_t  testlen = 0;
			uint8 * buf = reinterpret_cast<uint8 *>(g_ImageMapPF->getData(path, testlen));
			len = testlen;
			if (buf == nullptr) {
				c3errors_ErrorDialog("TileSet", "'%s not found in asset tree.", name);
				continue;
			}
			len -= sizeof(RIMHeader);
			RIMHeader * rhead = (RIMHeader *)buf;
			width = rhead->width;
			height = rhead->height;
			Pixel16 *   image = (Pixel16 *)(buf + sizeof(RIMHeader));
			data.reset((Pixel16 *)tileutils_EncodeTile16(image, width, height, &len, rhead->pitch));
			if (data) {
				POINT pt = {width, height};
				m_mapIconDimensions[i] = pt;

				tileutils_ConvertPixelFormatFrom555(data.get());
				m_mapIcons[i] = std::move(data);
			}
			continue;
		}

		tga.reset(tileutils_TGA2mem(path, &width, &height));
		if (tga) {
			data.reset((Pixel16 *)tileutils_EncodeTile16(tga.get(), width, height, &len));
			tga.reset();

			if (data) {

				tileutils_ConvertPixelFormatFrom555(data.get());

				POINT pt = {width, height};
				m_mapIconDimensions[i] = pt;
				m_mapIcons[i] = std::move(data);
			}
		}

	}
}

/*
Pixel16 TileSet::ConvertMapIcons(const MBCHAR *name)  //EMOD
{
	//MBCHAR		name[_MAX_PATH];
	MBCHAR		path[_MAX_PATH];
	uint16		width, height;
	uint32		len;
	Pixel16		*tga;
	Pixel16		*data;

	//for (int i = 0; i < MAPICON_MAX; ++i)
    //{
	//	snprintf(name, sizeof(name), "UPC%.3d.TGA", i+1);

		if (civpaths_Get()->FindFile(C3DIR_PICTURES, name, path, TRUE, FALSE) == NULL) {
			//snprintf(path, sizeof(path), "upc%.3d.rim", i+1);
            size_t  testlen = 0;
			uint8 * buf = reinterpret_cast<uint8 *>(g_ImageMapPF->getData(path, testlen));
            len = testlen;
			if (buf == NULL) {
				c3errors_ErrorDialog("TileSet", "'%s not found in asset tree.", name);
				//continue;
			}
			len -= sizeof(RIMHeader);
			RIMHeader * rhead = (RIMHeader *)buf;
			width = rhead->width;
			height = rhead->height;
			Pixel16 *   image = (Pixel16 *)(buf + sizeof(RIMHeader));
			data = (Pixel16 *)tileutils_EncodeTile16(image, width, height, &len, rhead->pitch);

				if (data) {
				m_mapIcons[i] = data;
				POINT pt = {width, height};
				m_mapIconDimensions[i] = pt;

				tileutils_ConvertPixelFormatFrom555(data);
			}
			continue;
		}

		tga = tileutils_TGA2mem(path, &width, &height);
		if (tga) {
			data = (Pixel16 *)tileutils_EncodeTile16(tga, width, height, &len);
			delete[] tga;   // tileutils_TGA2mem returns malloc'd — not unique_ptr
			tga = NULL;

			if (data) {

				tileutils_ConvertPixelFormatFrom555(data);

				m_mapIcons[i] = data;
				POINT pt = {width, height};
				m_mapIconDimensions[i] = pt;
			}
		}
	//}
}
*/

void TileSet::Load()
{
	FILE *  file = c3files_fopen(C3DIR_TILES, TileSetFile(), "rb");

	if (file)
	{
		LoadTransforms(file);
		LoadTransitions(file);
		LoadBaseTiles(file);
		LoadRiverTransforms(file);
		LoadImprovements(file);
		LoadMegaTiles(file);

		c3files_fclose(file);

		LoadMapIcons();
		m_quick = FALSE;
	}
}

void TileSet::QuickLoadTransforms(uint8 **dataPtr)
{
	if (dataPtr) {
	    memcpy(&m_numTransforms, *dataPtr, sizeof(uint16));
		(*dataPtr) += sizeof(uint16);

		m_transforms.resize(m_numTransforms);

		for (uint16 i = 0; i < m_numTransforms; ++i)
        {
			m_transforms[i] = (sint16 *)(*dataPtr);   // borrowed into m_tileSetData
			(*dataPtr) += sizeof(sint16)*k_TRANSFORM_SIZE;
		}
	}
}

void TileSet::QuickLoadTransitions(uint8 **dataPtr)
{

	uint32		transitionCount;

	memcpy(&transitionCount, *dataPtr, sizeof(uint32));
	(*dataPtr) += sizeof(uint32);

	uint32		transitionSize;

	memcpy(&transitionSize, *dataPtr, sizeof(uint32));
	(*dataPtr) += sizeof(uint32);

	for (uint32 i = 0; i < transitionCount; ++i)
    {
		sint16		from;
		memcpy(&from, *dataPtr, sizeof(sint16));
		(*dataPtr) += sizeof(sint16);

		sint16		to;

		memcpy(&to, *dataPtr, sizeof(sint16));
		(*dataPtr) += sizeof(sint16);

		for (size_t k = 0; k < k_TRANSITIONS_PER_TILE; ++k)
        {
			m_transitions[from][to][k] = (Pixel16 *)(*dataPtr);   // borrowed into m_tileSetData
			(*dataPtr) += transitionSize;
		}
	}
}

void TileSet::QuickLoadBaseTiles(uint8 **dataPtr)
{
	uint32		baseTileCount;
	memcpy(&baseTileCount, *dataPtr, sizeof(uint32));
	(*dataPtr) += sizeof(uint32);

	for (uint32 i = 0; i < baseTileCount; ++i)
    {
	    auto baseTile = std::make_unique<BaseTile>();
		baseTile->QuickRead(dataPtr, m_mapped);

		sint32 const tileNum = baseTile->GetTileNum();
		if (tileNum < 0 || tileNum >= k_MAX_BASE_TILES) {
			continue;
		}
		m_baseTiles[tileNum] = std::move(baseTile);
	}
}

void TileSet::QuickLoadRiverTransforms(uint8 **dataPtr)
{
	uint16		numRiverTransforms;
	memcpy(&numRiverTransforms, *dataPtr, sizeof(uint16));;
	(*dataPtr) += sizeof(uint16);

	if (numRiverTransforms > 0)
    {
		m_riverTransforms.resize(numRiverTransforms);
		m_numRiverTransforms = numRiverTransforms;
		m_riverData.resize(numRiverTransforms);

		for (uint16 i = 0; i < numRiverTransforms; ++i)
        {
			m_riverTransforms[i] = (sint16 *)(*dataPtr);   // borrowed into m_tileSetData
			(*dataPtr) += (sizeof(sint16)*k_RIVER_TRANSFORM_SIZE);

			uint32		len;

			memcpy(&len, *dataPtr, sizeof(uint32));
			(*dataPtr) += sizeof(uint32);

			if (len > 0)
            {
				m_riverData[i] = (Pixel16 *)(*dataPtr);   // borrowed into m_tileSetData
				(*dataPtr) += len;
			}
            else
            {
				m_riverData[i] = nullptr;
			}
		}
	}
}

void TileSet::QuickLoadImprovements(uint8 **dataPtr)
{
	uint16		numImprovements;
	memcpy(&numImprovements, *dataPtr, sizeof(uint16));
	(*dataPtr) += sizeof(numImprovements);

	for (uint16 i = 0; i < numImprovements; ++i)
    {
		uint16		impNum;
		memcpy(&impNum, *dataPtr, sizeof(uint16));
		(*dataPtr) += sizeof(uint16);

		uint32		len;

		memcpy(&len, *dataPtr, sizeof(uint32));
		(*dataPtr) += sizeof(uint32);

		if (len > 0)
        {
			m_improvementData[impNum] = (Pixel16 *)(*dataPtr);
			(*dataPtr) += len;
		}
        else
        {
			m_improvementData[impNum] = nullptr;
		}
	}
}

void TileSet::QuickLoadMegaTiles(uint8 **dataPtr)
{
	memcpy(&m_numMegaTiles, *dataPtr, sizeof(uint16));
	(*dataPtr) += sizeof(m_numMegaTiles);

	for (uint16 i = 0; i < m_numMegaTiles; ++i)
    {
		uint16		megaLen;
		memcpy(&megaLen, *dataPtr, sizeof(uint16));
		(*dataPtr) += sizeof(megaLen);

		m_megaTileLengths[i] = megaLen;

		if (megaLen > 0)
        {
			memcpy(m_megaTileData[i], *dataPtr, sizeof(MegaTileStep) * megaLen);
			(*dataPtr) += (megaLen * sizeof(MegaTileStep));
		}
	}
}

void TileSet::QuickLoad()
{
	FILE *  file = c3files_fopen(C3DIR_TILES, TileSetFile(), "rb");
	Assert(file != nullptr);
	if (file)
    {
	    size_t	fileSize = 0;

		if (m_tileSetData == nullptr)
        {
			if (c3files_fseek(file, 0, SEEK_END)) goto Error;

            fpos_t	pos;
			if (c3files_fgetpos(file, &pos)) goto Error;

#ifndef LINUX
			fileSize = (uint32)pos;
#else
			fileSize = pos.__pos;
#endif

			if (c3files_fseek(file, 0, SEEK_SET)) goto Error;

			// TODO(phase-2): class-member buffer — wave 3 migration
			m_tileSetDataOwner = std::make_unique<uint8[]>(fileSize);
			m_tileSetData = m_tileSetDataOwner.get();
		}
        else
        {
			Cleanup();
		}

		size_t const count =
			c3files_fread(m_tileSetData, 1, fileSize, file);
		if (count != (size_t)fileSize) goto Error;

		c3files_fclose(file);
	}

    {
	    uint8 * dataPtr = reinterpret_cast<uint8 *>(m_tileSetData);

	    QuickLoadTransforms(&dataPtr);
	    QuickLoadTransitions(&dataPtr);
	    QuickLoadBaseTiles(&dataPtr);
	    QuickLoadRiverTransforms(&dataPtr);
	    QuickLoadImprovements(&dataPtr);
	    QuickLoadMegaTiles(&dataPtr);
    }

	LoadMapIcons();
	m_quick = TRUE;

	return;

Error:
	if (file != nullptr)
		fclose(file);

	m_tileSetDataOwner.reset();
    m_tileSetData = nullptr;

	c3errors_FatalDialog("Tile Set", "Unable to load tileset.");
}

void TileSet::QuickLoadMapped()
{
	MBCHAR  path[_MAX_PATH];
	civpaths_Get()->FindFile(C3DIR_TILES, TileSetFile(), path);

#ifdef WIN32
	m_fileHandle = CreateFile(path,
						GENERIC_READ,
						FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);
	if (m_fileHandle == INVALID_HANDLE_VALUE) {
		c3errors_FatalDialog("Tile Set", "Unable to load tileset.");
		return;
	}

	sint32	size = GetFileSize(m_fileHandle, NULL);
	if (size <= 0) {
		c3errors_FatalDialog("Tile Set", "Unable to load tileset.");
		return;
	}

	m_mappedFileHandle = CreateFileMapping(m_fileHandle,
						NULL,
						PAGE_READONLY,
						0,
						0,
						NULL);

	if (m_mappedFileHandle == INVALID_HANDLE_VALUE) {
		CloseHandle(m_fileHandle);
		c3errors_FatalDialog("Tile Set", "Unable to load tileset.");
		return;
	}

	m_tileSetData = (uint8 *)MapViewOfFile(m_mappedFileHandle,
						FILE_MAP_READ,
						0,
						0,
						0);
#else
	struct stat st;
	int rc = stat(path, &st);
	if (0 != rc) {
		c3errors_FatalDialog("Tile Set", "Unable to load tileset.");
		return;
	}
	m_MMapSize = st.st_size;
	m_fd = open(path, O_RDONLY);
	if (m_fd < 0) {
		c3errors_FatalDialog("Tile Set", "Unable to load tileset.");
		return;
	}
	m_tileSetData = (uint8 *)mmap(nullptr, m_MMapSize, PROT_READ, MAP_PRIVATE, m_fd, 0);
#endif

	if (m_tileSetData == nullptr) {
#ifdef WIN32
		CloseHandle(m_fileHandle);
		CloseHandle(m_mappedFileHandle);
#else
		close(m_fd);
#endif
		c3errors_FatalDialog("Tile Set", "Unable to load tileset.");
		return;
	}

	m_mapped = TRUE;

	uint8 * dataPtr = m_tileSetData;

	QuickLoadTransforms(&dataPtr);
	QuickLoadTransitions(&dataPtr);
	QuickLoadBaseTiles(&dataPtr);

	QuickLoadRiverTransforms(&dataPtr);
	QuickLoadImprovements(&dataPtr);
	QuickLoadMegaTiles(&dataPtr);

	LoadMapIcons();

	m_quick = TRUE;
}
