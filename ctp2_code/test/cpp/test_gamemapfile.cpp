// test/cpp/test_gamemapfile.cpp
// GameMapFile JSON port: Save emits a bounded, self-describing document;
// Restore round-trips terrain while stripping unit-era cell state
// (the old SerializeJustMap semantics); ValidateGameMapFile fills the
// caller's SaveMapInfo without touching its path fields.

#include "ctp/c3.h"
#include "doctest.h"
#include "ctp/civapp.h"
#include "gs/world/World.h"
#include "gs/world/Cell.h"
#include "gs/gameobj/player.h"     // PLAYER_UNASSIGNED
#include "gs/fileio/gamefile.h"
#include "gs/fileio/CivPaths.h"
#include "gs/utility/gameinit.h"
#include "gs/database/profileDB.h"
#include <cstdio>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>

namespace {

// Same trampoline shape as test_citydata: world_Set routes through
// civapp_Get()->GetGame(), so a lightweight CivApp hosts the world.
// SetTerrain recomputes movement data through g_theTerrainDB, so the
// record DBs must be loaded — once per process, same pattern as
// BuildQueueFixture / HeavyCityDataFixture.
struct GameMapFixture
{
    static bool s_dbsLoaded;

    CivApp * app = nullptr;

    GameMapFixture()
    {
        app = new CivApp();
        civapp_Set(app);

        if (!s_dbsLoaded)
        {
            set_headless(true);

            fprintf(stderr, "[GameMapFixture] Loading databases...\n");
            CivPaths_InitCivPaths();
            gameinit_InitializeGameFiles();

            profiledb_Set(new ProfileDB());
            profiledb_Get()->Init(FALSE);

            app->InitializeAppDB();

            fprintf(stderr, "[GameMapFixture] Databases loaded.\n");
            s_dbsLoaded = true;
        }

        world_Set(new World(MapPoint(20, 20), false, false));
    }

    ~GameMapFixture()
    {
        delete app;   // ~Game tears down m_world for us
        civapp_Set(nullptr);
    }
};

bool GameMapFixture::s_dbsLoaded = false;

char const kMapPath[] = "/tmp/ctp2_test_gamemap.json";
char const kBadPath[] = "/tmp/ctp2_test_gamemap_bad.json";

void FillInfo(SaveMapInfo &info)
{
    strlcpy(info.gameMapName, "test map", sizeof(info.gameMapName));
    strlcpy(info.note,        "a note",   sizeof(info.note));
    info.radarMapWidth  = 2;
    info.radarMapHeight = 2;
    info.radarMapData   = {1, 2, 3, 4};
}

} // namespace

TEST_CASE_FIXTURE(GameMapFixture, "GameMapFile::Save emits magic, info and world blocks")
{
    SaveMapInfo info;
    FillInfo(info);

    std::remove(kMapPath);
    CHECK(GameMapFile().Save(kMapPath, &info) == GAMEFILE_ERR_STORE_OK);

    std::ifstream in(kMapPath);
    nlohmann::json const doc = nlohmann::json::parse(in);
    CHECK(doc.at("magic") == "CTP2-GAMEMAP");
    CHECK(doc.at("schema_version") == 1);
    CHECK(doc.at("world").at("size_x") == 20);
    CHECK(doc.at("world").at("size_y") == 20);
    CHECK(doc.at("world").at("cells").size() == 20);
    CHECK(doc.at("info").at("game_map_name") == "test map");
    CHECK(doc.at("info").at("note") == "a note");
    CHECK(doc.at("info").at("radar_map").at("data").size() == 4);
    std::remove(kMapPath);
}

TEST_CASE_FIXTURE(GameMapFixture, "GameMapFile::Restore round-trips terrain, strips unit state")
{
    Cell * cell = world_Get()->GetCell(3, 4);
    cell->SetTerrain(5);
    cell->SetEnvFast(cell->GetEnv() | k_MASK_ENV_ROAD | k_MASK_ENV_CITY);
    cell->SetOwner(2);

    std::remove(kMapPath);
    REQUIRE(GameMapFile().Save(kMapPath, nullptr) == GAMEFILE_ERR_STORE_OK);

    // Dirty the live world so restore has visible work to do.
    cell->SetTerrain(9);
    cell->SetEnvFast(0);

    CHECK(GameMapFile().Restore(kMapPath) == GAMEFILE_ERR_LOAD_OK);

    Cell * loaded = world_Get()->GetCell(3, 4);
    CHECK(loaded->GetTerrain() == 5);
    CHECK((loaded->GetEnv() & k_MASK_ENV_ROAD) == 0);
    CHECK((loaded->GetEnv() & k_MASK_ENV_CITY) == 0);
    CHECK(loaded->GetOwner() == PLAYER_UNASSIGNED);
    std::remove(kMapPath);
}

TEST_CASE_FIXTURE(GameMapFixture, "GameMapFile::Restore rejects missing and malformed files")
{
    std::remove(kMapPath);
    CHECK(GameMapFile().Restore(kMapPath) == GAMEFILE_ERR_LOAD_FAILED);

    {
        std::ofstream bad(kBadPath);
        bad << "{\"magic\":\"nope\",\"world\":{}}";
    }
    CHECK(GameMapFile().Restore(kBadPath) == GAMEFILE_ERR_LOAD_FAILED);
    std::remove(kBadPath);
}

TEST_CASE_FIXTURE(GameMapFixture, "ValidateGameMapFile fills info, preserves path fields")
{
    SaveMapInfo info;
    FillInfo(info);
    strlcpy(info.fileName, "ctp2_test_gamemap.json", sizeof(info.fileName));

    std::remove(kMapPath);
    REQUIRE(GameMapFile().Save(kMapPath, &info) == GAMEFILE_ERR_STORE_OK);

    SaveMapInfo probe;
    strlcpy(probe.fileName, "ctp2_test_gamemap.json", sizeof(probe.fileName));
    strlcpy(probe.pathName, "/sentinel",            sizeof(probe.pathName));

    REQUIRE(GameMapFile::ValidateGameMapFile("/tmp", &probe));
    CHECK(std::string(probe.gameMapName) == "test map");
    CHECK(std::string(probe.note)        == "a note");
    CHECK(probe.radarMapWidth  == 2);
    CHECK(probe.radarMapHeight == 2);
    CHECK(probe.radarMapData == std::vector<Pixel16>({1, 2, 3, 4}));
    // Caller-owned path fields survive validation.
    CHECK(std::string(probe.fileName) == "ctp2_test_gamemap.json");
    CHECK(std::string(probe.pathName) == "/sentinel");

    // Wrong magic rejected.
    {
        std::ofstream bad(kBadPath);
        bad << "{\"magic\":\"nope\"}";
    }
    SaveMapInfo probe2;
    strlcpy(probe2.fileName, "ctp2_test_gamemap_bad.json", sizeof(probe2.fileName));
    CHECK(!GameMapFile::ValidateGameMapFile("/tmp", &probe2));

    std::remove(kMapPath);
    std::remove(kBadPath);
}
