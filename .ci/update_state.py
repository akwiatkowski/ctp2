#!/usr/bin/env python3
# Atomically merge a tier result into .ci/state.json.
#
# Usage:
#   update_state.py --tier {a|b|c|d} --result <path-to-tier-result.json>
#       [--head-sha <sha>] [--head-subject <text>] [--duration-s <float>]
#       [--status {green|red|running}] [--running-tier {A|B|C}]
#       [--clear-running] [--failure-context-path <path>]
#
# Modes:
#   1. Start a run:    --tier X --status running [--running-tier X]
#   2. Finish a run:   --tier X --result FILE --duration-s N --status {green|red}
#                      --clear-running
#   3. Pure HEAD update (called when daemon notices new HEAD before scheduling):
#                      --head-sha SHA --head-subject TEXT
#
# Writes .ci/STATUS_RED iff any of tier_a/tier_b is red after the merge.
# Writes failure JSON to .ci/failures/<sha>-tier-X.json when red.

import argparse
import datetime
import json
import os
import sys
import tempfile
from pathlib import Path

CI_ROOT = Path(__file__).resolve().parent
STATE_PATH = CI_ROOT / "state.json"
STATUS_RED = CI_ROOT / "STATUS_RED"
FAILURES_DIR = CI_ROOT / "failures"

SCHEMA_VERSION = 1


def utcnow_iso():
    return datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def load_state():
    if STATE_PATH.exists():
        try:
            return json.loads(STATE_PATH.read_text())
        except json.JSONDecodeError:
            pass
    return {
        "schema_version": SCHEMA_VERSION,
        "updated_at": utcnow_iso(),
        "head": {"sha": "", "subject": "", "branch": ""},
        "overall_status": "unknown",
        "running": None,
        "tier_a": None,
        "tier_b": None,
        "tier_c": None,
        "tier_d": None,
    }


def atomic_write_json(path: Path, payload: dict):
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(
        "w", dir=str(path.parent), prefix=path.name + ".", delete=False
    ) as tmp:
        json.dump(payload, tmp, indent=2, sort_keys=False)
        tmp.write("\n")
        tmp_path = tmp.name
    os.replace(tmp_path, path)


def recompute_overall_status(state):
    statuses = []
    for tier in ("tier_a", "tier_b", "tier_c", "tier_d"):
        t = state.get(tier)
        if t is None:
            continue
        statuses.append(t.get("status", "unknown"))

    if "red" in statuses:
        return "red"
    if state.get("running"):
        return "running"
    if all(s == "green" for s in statuses) and statuses:
        return "green"
    return "unknown"


def update_status_red_sentinel(overall):
    if overall == "red":
        STATUS_RED.touch()
    elif STATUS_RED.exists():
        STATUS_RED.unlink()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--tier", choices=["a", "b", "c", "d"])
    ap.add_argument("--result", help="path to JSON file produced by parse_doctest_xml.py")
    ap.add_argument("--head-sha")
    ap.add_argument("--head-subject")
    ap.add_argument("--head-branch")
    ap.add_argument("--duration-s", type=float)
    ap.add_argument("--status", choices=["green", "red", "running"])
    ap.add_argument("--running-tier", choices=["A", "B", "C"])
    ap.add_argument("--clear-running", action="store_true")
    ap.add_argument("--summary", help="free-text summary (tier C)")
    ap.add_argument("--context-path", help="path to raw log for failure context")
    args = ap.parse_args()

    state = load_state()

    if args.head_sha is not None:
        state["head"]["sha"] = args.head_sha
    if args.head_subject is not None:
        state["head"]["subject"] = args.head_subject
    if args.head_branch is not None:
        state["head"]["branch"] = args.head_branch

    if args.running_tier:
        state["running"] = {
            "tier": args.running_tier,
            "started_at": utcnow_iso(),
        }
    elif args.clear_running:
        state["running"] = None

    if args.tier and args.status:
        tier_key = "tier_" + args.tier
        tier_state = state.get(tier_key) or {}
        tier_state["last_run_at"] = utcnow_iso()
        tier_state["head_sha"] = args.head_sha or state["head"].get("sha", "")
        tier_state["status"] = args.status

        if args.duration_s is not None:
            tier_state["duration_s"] = round(args.duration_s, 2)

        if args.result and Path(args.result).exists():
            result = json.loads(Path(args.result).read_text())
            tier_state["tests"] = result.get("tests", {})
            tier_state["failures"] = result.get("failures", [])
        else:
            tier_state.setdefault("tests", {})
            tier_state.setdefault("failures", [])

        if args.summary:
            tier_state["summary"] = args.summary

        # Persist failure context outside state.json so the agent can read
        # actionable failure summaries without loading the whole log.
        if args.status == "red" and tier_state.get("failures"):
            sha = tier_state["head_sha"] or "working"
            FAILURES_DIR.mkdir(exist_ok=True)
            fail_doc = {
                "sha": sha,
                "tier": args.tier.upper(),
                "tests_failed": tier_state["tests"].get("failed", len(tier_state["failures"])),
                "failures": tier_state["failures"],
                "context_path": args.context_path or "",
            }
            atomic_write_json(
                FAILURES_DIR / f"{sha}-tier-{args.tier}.json", fail_doc
            )

        state[tier_key] = tier_state

    state["overall_status"] = recompute_overall_status(state)
    state["updated_at"] = utcnow_iso()

    atomic_write_json(STATE_PATH, state)
    update_status_red_sentinel(state["overall_status"])

    print(state["overall_status"])


if __name__ == "__main__":
    main()
