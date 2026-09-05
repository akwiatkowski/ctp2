#!/usr/bin/env python3
"""Install original CTP2 data and generated sprite atlases under CTP2_HOME."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import stat
import subprocess
import sys


def default_ctp2_home() -> Path:
    configured = os.environ.get("CTP2_HOME")
    return Path(configured).expanduser() if configured else Path.home() / ".ctp2"


def find_data_directory(source: Path) -> Path:
    source = source.expanduser().resolve()
    candidates = (source, source / "ctp2_data")
    for candidate in candidates:
        if (candidate / "default" / "gamedata").is_dir() and (
            candidate / "default" / "graphics" / "sprites"
        ).is_dir():
            return candidate
    raise ValueError(
        f"{source} is neither a ctp2_data directory nor a directory containing one"
    )


def install_original_data(source: Path, ctp2_home: Path) -> Path:
    destination = ctp2_home / "original_data"
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(source, destination, dirs_exist_ok=True, copy_function=copy_if_changed)
    return destination


def copy_if_changed(source: str, destination: str) -> str:
    source_path = Path(source)
    destination_path = Path(destination)
    try:
        source_stat = source_path.stat()
        destination_stat = destination_path.stat()
        if (
            source_stat.st_size == destination_stat.st_size
            and source_stat.st_mtime_ns == destination_stat.st_mtime_ns
        ):
            return str(destination_path)
    except FileNotFoundError:
        pass

    # copy2 preserves source permissions. Make a previously copied read-only
    # file writable long enough to update it; copy2 restores the source mode.
    if destination_path.exists():
        destination_path.chmod(destination_path.stat().st_mode | stat.S_IWUSR)
    return shutil.copy2(source_path, destination_path)


def generate_modern_sprites(original_data: Path, ctp2_home: Path) -> None:
    sprites = original_data / "default" / "graphics" / "sprites"
    exporter = Path(__file__).with_name("spr_export.py")
    environment = os.environ.copy()
    environment["CTP2_HOME"] = str(ctp2_home)
    subprocess.run(
        [sys.executable, str(exporter), "--atlas", "--modern-assets", str(sprites)],
        check=True,
        env=environment,
    )


def install(source: Path, ctp2_home: Path, convert_sprites: bool = True) -> None:
    data_directory = find_data_directory(source)
    ctp2_home = ctp2_home.expanduser().resolve()
    original_data = install_original_data(data_directory, ctp2_home)
    (ctp2_home / "assets").mkdir(exist_ok=True)
    (ctp2_home / "saves").mkdir(exist_ok=True)
    if convert_sprites:
        generate_modern_sprites(original_data, ctp2_home)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Copy a legal CTP2 data installation into the canonical user-local layout."
    )
    parser.add_argument(
        "source", type=Path, help="ctp2_data directory, or a directory containing it"
    )
    parser.add_argument(
        "--ctp2-home",
        type=Path,
        default=default_ctp2_home(),
        help="destination root (default: $CTP2_HOME or ~/.ctp2)",
    )
    parser.add_argument(
        "--skip-modern-assets",
        action="store_true",
        help="copy original data without generating modern sprite atlases",
    )
    args = parser.parse_args(argv)

    try:
        install(args.source, args.ctp2_home, not args.skip_modern_assets)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1

    print(f"CTP2 data installed under {args.ctp2_home.expanduser().resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
