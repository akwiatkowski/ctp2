//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Slic context (values of variables)
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
// - Destructor cleaned up.
// - Repaired crashes.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/slic/SlicObject.h"
#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicSegment.h"
#include "gs/world/MapPoint.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/Civilisation.h"
#include "gs/gameobj/Advances.h"
#include "gs/slic/SlicFrame.h"
#include "gs/gameobj/Player.h"
#include "robot/aibackdoor/civarchive.h"
#include "gs/gameobj/Agreement.h"
#include "gs/gameobj/TradeOffer.h"
#include "gs/events/GameEventArgList.h"
#include "gs/events/GameEventArgument.h"
#include "gs/gameobj/Army.h"
#include "gs/slic/SlicBuiltinEnum.h"
#include "gs/slic/SlicArray.h"
#include "gs/slic/SlicSymbol.h"
#include "gs/utility/SimpleDynArr.h"
#include "gs/gameobj/Order.h"
#include "gs/diplomacy/diplomacy_types.h"

namespace
{

//----------------------------------------------------------------------------
//
// Name       : ImplementationType
//
// Description: Get the implementation type of a builtin variable type.
//
// Parameters : builtin     : the builtin type
//
// Globals    : -
//
// Returns    : SLIC_SYM    : the implementation type
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
    SLIC_SYM ImplementationType(SLIC_BUILTIN builtin)
    {
        switch (builtin)
        {
#if 0   // Unused CTP1 leftovers?
	    case SLIC_BUILTIN_GLOBAL:
	    case SLIC_BUILTIN_POP:
	    case SLIC_BUILTIN_IMPROVEMENT:
#endif
        default:
            return SLIC_SYM_UNDEFINED;

	    case SLIC_BUILTIN_ACTION:
            return SLIC_SYM_STRING;

	    case SLIC_BUILTIN_ADVANCE:
	    case SLIC_BUILTIN_BUILDING:
	    case SLIC_BUILTIN_GOLD:
	    case SLIC_BUILTIN_GOOD:
	    case SLIC_BUILTIN_GOVERNMENT:
        case SLIC_BUILTIN_PLAYER:
	    case SLIC_BUILTIN_UNITRECORD:
	    case SLIC_BUILTIN_VALUE:
	    case SLIC_BUILTIN_WONDER:
            return SLIC_SYM_IVAR;

	    case SLIC_BUILTIN_ARMY:
            return SLIC_SYM_ARMY;

	    case SLIC_BUILTIN_CITY:
            return SLIC_SYM_CITY;

	    case SLIC_BUILTIN_UNIT:
            return SLIC_SYM_UNIT;

	    case SLIC_BUILTIN_LOCATION:
            return SLIC_SYM_LOCATION;
        }
    }

//----------------------------------------------------------------------------
//
// Name       : ResizedArray
//
// Description: Get and prune the array belonging to a builtin variable
//
// Parameters : builtin     : the builtin type
//              new_size    : the size to prune to
//
// Globals    : slicengine_Get()
//
// Returns    : SlicArray   : the array for the builtin, after pruning
//
// Remark(s)  : No checks are applied.
//
//----------------------------------------------------------------------------
    SlicArray * ResizedArray(SLIC_BUILTIN builtin, sint32 new_size)
    {
        SlicArray * array   =
            slicengine_Get()->GetBuiltinSymbol(builtin)->GetArray();
        array->Prune(new_size);
        return array;
    }

//----------------------------------------------------------------------------
//
// Name       : SingleItemStack
//
// Description: Placeholder for a single item that has to be accessed as a
//              SlicStackValue
//
// Parameters : implementationType  : the type of the item
//
//----------------------------------------------------------------------------
    class SingleItemStack
    {
    public:
        SingleItemStack(SLIC_SYM implementationType)
        :   m_symbol    (implementationType)
        {
            m_stackValue.m_sym  = &m_symbol;
        };
        virtual ~SingleItemStack() { ; };

        SlicStackValue      Value() const   { return m_stackValue; };
        SlicSymbolData &    Symbol()        { return m_symbol; }

    private:
        SlicSymbolData  m_symbol;
 	    SlicStackValue  m_stackValue;
    };

} // namespace

SlicContext::SlicContext()
{
	m_cityList = nullptr;
	m_unitList = nullptr;
	m_playerList = nullptr;
	m_intList = nullptr;
	m_unitRecordList = nullptr;
	m_advanceList = nullptr;
	m_agreementList = nullptr;
	m_locationList = nullptr;
	m_calamityList = nullptr;
	m_numCalamities = 0;
	m_goldList = nullptr;
	m_numGolds = 0;
	m_goodList = nullptr;
	m_rankList = nullptr;
	m_numRanks = 0;
	m_wonderList = nullptr;
	m_numWonders = 0;
	m_tradeOffersList = nullptr;

	m_governmentList = nullptr;
	m_numOrders = 0;
	m_orderList = nullptr;
    m_numMadlibs = 0;
    m_madlibNameList = nullptr;
    m_madlibChoiceList = nullptr;
	m_numAttitudes = 0;
	m_attitudeList = nullptr;
	m_numAges = 0;
	m_ageList = nullptr;
	m_buildingList = nullptr;
	m_numBuildings = 0;
	m_tradeBidList = nullptr;
	m_numTradeBids = 0;
	m_armyList = nullptr;

	m_eventArgs = nullptr;
}

#define COPY_SIMPLE_ARRAY(name, type) \
    if(copy->name) {\
        name = new SimpleDynamicArray<type>;\
        for(i = 0; i < copy->name->Num(); i++) {\
			name->Insert(copy->name->Access(i));\
		}\
	} else {\
		name = NULL;\
	}

