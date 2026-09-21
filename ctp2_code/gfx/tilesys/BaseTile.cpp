//----------------------------------------------------------------------------//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Base Tile
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
// - None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - None
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "ctp/ctp2_utils/c3files.h"

#include "gfx/gfx_utils/pixelutils.h"
#include "gfx/tilesys/tileutils.h"
#include "gs/world/TileInfo.h"
#include "gfx/tilesys/tileset.h"
#include "gfx/tilesys/BaseTile.h"

BaseTile::BaseTile()
:
    m_flags         (0),
    m_baseType      (0),
    m_tileNum       (0),
    m_tileDataLen   (0),
    m_hatDataLen    (0),
    m_tileData      (nullptr),
    m_hatData       (nullptr)
{
}

BaseTile::~BaseTile() = default;

BOOL BaseTile::Read(FILE *file)
{
	Assert(file != nullptr);
	if (file == nullptr) return FALSE;

	c3files_fread(&m_tileNum    , 1, sizeof(m_tileNum), file);
	c3files_fread(&m_baseType   , 1, sizeof(m_baseType), file);

	c3files_fread(&m_flags      , 1, sizeof(m_flags), file);
	c3files_fread(&m_tileDataLen, 1, sizeof(m_tileDataLen), file);

	m_tileDataOwner = std::make_unique<Pixel16[]>(m_tileDataLen/2);
	m_tileData      = m_tileDataOwner.get();
	c3files_fread(m_tileData    , 1, m_tileDataLen, file);

	uint16	size;
	c3files_fread(&size         , 1, sizeof(size), file);

	std::unique_ptr<Pixel16[]>	hatData;
	if (size > 0)
	{
		hatData = std::make_unique<Pixel16[]>(size/2);
		c3files_fread(hatData.get(), 1, size, file);
	}

	m_hatDataLen   = size;
	m_hatDataOwner = std::move(hatData);
	m_hatData      = m_hatDataOwner.get();

	return TRUE;
}

BOOL BaseTile::QuickRead(uint8 **dataPtr, BOOL mapped)
{
	m_tileNum      = *(uint16 *)(*dataPtr);
	(*dataPtr)    += sizeof(uint16);

	m_baseType     = *(uint8 *)(*dataPtr);
	(*dataPtr)    += sizeof(uint8);

	m_flags        = *(uint8 *)(*dataPtr);
	(*dataPtr)    += sizeof(uint8);

	m_tileDataLen  = *(uint16 *)(*dataPtr);
	(*dataPtr)    += sizeof(uint16);

	m_tileData     = (Pixel16 *)(*dataPtr);
	(*dataPtr)    += (m_tileDataLen);

	uint16 size    = *(uint16 *)(*dataPtr);
	(*dataPtr)    += sizeof(uint16);

	Pixel16		*hatData;
	if (size > 0)
	{
		hatData = (Pixel16 *)(*dataPtr);
		(*dataPtr) += size;
	}
	else
	{
		hatData = nullptr;
	}

	m_hatDataLen = size;
	m_hatData    = hatData;

	m_flags |= k_BTF_QUICKLOADED;

	return TRUE;
}
