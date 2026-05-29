//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : ProgressWindow bridge — engine-side progress fan-out
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/core/progress_observer.h"

namespace progress_observer {

namespace {
Impl *s_impl = nullptr;
}

void Register(Impl *impl)
{
    s_impl = impl;
}

Impl *Get()
{
    return s_impl;
}

void BeginProgress(const char *ldlBlock, sint32 maxval)
{
    if (s_impl) s_impl->BeginProgress(ldlBlock, maxval);
}

void StartCountingTo(sint32 val, const char *message)
{
    if (s_impl) s_impl->StartCountingTo(val, message);
}

void EndProgress()
{
    if (s_impl) s_impl->EndProgress();
}

} // namespace progress_observer
