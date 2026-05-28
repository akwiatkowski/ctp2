// test/cpp/test_render_observer.cpp
// Unit tests for render_observer dispatch + null-safety.
//
// These tests exercise the observer dispatch logic without needing the full
// game engine (no database init, no SDL, no graphics).  They run in < 1 ms.

#include "ctp/c3.h"
#include "doctest.h"
#include "gs/core/render_observer.h"
#include "gs/gameobj/Unit.h"
#include "gs/world/MapPoint.h"

namespace {

struct RecordingSpy : render_observer::Impl {
    // --- Lifecycle counters ---
    int addShowCalls = 0;
    Unit lastAddShowUnit;
    void AddShow(Unit hider) override {
        ++addShowCalls;
        lastAddShowUnit = hider;
    }

    int addHideCalls = 0;
    Unit lastAddHideUnit;
    void AddHide(Unit hider) override {
        ++addHideCalls;
        lastAddHideUnit = hider;
    }

    int addDeathCalls = 0;
    void AddDeath(Unit dead) override { ++addDeathCalls; }

    int addFastKillCalls = 0;
    void AddFastKill(Unit dead) override { ++addFastKillCalls; }

    int fastKillCalls = 0;
    void FastKill(std::shared_ptr<UnitActor> actor) override { ++fastKillCalls; }

    int fastKillEffectCalls = 0;
    void FastKillEffect(EffectActor *actor) override { ++fastKillEffectCalls; }

    int addSetOwnerCalls = 0;
    void AddSetOwner(std::shared_ptr<UnitActor> actor, sint32 owner) override {
        ++addSetOwnerCalls;
    }

    int addSetVisibilityCalls = 0;
    void AddSetVisibility(std::shared_ptr<UnitActor> actor,
                          uint32 visibility) override {
        ++addSetVisibilityCalls;
    }

    int addSetVisionRangeCalls = 0;
    void AddSetVisionRange(std::shared_ptr<UnitActor> actor,
                           double range) override {
        ++addSetVisionRangeCalls;
    }

    int addMorphUnitCalls = 0;
    void AddMorphUnit(std::shared_ptr<UnitActor> morphingActor,
                      SpriteStatePtr ss, sint32 type, Unit id) override {
        ++addMorphUnitCalls;
    }

    int changeUnitImageCalls = 0;
    void ChangeUnitImage(std::shared_ptr<UnitActor> actor,
                         SpriteStatePtr ss, sint32 type, Unit id) override {
        ++changeUnitImageCalls;
    }

    int activeUnitRemoveCalls = 0;
    void ActiveUnitRemove(std::shared_ptr<UnitActor> unitActor) override {
        ++activeUnitRemoveCalls;
    }

    int tradeActorCreateCalls = 0;
    void TradeActorCreate(TradeRoute newRoute) override {
        ++tradeActorCreateCalls;
    }

    int tradeActorDestroyCalls = 0;
    void TradeActorDestroy(TradeRoute routeToDestroy) override {
        ++tradeActorDestroyCalls;
    }

    // --- Animation counters ---
    int addMoveCalls = 0;
    Unit lastMover;
    void AddMove(Unit mover,
                 MapPoint const &oldPos, MapPoint const &newPos,
                 const render_observer::UnitActorVec &revealedActors,
                 const render_observer::UnitActorVec &restOfStack,
                 bool isTransported, sint32 soundID) override {
        ++addMoveCalls;
        lastMover = mover;
    }

    int addTeleportCalls = 0;
    void AddTeleport(Unit top,
                     MapPoint const &oldPos, MapPoint const &newPos,
                     const render_observer::UnitActorVec &revealedActors,
                     const render_observer::UnitActorVec &moveActors) override {
        ++addTeleportCalls;
    }

    int addAttackCalls = 0;
    Unit lastAttacker;
    Unit lastAttacked;
    void AddAttack(Unit attacker, Unit attacked) override {
        ++addAttackCalls;
        lastAttacker = attacker;
        lastAttacked = attacked;
    }

    int addAttackPosCalls = 0;
    void AddAttackPos(Unit attacker, MapPoint const &pos) override {
        ++addAttackPosCalls;
    }

    int addSpecialAttackCalls = 0;
    void AddSpecialAttack(Unit attacker, Unit attacked,
                          sint32 attack) override {
        ++addSpecialAttackCalls;
    }

    int addSpecialEffectCalls = 0;
    void AddSpecialEffect(MapPoint &pos,
                          sint32 spriteID, sint32 soundID) override {
        ++addSpecialEffectCalls;
    }

    int addTerminateFaceoffCalls = 0;
    void AddTerminateFaceoff(Unit &faceoffer) override {
        ++addTerminateFaceoffCalls;
    }

