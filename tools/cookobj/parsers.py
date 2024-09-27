from bs4 import BeautifulSoup
import toml
from os.path import realpath, dirname, splitext

from datatypes import *


def _parse_animation(data) -> ObjectAnimation:
    anim = ObjectAnimation()
    anim.loopback = data["loopback"]
    for fr in data["frames"]:
        frame = Frame()
        fr_get = lambda i: (fr[i] == 1) if i < len(fr) else False
        # u0, v0, width, height, flipx, flipy
        frame.u0 = fr[0]
        frame.v0 = fr[1]
        frame.width = fr[2]
        frame.height = fr[3]
        frame.flipx = fr_get(4)
        frame.flipy = fr_get(5)
        anim.frames.append(frame)
    return anim


def parse_tileset(firstgid: int, set_src: str) -> (ObjectMap, str):
    toml_src = splitext(set_src)[0] + ".toml"
    ts = None
    extra_data = None
    o = ObjectMap()

    # Load tileset and extra .toml file data
    with open(set_src) as f:
        ts = BeautifulSoup(f, "xml")
    extra_data = toml.load(toml_src)

    ts = ts.find("tileset")

    tiles = ts.find_all("tile")
    o.num_objs = int(ts["tilecount"])
    # "classes" becomes an entry on dict o.object_types.
    # Emplace "od" there under a proper gid
    obj_id = 0
    for i in range(int(ts["tilecount"])):
        collision = None
        tile = next((x for x in tiles if x["id"] == f"{i}"), None)
        if tile:
            od = ObjectData()
            od.id = obj_id
            od.name = (str(tile["type"]) if tile else "none").lower()
            extra = extra_data.get(od.name)

            # If this is a dummy object (e.g. rows of rings), we don't
            # need to register it
            if extra and extra.get("dummy", False):
                o.num_objs -= 1
                continue
            # If not a dummy, increase sequential id for next object
            obj_id += 1

            # Get tile collision
            # collisions = tile.find("objectgroup")
            # if collisions:
            #     collisions = collisions.find_all("object", [])
            #     if collisions[0].get("width"):
            #         collision = {}
            #         collision["type"] = "rect"
            #         collision["x"] = int(collisions[0].get("x"))
            #         collision["y"] = int(collisions[0].get("y"))
            #         collision["width"] = int(collisions[0].get("width"))
            #         collision["height"] = int(collisions[0].get("height"))
            #     else:
            #         collision = {}
            #         collision["type"] = "polygon"
            #         poly = collisions[0].find("polygon")
            #         points = poly.get("points").split()
            #         points = [
            #             [int(float(p.split(",")[0])), int(float(p.split(",")[1]))]
            #             for p in points
            #         ]
            #         collision["points"] = points
            # Get other tile data

            idx = i + firstgid

            # Append TOML data
            if extra:
                animations = extra["animations"]
                animations.sort(key=lambda x: x.get("id"))
                for data in animations:
                    od.animations.append(_parse_animation(data))
                frag = extra.get("fragment")
                if frag:
                    od.fragment = ObjectFragment()
                    offset = frag["offset"]
                    od.fragment.offsetx = offset[0]
                    od.fragment.offsety = offset[1]
                    frag_animations = frag["animations"]
                    frag_animations.sort(key=lambda x: x.get("id"))
                    for data in animations:
                        od.fragment.animations.append(_parse_animation(data))

            # if collision:
            #     # TODO: append collision
            #     pass

            o.object_types[idx] = od
        else:
            o.num_objs -= 1

    o.firstgid = firstgid
    o.out = splitext(set_src)[0] + ".OTD"
    o.is_level_specific = ts["name"] != "objects_common"
    o.num_objs = len(o.object_types)
    return (o, ts["name"])


def parse_map(map_src: str) -> (typing.Dict[str, ObjectMap], [ObjectPlacement]):
    map = None
    with open(map_src) as f:
        map = BeautifulSoup(f, "xml")
    objmaps = {}
    placements = []

    # Get all tilesets that are not 128x128.
    # Depends on tileset name.
    # TODO: Perharps use a non-zero firstgid as parameter?
    tilesets = [t for t in map.find_all("tileset") if t["source"].find("128") == -1]

    for tileset in tilesets:
        tileset_src = realpath(dirname(map_src) + "/" + tileset["source"])
        loaded_set, ts_name = parse_tileset(int(tileset["firstgid"]), tileset_src)
        objmaps[ts_name] = loaded_set

    # Retrieve objects placement

    return (objmaps, placements)
