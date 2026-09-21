#ifndef __SLIC_CONTEXT_H__
#define __SLIC_CONTEXT_H__

class UnitDynamicArray;
template <class t> class DynamicArray;
template <class t> class SimpleDynamicArray;
class Civilisation;
typedef sint32 AdvanceType;
class MapPoint;
class Unit;
class SlicSegment;
class SlicFrame;
typedef sint32 PLAYER_INDEX;
class Agreement;
class TradeOffer;
class GameEventArgList;
class Army;

#include "gs/gameobj/UnitTypes.h"
#include "gs/diplomacy/diplomacy_types.h"
#include <nlohmann/json.hpp>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

class SlicContext {
private:
	GameEventArgList *m_eventArgs;

	std::unique_ptr<SimpleDynamicArray<Unit>> m_cityList;
	std::unique_ptr<SimpleDynamicArray<Unit>> m_unitList;
	std::unique_ptr<SimpleDynamicArray<Army>> m_armyList;
	std::unique_ptr<SimpleDynamicArray<sint32>> m_playerList;
	std::unique_ptr<SimpleDynamicArray<sint32>> m_advanceList;

	std::unique_ptr<SimpleDynamicArray<MapPoint>> m_locationList;
	std::unique_ptr<SimpleDynamicArray<ai::Agreement>> m_agreementList;
	std::unique_ptr<SimpleDynamicArray<sint32>> m_intList;
	std::unique_ptr<SimpleDynamicArray<sint32>> m_unitRecordList;
	std::unique_ptr<SimpleDynamicArray<sint32>> m_goodList;
	std::unique_ptr<SimpleDynamicArray<sint32>> m_governmentList;

	std::vector<sint32> m_calamityList;

	std::vector<sint32> m_goldList;

	std::vector<sint32> m_rankList;

	std::vector<sint32> m_wonderList;

	std::vector<std::string> m_actionList;

	std::vector<sint32> m_orderList;

    std::vector<sint32> m_madlibNameList;
    std::vector<sint32> m_madlibChoiceList;

	std::vector<sint32> m_attitudeList;

	std::vector<sint32> m_ageList;

	std::vector<sint32> m_buildingList;

	std::vector<uint32> m_tradeBidList;

	std::unique_ptr<SimpleDynamicArray<TradeOffer>> m_tradeOffersList;

protected:
	template <typename T>
	static void Expand(std::unique_ptr<T[]> &list, sint32 size)
	{
		std::unique_ptr<T[]> newList = std::make_unique<T[]>(size + 1);
		if (list) {
			memcpy(newList.get(), list.get(), size * sizeof(T));
		}
		list = std::move(newList);
	}
    sint32 Hash(char *name);

public:
	SlicContext();
	SlicContext(SlicContext *copy);
	virtual ~SlicContext();

#ifdef _DEBUG
	void Dump();
#endif

	void AddCity(const Unit &c);
	void AddUnit(const Unit &u);
	void AddArmy(const Army &a);
	void AddPlayer(const PLAYER_INDEX player);
	void AddCivilisation(const PLAYER_INDEX player);
	void AddPlayer(const Civilisation &civ);
	void AddInt(const sint32 val);
	void AddCivilisation(const Civilisation &civ);
	void AddAdvance(const AdvanceType advance);
	void AddLocation(const MapPoint &point);
	void AddCalamity(const sint32 calamityIndex);
	void AddGold(const sint32 goldAmount);
	void AddGood(const sint32 goodIndex);
	void AddRank(const sint32 rank);
	void AddWonder(const sint32 wonderIndex);
	void AddAction(const MBCHAR *action);
	void AddAgreement(const ai::Agreement &agreement);
	void AddTradeOffer(const TradeOffer &offer);
	void AddGovernment(const sint32 gov);
    void AddMadlib(char *name, const sint32 choice);
	void AddAttitude(const sint32 tude);
    void AddAge(sint32 ageIndex);
	void AddBuilding(sint32 building);
	void AddTradeBid(uint32 tradeBid);
	void AddOrder(UNIT_ORDER_TYPE order);
	void AddUnitRecord(sint32 type);

