#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __ACHIEVEMENT_TRACKER_H__
#define __ACHIEVEMENT_TRACKER_H__

#define ACHIEVE_UNDERSEA_CITY 0
#define ACHIEVE_SPACE_CITY    1

#include <nlohmann/json.hpp>

class AchievementTracker {
	friend void to_json(nlohmann::json &j, AchievementTracker const &at);
	friend void from_json(nlohmann::json const &j, AchievementTracker &at);

private:
	uint64 m_achievements;
public:
	AchievementTracker() { m_achievements = 0; }
    void SetData(uint64 data) { m_achievements = data; }
	uint64 GetData() const { return m_achievements; }

	BOOL HasAchieved(sint32 which);
	void AddAchievement(sint32 which);
};

// g_theAchievementTracker demoted to file-scope `static` in gameinit.cpp
// (where the lifecycle lives).  External callers go through
// achievementtracker_Get() (returns NULL before the game state is loaded).
AchievementTracker * achievementtracker_Get();
void                 achievementtracker_Set(AchievementTracker *p);

#endif
