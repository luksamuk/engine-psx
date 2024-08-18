#!/bin/env python

# chunkgen.py
# Tool for converting 128x128 chunks generated from a .tmx tilemap in Tiled
# to a proper .MAP file for the PlayStation engine-psx.
# Converts map128.csv -> MAP128.MAP
# Make sure you export your .tmx map to .csv before using this tool.

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
#    4.1. Tiles: Columns * Rows * short (16 bits per tile)
def export_binary(f, df):
    grid = get_max_grid(df)
    f.write(c_ushort(128))
    f.write(c_ushort(grid[0] * grid[1]))
    f.write(c_ushort(8))
    # Loop for each chunk
    for cy in range(0, grid[1]):
        for cx in range(0, grid[0]):
            chunk = get_chunk(df, cx, cy)
            chunk_id = (cy * grid[0]) + cx
            # print(f"Exporting tile {chunk_id}...")
            # Loop for each piece within chunk
            for py in range(0, 8):
                for px in range(0, 8):
                    f.write(c_ushort(max(chunk.iloc[py, px], 0)))


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
    df = load_data(sys.argv[1])
    # debug_print(df)
    with open(out, "wb") as f:
        export_binary(f, df)


if __name__ == "__main__":
    main()
