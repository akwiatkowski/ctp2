#!/usr/bin/env python3
"""Check installed startup outside the checkout and the CLI's resumed clock."""

import json
import os
from pathlib import Path
import subprocess
import sys
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


def main():
    binary = Path(sys.argv[1]).resolve()
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
                     "--seed", "42", "--turns", "10")
        assert before["turn"]["round"] == 10
        continuous = run(binary, folder, "continuous", "--new-game", "--players", "4",
                         "--seed", "42", "--turns", "11")
        for mode in ("--load-game", "--json-load"):
            setup = ["--new-game", "--players", "4", "--seed", "42"] if mode == "--json-load" else []
            resumed = run(binary, folder, mode[2:], *setup, mode,
                          str(folder / "before.json"), "--turns", "1")
            assert resumed["turn"]["round"] == 11, resumed["turn"]
            for slot in resumed["players"]:
                if slot["alive"]:
                    assert slot["data"]["current_round"] == 10
            for section in ("players", "ai_state", "world", "rng", "action_log"):
                assert resumed[section] == continuous[section], f"{mode}: {section} diverged"
        print("PASS: installed startup; both CLI load clocks and first AI round match")


if __name__ == "__main__":
    main()
