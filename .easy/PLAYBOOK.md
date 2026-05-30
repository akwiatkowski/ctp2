# Easy Ticket Orchestration Playbook

**Goal:** Land small, mechanical modernisation tickets in an ISOLATED repo, with
zero touches to the main ctp2 repo.

## Architecture

```
Main repo:     ctp2/           ← NEVER touched by easy workflow
               (master branch, big refactoring happens here)

Easy repo:     ctp2-easy/      ← Workers commit here
               ├── easy-base   (base branch, mirrors ctp2 master)
               ├── easy-1      (worker 1 branch)
               ├── easy-2      (worker 2 branch)
               ├── easy-3      (worker 3 branch)
               └── easy-4      (worker 4 branch)
```

Workers run in `ctp2-easy/`, commit to `easy-N` branches. Review checks build + tests
in `ctp2-easy/`. Cherry-pick to main `ctp2/` is a **separate manual step** done by
Claude when convenient.

## Why isolation matters

- Main `ctp2/` repo stays pristine — no worker accidents, no merge conflicts
- Big refactoring workflow continues undisturbed in its own worktrees
- Easy commits accumulate in `ctp2-easy/` and can be cherry-picked in batches
- If a worker goes off-script, only `ctp2-easy/` is affected

## The 4-step loop

### 1. Pick tickets

Filter `.scouts/tickets/` for worker-ready tickets:
- **H-container** — C arrays → `std::array` / `std::vector`
- **L-smell** — `NULL` → `nullptr`, missing `override`, range-based for loops
- Skip: H-memory (needs ownership audit), H-solid (needs design)

Store picked tickets in `.easy/tasks/easy-NNN.md`.

### 2. Spawn workers

```bash
.easy/bin/spawn.sh 1 .easy/tasks/easy-001.md
.easy/bin/spawn.sh 2 .easy/tasks/easy-002.md
.easy/bin/spawn.sh 3 .easy/tasks/easy-003.md
```

Each worker:
- Checks out `easy-N` branch in `ctp2-easy/`
- Resets the branch to `easy-base` (clean state)
- Runs opencode with the task brief
- Commits to `easy-N`

### 3. Review (auto)

```bash
.easy/bin/review.sh easy-1
```

Automated review in `ctp2-easy/`:
- Build passes (`meson compile -C build ctp2_fast_tests`)
- Fast tests pass (`./build/ctp2_fast_tests`)
- Diff touches ≤3 files (usually 1-2)

If all pass → print OK with cherry-pick instructions. If fail → print REJECT.

### 4. Cherry-pick to main (manual, by Claude)

```bash
cd /Users/olek/projects/llm/games/ctp2
git fetch ../ctp2-easy easy-1
git cherry-pick easy-base..easy-1
```

Or review first:
```bash
cd /Users/olek/projects/llm/games/ctp2-easy
git log --oneline easy-base..easy-1
git show easy-1
```

## Hard rules

1. **Main ctp2/ repo is NEVER touched by easy workflow**
2. **One ticket = one commit = ≤3 files touched**
3. **Fast tests must pass in ctp2-easy/** — no exceptions
4. **Auto-review only** — if tests pass, the commit is valid; cherry-pick later
5. **Skip on failure** — don't debug worker failures; reset branch and move on

## Workflow commands

```bash
make easy-status       # show easy branch states + pending tasks
make easy-spawn        # spawn next batch (manual)
make easy-review-all   # review all finished easy branches
```

## Setting up ctp2-easy (one-time)

```bash
# Clone from main repo
cd /Users/olek/projects/llm/games
git clone ctp2 ctp2-easy

# Setup build
cd ctp2-easy
meson setup build ctp2_code --buildtype=debug

# Create base + worker branches
git checkout -b easy-base master
git branch easy-1 easy-base
git branch easy-2 easy-base
git branch easy-3 easy-base
git branch easy-4 easy-base
```
