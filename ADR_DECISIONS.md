# Architecture Decision Records

Short log of non-trivial design decisions. Newest first.

## ADR-013 — P11 close-out: quads+raster supersede shaders; CPU fallback retained (2026-09-09)

**Context:** P11's remaining tail (Stage-2 E shader flags, Stage-3 G5 CPU-path
retirement, render fixtures, zoom matrix, dummy-video, tolerance metric) was
audited against the shipped default-on stack (LAYERS→CAMERA→QUADS→WORLDMAP→
RASTER, modern-first atlases, SDL3). Gallery audit: 81/81 sprite scenes zero
fallback, walls/forcefield parity uniform at 95.48% (capture-tick delta, not a
gap); 2 CPU cells there are by-design composite cells.

**Decision:** E (SDL3 GPU-API fragment shaders) is retired — the shader-free
quad + raster-overlay approach covers fog/desat/grid with pixel parity, so
shaders buy nothing. G5 (retire CPU path) is declined: the fallback is load-
bearing for missing-atlas installs (proven by gpu-actor-parity fallback mode)
and cut-content WORK frames. CPU fallback stays as the correctness net with
per-frame reason reporting. Dummy-video (309/310) declined: hidden-window
rendering works on dev and CI runners; a no-video abstraction is cost without
a consumer. Tolerance metric (311) declined: exact-RGB oracles are green.
Render fixtures landed instead (`render_new_scene/add_unit_sprite/set_tile/
set_fog/camera_pose` + `render-fixtures` integration test incl. 0.5–2.0 zoom
matrix); the `render_` prefix split (308) keeps fixture verbs unconfusable
with gameplay debug verbs. Tile/overlay exporter requirements (258): quads
sample the tile cache + sprite atlases; any exporter must preserve the
signature-keyed whole-cell composite contract.

**Alternatives:** Deleting the CPU path would strand missing-atlas installs.
Keeping E open would invite a second rendering backend with no parity gain.

**Open (not P11-blocking, tracked as ctp2-721):** three dev-only pixel gates
(gpu-actor-parity, gpu-trade-animation, gpu-sprite-refresh) fail identically
at clean HEAD — debug-created units never paint while idle (byte-identical
captures, no errors). CI never runs the integration suite. Suspects:
idle-present gating (presents only fire on camera motion/mouse), stale
oracle readback when QuadFrameComplete is false, per-unit visibility for
debug-created units.
acceptance. Tier-C cost note: marathon wall is dominated by JSON AI-history
autosave serialization (finding, 2026-09-09), not rendering.

## ADR-012 — Sanitizer hardening: init storable sentinels, validate enum loads, size slots from the save (2026-09-09)

**Context:** UBSan `halt_on_error` aborted three suites. A default-constructed
`Order` held an indeterminate `m_eventType`, so serializing it was an invalid
enum load; `TerrainImprovementData(ID)` left both bools uninitialized (value 176
loaded as bool). Headless `--load-game` sized player slots from the profile
(`NumPlayers=4`), not the save: a 5-player save restored 4 Players and the
empire-bounds size check rightly rejected it.

**Decision:** Initialize both members to their storable sentinels and
range-check `GAME_EVENT`/`UNIT_ORDER_TYPE` before `static_cast`, so corrupt
saves reject cleanly instead of aborting. Peek the save's alive count and call
`SetNPlayers` before `RestoreGame`; the bounds error now reports both sizes.
Keep the strict shape contract — growing slots beyond the gameinit shape would
need invented civs/types, so sizing-from-save is the permanent design unless
that requirement changes.

**Alternatives:** Zero-initializing the members would store a value outside the
valid enum range on some builds; sentinel-init keeps every stored value
representable. Growing player slots in the loader was rejected (see above).

**Consequences:** Full UBSan unit suite 643 passed, 0 failed, 0 findings;
500/500 marathon rounds plus deep save/load to round 520; 8/8 scenario tests.
JSON/SPR readers now validate input bounds at every layer (enum range,
`RequireSprite` frame/offset/truncation guards), with failed sprite loads
releasing partial state and logging the reason.

## ADR-011 — Shared presentation and explicit sprite fallback (2026-09-08)

**Context:** Installed V20 sprite manifests used ACTION_1/ACTION_3 instead of
ATTACK/VICTORY. Software drawing fell back to SPR, but whole-map presentation
ignored incomplete sprite frames. The CPU mirror also subtracted a margin
already absent from its painter coordinates, and screenshot readback duplicated
presentation policy. Goods omitted fog state and trade actors emitted no quads.

