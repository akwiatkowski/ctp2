//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  :
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
// - Incorrect check repaired.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "ui/aui_ctp2/grabitem.h"

#include <memory>

std::unique_ptr<GrabItem> g_grabbedItem;

GrabItem::GrabItem()
{
	m_isGrabbed = FALSE;
	m_grabbedItem = nullptr;
	m_grabbedItemType = GRABITEMTYPE_NONE;
}

GrabItem::~GrabItem()
= default;

//----------------------------------------------------------------------------
//
// Name       : GrabItem::Init
//
// Description: (Re)initialise the global grab item.
//
// Parameters : -
//
// Globals    : g_grabbedItem
//
// Returns    : -
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------

void GrabItem::Init()
{
	g_grabbedItem = std::make_unique<GrabItem>();
}

void GrabItem::Cleanup()
{
	g_grabbedItem.reset();
}

void GrabItem::SetGrabbedItem(Unit *unit)
{
	m_grabbedItemType = GRABITEMTYPE_UNIT;
	m_grabbedItem = (void *)unit;
}

void GrabItem::GetGrabbedItem(Unit **unit)
{
	Assert(m_grabbedItemType == GRABITEMTYPE_UNIT);
	if (m_grabbedItemType != GRABITEMTYPE_UNIT) {
		*unit = nullptr;
		return;
	}

	*unit = (Unit *)m_grabbedItem;
}

void GrabItem::SetGrabbedItem(TradeRoute *route)
{
	m_grabbedItemType = GRABITEMTYPE_TRADEROUTE;
	m_grabbedItem = (void *)route;
}

void GrabItem::GetGrabbedItem(TradeRoute **route)
{
	Assert(m_grabbedItemType == GRABITEMTYPE_TRADEROUTE);
	if (m_grabbedItemType != GRABITEMTYPE_TRADEROUTE) {
		*route = nullptr;
		return;
	}

	*route = (TradeRoute *)m_grabbedItem;
}
