from __future__ import annotations

import os
from pathlib import Path
import stat
import tempfile
import unittest
from unittest import mock

import install_ctp2_home


class InstallCtp2HomeTest(unittest.TestCase):
    def make_data_tree(self, root: Path) -> Path:
        data = root / "ctp2_data"
        (data / "default" / "gamedata").mkdir(parents=True)
        sprites = data / "default" / "graphics" / "sprites"
        sprites.mkdir(parents=True)
        (data / "default" / "gamedata" / "unit.txt").write_text("one", encoding="utf-8")
        (sprites / "GU001.SPR").write_bytes(b"sprite")
        return data

    def test_default_home_prefers_environment(self) -> None:
        with mock.patch.dict(os.environ, {"CTP2_HOME": "/tmp/custom-ctp2"}):
            self.assertEqual(install_ctp2_home.default_ctp2_home(), Path("/tmp/custom-ctp2"))

    def test_install_is_repeatable_and_updates_original_files(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            data = self.make_data_tree(root / "source")
            destination = root / "home"

            install_ctp2_home.install(data.parent, destination, convert_sprites=False)
            unit = destination / "original_data" / "default" / "gamedata" / "unit.txt"
            self.assertEqual(unit.read_text(encoding="utf-8"), "one")
            self.assertTrue((destination / "assets").is_dir())
            self.assertTrue((destination / "saves").is_dir())

            (data / "default" / "gamedata" / "unit.txt").write_text("two", encoding="utf-8")
            install_ctp2_home.install(data, destination, convert_sprites=False)
            self.assertEqual(unit.read_text(encoding="utf-8"), "two")

    def test_install_is_repeatable_with_read_only_source_files(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            data = self.make_data_tree(root / "source")
            source = data / "default" / "gamedata" / "unit.txt"
            destination = root / "home"
            source.chmod(0o555)

            install_ctp2_home.install(data, destination, convert_sprites=False)
            installed = destination / "original_data" / "default" / "gamedata" / "unit.txt"
            self.assertFalse(installed.stat().st_mode & stat.S_IWUSR)
            install_ctp2_home.install(data, destination, convert_sprites=False)

            source.chmod(0o755)
            source.write_text("updated", encoding="utf-8")
            source.chmod(0o555)
            install_ctp2_home.install(data, destination, convert_sprites=False)
            self.assertEqual(installed.read_text(encoding="utf-8"), "updated")
            self.assertFalse(installed.stat().st_mode & stat.S_IWUSR)

    def test_conversion_uses_canonical_sprite_directory_and_home(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            data = self.make_data_tree(root / "source")
            destination = root / "home"

            with mock.patch.object(install_ctp2_home.subprocess, "run") as run:
                install_ctp2_home.install(data, destination)

            command = run.call_args.args[0]
            self.assertEqual(command[2:4], ["--atlas", "--modern-assets"])
            self.assertEqual(
                Path(command[4]),
                destination.resolve()
                / "original_data"
                / "default"
                / "graphics"
                / "sprites",
            )
            self.assertEqual(run.call_args.kwargs["env"]["CTP2_HOME"], str(destination.resolve()))
            self.assertTrue(run.call_args.kwargs["check"])

    def test_rejects_unrecognised_source(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            with self.assertRaisesRegex(ValueError, "neither a ctp2_data"):
                install_ctp2_home.find_data_directory(Path(temporary))


if __name__ == "__main__":
    unittest.main()
