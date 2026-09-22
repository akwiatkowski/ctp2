// Real LDL screens and AUI hit-testing, without InitializeApp/StartGame,
// a socket server, audio initialization, world generation, or AI processing.
#include "ctp/c3.h"
#include "ctp/civ3_main.h"
#include "ctp/civapp.h"
#include "ctp/ctp2_utils/appstrings.h"
#include "ctp/ctp2_utils/civlog.h"
#include "doctest.h"
#include "gfx/tilesys/tiledmap.h"
#include "gs/core/tiledmap_observer.h"
#include "gs/database/profileDB.h"
#include "gs/fileio/CivPaths.h"
#include "gs/fileio/prjfile.h"
#include "gs/utility/gameinit.h"
#include "gs/world/World.h"
#include "ui/aui_common/aui_control.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_mouse.h"
#include "ui/aui_ctp2/c3_popupwindow.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/ctp2_button.h"
#include "ui/aui_ctp2/ctp2_listbox.h"
#include "ui/aui_sdl/aui_sdlsurface.h"
#include "ui/interface/initialplaywindow.h"
#include "ui/interface/splash.h"
#include "ui/interface/spnewgamewindow.h"
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

extern sint32 g_ScreenWidth;
extern sint32 g_ScreenHeight;
extern ProjectFile *g_ImageMapPF;
void InitializeImageMaps();

namespace {

// The real queued StartGameAction still executes. Only the expensive session
// launch is replaced; the menu must supply its actual selected settings.
struct MenuApp : CivApp {
    int launches = 0;
    std::string leader;
    sint32 difficulty = -1;
    sint32 StartGame() override {
        ++launches;
        leader = profiledb_Get()->GetLeaderName();
        difficulty = profiledb_Get()->GetDifficulty();
        return 0;
    }
};

struct MenuFixture {
    MenuApp app;
    // Logical event time in milliseconds; separated beyond double-click timing
    // so screen navigation cannot accidentally become a double-click gesture.
    uint32 eventTime = 0;
    static constexpr uint32 kClickIntervalMs = 1000;

    MenuFixture() {
        civlog::Init();
        appstrings_Initialize();
        civapp_Set(&app);
        REQUIRE(app.InitializeEngine() == 0);
        // Use the pinned profile's dimensions, not the host display resolution.
        g_ScreenWidth = profiledb_Get()->GetScreenResWidth();
        g_ScreenHeight = profiledb_Get()->GetScreenResHeight();
        REQUIRE(ui_Initialize() == 0);
        REQUIRE(sharedsurface_Initialize() == AUI_ERRCODE_OK);
        InitializeImageMaps();
        REQUIRE(gameinit_InitializeGameFiles());
        // ponytail: load existing databases intact; split menu records only if
        // this measured startup becomes too slow. No simulation is initialized.
        REQUIRE(app.InitializeAppDB());
        REQUIRE(initialplayscreen_Initialize() == AUI_ERRCODE_OK);
        REQUIRE(initialplayscreen_displayMyWindow() == 0);
        frame();
    }

    ~MenuFixture() {
        spnewgamescreen_Cleanup();
        initialplayscreen_Cleanup();
        sharedsurface_Cleanup();
        auto ui = std::unique_ptr<C3UI>(c3ui_Get());
        ui.reset();
        c3ui_Set(nullptr);
        auto images = std::unique_ptr<ProjectFile>(g_ImageMapPF);
        images.reset();
        g_ImageMapPF = nullptr;
        app.CleanupAppDB();
        auto profile = std::unique_ptr<ProfileDB>(profiledb_Get());
        profile.reset();
        profiledb_Set(nullptr);
        CivPaths_CleanupCivPaths();
        Splash::Cleanup();
        appstrings_Cleanup();
        civapp_Set(nullptr);
    }

    void frame() {
        // Preserve Process() action ordering, but don't poll physical host input.
        aui_MouseEvent noInput{};
        REQUIRE(c3ui_Get()->Process(0, &noInput) == AUI_ERRCODE_OK);
        REQUIRE_FALSE(app.IsGameLoaded());
        REQUIRE(world_Get() == nullptr);
    }

    aui_Control *control(const char *screen, const char *name) {
        auto *result = static_cast<aui_Control *>(aui_Ldl::GetObject(screen, name));
        REQUIRE_MESSAGE(result, screen, ".", name, " is missing");
        return result;
    }

    bool attached(const char *screen) {
        auto *window = static_cast<aui_Window *>(aui_Ldl::GetObject(screen));
        return window && !window->IsHidden() && c3ui_Get()->GetWindow(window->Id()) == window;
    }

