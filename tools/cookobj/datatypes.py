from dataclasses import dataclass, field
import typing
from ctypes import c_ubyte, c_byte, c_short, c_ushort, c_int
from enum import Enum

c_short = c_short.__ctype_be__
c_ushort = c_ushort.__ctype_be__
c_int = c_int.__ctype_be__


class DummyObjectId(Enum):
    RING_3H = -1
    RING_3V = -2
    STARTPOS = -3

    @staticmethod
    def get(name):
        switch = {
            "ring_3h": DummyObjectId.RING_3H,
            "ring_3v": DummyObjectId.RING_3V,
            "startpos": DummyObjectId.STARTPOS,
        }
        result = switch.get(name.lower())
        assert result is not None, f"Unknown dummy object {name}"
        return result


class ObjectId(Enum):
    RING = 0x00
    MONITOR = 0x01
    SPIKES = 0x02
    CHECKPOINT = 0x03
    SPRING_YELLOW = 0x04
    SPRING_RED = 0x05
    SPRING_YELLOW_DIAGONAL = 0x06
    SPRING_RED_DIAGONAL = 0x07
    SWITCH = 0x08
    GOAL_SIGN = 0x09
    EXPLOSION = 0x0A
    MONITOR_IMAGE = 0x0B
    SHIELD = 0x0C
    BUBBLE_PATCH = 0x0D
    BUBBLE = 0x0E
    END_CAPSULE = 0x0F
    END_CAPSULE_BUTTON = 0x10
    DOOR = 0x11
    ANIMAL = 0x12

    @staticmethod
    def get(name):
        switch = {
            "ring": ObjectId.RING,
            "monitor": ObjectId.MONITOR,
            "spikes": ObjectId.SPIKES,
            "checkpoint": ObjectId.CHECKPOINT,
            "spring_yellow": ObjectId.SPRING_YELLOW,
            "spring_red": ObjectId.SPRING_RED,
            "spring_yellow_diagonal": ObjectId.SPRING_YELLOW_DIAGONAL,
            "spring_red_diagonal": ObjectId.SPRING_RED_DIAGONAL,
            "goal_sign": ObjectId.GOAL_SIGN,
            "switch": ObjectId.SWITCH,
            "explosion": ObjectId.EXPLOSION,
            "monitor_image": ObjectId.MONITOR_IMAGE,
            "shield": ObjectId.SHIELD,
            "bubble_patch": ObjectId.BUBBLE_PATCH,
            "bubble": ObjectId.BUBBLE,
            "end_capsule": ObjectId.END_CAPSULE,
            "end_capsule_button": ObjectId.END_CAPSULE_BUTTON,
            "door": ObjectId.DOOR,
            "animal": ObjectId.ANIMAL,
        }
        result = switch.get(name.lower())
        assert result is not None, f"Unknown common object {name}"
        return result


# OBJECT TABLE DEFINITION (.OTD) LAYOUT
# - is_level_specific (u8)
# - num_classes (u16)
# - Classes of Objects:
#   - id (u8)  {types are sequential but id is used for auto-suficient parsing}
#   - has_fragment (u8)
#   - num_animations (u16)
#   - Animations:
#     - num_frames (u16)
#     - loopback_frame (s8)
#     - duration of a frame (u8)
#     - Frames:
#       - u0 (u8)
#       - v0 (u8)
#       - width (u8)
#       - height (u8)
#       - flipmask (u8)
#       - tpage (u8 -- either 0 or 1)
#   - Fragment: (single, exists depending on Type)
#     - offsetx (s16)
#     - offsety (s16)
#     - num_animations (u16)
#     - Fragment Animations: (see Animations above)


@dataclass
class Frame:
    u0: int = 0
    v0: int = 0
    width: int = 0
    height: int = 0
    flipx: bool = False
    flipy: bool = False

    def write_to(self, f):
        flipmask = ((1 << 0) if self.flipx else 0) | ((1 << 1) if self.flipy else 0)
        real_v0 = (self.v0 % 256)
        tpage = 1 if (self.v0 >= 256) else 0
        f.write(c_ubyte(self.u0))
        f.write(c_ubyte(real_v0))
        f.write(c_ubyte(self.width))
        f.write(c_ubyte(self.height))
        f.write(c_ubyte(flipmask))
        f.write(c_ubyte(tpage))


@dataclass
class ObjectAnimation:
    frames: [Frame] = field(default_factory=list)
    loopback: int = 0
    duration: int = 0

    def write_to(self, f):
        f.write(c_ushort(len(self.frames)))
        f.write(c_byte(self.loopback))
        f.write(c_ubyte(self.duration))
        for frame in self.frames:
            frame.write_to(f)


@dataclass
class ObjectFragment:
    offsetx: int = 0
    offsety: int = 0
    animations: [ObjectAnimation] = field(default_factory=list)

    def write_to(self, f):
        f.write(c_short(self.offsetx))
        f.write(c_short(self.offsety))
        f.write(c_ushort(len(self.animations)))
        for animation in self.animations:
            animation.write_to(f)


MaybeObjectFragment = ObjectFragment | None