**Decision:** Export canonical unit action names and accept the old names when
reading manifests. Route desktop and screenshot world presentation through
PresentWorldFrame, using the CPU mirror whenever sprite submission is incomplete.
Software mirror pixels use origin zero; window quads include their content margin.
Mirror source clipping also shrinks its destination to prevent SDL stretching
the remaining pixels near an edge. Picking inverts both zoom and pan on either path. Debug camera setters hold their
requested target/home zoom so fixed-pose tests do not drift during animation ticks.
Pass goods fog into GPU modulation, and emit trade sprites from the normal actor
painter. Keep CPU actor painting for fallback, with a macOS sample-based profiler
and presented-pixel tests documenting the cost and behavior.

**Alternatives:** Reinstalling every atlas would leave existing installations
broken. Dropping failed actors or checking quad counts alone hides missing pixels.
Skipping CPU drawing before checking the entire frame would leave the fallback
surface incomplete. Separate desktop/readback fixes would invite another drift.

**Consequences:** Whole-map fallback preserves actors at the cost of CPU composition.
Pixel tests isolate actor changes, including missing-atlas installations and fogged
goods. GPU health bars use the same four-pixel height as the software painter.
Profiling measures CPU stacks, not GPU execution time or frame rate; further CPU
elimination needs a measured fallback design, not unconditional draw suppression.

## ADR-010 — Reproducible renderer evidence and explicit GPU coverage (2026-09-08)

**Context.** UI tests discarded their seed, forced camera refreshes after actor
changes, and the gallery's map-only screenshot used the old window-quad path.
A fog comparison also sampled different map patches when the reference fell
back to the CPU window mirror. Flags and background-dominated pixel agreement
could conceal missing sprites.

**Decision.** Forward UI seeds/player counts through the smoke protocol; retain
fog scenes for replay; compare fog over matching map coordinates. Use presented
composition for gallery captures, explicit renderer flags, and fixed poses
exposed only by the render tool. Exercise lifecycle transitions through the
normal runtime. Report GPU and CPU cell counts separately from submitted quads.

**Consequences.** Gallery output is review evidence, not blanket acceptance.
The CPU/world-map registration discrepancy remains visible. Unsupported poses
are recorded as skips. CPU sprite drawing still runs alongside GPU emission;
removing it requires a separate completeness/fallback decision. The new runtime
checks exposed and fixed post-load view initialization and FLASH-only GPU effects.

## ADR-009 — Refresh GPU sprites through the normal actor painter (2026-09-08)

**Context.** GPU sprite quads were built only when terrain refreshed. Idle frames,
creation, removal, and active movement could leave sprites stale. The separate
GPU tile loop also chose only one actor, omitting units sharing a city tile.
Camera recentering in renderer tests hid the refresh dependency.

**Decision.** Rebuild the dynamic GPU sprite list during each full `RepaintSprites`
pass. Emit quads through the existing unit, good, and effect painters, using current
actor coordinates and the same visibility/stacking decisions as CPU drawing.
Keep terrain cached independently, and reset sprite fallback state per sprite frame.
Scroll-strip paints do not replace the full view's list.

**Alternatives.** Refreshing terrain every frame adds unnecessary work. Maintaining
a second GPU actor-selection loop duplicates visibility and animation rules.

**Consequences.** City labels and placement overlays join the dynamic pass. The
`gpu-sprite-refresh` renderer regression creates and disbands a city garrison with
a stationary camera, checking movement, quad counts, and presented map pixels.

## ADR-008 — Persist AI decision history and scheduler identity (2026-09-06)

**Context.** Rebuilding AI schedulers after loading discarded retained settlement
goals. Seed 42 first diverged at round 39 after saving at round 25. Settlement
scores and empire bounds also contain history that the current world cannot
reconstruct. Equal-utility pointer sorting introduced process-dependent ties.

**Decision.** Serialize the scheduler graph using stable agent/goal indices,
including generic goals, typed/active order, matches and commitments. Validate
ownership, references and numeric bounds in temporary storage before swapping
the graph into the scheduler. Restore after scheduler resizing, which can copy
objects. Persist settlement scores and empire bounds alongside it. Keep these
bridges in the AI module; the save layer calls two AI-history entry points.
Use stable utility sorting and refresh the CLI pathfinding heuristic at the same
round boundary as interactive play.

