# Modern Asset Pipeline — Design

Status: **design / spike** (M8 in `REFACTORING_PLAN.md`). The read-only
decoders exist (`tools/assets/spr_inspect.py`, `tools/assets/spr_export.py`),
and `spr_export.py --atlas --modern-assets` can pack decoded v0/v1 unit frames
into one PNG atlas plus manifest rects under `~/.ctp2/assets/<fingerprint>/`.
Source-set walking, first-run conversion, and engine integration are not built
yet.

## Goal

Let a player run the game with the original Call To Power 2 data they already
own, and have that data converted **once, locally**, into a modern, GPU-friendly
form that a future renderer can load directly. The original files stay the
canonical source; the generated modern files are a **persistent, rebuildable
derived asset set** the user creates on their own machine.

Per the project owner's direction: treat the output as *a modern version of the
assets*, not a throwaway cache — it is meant to persist, though it can always be
regenerated from the original data.

## Licensing — findings and constraints

The project owner asked whether converting the assets is acceptable under the
license. Findings (good-faith reading, not legal advice):

- **Engine source code** is the Activision 2001 source release
  (`Activision CTP2 Source Code_Readme.txt`) as continued by the Apolyton CtP2
  Source Code Project (`Apolyton CTP2 Source Code_Readme.txt`). The community
  has modified and redistributed the *code* for two decades; adding converter
  tools and loader changes is consistent with that.
- **Game data (sprites, sounds, graphics) is NOT part of the source release.**
  Both readmes state it explicitly: *"YOU NEED THE DATA DIRECTORY FROM THE
  ORIGINAL SHIPPING GAME. All the graphics, sounds, and other data files needed
  to actually run the game are not included here."* The data remains Activision
  copyrighted content that each user must supply from their own legitimate copy.

Consequences the design must honor:

1. **Convert locally, from the user's own data.** The modern assets are
   generated on the user's machine from data they legally possess — a personal
   format shift, analogous to transcoding your own media.
2. **Never redistribute or commit generated assets** — they are derivative works
   of Activision's copyrighted data. Likewise do not ship the originals.
3. **Store outside the repository**, in a user-home location (below), and add the
   output path to `.gitignore` if it could ever land under the tree.
4. Pre-existing risk to flag separately: this working tree currently contains a
   populated `ctp2_data/` with original assets. That is the owner's local copy
   for development; it should not be part of any public/redistributed artifact.

## Output location

```
~/.ctp2/assets/<source-fingerprint>/
```

- User-home, persistent (not a disposable cache). `~/.ctp2/` per the owner's
  preference; a platform-specific path (XDG / `~/Library/Application Support`)
  can be adopted later if desired.
- `<source-fingerprint>` is a hash of the source data so that pointing the
  converter at a different or updated data set produces a distinct,
  independently valid output tree rather than silently mixing versions. The
  current `spr_export.py --modern-assets` seam hashes one source `.SPR` file;
  the future first-run converter should hash the walked source set.

## Target format — packed texture atlas

Chosen direction: **packed texture atlas per unit**, not one PNG per frame.

- One atlas image per source sprite (e.g. `GU04.png`), packing every
  action/facing/frame into a single texture. Format PNG initially; KTX2/Basis
  compression can follow once a renderer needs it.
- A sidecar manifest (`GU04.json`) describing, per action:
  - sprite type, frame `width`x`height`, `num_frames`, facing count,
  - per-facing hot points (draw anchor),
  - for every (facing, frame): its rectangle `{x, y, w, h}` in the atlas.
- The per-frame PNG dump that `spr_export.py` already produces is the
  parity-proving intermediate; `spr_export.py --atlas` consumes the same decoded
  RGBA frames and metadata, just laid out into one texture.

Draw-time options (transparency, fog, desaturation, feathering) are **runtime
render flags**, not stored per frame in the source `.SPR`, so they are a renderer
concern, not part of the generated asset.

## Converter

Offline tool, extending the existing read-only Python decoders
(`spr_inspect.py` + `spr_export.py`), run via `mise`. It:

1. Walks the original data set (sprites first; tiles/other assets later).
2. Decodes each asset (already implemented for v0/v1 unit sprites; v2 has a
   synthetic LZW1 decoder seam but still needs real-asset parity).
3. Packs frames into atlases and writes the atlas + manifest. The current
   `--atlas --modern-assets` path writes a single source file's output under
   `~/.ctp2/assets/<source-fingerprint>/`; the future first-run converter should
   validate and populate a whole walked source set there.

Keeping the converter offline (rather than embedded in the engine) keeps all
format knowledge in one place and makes it fast to iterate. A future "run it
automatically on first launch" step can shell out to this tool.

## Engine fallback rule

The engine's asset loaders gain a modern-first path:

> When a valid generated modern asset exists for the requested item (correct
> `<source-fingerprint>`, manifest present and self-consistent), load it.
> Otherwise fall back to the existing legacy `.SPR`/loader path.

This keeps original-data and mod compatibility intact: nothing breaks if the
modern assets are absent, partial, or invalidated — the game still runs from the
canonical originals.

## Open items before implementation

- Real-asset v2 (LZW1) sprite pixel parity.
- Non-unit asset types (tiles `.TIF`, cities, goods, effects, sounds).
- Whole-source-set fingerprinting and first-run output placement.
- Whether/when to auto-invoke the converter on first launch.
