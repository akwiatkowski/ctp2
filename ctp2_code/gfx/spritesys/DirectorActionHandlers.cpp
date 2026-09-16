//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Handling of the action on the screen
// Id           : $Id$
//
//----------------------------------------------------------------------------
//
// Disclaimer
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
// This material has been developed at apolyton.net by the Apolyton CtP2
// Source Code Project. Contact the authors at ctp2source@apolyton.net.
//
//----------------------------------------------------------------------------
//
// Compiler flags
//
// _DEBUG
// - Generate debug version when set.
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// Extracted from Director.cpp
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gfx/spritesys/DirectorActionHandlers.h"

#include <memory>

#include "gfx/spritesys/director.h"
#include "gfx/spritesys/EffectActor.h"
#include "gfx/spritesys/SpriteGroupList.h"
#include "gfx/spritesys/UnitActor.h"
#include "gfx/spritesys/spriteutils.h"
#include "gfx/tilesys/maputils.h"
#include "gfx/tilesys/tiledmap.h"  // tiledmap_Get()

#include "ui/aui_ctp2/SelItem.h"

#include "gs/database/profileDB.h"       // profiledb_Get()
#include "gs/events/GameEventManager.h"  // gevmanager_Get()
#include "gs/gameobj/MessagePool.h"

#include "net/general/net_info.h"
#include "net/general/network.h"

#include "sound/soundmanager.h"  // soundmgr_Get()

#include "ui/aui_ctp2/background.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_ctp2/radarmap.h"  // radar_map_Get()

#include "ui/interface/backgroundwin.h"
#include "ui/interface/battleviewwindow.h"  // g_battleViewWindow
#include "ui/interface/cursormanager.h"
#include "ui/interface/messagewin.h"
#include "ui/interface/sci_advancescreen.h"
#include "ui/interface/screenutils.h"
#include "ui/interface/victorymoviewin.h"
#include "ui/interface/wondermoviewin.h"

extern SpriteGroupList* g_unitSpriteGroupList;

void dh_move(DQAction* itemAction,
             SequenceWeakPtr weakSeq,
             DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionMove* action = (DQActionMove*)itemAction;

  Assert(!action->move_actor.expired());
  if (action->move_actor.expired()) {
    director_Get()->ActionFinished(seq);
    return;
  }
  UnitActorPtr theActor = action->move_actor.lock();

  uint32 maxActionCounter = 1;
  sint32 speed = profiledb_Get()->GetUnitSpeed();
  BOOL visible = FALSE;

  ActionPtr actionObj(new Action());

  MapPoint oldP = action->move_oldPos;
  MapPoint newP = action->move_newPos;

  actionObj->SetSequence(seq);
  seq->AddRef();

  actionObj->SetStartMapPoint(oldP);
  actionObj->SetEndMapPoint(newP);

  actionObj->CreatePath(oldP.x, oldP.y, newP.x, newP.y);

  theActor->PositionActor(oldP);

  if (profiledb_Get()->IsUnitAnim())
    maxActionCounter = k_MAX_UNIT_MOVEMENT_ITERATIONS - speed;

  actionObj->SetMaxActionCounter(maxActionCounter);
  actionObj->SetCurActionCounter(0);

  actionObj->SetSoundEffect(action->move_soundID);

  actionObj->SetMoveActors(action->moveActors);

  if (!theActor->ActionMove(std::move(actionObj))) {
    director_Get()->ActionFinished(seq);
    return;
  }

  visible = director_Get()->TileIsVisibleToPlayer(oldP) ||
            director_Get()->TileIsVisibleToPlayer(newP);

  if (visible && executeType == DHEXECUTE_NORMAL) {
    seq->SetAddedToActiveList(SEQ_ACTOR_PRIMARY, TRUE);
    director_Get()->ActiveUnitAdd(theActor);

    if (selitem_Get()->GetVisiblePlayer() != theActor->GetPlayerNum() &&
        !tiledmap_Get()->TileIsVisible(theActor->GetPos().x,
                                   theActor->GetPos().y)) {
      radar_map_Get()->CenterMap(theActor->GetPos());
      tiledmap_Get()->Refresh();
      tiledmap_Get()->InvalidateMap();
      tiledmap_Get()->InvalidateMix();
      background_draw_handler(background_Get());
    }
  } else {
    if (theActor->WillDie())
      director_Get()->FastKill(theActor);
    else
      theActor->EndTurnProcess();
  }
}

void dh_teleport(DQAction* itemAction,
                 SequenceWeakPtr weakSeq,
                 DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionMove* action = (DQActionMove*)itemAction;

  UnitActorPtr theActor = action->move_actor.lock();

  if (theActor == nullptr)
    return;

  if (!action->revealedActors.empty()) {
    for (UnitActorWeakPtr a : action->revealedActors) {
      if (!a.expired())
        a.lock()->SetVisSpecial(TRUE);
    }
  }

  theActor->PositionActor(action->move_newPos);

  for (auto moveActor : action->moveActors) {
    moveActor.lock()->PositionActor(action->move_newPos);
  }

  director_Get()->ActionFinished(seq);
}

