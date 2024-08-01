#!/bin/env python
# cookcollision.py
# Cook 16x16 tile collision from Tiled tile data.
# Make sure you exported a 16x16 tile with proper collision data.

import json
import sys
from ctypes import c_ushort, c_ubyte
from enum import Enum
from pprint import pp as pprint
from math import sqrt

# Ensure binary C types are encoded as big endian
c_ushort = c_ushort.__ctype_be__

# This package depends on shapely because I'm fed up with attempting to code
# point and polygon checking myself. On arch linux, install python-shapely.
from shapely.geometry import Point
from shapely.geometry.polygon import Polygon


class Direction(Enum):
    DOWN = 0
    UP = 1
    LEFT = 2
    RIGHT = 3


def point_in_geometry(p, points):
    point = Point(p[0], p[1])
    shape = Polygon(points)
    return shape.contains(point) or shape.intersects(point)


def get_height_mask(d: Direction, points):
    # Perform iterative linecast.
    # Linecast checks for a point within a geometry starting at a height
    # of 15 until 1 (inclusive). 0 means no collision at that height.
    # We do that for each X spot on our geometry.
    # Of course, if pointing downwards, we go from left to right, top to bottom.
    # If using any other direction... flip it accordingly.
    heightmask = []
    for pos in range(16):
        found = False
        for height in reversed(range(1, 16)):
            if d == Direction.DOWN:
                x = pos
                y = 16 - height
            elif d == Direction.UP:
                x = 15 - pos
                y = height
            elif d == Direction.LEFT:
                x = height
                y = pos
            elif d == Direction.RIGHT:
                x = 16 - height
                y = 15 - pos

            if point_in_geometry([x, y], points):
                found = True
                heightmask.append(height)
                break
        if not found:
            heightmask.append(0)
    return heightmask


def parse_masks(tiles):
    res = []
    for tile in tiles:
        points = tile.get("points")
        id = tile.get("id")
        res.append(
            {
                "id": tile.get("id"),
                "masks": {
                    "floor": get_height_mask(Direction.DOWN, points),
                    "ceiling": get_height_mask(Direction.UP, points),
                    "rwall": get_height_mask(Direction.RIGHT, points),
                    "lwall": get_height_mask(Direction.LEFT, points),
                },
            }
        )
    return res


def load_json(filename):
    with open(filename) as fp:
        return json.load(fp)


def parse_json(j):
    tiles = j.get("tiles")
    res = []
    for tile in tiles:
        grp = tile.get("objectgroup")
        if grp:
            objs = grp.get("objects")
            if objs:
                o = objs[0]
                id = tile.get("id")
                x = round(o.get("x"), 0)
                y = round(o.get("y"), 0)
                if o.get("polygon"):
                    vertices = o.get("polygon")
                    if len(vertices) == 3:
                        # Triangle
                        points = [
                            # xy0
                            [
                                round(vertices[0].get("x"), 0) + x,
                                round(vertices[0].get("y"), 0) + y,
                            ],
                            # xy1
                            [
                                round(vertices[1].get("x"), 0) + x,
                                round(vertices[1].get("y"), 0) + y,
                            ],
                            # xy2
                            [
                                round(vertices[2].get("x"), 0) + x,
                                round(vertices[2].get("y"), 0) + y,
                            ],
                        ]
                        res.append(
                            {
                                "id": id,
                                "points": points,
                            }
                        )
                    elif len(vertices) == 4:
                        # Degenerate quad
                        points = [
                            # xy0
                            [
                                round(vertices[0].get("x"), 0) + x,
                                round(vertices[0].get("y"), 0) + y,
                            ],
                            # xy1
                            [
                                round(vertices[1].get("x"), 0) + x,
                                round(vertices[1].get("y"), 0) + y,
                            ],
                            # xy2
                            [
                                round(vertices[2].get("x"), 0) + x,
                                round(vertices[2].get("y"), 0) + y,
                            ],
                            # xy3
                            [
                                round(vertices[3].get("x"), 0) + x,
                                round(vertices[3].get("y"), 0) + y,
                            ],
                        ]
                        res.append(
                            {
                                "id": id,
                                "points": points,
                            }
                        )
                else:
                    # Treat as quad
                    width = round(o.get("width"), 0)
                    height = round(o.get("height"), 0)
                    points = [
                        # xy0
                        [x, y],
                        # xy1
                        [x + width, y],
                        # xy3
                        [x + width, x + height],
                        # xy2
                        [x, y + height],
                    ]
                    res.append(
                        {
                            "id": id,
                            "points": points,
                        }
                    )
    print(f"Number of collidable tiles: {len(res)}")
    return res


def write_mask_data(f, mask_data):
    # Join mask data. We have 16 heights; turn them into 8 bytes
    data = []
    for i in range(0, 16, 2):
        a = (mask_data[i] & 0x0F) << 4
        b = mask_data[i + 1] & 0x0F
        data.append(c_ubyte(a | b))
    for b in data:
        f.write(b)


# Binary layout:
# 1. Number of tiles (ushort, 2 bytes)
# 2. Tile data
#   2.1. tile id (ushort, 2 bytes)
#   2.2. Floor mode data (8 bytes)
#   2.3. Right wall mode data (8 bytes)
#   2.4. Ceiling mode data (8 bytes)
#   2.5. Left wall mode data (8 bytes)
def write_file(f, tile_data):
    f.write(c_ushort(len(tile_data)))
    for tile in tile_data:
        f.write(c_ushort(tile.get("id")))
        write_mask_data(f, tile.get("masks").get("floor"))
        write_mask_data(f, tile.get("masks").get("rwall"))
        write_mask_data(f, tile.get("masks").get("ceiling"))
        write_mask_data(f, tile.get("masks").get("lwall"))


def main():
    jsonfile = sys.argv[1]
    outfile = sys.argv[2]
    j = load_json(jsonfile)
    parsed = parse_json(j)
    masks = parse_masks(parsed)
    # pprint(list(filter(lambda x: x.get("id") == 1, masks))[0])
    with open(outfile, "wb") as f:
        write_file(f, masks)


if __name__ == "__main__":
    main()