    void click(aui_Control *button) {
        REQUIRE(button);
        REQUIRE_FALSE(button->IsHidden());
        REQUIRE_FALSE(button->IsDisabled());
        auto *window = button->GetParentWindow();
        REQUIRE(window);
        REQUIRE(c3ui_Get()->GetWindow(window->Id()) == window);
        REQUIRE_FALSE(window->IsHidden());
        // ToScreen expects parent-relative coordinates, including the control offset.
        POINT point{button->X() + button->Width() / 2, button->Y() + button->Height() / 2};
        REQUIRE(button->ToScreen(&point) == AUI_ERRCODE_OK);
        REQUIRE(point.x >= 0);
        REQUIRE(point.y >= 0);
        REQUIRE(point.x < c3ui_Get()->Width());
        REQUIRE(point.y < c3ui_Get()->Height());
        // Submit separately: dispatch must see hover, press and release even
        // when HandleMouseEvents coalesces adjacent motion events.
        for (BOOL down : {FALSE, TRUE, FALSE}) {
            aui_MouseEvent event{};
            event.position = point;
            event.lbutton = down;
            event.time = ++eventTime;
            REQUIRE(c3ui_Get()->Process(1, &event) == AUI_ERRCODE_OK);
            frame();
        }
        eventTime += kClickIntervalMs;
    }

    void click(const char *screen, const char *name) {
        INFO("Clicking " << screen << "." << name);
        REQUIRE(attached(screen));
        click(control(screen, name));
    }

    void requirePainted(const char *screen, const char *name) {
        auto *button = control(screen, name);
        POINT topLeft{button->X(), button->Y()};
        button->ToScreen(&topLeft);
        auto *surface = static_cast<aui_SDLSurface *>(c3ui_Get()->Secondary())->DDS();
        // Check the actual control rectangle, not unrelated splash/HUD pixels.
        // A uniform rectangle cannot contain a rendered button label.
        Uint8 firstR, firstG, firstB, alpha;
        REQUIRE(
            SDL_ReadSurfacePixel(surface, topLeft.x, topLeft.y, &firstR, &firstG, &firstB, &alpha));
        bool varied = false;
        for (sint32 y = 0; y < button->Height() && !varied; ++y) {
            for (sint32 x = 0; x < button->Width() && !varied; ++x) {
                Uint8 r, g, b;
                REQUIRE(SDL_ReadSurfacePixel(surface, topLeft.x + x, topLeft.y + y, &r, &g, &b,
                                             &alpha));
                varied = r != firstR || g != firstG || b != firstB;
            }
        }
        REQUIRE_MESSAGE(
            varied, (std::string(screen) + "." + name + " was not painted by the normal UI frame"));
    }

    void capture(const char *name) {
        // Passive software-frame artifact: never invalidate/redraw to take a shot.
        // This is not a GPU presentation oracle; native-backend tests remain separate.
        const char *directory = std::getenv("CTP2_MENU_ARTIFACTS");
        if (!directory)
            return;
        std::filesystem::create_directories(directory);
        const auto path = std::filesystem::path(directory) / name;
        auto *surface = static_cast<aui_SDLSurface *>(c3ui_Get()->Secondary());
        REQUIRE(CTP2_SDL_SaveBMP(surface->DDS(), path.string().c_str()));
    }
};

} // namespace

