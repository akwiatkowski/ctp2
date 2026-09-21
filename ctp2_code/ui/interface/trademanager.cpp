//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Trade manager window
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
// - Corrected non-standard syntax.
// - Initialized local variables. (Sep 9th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include <memory>
#include "ui/interface/trademanager.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_ctp2/c3ui.h"

#include "ui/aui_ctp2/ctp2_Window.h"
#include "ui/aui_ctp2/ctp2_listbox.h"
#include "ui/aui_ctp2/ctp2_button.h"
#include "ui/aui_ctp2/ctp2_listitem.h"
#include "ui/aui_ctp2/ctp2_Static.h"
#include "ui/aui_ctp2/ctp2_hypertextbox.h"
#include "ui/aui_ctp2/ctp2_Tab.h"
#include "ui/aui_ctp2/ctp2_TabGroup.h"

#include "ResourceRecord.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/citydata.h"
#include "gs/gameobj/player.h"
#include "gs/gameobj/tradeutil.h"
#include "gs/gameobj/TradeRouteData.h"

#include "ui/aui_ctp2/SelItem.h"

#include "gfx/gfx_utils/pixelutils.h"
#include "gfx/gfx_utils/colorset.h"           // colorset_Get()
#include "ui/aui_common/aui_blitter.h"

#include "gs/events/GameEventManager.h"
#include "gs/events/GameEventUser.h"
#include "gs/gameobj/Events.h"
#include "gs/database/StrDB.h"
#include "gs/utility/stringutils.h"
#include "gs/slic/SlicContext.h"
#include "gs/gameobj/TradeRoute.h"
#include "IconRecord.h"
#include "ui/aui_ctp2/c3slider.h"

#include "ai/diplomacy/AgreementMatrix.h"
#include "ai/diplomacy/Diplomat.h"

#include "net/general/network.h"
#include "net/general/net_action.h"
#include "gfx/tilesys/tiledmap.h"

static std::unique_ptr<TradeManager> s_tradeManager;
static MBCHAR const *   s_tradeManagerBlock = "TradeManager";
static MBCHAR const *   s_tradeAdviceBlock  = "TradeAdvice";

#define k_MAX_CITIES_PER_GOOD 5 //make this a constDB? - it should be 6.12.2007

#define k_CITY_COL_INDEX 0
#define k_GOODICON_COL_INDEX 1
#define k_GOOD_COL_INDEX 2
#define k_TOCITY_COL_INDEX 3
#define k_NATION_COL_INDEX 4
#define k_PRICE_COL_INDEX 5
#define k_CARAVANS_COL_INDEX 6

#define k_CITY_COL_SUM_INDEX 0
#define k_PIRACY_COL_SUM_INDEX 1
#define k_GOODICON_COL_SUM_INDEX 2
#define k_GOOD_COL_SUM_INDEX 3
#define k_TOCITY_COL_SUM_INDEX 4
#define k_NATION_COL_SUM_INDEX 5
#define k_PRICE_COL_SUM_INDEX 6
#define k_CARAVANS_COL_SUM_INDEX 7

TradeManager::TradeManager(AUI_ERRCODE *err)
{
	m_window = (ctp2_Window *)aui_Ldl::BuildHierarchyFromRoot(s_tradeManagerBlock);
	Assert(m_window);
	if(!m_window) {
		*err = AUI_ERRCODE_INVALIDPARAM;
		return;
	}

	m_createList = (ctp2_ListBox *)aui_Ldl::GetObject(s_tradeManagerBlock, "TradeTabs.Market.TabPanel.List");
	m_summaryList = (ctp2_ListBox *)aui_Ldl::GetObject(s_tradeManagerBlock, "TradeTabs.Summary.TabPanel.List");

	Assert(m_createList);

	aui_Ldl::SetActionFuncAndCookie(s_tradeManagerBlock, "CloseButton", Close, nullptr);
	m_createButton = (ctp2_Button *)aui_Ldl::GetObject(s_tradeManagerBlock, "TradeTabs.Market.TabPanel.CreateRouteButton");

	aui_Ldl::SetActionFuncAndCookie(s_tradeManagerBlock, "TradeTabs.Market.TabPanel.CreateRouteButton", CreateRoute, (void *)nullptr);

	m_breakButton = (ctp2_Button *)aui_Ldl::GetObject(s_tradeManagerBlock, "TradeTabs.Summary.TabPanel.BreakRouteButton");

	aui_Ldl::SetActionFuncAndCookie(s_tradeManagerBlock, "TradeTabs.Summary.TabPanel.BreakRouteButton", CreateRoute, (void *)1);
	aui_Ldl::SetActionFuncAndCookie(s_tradeManagerBlock, "ShowAdviceButton", ShowAdvice, nullptr);

	if(m_createList) {
		m_createList->SetActionFuncAndCookie(ListSelect, nullptr);
	}

	if(m_summaryList) {
		m_summaryList->SetActionFuncAndCookie(SummaryListSelect, nullptr);
	}

	m_adviceWindow = (ctp2_Window *)aui_Ldl::BuildHierarchyFromRoot(s_tradeAdviceBlock);
	Assert(m_adviceWindow);

	if(m_adviceWindow) {
		m_adviceWindow->Move(m_window->X() - m_adviceWindow->Width(), m_window->Y());
		m_window->AddDockedWindow(m_adviceWindow);
		m_adviceWindow->SetDock(m_window);
	}

	m_ownCitiesButton = (ctp2_Button *)aui_Ldl::GetObject(s_tradeManagerBlock, "TradeTabs.Market.TabPanel.Filters.OwnButton");
	m_friendlyCitiesButton = (ctp2_Button *)aui_Ldl::GetObject(s_tradeManagerBlock, "TradeTabs.Market.TabPanel.Filters.FriendlyButton");
	m_allCitiesButton = (ctp2_Button *)aui_Ldl::GetObject(s_tradeManagerBlock, "TradeTabs.Market.TabPanel.Filters.AllButton");

	m_ownCitiesButton->SetActionFuncAndCookie(CityFilterButton, (void *)TRADE_CITIES_OWN);
	m_friendlyCitiesButton->SetActionFuncAndCookie(CityFilterButton, (void *)TRADE_CITIES_FRIENDLY);
	m_allCitiesButton->SetActionFuncAndCookie(CityFilterButton, (void *)TRADE_CITIES_ALL);

	m_showCities = TRADE_CITIES_OWN;
	m_ownCitiesButton->SetToggleState(true);

	m_citiesSlider = (C3Slider *)aui_Ldl::GetObject(s_tradeManagerBlock, "TradeTabs.Market.TabPanel.Filters.CitiesSlider");
	m_citiesSlider->SetActionFuncAndCookie(NumCitiesSlider, nullptr);

	m_numCitiesLabel = (ctp2_Static *)aui_Ldl::GetObject(s_tradeManagerBlock, "TradeTabs.Market.TabPanel.Filters.NumCities");

	SetNumCities(1);

	*err = AUI_ERRCODE_OK;
}

