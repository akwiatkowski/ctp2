//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : City ressource lists
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
// - Added Resize method for loading of savegames with different
//   number of goods than in the database. - May 28th 2005 Martin Gühmann
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/gameobj/Resources.h"
#include "ResourceRecord.h"

Resources::Resources()
{
	m_numGoods = g_theResourceDB->NumRecords();
	m_supply.resize(m_numGoods);
	Clear();
}

Resources::Resources(const Resources &copyme)
{
	m_numGoods = copyme.m_numGoods;
	m_totalResources = copyme.m_totalResources;
	m_supply = copyme.m_supply;
}

Resources & Resources::operator = (Resources &copyme)
{
	Assert(m_numGoods == copyme.m_numGoods);
	m_numGoods = copyme.m_numGoods;
	m_totalResources = copyme.m_totalResources;
	m_supply = copyme.m_supply;
	return *this;
}

//----------------------------------------------------------------------------
//
// Name       : Resources::Resize
//
// Description: Resizes the underlying array m_supply by generating a new one
//              and copying the data from the old one.
//
// Parameters : newSize: The new size of m_supply
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : Added for valid check so that savegames with modified
//              ressource database can be loaded.
//
//----------------------------------------------------------------------------
void Resources::Resize(sint32 newSize)
{

	m_numGoods = newSize;
	m_supply.resize(newSize, 0);
}
