---
name: sonic-collision-debug
description: Collision system knowledge for the Sonic XA PS1 engine (PSn00bSDK, C/MIPS). 360° ground/ceiling/wall modes, linecast sensor internals, height-mask data model, known bug patterns, and the PCSX-Redux Lua debug tooling (telemetry probing, scripted input, emulator workarounds). Use when working on player/enemy collision, slope physics, loops, sensor behavior, collision tile tooling, or reproducing/debugging physics bugs in the emulator.
---

# Sonic XA — Collision & Debugging Knowledge Base

Engine: C (PSn00bSDK), MIPS, PS1. Physics reference: Sonic Physics Guide (SPG) from Sonic Retro; local transcription at `~/Documentos/sonic-physics-guide/`.

## 1. Core conventions

- **Angles are 12-bit** (0x0000..0x1000 = 0..360°), counter-clockwise, 0 = +X right (PSX `rsin`/`rcos` convention; `rsin(0x400)=1` at 90°). Byte-angle from the classics = ×16 to get engine units.
- **Positions/speeds are 20.12 fixed point** (`<<12` per pixel). **Sensors work at whole-pixel anchors** — the SPG explicitly says collision happens disregarding subpixels. Only motion uses the fraction.
- Slopes: `xsp = gsp*cos(angle)>>12`, `ysp = gsp*(-sin(angle))>>12`. A "/" hill ascending right = +45°; "\" descending right = 315°; ceiling travel moving left = 180°; descending left wall = 270°.

## 2. Collision data pipeline (tools/)

- `tiles16.tsx` (Tiled) → `tiled --export-tileset` → `collision16.json` → `tools/cookcollision.py` → `MAP16.COL`.
- Cookcollision computes **height masks for 4 directions** (floor/rwall/ceil/lwall), each 16 nibbles (values 0..15!). Semantics per direction (verified live):
  - `floor[c]`   = fill height from bottom; contact face = `base + (16 - h)` (first filled row from top).
  - `rwall[15-r]` = `16 - x_left`         ; contact face = `base + (16 - h)` (first filled col from left).
  - `ceil[15-c]`  = lowest filled row index ; contact face = `base + h`.
  - `lwall[r]`    = rightmost filled col    ; contact face = `base + h`.
  - `props` per 128 chunk: SOLID 0, ONEWAY 1 (down-sensors only), NONE 2, FRONT 4.
- **Masks cap at 15** (nibble) → uniform 1px sink in fully-solid tiles. Consistent everywhere; changing it needs an asset recook.
- Tiles may carry predefined angles (`floor_angle`/`ceil_angle`/`rwall_angle`/`lwall_angle` properties, hex). **The auto estimator is unreliable on steep/curved tiles** (it takes a border-to-border vector; errors of ±35-70° vs hand values on >45° faces). Hand-authored angles win. Env var `COOK_NOPREDEF=1` regenerates ignoring them (useful for A/B tests).
- Angle range checks when hand-editing: mode boundaries must match §4 or you'll get walls treated as floors.

## 3. `linecast()` invariants (src/collision.c)

- Anchor (vx,vy) is a pixel; `magnitude` extends to `anchor+mag` (tip); scan walks **backwards from tip to anchor in 16px steps**; last accepted hit (nearest to anchor) wins. Each 16px step = one tile evaluated (hpos from low bits).
- Clamps in `_move_point_linecast` must never let samples pass behind the anchor (a past bug here broke every horizontal cast > 16px).
- `floor_direction` param: casts with `direction == floor_direction` accept any h>0; other casts require `h >= tip_height` (surface covers the sample).
- ONEWAY props are skipped when `direction != floor_direction` — pass the *ground* direction there (player passes gsmode; enemies pass CDIR_FLOOR).
- Returned `coord` is an absolute world coordinate of the contact face along the cast axis (Y for floor/ceiling casts, X for wall casts).

## 4. The 360° mode system (src/player.c)

Modes come **only from ground angle while grounded**. Airborne sensors NEVER rotate (always floor mode) — violating this caused the "insta teleport up" bug (a wall-mode X coord got snapped into pos.vy).

- Ground modes (SPG-exact): FLOOR angle≤0x1F0(44°) or ≥0xE10(316°); RWALL 0x200..0x600; CEILING 0x610..0x9F0; LWALL 0xA00..0xE00.
- Push modes (wider walls): RWALL 0x1B0..0x5B0, CEILING 0x5B1..0xA4F, LWALL 0xA50..0xE50.
- **Push sensors fire only when `angle % 0x400 == 0`** (S3K rule: flat ground, vertical walls, ceiling — never on slopes). This is load-bearing: a floor-mode push sensor on a diagonal ramp face registers it as a "wall bump" and kills all speed (the Eggmanland left-ramp "invisible wall" bug).
- Sensors radiate from the player anchor (center); lateral A/B offsets ±(9,8) rotate per mode exactly (floor anchors rotated 0/90/180/270°).
- `TERRAIN_SNAP_RADIUS` 16 = body edge offset used when snapping; casts use reach `SNAP+16` and **distance acceptance decides**:
  - grounded ground sensors: dist ∈ [−14, min(4+|gsp_px|, 14)];
  - airborne landing: dist ∈ [−(ysp_px+8), 0], and vy must be ≥ 0;
  - ceiling: dist ∈ [−SNAP, 0] only — a face below the center is NOT a ceiling (previously caused Sonic being pushed down inside walls when jumping into them fast).
  - push sensors: embedded only (face within ±10 of anchor axis).