TradeManager::~TradeManager()
{
	aui_Ldl::DeleteHierarchyFromRoot(s_tradeManagerBlock);
	m_window = nullptr;

	aui_Ldl::DeleteHierarchyFromRoot(s_tradeAdviceBlock);
	m_adviceWindow = nullptr;

	m_createData.DeleteAll();
}

AUI_ERRCODE TradeManager::Initialize()
{
	if(s_tradeManager)
		return AUI_ERRCODE_OK;

	AUI_ERRCODE err = AUI_ERRCODE_OK;
	s_tradeManager = std::make_unique<TradeManager>(&err);
	Assert(err == AUI_ERRCODE_OK);

	return err;
}

AUI_ERRCODE TradeManager::Cleanup()
{
	if(s_tradeManager) {
		Hide();

		s_tradeManager.reset();
	}

	return AUI_ERRCODE_OK;
}

AUI_ERRCODE TradeManager::Display()
{
	if(!s_tradeManager)
		Initialize();

	Assert(s_tradeManager);
	if(!s_tradeManager) {
		return AUI_ERRCODE_HACK;
	}

	AUI_ERRCODE err = AUI_ERRCODE_INVALIDPARAM;
	Assert(s_tradeManager->m_window);
	if(s_tradeManager->m_window && !c3ui_Get()->GetWindow(s_tradeManager->m_window->Id())) {
		err = c3ui_Get()->AddWindow(s_tradeManager->m_window);
		Assert(err == AUI_ERRCODE_OK);
		if(err == AUI_ERRCODE_OK) {
			err = s_tradeManager->m_window->Show();
		}

		if(s_tradeManager->m_adviceWindow) {
			err = c3ui_Get()->AddWindow(s_tradeManager->m_adviceWindow);
			if(err == AUI_ERRCODE_OK) {
				err = s_tradeManager->m_adviceWindow->Show();
			}
		}

//		ctp2_Tab *tab = (ctp2_Tab *)aui_Ldl::GetObject(s_tradeManagerBlock, "TradeTabs.Market");
//		ctp2_TabGroup *tabGroup = (ctp2_TabGroup *)aui_Ldl::GetObject(s_tradeManagerBlock, "TradeTabs");






		s_tradeManager->Update();
	}
	return err;
}

AUI_ERRCODE TradeManager::Hide()
{
	if(!s_tradeManager)
		return AUI_ERRCODE_OK;

	Assert(s_tradeManager->m_window);
	if(!s_tradeManager->m_window)
		return AUI_ERRCODE_INVALIDPARAM;

	if(s_tradeManager->m_adviceWindow) {
		c3ui_Get()->RemoveWindow(s_tradeManager->m_adviceWindow->Id());
	}

	return c3ui_Get()->RemoveWindow(s_tradeManager->m_window->Id());
}

void TradeManager::SetMode(TRADE_MANAGER_MODE mode)
{
//	MBCHAR *tradeBlock="TradeManager:TradeTabs";
	ctp2_Tab *market = (ctp2_Tab *)aui_Ldl::GetObject(s_tradeManagerBlock, "TradeTabs.Market");
	ctp2_Tab *summary = (ctp2_Tab *)aui_Ldl::GetObject(s_tradeManagerBlock, "TradeTabs.Summary");
	ctp2_TabGroup *group = (ctp2_TabGroup *)aui_Ldl::GetObject(s_tradeManagerBlock, "TradeTabs");






	switch(mode) {
		case TRADE_MANAGER_MARKET:
			market->Activate();
			summary->Deactivate();
			group->SelectTab(market);








			break;
		case TRADE_MANAGER_SUMMARY:
			market->Deactivate();
			summary->Activate();
			group->SelectTab(summary);








			break;
		default:
			Assert(FALSE);
			break;
	}
}

void TradeManager::Notify()
{
	if(s_tradeManager && c3ui_Get()->GetWindow(s_tradeManager->m_window->Id())) {
		s_tradeManager->Update();
	}
}

void TradeManager::Update()
{
	UpdateCreateList(selitem_Get()->GetVisiblePlayer());
	UpdateSummaryList();
	UpdateAdviceWindow();

}