@dataclass
class ObjectData:
    id: int = -1
    gid: int = -1
    name: str = ""
    animations: [ObjectAnimation] = field(default_factory=list)
    fragment: MaybeObjectFragment = None

    def write_to(self, f):
        f.write(c_byte(self.id))
        f.write(c_ubyte(int(self.fragment is not None)))
        f.write(c_ushort(len(self.animations)))
        for animation in self.animations:
            animation.write_to(f)
        if self.fragment:
            self.fragment.write_to(f)


# Root for the .OTN data type
@dataclass
class ObjectMap:
    is_level_specific: bool = False
    name: str = ""
    out: str = ""
    firstgid: int = 0
    num_objs: int = 0
    object_types: typing.Dict[int, ObjectData] = field(default_factory=dict)

    # Mapping of dummy objects (gid -> actual id)
    obj_mapping: typing.Dict[int, int] = field(default_factory=dict)

    def get_otype_from_gid(self, gid: int) -> int:
        gid = gid & ~(0b1111 << 29)
        return self.obj_mapping[gid]

    def write(self):
        with open(self.out, "wb") as f:
            self.write_to(f)

    def write_to(self, f):
        f.write(c_ubyte(int(self.is_level_specific)))
        f.write(c_ushort(self.num_objs))
        for key, t in self.object_types.items():
            # print(f"Writing object class id {t.id} ({t.name})...")
            t.write_to(f)

    # I don't have a better name for this. Sorry
    def get_is_specific_if_from_this_map(self, dirty_gid: int) -> bool | None:
        # Clean GID
        gid = dirty_gid & ~(0b1111 << 29)
        if gid >= self.firstgid:
            return self.is_level_specific
        return None


# =======================================


class MonitorKind(Enum):
    NONE = 0
    RING = 1
    SPEEDSHOES = 2
    SHIELD = 3
    INVINCIBILITY = 4
    LIFE = 5
    SUPER = 6

    @staticmethod
    def get(name):
        switch = {
            "NONE": MonitorKind.NONE,
            "RING": MonitorKind.RING,
            "SPEEDSHOES": MonitorKind.SPEEDSHOES,
            "SHIELD": MonitorKind.SHIELD,
            "INVINCIBILITY": MonitorKind.INVINCIBILITY,
            "1UP": MonitorKind.LIFE,
            "SUPER": MonitorKind.SUPER,
        }
        result = switch.get(name.upper())
        assert result is not None, f"Unknown monitor kind {name}"
        return result


@dataclass
class MonitorProperties:
    kind: int = 0

    def write_to(self, f):
        f.write(c_ubyte(self.kind))


@dataclass
class BubblePatchProperties:
    frequency: int = 0

    def write_to(self, f):
        f.write(c_ubyte(self.frequency))


ObjectProperties = MonitorProperties | BubblePatchProperties | None


@dataclass
class ObjectPlacement:
    is_level_specific: bool = False
    otype: int = 0
    unique_id: int = 0
    parent_id: int = 0
    x: int = 0
    y: int = 0
    flipx: bool = False
    flipy: bool = False
    rotcw: bool = False  # clockwise rotation
    rotct: bool = False  # counterclockwise rotation
    properties: ObjectProperties = None

    def write_to(self, f):
        flipmask = (
            ((1 << 0) if self.flipx else 0)
            | ((1 << 1) if self.flipy else 0)
            | ((1 << 2) if self.rotcw else 0)
            | ((1 << 3) if self.rotct else 0)
        )

        f.write(c_ubyte(int(self.is_level_specific)))
        f.write(c_byte(self.otype))
        f.write(c_ushort(self.unique_id))
        f.write(c_ushort(self.parent_id))
        f.write(c_ubyte(flipmask))
        f.write(c_int(self.x + 32))  # Center X position
        f.write(c_int(self.y))  # Already at extreme bottom Y position
        if self.properties is not None:
            self.properties.write_to(f)


# OBJECT MAP PLACEMENT (.OMP) LAYOUT
# - num_objects (u16)
# - Array of object placements:
#   - is_level_specific (u8)
#   - Type / ID (s8)
#   - unique_id (u16) (actual object id on Tiled map)
#   - parent_id (u16) (parent reference on Tiled map)
#   - Flip Mask (u8)
#   - vx (s32)
#   - vy (s32)
#   - Properties (exists depending on Type)
#     * Properties layout for monitor (id = 1):
#       - kind (u8)


# Root for the .OMP datatype
@dataclass
class ObjectLevelLayout:
    out: str = ""
    placements: [ObjectPlacement] = field(default_factory=list)

    def write_to(self, f):
        f.write(c_ushort(len(self.placements)))
        for p in self.placements:
            # description = DummyObjectId(p.otype) if p.otype < 0 else ObjectId(p.otype)
            # if description == ObjectId.MONITOR:
            #     description = f"{description}: {MonitorKind(p.properties.kind)}"
            # print(f"Placing object at {(p.x, p.y)} => {description}...")
            p.write_to(f)

    def write(self):
        with open(self.out, "wb") as f:
            self.write_to(f)