    // --- Camera / visibility query ---
    int addCenterMapCalls = 0;
    MapPoint lastCenterMapPos;
    void AddCenterMap(const MapPoint &pos) override {
        ++addCenterMapCalls;
        lastCenterMapPos = pos;
    }

    int tileWillBeCompletelyVisibleCalls = 0;
    sint32 lastTileWillBeVisibleX = -1;
    sint32 lastTileWillBeVisibleY = -1;
    bool tileWillBeCompletelyVisibleResult = false;
    bool TileWillBeCompletelyVisible(sint32 x, sint32 y) override {
        ++tileWillBeCompletelyVisibleCalls;
        lastTileWillBeVisibleX = x;
        lastTileWillBeVisibleY = y;
        return tileWillBeCompletelyVisibleResult;
    }

    // --- Turn flow counters ---
    int nextPlayerCalls = 0;
    sint32 lastNextPlayerForcedUpdate = -1;
    void NextPlayer(sint32 forcedUpdate) override {
        ++nextPlayerCalls;
        lastNextPlayerForcedUpdate = forcedUpdate;
    }

    int addCopyVisionCalls = 0;
    void AddCopyVision() override { ++addCopyVisionCalls; }

    int addEndTurnCalls = 0;
    void AddEndTurn() override { ++addEndTurnCalls; }

    int catchUpCalls = 0;
    void CatchUp() override { ++catchUpCalls; }

    int addBeginSchedulerCalls = 0;
    void AddBeginScheduler(sint32 player) override { ++addBeginSchedulerCalls; }

    int addPlaySoundCalls = 0;
    sint32 lastPlaySoundID = -1;
    MapPoint lastPlaySoundPos;
    void AddPlaySound(sint32 soundID, MapPoint const &pos) override {
        ++addPlaySoundCalls;
        lastPlaySoundID = soundID;
        lastPlaySoundPos = pos;
    }

    int addPlayWonderMovieCalls = 0;
    void AddPlayWonderMovie(sint32 which) override { ++addPlayWonderMovieCalls; }

    int incrementPendingGameActionsCalls = 0;
    void IncrementPendingGameActions() override {
        ++incrementPendingGameActionsCalls;
    }

    int decrementPendingGameActionsCalls = 0;
    void DecrementPendingGameActions() override {
        ++decrementPendingGameActionsCalls;
    }
};

// RAII helper that restores the previous Impl on destruction.
struct ScopedSpy {
    render_observer::Impl *prev;
    explicit ScopedSpy(render_observer::Impl *s) {
        prev = render_observer::Get();
        render_observer::Register(s);
    }
    ~ScopedSpy() { render_observer::Register(prev); }
};

} // anonymous namespace

TEST_CASE("render_observer::Get returns null when no Impl is registered")
{
    render_observer::Impl *prev = render_observer::Get();
    render_observer::Register(nullptr);
    CHECK(render_observer::Get() == nullptr);
    render_observer::Register(prev);  // restore
}

TEST_CASE("render_observer::Register installs the Impl and Get returns it")
{
    RecordingSpy spy;
    {
        ScopedSpy guard(&spy);
        CHECK(render_observer::Get() == &spy);
    }
}

TEST_CASE("render_observer::AddShow dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    Unit u;
    render_observer::AddShow(u);
    CHECK(spy.addShowCalls == 1);
}

TEST_CASE("render_observer::AddMove dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    MapPoint oldPos(1, 2);
    MapPoint newPos(3, 4);
    Unit mover;
    render_observer::UnitActorVec emptyVec;
    render_observer::AddMove(mover, oldPos, newPos, emptyVec, emptyVec, false, 42);
    CHECK(spy.addMoveCalls == 1);
}

TEST_CASE("render_observer::AddAttack dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    Unit attacker;
    Unit attacked;
    render_observer::AddAttack(attacker, attacked);
    CHECK(spy.addAttackCalls == 1);
}

TEST_CASE("render_observer::AddCenterMap dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    MapPoint pt(5, 7);
    render_observer::AddCenterMap(pt);
    CHECK(spy.addCenterMapCalls == 1);
    CHECK(spy.lastCenterMapPos.x == 5);
    CHECK(spy.lastCenterMapPos.y == 7);
}

TEST_CASE("render_observer::NextPlayer dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    render_observer::NextPlayer(1);
    CHECK(spy.nextPlayerCalls == 1);
    CHECK(spy.lastNextPlayerForcedUpdate == 1);
}

TEST_CASE("render_observer::AddEndTurn dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    render_observer::AddEndTurn();
    CHECK(spy.addEndTurnCalls == 1);
}

