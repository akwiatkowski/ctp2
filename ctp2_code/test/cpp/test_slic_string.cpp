#include "ctp/c3.h"

#include "doctest.h"

#include "gs/slic/SlicConst.h"
#include "gs/slic/SlicNamedSymbol.h"
#include "gs/slic/SlicFunc.h"

TEST_CASE("SlicConst stores name and value")
{
    SlicConst c("TEST_CONSTANT", 42);

    CHECK(std::string(c.GetName()) == "TEST_CONSTANT");
    CHECK(c.GetValue() == 42);
}

TEST_CASE("SlicConst empty name is safe")
{
    SlicConst c("", 0);

    CHECK(std::string(c.GetName()) == "");
    CHECK(c.GetValue() == 0);
}

TEST_CASE("SlicConst copy preserves values")
{
    SlicConst original("ORIGINAL", 123);
    SlicConst copy = original;  // implicit copy ctor

    CHECK(std::string(copy.GetName()) == "ORIGINAL");
    CHECK(copy.GetValue() == 123);
}

TEST_CASE("SlicNamedSymbol stores name")
{
    SlicNamedSymbol sym("my_symbol", SLIC_SYM_STRING);

    CHECK(std::string(sym.GetName()) == "my_symbol");
}

TEST_CASE("SlicNamedSymbol handles empty init")
{
    SlicNamedSymbol sym;
    sym.Init("");

    CHECK(std::string(sym.GetName()) == "");
}

TEST_CASE("SlicNamedSymbol DelName clears name")
{
    SlicNamedSymbol sym("to_clear", SLIC_SYM_STRING);
    sym.DelName();

    CHECK(std::string(sym.GetName()) == "");
}

TEST_CASE("SlicFunc prefixes name with underscore")
{
    SlicFunc func("MyFunction", SFR_VOID);

    CHECK(std::string(func.GetName()) == "_MyFunction");
}

TEST_CASE("SlicFunc empty name is safe")
{
    SlicFunc func("", SFR_INT);

    CHECK(std::string(func.GetName()) == "_");
}