SlicContext::SlicContext(SlicContext *copy)
{
	sint32 i;
	COPY_SIMPLE_ARRAY(m_cityList, Unit);
	COPY_SIMPLE_ARRAY(m_unitList, Unit);
	COPY_SIMPLE_ARRAY(m_armyList, Army);
	COPY_SIMPLE_ARRAY(m_playerList, sint32);
	COPY_SIMPLE_ARRAY(m_intList, sint32);
	COPY_SIMPLE_ARRAY(m_unitRecordList, sint32);
	COPY_SIMPLE_ARRAY(m_locationList, MapPoint);
	COPY_SIMPLE_ARRAY(m_agreementList, ai::Agreement);
	COPY_SIMPLE_ARRAY(m_tradeOffersList, TradeOffer);
	COPY_SIMPLE_ARRAY(m_goodList, sint32);
	COPY_SIMPLE_ARRAY(m_governmentList, sint32);
	COPY_SIMPLE_ARRAY(m_advanceList, sint32);

	CopyArray(m_calamityList, copy->m_calamityList,
			  m_numCalamities, copy->m_numCalamities);
	CopyArray(m_goldList, copy->m_goldList,
			  m_numGolds, copy->m_numGolds);
	CopyArray(m_rankList, copy->m_rankList,
			  m_numRanks, copy->m_numRanks);
	CopyArray(m_wonderList, copy->m_wonderList,
			  m_numWonders, copy->m_numWonders);
	CopyArray(m_orderList, copy->m_orderList,
			  m_numOrders, copy->m_numOrders);

	CopyArray(m_madlibChoiceList, copy->m_madlibChoiceList,
			  m_numMadlibs, copy->m_numMadlibs);
	CopyArray(m_madlibNameList, copy->m_madlibNameList,
			  m_numMadlibs, copy->m_numMadlibs);
	CopyArray(m_attitudeList, copy->m_attitudeList,
			  m_numAttitudes, copy->m_numAttitudes);
	CopyArray(m_ageList, copy->m_ageList,
			  m_numAges, copy->m_numAges);
	CopyArray(m_buildingList, copy->m_buildingList,
			  m_numBuildings, copy->m_numBuildings);
	CopyArray((sint32*&)m_tradeBidList, (sint32*&)copy->m_tradeBidList,
			  m_numTradeBids, copy->m_numTradeBids);

	m_actionList = copy->m_actionList;

	m_eventArgs = copy->m_eventArgs;
}

SlicContext::~SlicContext()
{
	// m_eventArgs not deleted: reference only
	delete m_cityList;
	delete m_unitList;
	delete m_armyList;
	delete m_playerList;
	delete m_advanceList;
	delete m_locationList;
	delete m_agreementList;
	delete m_intList;
	delete m_unitRecordList;
	delete m_goodList;
	delete m_governmentList;
    delete m_tradeOffersList;

	delete [] m_calamityList;
	delete [] m_goldList;
    delete [] m_rankList;
    delete [] m_wonderList;
	delete [] m_orderList;
    delete [] m_madlibNameList;
    delete [] m_madlibChoiceList;
	delete [] m_attitudeList;
    delete [] m_ageList;
    delete [] m_buildingList;
	delete [] m_tradeBidList;
}

#define SER_ARRAY(list) \
    hasList = list != NULL;\
    archive << hasList;\
    if(hasList) list->Serialize(archive);

#define UNSER_ARRAY(list, type) \
		archive >> hasList;\
		if(hasList) {\
			list = new SimpleDynamicArray<type>;\
			list->Serialize(archive);\
		} else\
			list = NULL;