TEST_CASE_FIXTURE(MenuFixture, "menu: mouse navigation and selected settings reach one launch") {
    REQUIRE(attached("InitPlayWindow"));
    capture("main-menu.bmp");
    requirePainted("InitPlayWindow", "NewGameButton");
    click("InitPlayWindow", "NewGameButton");
    REQUIRE_FALSE(attached("InitPlayWindow"));
    REQUIRE(attached("SPNewGameWindow"));
    CHECK(app.launches == 0);
    capture("new-game.bmp");
    requirePainted("SPNewGameWindow", "StartButton");

    click("SPNewGameWindow", "ReturnButton");
    REQUIRE(attached("InitPlayWindow"));
    REQUIRE_FALSE(attached("SPNewGameWindow"));
    click("InitPlayWindow", "NewGameButton");
    REQUIRE(attached("SPNewGameWindow"));

    click("SPNewGameWindow", "DifficultyButton");
    REQUIRE(attached("SPNewGameDiffScreen"));
    auto *choices = static_cast<ctp2_ListBox *>(control("SPNewGameDiffScreen", "DiffBox"));
    // Choose a different visible entry, not the profile's incidental default.
    const sint32 chosenDifficulty = choices->GetSelectedItemIndex() == 0 ? 1 : 0;
    REQUIRE(choices->NumItems() > chosenDifficulty);
    click(choices->GetItemByIndex(chosenDifficulty));
    REQUIRE(choices->GetSelectedItemIndex() == chosenDifficulty);
    auto *popup = static_cast<c3_PopupWindow *>(aui_Ldl::GetObject("SPNewGameDiffScreen"));
    click(popup->Ok());
    REQUIRE_FALSE(attached("SPNewGameDiffScreen"));
    REQUIRE(attached("SPNewGameWindow"));
    CHECK(profiledb_Get()->GetDifficulty() == chosenDifficulty);

    // Reopening the editor must preserve the user's selection.
    click("SPNewGameWindow", "DifficultyButton");
    REQUIRE(choices->GetSelectedItemIndex() == chosenDifficulty);
    click(popup->Ok());
    capture("configured-game.bmp");

    const std::string expectedLeader = profiledb_Get()->GetLeaderName();
    click("SPNewGameWindow", "StartButton");
    CHECK_FALSE(attached("SPNewGameWindow"));
    CHECK(app.launches == 1);
    CHECK(app.leader == expectedLeader);
    CHECK(app.difficulty == chosenDifficulty);
    frame();
    CHECK(app.launches == 1);
}

TEST_CASE_FIXTURE(MenuFixture, "tile cursor: viewport jumps do not write outside the overlay") {
    // An empty World supplies projection dimensions; no map generation or AI.
    // It is larger than this four-tile overlay so the fixture never wraps.
    MapPoint extent(32, 32);
    world_Set(std::make_unique<World>(extent, false, false).release());
    auto *previousObserver = tiledmap_observer::Get();
    TiledMap map(extent);
    struct RestoreMap {
        TiledMap *previous = tiledmap_Get();
        tiledmap_observer::Impl *observer;
        ~RestoreMap() {
            tiledmap_Set(previous);
            tiledmap_observer::Register(observer);
            world_Set(nullptr);
        }
    } restore{tiledmap_Get(), previousObserver};
    tiledmap_Set(&map);
    const sint32 width = 4 * map.GetZoomTilePixelWidth();
    const sint32 height = 4 * map.GetZoomTilePixelHeight() + map.GetZoomTileHeadroom();
    // GPU overlay scratch is RGB565. Extra storage on both sides makes an
    // erroneous negative write deterministic and observable, rather than UB.
    const sint32 pitch = width * sizeof(Pixel16);
    const size_t bytes = static_cast<size_t>(pitch) * height;
    constexpr uint8 kUntouched = 0xA5; // Canary distinct from selection colors.
    std::vector<uint8> backing(3 * bytes, kUntouched);
    SDL_Surface *native =
        SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGB565, backing.data() + bytes, pitch);
    REQUIRE(native);
    AUI_ERRCODE error = AUI_ERRCODE_OK;
    aui_SDLSurface overlay(&error, width, height, 16, native, FALSE, FALSE, TRUE);
    REQUIRE(error == AUI_ERRCODE_OK);
    const MapPoint oldCursor(1, 2);
    *map.GetMapViewRect() = RECT{0, 0, 4, 8};

    SUBCASE("previous hover lies entirely left of the new viewport") {
        map.GetMapViewRect()->left += 4;
        map.GetMapViewRect()->right += 4;
        map.DrawHitMask(&overlay, oldCursor);
        CHECK(std::all_of(backing.begin(), backing.end(),
                          [](uint8 byte) { return byte == kUntouched; }));
    }
    SUBCASE("previous hover lies entirely above the new viewport") {
        map.GetMapViewRect()->top += 8;
        map.GetMapViewRect()->bottom += 8;
        map.DrawHitMask(&overlay, oldCursor);
        CHECK(std::all_of(backing.begin(), backing.end(),
                          [](uint8 byte) { return byte == kUntouched; }));
    }
    SUBCASE("visible cursor still paints only inside the overlay") {
        map.DrawHitMask(&overlay, oldCursor);
        CHECK(std::any_of(backing.begin() + bytes, backing.begin() + 2 * bytes,
                          [](uint8 byte) { return byte != kUntouched; }));
        CHECK(std::all_of(backing.begin(), backing.begin() + bytes,
                          [](uint8 byte) { return byte == kUntouched; }));
        CHECK(std::all_of(backing.begin() + 2 * bytes, backing.end(),
                          [](uint8 byte) { return byte == kUntouched; }));
    }
}