void dh_projectileMove(DQAction* itemAction,
                       SequenceWeakPtr weakSeq,
                       DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionMoveProjectile* action = (DQActionMoveProjectile*)itemAction;

  EffectActor* projectileEnd = action->end_projectile.get();
  UnitActorPtr shootingActor = action->pshooting_actor.lock();
  UnitActorPtr targetActor = action->ptarget_actor.lock();
  MapPoint startPos = action->pmove_oldPos;

  Assert(shootingActor != nullptr && targetActor != nullptr);

  if (shootingActor == nullptr || targetActor == nullptr)
    return;

  if (projectileEnd && director_Get()->TileIsVisibleToPlayer(startPos)) {
    ActionPtr actionObj;

    std::unique_ptr<Anim> anim = projectileEnd->CreateAnim(EFFECTACTION_PLAY);
    if (anim == nullptr) {
      anim = projectileEnd->CreateAnim(EFFECTACTION_FLASH);
      Assert(anim != nullptr);
      if (anim == nullptr) {
        director_Get()->ActionFinished(seq);
        return;
      } else {
        actionObj = std::make_shared<Action>(EFFECTACTION_FLASH, ACTIONEND_PATHEND);
      }
    } else {
      actionObj = std::make_shared<Action>(EFFECTACTION_PLAY, ACTIONEND_PATHEND);
    }

    Assert(actionObj);
    if (actionObj) {
      actionObj->SetAnim(std::move(anim));
      projectileEnd->AddAction(std::move(actionObj));
      director_Get()->ActiveEffectAdd(projectileEnd);

      // Management taken over by director, no longer managed by item queue:
      // release ownership without deleting (director now owns the actor).
      action->end_projectile.release();
    }
  }

  director_Get()->ActionFinished(seq);
}

void dh_attack(DQAction* itemAction,
               SequenceWeakPtr weakSeq,
               DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionAttack* action = (DQActionAttack*)itemAction;

  UnitActorPtr theAttacker = action->attacker.lock();
  UnitActorPtr theDefender = action->defender.lock();

  Assert(theAttacker != nullptr);
  Assert(theDefender != nullptr);

  if ((theAttacker == nullptr) || (theDefender == nullptr))
    return;

  //	bool attackerVisible =
  // director_Get()->TileIsVisibleToPlayer(action->attacker_Pos);
  bool defenderVisible =
      director_Get()->TileIsVisibleToPlayer(action->defender_Pos);

  bool playerInvolved =
      (theDefender->GetPlayerNum() == selitem_Get()->GetVisiblePlayer()) ||
      (theAttacker->GetPlayerNum() == selitem_Get()->GetVisiblePlayer());

  POINT AttackerPoints;
  POINT DefenderPoints;

  maputils_MapXY2PixelXY(action->attacker_Pos.x, action->attacker_Pos.y,
                         AttackerPoints);
  maputils_MapXY2PixelXY(action->defender_Pos.x, action->defender_Pos.y,
                         DefenderPoints);

  sint32 deltax = DefenderPoints.x - AttackerPoints.x;
  sint32 deltay = DefenderPoints.y - AttackerPoints.y;

  sint32 facingIndex = spriteutils_DeltaToFacing(deltax, deltay);

  ActionPtr ActionObj(new Action());

  ActionObj->SetSequence(seq);
  seq->AddRef();

  ActionObj->SetStartMapPoint(action->attacker_Pos);
  ActionObj->SetEndMapPoint(action->attacker_Pos);

  theAttacker->ActionAttack(std::move(ActionObj), facingIndex);

  if (playerInvolved && (executeType == DHEXECUTE_NORMAL)) {
    seq->SetAddedToActiveList(SEQ_ACTOR_PRIMARY, TRUE);
    director_Get()->ActiveUnitAdd(theAttacker);
  } else {
    if (theAttacker->WillDie())
      director_Get()->FastKill(theAttacker);
    else
      theAttacker->EndTurnProcess();
  }

  if (!action->defender_IsCity) {
    facingIndex = spriteutils_DeltaToFacing(-deltax, -deltay);

    ActionObj = std::make_shared<Action>();

    ActionObj->SetSequence(seq);
    seq->AddRef();

    ActionObj->SetStartMapPoint(action->defender_Pos);
    ActionObj->SetEndMapPoint(action->defender_Pos);

    theDefender->ActionAttack(std::move(ActionObj), facingIndex);

    if (playerInvolved)
      defenderVisible = true;

    if (defenderVisible && (executeType == DHEXECUTE_NORMAL)) {
      seq->SetAddedToActiveList(SEQ_ACTOR_SECONDARY, TRUE);
      director_Get()->ActiveUnitAdd(theDefender);
    } else {
      if (theDefender->WillDie())
        director_Get()->FastKill(theDefender);
      else
        theDefender->EndTurnProcess();
    }
  }
}