void TradeManager::UpdateCreateList(const PLAYER_INDEX & player_id)
{
	Assert(player_id >= 0 && player_id < k_MAX_PLAYERS);
	if(player_id < 0 || player_id >= k_MAX_PLAYERS) return;

	Player *    p = player_Get(player_id);
	Assert(p);
	if (!p) return;

	sint32 d;
	Unit maxCity[k_MAX_CITIES_PER_GOOD];

	m_createList->Clear();
	m_createData.DeleteAll();

	for (sint32 c = 0; c < p->m_all_cities->Num(); c++)
    {
		Unit city = p->m_all_cities->Access(c);

		for (sint32 g = 0; g < g_theResourceDB->NumRecords(); g++)
        {
			if(city.CD()->IsLocalResource(g)) {

				sint32 op;
				sint32 maxPrice[k_MAX_CITIES_PER_GOOD];

				Unit curDestCity;

				sint32 i;
				sint32 j;
				for(i = 0; i < k_MAX_CITIES_PER_GOOD; i++) {
					maxCity[i].m_id = 0;
					maxPrice[i] = 0;
				}


				if(!city.CD()->HasResource(g) &&
					city.CD()->IsSellingResourceTo(g, curDestCity) ) {
					tradeutil_GetTradeValue(player_id, curDestCity, g);

				//need to add something here where cities that have an improvement that needs a good will demand the good. May be increase the value of selling that good?




				}
				else {
					curDestCity.m_id = 0;

				}

				for(op = 1; op < k_MAX_PLAYERS; op++) {
					if(!player_Get(op)) continue;
					if(player_id != op && !p->HasContactWith(op)) continue;
					if(m_showCities == TRADE_CITIES_OWN && op != selitem_Get()->GetVisiblePlayer()) continue;
					if ((m_showCities == TRADE_CITIES_ALL)			&&
						(op != selitem_Get()->GetVisiblePlayer()) &&
						(AgreementMatrix::s_agreements.TurnsAtWar(player_id, op) >= 0)
					   )
						continue;

					if ((m_showCities == TRADE_CITIES_FRIENDLY)		&&
						(op != selitem_Get()->GetVisiblePlayer()) &&
						(!AgreementMatrix::s_agreements.HasAgreement
							(player_id, op, PROPOSAL_TREATY_PEACE)
						)
					   )
						continue;

					if(Diplomat::GetDiplomat(op).GetEmbargo(player_id))
						continue;

					for(d = 0; d < player_Get(op)->m_all_cities->Num(); d++) {
						Unit destCity = player_Get(op)->m_all_cities->Access(d);
						if(!(destCity.IsValid())) continue;
						if(!(destCity.GetVisibility() & (1 << player_id))) continue;
						if(destCity.m_id == city.m_id) continue;


						if(curDestCity.m_id == destCity.m_id) continue;

						sint32 price = tradeutil_GetTradeValue(player_id, destCity, g);
						for(i = 0; i < m_numCities; i++) {
							if(price > maxPrice[i]) {
								for(j = m_numCities - 1; j>= i; j--) {
									maxPrice[j] = maxPrice[j - 1];
									maxCity[j].m_id = maxCity[j - 1].m_id;
								}
								maxPrice[i] = price;
								maxCity[i] = destCity;
								break;
							}
						}
					}
				}

				for(i = 0; i < k_MAX_CITIES_PER_GOOD; i++) {
					if(maxPrice[i] > 0) {

						if(maxCity[i].m_id == curDestCity.m_id)
							continue;


						ctp2_ListItem * item =
                            (ctp2_ListItem *) aui_Ldl::BuildHierarchyFromRoot("CreateRouteItem");
						Assert(item);
						if(!item)
							break;

						auto data = std::make_unique<CreateListData>();
						data->m_source = city;
						data->m_resource = g;
						data->m_destination = maxCity[i];
						data->m_price = maxPrice[i];
						data->m_caravans = tradeutil_GetAccurateTradeDistance(city, maxCity[i]);
						data->m_curDestination.m_id = curDestCity.m_id;

						m_createData.AddTail(data.get());
						item->SetUserData(data.get());
						data.release();

						if (ctp2_Static * origin = (ctp2_Static *)item->GetChildByIndex(k_CITY_COL_INDEX))
                        {
						MBCHAR name[k_MAX_NAME_LEN + 1];
						strlcpy(name, city.GetName(), sizeof(name));
						origin->TextReloadFont();
							origin->GetTextFont()->TruncateString(name, origin->Width());
							origin->SetText(name);
						}

						if (ctp2_Static * icon = (ctp2_Static *)item->GetChildByIndex(k_GOODICON_COL_INDEX))
                        {
							const char *iconname = g_theResourceDB->Get(g)->GetIcon()->GetIcon();
							if (stricmp(iconname, "NULL") == 0)
                            {
								iconname = nullptr;
							}
							icon->SetImage(iconname);
						}

						if (ctp2_Static * good = (ctp2_Static *)item->GetChildByIndex(k_GOOD_COL_INDEX))
                        {
							good->SetText(g_theResourceDB->Get(g)->GetNameText());
							if (curDestCity.m_id != 0)
                            {
								good->SetTextColor(colorset_Get()->GetColorRef(COLOR_RED));
							}
						}

						if (ctp2_Static * dest = (ctp2_Static *)item->GetChildByIndex(k_TOCITY_COL_INDEX))
                        {
						MBCHAR name[k_MAX_NAME_LEN + 1];
						strlcpy(name, maxCity[i].GetName(), sizeof(name));
						dest->TextReloadFont();
							dest->GetTextFont()->TruncateString(name, dest->Width());
							dest->SetText(name);
						}

						MBCHAR buf[20];
						snprintf(buf, sizeof(buf), "%d", maxPrice[i]);
						if (ctp2_Static * price = (ctp2_Static *)item->GetChildByIndex(k_PRICE_COL_INDEX))
                        {
							price->SetText(buf);
						}

						if (ctp2_Static * count = (ctp2_Static *)item->GetChildByIndex(k_CARAVANS_COL_INDEX))
                        {
							snprintf(buf, sizeof(buf), "%d", data->m_caravans);
							count->SetText(buf);
						}

						if (ctp2_Static * nation = (ctp2_Static *)item->GetChildByIndex(k_NATION_COL_INDEX))
                        {
							nation->SetDrawCallbackAndCookie
                                (DrawNationColumn, reinterpret_cast<void *>(static_cast<intptr_t>(data->m_destination.GetOwner())));
						}

						item->SetCompareCallback(CompareCreateItems);

						m_createList->AddItem(item);
					}
				}
			}
		}
	}







	m_createButton->Enable(FALSE);
}

