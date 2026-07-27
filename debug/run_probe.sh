#!/bin/bash
# Runs the collision probe in PCSX-Redux (headless, fast).
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
REDUX="${REDUX:-/tmp/pcsx-extract/squashfs-root/usr/bin/pcsx-redux}"
MAP="$ROOT/build/sonic.map"
CSV=/tmp/collision_probe.csv
SECS="${1:-150}"

ADDR_HEX="$(grep 'player_debug B' "$MAP" | awk '{print $3}' | sed 's/ffffffff//')"
[ -z "$ADDR_HEX" ] && { echo "player_debug not in map" >&2; exit 1; }
PHYS_ADDR=$((16#$ADDR_HEX & 0x1fffff))
printf 'player_debug @ 0x%s (phys 0x%x)
' "$ADDR_HEX" "$PHYS_ADDR"

LUA_RUN=/tmp/collision_probe_run.lua
sed "s/__PLAYER_DEBUG_ADDR__/$PHYS_ADDR/" \
    "$ROOT/debug/lua/collision_probe.lua" > "$LUA_RUN"

rm -f "$CSV" /tmp/probe_stdout.log
# -stdout/-lua_stdout throttle emulation to ~3fps on this setup; avoid them.
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout "$SECS" "$REDUX" \
    -run -interpreter -fastboot \
    -iso "$ROOT/build/SONICXA.cue" \
    -dofile "$LUA_RUN" > /tmp/probe_stdout.log 2>&1
echo "done: $(grep -c '^CSV' "$CSV" 2>/dev/null || echo 0) CSV frames"
