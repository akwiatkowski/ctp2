// test/cpp/test_bugfixes.cpp
// Regression tests for bugs from BUG_HUNT_REPORT.md

#include "doctest.h"
#include "ctp/c3.h"
#include "gs/gameobj/Order.h"

TEST_CASE("OrderToEvent returns GEV_MAX for out-of-bounds order")
{
    // The array s_orderToEventMap has UNIT_ORDER_MAX (64) entries.
    // An out-of-bounds index must not crash.
    CHECK(Order::OrderToEvent((UNIT_ORDER_TYPE)(-1)) == GEV_MAX);
    CHECK(Order::OrderToEvent((UNIT_ORDER_TYPE)(UNIT_ORDER_MAX)) == GEV_MAX);
    CHECK(Order::OrderToEvent((UNIT_ORDER_TYPE)(UNIT_ORDER_MAX + 100)) == GEV_MAX);
}