- Winner of the two ground/ceiling sensors: **lesser signed distance**, sensor A(1) on ties.
- Landing speed transfer per SPG (flat/slope/steep angle ranges). Steep ceilings while airborne can be landed on (unless |vx|>|vy|).
- Slip/fall: <2.5px/f on >35° slopes: ctrllock=30, gsp∓0.5; detach above 69°. You cannot walk up steep walls slowly — by design. You need >2.5px/f momentum.
- Monitors/objects may pre-mark `ev_*` events (coords are contact-face absolute) — monitor right face is `solidity_vx + 30`.

## 5. Bug patterns diagnosed (symptom → root cause)

- "Teleporta pro alto ao soltar da parede": mode resolution used angle≠0 while airborne → wall-direction cast hit X face; airborne landing wrote that X coord into pos.vy.
- "Parece bater numa parede invisível na rampa": push sensor firing on non-cardinal slope angles (see §4 gate).
- "Entra no chão/parede em velocidade alta ao pular contra parede": ceiling acceptance had no lower bound; bottom row of solid tiles at torso level counted as ceilings, snapping him down into terrain.
- Casts horizontais aleatórios/pulos de tile: clamps invertidos em `_move_point_linecast` para RWALL/LWALL (>16px de mag).
- Bordas erradas em LWALL/CEILING snaps: `_get_new_position` retornava a face oposta do tile (código compensava com +25/+32 na unha — removido).
- Leitura de máscara trocada nas emendas: `_get_height_position` usava `16-x` onde o cooker grava `15-x` (índice dava a volta no px 0).

## 6. Debug tooling (use it!)

- **`PlayerDebugState player_debug`** (player.h) — fixed-offset telemetry struct, updated at end of player_update (pos/vel/angle/modes/flags/cam/raw+final sensor coords/events). Symbol comes from `build/sonic.map` (`grep 'player_debug B' build/sonic.map`, phys = addr & 0x1fffff).
- **`TEST_WARP_LEVEL`** CMake define (-DTEST_WARP_LEVEL=17, -DTEST_WARP_CHARACTER, -DTEST_WARP_DEBUG_MODE=2): boots straight into a level with debug HUD+sensors. Define lives in CMakeLists; `cmake -DTEST_WARP_LEVEL=17 . && make sonic && make iso`.
- **Lua probes** in `debug/lua/` + `debug/run_probe2.sh <secs> <probe_name>`: per-vsync CSV of telemetry + scripted pad input with gated state machines (spindash: rev while stopped only — vz must be exactly 0 and direction NOT held; release DOWN before holding direction; wait ~1.5s stopped before reving again — don't trust zero-crossing).
- PCSX-Redux Lua notes: LuaJIT 2.1 = Lua 5.1 semantics: **no `&`, `<<`, `>>` operators**. Use `io.open` for logs; printing every frame to the console crash-logs this build.
- Attaching gdb: `-gdb -gdb-port 33333`, `gdb -ex "target remote ..." -ex "info registers pc ra"`; map PCs with the map-file awk/python helper (map is `name T addr size\tfile:line`).
- Emulator flakiness (why "restart fixes it"): **the AppImage wedges on FUSE** (`fuse_dev_do_read`, 0% CPU zombie). Extract once with `--appimage-extract` (already at `/tmp/pcsx-extract/squashfs-root/usr/bin/pcsx-redux`) — also survives `-Dtestmode` aborts. **`-stdout`/`-lua_stdout` throttle emulation to ~3fps** here; avoid both, log via `io.open` in Lua instead.
- `SDL_AUDIODRIVER/VIDEODRIVER=dummy` is NOT respected: windows still open. Real speed check: GPU::Vsync listeners fire even when the game is stalled — count game-side state (pos/frames), not listener ticks, to judge liveness.
- Eggman Land 2 (R8/Z2) is the 360 testbed: inner loop (clockwise+ccw runs) + big hollow box (jump-into-wall tests). Startpos = object gid 97 in Z2.tmx (center = x+32,y; engine spawns at vy−8). Level select indexes: Test=0..3, GH=4-5, SW=6-7, DC=8-9, AO=10-11, R6=12-13, R7=14-15, **EGGMAN=16-18**, WI=19.

## 7. Quick recipes

- Rebuild warp test ISO: `cd build && cmake -DTEST_WARP_LEVEL=17 -DTEST_WARP_DEBUG_MODE=2 . && make sonic && cd .. && make iso`
- Run probe: `debug/run_probe2.sh 200 colprobe3` → analyze `/tmp/colprobe3.csv` (columns documented in the script).
- Re-cook collision ignoring hand angles: `COOK_NOPREDEF=1 python3 tools/cookcollision.py assets/levels/RX/collision16.json /tmp/out.COL` (regenerate collision16.json with `tiled --export-tileset tiles16.tsx collision16.json` first).
- Render collision raster of a level: parse MAP16.COL + MAP128.MAP (3-byte frames) + Z?.LVL (w,h u8, u16 BE chunks) — python snippets from this session are in the git history/session notes.

## 8. Remaining caveats / TODOs

- 15px mask cap (uniform 1px sink) — cosmetic, asset-level change if ever fixed.
- Auto angle estimator weak on steep tiles; improve with largest-monotone-section vector if regenerated often.
- No terminal velocity cap on vy: falls > ~16px/frame can tunnel through 16px floors (classic-equivalent; acceptance band `-14` grounded, `-ysp-8` airborne mitigates most cases).
- Push sensor FLOOR anchors are hand-tuned (y−8, +8 on flat ground) — matches this engine's level design tastes, not exactly SPG (SPG: y+8 when angle==0, y+0 otherwise).
