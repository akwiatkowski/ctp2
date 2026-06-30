# CTP2 Refactoring Plan

Purpose: give any assistant or developer a quick, shared answer to: "how much more refactoring work remains?"

This file is deliberately plain Markdown so it is easy for different LLMs (OpenAI, Claude Opus, local models) to read and update. Keep it short. Update the table after each committed batch.

## Current Definition Of Done

The refactoring effort is "done enough" when CTP2 has:

- Fast pre-commit tests that stay green.
- A working sanitizer smoke tier on macOS/Linux.
- Modernization ratchets preventing regression in unsafe patterns.
- Warning noise reduced enough that new warnings are visible.
- Timelapse/play-session tooling stable enough for repeated use.
- A documented list of remaining legacy-risk areas instead of open-ended cleanup.

This is not a full rewrite. A fully modernized CTP2 engine is open-ended and likely much larger than this project needs.

## Work Remaining Estimate

| Track | Current State | Done When | Estimate |
| --- | --- | --- | --- |
| Commit hygiene | Recent batches committed; worktree should usually be clean | One logical commit per batch; no uncommitted drift | Ongoing |
| Fast test loop | `make test` runs ratchets + fast/unit tests | Stays under practical pre-commit time and is trusted | 0-1 sessions |
| Sanitizer smoke | `make ubsan-smoke` works; ASan hangs before `main` on current macOS setup | UBSan smoke documented and ASan either fixed or explicitly marked platform-blocked | 1-3 sessions |
| Warning cleanup | Several low-risk batches done; many legacy warnings remain | High-signal warnings fixed: include case, precedence, scalar NULL, dead locals | 4-8 sessions |
| Modernization ratchets | Ratchets exist for raw new/delete, unsafe strings, C allocation, casts | Ratchets kept green and lowered after each intentional cleanup batch | 4-8 sessions |
| Timelapse tooling | Fogged hero timelapse works; captions have names metadata | Captions are useful, short runs are reproducible, docs explain common commands | 1-3 sessions |
| UI/frame polish | Not started in this sequence | Pick 1-2 visible polish wins, not a full UI rewrite | 2-4 sessions |
| Deeper architecture cleanup | Large legacy systems still coupled | Only targeted cleanup with tests; no broad rewrite | Open-ended |

## Short Answer Formula

When asked "how much more work remains?", answer from this scale:

- Minimum useful cleanup: **4-6 sessions**.
- Solid modernization pass: **10-16 sessions**.
- Mostly quiet warnings + sanitizer/ratchet discipline: **16-24 sessions**.
- Fully refactored engine: **open-ended / not a bounded goal**.

Recommended default answer: **about 10-16 focused sessions** to reach a solid, practical refactoring milestone.

## Next Best Batches

Do these in order unless Olek changes priorities:

1. Finish sanitizer story: document ASan macOS blocker or find a working ASan config.
2. Run one more small warning cleanup batch from `make ubsan-smoke` output.
3. Normalize `Player.h` include casing in a mechanical batch, then verify.
4. Fix remaining low-risk precedence warnings in AI code.
5. Lower one modernization ratchet category intentionally, then update baseline.
6. Add a short timelapse usage note once captions are good enough.

## Assistant Protocol

For any LLM continuing this work:

- Read this file first, then `git log --oneline -10`, then `git status --short`.
- Prefer one small batch per commit.
- Verify with `mise exec -- make test` and `git diff --check`.
- Use `mise exec -- make ubsan-smoke` after touching headless/game-loop code.
- Do not treat "fully refactored" as the goal unless Olek explicitly redefines scope.
- After each committed batch, update the estimate table only if the estimate materially changed.

## Last Known Verification Commands

```sh
mise exec -- make test
mise exec -- make ubsan-smoke
```

## Known Blockers / Caveats

- `make sanitized-smoke` exists, but ASan currently hangs before `main` on this macOS/Apple clang setup.
- `make ubsan-smoke` is the working sanitizer smoke path.
- Windows is best-effort and should not drive refactoring decisions unless Olek asks.
- Avoid `MBCHAR *` UI string-literal cleanup unless intentionally doing a broader UI const-correctness batch.
