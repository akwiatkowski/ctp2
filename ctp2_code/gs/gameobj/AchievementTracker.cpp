#include "ctp/c3.h"
#include <memory>
#include "gs/gameobj/AchievementTracker.h"
#include "gs/utility/safety.h"
#include "gs/gameobj/player.h"
#include "net/general/network.h"
#include "net/general/net_info.h"
#include "gs/fileio/gamefile.h"


// g_theAchievementTracker is defined in gameinit.cpp (where the lifecycle
// lives); reachable via achievementtracker_Get() declared in
// AchievementTracker.h.

BOOL AchievementTracker::HasAchieved(sint32 which)
{
	return (m_achievements & safe_shift_left_u64(which)) != 0;
}

void AchievementTracker::AddAchievement(sint32 which)
{
	m_achievements |= safe_shift_left_u64(which);
	if(network_Get().IsHost()) {
		network_Get().Enqueue(std::make_unique<NetInfo>(NET_INFO_CODE_ACHIEVEMENTS,
									  (uint32)(m_achievements & 0xffffffff),
									  (uint32)(m_achievements >> 32)).release());
	}
}
