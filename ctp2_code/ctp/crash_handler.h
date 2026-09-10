//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : In-process crash reporter.
//
//----------------------------------------------------------------------------
//
// On SIGSEGV/SIGBUS/SIGILL/SIGFPE/SIGABRT/SIGTRAP, writes a symbolised backtrace
// and the most recently dispatched game events to stderr and to a crash
// log file, then re-raises the signal (exit code / core dump unchanged).
//
// Motivation: every crash used to cost an lldb session + often an ASAN
// rebuild just to learn WHERE it happened. The handler makes the game log
// self-reporting — `make repro` and the CI tiers read the trace directly.
//
//----------------------------------------------------------------------------

#if defined(HAVE_PRAGMA_ONCE)
#pragma once
#endif

#ifndef CRASH_HANDLER_H__
#define CRASH_HANDLER_H__

namespace crash_handler {

// Install the signal handlers. Idempotent. crash_log_path may be NULL
// (stderr only); the file is opened eagerly so the handler never allocates.
void Install(const char * crash_log_path);

// Record a dispatched game event in the lock-free ring the crash report
// dumps ("what was the engine doing"). Cheap: one strncpy per event.
void NoteEvent(const char * name);

}  // namespace crash_handler

#endif
