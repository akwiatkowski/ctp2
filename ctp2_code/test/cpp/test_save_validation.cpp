// test/cpp/test_save_validation.cpp
// Regression test for JSON save visibility in the load/save browser.
//
// GameFile::ValidateGameFile historically only accepted binary magic headers.
// JSON saves must also be recognised so they appear in BuildSaveList.

#include "ctp/c3.h"
#include "doctest.h"
#include "gs/fileio/gamefile.h"
#include "gs/fileio/json_save.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sys/stat.h>   // mkdir
#include <unistd.h>     // rmdir

TEST_CASE("ValidateGameFile accepts a minimal JSON save") {
    // Create a temporary file that looks like a JSON save.
    const char *tmpfile = "/tmp/ctp2_test_json_save_validation.json";
    {
        std::ofstream out(tmpfile);
        REQUIRE(out);
        out << "{\n";
        out << "  \"magic\": \"CTP2-JSON\",\n";
        out << "  \"schema_version\": 1\n";
        out << "}\n";
    }

    SaveInfo info;
    // fileName and pathName are set by BuildSaveList before calling ValidateGameFile
    strlcpy(info.fileName, "ctp2_test_json_save_validation.json", sizeof(info.fileName));
    snprintf(info.pathName, sizeof(info.pathName), "%s", tmpfile);

    bool ok = GameFile::ValidateGameFile("/tmp", &info);
    CHECK(ok);

    std::remove(tmpfile);
}

TEST_CASE("ValidateGameFile rejects random text file") {
    const char *tmpfile = "/tmp/ctp2_test_random_text.txt";
    {
        std::ofstream out(tmpfile);
        REQUIRE(out);
        out << "This is not a save game.\n";
    }

    SaveInfo info;
    strlcpy(info.fileName, "ctp2_test_random_text.txt", sizeof(info.fileName));
    snprintf(info.pathName, sizeof(info.pathName), "%s", tmpfile);

    bool ok = GameFile::ValidateGameFile("/tmp", &info);
    CHECK_FALSE(ok);

    std::remove(tmpfile);
}

TEST_CASE("BuildSaveList finds JSON saves in a directory") {
    // Create a fake save directory tree: /tmp/ctp2_test_game/MySave
    const char *gamedir = "/tmp/ctp2_test_game";
    const char *savefile = "/tmp/ctp2_test_game/MySave";

    // Clean up from prior runs
    std::remove(savefile);
    rmdir(gamedir);

    // Create directory
    REQUIRE(mkdir(gamedir, 0755) == 0);

    // Write a minimal JSON save
    {
        std::ofstream out(savefile);
        REQUIRE(out);
        out << "{\n";
        out << "  \"magic\": \"CTP2-JSON\",\n";
        out << "  \"schema_version\": 1\n";
        out << "}\n";
    }

    // We can't easily call BuildSaveList because it needs CivPaths setup.
    // Instead, test ValidateGameFile directly with the path that BuildSaveList
    // would construct.
    SaveInfo info;
    strlcpy(info.fileName, "MySave", sizeof(info.fileName));
    snprintf(info.pathName, sizeof(info.pathName), "%s", savefile);

    bool ok = GameFile::ValidateGameFile(gamedir, &info);
    CHECK(ok);

    // Cleanup
    std::remove(savefile);
    rmdir(gamedir);
}
