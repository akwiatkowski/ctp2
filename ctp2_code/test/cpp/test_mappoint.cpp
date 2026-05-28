// test/cpp/test_mappoint.cpp
// Pure unit tests for MapPoint — constructors, comparators, and arithmetic.
// These tests exercise only methods with zero global-state dependencies.

#include "ctp/c3.h"
#include "doctest.h"
#include "gs/world/MapPoint.h"

TEST_CASE("MapPoint default constructor produces zero point") {
    MapPoint p;
    CHECK(p.x == 0);
    CHECK(p.y == 0);
}

TEST_CASE("MapPoint two-arg constructor stores coordinates") {
    MapPoint p(3, 7);
    CHECK(p.x == 3);
    CHECK(p.y == 7);
}

#if !defined(_SMALL_MAPPOINTS)
TEST_CASE("MapPoint three-arg constructor stores coordinates") {
    MapPoint p(1, 2, 3);
    CHECK(p.x == 1);
    CHECK(p.y == 2);
    CHECK(p.z == 3);
}
#endif

TEST_CASE("MapPoint operator== matches identical points") {
    MapPoint a(5, 9);
    MapPoint b(5, 9);
    CHECK(a == b);
    CHECK_FALSE(a != b);
}

TEST_CASE("MapPoint operator!= differs for different points") {
    MapPoint a(5, 9);
    MapPoint b(5, 8);
    CHECK(a != b);
    CHECK_FALSE(a == b);
}

TEST_CASE("MapPoint operator+= adds field-wise") {
    MapPoint a(2, 3);
    MapPoint b(4, 5);
    a += b;
    CHECK(a.x == 6);
    CHECK(a.y == 8);
}

TEST_CASE("MapPoint operator-= subtracts field-wise") {
    MapPoint a(10, 7);
    MapPoint b(3, 4);
    a -= b;
    CHECK(a.x == 7);
    CHECK(a.y == 3);
}

TEST_CASE("MapPoint explicit MapPointData copy constructor copies fields") {
    MapPointData data;
    data.x = 11;
    data.y = 22;
    data.z = 33;
    MapPoint p(data);
    CHECK(p.x == 11);
    CHECK(p.y == 22);
#if !defined(_SMALL_MAPPOINTS)
    CHECK(p.z == 33);
#endif
}

TEST_CASE("MapPoint direct field access works") {
    MapPoint p(42, -7);
    CHECK(p.x == 42);
    CHECK(p.y == -7);
}
