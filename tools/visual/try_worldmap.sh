#!/usr/bin/env bash
# try_worldmap.sh — hands-on check of the P13 whole-map camera (trackpad pan +
# pinch zoom). This is the part no automated test can do: the transforms are
# unit-proven, but how it FEELS is yours to judge.
#
#   ./tools/visual/try_worldmap.sh          # whole-map camera ON  (P13, new)
#   ./tools/visual/try_worldmap.sh legacy   # ADR-002 window mirror (shipping)
#
# Run both and compare. Logs land in logs/worldmap-<mode>.log.
set -euo pipefail
cd "$(dirname "$0")/../.."

MODE="${1:-worldmap}"
mkdir -p logs
[ -f appstr.txt ] || ln -sf ctp2_code/ctp/appstr.txt appstr.txt

if [ "$MODE" = "legacy" ]; then
    export CTP2_GPU_WORLDMAP=0
    echo "=== LEGACY path (ADR-002 window mirror) — today's shipping behaviour ==="
else
    export CTP2_GPU_WORLDMAP=1
    echo "=== WHOLE-MAP path (P13) — camera owns zoom ==="
fi

cat <<'EOF'

Start or load a game first — everything below is inert in the menus.

EXPECT ON THE WHOLE-MAP PATH: terrain, units, cities and effects. Units and
cities pan and zoom WITH the terrain -- that is the part to check.

Also present now: roads and other improvements, rivers, goody huts, the grid,
goods, and national borders (the smooth/icon style).

Still missing: LINE-style national borders, which draw through a routine that
cannot target a tile-sized buffer. Switch borders to smooth in the graphics
options to see them here.

WHAT TO JUDGE — the questions the code deliberately left open:

  1. PINCH ZOOM
     Pinch in and out over the map. It should be continuous, not stepped.
     Range is ~0.21x (whole map on screen, large maps) to 2.0x.
     - Does it track your fingers, or lag / jump?
     - At 200%: crisp pixel-art blocks are INTENDED. Blurry is a bug.

  2. THE 100% DETENT
     Zoom slowly through 1:1 and let go near it. It should settle exactly on
     100%, where terrain is pixel-exact.
     - Too grabby (hard to sit just off 1.0)? Band is too wide.
     - Never catches? Too narrow. It is 0.04 in camera_window.h (ZoomDetent).

  3. TWO-FINGER PAN
     Drag at 100%, then zoomed right in, then zoomed right out.
     - Does the map track your fingers 1:1 at EVERY zoom? That is the fix in
       bf41e099; sluggishness when zoomed out means it regressed.
     - At the map edges it should stop cleanly, not judder or show black.

  4. SHIMMER WHEN ZOOMED OUT  <-- this one decides real work
     Zoom out near the minimum and pan slowly. Watch for crawling / sparkling
     on terrain edges.
     - Steady => linear filtering is enough, and we skip the mip pyramid.
     - Shimmering => the pyramid is needed (~41MB, ADR-003 anticipated it).

  5. UNITS AND CITIES SIT ON THEIR TILES  <-- new, and the one most likely wrong
     Sprites are drawn over the windowed terrain rather than baked into it,
     converted by a single offset from the engine's view-relative coordinates.
     - At 100%, is each unit ON its tile, not beside or under it?
     - Zoom in and out: do sprites stay glued to their tiles, or drift?
     - Pan: do they travel with the terrain, or lag/slide against it?
     - Move a unit around: no smearing or trails. (If you see a trail, a sprite
       has been drawn INTO the persistent whole-map texture, which is exactly
       what this design avoids.)

  6. CLICK ACCURACY  <-- the correctness one
     Zoom to something well away from 1.0, then click tiles, especially near
     the screen corners. Selection must land on the tile under the cursor.
     The transform is round-trip unit-tested, but only you can confirm it
     matches what your eyes see.

ALSO WORTH A LOOK: zoomed out, terrain is GPU-downscaled rather than using
CTP2's hand-authored low-zoom tiles. Acceptable, or does it look wrong?

Press Ctrl-C here (or quit in-game) when done.

EOF

CIVLOG_LEVEL=debug ./build/ctp2 --resolution 1920x1080 2> "logs/worldmap-$MODE.log"
