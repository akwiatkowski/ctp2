#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __TRADEACTOR_H__
#define __TRADEACTOR_H__

#include <deque>
#include <memory>

#include "gfx/spritesys/Actor.h"
#include "gfx/spritesys/GoodSpriteGroup.h"
#include "gs/gameobj/XY_Coordinates.h"
#include "gs/world/World.h"
#include "gs/gameobj/TradeRoute.h"

class SpriteState;
class SpriteGroup;
class aui_Surface;
class ActorPath;
class Action;

class TradeActor : public Actor
{
public:
	explicit TradeActor(TradeRoute newRoute);
	//TradeActor(TradeActor *copy);
	~TradeActor();

	void	Process() override;

  void			AddAction(ActionPtr actionObj) override;
	void			GetNextAction();
	void			AddIdle();

	std::unique_ptr<Anim>	CreateAnim(GOODACTION action);

	void			Draw(const Vision *tileLocalVision);
	void			DrawText(sint32 x, sint32 y, MBCHAR *goodText);

	BOOL			IsAnimating();

	TradeRoute		GetRouteID() { return m_routeID; }

	MapPoint		GetCurrentPos() { return m_currentPos; }
	MapPoint		GetSourcePos() { return m_sourcePos; }
	MapPoint		GetDestPos() { return m_destPos; }
	MapPoint		GetNextPos();
	MapPoint		LookAtNextPos();

	uint16			GetWidth();
	uint16			GetHeight();

	void			GetBoundingRect(RECT *rect);

protected:
	uint32							m_currentPosID;
	uint32							m_sourcePosID;
	uint32							m_destPosID;

	MapPoint						m_currentPos;
	MapPoint						m_sourcePos;
	MapPoint						m_destPos;

	TradeRoute						m_routeID;
	const DynamicArray<MapPoint>*	m_routePath;

	ROUTE_TYPE						m_routeType;
	sint32							m_routeResource;

	GoodSpriteGroup					*m_goodSpriteGroup;
	sint32							m_facing;
	sint32							m_frame;
	uint16							m_transparency;

	GOODACTION						m_curGoodAction;
};

#endif
