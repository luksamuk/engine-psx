# chunkexporter.py
# Chunk map exporter for Tiled Editor.
# Basically a custom CSV exporter.
# Go to Preferences > plugins, enable libpython.so.
# Then put this script in ~/.tiled.
from tiled import *
import os


class PSXChunk(Plugin):
    @classmethod
    def nameFilter(cls):
        return "PlayStation chunk proto map (*.psxcsv)"

    @classmethod
    def shortName(cls):
        return "psxcsv"

    @classmethod
    def write(cls, tileMap, fileName):
        for ilayer in range(tileMap.layerCount()):
            if isTileLayerAt(tileMap, ilayer):
                tileLayer = tileLayerAt(tileMap, ilayer)
                actual_filename = fileName
                if tileMap.layerCount() > 1:
                    actual_filename = f"{os.path.splitext(fileName)[0]}_{tileLayer.name().lower()}.psxcsv"
                with open(actual_filename, "w") as f:
                    width = tileLayer.width()
                    if width % 8 > 0:
                        width += 1
                    height = tileLayer.height()
                    if height % 8 > 0:
                        height += 1
                    for y in range(height):
                        tiles = []
                        for x in range(width):
                            if tileLayer.cellAt(x, y).tile() != None:
                                tiles.append(str(tileLayer.cellAt(x, y).tile().id()))
                            else:
                                tiles.append(str(-1))
                        line = ",".join(tiles)
                        print(line, file=f)
        return True
