# Local CI

Run project commands through `mise exec --`. CI uses the installed game data
under `CTP2_HOME` (default `~/.ctp2`); licensed assets stay outside the repository.

| Command | Coverage |
|---|---|
| `make ci-tier-a` | Fast C++ build and tests |
| `make ci-tier-b` | Player UI, headless and test builds; unit tests; eight short headless scenarios; fast/installation/CI/harness checks; save replays through rounds 11, 40 and 75 |
| `make ci-nightly` | Sanitized headless/unit build; full C++ unit and integration suite; eight short headless scenarios; 500-round campaign plus deep save/load |
| `.ci/tiers/tier-d.sh` | Standalone UBSan build and full C++ suite |
| `make test-render` | Eleven pixel/renderer checks; requires a desktop session |

Each tier returns nonzero on failure and records its result in `.ci/state.json`.
Logs, doctest XML and parsed failures live in `.ci/log/`; `context_path` identifies
the complete log. A build failure, missing/malformed report or failed scenario
cannot be overwritten by a passing unit report. Any red tier sets `STATUS_RED`.
Run `mise exec -- python3 .ci/test_results.py` to verify failure reporting,
including failed builds through all four real shell wrappers.

Tier C uses UBSan on macOS: native ASan currently hangs before `main` on this
macOS 26 host, including a trivial executable. On Linux it uses ASan + UBSan.
Sanitizers halt on the first error. ASan leak detection remains disabled;
a passing run does not establish leak freedom. Existing build directories are
reconfigured to the requested sanitizer, single-player mode (`anet=false`) and
`debugoptimized` before building. Debug symbols remain enabled; optimization
keeps long campaigns practical even when every turn serializes an autosave.

`make ci-start` starts the optional working-tree/commit daemon; `ci-stop`,
`ci-status`, `ci-watch` and `ci-failures` inspect/control it. The daemon runs
Tier A after pending builds and Tier B after HEAD changes. The 300-turn Meson
soak is tagged `marathon`; Tier C runs the stronger 500-turn version separately.
Sanitizer tiers are disabled on commits by default; schedule `mise exec -- make ci-nightly` separately
with cron or launchd. `ENABLE_TIER_C=1` / `ENABLE_TIER_D=1` explicitly restore
post-commit runs, gated on that Tier B invocation succeeding.

Run one CI invocation at a time: tiers share build directories and state. No
nightly service or hosted workflow is installed automatically. A hosted macOS /
Linux build matrix remains separate work and needs an asset provisioning policy.

For supervised background work, use the single approvable command prefix
`mise exec -- python3 tools/job.py`:

```sh
mise exec -- python3 tools/job.py start scenarios mise exec -- meson test -C build --suite scenario --no-suite marathon
mise exec -- python3 tools/job.py status scenarios
mise exec -- python3 tools/job.py logs scenarios
mise exec -- python3 tools/job.py stop scenarios
mise exec -- python3 tools/job.py self-test
```

Jobs run from the repository root, with metadata and combined output in
`build/jobs/`. Stop verifies process identities, sends TERM to the job and its
descendants (including separate test sessions), then kills survivors after five
seconds. Only recorded jobs can be stopped. Reusing an inactive name replaces its
log. Status reports whether the process is running; check the log for test results.