void dh_specialAttack(DQAction* itemAction,
                      SequenceWeakPtr weakSeq,
                      DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionAttack* action = (DQActionAttack*)itemAction;

  UnitActorPtr theAttacker = action->attacker.lock();
  UnitActorPtr theDefender = action->defender.lock();

  Assert(theAttacker != nullptr);
  Assert(theDefender != nullptr);

  if (theAttacker == nullptr)
    return;
  if (theDefender == nullptr)
    return;

  BOOL attackerCanAttack = !action->attacker_IsCity;
  BOOL defenderIsAttackable = !action->defender_IsCity;

  if (!attackerCanAttack && !defenderIsAttackable) {
    director_Get()->ActionFinished(seq);
    return;
  }

  sint32 facingIndex;
  POINT AttackerPoints;
  POINT DefenderPoints;

  maputils_MapXY2PixelXY(action->attacker_Pos.x, action->attacker_Pos.y,
                         AttackerPoints);
  maputils_MapXY2PixelXY(action->defender_Pos.x, action->defender_Pos.y,
                         DefenderPoints);

  sint32 deltax = DefenderPoints.x - AttackerPoints.x;
  sint32 deltay = DefenderPoints.y - AttackerPoints.y;

  if (action->attacker_ID >= 0) {
    soundmgr_Get()->AddSound(SOUNDTYPE_SFX, (uint32)0, action->attacker_ID, 0,
                             0);
  }

  if (attackerCanAttack) {
    ActionPtr AttackerActionObj(new Action());

    AttackerActionObj->SetStartMapPoint(action->attacker_Pos);
    AttackerActionObj->SetEndMapPoint(action->attacker_Pos);

    facingIndex = spriteutils_DeltaToFacing(deltax, deltay);

    AttackerActionObj->SetSequence(seq);
    seq->AddRef();

    if (!theAttacker->ActionSpecialAttack(AttackerActionObj, facingIndex)) {
      director_Get()->ActionFinished(seq);
      return;
    }

    if (director_Get()->TileIsVisibleToPlayer(action->attacker_Pos) &&
        executeType == DHEXECUTE_NORMAL) {
      seq->SetAddedToActiveList(SEQ_ACTOR_PRIMARY, TRUE);
      director_Get()->ActiveUnitAdd(theAttacker);
    } else {
      if (theAttacker->WillDie())
        director_Get()->FastKill(theAttacker);
      else
        theAttacker->EndTurnProcess();
    }
  }

  if (defenderIsAttackable) {
    ActionPtr DefenderActionObj(new Action());

    DefenderActionObj->SetStartMapPoint(action->defender_Pos);
    DefenderActionObj->SetEndMapPoint(action->defender_Pos);

    facingIndex = spriteutils_DeltaToFacing(-deltax, -deltay);

    DefenderActionObj->SetSequence(seq);
    seq->AddRef();

    if (!theDefender->ActionSpecialAttack(std::move(DefenderActionObj),
                                          facingIndex)) {
      director_Get()->ActionFinished(seq);
      return;
    }

    if (director_Get()->TileIsVisibleToPlayer(action->defender_Pos) &&
        executeType == DHEXECUTE_NORMAL) {
      seq->SetAddedToActiveList(SEQ_ACTOR_SECONDARY, TRUE);
      director_Get()->ActiveUnitAdd(theDefender);
    } else {
      if (theDefender->WillDie())
        director_Get()->FastKill(theDefender);
      else
        theDefender->EndTurnProcess();
    }
  }
}

void dh_death(DQAction* itemAction,
              SequenceWeakPtr weakSeq,
              DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionDeath* action = (DQActionDeath*)itemAction;
  UnitActorPtr theDead = action->death_dead;
  UnitActorPtr theVictor = action->death_victor.lock();
  std::unique_ptr<Anim> deathAnim;
  std::unique_ptr<Anim> victorAnim;
  sint32 deathActionType = UNITACTION_NONE;
  sint32 victorActionType = UNITACTION_NONE;

  if (theDead != nullptr && !theDead->GetNeedsToDie()) {
    theDead->SetNeedsToDie(TRUE);

    if (theDead->HasDeath()) {
      if (theDead->GetLoadType() != LOADTYPE_FULL)
        theDead->FullLoad(UNITACTION_VICTORY);

      deathAnim = theDead->CreateAnim(
          (UNITACTION)deathActionType);  // deathAnim must be deleted

      if (deathAnim) {
        deathActionType = UNITACTION_VICTORY;
      } else {
        deathActionType = UNITACTION_FAKE_DEATH;
        deathAnim = theDead->MakeFakeDeath();
      }
    } else {
      deathActionType = UNITACTION_FAKE_DEATH;
      deathAnim = theDead->MakeFakeDeath();
    }
  } else {
    theDead = nullptr;
  }

  if (theVictor != nullptr && !theVictor->GetNeedsToDie()) {
    director_Get()->ActiveUnitRemove(theVictor);

    theVictor->SetNeedsToVictor(TRUE);

    if (theVictor->HasDeath()) {
    } else {
      if (theVictor->GetLoadType() != LOADTYPE_FULL) {
        theVictor->FullLoad(UNITACTION_VICTORY);
      }

      victorActionType = UNITACTION_VICTORY;

      victorAnim = theVictor->CreateAnim((UNITACTION)victorActionType);
      if (victorAnim == nullptr) {
        theVictor = nullptr;
      }
    }
  }

  if (theDead != nullptr) {
    theDead->SetHealthPercent(-1.0);
    theDead->SetTempStackSize(0);

    ActionPtr deadActionObj(
        new Action((UNITACTION)deathActionType, ACTIONEND_ANIMEND));
    Assert(deadActionObj != nullptr);
    if (deadActionObj == nullptr) {
      c3errors_ErrorDialog("Director",
                           "Internal Failure to create death action");
      return;
    }

    deadActionObj->SetSequence(seq);
    seq->AddRef();

    deadActionObj->SetStartMapPoint(action->dead_Pos);
    deadActionObj->SetEndMapPoint(action->dead_Pos);

    deadActionObj->SetAnim(std::move(deathAnim));

    deadActionObj->SetUnitVisionRange(theDead->GetUnitVisionRange());
    deadActionObj->SetUnitsVisibility(theDead->GetUnitVisibility());
    deadActionObj->SetFacing(theDead->GetFacing());

    if (soundmgr_Get())
      soundmgr_Get()->TerminateLoopingSound(SOUNDTYPE_SFX,
                                            (uint32)theDead->GetUnitID());

    if (soundmgr_Get()) {
      sint32 visiblePlayer = selitem_Get()->GetVisiblePlayer();
      if ((visiblePlayer == theDead->GetPlayerNum()) ||
          (theDead->GetUnitVisibility() & (1 << visiblePlayer))) {
        soundmgr_Get()->AddSound(SOUNDTYPE_SFX, (uint32)theDead->GetUnitID(),
                                 action->dead_soundID, theDead->GetPos().x,
                                 theDead->GetPos().y);
      }
    }

    theDead->AddAction(std::move(deadActionObj));

    if (director_Get()->TileIsVisibleToPlayer(action->dead_Pos) &&
        executeType == DHEXECUTE_NORMAL) {
      seq->SetAddedToActiveList(SEQ_ACTOR_PRIMARY, TRUE);
      director_Get()->ActiveUnitAdd(theDead);
    } else {
      if (theDead->WillDie()) {
        director_Get()->FastKill(theDead);
      } else {
        theDead->EndTurnProcess();
      }
    }
  }

  if (theVictor != nullptr) {
    theVictor->SetHealthPercent(-1.0);
    theVictor->SetTempStackSize(0);

    if (theVictor->HasDeath() || victorAnim == nullptr) {
      theVictor->ActionQueueUpIdle();
    } else {
      ActionPtr victorActionObj(
          new Action((UNITACTION)victorActionType, ACTIONEND_ANIMEND));
      if (victorActionObj == nullptr) {
        c3errors_ErrorDialog("Director",
                             "Internal Failure to create victory action");
        return;
      }
      victorActionObj->SetSequence(seq);
      seq->AddRef();
      victorActionObj->SetStartMapPoint(action->victor_Pos);
      victorActionObj->SetEndMapPoint(action->victor_Pos);

      victorActionObj->SetAnim(std::move(victorAnim));

      victorActionObj->SetUnitVisionRange(theVictor->GetUnitVisionRange());
      victorActionObj->SetUnitsVisibility(theVictor->GetUnitVisibility());

      if (soundmgr_Get())
        soundmgr_Get()->TerminateLoopingSound(SOUNDTYPE_SFX,
                                              (uint32)theVictor->GetUnitID());
      if (soundmgr_Get()) {
        sint32 visiblePlayer = selitem_Get()->GetVisiblePlayer();
        if ((visiblePlayer == theVictor->GetPlayerNum()) ||
            (theVictor->GetUnitVisibility() & (1 << visiblePlayer))) {
          soundmgr_Get()->AddSound(
              SOUNDTYPE_SFX, (uint32)theVictor->GetUnitID(),
              action->victor_soundID, theVictor->GetPos().x,
              theVictor->GetPos().y);
        }
      }

      theVictor->AddAction(std::move(victorActionObj));
    }

    if (director_Get()->TileIsVisibleToPlayer(action->victor_Pos) &&
        executeType == DHEXECUTE_NORMAL) {
      seq->SetAddedToActiveList(SEQ_ACTOR_SECONDARY, TRUE);
      director_Get()->ActiveUnitAdd(theVictor);
    } else {
      if (theVictor->WillDie()) {
        director_Get()->FastKill(theVictor);
      } else {
        theVictor->EndTurnProcess();
      }
    }
  }
}

