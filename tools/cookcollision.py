#!/bin/env python
# cookcollision.py
# Cook 16x16 tile collision from Tiled tile data.
# Make sure you exported a 16x16 tile with proper collision data.

import json
import sys
import numpy as np
import math
from ctypes import c_ushort, c_ubyte, c_int32
from enum import Enum
from pprint import pp as pprint
from math import sqrt

# Ensure binary C types are encoded as big endian
c_ushort = c_ushort.__ctype_be__
c_int32 = c_int32.__ctype_be__

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


def normalize(v):
    norm = np.linalg.norm(v)
    return [c / norm for c in v]


# def fix_angle(x):
#     # This ensures that an angle in radians is always on their
#     # 1st or 4th quadrant equivalent, and also on the first lap.
#     fixed = x
#     if (x >= (np.pi / 2)) and (x < np.pi):
#         fixed = (2 * np.pi) - (np.pi - x)
#     if (x >= np.pi) and (x < (1.5 * np.pi)):
#         fixed = x - np.pi
#     return fixed % (2 * np.pi)


def to_psx_angle(a):
    # PSX angles are given in degrees, ranged from 0.0 to 1.0 in 20.12
    # fixed-point format (therefore from 0 to 4096).
    # All we need to do is fix its quadrant and lap, convert it to a
    # ratio [0..360], then multiply it by 4096. This is how we get
    # our angle.
    # Final gsp->(xsp, ysp) conversions in-game are given as
    # {x: (gsp * cos(x) >> 12), y: (gsp * -sin(x)) >> 12}.
    # a = np.rad2deg(fix_angle(a))
    a = np.rad2deg(a)
    a = round(a, 0)
    rat = a / 360
    return math.floor(rat * 4096)


def get_height_mask(d: Direction, points):
    # Perform iterative linecast.
    # Linecast checks for a point within a geometry starting at a height
    # of 15 until 1 (inclusive). 0 means no collision at that height.
    # We do that for each X spot on our geometry.
    # Of course, if pointing downwards, we go from left to right, top to bottom.
    # If using any other direction... flip it accordingly.
    heightmask = []
    angle = 0
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

    # Build vector according to direction
    # and heightmask
    vector = [0, 0]
    dirvec = [1, 0]
    dirv = 0
    delta = heightmask[0] - heightmask[-1]
    if d == Direction.DOWN:
        # Vector points left to right
        vector = [16, delta]
        # dirvec = [1, 0]
        dirv = 0
    elif d == Direction.UP:
        # Vector points right to left
        vector = [-16, -delta]
        # dirvec = [-1, 0]
        dirv = 0
    elif d == Direction.LEFT:
        # Vector points top to bottom
        vector = [-delta, 16]
        # dirvec = [0, 1]
        dirv = 1
    elif d == Direction.RIGHT:
        # Vector points bottom to top
        vector = [delta, -16]
        # dirvec = [0, -1]
        dirv = 1

    vector = normalize(vector)
    angle = math.atan2(dirvec[1], dirvec[0]) - math.atan2(vector[1], vector[0])
    # Angles are always converted to degrees and rounded
    # to zero decimals
    angle = to_psx_angle(angle)
    return (heightmask, angle)


def parse_masks(tiles):
    res = []
    for tile in tiles:
        points = tile.get("points")
        id = tile.get("id")
        (floor, floor_angle) = get_height_mask(Direction.DOWN, points)
        (ceil, ceil_angle) = get_height_mask(Direction.UP, points)
        (rwall, rwall_angle) = get_height_mask(Direction.RIGHT, points)
        (lwall, lwall_angle) = get_height_mask(Direction.LEFT, points)

        res.append(
            {
                "id": tile.get("id"),
                "masks": {
                    "floor": [floor_angle, floor],
                    "ceiling": [ceil_angle, ceil],
                    "rwall": [rwall_angle, rwall],
                    "lwall": [lwall_angle, lwall],
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
                    # Could be a triangle or a quad,
                    # but could also be anything, in fact.
                    vertices = o.get("polygon")
                    points = []
                    for vertex in vertices:
                        points.append(
                            [round(vertex.get("x") + x), round(vertex.get("y") + y)]
                        )
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
    # print(f"Number of collidable tiles: {len(res)}")
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
# 2. Tile data [many]
#   2.1. tile id (ushort, 2 bytes)
#   2.2. Floor
#        2.2.1. Angle (4 bytes - PSX format)
#        2.2.2. Data (8 bytes)
#   2.3. Right wall
#        2.3.1. Angle (4 bytes - PSX format)
#        2.3.2. Data (8 bytes)
#   2.4. Ceiling
#        2.4.1. Angle (4 bytes - PSX format)
#        2.4.2. Data (8 bytes)
#   2.5. Left wall
#        2.5.1. Angle (4 bytes - PSX format)
#        2.5.2. Data (8 bytes)
def write_file(f, tile_data):
    f.write(c_ushort(len(tile_data)))
    for tile in tile_data:
        f.write(c_ushort(tile.get("id")))
        masks = tile.get("masks")
        f.write(c_int32(masks.get("floor")[0]))
        write_mask_data(f, masks.get("floor")[1])
        f.write(c_int32(masks.get("rwall")[0]))
        write_mask_data(f, masks.get("rwall")[1])
        f.write(c_int32(masks.get("ceiling")[0]))
        write_mask_data(f, masks.get("ceiling")[1])
        f.write(c_int32(masks.get("lwall")[0]))
        write_mask_data(f, masks.get("lwall")[1])


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
