# `.ci/` — local CI daemon

Background daemon that runs the project's test tiers automatically. The
consumer is a coding agent (Claude Code, opencode/Kimi workers) doing
fast edit-test-commit loops, not a human reading a dashboard. Designed
so:

- An agent never has to invoke `meson test` itself — the daemon already
  ran it in the background while the agent was thinking.
- All state lives in a single JSON file the agent can parse in one read.
- Failures are pre-digested into `{test, file, line, message}` records —
  no log scraping required.
- Orchestration scripts honour `STATUS_RED`, so workers can't pile bad
  commits on a red master.

## Lifecycle

```
make ci-start       # nohup-launch the daemon (PID in .ci/daemon.pid)
make ci-stop        # SIGTERM the daemon
make ci-status      # dump state.json
make ci-watch       # tail .ci/log/daemon.log
make ci-failures    # cat the most recent failure record
make ci-reset       # nuke state + logs (use rarely)
make ci-tier-a      # run tier A once, in the foreground
```

Run `make ci-start` once at the start of a coding session. The daemon
keeps running until `make ci-stop` or `kill $(cat .ci/daemon.pid)`.
`launchd` integration is queued for after the v1 has been used in
anger; for now the daemon is plain `nohup`.

## What the daemon does

Every 3s (configurable via `POLL_INTERVAL`):

1. **HEAD moved?** → schedule **Tier B** (`ninja ctp2 ctp2_headless ctp2_unit_tests`
   + `./build/ctp2_unit_tests`). Records the new HEAD in `.ci/last_head`.
2. **`ninja -n ctp2_fast_tests` shows pending work** AND last Tier A
   was ≥ `TIER_A_DEBOUNCE` (default 5s) ago → schedule **Tier A**
   (`ninja ctp2_fast_tests` + `./build/ctp2_fast_tests`).
3. Run one queued tier at a time (no overlap; the build cache is shared).

Tier A reflects the **working tree** (uncommitted edits included). Tier B
reflects the most recently committed master HEAD. Tier C
(`autoplay_run.sh` sanitized, marathon metric-sanity test) is not yet
scheduled by the daemon — it's intended for nightly cron / launchd.

## Files

```
.ci/
├── daemon.sh                  # main loop
├── daemon.pid                 # running daemon's PID (auto-removed on exit)
├── last_head                  # last HEAD SHA seen by the daemon
├── parse_doctest_xml.py       # doctest XML → JSON failure records
├── update_state.py            # atomic merge into state.json
├── state.json                 # the agent contract — single source of truth
├── STATUS_RED                 # sentinel: present iff any committed tier is red
├── tiers/
│   ├── tier-a.sh              # fast tier driver
│   ├── tier-b.sh              # commit tier driver
│   └── tier-c.sh              # marathon tier (placeholder — see ROADMAP)
├── failures/<sha>-tier-X.json # one per red run; older runs retained
├── log/
│   ├── daemon.log             # daemon's stdout/stderr
│   ├── tier-{a,b}-<ts>.log    # raw test stdout (last 50 / 30 kept)
│   ├── tier-{a,b}-<ts>.xml    # doctest XML
│   └── tier-{a,b}-<ts>.json   # parsed failures (input to update_state.py)
└── metrics/
    └── tier-b-times.csv       # duration per commit — "is CI getting slow?"
```

## `state.json` schema (the agent contract)

```jsonc
{
  "schema_version": 1,
  "updated_at": "2026-05-28T20:12:52Z",
  "head": {
    "sha": "4cac7834",
    "subject": "refactor(gfx/spritesys): delete m_savePos dead state",
    "branch": "master"
  },
  "overall_status": "green",         // green | red | running | unknown
  "running": null,                    // or { "tier": "A|B|C", "started_at": "..." }
  "tier_a": {
    "head_sha": "4cac7834",
    "last_run_at": "2026-05-28T20:12:52Z",
    "status": "green",
    "duration_s": 24.0,
    "tests": { "passed": 201, "failed": 0, "skipped": 0 },
    "failures": []
  },
  "tier_b": { /* same shape */ },
  "tier_c": { /* same shape, plus optional "summary" */ }
}
```

`overall_status` is derived: `red` if any of `tier_a` / `tier_b` is
red; `running` if a tier is in flight and nothing is red; `green` if
both tiers are green; `unknown` if no tier has ever run.

Failure records (one file per red run, `.ci/failures/<sha>-tier-X.json`):

```jsonc
{
  "sha": "deadbeef",
  "tier": "B",
  "tests_failed": 2,
  "failures": [
    {
      "test": "Save-load round-trip: 25t save + 25t resume = 50t continuous",
      "file": "ctp2_code/test/cpp/test_save_load.cpp",
      "line": 430,
      "type": "CHECK",
      "original": "cont_m.players[i].score == loaded_m.players[i].score",
      "expanded": "1430 == 1360",
      "message": "CHECK(...) failed: 1430 == 1360",
      "info": [
        "player 2 (Tomoe)"
      ]
    }
  ],
  "context_path": ".ci/log/tier-b-20260528-213201.log"
}
```

`context_path` points at the raw test stdout if the agent needs full
context. `info` is the captured `INFO()` chain from doctest — usually
enough to locate the failing input without reading the log.

## Recipes for an agent

```bash
# After making an edit, before committing — is my edit safe?
jq -r '.tier_a.status' .ci/state.json
# → green | red | running

# After committing — is master still green?
[ -f .ci/STATUS_RED ] && echo RED || echo GREEN

# What broke?
jq '.failures' .ci/failures/$(ls -t .ci/failures/ | head -1)

# Drill into the raw log for the most recent red run:
jq -r '.context_path' .ci/failures/$(ls -t .ci/failures/ | head -1) | xargs cat
```

The daemon never lies about its own state: if the daemon process is
not running, `make ci-status` still shows the last known state.
Check `make ci-start` if you suspect it's stopped.

## When the daemon should be off

- Heavy refactor batches under `.orchestration/` already run their own
  `review.sh` on the eval worktree — the daemon doesn't double-check.
- During a known-red recovery batch where every commit is intentionally
  trying to repair `STATUS_RED`. Set `CTP2_CI_OVERRIDE=1` on the
  orchestration scripts, OR just `make ci-stop` until the rebuild
  series lands.

## Override

Orchestration scripts (`.orchestration/bin/merge.sh`, `.easy/bin/review.sh`)
refuse to operate when `STATUS_RED` is present. To bypass for
intentional cleanup commits:

```bash
CTP2_CI_OVERRIDE=1 .orchestration/bin/merge.sh oc/refactor-1
```

## Roadmap (not yet implemented)

- **Tier C — marathon sanity** (`autoplay_run.sh` sanitized + 500–1000
  turn metric-sanity assertions). Scheduled nightly via cron/launchd.
  Reports `STATUS_YELLOW` (warning, doesn't block orchestration) so a
  sanitizer hit during marathon doesn't gate easy-ticket merges.
- **`launchd` integration** — `~/Library/LaunchAgents/com.olek.ctp2-ci.plist`
  so the daemon survives reboots without `make ci-start`.
- **`ccache`** — shared cache across all worktrees (`ctp2-refactor-*`,
  `ctp2-easy`). Would cut Tier B build time substantially when workers
  jump between branches.
- **Diff-aware routing** — skip Tier A on edits to `*.md`, `*.txt`,
  `.scouts/`, `.orchestration/`. Currently every edit that touches
  ninja's dep graph triggers a build.