	void DelCity();
	void DelUnit();
	void DelPlayer();
	void DelAdvance();
	void DelLocation();
	void DelCalamity();
	void DelGold();
	void DelGood();
	void DelRank();
	void DelWonder();

	void DelAgreement();
	void DelTradeOffer();
	void DelGovernment();
	void DelMadlib();
	void DelAttitude();
    void DelAge();
	void DelPop();

	Unit GetCity(sint32 index) const;
	void SetCity(sint32 index, Unit &city);
	sint32 GetNumCities() const;

	Unit GetUnit(sint32 index) const;
	void SetUnit(sint32 index, Unit &unit);
	sint32 GetNumUnits() const;

	Army GetArmy(sint32 index) const;
	void SetArmy(sint32 index, Army &army);
	sint32 GetNumArmies() const;

	sint32 GetPlayer(sint32 index) const;
	void SetPlayer(sint32 index, sint32 &civ);
	sint32 GetNumPlayers() const;

	sint32 GetInt(sint32 index) const;
	void SetInt(sint32 index, sint32 &val);
	sint32 GetNumInts() const;

	AdvanceType GetAdvance(sint32 index) const;
	void SetAdvance(sint32 index, sint32 &adv);
	sint32 GetNumAdvances() const;

	MapPoint GetLocation(sint32 index) const;
	void SetLocation(sint32 index, MapPoint &point);
	sint32 GetNumLocations() const;
	sint32 GetCalamity(sint32 index) const;
	sint32 GetNumCalamities() const;
	sint32 GetGold(sint32 index) const;
	sint32 GetNumGolds() const;
	sint32 GetGood(sint32 index) const;
	void   SetGood(sint32 index, sint32 &good);
	sint32 GetNumGoods() const;
	bool   HaveGoodOfType(sint32 good) const;
	sint32 GetRank(sint32 index) const;
	sint32 GetNumRanks() const;
	sint32 GetWonder(sint32 index) const;
	sint32 GetNumWonders() const;
	MBCHAR *GetAction(sint32 index) const;
	sint32 GetNumActions() const;
	void SetAction(sint32 index, const MBCHAR *action);
	ai::Agreement GetAgreement(sint32 index) const;
	sint32 GetNumAgreements() const;
	sint32 GetNumTradeOffers() const;
	TradeOffer GetTradeOffer(sint32 index) const;

	sint32 GetNumGovernments() const;
	sint32 GetGovernment(sint32 index) const;
	void SetGovernment(sint32 index, sint32 &gov);

    sint32 GetNumMadlibs() const;
    sint32 GetMadlib(char *name);
	sint32 GetNumAttitudes() const;
	sint32 GetAttitude(sint32 index) const;
    sint32 GetNumAges() const;
	sint32 GetAge(sint32 index) const;
	sint32 GetBuilding(sint32 index) const;
	sint32 GetNumBuildings() const;
	uint32 GetTradeBid(sint32 index) const;
	sint32 GetNumTradeBids() const;
	sint32 GetNumPops() const;
	sint32 GetNumOrders() const;
	sint32 GetNumUnitRecords() const;

	UNIT_ORDER_TYPE GetOrder(const sint32 index) const;
	void SetOrder(sint32 index, UNIT_ORDER_TYPE order);

	sint32 GetUnitRecord(const sint32 index) const;
	void SetUnitRecord(sint32 index, sint32 rec);

	virtual bool ConcernsPlayer(PLAYER_INDEX player) const;

	void Snarf(GameEventArgList *args);
	void FillBuiltins();
	void CopyFromBuiltins();

	friend void to_json(nlohmann::json &j, SlicContext const &c);
	friend void from_json(nlohmann::json const &j, SlicContext &c);
};

#endif
