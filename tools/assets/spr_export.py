#!/usr/bin/env python3
"""Decode CTP2 ``.SPR`` sprite frames to debug PNGs / atlases (read-only).

Second slice of the M8 "modern asset pipeline" spike (see
``REFACTORING_PLAN.md``). Given a ``.SPR`` this decodes the run-length
encoded 16-bit frames into RGBA and writes one PNG per action / facing /
frame (or a packed atlas). UNIT sprites (incl. city ``GC*``), GOOD and EFFECT
containers are all supported. It never modifies the original ``.SPR`` —
original assets stay canonical; the output is a rebuildable debug artifact.

Format reference
----------------
Reverse-engineered from the engine's own code:

* ``gfx/spritesys/spritefile.cpp`` — ``ReadFacedSpriteDataBasic`` gives the
  per-facing size tables and the frame byte layout.
* ``gfx/spritesys/spritelow.cpp`` — ``Sprite::DrawLow565`` is the authoritative
  per-row RLE decoder used at blit time.
* ``gfx/spritesys/spriteutils.cpp`` — ``spriteutils_ConvertPixelFormat`` shows
  the stored pixels are **RGB565** (it converts 565->555 for 15-bit displays).

Frame byte layout (per FACED action), all little-endian::

    uint16  sprite_type
    uint16  width
    uint16  height
    POINT   hot_points[5]            # 5 * {int32 x, int32 y}
    uint16  first_frame
    uint16  num_frames               # frames per facing (nf)
    # size tables, per facing j in 0..4:
    uint32  ssizes[j][0..nf-1]       # normal (full size) frame byte sizes
    uint32  msizes[j][0..nf-1]       # mini (half size) frame byte sizes
    # frame payloads, per facing j in 0..4:
    #   nf normal frames  (each ssizes[j][i] bytes; v2 prefixes a uint32
    #                      actual_size and LZW1-compresses the payload)
    #   nf mini frames    (each msizes[j][i] bytes, always raw)

Each normal frame payload is itself an array of ``Pixel16``::

    Pixel16 frame[0]                 # skipped
    Pixel16 table[height]            # per-row offset into dataStart,
                                     # 0xFFFF (k_EMPTY_TABLE_ENTRY) = empty row
    Pixel16 dataStart[...]           # RLE runs, read forward per row

Per-row RLE runs: ``tag = *rowData++`` (first tag masked to 0x0FFF); the row
ends when ``tag & 0xF000`` is set. Opcode ``(tag & 0x0F00) >> 8``, run length
``tag & 0x00FF``:

* ``0x0A`` chromakey  -> advance N transparent pixels
* ``0x0C`` copy       -> copy N Pixel16 from the stream
* ``0x0E`` shadow     -> N pixels that only darken the background (exported as
                         transparent, since a standalone frame has no backdrop)
* ``0x0F`` feathered  -> one edge-blend Pixel16 (exported opaque)
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import struct
import sys
import tempfile
import zlib

# Reuse the container/header parser from the inspector.
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import spr_inspect as spr  # noqa: E402

# LZW1 constants (SpriteFile.h:57-59)
LZW1_FLAG_BYTES = 4
LZW1_FLAG_COMPRESS = 0
LZW1_FLAG_COPY = 1

# RLE opcodes (spriteutils.h:12-22)
CHROMAKEY_RUN_ID = 0x0A
COPY_RUN_ID = 0x0C
SHADOW_RUN_ID = 0x0E
FEATHERED_RUN_ID = 0x0F
EMPTY_TABLE_ENTRY = 0xFFFF

# SpriteFile.h SPRITEFILETYPE enum (mirrored in spr_inspect.SPRITEFILETYPES).
SPRITEFILETYPE_EFFECT = 6
SPRITEFILETYPE_GOOD = 7

# Per-group action counts for the non-UNIT container walk.
#   GoodSpriteGroup.h:  GOODACTION_IDLE=0, GOODACTION_MAX=1  -> 1 action, IDLE.
#   EffectSpriteGroup.h: EFFECTACTION_PLAY=0, _FLASH=1, _MAX=2 -> PLAY, FLASH.
GOODACTION_MAX = 1
EFFECT_ACTION_NAMES = ("PLAY", "FLASH")

TRANSPARENT = None  # marker for an unset pixel
MAX_INPUT_BYTES = 256 * 1024 * 1024
MAX_MANIFEST_BYTES = 4 * 1024 * 1024
MAX_ATLAS_DIMENSION = 16384
MAX_ATLAS_PIXELS = 64 * 1024 * 1024


class SprExportError(Exception):
    """Raised when a frame payload cannot be decoded safely."""


def decompress_lzw1(payload: bytes, actual_size: int) -> bytes:
    """Decode the engine's simple LZW1 stream from ``SpriteFile``.

    The on-disk payload starts with a 4-byte mode/padding header. Mode 1 is a
    raw copy fallback; mode 0 is a control-bit stream where set bits read a
    12-bit backward offset + 4-bit length token, and clear bits read one
    literal byte. Control bits are consumed least-significant bit first, exactly
    like ``SpriteFile::DeCompressData_LZW1``.
    """
    if actual_size < 0:
        raise SprExportError(f"negative LZW1 actual size: {actual_size}")
    if len(payload) < LZW1_FLAG_BYTES:
        raise SprExportError("LZW1 payload shorter than flag header")

    mode = payload[0]
    src = LZW1_FLAG_BYTES
    if mode == LZW1_FLAG_COPY:
        out = payload[src:]
        if len(out) != actual_size:
            raise SprExportError(
                f"LZW1 copy size mismatch: got {len(out)}, expected {actual_size}")
        return out
    if mode != LZW1_FLAG_COMPRESS:
        raise SprExportError(f"unknown LZW1 mode byte: {mode}")

    out = bytearray()
    control = 0
    control_bits = 0
    while src < len(payload) and len(out) < actual_size:
        if control_bits == 0:
            if src + 2 > len(payload):
                raise SprExportError("truncated LZW1 control word")
            control = payload[src] | (payload[src + 1] << 8)
            src += 2
            control_bits = 16

        if control & 1:
            if src + 2 > len(payload):
                raise SprExportError("truncated LZW1 back-reference")
            first = payload[src]
            second = payload[src + 1]
            src += 2
            offset = ((first & 0xF0) << 4) + second
            length = 1 + (first & 0x0F)
            if offset == 0 or offset > len(out):
                raise SprExportError(
                    f"invalid LZW1 back-reference offset {offset} at output {len(out)}")
            for _ in range(length):
                if len(out) >= actual_size:
                    break
                out.append(out[-offset])
        else:
            if src >= len(payload):
                raise SprExportError("truncated LZW1 literal")
            out.append(payload[src])
            src += 1

        control >>= 1
        control_bits -= 1

    if len(out) != actual_size:
        raise SprExportError(
            f"LZW1 output size mismatch: got {len(out)}, expected {actual_size}")
    return bytes(out)


def rgb565_to_rgba(p: int) -> tuple[int, int, int, int]:
    """Expand a stored RGB565 Pixel16 to 8-bit RGBA (fully opaque)."""
    r5 = (p >> 11) & 0x1F
    g6 = (p >> 5) & 0x3F
    b5 = p & 0x1F
    # 5/6-bit -> 8-bit by bit-replication (keeps white at 255, black at 0)
    r8 = (r5 << 3) | (r5 >> 2)
    g8 = (g6 << 2) | (g6 >> 4)
    b8 = (b5 << 3) | (b5 >> 2)
    return r8, g8, b8, 255


def decode_frame(frame: bytes, width: int, height: int) -> list[list]:
    """RLE-decode one normal frame payload into a height x width grid.

    Cells hold a Pixel16 int (opaque) or ``TRANSPARENT``.
    """
    if (width <= 0 or height <= 0 or width > MAX_ATLAS_DIMENSION
            or height > MAX_ATLAS_DIMENSION
            or width * height > MAX_ATLAS_PIXELS):
        raise SprExportError(f"invalid frame dimensions {width}x{height}")
    if len(frame) < 2:
        return [[TRANSPARENT] * width for _ in range(height)]
    u16 = struct.unpack(f"<{len(frame) // 2}H", frame[: (len(frame) // 2) * 2])
    n = len(u16)
    data_start = 1 + height            # index of dataStart in u16 units
    out = [[TRANSPARENT] * width for _ in range(height)]

    for j in range(height):
        if 1 + j >= n:
            break
        entry = u16[1 + j]
        if entry == EMPTY_TABLE_ENTRY:
            continue
        idx = data_start + entry
        if idx >= n:
            continue
        x = 0
        tag = u16[idx] & 0x0FFF
        idx += 1
        # Guard against corrupt rows (mirrors the engine's runCount guard).
        max_runs = width * 2 + 10
        runs = 0
        while (tag & 0xF000) == 0:
            runs += 1
            if runs > max_runs or idx >= n:
                break
            opcode = (tag & 0x0F00) >> 8
            length = tag & 0x00FF
            if opcode == CHROMAKEY_RUN_ID:
                x += length
            elif opcode == COPY_RUN_ID:
                for _ in range(length):
                    if idx >= n or x >= width:
                        break
                    if 0 <= x < width:
                        out[j][x] = u16[idx]
                    idx += 1
                    x += 1
            elif opcode == SHADOW_RUN_ID:
                x += length  # background darken -> transparent in a lone frame
            elif opcode == FEATHERED_RUN_ID:
                if idx < n and x < width:
                    out[j][x] = u16[idx]
                    idx += 1
                    x += 1
            else:
                break
            if idx >= n:
                break
            tag = u16[idx]
            idx += 1
    return out


def verify_frame(frame: bytes, width: int, height: int) -> tuple[int, int]:
    """Parity check: no non-empty row's runs may overflow the scanline.

    ``Sprite::DrawLow565`` writes each row's runs into a scanline of stride
    ``width`` with no intra-row clipping, so a correct decode must never
    advance past ``width`` (trailing transparent pixels are implicit and left
    unencoded, so rows legitimately end at x <= width). A row whose runs push
    x > width would corrupt the next scanline in the engine, so that is the
    real failure signal. Returns (rows_checked, rows_overflowed).
    """
    if len(frame) < 2:
        return 0, 0
    u16 = struct.unpack(f"<{len(frame) // 2}H", frame[: (len(frame) // 2) * 2])
    n = len(u16)
    data_start = 1 + height
    checked = mismatched = 0
    for j in range(height):
        if 1 + j >= n:
            break
        entry = u16[1 + j]
        if entry == EMPTY_TABLE_ENTRY:
            continue
        idx = data_start + entry
        if idx >= n:
            mismatched += 1
            checked += 1
            continue
        x = 0
        tag = u16[idx] & 0x0FFF
        idx += 1
        runs = 0
        max_runs = width * 2 + 10
        while (tag & 0xF000) == 0:
            runs += 1
            if runs > max_runs or idx >= n:
                break
            opcode = (tag & 0x0F00) >> 8
            length = tag & 0x00FF
            if opcode == CHROMAKEY_RUN_ID:
                x += length
            elif opcode == COPY_RUN_ID:
                x += length
                idx += length
            elif opcode == SHADOW_RUN_ID:
                x += length
            elif opcode == FEATHERED_RUN_ID:
                x += 1
                idx += 1
            else:
                break
            if idx >= n:
                break
            tag = u16[idx]
            idx += 1
        checked += 1
        if x > width:
            mismatched += 1
    return checked, mismatched


def grid_to_rgba_bytes(grid: list[list], width: int, height: int) -> bytes:
    """Flatten a decoded grid to raw RGBA8 (transparent = 0,0,0,0)."""
    row_bytes = bytearray()
    for j in range(height):
        for x in range(width):
            cell = grid[j][x]
            if cell is TRANSPARENT:
                row_bytes += b"\x00\x00\x00\x00"
            else:
                row_bytes += bytes(rgb565_to_rgba(cell))
    return bytes(row_bytes)


def write_png(path: str, width: int, height: int, rgba: bytes) -> None:
    """Write an 8-bit RGBA PNG with no third-party dependencies.

    Engine-decode contract (the modern-first C++ loader relies on this exact
    shape so it can decode with the already-linked zlib and no un-filtering):
    colour type 6 (RGBA), bit depth 8, no interlace, filter type 0 on every
    scanline, and a single IDAT chunk. Decoding is then: inflate the IDAT,
    then drop one leading zero byte per row to recover raw RGBA. ``--self-test``
    pins this structure.
    """
    def chunk(kind: bytes, data: bytes) -> bytes:
        return (struct.pack(">I", len(data)) + kind + data
                + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF))

    # Prepend the mandatory per-scanline filter byte (0 = none).
    raw = bytearray()
    stride = width * 4
    for j in range(height):
        raw.append(0)
        raw += rgba[j * stride:(j + 1) * stride]

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)  # 6 = RGBA
    png = (b"\x89PNG\r\n\x1a\n"
           + chunk(b"IHDR", ihdr)
           + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
           + chunk(b"IEND", b""))
    with open(path, "wb") as f:
        f.write(png)


def pack_atlas(frames: list[dict], max_width: int = 2048) -> tuple[int, int, bytes]:
    """Pack decoded RGBA frames into a simple row atlas.

    This is intentionally boring: rows are filled left-to-right and wrapped at
    ``max_width``. It is not space-optimal, but it is deterministic,
    dependency-free, and enough to prove the manifest/atlas contract before the
    engine loader exists.
    """
    x = y = row_height = atlas_width = 0
    for frame in frames:
        width = frame["width"]
        height = frame["height"]
        if width <= 0 or height <= 0:
            raise SprExportError(f"invalid atlas frame size {width}x{height}")
        if x and x + width > max_width:
            y += row_height
            x = 0
            row_height = 0
        frame["rect"] = {"x": x, "y": y, "w": width, "h": height}
        x += width
        row_height = max(row_height, height)
        atlas_width = max(atlas_width, x)

    atlas_height = y + row_height
    if atlas_width == 0 or atlas_height == 0:
        return 1, 1, b"\x00\x00\x00\x00"
    if (atlas_width > MAX_ATLAS_DIMENSION or atlas_height > MAX_ATLAS_DIMENSION
            or atlas_width * atlas_height > MAX_ATLAS_PIXELS):
        raise SprExportError(
            f"atlas dimensions exceed supported limit: {atlas_width}x{atlas_height}")

    atlas = bytearray(atlas_width * atlas_height * 4)
    for frame in frames:
        rect = frame["rect"]
        src = frame["rgba"]
        stride = frame["width"] * 4
        for row in range(frame["height"]):
            src_start = row * stride
            dst_start = ((rect["y"] + row) * atlas_width + rect["x"]) * 4
            atlas[dst_start:dst_start + stride] = src[src_start:src_start + stride]
    return atlas_width, atlas_height, bytes(atlas)


def source_fingerprint(path: str) -> str:
    """Return the stable content fingerprint for one source asset file."""
    digest = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()[:16]


def iter_spr_files(path: str) -> list[str]:
    """Return all sprite files below ``path`` in deterministic order."""
    if os.path.isfile(path):
        return [path]

    matches: list[str] = []
    for root, _, names in os.walk(path):
        for name in names:
            if name.lower().endswith(".spr"):
                matches.append(os.path.join(root, name))
    return sorted(matches, key=lambda item: os.path.relpath(item, path).lower())


def source_set_fingerprint(path: str) -> str:
    """Return a stable fingerprint for one file or a walked sprite set."""
    if os.path.isfile(path):
        return source_fingerprint(path)

    digest = hashlib.sha256()
    for filename in iter_spr_files(path):
        rel = os.path.relpath(filename, path).replace(os.sep, "/").lower()
        digest.update(rel.encode("utf-8"))
        digest.update(b"\0")
        with open(filename, "rb") as f:
            for chunk in iter(lambda: f.read(1024 * 1024), b""):
                digest.update(chunk)
        digest.update(b"\0")
    return digest.hexdigest()[:16]


def modern_assets_root() -> str:
    """Root of the user-local modern-asset cache."""
    ctp2_home = os.environ.get("CTP2_HOME")
    if not ctp2_home:
        ctp2_home = os.path.expanduser(os.path.join("~", ".ctp2"))
    return os.path.join(ctp2_home, "assets")


def modern_assets_dir(path: str) -> str:
    """User-local output directory for modern assets derived from ``path``."""
    return os.path.join(modern_assets_root(), source_set_fingerprint(path))


def update_current_pointer(fingerprint_dir: str) -> None:
    """Point ``<root>/current`` at the freshly generated fingerprint dir.

    The engine has no sha256 to reproduce the content fingerprint, so it looks
    up manifests under a stable ``current`` symlink instead. Regenerating the
    cache re-points it (this is the cache-invalidation mechanism)."""
    link = os.path.join(modern_assets_root(), "current")
    target = os.path.basename(fingerprint_dir.rstrip("/"))  # relative → the fp dir
    try:
        if os.path.islink(link) or os.path.exists(link):
            os.remove(link)
        os.symlink(target, link)
    except OSError:
        # Fall back to a plain text pointer where symlinks are unavailable.
        with open(link + ".txt", "w") as f:
            f.write(target + "\n")


def _read_faced_frames(buf: bytes, offset: int, version: int):
    """Parse a FACED action's header + size tables + normal frame payloads.

    Returns (width, height, num_frames, frames[facing][frame] = bytes, stype,
    is_v2, end_offset) or None if the action is not a decodable faced/normal
    sprite. ``end_offset`` is the byte position just past the action's frame
    payloads (the start of the following ``Anim`` block in a GOOD/EFFECT
    container), so callers can walk sequentially like the engine's ReadFull.
    """
    stype = spr._u16(buf, offset)
    pos = offset + 2
    width = spr._u16(buf, pos)
    height = spr._u16(buf, pos + 2)
    pos += 4
    facings = spr.K_NUM_FACINGS if stype != 0 else 1
    pos += facings * spr.POINT_SIZE  # hot points
    pos += 2                          # first_frame
    nf = spr._u16(buf, pos)
    pos += 2
    if (width == 0 or height == 0 or width > MAX_ATLAS_DIMENSION
            or height > MAX_ATLAS_DIMENSION
            or width * height > MAX_ATLAS_PIXELS
            or nf == 0 or nf > 512):
        return None

    # Size tables are interleaved per facing: ssizes[j], then msizes[j].
    ssizes = [[0] * nf for _ in range(facings)]
    msizes = [[0] * nf for _ in range(facings)]
    for j in range(facings):
        for i in range(nf):
            ssizes[j][i] = spr._u32(buf, pos)
            pos += 4
        for i in range(nf):
            msizes[j][i] = spr._u32(buf, pos)
            pos += 4

    # Frame payloads are interleaved per facing: nf normal frames, then nf mini
    # frames. We keep the normal frames and skip past the (raw) mini frames so
    # the next facing starts at the right offset.
    is_v2 = version == spr.VERSION_V2
    frames: list[list[bytes]] = [[b"" for _ in range(nf)] for _ in range(facings)]
    for j in range(facings):
        for i in range(nf):
            actual_size = None
            if is_v2:
                actual_size = spr._u32(buf, pos)
                pos += 4
            size = ssizes[j][i]
            payload = buf[pos:pos + size]
            frames[j][i] = decompress_lzw1(payload, actual_size) if is_v2 else payload
            pos += size
        for i in range(nf):
            pos += msizes[j][i]  # skip mini (zoomed-out) frames
    return width, height, nf, frames, stype, is_v2, pos


def _header_end(version: int) -> int:
    """Byte offset just past the file header (tag, version, [compression], type)."""
    return 8 + (4 if version == spr.VERSION_V2 else 0) + 4


def _skip_anim_full(buf: bytes, pos: int) -> int:
    """Advance past one ``ReadAnimDataFull`` block and return the new position.

    Layout (spritefile.cpp ReadAnimDataFull): uint16 type, num_frames,
    playback_time, delay; then num_frames * (uint16 frame, POINT delta,
    uint16 transparency). A zero num_frames is clamped to 1 by the engine.
    """
    num = spr._u16(buf, pos + 2)
    if num == 0:
        num = 1
    pos += 8                       # type, num_frames, playback_time, delay
    pos += num * 2                 # frames (uint16)
    pos += num * spr.POINT_SIZE    # deltas (POINT)
    pos += num * 2                 # transparencies (uint16)
    return pos


def _walk_non_unit_offsets(buf: bytes, version: int, type_id: int) -> list[tuple[str, int]]:
    """Return [(action_name, sprite_header_offset)] for GOOD/EFFECT sprites.

    Mirrors the engine's sequential readers (SpriteFile::ReadFull(GoodSpriteGroup)
    and Read(EffectSpriteGroup)): GOOD is a 1-entry offset table followed by a
    present-flag + sprite + anim; EFFECT is a present-flag + sprite +
    anim-present-flag + anim, repeated for PLAY then FLASH.
    """
    pos = _header_end(version)
    result: list[tuple[str, int]] = []
    if type_id == SPRITEFILETYPE_GOOD:
        pos += GOODACTION_MAX * 4           # leading offset table (unused; read sequentially)
        if spr._u32(buf, pos):              # sprite-present flag
            result.append(("IDLE", pos + 4))
        return result
    if type_id == SPRITEFILETYPE_EFFECT:
        for name in EFFECT_ACTION_NAMES:
            sprite_present = spr._u32(buf, pos)
            pos += 4
            if not sprite_present:
                continue
            sprite_off = pos
            result.append((name, sprite_off))
            parsed = _read_faced_frames(buf, sprite_off, version)
            if not parsed:
                break
            pos = parsed[6]                 # end of frame payloads
            anim_present = spr._u32(buf, pos)
            pos += 4
            if anim_present:
                pos = _skip_anim_full(buf, pos)
        return result
    return []


_PREFIX_GROUP = {
    "GU": spr.SPRITEFILETYPE_UNIT,   # unit
    "GC": spr.SPRITEFILETYPE_UNIT,   # city (stored as a UNIT container)
    "GG": SPRITEFILETYPE_GOOD,       # good
    "GX": SPRITEFILETYPE_EFFECT,     # effect
}


def _effective_group(info: "spr.SprInfo") -> int | None:
    """Return the group layout to read ``info`` with.

    The engine loads a sprite by its filename prefix (GROUPTYPE) and ignores
    the .SPR ``type`` field, which is occasionally junk — e.g. GG023.SPR (the
    only compressed v2 file) stores type 24 but is a normal GOOD container.
    So trust the type field when it names a known group, else fall back to the
    GU/GC/GG/GX filename prefix exactly as SpriteGroupList::LoadSprite does.
    """
    if info.type_id in (spr.SPRITEFILETYPE_UNIT, SPRITEFILETYPE_GOOD, SPRITEFILETYPE_EFFECT):
        return info.type_id
    return _PREFIX_GROUP.get(os.path.basename(info.path)[:2].upper())


def _resolve_actions(info: "spr.SprInfo", buf: bytes):
    """Return the decodable action headers for any supported sprite type.

    UNIT (incl. city GC* sprites) uses the offset table the inspector already
    parses; GOOD/EFFECT are walked here. Returns None for files that match no
    known group layout.
    """
    group = _effective_group(info)
    if group == spr.SPRITEFILETYPE_UNIT:
        # inspect() only populates actions when the type field itself is UNIT;
        # every real GU/GC file has the correct type, so this branch is exact.
        return info.actions
    if group in (SPRITEFILETYPE_GOOD, SPRITEFILETYPE_EFFECT):
        return [spr._read_action_header(buf, off, name)
                for name, off in _walk_non_unit_offsets(buf, info.version, group)]
    return None


def export(path: str, out_dir: str, action_filter: str | None, atlas: bool = False) -> int:
    if os.path.getsize(path) > MAX_INPUT_BYTES:
        raise SprExportError(f"input exceeds {MAX_INPUT_BYTES} byte limit: {path}")
    info = spr.inspect(path)
    with open(path, "rb") as f:
        buf = f.read()
    actions = _resolve_actions(info, buf)
    if actions is None:
        print(f"skip: {os.path.basename(path)} is {info.type_name}, not decodable",
              file=sys.stderr)
        return 2

    os.makedirs(out_dir, exist_ok=True)
    base = os.path.splitext(os.path.basename(path))[0]
    written = 0
    atlas_frames: list[dict] = []
    group = _effective_group(info)
    manifest = {
        "source": os.path.basename(path),
        "version": info.version_name,
        "source_fingerprint": source_fingerprint(path),
        # Effective group (how the engine loads it), not the raw type field —
        # which is junk on GG023 (stored 24, read as GOOD).
        "type": spr.SPRITEFILETYPES[group] if group is not None else info.type_name,
        # Draw flags (transparency/fog/desaturate) are runtime render options,
        # not stored per frame in the .SPR; the renderer-relevant per-frame
        # metadata is the sprite type, size and hot points captured below.
        "actions": [],
    }
    for action in actions:
        if action_filter and action.name != action_filter:
            continue
        if action.sprite_type not in ("FACED", "NORMAL"):
            continue
        parsed = _read_faced_frames(buf, action.offset, info.version)
        if not parsed:
            continue
        width, height, nf, frames, stype, _, _ = parsed
        entry = {
            "name": action.name,
            "sprite_type": action.sprite_type,
            "width": width,
            "height": height,
            "num_frames": nf,
            "facings": len(frames),
            "hot_points": [[p.x, p.y] for p in action.hot_points],
            "frames": [],
        }
        for j, facing_frames in enumerate(frames):
            for i, frame in enumerate(facing_frames):
                grid = decode_frame(frame, width, height)
                rgba = grid_to_rgba_bytes(grid, width, height)
                name = f"{base}_{action.name}_f{j}_frame{i}.png"
                frame_entry = {"facing": j, "frame": i}
                if atlas:
                    atlas_frame = {
                        "action": action.name,
                        "facing": j,
                        "frame": i,
                        "width": width,
                        "height": height,
                        "rgba": rgba,
                    }
                    atlas_frames.append(atlas_frame)
                    frame_entry["atlas_index"] = len(atlas_frames) - 1
                else:
                    write_png(os.path.join(out_dir, name), width, height, rgba)
                    frame_entry["png"] = name
                entry["frames"].append(frame_entry)
                written += 1
        manifest["actions"].append(entry)

    if atlas:
        atlas_width, atlas_height, atlas_rgba = pack_atlas(atlas_frames)
        atlas_name = f"{base}.png"
        write_png(os.path.join(out_dir, atlas_name), atlas_width, atlas_height, atlas_rgba)
        manifest["atlas"] = {
            "png": atlas_name,
            "width": atlas_width,
            "height": atlas_height,
        }
        for action in manifest["actions"]:
            for frame in action["frames"]:
                packed = atlas_frames[frame.pop("atlas_index")]
                frame["rect"] = packed["rect"]

    manifest_name = f"{base}.json" if atlas else f"{base}_manifest.json"
    with open(os.path.join(out_dir, manifest_name), "w") as f:
        json.dump(manifest, f, indent=2)
    output_kind = "atlas frame(s)" if atlas else "PNG frame(s)"
    print(f"wrote {written} {output_kind} + manifest to {out_dir}")
    return 0


def export_tree(path: str, out_dir: str, action_filter: str | None, atlas: bool) -> int:
    """Export every sprite below ``path``; unsupported files are skipped."""
    files = iter_spr_files(path)
    if not files:
        print(f"error: no .SPR files found under {path}", file=sys.stderr)
        return 1

    exported = skipped = failed = 0
    for filename in files:
        rel = os.path.relpath(filename, path)
        target_dir = os.path.join(out_dir, os.path.dirname(rel))
        result = export(filename, target_dir, action_filter, atlas)
        if result == 0:
            exported += 1
        elif result == 2:
            skipped += 1
        else:
            failed += 1

    print(f"walked {len(files)} SPR file(s): exported={exported} skipped={skipped} failed={failed}")
    return 1 if failed else 0


def validate_manifest_data(manifest: dict) -> list[str]:
    """Return manifest contract errors; empty means self-consistent."""
    errors: list[str] = []
    actions = manifest.get("actions")
    if not isinstance(actions, list):
        return ["actions must be a list"]

    atlas = manifest.get("atlas")
    atlas_width = atlas_height = None
    if atlas is not None:
        if not isinstance(atlas, dict):
            errors.append("atlas must be an object")
        else:
            atlas_width = atlas.get("width")
            atlas_height = atlas.get("height")
            if not isinstance(atlas.get("png"), str) or not atlas["png"]:
                errors.append("atlas.png must be a non-empty string")
            if not isinstance(atlas_width, int) or atlas_width <= 0:
                errors.append("atlas.width must be a positive integer")
            if not isinstance(atlas_height, int) or atlas_height <= 0:
                errors.append("atlas.height must be a positive integer")

    for action_index, action in enumerate(actions):
        prefix = f"actions[{action_index}]"
        if not isinstance(action, dict):
            errors.append(f"{prefix} must be an object")
            continue
        frames = action.get("frames")
        if not isinstance(frames, list):
            errors.append(f"{prefix}.frames must be a list")
            continue
        if action.get("num_frames") != len({frame.get("frame") for frame in frames if isinstance(frame, dict)}):
            errors.append(f"{prefix}.num_frames does not match distinct frame indices")
        for frame_index, frame in enumerate(frames):
            frame_prefix = f"{prefix}.frames[{frame_index}]"
            if not isinstance(frame, dict):
                errors.append(f"{frame_prefix} must be an object")
                continue
            if atlas is None:
                if not isinstance(frame.get("png"), str) or not frame["png"]:
                    errors.append(f"{frame_prefix}.png must be a non-empty string")
                continue
            rect = frame.get("rect")
            if not isinstance(rect, dict):
                errors.append(f"{frame_prefix}.rect must be an object")
                continue
            x = rect.get("x")
            y = rect.get("y")
            width = rect.get("w")
            height = rect.get("h")
            if not all(isinstance(value, int) for value in (x, y, width, height)):
                errors.append(f"{frame_prefix}.rect fields must be integers")
                continue
            if x < 0 or y < 0 or width <= 0 or height <= 0:
                errors.append(f"{frame_prefix}.rect has invalid dimensions")
                continue
            if atlas_width is not None and atlas_height is not None:
                if x + width > atlas_width or y + height > atlas_height:
                    errors.append(f"{frame_prefix}.rect exceeds atlas bounds")
    return errors


def validate_manifest(path: str) -> int:
    if os.path.getsize(path) > MAX_MANIFEST_BYTES:
        print(f"BAD {path}: manifest exceeds size limit", file=sys.stderr)
        return 1
    with open(path) as f:
        manifest = json.load(f)
    errors = validate_manifest_data(manifest)
    if errors:
        for error in errors:
            print(f"BAD {path}: {error}", file=sys.stderr)
        return 1
    print(f"OK  {path}")
    return 0


def verify(path: str) -> int:
    """Run the per-row width invariant across every frame of a sprite."""
    if os.path.getsize(path) > MAX_INPUT_BYTES:
        raise SprExportError(f"input exceeds {MAX_INPUT_BYTES} byte limit: {path}")
    info = spr.inspect(path)
    with open(path, "rb") as f:
        buf = f.read()
    actions = _resolve_actions(info, buf)
    if actions is None:
        print(f"skip: {os.path.basename(path)} is {info.type_name}, not decodable")
        return 0
    total_checked = total_bad = 0
    for action in actions:
        if action.sprite_type not in ("FACED", "NORMAL"):
            continue
        parsed = _read_faced_frames(buf, action.offset, info.version)
        if not parsed:
            continue
        width, height, nf, frames, stype, _, _ = parsed
        for facing_frames in frames:
            for frame in facing_frames:
                c, m = verify_frame(frame, width, height)
                total_checked += c
                total_bad += m
    status = "OK " if total_bad == 0 else "BAD"
    print(f"{status} {os.path.basename(path):<14} rows_checked={total_checked} "
          f"mismatched={total_bad}")
    return 0 if total_bad == 0 else 1


def self_test() -> int:
    """Exercise LZW1 without requiring licensed game assets."""
    copy_payload = bytes([LZW1_FLAG_COPY, 0, 0, 0]) + b"raw-frame"
    if decompress_lzw1(copy_payload, len(b"raw-frame")) != b"raw-frame":
        print("self-test failed: LZW1 copy mode", file=sys.stderr)
        return 1

    # Control word 0b1000 (LSB-first): literal A, literal B, literal C,
    # then back-reference offset=3, len=3 -> ABCABC.
    compressed = bytes([
        LZW1_FLAG_COMPRESS, 0, 0, 0,
        0x08, 0x00,
        ord("A"), ord("B"), ord("C"),
        0x02, 0x03,
    ])
    if decompress_lzw1(compressed, 6) != b"ABCABC":
        print("self-test failed: LZW1 back-reference mode", file=sys.stderr)
        return 1

    raw_frame = struct.pack("<HH", 0, EMPTY_TABLE_ENTRY)
    v2_payload = bytes([LZW1_FLAG_COPY, 0, 0, 0]) + raw_frame
    v2_action = b"".join([
        struct.pack("<HHH", 0, 1, 1),   # NORMAL, 1x1
        struct.pack("<ii", 0, 0),       # hot point
        struct.pack("<HH", 0, 1),       # first frame, frame count
        struct.pack("<I", len(v2_payload)),
        struct.pack("<I", 0),           # no mini frame
        struct.pack("<I", len(raw_frame)),
        v2_payload,
    ])
    parsed = _read_faced_frames(v2_action, 0, spr.VERSION_V2)
    if not parsed or parsed[3][0][0] != raw_frame:
        print("self-test failed: v2 frame decompression", file=sys.stderr)
        return 1

    manifest_errors = validate_manifest_data({
        "atlas": {"png": "GU04.png", "width": 2, "height": 3},
        "actions": [{
            "name": "MOVE",
            "num_frames": 2,
            "frames": [
                {"facing": 0, "frame": 0, "rect": {"x": 0, "y": 0, "w": 2, "h": 1}},
                {"facing": 0, "frame": 1, "rect": {"x": 0, "y": 1, "w": 1, "h": 2}},
            ],
        }],
    })
    if manifest_errors:
        print("self-test failed: manifest validator", file=sys.stderr)
        return 1

    frames = [
        {"width": 2, "height": 1, "rgba": b"\xff\x00\x00\xff\x00\xff\x00\xff"},
        {"width": 1, "height": 2, "rgba": b"\x00\x00\xff\xff\xff\xff\xff\xff"},
    ]
    width, height, rgba = pack_atlas(frames, max_width=2)
    if (width, height) != (2, 3):
        print("self-test failed: atlas dimensions", file=sys.stderr)
        return 1
    if frames[0]["rect"] != {"x": 0, "y": 0, "w": 2, "h": 1}:
        print("self-test failed: atlas first rect", file=sys.stderr)
        return 1
    if frames[1]["rect"] != {"x": 0, "y": 1, "w": 1, "h": 2}:
        print("self-test failed: atlas wrapped rect", file=sys.stderr)
        return 1
    if len(rgba) != width * height * 4:
        print("self-test failed: atlas byte size", file=sys.stderr)
        return 1

    with tempfile.NamedTemporaryFile() as tmp:
        tmp.write(b"abc")
        tmp.flush()
        if source_fingerprint(tmp.name) != "ba7816bf8f01cfea":
            print("self-test failed: source fingerprint", file=sys.stderr)
            return 1

    with tempfile.TemporaryDirectory() as tmpdir:
        os.makedirs(os.path.join(tmpdir, "b"))
        with open(os.path.join(tmpdir, "b", "TWO.SPR"), "wb") as f:
            f.write(b"two")
        with open(os.path.join(tmpdir, "one.spr"), "wb") as f:
            f.write(b"one")
        digest = hashlib.sha256()
        for rel, data in (("b/two.spr", b"two"), ("one.spr", b"one")):
            digest.update(rel.encode("utf-8"))
            digest.update(b"\0")
            digest.update(data)
            digest.update(b"\0")
        if source_set_fingerprint(tmpdir) != digest.hexdigest()[:16]:
            print("self-test failed: source-set fingerprint", file=sys.stderr)
            return 1

    # Filename-prefix fallback: the engine loads by GROUPTYPE, so a junk type
    # field (GG023 stores 24) must still resolve to its prefix's group.
    junk = spr.SprInfo(path="graphics/GG023.SPR", size=0, version=spr.VERSION_V2,
                       version_name="v2", compression=1, type_id=24,
                       type_name="unknown(24)")
    if _effective_group(junk) != SPRITEFILETYPE_GOOD:
        print("self-test failed: prefix fallback for junk type", file=sys.stderr)
        return 1
    known = spr.SprInfo(path="GX22.SPR", size=0, version=0, version_name="v0",
                        compression=None, type_id=SPRITEFILETYPE_EFFECT,
                        type_name="EFFECT")
    if _effective_group(known) != SPRITEFILETYPE_EFFECT:
        print("self-test failed: effective group for known type", file=sys.stderr)
        return 1

    # PNG engine-decode contract: colour type 6 / depth 8 / no interlace,
    # a single IDAT, and filter byte 0 on every scanline. The modern-first
    # C++ loader assumes exactly this, so pin it here.
    with tempfile.NamedTemporaryFile(suffix=".png") as tmp:
        write_png(tmp.name, 2, 2, bytes(range(2 * 2 * 4)))
        blob = open(tmp.name, "rb").read()
    if blob[:8] != b"\x89PNG\r\n\x1a\n":
        print("self-test failed: PNG signature", file=sys.stderr)
        return 1

    def _png_chunks(data):
        pos, out = 8, []
        while pos < len(data):
            length = struct.unpack_from(">I", data, pos)[0]
            kind = data[pos + 4:pos + 8]
            out.append((kind, data[pos + 8:pos + 8 + length]))
            pos += 12 + length
        return out

    chunks = _png_chunks(blob)
    ihdr = next(payload for kind, payload in chunks if kind == b"IHDR")
    w, h, depth, colour, comp, filt, interlace = struct.unpack(">IIBBBBB", ihdr)
    if (depth, colour, comp, filt, interlace) != (8, 6, 0, 0, 0):
        print("self-test failed: PNG IHDR not RGBA8/no-interlace", file=sys.stderr)
        return 1
    idats = [payload for kind, payload in chunks if kind == b"IDAT"]
    if len(idats) != 1:
        print("self-test failed: PNG must have a single IDAT", file=sys.stderr)
        return 1
    raw = zlib.decompress(idats[0])
    if len(raw) != h * (1 + w * 4) or any(raw[j * (1 + w * 4)] != 0 for j in range(h)):
        print("self-test failed: PNG scanline filter bytes must be 0", file=sys.stderr)
        return 1

    print("self-test OK: LZW1 streams + atlas packer + group resolver + PNG contract")
    return 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(
        description="Export CTP2 .SPR frames to debug PNGs / atlases (read-only).")
    ap.add_argument("path", nargs="?", help="path to a .SPR or directory of .SPR files")
    ap.add_argument("-o", "--out-dir", default="spr_export",
                    help="output directory for PNGs (default: ./spr_export)")
    ap.add_argument("--action", help="only export this action (e.g. MOVE)")
    ap.add_argument("--verify", action="store_true",
                    help="parity check only (per-row width invariant), no PNGs")
    ap.add_argument("--atlas", action="store_true",
                    help="write one packed atlas PNG and rect manifest instead of per-frame PNGs")
    ap.add_argument("--modern-assets", action="store_true",
                    help="write to $CTP2_HOME/assets/<source-fingerprint>/ (default: ~/.ctp2)")
    ap.add_argument("--self-test", action="store_true",
                    help="run synthetic decoder self-tests; does not read assets")
    ap.add_argument("--validate-manifest", metavar="JSON",
                    help="check an exported manifest for atlas/rect consistency")
    args = ap.parse_args(argv)
    try:
        if args.self_test:
            return self_test()
        if args.validate_manifest:
            return validate_manifest(args.validate_manifest)
        if not args.path:
            ap.error("path is required unless --self-test is used")
        if args.verify:
            return verify(args.path)
        out_dir = modern_assets_dir(args.path) if args.modern_assets else args.out_dir
        if os.path.isdir(args.path):
            rc = export_tree(args.path, out_dir, args.action, args.atlas)
        else:
            rc = export(args.path, out_dir, args.action, args.atlas)
        if args.modern_assets and rc == 0:
            update_current_pointer(out_dir)  # engine reads <root>/current/<base>.json
        return rc
    except (OSError, spr.SprError, SprExportError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
