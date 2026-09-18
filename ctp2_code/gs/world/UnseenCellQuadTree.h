#pragma once
#ifndef _UNSEEN_CELL_QUAD_TREE_H_
#define _UNSEEN_CELL_QUAD_TREE_H_

#include "gs/utility/QuadTree.h"
#include "gs/world/UnseenCell.h"

class UnseenCellQuadTree : public QuadTree<UnseenCellCarton>
{

public:
	uint32 GetFlags(UnseenCellCarton cell) override { return 0xffffffff; }

	UnseenCellQuadTree(sint16 width, sint16 height, BOOL yWrap) :
		QuadTree<UnseenCellCarton>(width, height, yWrap)
	{
	}
	// Intentional no-op override: unseen-cell positions must not be y-wrapped
	// like QuadTree<T>::Convert does.
	void Convert(MapPoint &pos) const override {}
	void Clear() override;

	~UnseenCellQuadTree()
	override = default;
};

inline void UnseenCellQuadTree::Clear()
{
	DynamicArray<UnseenCellCarton> array;
	BuildList(array);
	sint32 i;
	sint32 n = array.Num();
	for(i = 0; i < n; i++) {
		delete array[i].m_unseenCell;
	}
	QuadTree<UnseenCellCarton>::Clear();
}

#endif
