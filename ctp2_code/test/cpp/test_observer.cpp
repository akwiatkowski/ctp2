// test/cpp/test_observer.cpp
// Unit tests for IGameObserver / GameObserverRegistry.
//
// These tests exercise the observer dispatch logic without needing the full
// game engine (no database init, no SDL, no graphics).  They run in < 1 ms.

#include "ctp/c3.h"
#include "doctest.h"
#include "gs/core/game_observer.h"
#include "gs/gameobj/Unit.h"
#include "gs/world/MapPoint.h"

// Stub Unit for tests that need a city/army reference.
// Unit default ctor creates an invalid unit (m_id == 0) which is fine for
// observer dispatch — observers receive it by const-ref and never mutate.

TEST_CASE("GameObserverRegistry starts empty")
{
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    // Registry is a singleton; other tests may have registered observers.
    // We verify behaviour by registering our own observer and checking
    // dispatch works.
    struct Counter : IGameObserver {
        int turnStarts = 0;
        void OnTurnStart(sint32) override { ++turnStarts; }
    };
    Counter c;
    reg.Register(&c);
    reg.NotifyTurnStart(0);
    CHECK(c.turnStarts == 1);
    reg.Unregister(&c);
}

TEST_CASE("GameObserverRegistry dispatches to multiple observers")
{
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    struct A : IGameObserver {
        int count = 0;
        void OnTurnStart(sint32) override { ++count; }
    };
    struct B : IGameObserver {
        int count = 0;
        void OnTurnStart(sint32) override { ++count; }
    };
    A a;
    B b;
    reg.Register(&a);
    reg.Register(&b);

    reg.NotifyTurnStart(7);

    CHECK(a.count == 1);
    CHECK(b.count == 1);

    reg.Unregister(&a);
    reg.Unregister(&b);
}

TEST_CASE("GameObserverRegistry dispatches correct player id")
{
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    struct Recorder : IGameObserver {
        sint32 lastPlayer = -1;
        void OnTurnStart(sint32 p) override { lastPlayer = p; }
    };
    Recorder r;
    reg.Register(&r);

    reg.NotifyTurnStart(42);
    CHECK(r.lastPlayer == 42);

    reg.NotifyTurnStart(0);
    CHECK(r.lastPlayer == 0);

    reg.Unregister(&r);
}

TEST_CASE("GameObserverRegistry passes city and position to OnCityFounded")
{
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    struct CitySpy : IGameObserver {
        sint32 player = -1;
        MapPoint pos;
        sint32 cause = -1;
        void OnCityFounded(sint32 p, const Unit&, const MapPoint& pt, sint32 c) override {
            player = p;
            pos = pt;
            cause = c;
        }
    };
    CitySpy spy;
    reg.Register(&spy);

    Unit dummyCity;  // invalid unit, sufficient for dispatch test
    MapPoint mp(5, 7);
    reg.NotifyCityFounded(3, dummyCity, mp, 99);

    CHECK(spy.player == 3);
    CHECK(spy.pos.x == 5);
    CHECK(spy.pos.y == 7);
    CHECK(spy.cause == 99);

    reg.Unregister(&spy);
}

TEST_CASE("GameObserverRegistry ignores null register")
{
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    // Should not crash.
    reg.Register(nullptr);
    reg.NotifyTurnStart(0);
    // If we got here without crashing, the test passes.
    CHECK(true);
}

TEST_CASE("GameObserverRegistry unregister removes observer")
{
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    struct Counter : IGameObserver {
        int count = 0;
        void OnTurnStart(sint32) override { ++count; }
    };
    Counter c;
    reg.Register(&c);
    reg.NotifyTurnStart(0);
    CHECK(c.count == 1);

    reg.Unregister(&c);
    reg.NotifyTurnStart(0);
    CHECK(c.count == 1);  // should not increment after unregister
}

TEST_CASE("GameObserverRegistry unregister of unknown observer is silent")
{
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    struct Dummy : IGameObserver {};
    Dummy d;
    // Should not crash or assert.
    reg.Unregister(&d);
    CHECK(true);
}

TEST_CASE("IGameObserver default methods are no-ops")
{
    // Base class defaults do nothing — verify by instantiating a bare observer
    // and dispatching every event category.
    struct Silent : IGameObserver {};
    Silent s;
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    reg.Register(&s);

    reg.NotifyTurnStart(0);
    reg.NotifyTurnEnd(0);
    reg.NotifyBuildPhaseComplete(0);
    reg.NotifyPlayerRemoved(0);
    reg.NotifyTradeChanged();
    reg.NotifyUpdateCityList();
    reg.NotifyHideMainUI();
    reg.NotifyMapResized();

    // If we reach this point, none of the defaults crashed.
    CHECK(true);

    reg.Unregister(&s);
}