**Alternatives.** Regenerating all goals loses earlier decisions. Serializing
addresses cannot preserve identity across processes. Recomputing settlement
scores and empire bounds from current positions loses growth and exploration
history.

**Consequences.** New saves preserve AI state across a load. Older saves without
this state retain the regeneration fallback and do not gain exact replay
guarantees. Replay tests compare AI, world, players, RNG and action logs through
rounds 11, 40 and 75 in both CLI load modes. These are bounded regression checks,
not a proof of determinism for every possible campaign.

## ADR-007 — Standard data paths are built-in defaults (2026-09-05)

**Context.** CivPaths required a working-directory `civpaths.txt` before it could
select an installed `CTP2_HOME`, so launching outside the checkout crashed.

**Decision.** Initialize the standard layout in CivPaths and treat the existing
configuration file as an optional override. Apply the installed home afterwards.

**Alternatives.** Copying a required configuration file into each installation
would make existing installations require migration. Finding the source checkout
from the executable would retain a development-only runtime dependency.

**Consequences.** Existing installs launch from arbitrary directories. Custom
checkout configurations keep working; standard layout changes must update these
defaults alongside `civpaths.txt`. The CLI startup regression uses an empty directory.

## ADR-006 — Cache overlay stamps and composite shadowed cells on the GPU (2026-09-05)

**Context.** Improvements and political borders forced whole-map cells through CPU
composition. Their existing selection rules encode road connectivity and visibility;
shadow runs also depend on the destination pixels.

**Decision.** Reuse those selection rules while capturing their drawing calls as
cached atlas quads. Decode colour pixels and multiplicative shadow masks separately.
Composite shadowed cells into one reusable transparent GPU target before placing them
on the map, preserving the CPU cell boundaries. Keep CPU fallback for unsupported
entries or a full atlas. Include improvement types, road connections and ruin variants
in the shared tile cache key.

**Alternatives.** Reimplementing road and visibility rules would duplicate gameplay
logic. Drawing shadows directly onto the whole map would darken overlapping neighbours.
Precompositing every improvement variant would retain CPU work and multiply cache entries.

**Consequences.** The native-zoom renderer can draw improvements and either border style
without uploading a CPU-composited tile. Generated stamps remain disposable caches.
The presented-frame parity test requires exact pixels and a visibly populated fixture;
`make test-render` runs it with the other renderer gates in an isolated installation.

## ADR-005 — One configurable home for installed data and generated assets (2026-09-04)

**Context.** The engine historically depended on the repository working directory for
`ctp2_data`, while converted sprites lived separately under a hard-coded `~/.ctp2/assets`.
That made the source checkout part of the runtime installation and let profiles and saves
leak back into the working tree.

**Decision.** `$CTP2_HOME` is the single installed-data root, defaulting to `~/.ctp2`. Its
stable layout is `original_data/` for the complete verbatim game data, `assets/` for
fingerprinted generated caches with a `current` pointer, and `saves/` for mutable games and
scenario data. The engine, profile database, sprite loader, converter, and installer all use
that root. Modern sprites are default-on and may be disabled with
`CTP2_MODERN_SPRITES=0`; every asset type without a modern converter continues to load from
`original_data`. If `original_data` is absent, the existing working-directory layout remains
as a compatibility fallback.

**Alternatives considered.** Making converted files canonical was rejected because only
sprites currently have a converter and original/mod data remains authoritative. Removing the
legacy fallback immediately was rejected because it would make migration needlessly brittle.

**Consequences.** A checkout is no longer required after running the idempotent installer,
and tests can isolate all mutable state with a temporary `CTP2_HOME`. The installer never
deletes source data; removing the in-tree copy remains a separate explicit user action after
visual verification.

## ADR-004 — Networking is opt-in at compile time (2026-09-04)

**Context.** Multiplayer is outside the completion target, but the normal SDL build still
linked Anet and compiled the legacy network transport and lobby UI. That expanded the runtime
and security surface while giving single-player no benefit.

**Decision.** Meson's `anet` option defaults to `false`. Normal `ctp2` and
`ctp2_headless` builds omit Anet, transport threads, and netshell UI and compile guarded
single-player entry paths. `-Danet=true` retains the source-compatible multiplayer build for
future work. The small `net/general` replication shims still referenced by gameplay remain in
both configurations until those call sites are separated.

