"""A scenario must fail if the game crashes after its final assertion."""
import subprocess
import unittest
from unittest.mock import Mock

from ctp2_client import Ctp2Client, Ctp2Error


class ShutdownTest(unittest.TestCase):
    def client(self, returncode):
        client = Ctp2Client.__new__(Ctp2Client)
        client._rpc = Mock()
        client.sock = Mock()
        client.proc = Mock(returncode=returncode)
        client._log = Mock()
        return client

    def test_exit_codes(self):
        self.client(0).close()
        for code in [1, -6, -11]:
            with self.subTest(code=code):
                client = self.client(code)
                with self.assertRaises(Ctp2Error):
                    client.close()
                client._log.close.assert_called_once()

    def test_timeout_fails_and_reaps_child(self):
        client = self.client(-9)
        client.proc.wait.side_effect = [subprocess.TimeoutExpired("game", 10), None]
        with self.assertRaisesRegex(Ctp2Error, "timed out"):
            client.close()
        client.proc.kill.assert_called_once()

    def test_cleanup_does_not_replace_original_failure(self):
        self.client(-11).__exit__(ValueError, ValueError("original"), None)


class SeedTest(unittest.TestCase):
    def test_ui_start_forwards_seed_and_players(self):
        client = Ctp2Client.__new__(Ctp2Client)
        client.mode, client.seed, client.players = "ui", 123, 4
        client._rpc = Mock()
        client.command("start_game")
        client._rpc.assert_called_once_with("start_game 123 4")
        client.seed = 0
        with self.assertRaises(ValueError):
            client.command("start_game")


if __name__ == "__main__":
    unittest.main()
