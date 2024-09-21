from ctypes import c_ubyte, c_short, c_ushort
from enum import Enum

c_short = c_short.__ctype_be__
c_ushort = c_ushort.__ctype_be__
# c_int = c_int.__ctype_be__
# c_uint = c_uint.__ctype_be__


class SVECTOR:
    vx: c_short
    vy: c_short
    vz: c_short

    def __repr__(self):
        return f"{{ {self.vx.value:05X}, {self.vy.value:05X}, {self.vz.value:05X} }}"


# ------------------------------------------


class FaceType(Enum):
    TRIANGLE = 0
    QUAD = 1
    LINE = 2
    SPRITE = 3


class MaterialType(Enum):
    Flat = "C"
    Gouraud = "G"
    Texture = "T"
    TextureFlat = "D"
    TextureGouraud = "H"

    # Don't care about these others:
    MATERIAL_W = "W"  # Repeating textures, no-color
    MATERIAL_S = "S"  # Repeating textures, flat colored
    MATERIAL_N = "N"  # Repeating textures, gouraud-shaded


class ShadingType(Enum):
    FLAT = "F"
    GOURAUD = "G"
