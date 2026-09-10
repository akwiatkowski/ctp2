#!/usr/bin/env python3
"""Read-only inspector for Call To Power 2 ``.SPR`` sprite files.

This is the first slice of the M8 "modern asset pipeline" spike (see
``REFACTORING_PLAN.md``). It parses the original ``.SPR`` container and
reports its structure *without* decoding pixel data and *without* touching
the game engine. Original assets stay canonical; nothing here rewrites them.

Format reference
----------------
Everything below is derived from the engine's own reader,
``ctp2_code/gfx/spritesys/spritefile.cpp`` (``SpriteFile::Open`` and the
``ReadBasic_v13`` / ``ReadBasic_v20`` / ``ReadFacedSpriteDataBasic`` paths).
All integers are little-endian.

File header (``SpriteFile::Open``)::

    uint32  tag          # 0x53505246, on disk the ASCII bytes "FRPS"
    uint32  version      # 0x00010003 (v0), 0x00020000 (v1), 0x00020001 (v2)
    uint32  compression  # ONLY present when version == 0x00020001 (v2)
    uint32  type         # SPRITEFILETYPE (4 == UNIT)

Unit sprite body, version 0 ("v13", ``ReadBasic_v13``)::

    int32   offsets[UNITACTION_MAX]   # 5 entries: MOVE ATTACK IDLE VICTORY WORK
                                      # >0 => that action's sprite lives there
                                      # (-1 / 0 => absent)

Unit sprite body, versions 1 & 2 ("v20", ``ReadBasic_v20``)::

    int32   offsets[ACTION_MAX + 1]   # 17 entries, indexed by GAME_ACTION;
                                      # for units only MOVE(0) and IDLE(2) are set

Per-action sprite header (``ReadSpriteDataGeneralBasic`` dispatch on a leading
``uint16`` sprite type, then ``ReadSpriteDataBasic`` / ``ReadFacedSpriteDataBasic``)::

    uint16  sprite_type              # 0 NORMAL, 1 FACED, 2 FACEDWSHADOW
    uint16  width
    uint16  height
    # NORMAL:  int32 hot_x; int32 hot_y
    # FACED:   POINT hot_points[k_NUM_FACINGS]   (5 * {int32 x, int32 y})
    uint16  first_frame
    uint16  num_frames               # frame count per facing

Frame pixel payloads (compressed, LZW1 or raw) follow the header and are NOT
decoded here — that is a later M8 slice.
"""

from __future__ import annotations

import argparse
import struct
import sys
from dataclasses import dataclass, field

# --- constants mirrored from the engine headers ---------------------------
SPRITEFILE_TAG = 0x53505246  # 'SPRF'; SpriteFile.h:37

# SpriteFile.h:38-40
VERSIONS = {
    0x00010003: "v0 (0x00010003, v13 layout)",
    0x00020000: "v1 (0x00020000, v20 layout)",
    0x00020001: "v2 (0x00020001, v20 layout, compressed)",
}
VERSION_V2 = 0x00020001  # only this version carries a compression field

# SpriteFile.h:63-76
SPRITEFILETYPES = [
    "PLAIN", "FACED", "FACEDWSHADOW", "GROUP", "UNIT",
    "PROJECTILE", "EFFECT", "GOOD", "CITY",
]
SPRITEFILETYPE_UNIT = 4

# Sprite.h:49-53
SPRITETYPES = {0: "NORMAL", 1: "FACED", 2: "FACEDWSHADOW"}

# UnitSpriteGroup.h:20 (only the non-negative, on-disk members, in order)
UNITACTIONS = ["MOVE", "ATTACK", "IDLE", "VICTORY", "WORK"]
UNITACTION_MAX = len(UNITACTIONS)

# Action.h:82  (GAME_ACTION table used by the v20 offset array)
ACTION_MAX = 16

K_NUM_FACINGS = 5   # FacedSprite.h:50
POINT_SIZE = 8      # os/nowin32/windows.h: struct POINT { sint32 x, y; }


@dataclass
class Facing:
    x: int
    y: int


@dataclass
class ActionInfo:
    name: str
    offset: int
    sprite_type: str = "?"
    width: int = 0
    height: int = 0
    first_frame: int = 0
    num_frames: int = 0
    hot_points: list[Facing] = field(default_factory=list)
    note: str = ""


@dataclass
class SprInfo:
    path: str
    size: int
    version: int
    version_name: str
    compression: int | None
    type_id: int
    type_name: str
    actions: list[ActionInfo] = field(default_factory=list)
    note: str = ""


class SprError(Exception):
    """Raised when the file is not a readable .SPR container."""


def _u16(buf: bytes, off: int) -> int:
    return struct.unpack_from("<H", buf, off)[0]


def _u32(buf: bytes, off: int) -> int:
    return struct.unpack_from("<I", buf, off)[0]


