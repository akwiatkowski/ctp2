//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Engine-side bridge for DipWizard agreement-feedback events
//
//----------------------------------------------------------------------------
//
// Diplomat::ExecuteResponse(sender, receiver) used to call DipWizard::
// NotifyResponse and DipWizard::NotifyThreatRejected directly to make the
// wizard render the "new agreement" or "threat rejected" UI flourish.
// That coupling — ai/ knowing about ui/interface/dipwizard.h — is the
// wrong direction.
//
// The two callbacks share semantics: each carries one (or two) finalized
// Response objects plus the responder/other player ids.  They sit on a
// different code path from GEV_ResponseReady (which the wizard listens
// to via s_DipWizResponseReady).  The gevent path fires
// SetViewResponse(p1, p2, false) — "ack of response, no celebration";
// the bridge path fires SetViewResponse(senderId, receiverId, true, &resp)
// — "this is the finalized agreement, show the flourish".
//
// `Response` is defined in gs/diplomacy/diplomacy_types.h (relocated from
// ai/diplomacy/diplomattypes.h in the 2026-05-30 H-0 refactor) so this
// header lives cleanly in gs/core/.
//
//----------------------------------------------------------------------------

#pragma once

#include "ctp2_inttypes.h"
#include "gs/diplomacy/diplomacy_types.h"   // Response

namespace diplomacy_observer {

class Impl
{
public:
    virtual ~Impl() = default;

    // Finalized agreement response — wizard should display the parchment
    // with the agreement details.  `responder` is the player whose
    // Diplomat just produced the response; `other_player` is the side
    // observing the negotiation.
    virtual void NotifyResponse(const Response &resp,
                                sint32 responder,
                                sint32 other_player) = 0;

    // Threat-rejected variant — the wizard renders both the original
    // threat (`sender_response`) and the rejection (`resp`).
    virtual void NotifyThreatRejected(const Response &resp,
                                      const Response &sender_response,
                                      sint32 responder,
                                      sint32 other_player) = 0;
};

void Register(Impl *impl);
Impl *Get();   // NULL in headless / pre-registration

// Free-function fan-outs.  Both no-op when no Impl is registered.
void NotifyResponse(const Response &resp,
                    sint32 responder, sint32 other_player);
void NotifyThreatRejected(const Response &resp,
                          const Response &sender_response,
                          sint32 responder, sint32 other_player);

} // namespace diplomacy_observer