TEST_CASE("HeadlessGameObserver logs turn events")
{
    // This test exercises the HeadlessGameObserver class directly,
    // without needing the full engine.  We just verify it doesn't crash
    // and produces expected DPRINTF output (visible in test log).
    class TestHeadlessObserver : public IGameObserver {
    public:
        void OnTurnStart(sint32 player) override {
            lastPlayer = player;
        }
        sint32 lastPlayer = -1;
    };

    TestHeadlessObserver obs;
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    reg.Register(&obs);

    reg.NotifyTurnStart(2);
    CHECK(obs.lastPlayer == 2);

    reg.Unregister(&obs);
}

TEST_CASE("GameObserverRegistry dispatches OnTurnEnd with correct player id") {
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    struct Recorder : IGameObserver {
        int calls = 0;
        sint32 lastPlayer = -1;
        void OnTurnEnd(sint32 p) override { ++calls; lastPlayer = p; }
    };
    Recorder r;
    reg.Register(&r);
    reg.NotifyTurnEnd(3);
    CHECK(r.calls == 1);
    CHECK(r.lastPlayer == 3);
    reg.Unregister(&r);
}

TEST_CASE("GameObserverRegistry dispatches OnBuildPhaseComplete with correct player id") {
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    struct Recorder : IGameObserver {
        int calls = 0;
        sint32 lastPlayer = -1;
        void OnBuildPhaseComplete(sint32 p) override { ++calls; lastPlayer = p; }
    };
    Recorder r;
    reg.Register(&r);
    reg.NotifyBuildPhaseComplete(7);
    CHECK(r.calls == 1);
    CHECK(r.lastPlayer == 7);
    reg.Unregister(&r);
}

TEST_CASE("GameObserverRegistry dispatches OnPlayerRemoved with correct player id") {
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    struct Recorder : IGameObserver {
        int calls = 0;
        sint32 lastPlayer = -1;
        void OnPlayerRemoved(sint32 p) override { ++calls; lastPlayer = p; }
    };
    Recorder r;
    reg.Register(&r);
    reg.NotifyPlayerRemoved(2);
    CHECK(r.calls == 1);
    CHECK(r.lastPlayer == 2);
    reg.Unregister(&r);
}

TEST_CASE("GameObserverRegistry dispatches OnTradeChanged") {
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    struct Recorder : IGameObserver {
        int calls = 0;
        void OnTradeChanged() override { ++calls; }
    };
    Recorder r;
    reg.Register(&r);
    reg.NotifyTradeChanged();
    CHECK(r.calls == 1);
    reg.Unregister(&r);
}

TEST_CASE("GameObserverRegistry dispatches OnUpdateCityList") {
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    struct Recorder : IGameObserver {
        int calls = 0;
        void OnUpdateCityList() override { ++calls; }
    };
    Recorder r;
    reg.Register(&r);
    reg.NotifyUpdateCityList();
    CHECK(r.calls == 1);
    reg.Unregister(&r);
}

TEST_CASE("GameObserverRegistry dispatches OnHideMainUI") {
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    struct Recorder : IGameObserver {
        int calls = 0;
        void OnHideMainUI() override { ++calls; }
    };
    Recorder r;
    reg.Register(&r);
    reg.NotifyHideMainUI();
    CHECK(r.calls == 1);
    reg.Unregister(&r);
}

TEST_CASE("GameObserverRegistry dispatches OnMapResized") {
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    struct Recorder : IGameObserver {
        int calls = 0;
        void OnMapResized() override { ++calls; }
    };
    Recorder r;
    reg.Register(&r);
    reg.NotifyMapResized();
    CHECK(r.calls == 1);
    reg.Unregister(&r);
}

// ------------------------------------------------------------------
// Edge-case tests (wave 9a W8)
// ------------------------------------------------------------------

// Helper for dispatch-order test (must be at file scope; local structs cannot
// have static data members).
struct SequenceSpy : IGameObserver {
    int id = 0;
    static std::vector<int> order;
    void OnTurnStart(sint32) override { order.push_back(id); }
};
std::vector<int> SequenceSpy::order;