def _i32(buf: bytes, off: int) -> int:
    return struct.unpack_from("<i", buf, off)[0]


def _read_action_header(buf: bytes, offset: int, name: str) -> ActionInfo:
    """Parse the sprite header that lives at ``offset`` (no pixel decode)."""
    info = ActionInfo(name=name, offset=offset)
    if offset < 0 or offset + 6 > len(buf):
        info.note = "offset out of range"
        return info

    stype = _u16(buf, offset)
    info.sprite_type = SPRITETYPES.get(stype, f"unknown({stype})")
    pos = offset + 2
    info.width = _u16(buf, pos)
    info.height = _u16(buf, pos + 2)
    pos += 4

    if stype == 0:  # NORMAL: single int32 x / int32 y hot point
        info.hot_points = [Facing(_i32(buf, pos), _i32(buf, pos + 4))]
        pos += 8
    else:  # FACED / FACEDWSHADOW: one POINT per facing
        for _ in range(K_NUM_FACINGS):
            info.hot_points.append(Facing(_i32(buf, pos), _i32(buf, pos + 4)))
            pos += POINT_SIZE

    info.first_frame = _u16(buf, pos)
    info.num_frames = _u16(buf, pos + 2)
    return info


def inspect(path: str) -> SprInfo:
    with open(path, "rb") as f:
        buf = f.read()

    if len(buf) < 12:
        raise SprError(f"file too small ({len(buf)} bytes) to be a .SPR")

    tag = _u32(buf, 0)
    if tag != SPRITEFILE_TAG:
        raise SprError(
            f"bad tag 0x{tag:08X} (expected 0x{SPRITEFILE_TAG:08X} 'FRPS')")

    version = _u32(buf, 4)
    pos = 8
    compression = None
    if version == VERSION_V2:
        compression = _u32(buf, pos)
        pos += 4
    type_id = _u32(buf, pos)
    pos += 4

    info = SprInfo(
        path=path,
        size=len(buf),
        version=version,
        version_name=VERSIONS.get(version, f"unknown(0x{version:08X})"),
        compression=compression,
        type_id=type_id,
        type_name=(SPRITEFILETYPES[type_id]
                   if 0 <= type_id < len(SPRITEFILETYPES)
                   else f"unknown({type_id})"),
    )

    if type_id != SPRITEFILETYPE_UNIT:
        info.note = "detailed action decode implemented for UNIT sprites only"
        return info

    # Offset table: layout depends on the version family.
    #   v13: int32[UNITACTION_MAX] — every entry is an action offset.
    #   v20: int32[ACTION_MAX + 1] — indices 0..ACTION_MAX-1 are action
    #        offsets (GAME_ACTION indexed); the final entry offsets[ACTION_MAX]
    #        is the "offset to special data" (shield/fire points), not a sprite.
    if version in (0x00020000, VERSION_V2):  # v20
        action_count = ACTION_MAX
        names = [f"ACTION_{i}" for i in range(action_count)]
        # V20 keeps unit actions at their UNITACTION indices too.
        names[:UNITACTION_MAX] = UNITACTIONS
        special_off = _i32(buf, pos + ACTION_MAX * 4)
    else:  # v0 / v13
        action_count = UNITACTION_MAX
        names = list(UNITACTIONS)
        special_off = None

    for i in range(action_count):
        off = _i32(buf, pos + i * 4)
        if off > 0:
            info.actions.append(_read_action_header(buf, off, names[i]))

    if special_off is not None and special_off > 0:
        info.note = f"special-data (shield/fire points) offset @0x{special_off:06X}"

    return info


def format_report(info: SprInfo) -> str:
    lines = []
    lines.append(f"File        : {info.path}")
    lines.append(f"Size        : {info.size} bytes")
    lines.append(f"Version     : {info.version_name}")
    if info.compression is not None:
        lines.append(f"Compression : {info.compression}")
    lines.append(f"Type        : {info.type_name} ({info.type_id})")
    if info.note:
        lines.append(f"Note        : {info.note}")
    if info.actions:
        lines.append(f"Actions     : {len(info.actions)} present")
        for a in info.actions:
            lines.append(
                f"  - {a.name:<8} @0x{a.offset:06X}  {a.sprite_type:<12}"
                f" {a.width}x{a.height}  frames={a.num_frames}"
                f" first={a.first_frame}")
            if a.note:
                lines.append(f"      note: {a.note}")
            elif a.hot_points:
                hp = " ".join(f"({p.x},{p.y})" for p in a.hot_points)
                lines.append(f"      hot points: {hp}")
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Inspect a CTP2 .SPR sprite file (read-only).")
    parser.add_argument("path", help="path to a .SPR file (e.g. GU04.SPR)")
    args = parser.parse_args(argv)

    try:
        info = inspect(args.path)
    except (OSError, SprError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    print(format_report(info))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