void TradeManager::UpdateAdviceWindow()
{
	Assert(m_adviceWindow);
	if(!m_adviceWindow) return;

	ctp2_Button *showButton = (ctp2_Button *)aui_Ldl::GetObject(s_tradeManagerBlock, "ShowAdviceButton");
	if(showButton) {
		if(c3ui_Get()->GetWindow(m_adviceWindow->Id())) {
			showButton->SetText(stringdb_Get()->GetNameStr("str_ldl_TradeHideAdvisor"));
		} else {
			showButton->SetText(stringdb_Get()->GetNameStr("str_ldl_TradeShowAdvisor"));
		}
	}

	MBCHAR buf[20];
	sint32 pl = selitem_Get()->GetVisiblePlayer();
	if(!player_Get(pl)) return;

	ctp2_Static *child = (ctp2_Static *)aui_Ldl::GetObject(s_tradeAdviceBlock, "Available");
	if(child) {
		snprintf(buf, sizeof(buf), "%d", player_Get(pl)->m_tradeTransportPoints - player_Get(pl)->m_usedTradeTransportPoints);
		child->SetText(buf);
	}

	child = (ctp2_Static *)aui_Ldl::GetObject(s_tradeAdviceBlock, "InUse");
	if(child) {
		snprintf(buf, sizeof(buf), "%d", player_Get(pl)->m_usedTradeTransportPoints);
		child->SetText(buf);
	}

	sint32 i;
	sint32 totalProfit = 0;
	sint32 totalRoutes = 0;
	for(i = 0; i < player_Get(pl)->m_all_cities->Num(); i++) {
		Unit city = player_Get(pl)->m_all_cities->Access(i);
		totalRoutes += city.CD()->GetTradeSourceList()->Num();
		sint32 r;
		for(r = 0; r < city.CD()->GetTradeSourceList()->Num(); r++) {
			totalProfit += city.CD()->GetTradeSourceList()->Access(r)->GetValue();
		}
	}

	child = (ctp2_Static *)aui_Ldl::GetObject(s_tradeAdviceBlock, "Profit");
	if(child) {
		snprintf(buf, sizeof(buf), "%d", totalProfit);
		child->SetText(buf);
	}

	child = (ctp2_Static *)aui_Ldl::GetObject(s_tradeAdviceBlock, "Routes");
	if(child) {
		snprintf(buf, sizeof(buf), "%d", totalRoutes);
		child->SetText(buf);
	}

	UpdateAdviceText();
}

void TradeManager::UpdateAdviceText()
{
	ctp2_HyperTextBox *advice = (ctp2_HyperTextBox *)aui_Ldl::GetObject(s_tradeAdviceBlock, "Advice");
	Assert(advice);
	if(advice) {
		Assert(m_createList);

		SlicContext sc;

		Player *p = player_Get(selitem_Get()->GetVisiblePlayer());

		if(m_createList) {
			ctp2_ListItem *selItem = (ctp2_ListItem *)m_createList->GetSelectedItem();
			if(selItem && !m_createList->IsHidden()) {
				CreateListData *data = (CreateListData *)selItem->GetUserData();
				sc.AddCity(data->m_source);
				sc.AddCity(data->m_destination);
				sc.AddGood(data->m_resource);
				sc.AddGold(data->m_price);
				sc.AddInt(data->m_caravans);

				MBCHAR interp[k_MAX_NAME_LEN];
				interp[0] = 0;

				stringutils_Interpret(stringdb_Get()->GetNameStr("SELECTED_TRADE_ADVICE"),
									  sc, interp);

				if(p) {
					if(data->m_caravans > (p->m_tradeTransportPoints - p->m_usedTradeTransportPoints)) {
						SlicContext sc2;
						sc2.AddInt(data->m_caravans - (p->m_tradeTransportPoints - p->m_usedTradeTransportPoints));

						strlcat(interp, "  ", sizeof(interp));

						stringutils_Interpret(stringdb_Get()->GetNameStr("NEED_MORE_CARAVANS"),
											  sc2, interp + strlen(interp));
					}
				}

				advice->SetHyperText(interp);
			} else {

				if(p) {
					PointerList<CreateListData>::Walker walk(&m_createData);
					CreateListData *maxData = nullptr;
					while(walk.IsValid()) {
						CreateListData *data = walk.GetObj();
						if(data->m_curDestination.m_id != 0) {

							walk.Next();
							continue;
						}
						if(data->m_caravans <= (p->m_tradeTransportPoints - p->m_usedTradeTransportPoints)) {
							if(!maxData || data->m_price > maxData->m_price) {
								maxData = data;
							}
						}
						walk.Next();
					}

					MBCHAR interp[k_MAX_NAME_LEN];
					interp[0] = 0;

					if(maxData) {
						SlicContext sc;
						sc.AddCity(maxData->m_source);
						sc.AddCity(maxData->m_destination);
						sc.AddGood(maxData->m_resource);

						stringutils_Interpret(stringdb_Get()->GetNameStr("CREATE_ROUTE_ADVICE"),
											  sc, interp);
					} else if(m_createData.GetCount() > 0) {
						strlcpy(interp, stringdb_Get()->GetNameStr("BUILD_MORE_CARAVANS"), sizeof(interp));
					} else {
						strlcpy(interp, stringdb_Get()->GetNameStr("MAXIMUM_TRADE_EFFICIENCY"), sizeof(interp));
					}

					advice->SetHyperText(interp);
				}
			}
		}
	}
}

