//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Data check utility
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
// - Replced old civilisation database by new one. (Aug 20th 2005 Martin G�hmann)
// - Fixed the BeginTurn, DumpChecksum and DisplayCRC methods. (Aug 25th 2005 Martin G�hmann)
// - Added the risk database. (Aug 29th 2005 Martin G�hmann)
// - Replaced old difficulty database by new one. (April 29th 2006 Martin G�hmann)
// - Replaced old pollution database by new one. (July 15th 2006 Martin G�hmann)
// - Replaced old global warming database by new one. (July 15th 2006 Martin G�hmann)
// - Added sync check for the new map icon database. (27-Mar-2007 Martin G�hmann)
// - Added sync check for the new map database. (27-Mar-2007 Martin G�hmann)
// - Replaced old const database by new one. (5-Aug-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"                        // Precompiled header
#include "gs/utility/DataCheck.h"
#include "gs/core/text_observer.h"         // text_observer::DrawText

// Phase 0.C-5: ~50 database/pool/world includes dropped — they were
// needed by the old CivArchive Serialize loop in BeginTurn (gutted in
// Phase 0.C-4) and the CHECK_DB macro (removed alongside it).  The
// surviving CRC-display code only touches m_crc[] / m_old_crc[] and
// the CRC_TYPE_* enum.

static DataCheck                *g_dataCheck = nullptr;

DataCheck * datacheck_Get()
{
	return g_dataCheck;
}











void DataCheck_Init()
{

}












void DataCheck_Requiem()
{


	if (g_dataCheck)
	{
		delete g_dataCheck;
		g_dataCheck = nullptr;
	}

}












DataCheck::DataCheck()
{
	sint32  i;
	sint32  j;

	m_is_display = FALSE ;
	for (i=CRC_TYPE_MIN; i<CRC_TYPE_MAX; i++)
	{
		for (j=0; j<CRC_ARRAY_MAX; j++)
			m_old_crc[i][j] = m_crc[i][j] = 0;

		m_time[i] = 0;
	}

	m_total_time = 0;
}





void DataCheck::BeginTurn()
{
	// Phase 0.C-4: body gutted — used CivArchive to checksum every DB.
	// Net code disabled and CivArchive gone; m_crc stays zero.
}













sint32 DataCheck::IsWorldChanged () const
{
	sint32 i;

	for (i=0; i<CRC_ARRAY_MAX; i++)
		if (m_crc[CRC_TYPE_WORLD][i] != m_old_crc[CRC_TYPE_WORLD][i])
			return (TRUE);

	return (FALSE);
}













sint32 DataCheck::IsGlobalChanged () const
{
	sint32 i;

	for (i=0; i<CRC_ARRAY_MAX; i++)
		if (m_crc[CRC_TYPE_GLOBAL][i] != m_old_crc[CRC_TYPE_GLOBAL][i])
			return (TRUE);

	return (FALSE);
}














sint32 DataCheck::IsRandChanged () const
{
	sint32 i;

	for (i=0; i<CRC_ARRAY_MAX; i++) {
		if (m_crc[CRC_TYPE_RAND][i] != m_old_crc[CRC_TYPE_RAND][i]) {
			return TRUE;
		}
	}

	return FALSE;
}













sint32 DataCheck::IsDBChanged () const
{
	sint32	i;

	for (i=0; i<CRC_ARRAY_MAX; i++)
		if (m_crc[CRC_TYPE_DB][i] != m_old_crc[CRC_TYPE_DB][i])
			return (TRUE);

	return (FALSE) ;
}













sint32 DataCheck::IsPlayerChanged () const
{
	sint32 i;

	for (i=0; i<CRC_ARRAY_MAX; i++) {
		if (m_crc[CRC_TYPE_PLAYER][i] != m_old_crc[CRC_TYPE_PLAYER][i]) {
			return TRUE;
		}
	}
	return FALSE;
}












sint32 DataCheck::IsPopChanged () const
{
	return FALSE;
}












sint32 DataCheck::IsUnitChanged () const
{
	sint32 i;

	for (i=0; i<CRC_ARRAY_MAX; i++) {
		if (m_crc[CRC_TYPE_UNITPOOL][i] != m_old_crc[CRC_TYPE_UNITPOOL][i]) {
			return TRUE;
		}
	}
	return FALSE;
}