void SlicContext::Serialize(CivArchive &archive)
{
	sint32 hasList;
	sint32 l;

	if(archive.IsStoring()) {
		SER_ARRAY(m_cityList);
		SER_ARRAY(m_unitList);
		SER_ARRAY(m_armyList);
		SER_ARRAY(m_playerList);
		SER_ARRAY(m_intList);
		SER_ARRAY(m_unitRecordList);
		SER_ARRAY(m_locationList);
		SER_ARRAY(m_agreementList);
		SER_ARRAY(m_tradeOffersList);
		SER_ARRAY(m_goodList);
		SER_ARRAY(m_governmentList);
		SER_ARRAY(m_advanceList);

		archive << m_numCalamities;
		if(m_numCalamities > 0)
			archive.Store((uint8*)m_calamityList, m_numCalamities * sizeof(sint32));

		archive << m_numGolds;
		if(m_numGolds > 0)
			archive.Store((uint8*)m_goldList, m_numGolds * sizeof(sint32));

		archive << m_numRanks;
		if(m_numRanks > 0)
			archive.Store((uint8*)m_rankList, m_numRanks * sizeof(sint32));

		archive << m_numWonders;
		if(m_numWonders > 0)
			archive.Store((uint8*)m_wonderList, m_numWonders * sizeof(sint32));

		archive << m_numOrders;
		if(m_numOrders > 0) {
			archive.Store((uint8*)m_orderList, m_numOrders * sizeof(sint32));
		}

		archive << m_numMadlibs;
		if(m_numMadlibs > 0) {
			archive.Store((uint8*)m_madlibChoiceList, m_numMadlibs * sizeof(sint32));
			archive.Store((uint8*)m_madlibNameList, m_numMadlibs * sizeof(sint32));
        }

		archive << m_numAttitudes;
		if(m_numAttitudes > 0)
			archive.Store((uint8*)m_attitudeList, m_numAttitudes * sizeof(sint32));

		archive << m_numAges;
		if(m_numAges > 0)
			archive.Store((uint8*)m_ageList, m_numAges * sizeof(sint32));

		{
			sint32 num = static_cast<sint32>(m_actionList.size());
			archive << num;
			for (auto const &action : m_actionList) {
				sint32 len = static_cast<sint32>(action.size() + 1);
				archive << len;
				archive.Store((uint8 *)action.c_str(), len);
			}
		}

		archive << m_numBuildings;
		if(m_numBuildings > 0) {
			archive.Store((uint8 *)m_buildingList, m_numBuildings * sizeof(sint32));
		}

		archive << m_numTradeBids;
		if(m_numTradeBids > 0) {
			archive.Store((uint8 *)m_tradeBidList, m_numTradeBids * sizeof(uint32));
		}
	} else {
		UNSER_ARRAY(m_cityList, Unit);
		UNSER_ARRAY(m_unitList, Unit);
		UNSER_ARRAY(m_armyList, Army);
		UNSER_ARRAY(m_playerList, sint32);
		UNSER_ARRAY(m_intList, sint32);
		UNSER_ARRAY(m_unitRecordList, sint32);
		UNSER_ARRAY(m_locationList, MapPoint);
		UNSER_ARRAY(m_agreementList, ai::Agreement);
		UNSER_ARRAY(m_tradeOffersList, TradeOffer);
		UNSER_ARRAY(m_goodList, sint32);
		UNSER_ARRAY(m_governmentList, sint32);
		UNSER_ARRAY(m_advanceList, sint32);

		archive >> m_numCalamities;
		if(m_numCalamities > 0) {
			m_calamityList = new sint32[m_numCalamities];
			archive.Load((uint8*)m_calamityList, m_numCalamities * sizeof(sint32));
		} else {
			m_calamityList = nullptr;
		}

		archive >> m_numGolds;
		if(m_numGolds > 0) {
			m_goldList = new sint32[m_numGolds];
			archive.Load((uint8*)m_goldList, m_numGolds * sizeof(sint32));
		} else {
			m_goldList = nullptr;
		}

		archive >> m_numRanks;
		if(m_numRanks > 0) {
			m_rankList = new sint32[m_numRanks];
			archive.Load((uint8*)m_rankList, m_numRanks * sizeof(sint32));
		} else {
			m_rankList = nullptr;
		}

		archive >> m_numWonders;
		if(m_numWonders > 0) {
			m_wonderList = new sint32[m_numWonders];
			archive.Load((uint8*)m_wonderList, m_numWonders * sizeof(sint32));
		} else {
			m_wonderList = nullptr;
		}

		archive >> m_numOrders;
		if(m_numOrders > 0) {
			m_orderList = new sint32[m_numOrders];
			archive.Load((uint8*)m_orderList, m_numOrders * sizeof(sint32));
		} else {
			m_orderList = nullptr;
		}

		archive >> m_numMadlibs;
		if(m_numMadlibs > 0) {
			m_madlibChoiceList = new sint32[m_numMadlibs];
			archive.Load((uint8*)m_madlibChoiceList, m_numMadlibs * sizeof(sint32));
			m_madlibNameList = new sint32[m_numMadlibs];
			archive.Load((uint8*)m_madlibNameList, m_numMadlibs * sizeof(sint32));
		} else {
			m_madlibChoiceList = nullptr;
			m_madlibNameList = nullptr;
		}

		archive >> m_numAttitudes;
		if(m_numAttitudes > 0) {
			m_attitudeList = new sint32[m_numAttitudes];
			archive.Load((uint8*)m_attitudeList, m_numAttitudes * sizeof(sint32));
		} else {
			m_attitudeList = nullptr;
		}

		archive >> m_numAges;
		if(m_numAges > 0) {
			m_ageList = new sint32[m_numAges];
			archive.Load((uint8*)m_ageList, m_numAges * sizeof(sint32));
		} else {
			m_ageList = nullptr;
		}

		{
			sint32 num;
			archive >> num;
			m_actionList.clear();
			m_actionList.reserve(num);
			for (sint32 i = 0; i < num; i++) {
				archive >> l;
				std::vector<MBCHAR> buf(l);
				archive.Load((uint8*)buf.data(), l);
				m_actionList.emplace_back(buf.data());
			}
		}

		archive >> m_numBuildings;
		if(m_numBuildings > 0) {
			m_buildingList = new sint32[m_numBuildings];
			archive.Load((uint8*)m_buildingList, m_numBuildings * sizeof(sint32));
		} else {
			m_buildingList = nullptr;
		}

		archive >> m_numTradeBids;
		if(m_numTradeBids > 0) {
			m_tradeBidList = new uint32[m_numTradeBids];
			archive.Load((uint8*)m_tradeBidList, m_numTradeBids * sizeof(uint32));
		} else {
			m_tradeBidList = nullptr;
		}
	}
}

void SlicContext::AddCity(const Unit &c)
{
	if(!m_cityList)
		m_cityList = new SimpleDynamicArray<Unit>;

	m_cityList->Insert(c);
}

void SlicContext::AddUnit(const Unit &u)
{
	if(!m_unitList)
		m_unitList = new SimpleDynamicArray<Unit>;
	m_unitList->Insert(u);
}

void SlicContext::AddArmy(const Army &a)
{
	if(!m_armyList)
		m_armyList = new SimpleDynamicArray<Army>;
	m_armyList->Insert(a);
}

void SlicContext::AddPlayer(const PLAYER_INDEX player)
{
	if(!m_playerList)
		m_playerList = new SimpleDynamicArray<sint32>;
	m_playerList->Insert(player);
}

void SlicContext::AddInt(sint32 val)
{
	if(!m_intList)
		m_intList = new SimpleDynamicArray<sint32>;
	m_intList->Insert(val);
}

void SlicContext::AddUnitRecord(sint32 rec)
{
	if(!m_unitRecordList)
		m_unitRecordList = new SimpleDynamicArray<sint32>;
	m_unitRecordList->Insert(rec);
}

void SlicContext::AddCivilisation(const PLAYER_INDEX player)
{
	AddPlayer(player);
}

void SlicContext::AddPlayer(const Civilisation &civ)
{
	if(!m_playerList)
		m_playerList = new SimpleDynamicArray<sint32>;
	m_playerList->Insert(civ.GetOwner());
}

void SlicContext::AddCivilisation(const Civilisation &civ)
{
	AddPlayer(civ);
}

void SlicContext::AddAdvance(const AdvanceType advance)
{
	if(!m_advanceList)
		m_advanceList = new SimpleDynamicArray<sint32>;

	m_advanceList->Insert(advance);
}

