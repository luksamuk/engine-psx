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
from ctypes import c_ushort, c_ubyte

# Set endianness of some types to big endian
c_ushort = c_ushort.__ctype_be__


# Global variables
jsonfile = ""
outfile = ""
is_level = False


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
            "width": t.get("bounds").get("width"),
            "height": t.get("bounds").get("height"),
            "x": t.get("bounds").get("x"),
            "y": t.get("bounds").get("y"),
            "tiles": t.get("tilemap").get("tiles"),
        }
        for t in tiles
    ]
    tags = j.get("tags")
    animations = (
        [
            {
                "name": "{:<16}".format(t.get("name").upper().replace(" ", "")),
                "start": t.get("from"),
                "end": t.get("to"),
            }
            for t in tags
        ]
        if tags
        else []
    )

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
#    5.5. Width: short (16 bits)
#    5.6. Height: short (16 bits)
#    5.7. Tiles: Columns * Rows * short (16 bits per tile)
# 6. Array of animation data.
#    6.1. Name: 16 bytes.
#    6.2. Frame start: byte (8 bits)
#    6.3. Frame end: byte (8 bits)
def write_binary_data_sprite(layout, f):
    f.write(c_ushort(layout.get("sprite_width")))
    f.write(c_ushort(layout.get("sprite_height")))
    f.write(c_ushort(layout.get("num_frames")))
    f.write(c_ushort(layout.get("num_animations")))
    for frame in layout.get("frames"):
        f.write(c_ubyte(frame.get("x")))
        f.write(c_ubyte(frame.get("y")))
        f.write(c_ubyte(frame.get("cols")))
        f.write(c_ubyte(frame.get("rows")))
        f.write(c_ushort(frame.get("width")))
        f.write(c_ushort(frame.get("height")))
        for t in frame.get("tiles"):
            f.write(c_ushort(t))
    for anim in layout.get("animations"):
        name = [c_ubyte(0 if x == " " else ord(x)) for x in list(anim.get("name"))]
        for c in name:
            f.write(c)
        f.write(c_ubyte(anim.get("start")))
        f.write(c_ubyte(anim.get("end")))


# Binary layout:
# --> MUST BE SAVED IN BIG ENDIAN FORMAT.
# 1. Tile width: short (16 bits)
# 2. Number of tiles: short (16 bits)
# 3. Frame rows / columns: short (16 bits)
# 4. Array of frame data.
#    4.1. Tiles: Columns * Rows * short (16 bits per tile)
def write_binary_data_level(layout, f):
    f.write(c_ushort(layout.get("sprite_width")))
    f.write(c_ushort(layout.get("num_frames")))
    frames = layout.get("frames")
    f.write(c_ushort(frames[0].get("cols") if frames else 0))
    for frame in layout.get("frames"):
        for t in frame.get("tiles"):
            f.write(c_ushort(t))


def main():
    global jsonfile, outfile, is_level

    # Parse command options
    i = 1
    while i < len(sys.argv):
        if sys.argv[i] == "--tilemap":
            is_level = True
        else:
            jsonfile = sys.argv[i]
            outfile = sys.argv[i + 1]
            break
        i += 1

    j = load_json(jsonfile)
    l = parse_layout(j)
    with open(outfile, "wb") as f:
        if not is_level:
            write_binary_data_sprite(l, f)
        else:
            write_binary_data_level(l, f)


if __name__ == "__main__":
    main()