BOOL DataCheck::GetCRC(CRC_TYPE group, uint32 &a, uint32 &b, uint32 &c, uint32 &d)
{
	Assert(group>=CRC_TYPE_MIN);
	Assert(group<CRC_TYPE_MAX);
	if ((group<CRC_TYPE_MIN) || (group>=CRC_TYPE_MAX))
		return (FALSE);

	a = m_crc[group][CRC_ARRAY_0];
	b = m_crc[group][CRC_ARRAY_1];
	c = m_crc[group][CRC_ARRAY_2];
	d = m_crc[group][CRC_ARRAY_3];

	return (TRUE);
}













void DataCheck::SetDisplay(sint32 val)
{
	m_is_display = val;
}













sint32 DataCheck::IsChanged(sint32 t) const
{
	sint32 i;

	Assert((t>=0) && (t<CRC_TYPE_MAX));
	for(i=0; i<CRC_ARRAY_MAX; i++)
		if (m_crc[t][i] != m_old_crc[t][i])
			return (TRUE);

	return (FALSE);
}









void DataCheck::draw_crc(aui_Surface *surf, const char *str1, sint32 t, sint32 x, sint32 y) const
{
	MBCHAR	str2[80];

	text_observer::DrawText(surf, x, y, str1, 0, false);

	if (IsChanged(t))
	{
		snprintf(str2, sizeof(str2), "***");
		text_observer::DrawText(surf, x+100, y, str2, 3050, true);
	}

	snprintf(str2, sizeof(str2), "%08X %08X %08X %08X  %4.2lf", m_crc[t][CRC_ARRAY_0], m_crc[t][CRC_ARRAY_1], m_crc[t][CRC_ARRAY_2], m_crc[t][CRC_ARRAY_3], (double)(m_time[t]) / CLOCKS_PER_SEC);
	text_observer::DrawText(surf, x+125, y, str2, 0, false);
}









void DataCheck::draw_time(aui_Surface *surf, sint32 x, sint32 y) const
{
	MBCHAR s[80];

	snprintf(s, sizeof(s), "Total time %4.2lf", (double)(m_total_time) / CLOCKS_PER_SEC);
	text_observer::DrawText(surf, x, y, s, 0, false);
}