void SlicContext::AddLocation(const MapPoint &point)
{
	if(!m_locationList)
		m_locationList = new SimpleDynamicArray<MapPoint>;
	m_locationList->Insert(point);
}

void SlicContext::AddCalamity(const sint32 calamity)
{
	m_calamityList = Expand(m_calamityList, m_numCalamities);
	m_calamityList[m_numCalamities++] = calamity;
}

void SlicContext::AddGold(const sint32 goldAmount)
{
	m_goldList = Expand(m_goldList, m_numGolds);
	m_goldList[m_numGolds++] = goldAmount;
}

void SlicContext::AddGood(const sint32 goodIndex)
{
	if(!m_goodList)
		m_goodList = new SimpleDynamicArray<sint32>;
	m_goodList->Insert(goodIndex);
}

void SlicContext::AddRank(const sint32 rank)
{
	m_rankList = Expand(m_rankList, m_numRanks);
	m_rankList[m_numRanks++] = rank;
}

void SlicContext::AddWonder(const sint32 wonderIndex)
{
	m_wonderList = Expand(m_wonderList, m_numWonders);
	m_wonderList[m_numWonders++] = wonderIndex;
}

void SlicContext::AddAction(const MBCHAR *action)
{
	m_actionList.emplace_back(action ? action : "");
}

void SlicContext::SetAction(sint32 index, const MBCHAR *action)
{
	while(static_cast<sint32>(m_actionList.size()) <= index) {
		AddAction("");
	}
	m_actionList[index] = action ? action : "";
}

sint32 *SlicContext::Expand(sint32 *list, sint32 size)
{
	sint32 *newList = new sint32[size + 1];
	if(list) {

		memcpy(newList, list, size * sizeof(sint32));
		delete [] list;
	}

	return newList;
}

void SlicContext::CopyArray(sint32 *&to, sint32 *from,
							sint32 &tosize, sint32 size)
{
	if(!from || size == 0) {
		to = nullptr;
		tosize = 0;
		return;
	}
	to = new sint32[size];
	memcpy(to, from, size * sizeof(sint32));
	tosize = size;
}

Unit SlicContext::GetCity(sint32 index) const
{
	Unit city;
	if (m_eventArgs && m_eventArgs->GetCity(index, city))
    {
		return city;
	}

    return m_cityList ? m_cityList->Access(index) : Unit();
}

sint32 SlicContext::GetNumCities() const
{
    return m_cityList ? m_cityList->Num() : 0;
}

Unit SlicContext::GetUnit(sint32 index) const
{
	Unit u;
	if (m_eventArgs && m_eventArgs->GetUnit(index, u))
    {
		return u;
	}

    return m_unitList ? m_unitList->Access(index) : Unit();
}

sint32 SlicContext::GetNumUnits() const
{
    return m_unitList ? m_unitList->Num() : 0;
}

Army SlicContext::GetArmy(sint32 index) const
{
    return m_armyList ? m_armyList->Access(index) : Army();
}

sint32 SlicContext::GetNumArmies() const
{
    return m_armyList ? m_armyList->Num() : 0;
}

sint32 SlicContext::GetPlayer(sint32 index) const
{
	sint32 p;
	if (m_eventArgs && m_eventArgs->GetPlayer(index, p))
    {
		return p;
	}

    return m_playerList ? (*m_playerList)[index] : PLAYER_UNASSIGNED;
}

sint32 SlicContext::GetInt(sint32 index) const
{
	if(!m_intList)
		return 0;
	return (*m_intList)[index];
}

sint32 SlicContext::GetUnitRecord(sint32 index) const
{
	if(!m_unitRecordList)
		return 0;
	return (*m_unitRecordList)[index];
}

void SlicContext::SetPlayer(sint32 index, sint32 &player)
{
	if(!m_playerList)
		m_playerList = new SimpleDynamicArray<sint32>;

	if(index < 0)
		return;

	if(index >= m_playerList->Num()) {
		while(index >= m_playerList->Num()) {
			m_playerList->Insert(player);
		}
	} else {
		(*m_playerList)[index] = player;
	}
}

void SlicContext::SetInt(sint32 index, sint32 &val)
{
	if(!m_intList)
		m_intList = new SimpleDynamicArray<sint32>;

	if(index < 0)
		return;

	if(index >= m_intList->Num()) {
		while(index >= m_intList->Num()) {
			m_intList->Insert(val);
		}
	} else {
		(*m_intList)[index] = val;
	}
}

void SlicContext::SetUnitRecord(sint32 index, sint32 rec)
{
	if(!m_unitRecordList)
		m_unitRecordList = new SimpleDynamicArray<sint32>;

	if(index < 0)
		return;

	if(index >= m_unitRecordList->Num()) {
		while(index >= m_unitRecordList->Num()) {
			m_unitRecordList->Insert(rec);
		}
	} else {
		(*m_unitRecordList)[index] = rec;
	}
}

void SlicContext::SetUnit(sint32 index, Unit &u)
{
	if(!m_unitList)
		m_unitList = new SimpleDynamicArray<Unit>;

	if(index < 0)
		return;
	if(index >= m_unitList->Num()) {
		while(index >= m_unitList->Num()) {
			m_unitList->Insert(u);
		}
	} else {
		m_unitList->Access(index) = u;
	}
}

void SlicContext::SetArmy(sint32 index, Army &a)
{
	if(!m_armyList)
		m_armyList = new SimpleDynamicArray<Army>;
	if(index < 0)
		return ;
	if(index >= m_armyList->Num()) {
		while(index >= m_armyList->Num()) {
			m_armyList->Insert(a);
		}
	} else {
		m_armyList->Access(index) = a;
	}
}

void SlicContext::SetCity(sint32 index, Unit &city)
{
	if(!m_cityList)
		m_cityList = new SimpleDynamicArray<Unit>;

	if(index < 0)
		return;

	if(index >= m_cityList->Num()) {
		while(index >= m_cityList->Num()) {
			m_cityList->Insert(city);
		}
	} else {
		m_cityList->Access(index) = city;
	}
}