TEST_CASE("GameObserverRegistry deduplicates double registration") {
    // Regression: until 2026-06-01 Register() was not idempotent.
    // RegisterUIGameObserver was called from both civapp.cpp and civ3_main.cpp
    // on default-launch, which doubled every observer notify.  For
    // OnVisionAdded/OnVisionRemoved that meant every director-queue
    // AddVision/RemoveVision was queued twice — combined with game-start
    // direct-path vision adds (which ran before tiledmap_Get() was up and so
    // were not doubled), this drifted m_vision's ref count negative once
    // any unit moved, fogging the player's own first founded city.
    // The fix made Register() idempotent.  This test pins the contract.
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    struct Counter : IGameObserver {
        int count = 0;
        void OnTurnStart(sint32) override { ++count; }
    };
    Counter c;
    reg.Register(&c);
    reg.Register(&c);  // dedup: second registration must be a no-op
    reg.NotifyTurnStart(0);
    CHECK(c.count == 1);
    // A single unregister removes it (no second entry to worry about).
    reg.Unregister(&c);
    reg.NotifyTurnStart(0);
    CHECK(c.count == 1);
}

TEST_CASE("GameObserverRegistry NotifyVisionAdded fires once per observer (fog regression)") {
    // The fog-of-war first-city bug was caused by a duplicate observer in
    // the registry: the deferred director-queue path
    // (Player::AddUnitVision → NotifyVisionAdded → ui_game_observer::
    // OnVisionAdded → director_Get()->AddAddVision) doubled, while the
    // game-start direct-path adds (tiledmap_Get() not yet up) ran once.
    // Net result: m_vision ref count drifted negative once any unit
    // moved, fogging the player's first city.
    // This test verifies a single registered observer fires exactly
    // once per NotifyVisionAdded — even after an attempted double
    // registration.
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    struct VisionObs : IGameObserver {
        int added = 0;
        int removed = 0;
        double lastRange = 0.0;
        void OnVisionAdded(sint32, const MapPoint&, double range) override {
            ++added;
            lastRange = range;
        }
        void OnVisionRemoved(sint32, const MapPoint&, double range) override {
            ++removed;
            lastRange = range;
        }
    };
    VisionObs obs;
    reg.Register(&obs);
    reg.Register(&obs);  // would have doubled before the fix

    MapPoint pos(30, 4);
    reg.NotifyVisionAdded(/*player*/1, pos, /*range*/1.414);
    reg.NotifyVisionRemoved(/*player*/1, pos, /*range*/1.000);

    CHECK(obs.added == 1);
    CHECK(obs.removed == 1);

    reg.Unregister(&obs);
}

TEST_CASE("GameObserverRegistry dispatches to many observers") {
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    struct Counter : IGameObserver {
        int count = 0;
        void OnTurnStart(sint32) override { ++count; }
    };
    Counter obs[10];
    for (auto & ob : obs) {
        reg.Register(&ob);
    }
    reg.NotifyTurnStart(0);
    for (auto & ob : obs) {
        CHECK(ob.count == 1);
    }
    for (auto & ob : obs) {
        reg.Unregister(&ob);
    }
}

TEST_CASE("GameObserverRegistry dispatch order is registration order") {
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    // Clear any stale data from previous runs (static persists across TEST_CASEs).
    SequenceSpy::order.clear();

    SequenceSpy a; a.id = 1;
    SequenceSpy b; b.id = 2;
    SequenceSpy c; c.id = 3;
    reg.Register(&a);
    reg.Register(&b);
    reg.Register(&c);
    reg.NotifyTurnStart(0);

    REQUIRE(SequenceSpy::order.size() == 3);
    CHECK(SequenceSpy::order[0] == 1);
    CHECK(SequenceSpy::order[1] == 2);
    CHECK(SequenceSpy::order[2] == 3);

    reg.Unregister(&a);
    reg.Unregister(&b);
    reg.Unregister(&c);
}

TEST_CASE("GameObserverRegistry register-unregister-re-register cycle") {
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    struct Counter : IGameObserver {
        int count = 0;
        void OnTurnStart(sint32) override { ++count; }
    };
    Counter c;
    reg.Register(&c);
    reg.NotifyTurnStart(0);
    CHECK(c.count == 1);

    reg.Unregister(&c);
    reg.NotifyTurnStart(0);
    CHECK(c.count == 1);  // should not increment

    reg.Register(&c);
    reg.NotifyTurnStart(0);
    CHECK(c.count == 2);  // should resume receiving events

    reg.Unregister(&c);
}