TEST_CASE("render_observer::AddPlaySound dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    MapPoint pt(10, 20);
    render_observer::AddPlaySound(99, pt);
    CHECK(spy.addPlaySoundCalls == 1);
    CHECK(spy.lastPlaySoundID == 99);
    CHECK(spy.lastPlaySoundPos.x == 10);
    CHECK(spy.lastPlaySoundPos.y == 20);
}

TEST_CASE("render_observer::IncrementPendingGameActions dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    render_observer::IncrementPendingGameActions();
    CHECK(spy.incrementPendingGameActionsCalls == 1);
}

TEST_CASE("render_observer::TileWillBeCompletelyVisible returns false when no Impl registered")
{
    render_observer::Impl *prev = render_observer::Get();
    render_observer::Register(nullptr);
    CHECK(!render_observer::TileWillBeCompletelyVisible(0, 0));
    render_observer::Register(prev);  // restore
}

TEST_CASE("render_observer::TileWillBeCompletelyVisible forwards to Impl and returns its result")
{
    RecordingSpy spy;
    spy.tileWillBeCompletelyVisibleResult = true;
    ScopedSpy guard(&spy);
    CHECK(render_observer::TileWillBeCompletelyVisible(42, 99) == true);
    CHECK(spy.tileWillBeCompletelyVisibleCalls == 1);
    CHECK(spy.lastTileWillBeVisibleX == 42);
    CHECK(spy.lastTileWillBeVisibleY == 99);
}

TEST_CASE("render_observer::AddHide dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    Unit u;
    render_observer::AddHide(u);
    CHECK(spy.addHideCalls == 1);
}

TEST_CASE("render_observer::AddDeath dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    Unit u;
    render_observer::AddDeath(u);
    CHECK(spy.addDeathCalls == 1);
}

TEST_CASE("render_observer::AddFastKill dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    Unit u;
    render_observer::AddFastKill(u);
    CHECK(spy.addFastKillCalls == 1);
}

TEST_CASE("render_observer::AddSetOwner dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    render_observer::AddSetOwner(nullptr, 7);
    CHECK(spy.addSetOwnerCalls == 1);
}

TEST_CASE("render_observer::AddSetVisibility dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    render_observer::AddSetVisibility(nullptr, 0xFF);
    CHECK(spy.addSetVisibilityCalls == 1);
}

TEST_CASE("render_observer::AddSetVisionRange dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    render_observer::AddSetVisionRange(nullptr, 3.5);
    CHECK(spy.addSetVisionRangeCalls == 1);
}

TEST_CASE("render_observer::ActiveUnitRemove dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    render_observer::ActiveUnitRemove(nullptr);
    CHECK(spy.activeUnitRemoveCalls == 1);
}

TEST_CASE("render_observer::AddTeleport dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    Unit top;
    MapPoint oldPos(1, 2);
    MapPoint newPos(3, 4);
    render_observer::UnitActorVec emptyVec;
    render_observer::AddTeleport(top, oldPos, newPos, emptyVec, emptyVec);
    CHECK(spy.addTeleportCalls == 1);
}

TEST_CASE("render_observer::AddAttackPos dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    Unit attacker;
    MapPoint pt(8, 9);
    render_observer::AddAttackPos(attacker, pt);
    CHECK(spy.addAttackPosCalls == 1);
}

TEST_CASE("render_observer::AddSpecialAttack dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    Unit attacker;
    Unit attacked;
    render_observer::AddSpecialAttack(attacker, attacked, 42);
    CHECK(spy.addSpecialAttackCalls == 1);
}

TEST_CASE("render_observer::AddSpecialEffect dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    MapPoint pt(5, 6);
    render_observer::AddSpecialEffect(pt, 100, 200);
    CHECK(spy.addSpecialEffectCalls == 1);
}

TEST_CASE("render_observer::AddTerminateFaceoff dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    Unit faceoffer;
    render_observer::AddTerminateFaceoff(faceoffer);
    CHECK(spy.addTerminateFaceoffCalls == 1);
}

TEST_CASE("render_observer::AddCopyVision dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    render_observer::AddCopyVision();
    CHECK(spy.addCopyVisionCalls == 1);
}

TEST_CASE("render_observer::CatchUp dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    render_observer::CatchUp();
    CHECK(spy.catchUpCalls == 1);
}

TEST_CASE("render_observer::AddBeginScheduler dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    render_observer::AddBeginScheduler(3);
    CHECK(spy.addBeginSchedulerCalls == 1);
}

TEST_CASE("render_observer::AddPlayWonderMovie dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    render_observer::AddPlayWonderMovie(5);
    CHECK(spy.addPlayWonderMovieCalls == 1);
}

