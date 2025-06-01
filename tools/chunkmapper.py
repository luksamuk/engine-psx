#!/bin/env python

# chunkmapper.py
# Generates a .tmx file from a .json tile mapping.
# This is normally used when the chunks already exist but we need to generate
# smaller tiles from it.
# Import chunks to Aseprite as a 128x128 sprite sheet first, then use
# export_tilemap_psx to export the .json that should be fed to this script.
import json
import os
import sys
import pandas as pd
import numpy as np

def load_json(filename):
    with open(filename) as fp:
        return json.load(fp)

def get_tiles(json):
    return json.get("layers")[0].get("cels")

def generate_chunk_dataframe(data):
    bounds = data.get("bounds")
    tilemap = data.get("tilemap")
    cols = tilemap.get("width")
    rows = tilemap.get("height")
    if cols < 8 or rows < 8:
        print("ERROR")
    tiles = tilemap.get("tiles")
    tiles = np.asmatrix(tiles)
    tiles = tiles.reshape(cols, rows)
    return pd.DataFrame(tiles).add(1)

def generate_map_row_dataframes(json_data):
    chunks = []
    # First chunk must also be fully-empty
    chunks.append(
        pd.DataFrame(0, index=range(8), columns=range(8)),
    )
    # Generate a dataframe for every chunk
    for t in json_data:
        chunks.append(generate_chunk_dataframe(t))
    # Ensure that the number of dataframes is divisible
    # by 4. Otherwise add a couple more dataframes.
    for i in range(4 - (len(chunks) % 4)):
        chunks.append(
            pd.DataFrame(0, index=range(8), columns=range(8)),
        )
    # Take the dataframes 4 by 4, concatenate them along the columns.
    # Form a bigger dataframe that represents a row of four chunks.
    ts = []
    for tuple in zip(*[iter(chunks)] * 4):
        ts.append(pd.concat(tuple, axis=1))
    return ts

def generate_csv_lines(dataframes):
    # Take each row of chunks, go through them row by row,
    # generate eight lines total of tiles per chunk.
    lines = []
    for df in dataframes:
        for index, row in df.iterrows():
            r = ",".join(map(str, row)) + ","
            lines.append(r)
    lines = lines[:-1] # Remove last line
    last_line = lines.pop()
    last_line = last_line[:-1] # Remove last ,
    lines.append(last_line)
    return lines

def generate_tmx(csv_lines):
    # Map properties
    version="1.10"
    tiledversion="1.11.2"
    orientation="orthogonal"
    renderorder="right-down"
    nextlayerid="2"
    nextobjectid="1"

    # Tileset properties
    firstgid="1"
    source="tiles16.tsx"

    # Layer properties
    layer_id="1"
    name="solid"
    width="32"
    height=str(len(csv_lines))

    with open("tilemap128.tmx", "w") as f:
        f.write('<?xml version="1.0" encoding="UTF-8"?>\n')
        f.write(f'<map version="{version}" tiledversion="{tiledversion}" orientation="{orientation}" renderorder="{renderorder}" width="{width}" height="{height}" tilewidth="16" tileheight="16" infinite="0" nextlayerid="{nextlayerid}" nextobjectid="{nextobjectid}">\n')
        f.write(f' <tileset firstgid="{firstgid}" source="tiles16.tsx" />\n')
        f.write(f' <layer id="{layer_id}" name="{name}" width="{width}" height="{height}">\n')
        f.write( '  <data encoding="csv">\n')
        for line in csv_lines:
            f.write(line)
            f.write('\n')
        f.write( '</data>\n')
        f.write( ' </layer>\n')
        f.write( '</map>\n')

def main():
    jsonfile = sys.argv[1]
    j = load_json(jsonfile)
    j = get_tiles(j)
    dfs = generate_map_row_dataframes(j)
    lines = generate_csv_lines(dfs)
    generate_tmx(lines)
    
if __name__ == "__main__":
    main()
