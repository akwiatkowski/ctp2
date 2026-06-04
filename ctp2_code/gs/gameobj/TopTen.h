#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __TOPTEN_H__
#define __TOPTEN_H__

#include "gs/gameobj/Unit.h"

#include <nlohmann/json.hpp>


typedef struct tagTopEntry
	{
	Unit	unit ;

	sint32	value ;

	} TopEntry ;

inline void to_json(nlohmann::json &j, TopEntry const &e)
{
	j = nlohmann::json{{"unit", e.unit}, {"value", e.value}};
}

inline void from_json(nlohmann::json const &j, TopEntry &e)
{
	j.at("unit") .get_to(e.unit);
	j.at("value").get_to(e.value);
}

enum
	{
	TOPTENTYPE_FIRST,
	TOPTENTYPE_BIGGEST_CITY = TOPTENTYPE_FIRST,
	TOPTENTYPE_HAPPIEST_CITY,
	TOPTENTYPE_MAX,
	} ;

#define TOPTEN_LIST_SIZE	10

class TopTen
	{
private :








	TopEntry m_biggestCities[TOPTEN_LIST_SIZE] ;

	TopEntry m_happiestCities[TOPTEN_LIST_SIZE] ;




	BOOL InsertCity(TopEntry *cityList, const Unit &c, const sint32 value, sint32 &pos) ;

public:

	TopTen() ;
	~TopTen() ;

	void Clear();
	void CalculateBiggestCities() ;
	BOOL FindCity(const Unit &c, TopEntry *list, sint32 &pos) ;
	void EndTurn() ;
	BOOL IsTopTenCity(const Unit &c, const sint32 category, sint32 &pos) ;
	BOOL GetCityPosition(const Unit &c, const sint32 category, sint32 &pos) ;
	void InsertCity(const Unit &c) ;
	Unit GetBiggestCity(const sint32 idx) { Assert((idx>=0) && (idx<TOPTEN_LIST_SIZE)) ; return (m_biggestCities[idx].unit) ; }
	Unit GetHappiestCity(const sint32 idx) { Assert((idx>=0) && (idx<TOPTEN_LIST_SIZE)) ; return (m_happiestCities[idx].unit) ; }

	// JSON bridge — mirrors TopTen::Serialize.  Persists the two
	// fixed-size leaderboard arrays.  Implementation inline below.
	friend void to_json(nlohmann::json &j, TopTen const &t);
	friend void from_json(nlohmann::json const &j, TopTen &t);
	} ;

inline void to_json(nlohmann::json &j, TopTen const &t)
{
	std::vector<TopEntry> biggest (t.m_biggestCities,  t.m_biggestCities  + TOPTEN_LIST_SIZE);
	std::vector<TopEntry> happiest(t.m_happiestCities, t.m_happiestCities + TOPTEN_LIST_SIZE);
	j = nlohmann::json{
		{"biggest_cities",  biggest},
		{"happiest_cities", happiest},
	};
}

inline void from_json(nlohmann::json const &j, TopTen &t)
{
	auto const &biggest  = j.at("biggest_cities");
	auto const &happiest = j.at("happiest_cities");
	if (biggest.size()  != TOPTEN_LIST_SIZE ||
	    happiest.size() != TOPTEN_LIST_SIZE)
	{
		throw nlohmann::json::other_error::create(
			582, "top_ten leaderboard size != TOPTEN_LIST_SIZE", &j);
	}
	for (sint32 i = 0; i < TOPTEN_LIST_SIZE; ++i)
	{
		biggest[i] .get_to(t.m_biggestCities[i]);
		happiest[i].get_to(t.m_happiestCities[i]);
	}
}

#else

class TopTen ;

#endif

// Session-singleton accessor pair.  Callers should use topten_Get()
// instead of reaching for the legacy g_theTopTen global directly.
TopTen * topten_Get();
void     topten_Set(TopTen *p);