void DataCheck::DisplayCRC(aui_Surface *surf) const
{
	sint32 x=100;
	sint32 y=80;
	sint32 d=16;

	if (!m_is_display)
		return ;

	if(m_is_display > 1) {
		draw_crc(surf, "DB", CRC_TYPE_DB, x, y);                                      y+=d;
		draw_crc(surf, "PROFILE", CRC_TYPE_PROFILE_DB, x, y);                         y+=d;
		draw_crc(surf, "STRING", CRC_TYPE_STRING_DB, x, y);                           y+=d;
		draw_crc(surf, "ADVANCE", CRC_TYPE_ADVANCE_DB, x, y);                         y+=d;
		draw_crc(surf, "ADVANCE_BRANCH", CRC_TYPE_ADVANCE_BRANCH_DB, x, y);           y+=d;
		draw_crc(surf, "ADVANCE_LIST", CRC_TYPE_ADVANCE_LIST_DB, x, y);               y+=d;
		draw_crc(surf, "AGE", CRC_TYPE_AGE_DB, x, y);                                 y+=d;
		draw_crc(surf, "AGE_CITY_STYLE", CRC_TYPE_AGE_CITY_STYLE_DB, x, y);           y+=d;
		draw_crc(surf, "BUILD_LIST_SEQUENCE", CRC_TYPE_BUILD_LIST_SEQUENCE_DB, x, y); y+=d;
		draw_crc(surf, "BUILDING", CRC_TYPE_BUILDING_DB, x, y);                       y+=d;
		draw_crc(surf, "BUILDING_BUILD_LIST", CRC_TYPE_BUILDING_BUILD_LIST_DB, x, y); y+=d;
		draw_crc(surf, "CITY_SIZE", CRC_TYPE_CITY_SIZE_DB, x, y);                     y+=d;
		draw_crc(surf, "CITY_STYLE", CRC_TYPE_CITY_STYLE_DB, x, y);                   y+=d;
		draw_crc(surf, "CIVILISATION", CRC_TYPE_CIVILISATION_DB, x, y);               y+=d;
		draw_crc(surf, "CONST", CRC_TYPE_CONST_DB, x, y);                             y+=d;
		draw_crc(surf, "DIFFICULTY", CRC_TYPE_DIFFICULTY_DB, x, y);                   y+=d;
		draw_crc(surf, "DIPLOMACY", CRC_TYPE_DIPLOMACY_DB, x, y);                     y+=d;
		draw_crc(surf, "DIPLOMACY_PROPOSAL", CRC_TYPE_DIPLOMACY_PROPOSAL_DB, x, y);   y+=d;
		draw_crc(surf, "DIPLOMACY_THREAT", CRC_TYPE_DIPLOMACY_THREAT_DB, x, y);       y+=d;
		draw_crc(surf, "END_GAME_OBJECT", CRC_TYPE_END_GAME_OBJECT_DB, x, y);         y+=d;
		draw_crc(surf, "FEAT", CRC_TYPE_FEAT_DB, x, y);                               y+=d;
		draw_crc(surf, "GLOBAL_WARMING", CRC_TYPE_GLOBAL_WARMING_DB, x, y);           y+=d;
		draw_crc(surf, "GOAL", CRC_TYPE_GOAL_DB, x, y);                               y+=d;
		draw_crc(surf, "GOVERNMENT", CRC_TYPE_GOVERNMENT_DB, x, y);                   y+=d;
		draw_crc(surf, "ICON", CRC_TYPE_ICON_DB, x, y);                               y+=d;
		draw_crc(surf, "IMPROVEMENT_LIST", CRC_TYPE_IMPROVEMENT_LIST_DB, x, y);       y+=d;
		draw_crc(surf, "ORDER", CRC_TYPE_ORDER_DB, x, y);                             y+=d;
		draw_crc(surf, "OZONE", CRC_TYPE_OZONE_DB, x, y);                             y+=d;
		draw_crc(surf, "PERSONALITY", CRC_TYPE_PERSONALITY_DB, x, y);                 y+=d;
		draw_crc(surf, "POLLUTION", CRC_TYPE_POLLUTION_DB, x, y);                     y+=d;
		draw_crc(surf, "POP", CRC_TYPE_POPULATION_DB, x, y);                          y+=d;
		draw_crc(surf, "RESOURCE", CRC_TYPE_RESOURCE_DB, x, y);                       y+=d;
		draw_crc(surf, "RISK", CRC_TYPE_RISK_DB, x, y);                               y+=d;
		draw_crc(surf, "SOUND", CRC_TYPE_SOUND_DB, x, y);                             y+=d;
		draw_crc(surf, "SPECIAL_ATTACK_INFO", CRC_TYPE_SPECIAL_ATTACK_INFO_DB, x, y); y+=d;
		draw_crc(surf, "SPECIAL_EFFECT", CRC_TYPE_SPECIAL_EFFECT_DB, x, y);           y+=d;
		draw_crc(surf, "SPRITE", CRC_TYPE_SPRITE_DB, x, y);                           y+=d;
		draw_crc(surf, "STRATEGY", CRC_TYPE_STRATEGY_DB, x, y);                       y+=d;
		draw_crc(surf, "TERRAIN", CRC_TYPE_TERRAIN_DB, x, y);                         y+=d;
		draw_crc(surf, "UNIT", CRC_TYPE_UNIT_DB, x, y);                               y+=d;
		draw_crc(surf, "UNIT_BUILD_LIST", CRC_TYPE_UNIT_BUILD_LIST_DB, x, y);         y+=d;
		draw_crc(surf, "WONDER", CRC_TYPE_WONDER_DB, x, y);                           y+=d;
		draw_crc(surf, "WONDER_BUILD_LIST", CRC_TYPE_WONDER_BUILD_LIST_DB, x, y);     y+=25;
	}

	x+=300;
	draw_crc(surf, "GLOBAL", CRC_TYPE_GLOBAL, x, y);                                     y+=d;
	draw_crc(surf, "RAND", CRC_TYPE_RAND, x, y);                                         y+=d;
	draw_crc(surf, "AGREEMENT_POOL", CRC_TYPE_AGREEMENTPOOL, x, y);                      y+=d;
	draw_crc(surf, "CIVILISATION_POOL", CRC_TYPE_CIVILISATIONPOOL, x, y);                y+=d;
	draw_crc(surf, "DIPLOMACY_REQUEST_POOL", CRC_TYPE_DIPLOMATICREQUESTPOOL, x, y);      y+=d;
	draw_crc(surf, "TERRAINIMPROVEMENT_POOL", CRC_TYPE_TERRAIN_IMPROVEMENT_POOL, x, y);  y+=d;
	draw_crc(surf, "MESSAGE_POOL", CRC_TYPE_MESSAGEPOOL, x, y);                          y+=d;
	draw_crc(surf, "TERRAIN_IMPROVEMENT_POOL", CRC_TYPE_TERRAIN_IMPROVEMENT_POOL, x, y); y+=d;
	draw_crc(surf, "TRADE_POOL", CRC_TYPE_TRADEPOOL, x, y);                              y+=d;
	draw_crc(surf, "TRADEOFFER_POOL", CRC_TYPE_TRADEOFFERPOOL, x, y);                    y+=d;
	draw_crc(surf, "UNIT_POOL", CRC_TYPE_UNITPOOL, x, y);                                y+=d;

	draw_crc(surf, "POLLUTION", CRC_TYPE_POLLUTION, x, y);                               y+=d;
	draw_crc(surf, "SELECTED", CRC_TYPE_SELECTED_ITEM, x, y);                            y+=d;
	draw_crc(surf, "TOPTEN", CRC_TYPE_TOPTEN, x, y);                                     y+=d;
	draw_crc(surf, "WORLD", CRC_TYPE_WORLD, x, y);                                       y+=d;
	draw_crc(surf, "PLAYER", CRC_TYPE_PLAYER, x, y);                                     y+=d;

	draw_time(surf, x, y);
}

