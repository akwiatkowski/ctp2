#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#include "ctp/c3.h"
#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_item.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_ldl.h"

#include "gs/gameobj/TradeRoute.h"
#include "gs/gameobj/citydata.h"
#include "gs/database/StrDB.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/UnitData.h"
#include "tradewin.h"

#include "ui/aui_ctp2/c3_static.h"
#include "ui/aui_ctp2/c3_tradelistitem.h"
#include "ui/aui_ctp2/pattern.h"



c3_TradeListItem::c3_TradeListItem(AUI_ERRCODE *retval, TradeRoute *route, sint32 gold, sint32 resIndex, MBCHAR const *ldlBlock)
	:
	c3_ListItem( retval, ldlBlock),
	aui_ImageBase(ldlBlock),
	aui_TextBase(ldlBlock, (MBCHAR *)NULL)
{
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;

	*retval = InitCommonLdl(route, gold, resIndex, ldlBlock);
	Assert( AUI_SUCCESS(*retval) );
	if ( !AUI_SUCCESS(*retval) ) return;
}

AUI_ERRCODE c3_TradeListItem::InitCommonLdl(TradeRoute *route, sint32 gold, sint32 resIndex, MBCHAR const *ldlBlock)
{
	MBCHAR			block[ k_AUI_LDL_MAXBLOCK + 1 ];
	AUI_ERRCODE		retval;

	m_route = route;
	m_gold = gold;
	m_resIndex = resIndex;

	c3_Static		*subItem;

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "goodsName");
	subItem = new c3_Static(&retval, aui_UniqueId(), block);
	AddChild(subItem);

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "cityFrom");
	subItem = new c3_Static(&retval, aui_UniqueId(), block);
	AddChild(subItem);

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "cityTo");
	subItem = new c3_Static(&retval, aui_UniqueId(), block);
	AddChild(subItem);

	snprintf(block, sizeof(block), "%s.%s", ldlBlock, "goodsValue");
	subItem = new c3_Static(&retval, aui_UniqueId(), block);
	AddChild(subItem);

	Update();

	return AUI_ERRCODE_OK;
}

void c3_TradeListItem::Update(void)
{

	CityData		*srcCity, *destCity;
	MBCHAR			s[_MAX_PATH];

	c3_Static *subItem;

	if (!m_route)
	{

		subItem = (c3_Static *)GetChildByIndex(0);
		strlcpy(s, tradewin_GetResourceName(m_resIndex), sizeof(s));
		subItem->SetText(s);

		subItem = (c3_Static *)GetChildByIndex(1);
		subItem->SetText((char *)stringdb_Get()->GetNameStr("str_ldl_Local"));

		subItem = (c3_Static *)GetChildByIndex(2);
		subItem->SetText((char *)stringdb_Get()->GetNameStr("str_tbl_ldl_None"));
	}
	else
	{
		srcCity = m_route->GetSource().GetData()->GetCityData();
		destCity = m_route->GetDestination().GetData()->GetCityData();

		subItem = (c3_Static *)GetChildByIndex(0);
		strlcpy(s, stringdb_Get()->GetNameStr(m_route->GetResourceName()), sizeof(s));
		subItem->SetText(s);

		subItem = (c3_Static *)GetChildByIndex(1);
		subItem->SetText(srcCity->GetName());

		subItem = (c3_Static *)GetChildByIndex(2);
		subItem->SetText(destCity->GetName());
	}

	subItem = (c3_Static *)GetChildByIndex(3);
	snprintf(s, sizeof(s), "%ld", m_gold);
	subItem->SetText(s);
}

sint32 c3_TradeListItem::Compare(c3_ListItem *item2, uint32 column)
{
	c3_Static		*i1, *i2;

	if (column < 0) return 0;

	switch (column) {
	case 0:
	case 1:
	case 2:
		i1 = (c3_Static *)this->GetChildByIndex(column);
		i2 = (c3_Static *)item2->GetChildByIndex(column);

		return strcmp(i1->GetText(), i2->GetText());

		break;
	case 3:
		sint32	gold1 = m_gold,
				gold2 = ((c3_TradeListItem *)item2)->GetGold();
		if (gold1 < gold2) return -1;
		else if (gold1 > gold2) return 1;
		else return 0;
		break;
	}

	return 0;
}
