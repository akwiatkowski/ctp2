//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : SPLASH_STRING progress macro without the UI dependency
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/core/splash_progress.h"

namespace splash_progress {

static ShowFn s_show = nullptr;

void Register(ShowFn fn)
{
	s_show = fn;
}

void Show(const char *msg)
{
	if (s_show) {
		s_show(msg);
	}
}

} // namespace splash_progress
