#include <memory>

#include "ctp/c3.h"
#include "ui/aui_common/aui_control.h"
#include "ui/aui_ctp2/c3_updateaction.h"

c3_UpdateAction *c3_UpdateAction::CopyMe()
{
	auto action = std::make_unique<c3_UpdateAction>();

	memcpy(action.get(), this, sizeof(*this));

	return action.release();

}


c3_UpdateAction::~c3_UpdateAction()
= default;


void c3_UpdateAction::Execute(aui_Control *control, uint32 action, uint32 data)
{
}
