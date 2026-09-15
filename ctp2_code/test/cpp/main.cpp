// test/cpp/main.cpp
// doctest main — defines DOCTEST_CONFIG_IMPLEMENT and provides main().
// All other test files just include <doctest.h> without this define.

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

#include "headless_test_config.h"   // CTP2_TEST_PROFILE
#include <cstdlib>

int main(int argc, char **argv)
{
    // All test binaries run against the pinned repo profile, never the
    // developer's ~/.ctp2/userprofile.txt.  ProfileDB::Init honors
    // CTP2_PROFILE; setenv propagates to the popen'd ctp2_headless
    // children in the integration suite.  An externally set
    // CTP2_PROFILE still wins (overwrite=0).
    setenv("CTP2_PROFILE", CTP2_TEST_PROFILE, /*overwrite=*/0);

    doctest::Context context(argc, argv);
    return context.run();
}
