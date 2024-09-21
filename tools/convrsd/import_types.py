from dataclasses import dataclass, field

from common import *
from face import *
from material import *


@dataclass
class PlyData:
    magic: str = ""
    num_vertices: c_ushort = c_ushort(0)
    num_normals: c_ushort = c_ushort(0)
    numfaces: c_ushort = c_ushort(0)
    vertices: [VECTOR] = field(default_factory=list)
    normals: [VECTOR] = field(default_factory=list)
    faces: [Face] = field(default_factory=list)

    def pair_materials_and_faces(self, materials) -> (Face, Material):
        pairs = []
        for material in materials:
            # Use type(face) to determine if triangle or quad
            # Use type(material.info) to determine if F, G, FT, GT
            pairs.append((self.faces[material.ipolygon.value], material))
        return pairs


@dataclass
class MatData:
    magic: str = ""
    num_items: c_ushort = c_ushort(0)
    materials: [Material] = field(default_factory=list)


class RSDModel:
    magic: str = ""
    ply: PlyData | None = None
    mat: MatData | None = None
