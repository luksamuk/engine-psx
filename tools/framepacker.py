#!/bin/env python3

# framepacker.py
# Tool for packing frames from an exported aseprite .json file.
# Converts YOURCHARACTER.JSON -> YOURCHARACTER.CHARA
# Use:
# Make sure you export the your tileset with the export_tileset.lua script,
# and export your tilemap after properly adjusting your sprite's canvas size
# with export_tilemap.lua.
# Also: Edit export_tilemap.lua and set row_len in such a way that your tiles
# do not exceed a width of 256.
# You should also use Gimp with a G'Mic plugin to ensure that the generated
# file is in .gif format, with a 256 colormap, so that you can generate a TIM
# texture with 4bpp.
import json
import sys
from pprint import pp as pprint
from ctypes import c_ushort, c_ubyte
import ctypes

# Set endianness of some types to big endian
c_ushort = ctypes.c_ushort.__ctype_be__


def load_json(filename):
    with open(filename) as fp:
        return json.load(fp)


# JSON layout:
# - filename -- ignored.
# - frames -- ignored. frame durations.
# - tags -- (array) animation tag data.
#   - aniDir -- ignored.
#   - from   -- start frame (base 0)
#   - color  -- ignored.
#   - to     -- end frame (inclusive)
#   - name   -- animation name
# - width  -- absolute sprite width
# - height -- absolute sprite height
# - layers -- (array)
#   - name -- layer name
#   - cels -- (array)
#     - tilemap
#       - width  -- # columns of described tiles from frame upper left corner
#       - height -- # rows of described tiles from frame upper right corner
#       - tiles  -- (array; short) tile indices (base 0; 0 is a blank tile)
#       - frame -- actual frame index
#       - bounds -- described tilemap infoex
#         - x -- left starting border relative to frame
#         - y -- upper starting border relative to frame
#         - width -- absolute described frame width
#         - height -- absolute described frame height
def parse_layout(j):
    width = j.get("width")
    height = j.get("height")
    tiles = sorted(j.get("layers")[0].get("cels"), key=lambda t: t.get("frame"))
    frames = [
        {
            "cols": t.get("tilemap").get("width"),
            "rows": t.get("tilemap").get("height"),
            "x": t.get("bounds").get("x"),
            "y": t.get("bounds").get("y"),
            "tiles": t.get("tilemap").get("tiles"),
        }
        for t in tiles
    ]
    animations = [
        {
            "name": "{:<16}".format(t.get("name").upper().replace(" ", "")),
            "start": t.get("from"),
            "end": t.get("to"),
        }
        for t in j.get("tags")
    ]

    res = {
        "sprite_width": width,
        "sprite_height": height,
        "num_frames": len(frames),
        "frames": frames,
        "num_animations": len(animations),
        "animations": animations,
    }
    return res


# Binary layout:
# --> MUST BE SAVED IN BIG ENDIAN FORMAT.
# 1. Sprite width: short (16 bits)
# 2. Sprite height: short (16 bits)
# 3. Number of frames: short (16 bits)
# 4. Number of animations: short (16 bits)
# 5. Array of frame data.
#    5.1. X offset: byte (8 bits)
#    5.2. Y offset: byte (8 bits)
#    5.3. Columns: byte (8 bits)
#    5.4. Rows: byte (8 bits)
#    5.5. Tiles: Columns * Rows * short (16 bits per tile)
# 6. Array of animation data.
#    6.1. Name: 16 bytes.
#    6.2. Frame start: byte (8 bits)
#    6.3. Frame end: byte (8 bits)
def write_binary_data(layout, f):
    f.write(c_ushort(layout.get("sprite_width")))
    f.write(c_ushort(layout.get("sprite_height")))
    f.write(c_ushort(layout.get("num_frames")))
    f.write(c_ushort(layout.get("num_animations")))
    for frame in layout.get("frames"):
        f.write(c_ubyte(frame.get("x")))
        f.write(c_ubyte(frame.get("y")))
        f.write(c_ubyte(frame.get("cols")))
        f.write(c_ubyte(frame.get("rows")))
        for t in frame.get("tiles"):
            f.write(c_ushort(t))
    for anim in layout.get("animations"):
        name = [c_ubyte(0 if x == " " else ord(x)) for x in list(anim.get("name"))]
        for c in name:
            f.write(c)
        f.write(c_ubyte(anim.get("start")))
        f.write(c_ubyte(anim.get("end")))


def main():
    jsonfile = sys.argv[1]
    outfile = sys.argv[2]
    j = load_json(jsonfile)
    l = parse_layout(j)
    with open(outfile, "wb") as f:
        write_binary_data(l, f)


if __name__ == "__main__":
    main()
