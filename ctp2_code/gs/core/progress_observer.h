//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : ProgressWindow bridge — engine-side progress fan-out
//
//----------------------------------------------------------------------------
//
// GameFile.cpp drives a long save/load procedure that publishes progress to
// the on-screen ProgressWindow (ui/interface/progresswindow.h).  Calling that
// directly drags ui/ into gs/fileio/ — wrong layering.
//
// This namespace mirrors the three ProgressWindow entry points as free
// functions backed by a registered Impl.  The UI build registers a
// ProgressWindowObserver adapter (in
// ui/interface/progresswindow_observer_adapter.cpp); headless leaves the
// observer unregistered and every call becomes a no-op.
//
// Migration pattern at the call site:
//   before:  ProgressWindow::BeginProgress(g_theProgressWindow, "X", 340);
//   after:   progress_observer::BeginProgress("X", 340);
//
//   before:  g_theProgressWindow->StartCountingTo(50);
//   after:   progress_observer::StartCountingTo(50);
//
//   before:  ProgressWindow::EndProgress(g_theProgressWindow);
//   after:   progress_observer::EndProgress();
//
// The UI adapter owns the lifetime of `g_theProgressWindow`; call sites no
// longer need to know about it.
//
//----------------------------------------------------------------------------

#pragma once

#include "ctp2_inttypes.h"

namespace progress_observer {

class Impl
{
public:
    virtual ~Impl() = default;

    // `ldlBlock` is an LDL window-spec name (e.g. "InitProgressWindow").
    // `maxval` is the upper bound the subsequent StartCountingTo() calls
    // count up to.  Multiple BeginProgress() calls without an intervening
    // EndProgress() nest — the adapter mirrors the ProgressWindow contract.
    virtual void BeginProgress(const char *ldlBlock, sint32 maxval) = 0;

    // `val` is the new progress count.  `message` is optional UI text shown
    // alongside the bar; NULL leaves the previous text in place.
    virtual void StartCountingTo(sint32 val, const char *message) = 0;

    virtual void EndProgress() = 0;
};

// Register/Get follow the render_observer.h pattern.  The engine takes a
// non-owning pointer.  Passing NULL unregisters (used in shutdown / tests).
void Register(Impl *impl);
Impl *Get();   // NULL in headless or pre-registration

// --- Free-function fan-outs ---
// Each null-checks Get() and forwards.  Calls before Register() are no-ops.
void BeginProgress(const char *ldlBlock, sint32 maxval);
void StartCountingTo(sint32 val, const char *message = nullptr);
void EndProgress();

} // namespace progress_observer