void dh_morphUnit(DQAction* itemAction,
                  SequenceWeakPtr weakSeq,
                  DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionMorph* action = (DQActionMorph*)itemAction;

  UnitActorPtr theActor = action->morphing_actor.lock();

  Assert(theActor != nullptr);
  if (theActor) {
    theActor->ChangeType(action->ss, action->type, action->id, FALSE);
  }

  director_Get()->ActionFinished(seq);
}

void dh_hide(DQAction* itemAction,
             SequenceWeakPtr weakSeq,
             DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionHideShow* action = (DQActionHideShow*)itemAction;

  Assert(action);
  if (!action)
    return;

  UnitActorPtr actor = action->hiding_actor.lock();

  Assert(actor);
  if (!actor)
    return;

  actor->Hide();

  director_Get()->ActionFinished(seq);
}

void dh_show(DQAction* itemAction,
             SequenceWeakPtr weakSeq,
             DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionHideShow* action = (DQActionHideShow*)itemAction;

  Assert(action);
  if (!action)
    return;

  UnitActorPtr actor = action->hiding_actor.lock();

  Assert(actor);
  if (!actor)
    return;

  actor->PositionActor(action->hiding_pos);
  actor->Show();

  director_Get()->ActionFinished(seq);
}

void dh_work(DQAction* itemAction,
             SequenceWeakPtr weakSeq,
             DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionWork* action = (DQActionWork*)itemAction;

  Assert(action);
  if (!action)
    return;

  UnitActorPtr actor = action->working_actor.lock();

  Assert(actor);
  if (!actor)
    return;

  ActionPtr actionObj(new Action(UNITACTION_WORK, ACTIONEND_ANIMEND));

  Assert(actionObj);
  if (actionObj)
    return;

  actionObj->SetStartMapPoint(action->working_pos);
  actionObj->SetEndMapPoint(action->working_pos);

  if (actor->GetLoadType() != LOADTYPE_FULL)
    actor->FullLoad(UNITACTION_WORK);

  std::unique_ptr<Anim> anim = actor->CreateAnim(UNITACTION_WORK);
  if (anim == nullptr) {
    anim = actor->CreateAnim(UNITACTION_MOVE);

    if (!anim) {
      actionObj.reset();
      director_Get()->ActionFinished(seq);
      return;
    }
  }

  actionObj->SetSequence(seq);
  seq->AddRef();

  actionObj->SetAnim(std::move(anim));

  actor->AddAction(std::move(actionObj));

  if (soundmgr_Get()) {
    sint32 visiblePlayer = selitem_Get()->GetVisiblePlayer();
    if ((visiblePlayer == actor->GetPlayerNum()) ||
        (actor->GetUnitVisibility() & (1 << visiblePlayer))) {
      soundmgr_Get()->AddSound(SOUNDTYPE_SFX, (uint32)actor->GetUnitID(),
                               action->working_soundID, actor->GetPos().x,
                               actor->GetPos().y);
    }
  }

  if (director_Get()->TileIsVisibleToPlayer(action->working_pos) &&
      executeType == DHEXECUTE_NORMAL) {
    seq->SetAddedToActiveList(SEQ_ACTOR_PRIMARY, TRUE);
    director_Get()->ActiveUnitAdd(actor);
  } else {
    if (actor->WillDie()) {
      director_Get()->FastKill(actor);
    } else {
      actor->EndTurnProcess();
    }
  }
}