void SlicContext::SetLocation(sint32 index, MapPoint &point)
{
	if(!m_locationList)
		m_locationList = new SimpleDynamicArray<MapPoint>;

	if(index < 0)
		return;

	if(index >= m_locationList->Num()) {
		while(index >= m_locationList->Num()) {
			m_locationList->Insert(point);
		}
	} else {
		m_locationList->Access(index) = point;
	}
}

sint32 SlicContext::GetNumPlayers() const
{
    return m_playerList ? m_playerList->Num() : 0;
}

sint32 SlicContext::GetNumInts() const
{
    return m_intList ? m_intList->Num() : 0;
}

sint32 SlicContext::GetNumUnitRecords() const
{
    return m_unitRecordList ? m_unitRecordList->Num() : 0;
}

AdvanceType SlicContext::GetAdvance(sint32 index) const
{
	AdvanceType a;
	if (m_eventArgs && m_eventArgs->GetAdvance(index, a))
    {
		return a;
	}

	if(!m_advanceList)
		return 0;

	Assert(index >= 0 && index < m_advanceList->Num());
	return (*m_advanceList)[index];
}

sint32 SlicContext::GetNumAdvances() const
{
	return m_advanceList ? m_advanceList->Num() : 0;
}

MapPoint SlicContext::GetLocation(sint32 index) const
{
	MapPoint pos;
	if (m_eventArgs && m_eventArgs->GetPos(index, pos))
    {
		return pos;
	}

    return m_locationList ? m_locationList->Access(index) : MapPoint();
}

sint32 SlicContext::GetNumLocations() const
{
    return m_locationList ? m_locationList->Num() : 0;
}

sint32 SlicContext::GetCalamity(sint32 index) const
{
	if(!m_calamityList)
		return 0;
	Assert(index >= 0 && index < m_numCalamities);
	return m_calamityList[index];
}

sint32 SlicContext::GetNumCalamities() const
{
	return m_numCalamities;
}

sint32 SlicContext::GetGold(sint32 index) const
{
	if(!m_goldList)
		return 0;
	Assert(index >= 0 && index < m_numGolds);
	return m_goldList[index];
}

sint32 SlicContext::GetNumGolds() const
{
	return m_numGolds;
}

sint32 SlicContext::GetGood(sint32 index) const
{
	if(!m_goodList)
		return 0;
	Assert(index >= 0 && index < m_goodList->Num());
	return m_goodList->Access(index);
}

void SlicContext::SetGood(sint32 index, sint32 &good)
{
	if(!m_goodList)
		m_goodList = new SimpleDynamicArray<sint32>;

	if(index < 0)
		return;

	if(index >= m_goodList->Num()) {
		while(index >= m_goodList->Num()) {
			m_goodList->Insert(good);
		}
	} else {
		(*m_goodList)[index] = good;
	}
}

void SlicContext::SetAdvance(sint32 index, sint32 &adv)
{
	if(!m_advanceList)
		m_advanceList = new SimpleDynamicArray<sint32>;

	if(index < 0)
		return;

	if(index >= m_advanceList->Num()) {
		while(index >= m_advanceList->Num()) {
			m_advanceList->Insert(adv);
		}
	} else {
		(*m_advanceList)[index] = adv;
	}
}

void SlicContext::SetGovernment(sint32 index, sint32 &gov)
{
	if(!m_governmentList)
		m_governmentList = new SimpleDynamicArray<sint32>;

	if(index < 0)
		return;

	if(index >= m_governmentList->Num()) {
		while(index >= m_governmentList->Num()) {
			m_governmentList->Insert(gov);
		}
	} else {
		(*m_governmentList)[index] = gov;
	}
}

sint32 SlicContext::GetNumGoods() const
{
    return m_goodList ? m_goodList->Num() : 0;
}

bool SlicContext::HaveGoodOfType(sint32 good) const
{
	sint32 n    = GetNumGoods();
	for(sint32 i = 0; i < n; i++)
    {
		if (m_goodList->Access(i) == good)
			return true;
	}

	return false;
}

sint32 SlicContext::GetRank(sint32 index) const
{
	if(!m_rankList)
		return 0;
	Assert(index >= 0 && index < m_numRanks);
	return m_rankList[index];
}

sint32 SlicContext::GetNumRanks() const
{
	return m_numRanks;
}

sint32 SlicContext::GetWonder(sint32 index) const
{
	if(m_eventArgs) {
		sint32 w;
		if(m_eventArgs->GetWonder(index, w))
			return w;
	}

	if(!m_wonderList)
		return 0;
	Assert(index >= 0 && index < m_numWonders);
	return m_wonderList[index];
}

sint32 SlicContext::GetNumWonders() const
{
	return m_numWonders;
}

MBCHAR *SlicContext::GetAction(sint32 index) const
{
	if(m_actionList.empty())
		return nullptr;
	Assert(index >= 0 && index < static_cast<sint32>(m_actionList.size()));
	return const_cast<MBCHAR*>(m_actionList[index].c_str());
}

sint32 SlicContext::GetNumActions() const
{
	return static_cast<sint32>(m_actionList.size());
}

bool SlicContext::ConcernsPlayer(PLAYER_INDEX player) const
{
	if(!m_playerList)
		return false;

	for(sint32 i = 0; i < m_playerList->Num(); i++) {
		if((*m_playerList)[i] == player)
			return true;
	}
	return false;
}

void SlicContext::AddAgreement(const ai::Agreement &agreement)
{
	if(!m_agreementList) {
		m_agreementList = new SimpleDynamicArray<ai::Agreement>;
	}

	m_agreementList->Insert(agreement);
}

ai::Agreement SlicContext::GetAgreement(sint32 index) const
{
	ai::Agreement agreement;
	if(!m_agreementList || index < 0 || index >= m_agreementList->Num())
		return agreement;

	return m_agreementList->Access(index);
}

