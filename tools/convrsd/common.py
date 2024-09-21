from ctypes import c_ubyte, c_short, c_ushort, c_int, c_uint
from enum import Enum

c_short = c_short.__ctype_be__
c_ushort = c_ushort.__ctype_be__
c_int = c_int.__ctype_be__
c_uint = c_uint.__ctype_be__


class VECTOR:
    vx: c_int
    vy: c_int
    vz: c_int

    def __repr__(self):
        return f"{{ {self.vx.value:09X}, {self.vy.value:09X}, {self.vz.value:09X} }}"


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
