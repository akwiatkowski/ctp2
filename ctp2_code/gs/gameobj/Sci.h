#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __SCIENCE_H__
#define __SCIENCE_H__ 1


#include <nlohmann/json.hpp>

#define k_SCIENCE_VERSION_MAJOR	0
#define k_SCIENCE_VERSION_MINOR	0

class Science {

	friend void to_json(nlohmann::json &j, Science const &s);
	friend void from_json(nlohmann::json const &j, Science &s);







	sint32 m_level;










public:

	Science();
    void AddScience(const sint32 delta) {
     int newval = m_level + delta;

        if ((0 < delta) && (newval < m_level)) {
            m_level  = INT_MAX;
        } if (newval < 0) {
            m_level = 0;
        } else {
            m_level = newval;
        }
   };
	sint32 GetLevel() const { return m_level; };

	void SetLevel(sint32 level) { m_level = level;};

	static sint32 ComputeScienceFromResearchPacts(const sint32 playerId);

	static sint32 ComputeScienceFromResearchPact(const sint32 playerId, const sint32 foreignerId);

};

uint32 Sci_Science_GetVersion() ;
#endif