sint32 SlicContext::GetNumAgreements() const
{
	if(!m_agreementList) return 0;
	return m_agreementList->Num();
}

void SlicContext::AddTradeOffer(const TradeOffer &offer)
{
	if(!m_tradeOffersList)
		m_tradeOffersList = new SimpleDynamicArray<TradeOffer>;

	m_tradeOffersList->Insert(offer);
}

sint32 SlicContext::GetNumTradeOffers() const
{
	if(!m_tradeOffersList) return 0;
	return m_tradeOffersList->Num();
}

TradeOffer SlicContext::GetTradeOffer(sint32 index) const
{
	if(!m_tradeOffersList || index < 0 || index >= m_tradeOffersList->Num())
		return {};

	return m_tradeOffersList->Access(index);
}

void SlicContext::AddGovernment(const sint32 gov)
{
	if(!m_governmentList)
		m_governmentList = new SimpleDynamicArray<sint32>;
	m_governmentList->Insert(gov);
}

sint32 SlicContext::GetNumGovernments() const
{
	return m_governmentList ? m_governmentList->Num() : 0;
}

sint32 SlicContext::GetGovernment(const sint32 index) const
{
	if(!m_governmentList)
		return -1;

	return (*m_governmentList)[index];
}

void SlicContext::AddOrder(UNIT_ORDER_TYPE order)
{
	m_orderList = Expand(m_orderList, m_numOrders);
	m_orderList[m_numOrders++] = (sint32)order;
}

void SlicContext::SetOrder(sint32 index, UNIT_ORDER_TYPE order)
{
	if(index < 0)
		return;

	if(index >= m_numOrders) {
		m_orderList = Expand(m_orderList, index);
		m_numOrders = index + 1;
	}
	m_orderList[index] = order;
}

sint32 SlicContext::GetNumOrders() const
{
	return m_numOrders;
}

UNIT_ORDER_TYPE SlicContext::GetOrder(const sint32 index) const
{
	if(!m_orderList || index < 0 || index >= m_numOrders)
		return UNIT_ORDER_NONE;
	return (UNIT_ORDER_TYPE)m_orderList[index];
}

void SlicContext::AddBuilding(sint32 building)
{
	m_buildingList = Expand(m_buildingList, m_numBuildings);
	m_buildingList[m_numBuildings++] = building;
}

sint32 SlicContext::GetNumBuildings() const
{
	return m_numBuildings;
}

sint32 SlicContext::GetBuilding(sint32 index) const
{
	if(!m_buildingList || index < 0 || index >= m_numBuildings)
		return -1;
	return m_buildingList[index];
}

void SlicContext::AddTradeBid(uint32 bid)
{
	m_tradeBidList = (uint32*)Expand((sint32*)m_tradeBidList, m_numTradeBids);
	m_tradeBidList[m_numTradeBids++] = bid;
}

sint32 SlicContext::GetNumTradeBids() const
{
	return m_numTradeBids;
}

uint32 SlicContext::GetTradeBid(sint32 index) const
{
	if(!m_tradeBidList || index < 0 || index >= m_numTradeBids)
		return 0;
	return m_tradeBidList[index];
}

void SlicContext::AddMadlib(char *name, const sint32 choice)
{

	m_madlibChoiceList = Expand(m_madlibChoiceList, m_numMadlibs);
	m_madlibChoiceList[m_numMadlibs] = choice;

	m_madlibNameList = Expand(m_madlibNameList, m_numMadlibs);
	m_madlibNameList[m_numMadlibs++] = Hash(name);
}

sint32 SlicContext::GetNumMadlibs() const
{
	return m_numMadlibs;
}

sint32 SlicContext::GetMadlib(char *name)
{
    int i;

	if(!m_madlibNameList || !m_madlibChoiceList ||
       !name || (m_numMadlibs <= 0))
		return -1;

    sint32 nameHash = Hash(name);

    for(i=0; i<m_numMadlibs; i++) {
        if (m_madlibNameList[i] == nameHash) {
            return m_madlibChoiceList[i];
        }
    }

	return -1;
}

void SlicContext::AddAttitude(const sint32 tude)
{
	m_attitudeList = Expand(m_attitudeList, m_numAttitudes);
	m_attitudeList[m_numAttitudes++] = tude;
}

sint32 SlicContext::GetNumAttitudes() const
{
	return m_numAttitudes;
}

sint32 SlicContext::GetAttitude(const sint32 index) const
{
	if(!m_attitudeList || index < 0 || index >= m_numAttitudes)
		return -1;

	return m_attitudeList[index];
}

void SlicContext::AddAge(const sint32 tude)
{
	m_ageList = Expand(m_ageList, m_numAges);
	m_ageList[m_numAges++] = tude;
}

sint32 SlicContext::GetNumAges() const
{
	return m_numAges;
}

sint32 SlicContext::GetAge(const sint32 index) const
{
	if(!m_ageList || index < 0 || index >= m_numAges)
		return -1;

	return m_ageList[index];
}

sint32 SlicContext::Hash(char *name)
{
    char *p;
    unsigned long rval = 0;

    for(p=name; *p; p++) {
        rval = (rval << 8 | rval >> 24) + *p;
    }

    return((sint32)rval);
}


void SlicContext::DelCity()
{
    if (m_cityList) {
        m_cityList->DelIndex(m_cityList->Num() - 1);
    }
}

void SlicContext::DelUnit()
{
    if (m_unitList) {
        m_unitList->DelIndex(m_unitList->Num() - 1);
    }
}

void SlicContext::DelPlayer()
{
	Assert(FALSE);


}

void SlicContext::DelAdvance()
{
	Assert(FALSE);
}

void SlicContext::DelLocation()
{
    if (m_locationList) {
        m_locationList->DelIndex(m_locationList->Num() - 1);
    }
}

void SlicContext::DelCalamity()
{
    if (m_numCalamities > 0)
        m_numCalamities--;
}

