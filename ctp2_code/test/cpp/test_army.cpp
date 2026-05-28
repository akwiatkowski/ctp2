// test/cpp/test_army.cpp
// Pure unit tests for Army — constructors, comparators, and ID-level methods.
// These tests exercise only methods with zero global-state dependencies.

#include "ctp/c3.h"
#include "doctest.h"
#include "gs/gameobj/Army.h"

TEST_CASE("Army default constructor produces invalid Army") {
    Army a;
    CHECK(a.m_id == 0);
}

TEST_CASE("Army sint32 constructor stores value") {
    Army a(42);
    CHECK(a.m_id == 42);
}

TEST_CASE("Army uint32 constructor stores value") {
    Army a(99u);
    CHECK(a.m_id == 99);
}

TEST_CASE("Army ID copy constructor copies id") {
    ID id(123);
    Army a(id);
    CHECK(a.m_id == 123);
}

TEST_CASE("Army operator== matches identical IDs") {
    Army a(7);
    Army b(7);
    CHECK(a == b);
    CHECK_FALSE(a != b);
}

TEST_CASE("Army operator!= differs for different IDs") {
    Army a(7);
    Army b(8);
    CHECK(a != b);
    CHECK_FALSE(a == b);
}

TEST_CASE("Army operator= assigns id") {
    Army a(5);
    Army b(10);
    b = a;
    CHECK(b.m_id == 5);
    CHECK(b == a);
}

TEST_CASE("Army operator! returns true for zero id") {
    Army a;
    CHECK(!a);
}

TEST_CASE("Army operator! returns false for non-zero id") {
    Army a(1);
    CHECK_FALSE(!a);
}

TEST_CASE("Army operator< compares IDs") {
    Army a(3);
    Army b(5);
    CHECK(a < b);
    CHECK_FALSE(b < a);
}

TEST_CASE("Army operator<= compares IDs") {
    Army a(3);
    Army b(5);
    Army c(3);
    CHECK(a <= b);
    CHECK(a <= c);
    CHECK_FALSE(b <= a);
}

TEST_CASE("Army operator> compares IDs") {
    Army a(5);
    Army b(3);
    CHECK(a > b);
    CHECK_FALSE(b > a);
}

TEST_CASE("Army operator>= compares IDs") {
    Army a(5);
    Army b(3);
    Army c(5);
    CHECK(a >= b);
    CHECK(a >= c);
    CHECK_FALSE(b >= a);
}

TEST_CASE("Army implicit conversion to int") {
    Army a(77);
    int val = a;
    CHECK(val == 77);
}

TEST_CASE("Army implicit conversion to unsigned int") {
    Army a(88);
    unsigned int val = a;
    CHECK(val == 88);
}

TEST_CASE("Army m_id field is directly accessible") {
    Army a(55);
    CHECK(a.m_id == 55);
    a.m_id = 66;
    CHECK(a.m_id == 66);
}

TEST_CASE("Army bitwise AND works on IDs") {
    Army a(0x0F);
    Army b(0xF0);
    uint32 result = a & b;
    CHECK(result == 0x00);
}

TEST_CASE("Army bitwise OR works on IDs") {
    Army a(0x0F);
    Army b(0xF0);
    uint32 result = a | b;
    CHECK(result == 0xFF);
}

TEST_CASE("Army bitwise NOT works on IDs") {
    Army a(0x0F);
    uint32 result = ~a;
    CHECK(result == static_cast<uint32>(~0x0Fu));
}

TEST_CASE("Army logical AND works on IDs") {
    Army a(1);
    Army b(2);
    Army z(0);
    bool result1 = a && b;
    bool result2 = a && z;
    CHECK(result1 == true);
    CHECK(result2 == false);
}

TEST_CASE("Army logical OR works on IDs") {
    Army a(1);
    Army z(0);
    bool result1 = a || z;
    bool result2 = z || Army(0);
    CHECK(result1 == true);
    CHECK(result2 == false);
}
