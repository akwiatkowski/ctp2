#!/usr/bin/env python3
"""Prevent legacy C/C++ modernization counters from increasing.

The codebase is too large for a one-shot rewrite, so modernization needs a
ratchet: cleanup can reduce these counters, but new changes should not add more
raw ownership, unsafe C string APIs, C allocation, or type-erased casts.
"""

from __future__ import annotations

import argparse
import json
import re
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
    # P9 container modernization: the pre-STL custom containers. PointerList
    # is often OWNING (converting one is also a P6 ownership decision);
    # DynamicArray/SimpleDynamicArray are the memcpy-growth value arrays.
    "pointerlist_uses": r"\bPointerList\s*<",
    "dynarray_uses": r"\b(DynamicArray|SimpleDynamicArray)\s*<",
    # RAII-conversion guard: a standalone `std::unique_ptr<T>(x);` statement
    # with a plain-identifier or identifier[index] argument parses as a
    # DECLARATION (shadow variable / zero-bound VLA), never a deleting
    # temporary — the real delete silently never happens. Braces `{x}` are
    # unambiguous. Call/field arguments (`f()`, `a.b`) cannot parse as
    # declarators, so they are left unflagged to avoid false positives.
    "unique_ptr_parens_delete": r"(?m)^\s*std::unique_ptr\s*<[^<>]*(?:<[^<>]*>)?[^<>]*>\s*\(\s*[A-Za-z_]\w*(?:\[[^\]]*\])?\s*\)\s*;",
}


# Comments and string/char literals are not code, and counting them made the
# ratchet actively hostile to the cleanup it exists to encourage: documenting
# why a buffer needs an array release, or naming `new Pixel16[]` in a comment
# that explains an ownership fix, RAISED the counter and failed the check. The
# words also appear in ordinary prose ("the new frame list", "delete the old
# entry"), so the noise is not even confined to memory discussions.
#
# Matched left-to-right in one pass so that a "//" inside a string literal is
# seen as string content rather than starting a comment, and a quote inside a
# comment does not open a literal. Each match becomes a space, which keeps
# tokens on either side from being glued into a new false match.
NON_CODE_PATTERN = re.compile(
    r'"(?:\\.|[^"\\\n])*"'      # string literal
    r"|'(?:\\.|[^'\\\n])*'"     # character literal
    r"|//[^\n]*"                # line comment
    r"|/\*.*?\*/",              # block comment
    re.DOTALL,
)


def iter_source_files() -> list[Path]:
    """The first-party source files, selected by rg exactly as before.

    File selection stays with rg deliberately. It honours .gitignore, so a
    plain rglob picks up generated and ignored sources that were never in the
    counted set — enough to swing three counters upward and make a pure
    measurement change look like a regression.
    """
    command = ["rg", "--files"]
    for glob in SOURCE_GLOBS:
        command.extend(["--glob", glob])
    for glob in EXCLUDE_GLOBS:
        command.extend(["--glob", f"!{glob}"])
    command.append(str(SOURCE_ROOT))

    result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
    if result.returncode not in (0, 1):
        sys.stderr.write(result.stderr)
        raise RuntimeError("rg failed to list source files")
    return [Path(line) for line in result.stdout.splitlines() if line]


def code_only(path: Path) -> str:
    text = path.read_text(encoding="utf-8", errors="ignore")
    return NON_CODE_PATTERN.sub(" ", text)


def collect_counts() -> dict[str, int]:
    compiled = {name: re.compile(pattern) for name, pattern in CHECKS.items()}
    totals = {name: 0 for name in CHECKS}
    for path in iter_source_files():
        source = code_only(path)
        for name, pattern in compiled.items():
            totals[name] += len(pattern.findall(source))
    return totals


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
