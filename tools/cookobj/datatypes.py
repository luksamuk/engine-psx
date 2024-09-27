from dataclasses import dataclass, field
import typing
from ctypes import c_ubyte, c_byte, c_short, c_ushort, c_int

# from enum import Enum

c_short = c_short.__ctype_be__
c_ushort = c_ushort.__ctype_be__
c_int = c_int.__ctype_be__


# OBJECT TABLE DEFINITION (.OTN) LAYOUT
# - is_level_specific (u8)
# - num_classes (u16)
# - Classes of Objects:
#   - id (u8)  {types are sequential but id is used for auto-suficient parsing}
#   - has_fragment (u8)
#   - num_animations (u16)
#   - Animations:
#     - num_frames (u16)
#     - loopback_frame (s8)
#     - Frames:
#       - u0 (u8)
#       - v0 (u8)
#       - width (u8)
#       - height (u8)
#       - flipmask (u8)
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
        f.write(c_ubyte(self.u0))
        f.write(c_ubyte(self.v0))
        f.write(c_ubyte(self.width))
        f.write(c_ubyte(self.height))
        f.write(c_ubyte(flipmask))


@dataclass
class ObjectAnimation:
    frames: [Frame] = field(default_factory=list)
    loopback: int = 0

    def write_to(self, f):
        f.write(c_ushort(len(self.frames)))
        f.write(c_byte(self.loopback))
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
    name: str = ""
    animations: [ObjectAnimation] = field(default_factory=list)
    fragment: MaybeObjectFragment = None

    def write_to(self, f):
        f.write(c_ubyte(self.id))
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
    out: str = ""
    firstgid: int = 0
    num_objs: int = 0
    object_types: typing.Dict[int, ObjectData] = field(default_factory=dict)

    def write(self):
        with open(self.out, "wb") as f:
            self.write_to(f)

    def write_to(self, f):
        f.write(c_ubyte(int(self.is_level_specific)))
        f.write(c_ushort(self.num_objs))
        for key, t in self.object_types.items():
            print(f"Writing object class id {t.id} ({t.name})...")
            t.write_to(f)


# OBJECT MAP PLACEMENT (.OMP) LAYOUT
# - Type / ID (u8)
# - Flip Mask (u8)
# - vx (s32)
# - vy (s32)
# - Properties (exists depending on Type)
#   * Properties layout for monitor (id = 1):
#     - kind (u8)


# Root for the .OMP datatype
@dataclass
class ObjectPlacement:
    otype: int = 0
    x: int = 0
    y: int = 0
    flipx: bool = False
    flipy: bool = False
    rotcw: bool = False
    rotct: bool = False