void SlicContext::DelGold()
{
    if (m_numGolds > 0)
        m_numGolds--;
}

void SlicContext::DelGood()
{
    if (m_goodList->Num() > 0)
        m_goodList->DelIndex(m_goodList->Num() - 1);
}

void SlicContext::DelRank()
{
    if (m_numRanks > 0)
        m_numRanks--;
}

void SlicContext::DelWonder()
{
    if (m_numWonders > 0)
        m_numWonders--;
}








void SlicContext::DelAgreement()
{
    if (m_agreementList) {
        m_agreementList->DelIndex(m_agreementList->Num() - 1);
    }
}

void SlicContext::DelTradeOffer()
{
    if (m_tradeOffersList) {
        m_tradeOffersList->DelIndex(m_tradeOffersList->Num() - 1);
    }
}

void SlicContext::DelGovernment()
{
	Assert(FALSE);
}

void SlicContext::DelMadlib()
{
    if (m_numMadlibs > 0)
        m_numMadlibs--;
}

void SlicContext::DelAttitude()
{
    if (m_numAttitudes > 0)
        m_numAttitudes--;
}

void SlicContext::DelAge()
{
    if (m_numAges > 0)
        m_numAges--;
}

#ifdef _DEBUG
void SlicContext::Dump()
{
	sint32 i, n;
	if(m_cityList) {
		n = m_cityList->Num();
		for(i = 0; i < n; i++) {
			DPRINTF(k_DBG_INFO, (" City.%d: %lx\n", i, m_cityList->Access(i)));
		}
	}

	if(m_unitList) {
		n = m_unitList->Num();
		for(i = 0; i < n; i++) {
			DPRINTF(k_DBG_INFO, (" Unit.%d: %lx\n", i, m_unitList->Access(i)));
		}
	}

	if(m_playerList) {
		n = m_playerList->Num();
		for(i = 0; i < n; i++) {
			DPRINTF(k_DBG_INFO, (" Civ.%d: %lx\n", i, (*m_playerList)[i]));
		}
	}

	if(m_locationList) {
		n = m_locationList->Num();
		for(i = 0; i < n; i++) {
			MapPoint pos = m_locationList->Access(i);
			DPRINTF(k_DBG_INFO, (" Location.%d: (%d,%d)\n", i,
								 pos.x, pos.y));
		}
	}

	if(m_calamityList) {
		n = m_numCalamities;
		for(i = 0; i < n; i++) {
			DPRINTF(k_DBG_INFO, (" Calamity.%d: %d\n",i, m_calamityList[i]));
		}
	}

	if(m_goldList) {
		n = m_numGolds;
		for(i = 0; i < n; i++) {
			DPRINTF(k_DBG_INFO, (" Gold.%d: %d\n", i, m_goldList[i]));
		}
	}

	if(m_goodList) {
		n = m_goodList->Num();
		for(i = 0; i < n; i++) {
			DPRINTF(k_DBG_INFO, (" Good.%d: %d\n", i, m_goodList->Access(i)));
		}
	}

	if(m_rankList) {
		n = m_numRanks;
		for(i = 0; i < n; i++) {
			DPRINTF(k_DBG_INFO, (" Rank.%d: %d\n", i, m_rankList[i]));
		}
	}

	if(m_wonderList) {
		n = m_numWonders;
		for(i = 0; i < n; i++) {
			DPRINTF(k_DBG_INFO, (" Wonder.%d: %d\n", i, m_wonderList[i]));
		}
	}

	for(sint32 i = 0; i < static_cast<sint32>(m_actionList.size()); i++) {
		DPRINTF(k_DBG_INFO, (" Action.%d: %s\n", i, m_actionList[i].c_str()));
	}
}
#endif

void SlicContext::Snarf(GameEventArgList *args)
{
	sint32 i;
	sint32 c;

	Unit u;
	for(i = 0, c = args->GetArgCount(GEA_Unit); i < c; i++) {
		args->GetArg(GEA_Unit, i)->GetUnit(u);
		SetUnit(i, u);
	}

	Unit city;
	for(i = 0, c = args->GetArgCount(GEA_City); i < c; i++) {
		args->GetArg(GEA_City, i)->GetCity(city);
		SetCity(i, city);
	}

	Army a;
	for(i = 0, c = args->GetArgCount(GEA_Army); i < c; i++) {
		args->GetArg(GEA_Army, i)->GetArmy(a);
		SetArmy(i, a);
	}

	sint32 p;
	for(i = 0, c = args->GetArgCount(GEA_Player); i < c; i++) {
		args->GetArg(GEA_Player, i)->GetPlayer(p);
		SetPlayer(i, p);
	}

	MapPoint pos;
	for(i = 0, c = args->GetArgCount(GEA_MapPoint); i < c; i++) {
		args->GetArg(GEA_MapPoint, i)->GetPos(pos);
		SetLocation(i, pos);
	}

	sint32 val;
	for(i = 0, c = args->GetArgCount(GEA_Int); i < c; i++) {
		args->GetArg(GEA_Int, i)->GetInt(val);
		SetInt(i, val);
	}
}

//----------------------------------------------------------------------------
//
// Name       : FILL
//
// Description: -
//
// Parameters : builtin     : type of built-in object
//              list        : list to fill
//              setMethod   : method to use to copy from built-in to list
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : MACRO
//
//----------------------------------------------------------------------------
#define FILL(builtin, list, setMethod)                              \
{                                                                   \
    sint32 const    count   = list ? list->Num() : 0;               \
    SlicArray *     array   = ResizedArray(builtin, count);         \
	if (count > 0)                                                  \
    {                                                               \
        SingleItemStack item(ImplementationType(builtin));          \
                                                                    \
		for (sint32 i = 0; i < count; ++i)                          \
        {                                                           \
			item.Symbol().setMethod(list->Access(i));               \
			array->Insert(i, SS_TYPE_SYM, item.Value());            \
		}                                                           \
	}                                                               \
}