void TradeManager::UpdateSummaryList()
{
	sint32 pl = selitem_Get()->GetVisiblePlayer();
	Assert(pl >= 0 && pl < k_MAX_PLAYERS);
	if(pl < 0 || pl >= k_MAX_PLAYERS) return;

	Assert(player_Get(pl));
	if(!player_Get(pl)) return;

	Player *p = player_Get(pl);
	Unit maxCity;

	m_summaryList->Clear();

	for (sint32 c = 0; c < p->m_all_cities->Num(); c++)
    {
		Unit city = p->m_all_cities->Access(c);

		for (sint32 r = 0; r < city.CD()->GetTradeSourceList()->Num(); r++)
        {
			ctp2_ListItem * item = (ctp2_ListItem *)
                aui_Ldl::BuildHierarchyFromRoot("TradeSummaryItem");
			Assert(item);
			if(!item)
				break;

			TradeRoute route = city.CD()->GetTradeSourceList()->Access(r);

			if (ctp2_Static * origin = (ctp2_Static *)item->GetChildByIndex(k_CITY_COL_SUM_INDEX))
            {
				MBCHAR name[k_MAX_NAME_LEN + 1];
				strlcpy(name, city.GetName(), sizeof(name));
				origin->TextReloadFont();
				origin->GetTextFont()->TruncateString(name, origin->Width());
				origin->SetText(name);
			}

			ROUTE_TYPE rtype;
			sint32 resource;
			route.GetSourceResource(rtype, resource);

			if (ctp2_Static * icon = (ctp2_Static *)item->GetChildByIndex(k_GOODICON_COL_SUM_INDEX))
            {
				if (rtype == ROUTE_TYPE_RESOURCE)
                {
					const MBCHAR *imageName = g_theResourceDB->Get(resource)->GetIcon()->GetIcon();
					if (stricmp(imageName, "NULL") == 0)
                    {
						icon->SetImage(nullptr);
					}
                    else
                    {
						icon->SetImage(g_theResourceDB->Get(resource)->GetIcon()->GetIcon());
					}
				}
			}

			if (ctp2_Static * good = (ctp2_Static *)item->GetChildByIndex(k_GOOD_COL_SUM_INDEX))
            {
				if (rtype == ROUTE_TYPE_RESOURCE)
                {
					good->SetText(g_theResourceDB->Get(resource)->GetNameText());
				}
                else
                {
					good->SetText(stringdb_Get()->GetNameStr("ROUTE_TYPE_FOOD"));
				}
			}

			if (ctp2_Static * dest = (ctp2_Static *)item->GetChildByIndex(k_TOCITY_COL_SUM_INDEX))
            {
				MBCHAR name[k_MAX_NAME_LEN + 1];
				Unit dCity = route.GetDestination();
				strlcpy(name, dCity.GetName(), sizeof(name));
				dest->TextReloadFont();
				dest->GetTextFont()->TruncateString(name, dest->Width());
				dest->SetText(name);
			}

			if (ctp2_Static * piracy = (ctp2_Static *)item->GetChildByIndex(k_PIRACY_COL_SUM_INDEX))
            {
				piracy->SetDrawCallbackAndCookie(DrawPiracyColumn, reinterpret_cast<void *>(static_cast<intptr_t>(route.m_id)));
			}

			MBCHAR buf[20];
			if(rtype == ROUTE_TYPE_RESOURCE) {
				snprintf(buf, sizeof(buf), "%d", route->GetValue());
			} else {
				strlcpy(buf, "---", sizeof(buf));
			}

			if (ctp2_Static * price = (ctp2_Static *)item->GetChildByIndex(k_PRICE_COL_SUM_INDEX))
            {
				price->SetText(buf);
			}

			if (ctp2_Static * count = (ctp2_Static *)item->GetChildByIndex(k_CARAVANS_COL_SUM_INDEX))
            {
				snprintf(buf, sizeof(buf), "%.0f", route.GetCost());
				count->SetText(buf);
			}

			if (ctp2_Static * nation = (ctp2_Static *)item->GetChildByIndex(k_NATION_COL_SUM_INDEX))
            {
				nation->SetDrawCallbackAndCookie
                    (DrawNationColumn, reinterpret_cast<void *>(static_cast<intptr_t>(route.GetDestination().GetOwner())));
			}

			item->SetUserData(reinterpret_cast<void *>(static_cast<intptr_t>(route.m_id)));
			item->SetCompareCallback(CompareSummaryItems);

			m_summaryList->AddItem(item);
		}
	}

	m_breakButton->Enable(FALSE);
}

