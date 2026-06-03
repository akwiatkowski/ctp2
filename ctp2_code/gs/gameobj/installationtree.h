#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef _INSTALALLATIONTREE_H_
#define _INSTALALLATIONTREE_H_

#include "gs/utility/QuadTree.h"
#include "gs/gameobj/installation.h"

class InstallationQuadTree : public QuadTree<Installation>
{
public:

	InstallationQuadTree(sint16 width, sint16 height, BOOL yWrap) :
		QuadTree<Installation>(width, height, yWrap)
	{
	}
};

// Lifecycle in gs/utility/gameinit.cpp (new during world setup,
// Clear/delete on world reset); the variable is file-scope `static`
// there.  External readers go through installation_tree_Get(); the
// lifecycle code uses installation_tree_Set() for the new/clear
// transitions.
InstallationQuadTree * installation_tree_Get();
void                   installation_tree_Set(InstallationQuadTree *p);
#endif
