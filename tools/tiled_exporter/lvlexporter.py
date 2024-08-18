# lvlexporter.py
# Level exporter for Tiled Editor.
# Go to Preferences > plugins, enable libpython.so.
# Then put this script in ~/.tiled.
from tiled import *
import json


class PSXPreLevel(Plugin):
    @classmethod
    def nameFilter(cls):
        return "PlayStation proto map (*.psxlvl)"

    @classmethod
    def shortName(cls):
        return "psxlvl"

    @classmethod
    def write(cls, tileMap, fileName):
        assert tileMap.layerCount() <= 3, f"max number of layers is 3"
        filedata = {
            "num_layers": tileMap.layerCount(),
            "_unused": 0,
            "layer_data": [],
        }
        for i in range(tileMap.layerCount()):
            if isTileLayerAt(tileMap, i):
                layer = {}
                tileLayer = tileLayerAt(tileMap, i)
                assert tileLayer.width() < 256, f"max layer width is 255"
                assert tileLayer.height() < 32, f"max layer height is 31"
                layer["width"] = tileLayer.width()
                layer["height"] = tileLayer.height()
                layer["tiles"] = []
                for y in range(tileLayer.height()):
                    for x in range(tileLayer.width()):
                        tile = tileLayer.cellAt(x, y).tile()
                        # f.write(c_ushort(tile.id() if tile else 0))
                        layer["tiles"].append(tile.id() if tile else 0)
                filedata["layer_data"].append(layer)
        with open(fileName, "w") as f:
            print(json.dumps(filedata), file=f)
        return True