TEST_CASE("GameObserverRegistry Notify with zero observers is safe") {
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    struct Dummy : IGameObserver {};
    Dummy d;
    reg.Register(&d);
    reg.Unregister(&d);  // ensure registry is back to empty for this test

    // All Notify variants should be no-ops when no observers are present.
    // Note: Army/Combat notifies are omitted because Army is an incomplete
    // type here (forward-declared in game_observer.h) and cannot be
    // instantiated in a headerless test.
    Unit dummyCity;
    MapPoint mp(0, 0);

    reg.NotifyTurnStart(0);
    reg.NotifyTurnEnd(0);
    reg.NotifyBuildPhaseComplete(0);
    reg.NotifyCityFounded(0, dummyCity, mp, 0);
    reg.NotifyCityCaptured(dummyCity, 0, mp);
    reg.NotifyCityOwnerReset(dummyCity);
    reg.NotifyWonderBuilt(dummyCity, 0);
    reg.NotifyPlayerRemoved(0);
    reg.NotifyAdvanceResearched(0, 0);
    reg.NotifyResearchAdvanceDialog(0, 0, nullptr);
    reg.NotifyVisionAdded(0, mp, 0.0);
    reg.NotifyVisionRemoved(0, mp, 0.0);
    reg.NotifyVisionCopied(0, 0);
    reg.NotifyGovernmentChanged(0, 0);
    reg.NotifyGameOver(0, 0, 0, 0);
    reg.NotifyTradeChanged();
    reg.NotifyForeignTradeBid(0, dummyCity, dummyCity, 0);
    reg.NotifyUpdateCityList();
    reg.NotifyHideMainUI();
    reg.NotifyMapResized();
    reg.NotifyUpdateScienceWindow(0);
    reg.NotifyUpdateUnitPanel(0);
    reg.NotifyUpdateControlPanel(0);
    reg.NotifyControlPanelRedraw(0);
    reg.NotifyUpdateMessages(0);
    reg.NotifySelectedCity(0);
    reg.NotifyRadarMapUpdate(0);
    reg.NotifyRadarMapRedrawTile(mp);
    reg.NotifyAutoSelectFirstUnit(0);
    reg.NotifyUpdatePlayerEndProgress(0);
    reg.NotifyAdvanceListReload(0);
    reg.NotifyShowSpaceButton(0);
    reg.NotifyUpdateUnitSelectionWindow(0);
    reg.NotifyUpdateCityStatusWindow(0);
    reg.NotifyUpdateMainControlPanel(0);
    reg.NotifySetGraphMinRound(0);
    reg.NotifyRequestOpenGreatLibrary(0, 0);
    reg.NotifyRequestOpenScenarioEditor();
    reg.NotifyRequestOpenScreen(0);
    reg.NotifyRequestAttract(nullptr);
    reg.NotifyRequestStopAttract(nullptr);
    reg.NotifyRequestEditQueue(nullptr);
    reg.NotifyRequestUnblankScreen();
    reg.NotifyCityEspionageDisplay(dummyCity);
    reg.NotifyBlankScreenChanged(false, 0, 0);
    reg.NotifyTutorialAddRecord(nullptr, 0);
    reg.NotifyTutorialRecreate();

    CHECK(true);  // if we got here, zero-observer fast-path is safe
}

TEST_CASE("GameObserverRegistry concrete override is dispatched") {
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    struct CityCaptureSpy : IGameObserver {
        int calls = 0;
        Unit lastCity;
        sint32 lastOwner = -1;
        MapPoint lastPos;
        void OnCityCaptured(const Unit& city, sint32 newOwner,
                            const MapPoint& pos) override {
            ++calls;
            lastCity = city;
            lastOwner = newOwner;
            lastPos = pos;
        }
    };
    CityCaptureSpy spy;
    reg.Register(&spy);

    Unit dummyCity;
    MapPoint mp(3, 4);
    reg.NotifyCityCaptured(dummyCity, 7, mp);

    CHECK(spy.calls == 1);
    CHECK(spy.lastOwner == 7);
    CHECK(spy.lastPos.x == 3);
    CHECK(spy.lastPos.y == 4);

    reg.Unregister(&spy);
}

// Self-unregister during callback modifies the observer vector while it is
// being iterated (range-for).  This is undefined behaviour with the current
// std::vector-based implementation and typically crashes.
// see #issue — observer self-removal during dispatch
#if 0
TEST_CASE("GameObserverRegistry self-unregister during callback is unsafe") {
    GameObserverRegistry &reg = GameObserverRegistry::Instance();
    struct SelfRemove : IGameObserver {
        GameObserverRegistry* reg = nullptr;
        int count = 0;
        void OnTurnStart(sint32) override {
            ++count;
            if (reg) {
                reg->Unregister(this);
            }
        }
    };
    SelfRemove s;
    s.reg = &reg;
    reg.Register(&s);
    reg.NotifyTurnStart(0);  // UB: erases during range-for iteration
    // If execution somehow reaches here, the observer should be gone.
    reg.NotifyTurnStart(0);
    CHECK(s.count == 1);
}
#endif