void dh_fastkill(DQAction* itemAction,
                 SequenceWeakPtr weakSeq,
                 DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionFastKill* action = (DQActionFastKill*)itemAction;

  Assert(action);
  if (!action) {
    return;
  }

  director_Get()->FastKill(action->dead);

  director_Get()->ActionFinished(seq);
}

void dh_removeVision(DQAction* itemAction,
                     SequenceWeakPtr weakSeq,
                     DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionVision* action = (DQActionVision*)itemAction;

  Assert(action);
  if (!action)
    return;

  if (tiledmap_Get())
    tiledmap_Get()->RemoveVisible(action->vision_pos, action->vision_range);

  director_Get()->ActionFinished(seq);
}

void dh_addVision(DQAction* itemAction,
                  SequenceWeakPtr weakSeq,
                  DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionVision* action = (DQActionVision*)itemAction;

  Assert(action);
  if (!action)
    return;

  tiledmap_Get()->AddVisible(action->vision_pos, action->vision_range);

  director_Get()->ActionFinished(seq);
}

void dh_setVisibility(DQAction* itemAction,
                      SequenceWeakPtr weakSeq,
                      DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionSetVisibility* action = (DQActionSetVisibility*)itemAction;

  UnitActorPtr actor = action ? action->setvisibility_actor.lock() : nullptr;
  Assert(actor);
  if (!actor)
    return;

  actor->SetUnitVisibility(action->visibilityFlag);

  director_Get()->ActionFinished(seq);
}

void dh_setOwner(DQAction* itemAction,
                 SequenceWeakPtr weakSeq,
                 DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionSetOwner* action = (DQActionSetOwner*)itemAction;

  UnitActorPtr actor(action ? action->setowner_actor.lock() : nullptr);
  Assert(actor);
  if (!actor)
    return;

  actor->SetPlayerNum(action->owner);

  director_Get()->ActionFinished(seq);
}

void dh_setVisionRange(DQAction* itemAction,
                       SequenceWeakPtr weakSeq,
                       DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionSetVisionRange* action = (DQActionSetVisionRange*)itemAction;

  UnitActorPtr actor = action ? action->setvisionrange_actor.lock() : nullptr;
  Assert(actor);
  if (!actor)
    return;

  actor->SetUnitVisionRange(action->range);

  director_Get()->ActionFinished(seq);
}

void dh_combatflash(DQAction* itemAction,
                    SequenceWeakPtr weakSeq,
                    DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionCombatFlash* action = (DQActionCombatFlash*)itemAction;

  SpriteStatePtr ss(new SpriteState(99));
  EffectActor* flash = new EffectActor(ss, action->flash_pos);

  std::unique_ptr<Anim> anim = flash->CreateAnim(EFFECTACTION_PLAY);
  if (anim == nullptr) {
    anim = flash->CreateAnim(EFFECTACTION_FLASH);
    Assert(anim != nullptr);
  }

  if (anim) {
    ActionPtr actionObj(new Action(EFFECTACTION_FLASH, ACTIONEND_PATHEND));
    actionObj->SetAnim(std::move(anim));
    flash->AddAction(std::move(actionObj));
    director_Get()->ActiveEffectAdd(flash);
  }

  director_Get()->ActionFinished(seq);
}

void dh_copyVision(DQAction* itemAction,
                   SequenceWeakPtr weakSeq,
                   DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  //	DQActionCopyVision	*action = (DQActionCopyVision *)itemAction;

  tiledmap_Get()->CopyVision();
  radar_map_Get()->Update();
  director_Get()->ActionFinished(seq);
}

void dh_centerMap(DQAction* itemAction,
                  SequenceWeakPtr weakSeq,
                  DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionCenterMap* action = (DQActionCenterMap*)itemAction;

  if (!selitem_Get()->GetIsPathing()) {
    radar_map_Get()->CenterMap(action->centerMap_pos);

    tiledmap_Get()->Refresh();
    tiledmap_Get()->InvalidateMap();
    tiledmap_Get()->InvalidateMix();

    background_draw_handler(background_Get());
  }

  director_Get()->ActionFinished(seq);
}

void dh_selectUnit(DQAction* itemAction,
                   SequenceWeakPtr weakSeq,
                   DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionUnitSelection* action = (DQActionUnitSelection*)itemAction;

  selitem_Get()->DirectorUnitSelection(action->flags);

  director_Get()->ActionFinished(seq);
}

void dh_endTurn(DQAction* itemAction,
                SequenceWeakPtr weakSeq,
                DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  //	DQActionEndTurn	*action = (DQActionEndTurn *)itemAction;

  director_Get()->ActionFinished(seq);

  gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_EndTurn, GEA_Player,
                         selitem_Get()->GetCurPlayer(), GEA_End);
}

