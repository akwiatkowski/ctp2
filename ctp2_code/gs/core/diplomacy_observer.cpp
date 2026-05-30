//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Engine-side bridge for DipWizard agreement-feedback events
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/core/diplomacy_observer.h"

namespace diplomacy_observer {

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

void NotifyResponse(const Response &resp,
                    sint32 responder, sint32 other_player)
{
    if (s_impl) s_impl->NotifyResponse(resp, responder, other_player);
}

void NotifyThreatRejected(const Response &resp,
                          const Response &sender_response,
                          sint32 responder, sint32 other_player)
{
    if (s_impl)
    {
        s_impl->NotifyThreatRejected(resp, sender_response,
                                     responder, other_player);
    }
}

} // namespace diplomacy_observer