void TradeManager::Close(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if(action != AUI_BUTTON_ACTION_EXECUTE) return;

	TradeManager::Hide();
}

void TradeManager::CreateRoute(aui_Control *control, uint32 action, uint32 uidata, void *cookie)
{
	if(action != AUI_BUTTON_ACTION_EXECUTE) return;

	Assert(s_tradeManager);
	if(!s_tradeManager) return;
	bool breakInstead = (intptr_t)cookie != 0;

//	ctp2_Static *market = (ctp2_Static *)aui_Ldl::GetObject(s_tradeManagerBlock, "Market");
	if(!breakInstead) {
		Assert(s_tradeManager->m_createList);
		if(!s_tradeManager->m_createList) return;

		ctp2_ListItem *item = (ctp2_ListItem *)s_tradeManager->m_createList->GetSelectedItem();
		if(!item) return;

		CreateListData *data = (CreateListData *)item->GetUserData();

		Assert(data);
		if(!data) return;

		Assert(data->m_source.IsValid());
		if(!data->m_source.IsValid()) return;

		Assert(data->m_destination.IsValid());
		if(!data->m_destination.IsValid()) return;

		Assert(player_Get(data->m_source.GetOwner()));
		if(!player_Get(data->m_source.GetOwner())) return;

		gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_SendGood,
							   GEA_Int, data->m_resource,
							   GEA_City, data->m_source,
							   GEA_City, data->m_destination,
							   GEA_End);
	} else {

		Assert(s_tradeManager->m_summaryList);
		if(!s_tradeManager->m_summaryList) return;

		ctp2_ListItem *item = (ctp2_ListItem *)s_tradeManager->m_summaryList->GetSelectedItem();
		if(!item) return;

		TradeRoute route(static_cast<uint32>((uintptr_t)item->GetUserData()));
		Assert(route.IsValid());
		if(!route.IsValid()) return;

		gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_KillTradeRoute,
							   GEA_TradeRoute, route.m_id,
							   GEA_Int, CAUSE_KILL_TRADE_ROUTE_SENDER_KILLED,
							   GEA_End);

		if(network_Get().IsClient()) {
			network_Get().SendAction(std::make_unique<NetAction>(NET_ACTION_CANCEL_TRADE_ROUTE,
											   (uint32)route).release());
		}
	}
}

void TradeManager::ShowAdvice(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if(action != AUI_BUTTON_ACTION_EXECUTE) return;

	if(!s_tradeManager->m_adviceWindow) return;

	if(c3ui_Get()->GetWindow(s_tradeManager->m_adviceWindow->Id())) {
		c3ui_Get()->RemoveWindow(s_tradeManager->m_adviceWindow->Id());
	} else {
		c3ui_Get()->AddWindow(s_tradeManager->m_adviceWindow);
	}
	s_tradeManager->UpdateAdviceWindow();
}

void TradeManager::Summary(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if(action != AUI_BUTTON_ACTION_EXECUTE) return;

	Assert(s_tradeManager);
	if(!s_tradeManager)
		return;

	ctp2_Static *market = (ctp2_Static *)aui_Ldl::GetObject(s_tradeManagerBlock, "Market");
	Assert(market);
	if(market && market->IsHidden()) {
		TradeManager::SetMode(TRADE_MANAGER_MARKET);
	} else {
		TradeManager::SetMode(TRADE_MANAGER_SUMMARY);
	}
}

sint32 TradeManager::CompareCreateItems(ctp2_ListItem *item1, ctp2_ListItem *item2, sint32 column)
{
	CreateListData *data1 = (CreateListData *)item1->GetUserData();
	CreateListData *data2 = (CreateListData *)item2->GetUserData();

	Assert(data1 && data2);
	if(!data1 || !data2) return 0;

	switch(column) {
		case k_CITY_COL_INDEX:
			Assert(data1->m_source.IsValid());
			Assert(data2->m_source.IsValid());
			if(data1->m_source.IsValid() && data2->m_source.IsValid()) {
				return stricmp(data1->m_source.GetName(), data2->m_source.GetName());
			}
			return 0;
		case k_GOOD_COL_INDEX:
		case k_GOODICON_COL_INDEX:
			return stricmp(g_theResourceDB->Get(data1->m_resource)->GetNameText(),
						   g_theResourceDB->Get(data2->m_resource)->GetNameText());
		case k_TOCITY_COL_INDEX:
			Assert(data1->m_destination.IsValid());
			Assert(data2->m_destination.IsValid());
			if(data1->m_destination.IsValid() && data2->m_destination.IsValid()) {
				return stricmp(data1->m_destination.GetName(), data2->m_destination.GetName());
			}
			return 0;
		case k_PRICE_COL_INDEX:
			return data1->m_price - data2->m_price;
		case k_CARAVANS_COL_INDEX:
			return data1->m_caravans - data2->m_caravans;
		case k_NATION_COL_INDEX:
			Assert(data1->m_destination.IsValid());
			Assert(data2->m_destination.IsValid());
			if(data1->m_destination.IsValid() && data2->m_destination.IsValid()) {
				if(data1->m_destination.GetOwner() == data2->m_destination.GetOwner()) {
					return stricmp(data1->m_destination.GetName(), data2->m_destination.GetName());
				} else {
					return data1->m_destination.GetOwner() - data2->m_destination.GetOwner();
				}
			}
			return 0;
		default:
			Assert(FALSE);
			return 0;
	}
}

