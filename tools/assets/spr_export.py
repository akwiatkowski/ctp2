#!/usr/bin/env python3
"""Decode CTP2 ``.SPR`` unit-sprite frames to debug PNGs (read-only).

Second slice of the M8 "modern asset pipeline" spike (see
``REFACTORING_PLAN.md``). Given a unit ``.SPR`` this decodes the run-length
encoded 16-bit frames into RGBA and writes one PNG per action / facing /
frame. It never modifies the original ``.SPR`` — original assets stay
canonical; the PNGs are a rebuildable debug artifact.

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
import json
import os
import struct
import sys
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

TRANSPARENT = None  # marker for an unset pixel


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
    """Write an 8-bit RGBA PNG with no third-party dependencies."""
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


def _read_faced_frames(buf: bytes, offset: int, version: int):
    """Parse a FACED action's header + size tables + normal frame payloads.

    Returns (width, height, num_frames, frames[facing][frame] = bytes) or None
    if the action is not a decodable faced/normal sprite.
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
    if nf == 0 or nf > 512:
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
            if is_v2:
                pos += 4  # actual_size prefix (LZW1) — payload undecoded here
            size = ssizes[j][i]
            frames[j][i] = buf[pos:pos + size]
            pos += size
        for i in range(nf):
            pos += msizes[j][i]  # skip mini (zoomed-out) frames
    return width, height, nf, frames, stype, is_v2


def export(path: str, out_dir: str, action_filter: str | None) -> int:
    info = spr.inspect(path)
    if info.type_name != "UNIT":
        print(f"error: {path} is {info.type_name}, not UNIT", file=sys.stderr)
        return 1
    if info.version == spr.VERSION_V2:
        print("note: v2 sprites use LZW1 compression; pixel decode not yet "
              "implemented for v2 (header/metadata only).", file=sys.stderr)
        return 2

    with open(path, "rb") as f:
        buf = f.read()

    os.makedirs(out_dir, exist_ok=True)
    base = os.path.splitext(os.path.basename(path))[0]
    written = 0
    manifest = {
        "source": os.path.basename(path),
        "version": info.version_name,
        "type": info.type_name,
        # Draw flags (transparency/fog/desaturate) are runtime render options,
        # not stored per frame in the .SPR; the renderer-relevant per-frame
        # metadata is the sprite type, size and hot points captured below.
        "actions": [],
    }
    for action in info.actions:
        if action_filter and action.name != action_filter:
            continue
        if action.sprite_type not in ("FACED", "NORMAL"):
            continue
        parsed = _read_faced_frames(buf, action.offset, info.version)
        if not parsed:
            continue
        width, height, nf, frames, stype, _ = parsed
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
                write_png(os.path.join(out_dir, name), width, height, rgba)
                entry["frames"].append({"facing": j, "frame": i, "png": name})
                written += 1
        manifest["actions"].append(entry)

    with open(os.path.join(out_dir, f"{base}_manifest.json"), "w") as f:
        json.dump(manifest, f, indent=2)
    print(f"wrote {written} PNG frame(s) + manifest to {out_dir}")
    return 0


def verify(path: str) -> int:
    """Run the per-row width invariant across every frame of a unit sprite."""
    info = spr.inspect(path)
    if info.type_name != "UNIT":
        print(f"skip: {os.path.basename(path)} is {info.type_name}, not UNIT")
        return 0
    if info.version == spr.VERSION_V2:
        print(f"skip: {os.path.basename(path)} is v2 (LZW1, not yet decoded)")
        return 0
    with open(path, "rb") as f:
        buf = f.read()
    total_checked = total_bad = 0
    for action in info.actions:
        if action.sprite_type not in ("FACED", "NORMAL"):
            continue
        parsed = _read_faced_frames(buf, action.offset, info.version)
        if not parsed:
            continue
        width, height, nf, frames, stype, _ = parsed
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

    print("self-test OK: LZW1 copy + compressed streams")
    return 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(
        description="Export CTP2 unit .SPR frames to debug PNGs (read-only).")
    ap.add_argument("path", nargs="?", help="path to a unit .SPR (e.g. GU04.SPR)")
    ap.add_argument("-o", "--out-dir", default="spr_export",
                    help="output directory for PNGs (default: ./spr_export)")
    ap.add_argument("--action", help="only export this action (e.g. MOVE)")
    ap.add_argument("--verify", action="store_true",
                    help="parity check only (per-row width invariant), no PNGs")
    ap.add_argument("--self-test", action="store_true",
                    help="run synthetic decoder self-tests; does not read assets")
    args = ap.parse_args(argv)
    try:
        if args.self_test:
            return self_test()
        if not args.path:
            ap.error("path is required unless --self-test is used")
        if args.verify:
            return verify(args.path)
        return export(args.path, args.out_dir, args.action)
    except (OSError, spr.SprError, SprExportError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
