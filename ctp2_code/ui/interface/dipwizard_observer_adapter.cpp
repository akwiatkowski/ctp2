//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : UI-side adapter for diplomacy_observer — forwards engine
//                NotifyResponse / NotifyThreatRejected events into the
//                DipWizard wizard.
//
//----------------------------------------------------------------------------
//
// Diplomat::ExecuteResponse no longer reaches into ui/ directly; it fires
// engine-side bridge calls instead.  This adapter, registered once at UI
// startup, translates those notifications into the two static methods
// that still live on DipWizard:
//   DipWizard::NotifyResponse(const Response&, sint32, sint32)
//   DipWizard::NotifyThreatRejected(const Response&, const Response&,
//                                   sint32, sint32)
//
// Headless build leaves the adapter unregistered; the engine's
// diplomacy_observer free functions short-circuit to no-op.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/core/diplomacy_observer.h"
#include "ui/interface/dipwizard.h"

namespace {

class DipWizardObserverAdapter : public diplomacy_observer::Impl
{
public:
    void NotifyResponse(const Response &resp,
                        sint32 responder, sint32 other_player) override
    {
        DipWizard::NotifyResponse(resp, responder, other_player);
    }

    void NotifyThreatRejected(const Response &resp,
                              const Response &sender_response,
                              sint32 responder,
                              sint32 other_player) override
    {
        DipWizard::NotifyThreatRejected(resp, sender_response,
                                        responder, other_player);
    }
};

DipWizardObserverAdapter g_dipWizardObserverAdapter;

} // namespace

void RegisterDipWizardObserverAdapter()
{
    diplomacy_observer::Register(&g_dipWizardObserverAdapter);
}

void UnregisterDipWizardObserverAdapter()
{
    diplomacy_observer::Register(nullptr);
}
