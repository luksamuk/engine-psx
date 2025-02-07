#!/bin/env python

# chunkgen.py
# Tool for converting 128x128 chunks generated from a .tmx tilemap in Tiled
# to a proper .MAP file for the PlayStation engine-psx.
# Converts map128.cnk -> MAP128.MAP
# Make sure you export your .tmx map to .cnk before using this tool.
import os
import sys
import pandas as pd
import numpy as np
import math
from ctypes import c_ushort, c_ubyte

# Set endianness of some types to big endian
c_ushort = c_ushort.__ctype_be__


def reshape_dimension(d):
    if d % 8 > 0:
        extra = 8 - (d - (math.floor(d / 8) * 8))
        return d + extra
    return None


def load_data(csvfile):
    df = pd.read_csv(csvfile, header=None)
    cols = df.shape[1]
    rows = df.shape[0]
    # print(f"Old shape: {df.shape}")
    new_cols = reshape_dimension(cols)
    new_rows = reshape_dimension(rows)
    if new_cols:
        while df.shape[1] < new_cols:
            df.insert(
                df.shape[1], df.shape[1], np.full(shape=df.shape[0], fill_value=-1)
            )
    if new_rows:
        while df.shape[0] < new_rows:
            df.loc[len(df)] = np.full(shape=df.shape[1], fill_value=-1)
    return df


def get_chunk(df, x, y):
    startx = x * 8
    endx = startx + 8
    starty = y * 8
    endy = starty + 8
    return df.loc[list(range(starty, endy)), list(range(startx, endx))]


def get_max_grid(df):
    return (int(df.shape[1] / 8), int(df.shape[0] / 8))


# Binary layout:
# --> MUST BE SAVED IN BIG ENDIAN FORMAT.
# 1. Tile width: short (16 bits)
# 2. Number of tiles: short (16 bits)
# 3. Frame rows / columns: short (16 bits)
# 4. Array of frame data.
#    4.1. Frames: Columns * Rows
#         4.1.1 Tilenum: short (16 bits)
#         4.1.2 Props:   byte  (8 bits)
def export_binary(f, solid, oneway, nocol, front):
    grid = get_max_grid(solid)
    f.write(c_ushort(128))
    f.write(c_ushort(grid[0] * grid[1]))
    f.write(c_ushort(8))
    # Loop for each chunk
    for cy in range(0, grid[1]):
        for cx in range(0, grid[0]):
            chunk_solid = get_chunk(solid, cx, cy)
            chunk_oneway = None
            chunk_nocol = None
            chunk_front = None
            if oneway is not None:
                chunk_oneway = get_chunk(oneway, cx, cy)
            if nocol is not None:
                chunk_nocol = get_chunk(nocol, cx, cy)
            if front is not None:
                chunk_front = get_chunk(front, cx, cy)
            # chunk_id = (cy * grid[0]) + cx
            # print(f"Exporting tile {chunk_id}...")
            # Loop for each piece within chunk
            for py in range(0, 8):
                for px in range(0, 8):
                    props = 0
                    index = chunk_solid.iloc[py, px]
                    if index <= 0 and chunk_oneway is not None:
                        index = chunk_oneway.iloc[py, px]
                        props = 1 if index > 0 else 0
                    if index <= 0 and chunk_nocol is not None:
                        index = chunk_nocol.iloc[py, px]
                        props = 2 if index > 0 else 0
                    if index <= 0 and chunk_front is not None:
                        index = chunk_front.iloc[py, px]
                        props = 4 if index > 0 else 0
                    f.write(c_ushort(max(index, 0)))
                    f.write(c_ubyte(props))


# def debug_print(df):
#     print(df)
#     grid = get_max_grid(df)
#     for y in range(0, grid[1]):
#         for x in range(0, grid[0]):
#             chunk = get_chunk(df, x, y)
#             id = (y * grid[0]) + x
#             if id > 0:
#                 print(f"Tile {id}:")
#                 print(chunk)
#                 print()


def main():
    out = sys.argv[2]
    solid_layer = None
    oneway_layer = None
    none_layer = None
    if os.path.isfile(sys.argv[1]):
        solid_layer = load_data(sys.argv[1])
    else:
        basepath = os.path.splitext(sys.argv[1])[0]
        solid_layer = load_data(f"{basepath}_solid.cnk")
        oneway_layer = load_data(f"{basepath}_oneway.cnk")
        none_layer = load_data(f"{basepath}_none.cnk")
        front_layer = load_data(f"{basepath}_front.cnk")

    with open(out, "wb") as f:
        export_binary(f, solid_layer, oneway_layer, none_layer, front_layer)


if __name__ == "__main__":
    main()
