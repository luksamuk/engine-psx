#!/bin/env python
# cooklvl.py
# Converts exported Tiled levels (*.lvl) to binary PSX levels (*.LVL)
# THIS IS NOT a Tiled plugin. Use it to cook *.lvl levels only.
# If you want the plugin, you should be looking at lvlexporter.py.

import sys
import json
import ctypes
from ctypes import c_ushort, c_ubyte

c_ushort = c_ushort.__ctype_be__

# Binary layout:
# - number of layers (uint8_t, never above 3)
# - unused, alignment (uint8_t)
# - level data per layer:
#   - layer width in tiles (uint8_t, never above 256)
#   - layer height in tiles (uint8_t, never above 256)
#   - array of tiles ([]uint16_t, big endian)

# Example C structs:
# typedef struct {
#     uint8_t  width;
#     uint8_t  height;
#     uint16_t *tiles; // (big endian)
# } LayerData;
#
# typedef struct {
#     uint8_t   num_layers;
#     uint8_t   _unused;
#     LayerData *layer_data;
# } LevelData;

jsonfile = ""
outfile = ""


def load_json(filename):
    with open(filename) as fp:
        return json.load(fp)


def main():
    global jsonfile
    i = 1
    while i < len(sys.argv):
        jsonfile = sys.argv[i]
        outfile = sys.argv[i + 1]
        break
        i += 1
    j = load_json(jsonfile)

    with open(outfile, "wb") as f:
        f.write(c_ubyte(j.get("num_layers")))
        f.write(c_ubyte(j.get("_unused")))
        layer_data = j.get("layer_data")
        for layer in layer_data:
            print(layer.get("width"))
            print(layer.get("height"))
            f.write(c_ubyte(layer.get("width")))
            f.write(c_ubyte(layer.get("height")))
            for tile in layer.get("tiles"):
                f.write(c_ushort(tile))


if __name__ == "__main__":
    main()
