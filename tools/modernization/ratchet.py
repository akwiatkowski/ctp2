#!/usr/bin/env python3
"""Prevent legacy C/C++ modernization counters from increasing.

The codebase is too large for a one-shot rewrite, so modernization needs a
ratchet: cleanup can reduce these counters, but new changes should not add more
raw ownership, unsafe C string APIs, C allocation, or type-erased casts.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
BASELINE_PATH = ROOT / "tools" / "modernization" / "ratchet_baseline.json"
SOURCE_ROOT = ROOT / "ctp2_code"
SOURCE_GLOBS = ["*.c", "*.cc", "*.cpp", "*.cxx", "*.h", "*.hpp"]
# Vendored third-party libraries (anet, freetype, zlib, tiff, miles, GameWatch)
# are not first-party modernization targets — upstream owns them and any edit is
# clobbered on upgrade — so they are excluded from every ratchet counter, the
# same rationale that already excludes generated build-* directories.
EXCLUDE_GLOBS = ["build/**", "**/.!*", "**/build/**", "**/build-*/**", "**/libs/**"]

CHECKS = {
    "raw_new": r"\bnew\b",
    "raw_delete": r"\bdelete\b",
    "c_allocation": r"\b(malloc|calloc|realloc|free)\s*\(",
    "unsafe_string_api": r"\b(strcpy|strcat|sprintf)\s*\(",
    "type_erased_casting": r"void\s*\*|reinterpret_cast",
}


def count_matches(pattern: str) -> int:
    command = ["rg", "--count-matches", "--no-heading"]
    for glob in SOURCE_GLOBS:
        command.extend(["--glob", glob])
    for glob in EXCLUDE_GLOBS:
        command.extend(["--glob", f"!{glob}"])
    command.extend([pattern, str(SOURCE_ROOT)])

    result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
    if result.returncode == 1:
        return 0
    if result.returncode != 0:
        sys.stderr.write(result.stderr)
        raise RuntimeError(f"rg failed for pattern: {pattern}")

    total = 0
    for line in result.stdout.splitlines():
        try:
            total += int(line.rsplit(":", 1)[1])
        except (IndexError, ValueError) as exc:
            raise RuntimeError(f"unexpected rg output: {line}") from exc
    return total


def collect_counts() -> dict[str, int]:
    return {name: count_matches(pattern) for name, pattern in CHECKS.items()}


def load_baseline() -> dict[str, int]:
    with BASELINE_PATH.open(encoding="utf-8") as baseline_file:
        data = json.load(baseline_file)
    return {name: int(value) for name, value in data["counts"].items()}


def write_baseline(counts: dict[str, int]) -> None:
    data = {
        "description": "Maximum allowed legacy C/C++ pattern counts. Run `make modernization-ratchet-update` after intentional cleanup only.",
        "source_root": "ctp2_code",
        "counts": counts,
    }
    BASELINE_PATH.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def print_table(current: dict[str, int], baseline: dict[str, int] | None) -> None:
    print("Modernization ratchet:")
    for name in sorted(current):
        if baseline is None:
            print(f"  {name:20} {current[name]:6}")
        else:
            delta = current[name] - baseline[name]
            print(f"  {name:20} {current[name]:6} / {baseline[name]:6} ({delta:+})")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--update-baseline", action="store_true", help="write current counts as the new baseline")
    args = parser.parse_args()

    current = collect_counts()
    if args.update_baseline:
        write_baseline(current)
        print_table(current, None)
        print(f"Updated {BASELINE_PATH.relative_to(ROOT)}")
        return 0

    baseline = load_baseline()
    print_table(current, baseline)

    failures = [name for name, count in current.items() if count > baseline[name]]
    if failures:
        print("\nLegacy pattern counts increased:", file=sys.stderr)
        for name in failures:
            print(f"  {name}: {current[name]} > {baseline[name]}", file=sys.stderr)
        print("Reduce the new sites or intentionally refresh the baseline after cleanup.", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