**Alternatives considered.** Deleting networking was rejected because source preservation was
requested. Leaving it linked but hiding multiplayer UI was rejected because it would retain
the dependency and attack surface.

**Consequences.** Shipping/default binaries have no Anet dependency or transport symbols.
The opt-in configuration must remain buildable, but multiplayer behavior is not part of the
single-player completion gate.

## ADR-003 — Whole-map GPU texture; the camera owns zoom (2026-08-03)

**Context.** ADR-002's window mirror gives a buttery pan but only a screen+margin texture, so
zoom still has to be negotiated with the engine: `ui_StepPinchZoom` steps `ZoomIn`/`ZoomOut`
through the 6-level table (`s_zoomTileScale` = 0.505…1.0, a 2x range total) and counter-scales
the camera by `oldScale/newScale` while a spring-back eases it home. Two consequences.
(1) The UX is stepped, not continuous — pinch works but does not feel like a map view.
(2) A live regression: `ui_RecenterPanIfNeeded` budgets the pan offset against a fixed 94/72px
margin, but the present's zoom-centring term `(W - W/z)/2` (`aui_sdlsurface.cpp`) consumes
margin the recenter never accounts for — at z=0.9 on a 1920-wide screen that is ~107px against
a 94px margin. Any `CameraZoom() != 1` therefore makes the recenter fire at the wrong offset
and the compensating `ScrollMap`+`ShiftPan` teleports the view. Pinch creates exactly that
state, so panning after a pinch teleports. The goal is a real map view: ~0.21x (whole Gigantic
map on screen) to 2.0x, continuous, with a detent at 100%.

**Decision.** Render the **whole map** into one GPU render-target texture, updated by dirty
region only when the game changes it — never on pan or zoom. Pin the engine at zoom scale 1.0
and give the GPU camera **sole** ownership of zoom; the zoom table stops driving rendering.
Pan and zoom become a source rect over the full-map texture.

Consequent choices:
- **Quads, not CPU composite.** At 100% the largest map (Gigantic, 70x140 tiles, 94x72 grid) is
  6,580 x 5,040px = ~133MB. A CPU-composited surface that size plus its upload is untenable; a
  render target fed by tile quads never materializes it, and "dirty" reduces to redrawing N
  tiles. This un-parks the G-series — there as an upload optimization, here as the enabling
  mechanism.
- **Filter mode keys off zoom direction.** NEAREST above 1.0 (crisp pixel art, explicitly
  wanted at 200%); a small manual mip pyramid (full+half+quarter, ~174MB) below 1.0, since
  plain bilinear at 0.21x shimmers during pan.
- **Detent at 100%**: resistance near 1.0 plus ease-to-exact on gesture end, which also buys
  pixel-exact rendering at rest.

**Alternatives considered.**
- *Keep the window mirror, make the recenter zoom-aware.* Fixes the teleport but leaves zoom
  stepped and capped at the table's 2x. Worth doing as an interim fix on the shipping path,
  not as the destination.
- *Interpolate between engine zoom levels (treat the table as a mip pyramid).* Smooth within
  2x with far less work, but cannot reach 0.21x and keeps two systems owning zoom. Rejected as
  the endpoint; still a viable fallback if the whole-map substrate stalls.
- *Store the texture at 200% so zoom-in is native.* 4x the memory (~530MB) to avoid a
  pixel-art look that is explicitly desired. Rejected.

**Consequences.** Deletes the margin/recenter/`ShiftPan` machinery and the bug class above —
the camera can no longer window past a margin because there is no margin. Costs, in order of
risk: (1) **mouse picking** — `MousePointToTilePos` divides by `GetZoomScale`
(`tiledmap.cpp:5150`); it must go through the camera transform instead, and everything
downstream is affected; (2) **the pixel oracle** — `slice-ui` / `screenshot_map_only` assume a
screen-sized world texture and must be updated in the same steps, not after (a dead GPU present
already hid for a whole phase once); (3) **non-terrain content** — roads, rivers, borders, grid,
goods and actor overlays are all mixed into the one background surface by `CalculateWrap`;
quads reproduce only `DrawTransitionTile`, so each needs its own draw path. Fog moves from a
screen-space mask to a one-texel-per-tile whole-map mask (an improvement). CTP2's hand-authored
lower-zoom tile art is dropped in favour of GPU downscaling — a fidelity change judged
irrelevant at overview zooms. Supersedes ADR-002 for the world layer; ADR-002's mirror stays as
the fallback path until the new substrate is proven.

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
