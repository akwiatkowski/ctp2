//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Cell and Army text
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
// -None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - The army text now appears in the debug log. (13-Aug-2008 Martin G�hmann)
//
//----------------------------------------------------------------------------

#if defined(HAVE_PRAGMA_ONCE)
#pragma once
#endif

#ifndef GFX_OPTIONS_H__
#define GFX_OPTIONS_H__

class CellText;
class GraphicOptions;

#include <map>
#include <string>

#include "gs/gameobj/Army.h"             // Army
#include "os/include/ctp2_inttypes.h"    // uintN
class MapPoint;

class CellText
{
public:
	uint8	      m_color;
	std::string   m_text;
};

class GraphicsOptions
{
public:
	GraphicsOptions();
	~GraphicsOptions() = default;

	static void Initialize();
	static void Cleanup();

	bool IsArmyTextOn() const { return m_armyTextOn; }
	void ArmyTextOn();
	void ArmyTextOff();

	bool AddTextToArmy(Army army, const char *text, const uint8 &colorMagnitude, const sint32 goalType = -1) const;
	void ResetArmyText(Army army);

	bool IsArmyNameOn() const { return m_armyNameOn; }
	void ArmyNameOn();
	void ArmyNameOff();

	bool IsCellTextOn() const { return m_cellTextOn; }
	void CellTextOn();
	void CellTextOff();

	CellText * GetCellText(MapPoint const & pos);
	bool AddTextToCell(const MapPoint &pos, const char * text, const uint8 & colorMagnitude);
	void ResetCellText(const MapPoint &pos);

private:
	bool                      m_armyTextOn;
	bool                      m_cellTextOn;
	bool                      m_armyNameOn;
	// Packed MapPoint key (PackCellAVLKey) -> overlay text.  Value
	// semantics: the map owns the strings, no manual teardown needed.
	std::map<uint32, CellText> m_cellText;
};

// g_graphicsOptions is file-static in gfx_options.cpp; access via accessors.
GraphicsOptions * graphicsoptions_Get();
void              graphicsoptions_Set(GraphicsOptions *p);

#endif