sint32 TradeManager::CompareSummaryItems(ctp2_ListItem *item1, ctp2_ListItem *item2, sint32 column)
{
	TradeRoute route1 = TradeRoute(static_cast<uint32>((uintptr_t)item1->GetUserData()));
	TradeRoute route2 = TradeRoute(static_cast<uint32>((uintptr_t)item2->GetUserData()));

	Assert(route1.IsValid());
	Assert(route2.IsValid());
	if(!route1.IsValid() || !route2.IsValid()) {
		return 0;
	}

	ROUTE_TYPE rtype1;
	ROUTE_TYPE rtype2;
	sint32 resource1;
	sint32 resource2;

	route1.GetSourceResource(rtype1, resource1);
	route2.GetSourceResource(rtype2, resource2);

	switch(column) {
		case k_CITY_COL_SUM_INDEX:
			Assert(route1.GetSource().IsValid());
			Assert(route2.GetSource().IsValid());
			if(route1.GetSource().IsValid() && route2.GetSource().IsValid()) {
				return stricmp(route1.GetSource().GetName(), route2.GetSource().GetName());
			}
			return 0;
		case k_PIRACY_COL_SUM_INDEX:
		{
			sint32 pl1 = -1;
			sint32 pl2 = -1;
			if(route1->IsBeingPirated()) {
				pl1 = route1->GetPiratingArmy().GetOwner();
			}

			if(route2->IsBeingPirated()) {
				pl2 = route2->GetPiratingArmy().GetOwner();
			}

			return pl1 - pl2;
		}

		case k_GOOD_COL_SUM_INDEX:
		case k_GOODICON_COL_SUM_INDEX:
		{
			const MBCHAR *str1 = rtype1 == ROUTE_TYPE_RESOURCE ? g_theResourceDB->Get(resource1)->GetNameText() :
				stringdb_Get()->GetNameStr("ROUTE_TYPE_FOOD");
			const MBCHAR *str2 = rtype2 == ROUTE_TYPE_RESOURCE ? g_theResourceDB->Get(resource2)->GetNameText() :
				stringdb_Get()->GetNameStr("ROUTE_TYPE_FOOD");
			return stricmp(str1, str2);
		}
		case k_TOCITY_COL_SUM_INDEX:
			Assert(route1.GetDestination().IsValid());
			Assert(route2.GetDestination().IsValid());
			if(route1.GetDestination().IsValid() && route2.GetDestination().IsValid()) {
				return stricmp(route1.GetDestination().GetName(), route2.GetDestination().GetName());
			}
			return 0;
		case k_PRICE_COL_SUM_INDEX:
			return route1->GetValue() - route2->GetValue();
		case k_CARAVANS_COL_SUM_INDEX:
			return sint32(route1.GetCost() - route2.GetCost());
		case k_NATION_COL_SUM_INDEX:
			Assert(route1.GetDestination().IsValid());
			Assert(route2.GetDestination().IsValid());
			if(route1.GetDestination().IsValid() && route2.GetDestination().IsValid()) {
				if(route1.GetDestination().GetOwner() == route2.GetDestination().GetOwner()) {
					return stricmp(route1.GetDestination().GetName(), route2.GetDestination().GetName());
				} else {
					return route1.GetDestination().GetOwner() - route2.GetDestination().GetOwner();
				}
			}
			return 0;
		default:
			Assert(FALSE);
			return 0;
	}
}

AUI_ERRCODE TradeManager::DrawNationColumn(ctp2_Static *control,
										   aui_Surface *surface,
										   RECT &rect,
										   void *cookie)
{
	sint32 player = (intptr_t)cookie;
	Assert(colorset_Get());
	if(!colorset_Get())
		return AUI_ERRCODE_INVALIDPARAM;
	rect.left += 2;
	rect.top += 2;
	rect.right -= 2;
	rect.bottom -= 2;
	c3ui_Get()->TheBlitter()->ColorBlt16(surface, &rect, 0, 0);

	rect.left += 8;
	rect.top += 2;
	rect.right -= 8;
	rect.bottom -= 2;

	return c3ui_Get()->TheBlitter()->ColorBlt16(surface, &rect, colorset_Get()->GetPlayerColor(player), 0);
}

AUI_ERRCODE TradeManager::DrawPiracyColumn(ctp2_Static *control,
										   aui_Surface *surface,
										   RECT &rect,
										   void *cookie)
{
	TradeRoute route(static_cast<uint32>((uintptr_t)cookie));
	if(!route.IsValid()) return AUI_ERRCODE_OK;

	Pixel16 color = 0xffff;

	rect.left += 2;
	rect.top += 2;
	rect.right -= 2;
	rect.bottom -= 2;

	if(route->IsBeingPirated()) {
		color = colorset_Get()->GetPlayerColor(route->GetPiratingArmy().GetOwner());
	}
	return c3ui_Get()->TheBlitter()->ColorBlt16(surface, &rect, color, 0);
}

void TradeManager::ListSelect(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if(action != AUI_LISTBOX_ACTION_SELECT && action != AUI_LISTBOX_ACTION_DOUBLECLICKSELECT) {
		return;
	}

	Assert(s_tradeManager);
	if(!s_tradeManager) return;

	ctp2_ListBox *lb = (ctp2_ListBox *)control;
	Assert(lb == s_tradeManager->m_createList);
	if(lb != s_tradeManager->m_createList) return;

	s_tradeManager->UpdateAdviceText();

	ctp2_ListItem *item = (ctp2_ListItem *)lb->GetSelectedItem();
	bool canCreate = false;
	if(item) {
		CreateListData *data = (CreateListData *)item->GetUserData();
		Assert(data);
		if(data) {
			Player *pl = player_Get(selitem_Get()->GetVisiblePlayer());
			Assert(pl);
			if(pl && (data->m_caravans <= pl->m_tradeTransportPoints - pl->m_usedTradeTransportPoints)) {
				canCreate = true;
			}
		}
	}
	s_tradeManager->m_createButton->Enable(canCreate);

	if(action == AUI_LISTBOX_ACTION_DOUBLECLICKSELECT) {

		CreateRoute(nullptr, AUI_BUTTON_ACTION_EXECUTE, 0, nullptr);
	}
}

