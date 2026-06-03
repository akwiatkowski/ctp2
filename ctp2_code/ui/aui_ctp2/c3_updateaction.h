#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __C3_UPDATEACTION_H__
#define __C3_UPDATEACTION_H__

#include "ui/aui_common/aui_action.h"

class c3_UpdateAction;

class c3_UpdateAction : public aui_Action
{
public:
	c3_UpdateAction()
    :   aui_Action  ()
    { ; };

	~c3_UpdateAction() override;

	void	Execute
	(
		aui_Control	*	control,
		uint32			action,
		uint32			data
	) override;

	virtual c3_UpdateAction * CopyMe();
};


#endif