void DataCheck::DumpSingleCRC(MBCHAR *grp, sint32 t)
{
	DPRINTF(k_DBG_INFO, ("%s     %08X %08X %08X %08X  %4.2lf\n", grp, m_crc[t][CRC_ARRAY_0], m_crc[t][CRC_ARRAY_1], m_crc[t][CRC_ARRAY_2], m_crc[t][CRC_ARRAY_3], (double)(m_time[t]) / CLOCKS_PER_SEC)) ;
}









void DataCheck::DumpChecksum()
{
	DumpSingleCRC("GLOBAL",                   CRC_TYPE_GLOBAL);
	DumpSingleCRC("RAND",                     CRC_TYPE_RAND);
	DumpSingleCRC("DB",                       CRC_TYPE_DB);
	DumpSingleCRC("PROFILE",                  CRC_TYPE_PROFILE_DB);
	DumpSingleCRC("STRING",                   CRC_TYPE_STRING_DB);
	DumpSingleCRC("ADVANCE",                  CRC_TYPE_ADVANCE_DB);
	DumpSingleCRC("ADVANCE_BRANCH",           CRC_TYPE_ADVANCE_BRANCH_DB);
	DumpSingleCRC("ADVANCE_LIST",             CRC_TYPE_ADVANCE_LIST_DB);
	DumpSingleCRC("AGE",                      CRC_TYPE_AGE_DB);
	DumpSingleCRC("AGE_CITY_STYLE",           CRC_TYPE_AGE_CITY_STYLE_DB);
	DumpSingleCRC("BUILD_LIST_SEQUENCE",      CRC_TYPE_BUILD_LIST_SEQUENCE_DB);
	DumpSingleCRC("BUILDING",                 CRC_TYPE_BUILDING_DB);
	DumpSingleCRC("BUILDING_BUILD_LIST",      CRC_TYPE_BUILDING_BUILD_LIST_DB);
	DumpSingleCRC("CITY_SIZE",                CRC_TYPE_CITY_SIZE_DB);
	DumpSingleCRC("CITY_STYLE",               CRC_TYPE_CITY_STYLE_DB);
	DumpSingleCRC("CIVILISATION",             CRC_TYPE_CIVILISATION_DB);
	DumpSingleCRC("CONST",                    CRC_TYPE_CONST_DB);
	DumpSingleCRC("DIFFICULTY",               CRC_TYPE_DIFFICULTY_DB);
	DumpSingleCRC("DIPLOMACY",                CRC_TYPE_DIPLOMACY_DB);
	DumpSingleCRC("DIPLOMACY_PROPOSAL",       CRC_TYPE_DIPLOMACY_PROPOSAL_DB);
	DumpSingleCRC("DIPLOMACY_THREAT",         CRC_TYPE_DIPLOMACY_THREAT_DB);
	DumpSingleCRC("END_GAME_OBJECT",          CRC_TYPE_END_GAME_OBJECT_DB);
	DumpSingleCRC("FEAT",                     CRC_TYPE_FEAT_DB);
	DumpSingleCRC("GLOBAL_WARMING",           CRC_TYPE_GLOBAL_WARMING_DB);
	DumpSingleCRC("GOAL",                     CRC_TYPE_GOAL_DB);
	DumpSingleCRC("GOVERNMENT",               CRC_TYPE_GOVERNMENT_DB);
	DumpSingleCRC("ICON",                     CRC_TYPE_ICON_DB);
	DumpSingleCRC("IMPROVEMENT_LIST",         CRC_TYPE_IMPROVEMENT_LIST_DB);
	DumpSingleCRC("ORDER",                    CRC_TYPE_ORDER_DB);
	DumpSingleCRC("OZONE",                    CRC_TYPE_OZONE_DB);
	DumpSingleCRC("PERSONALITY",              CRC_TYPE_PERSONALITY_DB);
	DumpSingleCRC("POLLUTION",                CRC_TYPE_POLLUTION_DB);
	DumpSingleCRC("POP",                      CRC_TYPE_POPULATION_DB);
	DumpSingleCRC("RESOURCE",                 CRC_TYPE_RESOURCE_DB);
	DumpSingleCRC("RISK",                     CRC_TYPE_RISK_DB);
	DumpSingleCRC("SOUND",                    CRC_TYPE_SOUND_DB);
	DumpSingleCRC("SPECIAL_ATTACK_INFO",      CRC_TYPE_SPECIAL_ATTACK_INFO_DB);
	DumpSingleCRC("SPECIAL_EFFECT",           CRC_TYPE_SPECIAL_EFFECT_DB);
	DumpSingleCRC("SPRITE",                   CRC_TYPE_SPRITE_DB);
	DumpSingleCRC("STRATEGY",                 CRC_TYPE_STRATEGY_DB);
	DumpSingleCRC("TERRAIN",                  CRC_TYPE_TERRAIN_DB);
	DumpSingleCRC("UNIT",                     CRC_TYPE_UNIT_DB);
	DumpSingleCRC("UNIT_BUILD_LIST",          CRC_TYPE_UNIT_BUILD_LIST_DB);
	DumpSingleCRC("WONDER",                   CRC_TYPE_WONDER_DB);
	DumpSingleCRC("WONDER_BUILD_LIST",        CRC_TYPE_WONDER_BUILD_LIST_DB);

	DumpSingleCRC("AGREEMENT_POOL",           CRC_TYPE_AGREEMENTPOOL);
	DumpSingleCRC("CIVILISATION_POOL",        CRC_TYPE_CIVILISATIONPOOL);
	DumpSingleCRC("DIPLOMACY_REQUEST_POOL",   CRC_TYPE_DIPLOMATICREQUESTPOOL);
	DumpSingleCRC("TERRAINIMPROVEMENT_POOL",  CRC_TYPE_TERRAIN_IMPROVEMENT_POOL);
	DumpSingleCRC("MESSAGE_POOL",             CRC_TYPE_MESSAGEPOOL);
	DumpSingleCRC("TERRAIN_IMPROVEMENT_POOL", CRC_TYPE_TERRAIN_IMPROVEMENT_POOL);
	DumpSingleCRC("TRADE_POOL",               CRC_TYPE_TRADEPOOL);
	DumpSingleCRC("TRADEOFFER_POOL",          CRC_TYPE_TRADEOFFERPOOL);
	DumpSingleCRC("UNIT_POOL",                CRC_TYPE_UNITPOOL);

	DumpSingleCRC("POLLUTION",                CRC_TYPE_POLLUTION);
	DumpSingleCRC("SELECTED",                 CRC_TYPE_SELECTED_ITEM);
	DumpSingleCRC("TOPTEN",                   CRC_TYPE_TOPTEN);
	DumpSingleCRC("WORLD",                    CRC_TYPE_WORLD);
	DumpSingleCRC("PLAYER",                   CRC_TYPE_PLAYER);

	DPRINTF(k_DBG_INFO, ("Total time %4.2lf\n", (double)(m_total_time) / CLOCKS_PER_SEC));
}