//----------------------------------------------------------------------------
//
// Name       : FILL_ARRAY
//
// Description: -
//
// Parameters : builtin     : type of built-in object
//              plainArray  : list to fill
//              arrayCount  : number of items in list, if list exists
//              setMethod   : method to use to copy from built-in to list
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : MACRO
//
//----------------------------------------------------------------------------
#define FILL_ARRAY(builtin, plainArray, arrayCount, setMethod)      \
{                                                                   \
    sint32 const    count   = plainArray ? arrayCount : 0;          \
    SlicArray *     array   = ResizedArray(builtin, count);         \
	if (count > 0)                                                  \
    {                                                               \
        SingleItemStack item(ImplementationType(builtin));          \
                                                                    \
		for (sint32 i = 0; i < count; ++i)                          \
        {                                                           \
			item.Symbol().setMethod(plainArray[i]);                 \
			array->Insert(i, SS_TYPE_SYM, item.Value());            \
		}                                                           \
	}                                                               \
}

void SlicContext::FillBuiltins()
{
    // The following lists all have SimpleDynamicArray structure.

	FILL(SLIC_BUILTIN_CITY, m_cityList, SetCity);
	FILL(SLIC_BUILTIN_UNIT, m_unitList, SetUnit);
	FILL(SLIC_BUILTIN_PLAYER, m_playerList, SetIntValue);
	FILL(SLIC_BUILTIN_ARMY, m_armyList, SetArmy);
	FILL(SLIC_BUILTIN_LOCATION, m_locationList, SetPos);
	FILL(SLIC_BUILTIN_VALUE, m_intList, SetIntValue);
	FILL(SLIC_BUILTIN_UNITRECORD, m_unitRecordList, SetIntValue);
	FILL(SLIC_BUILTIN_GOOD, m_goodList, SetIntValue);
	FILL(SLIC_BUILTIN_ADVANCE, m_advanceList, SetIntValue);
	FILL(SLIC_BUILTIN_GOVERNMENT, m_governmentList, SetIntValue);

    // The following lists do not have SimpleDynamicArray structure,
    // but are plain arrays, with a separate count.

    {
        sint32 const    count   = static_cast<sint32>(m_actionList.size());
        SlicArray *     array   = ResizedArray(SLIC_BUILTIN_ACTION, count);
        if (count > 0)
        {
            SingleItemStack item(ImplementationType(SLIC_BUILTIN_ACTION));
            for (sint32 i = 0; i < count; ++i)
            {
                item.Symbol().SetString(m_actionList[i].c_str());
                array->Insert(i, SS_TYPE_SYM, item.Value());
            }
        }
    }
    FILL_ARRAY(SLIC_BUILTIN_BUILDING, m_buildingList, m_numBuildings, SetIntValue);
    FILL_ARRAY(SLIC_BUILTIN_WONDER, m_wonderList, m_numWonders, SetIntValue);
    FILL_ARRAY(SLIC_BUILTIN_GOLD, m_goldList, m_numGolds, SetIntValue);
}

#undef FILL
#undef FILL_ARRAY

#define UNFILL(list, type, GetMethod, SetMethod) \
		if(array->GetSize() > 0) {\
			for(i = 0; i < array->GetSize(); i++) {\
				if(array->Lookup(i, stype, sval)) {\
					type v;\
					if(sval.m_sym->GetMethod(v)) {\
						SetMethod(i, v);\
					}\
				}\
			}\
		} else {\
			delete list;\
			list = NULL;\
		}

void SlicContext::CopyFromBuiltins()
{
	sint32 i, b;
	for(b = 0; b < SLIC_BUILTIN_MAX; b++) {
		SlicSymbolData const * sym = slicengine_Get()->GetBuiltinSymbol((SLIC_BUILTIN)b);
		if(!sym) continue;

		if(sym->GetType() != SLIC_SYM_ARRAY) continue;

		SlicArray *array = sym->GetArray();
		Assert(array);
		if(!array) continue;

		SS_TYPE stype;
		SlicStackValue sval;

		if(array->GetSize() < 1) continue;

		switch(b) {
			case SLIC_BUILTIN_PLAYER:       UNFILL(m_playerList, sint32, GetPlayer, SetPlayer); break;
			case SLIC_BUILTIN_CITY:  		UNFILL(m_cityList, Unit, GetCity, SetCity); break;
			case SLIC_BUILTIN_UNIT:			UNFILL(m_unitList, Unit, GetUnit, SetUnit); break;
			case SLIC_BUILTIN_GLOBAL:       break;
			case SLIC_BUILTIN_ARMY:         UNFILL(m_armyList, Army, GetArmy, SetArmy); break;
			case SLIC_BUILTIN_LOCATION:     UNFILL(m_locationList, MapPoint, GetPos, SetLocation); break;

			case SLIC_BUILTIN_ADVANCE:      UNFILL(m_advanceList, sint32, GetIntValue, SetAdvance); break;
			case SLIC_BUILTIN_GOVERNMENT:   UNFILL(m_governmentList, sint32, GetIntValue, SetGovernment); break;
			case SLIC_BUILTIN_GOLD:         break;
			case SLIC_BUILTIN_POP:          break;
			case SLIC_BUILTIN_GOOD:         UNFILL(m_goodList, sint32, GetIntValue, SetGood); break;
			case SLIC_BUILTIN_ACTION:       break;
			case SLIC_BUILTIN_IMPROVEMENT:  break;
			case SLIC_BUILTIN_VALUE:        UNFILL(m_intList, sint32, GetIntValue, SetInt); break;
			case SLIC_BUILTIN_BUILDING:     break;
			case SLIC_BUILTIN_WONDER:       break;
			case SLIC_BUILTIN_UNITRECORD:   UNFILL(m_unitRecordList, sint32, GetIntValue, SetUnitRecord); break;
			default:
				Assert(FALSE);
				break;
		}
	}
}
#undef UNFILL
