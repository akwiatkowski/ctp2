//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : UI-side adapter that forwards progress_observer calls to
//                the existing ProgressWindow widget.
//
//----------------------------------------------------------------------------
//
// Engine code (gs/fileio/GameFile.cpp etc.) calls into
// `progress_observer::*` free functions.  This adapter, registered once at
// UI startup, forwards those calls into the singleton ProgressWindow that
// lives in civapp.  Headless builds skip registration and the calls
// short-circuit to no-ops at the bridge layer.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/core/progress_observer.h"
#include "ui/interface/progresswindow.h"

extern ProgressWindow *g_theProgressWindow;   // owned in ctp/civapp.cpp

namespace {

class ProgressWindowObserver : public progress_observer::Impl
{
public:
    void BeginProgress(const char *ldlBlock, sint32 maxval) override
    {
        // ProgressWindow::BeginProgress takes a non-const MBCHAR* purely for
        // historical reasons (the LDL lookup it performs is read-only).
        ProgressWindow::BeginProgress(
            g_theProgressWindow,
            const_cast<MBCHAR *>(ldlBlock),
            maxval);
    }

    void StartCountingTo(sint32 val, const char *message) override
    {
        if (g_theProgressWindow)
        {
            g_theProgressWindow->StartCountingTo(val, message);
        }
    }

    void EndProgress() override
    {
        if (g_theProgressWindow)
        {
            ProgressWindow::EndProgress(g_theProgressWindow);
        }
    }
};

ProgressWindowObserver g_progressWindowObserver;

} // namespace

void RegisterProgressWindowObserver()
{
    progress_observer::Register(&g_progressWindowObserver);
}

void UnregisterProgressWindowObserver()
{
    progress_observer::Register(nullptr);
}
