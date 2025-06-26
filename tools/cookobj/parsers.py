from bs4 import BeautifulSoup
import toml
from os.path import realpath, dirname, splitext
from pprint import pp

from datatypes import *


def _parse_animation(data) -> ObjectAnimation:
    anim = ObjectAnimation()
    anim.loopback = data["loopback"]
    anim.duration = data["duration"]
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

    o.is_level_specific = ts["name"] != "objects_common"

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
            gid = int(tile["id"]) + firstgid
            extra = extra_data.get(od.name)

            # If this is a dummy object (e.g. rows of rings), we don't
            # need to register it
            # TODO: These dummy objects will be needed somewhere else!
            # We actually need to register them, yes! But somewhere
            # else.
            if extra and extra.get("dummy", False):
                o.num_objs -= 1
                o.obj_mapping[gid] = DummyObjectId.get(od.name).value
                continue

            # If this isn't a dummy object, increase object ID.
            # ID's are sequential only for non-dummy objects.
            obj_id += 1

            o.obj_mapping[gid] = (
                (gid - firstgid) if o.is_level_specific else ObjectId.get(od.name).value
            )

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
                    for data in frag_animations:
                        od.fragment.animations.append(_parse_animation(data))

            o.object_types[gid] = od
        else:
            o.num_objs -= 1

    o.firstgid = firstgid
    o.out = splitext(set_src)[0] + ".OTD"
    o.num_objs = len(o.object_types)
    o.name = ts["name"]
    return (o, ts["name"])


def parse_object_group(
    tilesets: typing.Dict[str, ObjectMap], objgroup
) -> [ObjectPlacement]:
    is_level_specific = False
    current_ts = None
    placements = []

    objects = objgroup.find_all("object", [])
    if not objects:
        return []

    # Get first object's gid.
    first_obj = sorted(objects, key=lambda x: int(x.get("gid")))[0]
    first_obj_gid = int(first_obj.get("gid"))

    # Identify if this is from common objects tileset or from level-specific tileset.
    for key, ts in tilesets.items():
        result = ts.get_is_specific_if_from_this_map(first_obj_gid)
        if result is not None:
            current_ts = ts
            is_level_specific = result

    # If the tileset was not found... DON'T GO BEYOND THIS POINT!
    assert current_ts is not None, "Object was not found in any tilesets!"

    # Iterate over placements
    for obj in objects:
        p = ObjectPlacement()
        p.is_level_specific = is_level_specific
        gid = int(obj.get("gid"))
        p.otype = current_ts.get_otype_from_gid(gid)
        p.x = int(float(obj.get("x")))
        p.y = int(float(obj.get("y")))
        p.flipx = bool(gid & (1 << 31))
        p.flipy = bool(gid & (1 << 30))
        p.rotcw = int(float(obj.get("rotation", 0))) == 90
        p.rotct = int(float(obj.get("rotation", 0))) == -90
        props = obj.find("properties")
        if p.otype == ObjectId.MONITOR.value:
            m = MonitorProperties()
            if props:
                prop = props.find("property")
                m.kind = MonitorKind.get(prop.get("value")).value
            p.properties = m
        elif p.otype == ObjectId.BUBBLE_PATCH.value:
            bp = BubblePatchProperties()
            if props:
                prop = props.find("property")
                # Get first available value
                bp.frequency = int(prop.get("value"))
            p.properties = bp
        # print(
        #     f"Object type {current_ts.object_types[p.otype + current_ts.firstgid].name if p.otype >= 0 else 'DUMMY'}"
        # )
        # pp(p)
        placements.append(p)

    return placements


def parse_map(map_src: str) -> (typing.Dict[str, ObjectMap], ObjectLevelLayout):
    map = None
    with open(map_src) as f:
        map = BeautifulSoup(f, "xml")
    objmaps = {}
    layout = ObjectLevelLayout()
    layout.out = realpath(splitext(map_src)[0] + ".OMP")

    # Get all tilesets that are not 128x128.
    # Depends on tileset name.
    # TODO: Perharps use a non-zero firstgid as parameter?
    tilesets = [t for t in map.find_all("tileset") if t["source"].find("128") == -1]

    for tileset in tilesets:
        tileset_src = realpath(dirname(map_src) + "/" + tileset["source"])
        loaded_set, ts_name = parse_tileset(int(tileset["firstgid"]), tileset_src)
        print(f"Loaded tileset {ts_name} ({loaded_set.out})")
        objmaps[ts_name] = loaded_set

    # Retrieve objects placement
    layergroup = map.find(name="group", attrs={"name": "OBJECTS"})
    if layergroup:
        objgroups = layergroup.find_all("objectgroup")
        if objgroups:
            for objgroup in objgroups:
                layout.placements += parse_object_group(objmaps, objgroup)

    return (objmaps, layout)
