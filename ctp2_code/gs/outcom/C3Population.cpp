#include "ctp/c3.h"

#include "gs/outcom/C3Population.h"

#include "gs/gameobj/Player.h"
#include "gs/gameobj/XY_Coordinates.h"
#include "gs/world/World.h"
#include "civarchive.h"
#include "gs/database/DB.h"

enum POPTYPE {
	POPTYPE_WORKER,
	POPTYPE_MUSICIAN,
	POPTYPE_SCIENTIST,
	POPTYPE_GRUNT,
	POPTYPE_SLAVE,
	POPTYPE_BANKER
};

	extern	World	*g_theWorld ;

	extern	Player	**g_player ;

#include "gs/gameobj/UnitData.h"
#include "gs/gameobj/citydata.h"
#include "WonderRecord.h"
#include "gs/gameobj/Happy.h"
#include "ConstDB.h"

static CityData *GetCityData(Player *player, uint32 city_id, BOOL *is_unknown_id)
{
    *is_unknown_id = TRUE;

    sint32 n = player->m_all_cities->Num();
    for (sint32 i = 0; i < n; i++) {
        Unit city = player->m_all_cities->Get(i);
        if (city.m_id == city_id) {
            *is_unknown_id = FALSE;
            return city.GetData()->GetCityData();
        }
    }

    return NULL;
}








STDMETHODIMP C3Population::QueryInterface(REFIID riid, void **obj)
	{
	*obj = NULL;

	if(IsEqualIID(riid, IID_IUnknown))
		{
		*obj = (IUnknown *)this;
		AddRef();
		return S_OK;
		}
	else if(IsEqualIID(riid, CLSID_IC3Population))
		{
		*obj = (IC3Population*)this;
		AddRef();
		return S_OK;
		}

	return E_NOINTERFACE;
	}









STDMETHODIMP_(ULONG) C3Population::AddRef()
	{
	return ++m_refCount;
	}









STDMETHODIMP_(ULONG) C3Population::Release()
	{
	if (--m_refCount)
		return (m_refCount) ;

	delete this ;

	return (0) ;
	}









C3Population::C3Population(sint32 idx)
{
    m_refCount = 0 ;
    Assert(idx >= 0) ;
    Assert(idx < k_MAX_PLAYERS) ;

    m_owner = idx ;
    Assert(g_player[idx]) ;
    m_player = g_player[idx] ;
    Assert(m_player) ;

}








C3Population::C3Population(CivArchive &archive)
{

    Serialize(archive) ;
}









void C3Population::Serialize(CivArchive &archive)
	{

    CHECKSERIALIZE

    if (archive.IsStoring())
		{
        archive << m_refCount ;
		archive << m_owner ;
		}
	else
		{
        archive >> m_refCount ;
		archive >> m_owner ;
		m_player = g_player[m_owner] ;
		}

	}









BOOL C3Population::HasPopChanged(uint32 city_id, BOOL *is_unknown_id)
	{
	return FALSE;
	}









BOOL C3Population::HasPopGrown(uint32 city_id, BOOL *is_unknown_id)
	{
	return FALSE;
	}









BOOL C3Population::HasPopStarved(uint32 city_id, BOOL *is_unknown_id)
	{
	return FALSE;
	}









BOOL C3Population::WasPopExpelled(uint32 city_id, BOOL *is_unknown_id)
	{
	return FALSE;
	}









BOOL C3Population::WasImprovementBuilt(uint32 city_id, BOOL *is_unknown_id)
	{
	return FALSE;
	}









BOOL C3Population::WasTerrainImprovementBuilt(uint32 city_id, BOOL *is_unknown_id)
	{
	return FALSE;
	}









BOOL C3Population::WasHappinessAttacked(uint32 city_id, BOOL *is_unknown_id)
	{
	return FALSE;
	}









BOOL C3Population::WasTerrainPolluted(void)
	{
	return (m_player->WasTerrainPolluted()) ;
	}














sint32 C3Population::GetCityPopCount(uint32 city_id, BOOL *is_unknown_id)
	{
	CityData *cd = GetCityData(m_player, city_id, is_unknown_id);
	if (cd == NULL) return 0;
	return cd->PopCount();
	}









sint32 C3Population::GetCitySlaveCount(uint32 city_id, BOOL *is_unknown_id)
	{
	CityData *cd = GetCityData(m_player, city_id, is_unknown_id);
	if (cd == NULL) return 0;
	return cd->SlaveCount();
	}









void C3Population::SetCityPopInCity(uint32 city_id, uint32 pop_idx, BOOL inCity, BOOL *is_unknown_id)
{
}









