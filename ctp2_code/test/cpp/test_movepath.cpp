// test/cpp/test_movepath.cpp
// Pure unit tests for MovePath helpers that don't need a live world.
//
// These tests verify the API contract of army_AddMovePath / army_QueueMovePath
// under degenerate conditions (no pathfinder).  They are intentionally narrow:
// integration tests for real pathfinding live in the headless-smoke suite.

#include "ctp/c3.h"
#include "doctest.h"
#include "gs/gameobj/Army.h"
#include "gs/gameobj/MovePath.h"
#include "gs/world/MapPoint.h"

#include <memory>

class UnitAstar;
extern std::unique_ptr<UnitAstar> g_theUnitAstar;

TEST_CASE("army_AddMovePath returns false when pathfinder is unavailable") {
	// g_theUnitAstar is null in the fast-test binary (no robot init), so any
	// call with an invalid army must fail cleanly rather than crash.
	Army a;
	MapPoint src(0, 0);
	MapPoint dst(5, 5);
	CHECK_FALSE(army_AddMovePath(0, a, src, dst));
}

TEST_CASE("army_QueueMovePath returns false when pathfinder is unavailable") {
	Army a;
	MapPoint src(0, 0);
	MapPoint dst(5, 5);
	CHECK_FALSE(army_QueueMovePath(0, a, src, dst));
}
