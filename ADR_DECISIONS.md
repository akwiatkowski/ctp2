# Architecture Decision Records

Short log of non-trivial design decisions. Newest first.

## ADR-002 — World GPU layer is a 1:1 mirror of the background window (2026-07-16)

**Context.** ADR-001's dedicated margin render (`RenderWorldLayer`: temporarily widen the
view, repaint tiles + sprites into the oversized surface) placed terrain and actors at
inconsistent offsets — terrain shifted by the legacy window margin (+94,+72), sprites by
their cached screen positions. Invisible while the world layer never showed (the pre-fix
routing bug); exposed the moment the pixel proof could see the layer.

**Decision.** Drop the re-render entirely. The legacy background window surface already IS
an oversized world render — one tile-grid margin (94/72px) on every side, terrain AND actors
drawn by the battle-tested pipeline, kept fresh by ScrollMap. Mirror that whole surface 1:1
into the world layer at the `BltToSecondary` chokepoint (the composite clips its rects to
the screen, so a rect-scoped mirror would leave stale margins) and pin the present's content
offset to the window margin constants (static_asserted against `k_TILE_GRID_*`). Recenter
forces a synchronous window redraw + `DrawOne` composite so `ShiftPan` and fresh content land
in the same frame. Glide margin is 94/72px — at zoom 1 that is smaller than one 96px tile
column, so the recenter forces a ±1-tile scroll near the margin edge instead of waiting for
a full tile.

**Alternatives considered.** Fixing `RenderWorldLayer`'s two coordinate systems — rejected:
it duplicates the legacy projection (wrap, clamp, actor placement) that the window render
already gets right; a full-surface memcpy (~4.4MB) per composite is trivial on M4 Pro.

**Consequences.** The glide can slide at most the window margin before a recenter (more
recenters than ADR-001's 192px margin aimed for — measured invisible in the pixel proof).
Proven by the `pan-pixel-proof` integration test: forced-offset shift + a streamed glide
passing through sub-tile positions (6,26,46,…,190px) across recenter boundaries.

## ADR-001 — Buttery trackpad panning via an oversized GPU world texture (2026-07-16)

**Context.** Two-finger trackpad panning ships (default-on, real `ScrollMap`) but is
tile-stepped — chunky on slow drags. The goal is Apple-Maps-grade smoothness: pixel-perfect,
GPU-driven, with native momentum, on the default build. macOS already streams `SDL_MOUSEWHEEL`
events through the momentum-glide phase, so momentum is free; the missing piece is sub-tile
smoothness. A screen-sized world texture cannot pan sub-tile without revealing a black edge.

**Decision.** Render the world into an **oversized** GPU texture (screen + ~2-tile margin on
all sides). Pan the screen-sized viewport within it on the GPU via a moving source rect
(`CameraOff`), and re-render terrain underneath only when a whole tile is crossed
(`ScrollMap` into the oversized surface). The margin hides the seam. The smooth motion is
100% GPU; the CPU only does the occasional whole-tile `ScrollMap` (repaints just the revealed
strip). Requires the `CTP2_GPU_LAYERS`+`CTP2_GPU_CAMERA` stack to become default-on for the map.
The map's render extent already follows the destination surface size (`RetargetTileSurface`
sets `m_surfaceRect` from `dest->Width()/Height()`), so oversizing the world surface makes the
map render wider without a coordinate-system rewrite; UI stays screen-sized.

**Alternatives considered.**
- *Native `ScrollMapSmooth`* (CPU `ScrollPixels` memmove + edge repaint): dead/gated-off code
  (`IANSCROLL` undefined, `g_smoothScroll` hardcoded FALSE); reviving it is a CPU shortcut that
  contradicts "use the GPU as much as possible." Rejected.
- *GPU camera pan without margin + recenter*: reveals black edges before recenter (the same
  failure that parked the terrain-quad path). Rejected.
- *Terrain-as-GPU-quads*: parked — needs a full terrain/actor layer split for correctness.

**Consequences.** More GPU memory (oversized texture) and a larger per-recenter upload — both
trivial on Apple unified memory; the design explicitly need not be fast. Flipping the GPU
stack default-on inherits the D "UI-ghost on moving windows" caveat (does not affect panning,
since UI is static during a pan; fixed separately if it bites general use). Builds behind the
existing flags, verified green, then flipped default-on last.
