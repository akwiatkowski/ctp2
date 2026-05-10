#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __UI_TEST_H__
#define __UI_TEST_H__

#include "ui/aui_common/aui_action.h"


class Button1Action : public aui_Action
{
public:
	Button1Action() {}
	~Button1Action() {}

	virtual void Execute(aui_Control *control, uint32 action, uint32 data);

protected:
};

class Button2Action : public aui_Action
{
public:
	Button2Action() {}
	~Button2Action() {}

	virtual void Execute(aui_Control *control, uint32 action, uint32 data);

protected:
};

#endif
