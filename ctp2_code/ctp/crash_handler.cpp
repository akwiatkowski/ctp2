//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : In-process crash reporter (see crash_handler.h).
//
//----------------------------------------------------------------------------
//
// Everything the signal handler touches is async-signal-safe: write(2),
// backtrace(3)/backtrace_symbols_fd(3), and a preallocated event ring.
// No malloc, no stdio, no locks. The crash log fd is opened at Install
// time; the handler only ever write()s to it.
//
//----------------------------------------------------------------------------

#include "ctp/crash_handler.h"

#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <execinfo.h>
#include <fcntl.h>
#include <unistd.h>

namespace {

// ---- recent-event ring ----------------------------------------------------

constexpr int  kRingSize = 24;
constexpr int  kSlotSize = 48;

char                 s_ring[kRingSize][kSlotSize];
std::atomic<unsigned> s_ringHead{0};

// ---- handler state ---------------------------------------------------------

int           s_logFd     = -1;
volatile sig_atomic_t s_inHandler = 0;

constexpr int kSignals[] = { SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT, SIGTRAP };

void WriteBoth(const char * s, size_t n)
{
    // Best effort — a failed write in a crash handler has no recourse.
    (void)!write(STDERR_FILENO, s, n);
    if (s_logFd >= 0)
        (void)!write(s_logFd, s, n);
}

void WriteBoth(const char * s) { WriteBoth(s, strlen(s)); }

// Minimal async-safe number formatting (snprintf is not on the safe list).
void WriteNum(long v)
{
    char buf[24];
    char * p = buf + sizeof(buf);
    bool   neg = v < 0;
    unsigned long u = neg ? (unsigned long)(-v) : (unsigned long)v;
    do { *--p = (char)('0' + u % 10); u /= 10; } while (u);
    if (neg) *--p = '-';
    WriteBoth(p, (size_t)(buf + sizeof(buf) - p));
}

void Handler(int sig, siginfo_t * info, void *)
{
    // A crash inside the handler (or on a second thread) must not recurse.
    if (s_inHandler++) {
        signal(sig, SIG_DFL);
        raise(sig);
        return;
    }

    WriteBoth("\n==================== CTP2 CRASH ====================\nsignal ");
    WriteNum(sig);
    WriteBoth(" (");
    WriteBoth(sig == SIGSEGV ? "SIGSEGV" :
              sig == SIGBUS  ? "SIGBUS"  :
              sig == SIGILL  ? "SIGILL"  :
              sig == SIGFPE  ? "SIGFPE"  :
              sig == SIGABRT ? "SIGABRT" :
              sig == SIGTRAP ? "SIGTRAP" : "?");
    WriteBoth(") fault address 0x");
    {
        // hex, async-safe
        unsigned long a = (unsigned long)(info ? info->si_addr : nullptr);
        char buf[20];
        char * p = buf + sizeof(buf);
        do { unsigned d = (unsigned)(a & 0xF); *--p = (char)(d < 10 ? '0' + d : 'a' + d - 10); a >>= 4; } while (a);
        WriteBoth(p, (size_t)(buf + sizeof(buf) - p));
    }

    WriteBoth("\n\n--- backtrace (use `atos -o <binary> <addr>` for file:line) ---\n");
    void * frames[64];
    int    n = backtrace(frames, 64);
    backtrace_symbols_fd(frames, n, STDERR_FILENO);
    if (s_logFd >= 0)
        backtrace_symbols_fd(frames, n, s_logFd);

    WriteBoth("\n--- last dispatched game events (newest first) ---\n");
    unsigned head = s_ringHead.load(std::memory_order_relaxed);
    for (int i = 0; i < kRingSize; ++i) {
        const char * slot = s_ring[(head - 1 - (unsigned)i) % kRingSize];
        if (!slot[0]) break;
        WriteBoth("  ");
        WriteBoth(slot);
        WriteBoth("\n");
    }
    WriteBoth("====================================================\n");

    // Restore default disposition and re-raise: exit status, core dumps
    // and parent-process detection behave exactly as without the handler.
    signal(sig, SIG_DFL);
    raise(sig);
}

}  // namespace

namespace crash_handler {

void Install(const char * crash_log_path)
{
    static bool installed = false;
    if (installed) return;
    installed = true;

    memset(s_ring, 0, sizeof(s_ring));

    if (crash_log_path) {
        s_logFd = open(crash_log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (s_logFd >= 0) {
            char   stamp[64];
            time_t now = time(nullptr);
            struct tm tmv;
            localtime_r(&now, &tmv);
            size_t n = strftime(stamp, sizeof(stamp),
                                "\n--- crash handler armed %Y-%m-%d %H:%M:%S ---\n", &tmv);
            (void)!write(s_logFd, stamp, n);
        }
    }

    // Dedicated stack: a stack-overflow SIGSEGV cannot report from the
    // exhausted stack it just blew.
    static char   altstack[SIGSTKSZ * 2];
    stack_t ss;
    ss.ss_sp    = altstack;
    ss.ss_size  = sizeof(altstack);
    ss.ss_flags = 0;
    sigaltstack(&ss, nullptr);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = Handler;
    sa.sa_flags     = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&sa.sa_mask);
    for (int sig : kSignals)
        sigaction(sig, &sa, nullptr);
}

void NoteEvent(const char * name)
{
    if (!name) return;
    unsigned slot = s_ringHead.fetch_add(1, std::memory_order_relaxed) % kRingSize;
    strncpy(s_ring[slot], name, kSlotSize - 1);
    s_ring[slot][kSlotSize - 1] = '\0';
}

}  // namespace crash_handler
