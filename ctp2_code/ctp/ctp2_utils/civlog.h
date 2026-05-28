// civlog.h — thin wrapper around spdlog so subsystem code uses a
// project-stable API and we can swap the backend later without touching
// every call site.
//
// Conventions:
//   * Each subsystem creates (and reuses) a named logger via civlog::Get
//     ("headless", "civapp", "gameinit", ...).  Names show up as
//     [%n] in the default pattern, replacing the historical
//     fprintf(stderr, "[TAG] ...") prefixes.
//   * Levels: trace < debug < info < warn < err < critical.  Default
//     filter is info; CIVLOG_LEVEL=debug/trace in the env raises it.
//   * Format strings are fmt::format-style ({}-placeholders), not
//     printf %d/%s.  Type-safe, no narrowing/format-mismatch bugs.
//
// Usage:
//   #include "ctp/ctp2_utils/civlog.h"
//   auto log = civlog::Get("gameinit");
//   log->info("started (archive={})", archive ? "load" : "new");
//   log->debug("World ctor returning, x={}, y={}", w, h);
//
// Migration target: existing fprintf(stderr, "[HEADLESS] ...") calls
// in headless_main / civapp / etc.  Replace incrementally.

#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <memory>
#include <string>

namespace civlog {

// Returns (creating if needed) the named logger.  Thread-safe.  All
// loggers share the stderr color sink + the project's default pattern.
inline std::shared_ptr<spdlog::logger> Get(std::string const & name)
{
    auto existing = spdlog::get(name);
    if (existing) return existing;

    auto created = spdlog::stderr_color_mt(name);
    // [HH:MM:SS.mmm] [name] [level] message — close enough to the
    // historical [HEADLESS]/[CIVAPP] prefixes to be a drop-in upgrade.
    created->set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] %v");
    return created;
}

// One-time global setup.  Call from each binary's main() before the
// first Get().  Reads CIVLOG_LEVEL env var (trace/debug/info/warn/err)
// to override the compile-time default.
inline void Init()
{
    const char * env = std::getenv("CIVLOG_LEVEL");
    if (env)
    {
        spdlog::set_level(spdlog::level::from_str(env));
    }
    else
    {
        spdlog::set_level(spdlog::level::info);
    }
}

}  // namespace civlog
