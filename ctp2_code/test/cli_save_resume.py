#!/usr/bin/env python3
"""Check installed startup outside the checkout and the CLI's resumed clock."""

import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile


def run(binary, folder, name, *args):
    save = folder / f"{name}.json"
    log = folder / f"{name}.log"
    with log.open("w") as output:
        result = subprocess.run(
            [str(binary), *args, "--json-save", str(save)], cwd=folder,
            stdout=output, stderr=subprocess.STDOUT, timeout=120,
        )
    assert result.returncode == 0, log.read_text()[-6000:]
    return json.loads(save.read_text())


def difference(a, b, path=""):
    if a == b:
        return None
    if isinstance(a, dict) and isinstance(b, dict) and a.keys() == b.keys():
        for key in a:
            if a[key] != b[key]:
                return difference(a[key], b[key], f"{path}/{key}")
    if isinstance(a, list) and isinstance(b, list) and len(a) == len(b):
        for i, (left, right) in enumerate(zip(a, b)):
            if left != right:
                return difference(left, right, f"{path}/{i}")
    return f"{path}: {str(a)[:160]} != {str(b)[:160]}"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--checkpoint", type=int, default=10)
    parser.add_argument("--resume-turns", type=int, default=1)
    args = parser.parse_args()
    binary = args.binary.resolve()
    checkpoint = args.checkpoint
    final_round = checkpoint + args.resume_turns
    # Reuse installed data, but keep every write inside the disposable home.
    source_home = Path(os.environ.get("CTP2_HOME", Path.home() / ".ctp2"))
    data = (source_home / "original_data").resolve()
    if not data.is_dir():
        data = Path(__file__).resolve().parents[2] / "ctp2_data"
    with tempfile.TemporaryDirectory(prefix="ctp2-cli-resume-") as temporary:
        folder = Path(temporary)
        home = folder / "home"
        home.mkdir()
        (home / "original_data").symlink_to(data, target_is_directory=True)
        os.environ["CTP2_HOME"] = str(home)
        before = run(binary, folder, "before", "--new-game", "--players", "4",
                     "--seed", str(args.seed), "--turns", str(checkpoint))
        assert before["turn"]["round"] == checkpoint
        continuous = run(binary, folder, "continuous", "--new-game", "--players", "4",
                         "--seed", str(args.seed), "--turns", str(final_round))
        for mode in ("--load-game", "--json-load"):
            setup = ["--new-game", "--players", "4", "--seed", str(args.seed)] if mode == "--json-load" else []
            loaded = run(binary, folder, mode[2:] + "-unchanged", *setup, mode,
                         str(folder / "before.json"), "--turns", "0")
            assert loaded["ai_state"] == before["ai_state"], difference(
                loaded["ai_state"], before["ai_state"], f"{mode}: immediate AI restoration")
            resumed = run(binary, folder, mode[2:], *setup, mode,
                          str(folder / "before.json"), "--turns", str(args.resume_turns))
            assert resumed["turn"]["round"] == final_round, resumed["turn"]
            for slot in resumed["players"]:
                if slot["alive"]:
                    assert slot["data"]["current_round"] == final_round - 1
            for section in ("players", "ai_state", "world", "rng", "action_log"):
                assert resumed[section] == continuous[section], f"{mode}: {difference(resumed[section], continuous[section], section)}"
        print(f"PASS: installed startup; both CLI load modes match rounds {checkpoint}–{final_round}")


if __name__ == "__main__":
    main()