TEST_CASE("render_observer::DecrementPendingGameActions dispatches to Impl")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    render_observer::DecrementPendingGameActions();
    CHECK(spy.decrementPendingGameActionsCalls == 1);
}

TEST_CASE("Null fan-outs are no-ops when no Impl registered")
{
    render_observer::Impl *prev = render_observer::Get();
    render_observer::Register(nullptr);

    Unit u;
    MapPoint pt(1, 2);
    render_observer::UnitActorVec emptyVec;

    render_observer::AddShow(u);
    render_observer::AddHide(u);
    render_observer::AddDeath(u);
    render_observer::AddFastKill(u);
    render_observer::FastKill(nullptr);
    render_observer::FastKillEffect(nullptr);
    render_observer::AddSetOwner(nullptr, 0);
    render_observer::AddSetVisibility(nullptr, 0);
    render_observer::AddSetVisionRange(nullptr, 0.0);
    render_observer::AddMorphUnit(nullptr, nullptr, 0, u);
    render_observer::ActiveUnitRemove(nullptr);
    // TradeRoute is not default-constructible, skip TradeActorCreate/Destroy
    render_observer::AddMove(u, pt, pt, emptyVec, emptyVec, false, 0);
    render_observer::AddTeleport(u, pt, pt, emptyVec, emptyVec);
    render_observer::AddAttack(u, u);
    render_observer::AddAttackPos(u, pt);
    render_observer::AddSpecialAttack(u, u, 0);
    render_observer::AddSpecialEffect(pt, 0, 0);
    render_observer::AddTerminateFaceoff(u);
    render_observer::AddCenterMap(pt);
    render_observer::NextPlayer(0);
    render_observer::AddCopyVision();
    render_observer::AddEndTurn();
    render_observer::CatchUp();
    render_observer::AddBeginScheduler(0);
    render_observer::AddPlaySound(0, pt);
    render_observer::AddPlayWonderMovie(0);
    render_observer::IncrementPendingGameActions();
    render_observer::DecrementPendingGameActions();

    // If we reached this point without crashing, the test passes.
    CHECK(true);

    render_observer::Register(prev);  // restore
}

TEST_CASE("render_observer::AddMove dispatched 3 times accumulates counter")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    MapPoint oldPos(1, 2);
    MapPoint newPos(3, 4);
    Unit mover;
    render_observer::UnitActorVec emptyVec;
    render_observer::AddMove(mover, oldPos, newPos, emptyVec, emptyVec, false, 42);
    render_observer::AddMove(mover, oldPos, newPos, emptyVec, emptyVec, false, 43);
    render_observer::AddMove(mover, oldPos, newPos, emptyVec, emptyVec, false, 44);
    CHECK(spy.addMoveCalls == 3);
}

TEST_CASE("render_observer::AddCenterMap with different MapPoints forwards each correctly")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    MapPoint pt1(5, 7);
    render_observer::AddCenterMap(pt1);
    CHECK(spy.lastCenterMapPos.x == 5);
    CHECK(spy.lastCenterMapPos.y == 7);

    MapPoint pt2(9, 11);
    render_observer::AddCenterMap(pt2);
    CHECK(spy.lastCenterMapPos.x == 9);
    CHECK(spy.lastCenterMapPos.y == 11);
}

TEST_CASE("render_observer::NextPlayer called with default arg forwards 0")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    render_observer::NextPlayer();  // default arg = 0
    CHECK(spy.nextPlayerCalls == 1);
    CHECK(spy.lastNextPlayerForcedUpdate == 0);
}

TEST_CASE("render_observer::NextPlayer called with explicit forcedUpdate=1 forwards 1")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    render_observer::NextPlayer(1);
    CHECK(spy.nextPlayerCalls == 1);
    CHECK(spy.lastNextPlayerForcedUpdate == 1);
}

TEST_CASE("render_observer::IncrementPendingGameActions and DecrementPendingGameActions are distinct calls")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    render_observer::IncrementPendingGameActions();
    render_observer::DecrementPendingGameActions();
    CHECK(spy.incrementPendingGameActionsCalls == 1);
    CHECK(spy.decrementPendingGameActionsCalls == 1);
}

TEST_CASE("render_observer::AddPlaySound forwards soundID and MapPoint correctly")
{
    RecordingSpy spy;
    ScopedSpy guard(&spy);
    MapPoint pt(15, 25);
    render_observer::AddPlaySound(77, pt);
    CHECK(spy.addPlaySoundCalls == 1);
    CHECK(spy.lastPlaySoundID == 77);
    CHECK(spy.lastPlaySoundPos.x == 15);
    CHECK(spy.lastPlaySoundPos.y == 25);
}