BOOL C3Population::GetCityPopIsSlave(uint32 city_id, uint32 pop_idx, BOOL *is_unknown_id)
{
	return FALSE;
}









void C3Population::SetCityPopType(uint32 city_id, uint32 pop_idx, uint32 popType, BOOL *is_unknown_id)
{
}

















void C3Population::GetRawHappiness(uint32 city_id, BOOL *is_unknown_id,
    double *raw_happiness, double *happy_per_entertainer)

{
    CityData *the_city = GetCityData(m_player, city_id, is_unknown_id);
    if (the_city == NULL)
        return;

    *raw_happiness = the_city->m_happy->GetGreedyPopHappiness(*the_city);

    *happy_per_entertainer = 0;

}








sint32 C3Population::GetTileFood(uint32 city_id, MapPointData *pos, BOOL *is_unknown_id)
	{
	return 0;
	}









sint32 C3Population::GetTileProduction(uint32 city_id, MapPointData *pos, BOOL *is_unknown_id)
	{
	return 0;
	}









sint32 C3Population::GetTileResource(uint32 city_id, MapPointData *pos, BOOL *is_unknown_id)
	{
	return 0;
	}









void C3Population::GetCityProjectedFood(uint32 city_id, sint32 *food, BOOL *is_unknown_id)
	{
	CityData *cd = GetCityData(m_player, city_id, is_unknown_id);
	if (cd == NULL) {
		*food = 0;
		return;
	}
	*food = cd->GetStoredCityFood();
	}









void C3Population::GetCityProjectedTrade(uint32 city_id,
     BOOL *is_unknown_id, sint32 *projected_gross_gold, sint32 *projected_net_gold)
{
	CityData *cd = GetCityData(m_player, city_id, is_unknown_id);
	if (cd == NULL) {
		*projected_gross_gold = 0;
		*projected_net_gold = 0;
		return;
	}
	*projected_gross_gold = cd->GetNetCityGold();
	*projected_net_gold = cd->GetNetCityGold();
}









void C3Population::GetCityProjectedTradeFromCell(uint32 city_id, MapPointData *p, sint32 *trade, BOOL *is_unknown_id)
	{
	*trade = 0;
	}









void C3Population::GetCityProjectedProduction(uint32 city_id, sint32 *production, BOOL *is_unknown_id)
	{
	CityData *cd = GetCityData(m_player, city_id, is_unknown_id);
	if (cd == NULL) {
		*production = 0;
		return;
	}
	*production = cd->GetGrossCityProduction();
	}









void C3Population::GetCityRequiredFood(uint32 city_id, sint32 *food, BOOL *is_unknown_id)
	{
	CityData *cd = GetCityData(m_player, city_id, is_unknown_id);
	if (cd == NULL) {
		*food = 0;
		return;
	}
	*food = (cd->PopCount() - cd->SlaveCount()) * sint32(g_theConstDB->Get(0)->GetCityGrowthCoefficient());
	}









BOOL C3Population::TryPlacePop(uint32 city_id, uint32 pop_idx, sint32 player_id, MapPointData *p, BOOL *is_unknown_id)
	{
if (
    (p->x < 0) ||
   (g_theWorld->GetXWidth() <= p->x) ||
    (p->y < 0) ||
   (g_theWorld->GetYHeight() <= p->y))
{
    sint32 TryPlacePop_out_of_bounds=0;
    Assert(TryPlacePop_out_of_bounds);
    return FALSE;
}


		{
		MapPoint ipos ;
		ipos.Norm2Iso(*p) ;

        Assert(m_player);
        if (NULL == m_player)
            return FALSE;

		return FALSE;
		}

	return (FALSE) ;
	}









BOOL C3Population::IsPopAllowed(uint32 city_id, uint32 popType, BOOL *is_unknown_id)
	{
	return TRUE;
	}

double C3Population::GetCityScientistOutput(uint32 city_id, BOOL *is_unknown_id)
{
    return 0;
}

double C3Population::GetCityGruntOutput(uint32 city_id, BOOL *is_unknown_id)
{
    return 0;
}

double C3Population::GetCityMusicianOutput(uint32 city_id, BOOL *is_unknown_id)
{
    return 0;
}







void C3Population::GetCityScience(uint32 city_id, sint32 *science, BOOL *is_unknown_id)
	{
	CityData *cd = GetCityData(m_player, city_id, is_unknown_id);
	if (cd == NULL) {
		*science = 0;
		return;
	}
	*science = cd->m_science;
	}

double C3Population::GetSlaveHunger ()
{
    return 0.0;
}

double C3Population::GetCitizensHunger ()
{
    return 0.0;
}
