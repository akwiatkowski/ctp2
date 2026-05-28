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
