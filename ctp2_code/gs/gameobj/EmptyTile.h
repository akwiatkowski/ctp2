#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __EMPTY_TILE_H__
#define __EMPTY_TILE_H__

#include "gs/world/MapPoint.h"

class EmptyTile
	{
	private	:
		MapPoint	m_pos ;

		sint32	m_food,
				m_production,
				m_gold
				;

	public :
		EmptyTile(MapPoint &pos, sint32 f, sint32 p, sint32 g) ;
		EmptyTile(EmptyTile *tile) ;
		EmptyTile() ;

		MapPoint GetPos() { return (m_pos) ; }
		sint32 GetFood() { return (m_food) ; }
		sint32 GetProduction() { return (m_production) ; }
		sint32 GetGold() { return (m_gold) ; }

	} ;

#endif
