// test/cpp/test_order.cpp
// Fast unit test for Order round-passing refactor.
// Verifies that Order constructor takes explicit currentRound
// instead of reading g_turn->GetRound().

#include "ctp/c3.h"
#include "doctest.h"
#include "gs/gameobj/Order.h"

TEST_CASE("Order constructor stores explicit round") {
    MapPoint point(static_cast<sint32>(5), static_cast<sint32>(10));

    SUBCASE("Order with round 42") {
        Order order(UNIT_ORDER_MOVE, nullptr, point, 0, 42);
        CHECK(order.m_round == 42);
    }

    SUBCASE("Order with round 0") {
        Order order(UNIT_ORDER_MOVE, nullptr, point, 0, 0);
        CHECK(order.m_round == 0);
    }

    SUBCASE("Order with round -1 (sentinel)") {
        Order order(UNIT_ORDER_MOVE, nullptr, point, 0, -1);
        CHECK(order.m_round == -1);
    }
}

TEST_CASE("Order default constructor leaves round unset") {
    Order order;
    CHECK(order.m_round == -1);  // default from Order.h line 75
}

TEST_CASE("Order does not reach into global turn state") {
    // Construct and verify in a test binary where g_turn is null.
    MapPoint point(static_cast<sint32>(1), static_cast<sint32>(2));
    Order order(UNIT_ORDER_MOVE, nullptr, point, 0, 99);

    // Pure accessor — no global dereference
    CHECK(order.m_round == 99);
    CHECK(order.m_order == UNIT_ORDER_MOVE);
}
