#ifndef __BATTLEEVENT_H__
#define __BATTLEEVENT_H__

#include "ctp/ctp2_utils/pointerlist.h"

#include <memory>






enum BATTLE_EVENT_TYPE {
	BATTLE_EVENT_TYPE_NONE  = -1,

	BATTLE_EVENT_TYPE_PLACEMENT,
	BATTLE_EVENT_TYPE_ATTACK,
	BATTLE_EVENT_TYPE_EXPLODE,
	BATTLE_EVENT_TYPE_DEATH,

	BATTLE_EVENT_TYPE_MAX
};

class BattleViewActor;
class EffectActor;
class aui_Surface;

union BattleEventData {

	struct {
		BattleViewActor *actor;
	};

	struct {
		BattleViewActor *positionActor;
		sint32			positionColumn;
		sint32			positionRow;
		sint32			positionFacing;
		double			positionHP;
		BOOL			positionIsDefender;
	};

	struct {
		BattleViewActor *attackActor;
		sint32			attackSoundID;
		double			attackHP;
	};

	struct {
		BattleViewActor *explodeVictim;
		EffectActor		*explodeActor;
		sint32			explodeSoundID;
		double			explodeHP;
	};

	struct {
		BattleViewActor *deathVictim;
		sint32			deathSoundID;
		double			deathHP;
	};
};

class BattleEvent {
public:
	BattleEvent(BATTLE_EVENT_TYPE type);
	~BattleEvent();

	void Initialize();

	void ProcessPlacement();
	void ProcessAttack();
	void ProcessExplode();
	void ProcessDeath();
	void Process();

	void DrawExplosions(aui_Surface *surface);

	BOOL					IsFinished() const { return m_finished; }
	void					SetFinished(BOOL finished) { m_finished = TRUE; }

	void					SetType(BATTLE_EVENT_TYPE type) { m_type = type; }

	BATTLE_EVENT_TYPE		GetType() const { return m_type; }

	BOOL					HasActor(BattleViewActor *actor);

	void					AddPositionData(BattleViewActor *actor, sint32 column, sint32 row,
												sint32 facing, double hp, BOOL isDefender);
	void					AddAttackData(BattleViewActor *actor, sint32 soundID, double hp);
	void					AddExplosionData(BattleViewActor *actor, EffectActor *explodeActor, sint32 soundID, double hp);
	void					AddDeathData(BattleViewActor *actor, sint32 soundID, double hp);

	BattleViewActor			*GetActor();
	PointerList<BattleEventData>	*GetDataList() { return m_dataList.get(); }

	void RemoveDeadActor(BattleViewActor *actor);
private:
	BATTLE_EVENT_TYPE						m_type;
	std::unique_ptr<PointerList<BattleEventData>>			m_dataList;
	std::unique_ptr<PointerList<BattleEventData>::Walker>	m_walker;
	BOOL									m_animating;
	BOOL									m_finished;
};

#endif
