#!/usr/bin/env python3
# Read a doctest XML report from stdin (or a path), emit a JSON object with
# { "tests": {passed, failed, skipped}, "failures": [...] } on stdout.
#
# Failure record shape:
#   { "test": <case name>, "subcase": <subcase path or null>,
#     "file": <path>, "line": <int>,
#     "type": "CHECK" | "REQUIRE" | "FATAL ERROR",
#     "original": <source expression>,
#     "expanded": <values inserted>,
#     "message": <human-friendly one-liner>,
#     "info": [<INFO/CAPTURE lines>] }
#
# Designed for the CI daemon — keep output deterministic and small.

import argparse
import json
import sys
import xml.etree.ElementTree as ET


def text(node, default=""):
    return node.text if node is not None and node.text is not None else default


def parse_expressions(case_node, case_name, info_stack, failures):
    """Walk a TestCase / SubCase recursively, collecting failed Expressions."""
    for child in case_node:
        tag = child.tag
        if tag == "Info":
            info_stack.append(text(child).strip())
        elif tag == "SubCase":
            sub_name = child.get("name", "")
            new_info = list(info_stack)
            parse_expressions(
                child,
                case_name + (" / " + sub_name if sub_name else ""),
                new_info,
                failures,
            )
        elif tag == "Expression":
            if child.get("success", "true") == "false":
                original = text(child.find("Original")).strip()
                expanded = text(child.find("Expanded")).strip()
                failures.append({
                    "test": case_name,
                    "file": child.get("filename", ""),
                    "line": int(child.get("line", "0") or 0),
                    "type": child.get("type", "CHECK"),
                    "original": original,
                    "expanded": expanded,
                    "message": f"{child.get('type', 'CHECK')}({original}) failed: {expanded}",
                    "info": list(info_stack),
                })
        elif tag in ("Exception", "FatalErrorOccurred", "Message"):
            if tag == "Message" and child.get("type") not in ("FATAL ERROR", "CHECK", "REQUIRE"):
                continue
            message = " ".join(child.itertext()).strip()
            failures.append({
                "test": case_name, "file": child.get("filename", ""),
                "line": int(child.get("line", "0") or 0),
                "type": child.get("type", tag), "original": "", "expanded": "",
                "message": message, "info": list(info_stack),
            })


def parse(xml_text):
    # doctest emits a leading "[doctest] doctest version ..." banner before the
    # XML when invoked without --no-version; strip everything before the first
    # "<?xml" so we tolerate both.
    idx = xml_text.find("<?xml")
    if idx > 0:
        xml_text = xml_text[idx:]
    root = ET.fromstring(xml_text)

    passed = 0
    failed = 0
    skipped = 0
    failures = []

    # Iterate test cases. Doctest groups them under TestSuite tags but a flat
    # walk via root.iter() is simpler and tolerant of structure changes.
    for case in root.iter("TestCase"):
        if case.get("skipped") == "true":
            skipped += 1
            continue
        name = case.get("name", "")
        case_failures_before = len(failures)
        parse_expressions(case, name, [], failures)
        results = case.find("OverallResultsAsserts")
        if results is not None and results.get("test_case_success") == "false":
            failed += 1
        elif len(failures) > case_failures_before:
            failed += 1
        else:
            passed += 1

    return {
        "tests": {"passed": passed, "failed": failed, "skipped": skipped},
        "failures": failures,
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("path", nargs="?", default="-")
    ap.add_argument("--exit-code", type=int, default=0)
    args = ap.parse_args()
    try:
        if args.path == "-":
            xml_text = sys.stdin.read()
        else:
            with open(args.path, encoding="utf-8", errors="replace") as f:
                xml_text = f.read()
        result = runner_result(xml_text, args.exit_code)
    except OSError as error:
        result = runner_result("", args.exit_code, str(error))
    json.dump(result, sys.stdout, indent=2)
    sys.stdout.write("\n")
    return 1 if result["tests"]["failed"] else 0


def runner_result(xml_text, exit_code, error=""):
    """A passing unit report cannot erase a later scenario/sanitizer failure."""
    try:
        result = parse(xml_text)
        if not result["tests"]["passed"] and not result["tests"]["failed"]:
            error = "report contains no executed tests"
    except (ET.ParseError, ValueError) as exc:
        result = {"tests": {"passed": 0, "failed": 0, "skipped": 0}, "failures": []}
        error = error or f"invalid or incomplete doctest report: {exc}"
    if exit_code or error:
        result["tests"]["failed"] += 1
        result["failures"].append({
            "test": "CI build/run", "file": "", "line": 0, "type": "RUNNER",
            "original": "complete CI tier", "expanded": f"exit code {exit_code}",
            "message": error or f"CI tier exited {exit_code}; see context_path",
            "info": [],
        })
    return result


if __name__ == "__main__":
    sys.exit(main())
