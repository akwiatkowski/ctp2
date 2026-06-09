// UI-side rendering for TradePool. Lives here rather than in
// gs/gameobj/TradePool.cpp so the engine_only build does not pull in
// aui_Surface or the citylayer DrawTradeRoute helper.

#include "ctp/c3.h"
#include "gs/gameobj/TradePool.h"
#include "gs/gameobj/TradeRoute.h"
#include "gs/utility/TradeDynArr.h"
#include "gs/database/profileDB.h"
#include "gs/core/colorset_observer.h"

class aui_Surface;
class MapPoint;
template <class T> class DynamicArray;

void DrawTradeRoute(aui_Surface *pSurface, DynamicArray<MapPoint> *pRoute,
                    uint16 route, uint16 outline);

void TradePool::Draw(aui_Surface* surface)
{
	if(!profiledb_Get()->GetShowTradeRoutes())
		return;

	if (!m_all_routes)
		return;

	sint32 num = m_all_routes->Num();

	for (sint32 i = 0; i < num; i++) {
		TradeRoute route = m_all_routes->Access(i);

		DrawTradeRoute(surface, (DynamicArray<MapPoint>*)m_all_routes->Access(i).GetPath(),
			colorset_observer::GetPlayerColor(route.GetOwner()),
			(uint16)route.GetOutlineColor());
	}
}
