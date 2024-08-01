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

c_ushort = c_ushort.__ctype_be__


class Direction(Enum):
    DOWN = 0
    UP = 1
    LEFT = 2
    RIGHT = 3


def sign(p1, p2, p3):
    return ((p1[0] - p3[0]) * (p2[1] - p3[1])) - ((p2[0] - p3[0]) * (p1[1] - p3[1]))


def point_in_triangle(p, v1, v2, v3):
    d1 = sign(p, v1, v2)
    d2 = sign(p, v2, v3)
    d3 = sign(p, v3, v1)
    has_neg = (d1 < 0) or (d2 < 0) or (d3 < 0)
    has_pos = (d1 > 0) or (d2 > 0) or (d3 > 0)
    return not (has_neg and has_pos


def point_in_square(p, v1, v2, v3, v4):
    d1 = sign(p, v1, v2)
    d2 = sign(p, v2, v3)
    d3 = sign(p, v3, v4)
    d4 = sign(p, v4, v1)
    has_neg = (d1 < 0) or (d2 < 0) or (d3 < 0) or (d4 < 0)
    has_pos = (d1 > 0) or (d2 > 0) or (d3 > 0) or (d4 > 0)
    return not (has_neg and has_pos)


def point_in_geometry(p, geometry, points):
    if geometry == "quad":
        return point_in_square(p, points[0], points[1], points[2], points[3])
    return point_in_triangle(p, points[0], points[1], points[2])


def get_height_mask(d: Direction, geometry, points):
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

            if point_in_geometry([x, y], geometry, points):
                found = True
                heightmask.append(height)
                break
        if not found:
            heightmask.append(0)
    return heightmask


def parse_masks(tiles):
    res = []
    for tile in tiles:
        geometry = tile.get("type")
        points = tile.get("points")
        id = tile.get("id")
        res.append(
            {
                "id": tile.get("id"),
                "masks": {
                    "floor": get_height_mask(Direction.DOWN, geometry, points),
                    "ceiling": get_height_mask(Direction.UP, geometry, points),
                    "rwall": get_height_mask(Direction.RIGHT, geometry, points),
                    "lwall": get_height_mask(Direction.LEFT, geometry, points),
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
                                "type": "triangle",
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
                                "type": "quad",
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
                        # xy2
                        [x, y + height],
                        # xy3
                        [x + width, x + height],
                    ]
                    res.append(
                        {
                            "id": id,
                            "type": "quad",
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
    pprint(list(filter(lambda x: x.get("id") == 7, masks))[0])
    with open(outfile, "wb") as f:
        write_file(f, masks)


if __name__ == "__main__":
    main()