void TradeManager::SummaryListSelect(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if(action != AUI_LISTBOX_ACTION_SELECT && action != AUI_LISTBOX_ACTION_DOUBLECLICKSELECT) {
		return;
	}

	Assert(s_tradeManager);
	if(!s_tradeManager) return;

	ctp2_ListBox *lb = (ctp2_ListBox *)control;
	Assert(lb == s_tradeManager->m_summaryList);
	if(lb != s_tradeManager->m_summaryList) return;

	ctp2_ListItem *item = (ctp2_ListItem *)lb->GetSelectedItem();
	if(item) {
		s_tradeManager->m_breakButton->Enable(TRUE);
	} else {
		s_tradeManager->m_breakButton->Enable(FALSE);
	}
	if(action == AUI_LISTBOX_ACTION_DOUBLECLICKSELECT) {

		CreateRoute(nullptr, AUI_BUTTON_ACTION_EXECUTE, 0, nullptr);
	}
}

STDEHANDLER(TradeManagerSendGoodEvent)
{
	if(!s_tradeManager) return GEV_HD_Continue;

	Unit source;
	Unit destination;
	sint32 good;
	if(!args->GetInt(0, good)) return GEV_HD_Continue;
	if(!args->GetCity(0, source)) return GEV_HD_Continue;
	if(!args->GetCity(1, destination)) return GEV_HD_Continue;

	if(source.GetOwner() == selitem_Get()->GetVisiblePlayer() ||
	   destination.GetOwner() == selitem_Get()->GetVisiblePlayer()) {
		s_tradeManager->Update();
	}
	return GEV_HD_Continue;
}

AUI_ACTION_BASIC(UpdateTradeAction);

void UpdateTradeAction::Execute(aui_Control *control, uint32 action, uint32 data)
{

	if(s_tradeManager) {
		s_tradeManager->Update();
	}
}

STDEHANDLER(TradeManagerKillRouteEvent)
{
	if(!s_tradeManager) return GEV_HD_Continue;

	TradeRoute route;
	if(!args->GetTradeRoute(0, route)) return GEV_HD_Continue;

	if((route.GetSource().IsValid() && route.GetSource().GetOwner() == selitem_Get()->GetVisiblePlayer()) ||
	   (route.GetDestination().IsValid() && route.GetDestination().GetOwner() == selitem_Get()->GetVisiblePlayer())) {


		c3ui_Get()->AddAction(std::make_unique<UpdateTradeAction>());
	}
	return GEV_HD_Continue;
}

void TradeManager::InitializeEvents()
{
	gevmanager_Get()->AddCallback(GEV_SendGood, GEV_PRI_Post, &s_TradeManagerSendGoodEvent);
	gevmanager_Get()->AddCallback(GEV_KillTradeRoute, GEV_PRI_Pre, &s_TradeManagerKillRouteEvent);
}

void TradeManager::CleanupEvents()
{
}

void TradeManager::FilterButtonActivated(aui_Control *control)
{
	if(control == m_ownCitiesButton) {
		m_showCities = TRADE_CITIES_OWN;
		m_ownCitiesButton->SetToggleState(true);
		m_friendlyCitiesButton->SetToggleState(false);
		m_allCitiesButton->SetToggleState(false);
	} else if(control == m_friendlyCitiesButton) {
		m_showCities = TRADE_CITIES_FRIENDLY;
		m_ownCitiesButton->SetToggleState(false);
		m_friendlyCitiesButton->SetToggleState(true);
		m_allCitiesButton->SetToggleState(false);
	} else if(control == m_allCitiesButton) {
		m_showCities = TRADE_CITIES_ALL;
		m_ownCitiesButton->SetToggleState(false);
		m_friendlyCitiesButton->SetToggleState(false);
		m_allCitiesButton->SetToggleState(true);
	} else {

		Assert(FALSE);
	}

	UpdateCreateList(selitem_Get()->GetVisiblePlayer());
}

void TradeManager::CityFilterButton(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if(action != AUI_BUTTON_ACTION_EXECUTE) return;

	if(!s_tradeManager) return;
	s_tradeManager->FilterButtonActivated(control);
}

void TradeManager::SetNumCities(sint32 num)
{
	Assert(num <= k_MAX_CITIES_PER_GOOD);
    m_numCities = std::min<sint32>(num, k_MAX_CITIES_PER_GOOD);

	char buf[10];
	snprintf(buf, sizeof(buf), "%d", m_numCities);
	m_numCitiesLabel->SetText(buf);
}

void TradeManager::NumCitiesSlider(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if(!s_tradeManager) return;
	switch(action) {
		case AUI_RANGER_ACTION_VALUECHANGE:
			s_tradeManager->SetNumCities(s_tradeManager->m_citiesSlider->GetValueX() + 1);
			break;
		case AUI_RANGER_ACTION_RELEASE:
			s_tradeManager->UpdateCreateList(selitem_Get()->GetVisiblePlayer());
			break;
	}
}
