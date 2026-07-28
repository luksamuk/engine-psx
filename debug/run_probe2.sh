#!/bin/bash
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
REDUX="${REDUX:-/tmp/pcsx-extract/squashfs-root/usr/bin/pcsx-redux}"
PROBE="${2:-colprobe2}"
SECS="${1:-120}"
ADDR_HEX="$(grep 'player_debug B' "$ROOT/build/sonic.map" | awk '{print $3}' | sed 's/ffffffff//')"
PHYS_ADDR=$((16#$ADDR_HEX & 0x1fffff))
CSV="/tmp/${PROBE}.csv"
echo "player_debug phys = 0x$(printf %x $PHYS_ADDR); probe = $PROBE"
rm -f "$CSV"
sed "s/__PLAYER_DEBUG_ADDR__/$PHYS_ADDR/" "$ROOT/debug/lua/${PROBE}.lua" > "/tmp/${PROBE}_run.lua"
cd /tmp
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy timeout "$SECS" "$REDUX" \
    -run -interpreter -fastboot \
    -iso "$ROOT/build/SONICXA.cue" \
    -dofile "/tmp/${PROBE}_run.lua" > /dev/null 2>&1
echo "done: $(grep -c '^CSV' "$CSV" 2>/dev/null || echo 0) csv lines"