void dh_battle(DQAction* itemAction,
               SequenceWeakPtr weakSeq,
               DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionBattle* action = (DQActionBattle*)itemAction;

  if (battleviewwindow_Get()) {
    BattleViewWindow::Cleanup();
  }

  BattleViewWindow::Initialize(seq);

  if (BattleViewWindow *bvw = battleviewwindow_Get()) {
    bvw->SetupBattle(action->battle);
    c3ui_Get()->AddWindow(bvw);
    cursormanager_Get()->SetCursor(CURSORINDEX_DEFAULT);
  }
}

void dh_playSound(DQAction* itemAction,
                  SequenceWeakPtr weakSeq,
                  DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionPlaySound* action = (DQActionPlaySound*)itemAction;

  if (!soundmgr_Get())
    return;

  soundmgr_Get()->AddSound(SOUNDTYPE_SFX, 0, action->playsound_soundID,
                           action->playsound_pos.x, action->playsound_pos.y);

  director_Get()->ActionFinished(seq);
}

void dh_playWonderMovie(DQAction* itemAction,
                        SequenceWeakPtr weakSeq,
                        DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionPlayWonderMovie* action = (DQActionPlayWonderMovie*)itemAction;

  Assert(action);
  if (!action)
    return;

  sint32 which = action->playwondermovie_which;

  wondermoviewin_Initialize(seq);
  wondermoviewin_DisplayWonderMovie(which);
}

void dh_playVictoryMovie(DQAction* itemAction,
                         SequenceWeakPtr weakSeq,
                         DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionPlayVictoryMovie* action = (DQActionPlayVictoryMovie*)itemAction;

  Assert(action);
  if (!action)
    return;

  GAME_OVER reason = action->playvictorymovie_reason;

  victorymoviewin_Initialize(seq);
  victorymoviewin_DisplayVictoryMovie(reason);
}

void dh_message(DQAction* itemAction,
                SequenceWeakPtr weakSeq,
                DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionMessage* action = (DQActionMessage*)itemAction;

  Assert(action);
  if (!action)
    return;

  if (messagepool_Get()->IsValid(action->message)) {
    if (action->message.IsAlertBox()) {
      if (!messagewin_IsModalMessageDisplayed()) {
        messagewin_CreateModalMessage(action->message);
      }
    } else {
      if (!action->message.AccessData()->GetMessageWindow()) {
        messagewin_CreateMessage(action->message);
      }
      if (action->message.IsInstantMessage()
          // JJB added this to prevent instant messages showing
          // out of turn in hotseat games.
          // With the existing behaviour they would show immediately
          // which would often mean that they show on the wrong players
          // turn.
          &&
          selitem_Get()->GetVisiblePlayer() == action->message.GetOwner()) {
        action->message.Show();
      }
    }
  }

  director_Get()->ActionFinished(seq);
}

