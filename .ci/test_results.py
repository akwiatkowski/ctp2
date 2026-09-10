"""CI must report failed commands even when the unit-test XML is green."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

from parse_doctest_xml import runner_result


PASSING = '<doctest><TestSuite><TestCase name="ok"><OverallResultsAsserts test_case_success="true"/></TestCase></TestSuite></doctest>'


class ResultsTest(unittest.TestCase):
    def test_fatal_messages_are_preserved_without_summary(self):
        for failure in ['<Message type="FATAL ERROR"><Text>ratchet exceeded</Text></Message>',
                        '<Exception>unexpected exception</Exception>',
                        '<FatalErrorOccurred>segmentation fault</FatalErrorOccurred>']:
            with self.subTest(failure=failure):
                result = runner_result(f'<doctest><TestCase name="broken">{failure}</TestCase></doctest>', 0)
                self.assertEqual(result["tests"]["failed"], 1)
                self.assertEqual(result["failures"][0]["test"], "broken")
                self.assertTrue(result["failures"][0]["message"])

    def test_reports_and_runner_exit(self):
        for xml, code, failed in [
            (PASSING, 0, False), (PASSING, 1, True),
            ("", 0, True), ("<doctest>", 0, True),
            ("<doctest/>", 0, True),
            (PASSING.replace('success="true"', 'success="false"'), 0, True),
        ]:
            with self.subTest(xml=xml, code=code):
                self.assertEqual(bool(runner_result(xml, code)["tests"]["failed"]), failed)

    def test_failed_build_finishes_state_for_every_tier(self):
        # Execute the real shell wrappers with failing build tools, in an
        # isolated checkout. This catches `exit` bypassing result publication.
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            ci = root / ".ci"
            (ci / "tiers").mkdir(parents=True)
            (root / "build-asan").mkdir()
            (root / "build-ubsan").mkdir()
            (root / "ctp2_code").mkdir()
            for name in ["update_state.py", "parse_doctest_xml.py"]:
                shutil.copy(Path(__file__).parent / name, ci / name)
            mise = root / "mise"
            mise.write_text('#!/bin/sh\nshift 2\nif [ "$1" = python3 ]; then exec "$@"; fi\nexit 7\n')
            mise.chmod(0o755)
            env = dict(os.environ, PATH=f"{root}:{os.environ['PATH']}")
            for tier in "abcd":
                with self.subTest(tier=tier):
                    script = ci / "tiers" / f"tier-{tier}.sh"
                    shutil.copy(Path(__file__).parent / "tiers" / script.name, script)
                    result = subprocess.run(["bash", str(script)], env=env,
                                            capture_output=True, text=True, timeout=15)
                    self.assertNotEqual(result.returncode, 0, result.stderr)
                    state = json.loads((ci / "state.json").read_text())
                    self.assertIsNone(state["running"])
                    self.assertEqual(state[f"tier_{tier}"]["status"], "red")


if __name__ == "__main__":
    unittest.main()