void dh_faceoff(DQAction* itemAction,
                SequenceWeakPtr weakSeq,
                DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionFaceoff* action = (DQActionFaceoff*)itemAction;

  Assert(action);
  if (!action)
    return;

  UnitActorPtr theAttacker = action->faceoff_attacker.lock();
  UnitActorPtr theAttacked = action->faceoff_attacked.lock();

  bool attackedIsAttackable = true;

  Assert(theAttacker != nullptr);
  if (theAttacker == nullptr)
    return;

  Assert(theAttacked != nullptr);
  if (theAttacked == nullptr)
    return;

  ActionPtr AttackedActionObj;

  ActionPtr AttackerActionObj(
      new Action(UNITACTION_FACE_OFF, ACTIONEND_INTERRUPT));

  AttackerActionObj->SetSequence(SequenceWeakPtr());

  if (attackedIsAttackable) {
    AttackedActionObj = std::make_shared<Action>(
        UNITACTION_FACE_OFF, ACTIONEND_INTERRUPT);
    AttackedActionObj->SetSequence(SequenceWeakPtr());
  }

  AttackerActionObj->SetStartMapPoint(action->faceoff_attacker_pos);
  AttackerActionObj->SetEndMapPoint(action->faceoff_attacker_pos);

  if (attackedIsAttackable) {
    AttackedActionObj->SetStartMapPoint(action->faceoff_attacked_pos);
    AttackedActionObj->SetEndMapPoint(action->faceoff_attacked_pos);
  }

  std::unique_ptr<Anim> AttackedAnim;

  std::unique_ptr<Anim> AttackerAnim = theAttacker->MakeFaceoff();
  if (AttackerAnim == nullptr) {
    theAttacker->AddIdle(TRUE);
    return;
  }
  AttackerActionObj->SetAnim(std::move(AttackerAnim));

  if (attackedIsAttackable) {
    if (theAttacked->GetLoadType() != LOADTYPE_FULL)
      theAttacked->FullLoad(UNITACTION_IDLE);

    AttackedAnim = theAttacked->MakeFaceoff();

    if (AttackedAnim == nullptr) {
      theAttacked->AddIdle(TRUE);
    }
  }

  POINT AttackerPoints;
  POINT AttackedPoints;

  maputils_MapXY2PixelXY(action->faceoff_attacker_pos.x,
                         action->faceoff_attacker_pos.y, AttackerPoints);
  maputils_MapXY2PixelXY(action->faceoff_attacked_pos.x,
                         action->faceoff_attacked_pos.y, AttackedPoints);

  AttackerActionObj->SetFacing(
      spriteutils_DeltaToFacing(AttackedPoints.x - AttackerPoints.x,
                                AttackedPoints.y - AttackerPoints.y));

  if (AttackedAnim != nullptr) {
    AttackedActionObj->SetAnim(std::move(AttackedAnim));
    AttackedActionObj->SetFacing(
        spriteutils_DeltaToFacing(AttackerPoints.x - AttackedPoints.x,
                                  AttackerPoints.y - AttackedPoints.y));
  }

  AttackerActionObj->SetUnitVisionRange(theAttacker->GetUnitVisionRange());
  AttackerActionObj->SetUnitsVisibility(theAttacker->GetUnitVisibility());

  theAttacker->AddAction(std::move(AttackerActionObj));

  bool attackedVisible = true;

  if (attackedIsAttackable) {
    if (AttackedAnim != nullptr) {
      AttackedActionObj->SetUnitVisionRange(theAttacked->GetUnitVisionRange());

      AttackedActionObj->SetUnitsVisibility(theAttacker->GetUnitVisibility());
      theAttacked->AddAction(std::move(AttackedActionObj));
    }

    attackedVisible =
        director_Get()->TileIsVisibleToPlayer(action->faceoff_attacked_pos);

    if (theAttacker->GetPlayerNum() == selitem_Get()->GetVisiblePlayer() ||
        theAttacked->GetPlayerNum() == selitem_Get()->GetVisiblePlayer())
      attackedVisible = TRUE;

    if (attackedVisible && executeType == DHEXECUTE_NORMAL) {
      seq->SetAddedToActiveList(SEQ_ACTOR_SECONDARY, TRUE);
      director_Get()->ActiveUnitAdd(theAttacked);
    } else {
      if (theAttacked->WillDie()) {
        director_Get()->FastKill(theAttacked);
      } else {
        theAttacked->EndTurnProcess();
      }
    }
  }

  BOOL attackerVisible =
      director_Get()->TileIsVisibleToPlayer(action->faceoff_attacker_pos);

  if (theAttacked->GetPlayerNum() == selitem_Get()->GetVisiblePlayer() ||
      theAttacker->GetPlayerNum() == selitem_Get()->GetVisiblePlayer()) {
    attackerVisible = TRUE;
    attackedVisible = TRUE;
  }
  if (attackerVisible && attackedVisible && executeType == DHEXECUTE_NORMAL) {
    seq->SetAddedToActiveList(SEQ_ACTOR_PRIMARY, TRUE);
    director_Get()->ActiveUnitAdd(theAttacker);
  } else {
    if (theAttacker->WillDie()) {
      director_Get()->FastKill(theAttacker);
    } else {
      theAttacker->EndTurnProcess();
    }
  }

  director_Get()->ActionFinished(seq);
}

void dh_terminateFaceoff(DQAction* itemAction,
                         SequenceWeakPtr weakSeq,
                         DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionTerminateFaceOff* action = (DQActionTerminateFaceOff*)itemAction;

  Assert(action);
  if (!action)
    return;

  UnitActorPtr facerOffer = action->faceroffer.lock();

  if (facerOffer) {
    facerOffer->SetHealthPercent(-1.0);
    facerOffer->SetTempStackSize(0);

    director_Get()->ActiveUnitRemove(facerOffer);
  }

  director_Get()->ActionFinished(seq);
}

void dh_terminateSound(DQAction* itemAction,
                       SequenceWeakPtr weakSeq,
                       DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionTerminateSound* action = (DQActionTerminateSound*)itemAction;

  Assert(action);
  if (!action)
    return;

  if (soundmgr_Get())
    soundmgr_Get()->TerminateLoopingSound(SOUNDTYPE_SFX,
                                          action->terminate_sound_unit.m_id);

  director_Get()->ActionFinished(seq);
}

void dh_speceffect(DQAction* itemAction,
                   SequenceWeakPtr weakSeq,
                   DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionSpecialEffect* action = (DQActionSpecialEffect*)itemAction;

  Assert(action);
  if (!action) {
    director_Get()->ActionFinished(seq);
    return;
  }

  MapPoint pos = action->speceffect_pos;
  sint32 soundID = action->speceffect_soundID;
  sint32 spriteID = action->speceffect_spriteID;

  if (!tiledmap_Get()->GetLocalVision()->IsVisible(pos)) {
    director_Get()->ActionFinished(seq);
    return;
  }

  SpriteStatePtr ss(new SpriteState(spriteID));
  EffectActor* effectActor = new EffectActor(ss, pos);

  std::unique_ptr<Anim> anim = effectActor->CreateAnim(EFFECTACTION_PLAY);

  if (anim) {
    ActionPtr actionObj(new Action(EFFECTACTION_PLAY, ACTIONEND_PATHEND));
    actionObj->SetAnim(std::move(anim));
    effectActor->AddAction(std::move(actionObj));
    director_Get()->ActiveEffectAdd(effectActor);

    if (soundmgr_Get()) {
      soundmgr_Get()->AddSound(SOUNDTYPE_SFX, 0, soundID, pos.x, pos.y);
    }
  }

  director_Get()->ActionFinished(seq);
}

void dh_attackpos(DQAction* itemAction,
                  SequenceWeakPtr weakSeq,
                  DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionAttackPos* action = (DQActionAttackPos*)itemAction;

  Assert(action);
  if (!action)
    return;

  UnitActorPtr theAttacker = action->attackpos_attacker.lock();

  Assert(theAttacker != nullptr);
  if (theAttacker == nullptr || theAttacker->GetNeedsToDie()) {
    director_Get()->ActionFinished(seq);
    return;
  }
  ActionPtr AttackerActionObj(new Action(
      UNITACTION_ATTACK, ACTIONEND_ANIMEND,
      theAttacker->GetHoldingCurAnimPos(UNITACTION_ATTACK),
      theAttacker->GetHoldingCurAnimSpecialDelayProcess(UNITACTION_ATTACK)));

  AttackerActionObj->SetSequence(seq);
  seq->AddRef();

  AttackerActionObj->SetStartMapPoint(action->attackpos_attacker_pos);
  AttackerActionObj->SetEndMapPoint(action->attackpos_attacker_pos);

  std::unique_ptr<Anim> AttackerAnim;

  if (theAttacker->GetLoadType() != LOADTYPE_FULL)
    theAttacker->FullLoad(UNITACTION_ATTACK);

  AttackerAnim = theAttacker->CreateAnim(UNITACTION_ATTACK);

  if (AttackerAnim == nullptr)
    AttackerAnim = theAttacker->CreateAnim(UNITACTION_IDLE);

  AttackerActionObj->SetAnim(std::move(AttackerAnim));

  POINT AttackerPoints;
  POINT AttackedPoints;

  maputils_MapXY2PixelXY(action->attackpos_attacker_pos.x,
                         action->attackpos_attacker_pos.y, AttackerPoints);
  maputils_MapXY2PixelXY(action->attackpos_target_pos.x,
                         action->attackpos_target_pos.y, AttackedPoints);

  AttackerActionObj->SetFacing(
      spriteutils_DeltaToFacing(AttackedPoints.x - AttackerPoints.x,
                                AttackedPoints.y - AttackerPoints.y));

  AttackerActionObj->SetUnitVisionRange(theAttacker->GetUnitVisionRange());
  AttackerActionObj->SetUnitsVisibility(theAttacker->GetUnitVisibility());

  if (soundmgr_Get())
    soundmgr_Get()->TerminateLoopingSound(SOUNDTYPE_SFX,
                                          (uint32)theAttacker->GetUnitID());

  if (soundmgr_Get()) {
    sint32 visiblePlayer = selitem_Get()->GetVisiblePlayer();
    if ((visiblePlayer == theAttacker->GetPlayerNum()) ||
        (theAttacker->GetUnitVisibility() & (1 << visiblePlayer))) {
      soundmgr_Get()->AddSound(SOUNDTYPE_SFX, (uint32)theAttacker->GetUnitID(),
                               action->attackpos_soundID,
                               theAttacker->GetPos().x,
                               theAttacker->GetPos().y);
    }
  }

  theAttacker->AddAction(std::move(AttackerActionObj));

  bool attackerVisible =
      director_Get()->TileIsVisibleToPlayer(action->attackpos_attacker_pos);

  if (attackerVisible && executeType == DHEXECUTE_NORMAL) {
    seq->SetAddedToActiveList(SEQ_ACTOR_PRIMARY, TRUE);
    director_Get()->ActiveUnitAdd(theAttacker);
  } else {
    if (theAttacker->WillDie()) {
      director_Get()->FastKill(theAttacker);
    } else {
      theAttacker->EndTurnProcess();
    }
  }
}

void dh_invokeThroneRoom(DQAction* itemAction,
                         SequenceWeakPtr weakSeq,
                         DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  director_Get()->ActionFinished(seq);
}

void dh_invokeResearchAdvance(DQAction* itemAction,
                              SequenceWeakPtr weakSeq,
                              DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionInvokeResearchAdvance* action =
      (DQActionInvokeResearchAdvance*)itemAction;

  if (!action) {
    director_Get()->ActionFinished(seq);
    return;
  }

  // Preserve the original nullptr-vs-text distinction: an empty message was
  // previously a null pointer (the setter only allocated for non-null input).
  sci_advancescreen_displayMyWindow(
      action->message.empty() ? nullptr
                              : const_cast<MBCHAR*>(action->message.c_str()),
      0, seq);

  action->message.clear();
}

void dh_beginScheduler(DQAction* itemAction,
                       SequenceWeakPtr weakSeq,
                       DHEXECUTE executeType) {
  Assert(!weakSeq.expired());
  SequencePtr seq = weakSeq.lock();

  DQActionBeginScheduler* action = (DQActionBeginScheduler*)itemAction;

  if (!action) {
    director_Get()->ActionFinished(seq);
    return;
  }

#ifdef _DEBUG
  static bool isCurrentPlayerOk =
      true;  // static, to report the error only once
  if (isCurrentPlayerOk) {
    isCurrentPlayerOk = action->player == selitem_Get()->GetCurPlayer();
    Assert(isCurrentPlayerOk);
  }
#endif

  if (network_Get().IsHost()) {
    network_Get().Enqueue(
        new NetInfo(NET_INFO_CODE_BEGIN_SCHEDULER, action->player));
  }

  Assert(director_Get()->m_holdSchedulerSequence.expired());
  if (!network_Get().IsActive() || network_Get().IsLocalPlayer(action->player)) {
    director_Get()->SetHoldSchedulerSequence(seq);
  } else {
    director_Get()->SetHoldSchedulerSequence(SequenceWeakPtr());
  }

  gevmanager_Get()->Pause();
  gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_BeginScheduler, GEA_Player,
                         action->player, GEA_End);
  gevmanager_Get()->Resume();

  if (director_Get()->m_holdSchedulerSequence.expired()) {
    director_Get()->ActionFinished(seq);
  }
}
