from dataclasses import dataclass, field
from common import SVECTOR
from enum import Enum
from face import *
from material import *
from import_types import RSDModel


class PolyType(Enum):
    F3 = c_ubyte(0)
    G3 = c_ubyte(1)
    F4 = c_ubyte(2)
    G4 = c_ubyte(3)
    FT3 = c_ubyte(4)
    GT3 = c_ubyte(5)
    FT4 = c_ubyte(6)
    GT4 = c_ubyte(7)


@dataclass
class PolyF3:
    ftype: c_ubyte = PolyType.F3
    r0: c_ubyte = c_ubyte(0)
    g0: c_ubyte = c_ubyte(0)
    b0: c_ubyte = c_ubyte(0)
    iv0: c_ushort = c_ushort(0)
    iv1: c_ushort = c_ushort(0)
    iv2: c_ushort = c_ushort(0)
    in0: c_ushort = c_ushort(0)

    @staticmethod
    def from_pair(face: TriangleFace, material: FlatMaterial) -> "PolyF3":
        p = PolyF3()
        p.r0 = material.r0
        p.g0 = material.g0
        p.b0 = material.b0
        p.iv0 = face.iv0
        p.iv1 = face.iv1
        p.iv2 = face.iv2
        p.in0 = face.in0
        return p

    def write(self, f):
        f.write(self.ftype.value)
        f.write(self.r0)
        f.write(self.g0)
        f.write(self.b0)
        f.write(self.iv0)
        f.write(self.iv1)
        f.write(self.iv2)
        f.write(self.in0)


@dataclass
class PolyG3:
    ftype: c_ubyte = PolyType.G3
    r0: c_ubyte = c_ubyte(0)
    g0: c_ubyte = c_ubyte(0)
    b0: c_ubyte = c_ubyte(0)
    r1: c_ubyte = c_ubyte(0)
    g1: c_ubyte = c_ubyte(0)
    b1: c_ubyte = c_ubyte(0)
    r2: c_ubyte = c_ubyte(0)
    g2: c_ubyte = c_ubyte(0)
    b2: c_ubyte = c_ubyte(0)
    iv0: c_ushort = c_ushort(0)
    iv1: c_ushort = c_ushort(0)
    iv2: c_ushort = c_ushort(0)
    in0: c_ushort = c_ushort(0)
    in1: c_ushort = c_ushort(0)
    in2: c_ushort = c_ushort(0)

    @staticmethod
    def from_pair(face: TriangleFace, material: GouraudMaterial) -> "PolyG3":
        p = PolyG3()
        p.r0 = material.r0
        p.g0 = material.g0
        p.b0 = material.b0
        p.r1 = material.r1
        p.g1 = material.g1
        p.b1 = material.b1
        p.r2 = material.r2
        p.g2 = material.g2
        p.b2 = material.b2
        p.iv0 = face.iv0
        p.iv1 = face.iv1
        p.iv2 = face.iv2
        p.in0 = face.in0
        p.in1 = face.in1
        p.in2 = face.in2
        return p

    def write(self, f):
        f.write(self.ftype.value)
        f.write(self.r0)
        f.write(self.g0)
        f.write(self.b0)
        f.write(self.r1)
        f.write(self.g1)
        f.write(self.b1)
        f.write(self.r2)
        f.write(self.g2)
        f.write(self.b2)
        f.write(self.iv0)
        f.write(self.iv1)
        f.write(self.iv2)
        f.write(self.in0)
        f.write(self.in1)
        f.write(self.in2)


@dataclass
class PolyF4:
    ftype: c_ubyte = PolyType.F4
    r0: c_ubyte = c_ubyte(0)
    g0: c_ubyte = c_ubyte(0)
    b0: c_ubyte = c_ubyte(0)
    iv0: c_ushort = c_ushort(0)
    iv1: c_ushort = c_ushort(0)
    iv2: c_ushort = c_ushort(0)
    iv3: c_ushort = c_ushort(0)
    in0: c_ushort = c_ushort(0)

    @staticmethod
    def from_pair(face: QuadFace, material: FlatMaterial) -> "PolyF4":
        p = PolyF4()
        p.r0 = material.r0
        p.g0 = material.g0
        p.b0 = material.b0
        p.iv0 = face.iv0
        p.iv1 = face.iv1
        p.iv2 = face.iv2
        p.iv3 = face.iv3
        p.in0 = face.in0
        return p

    def write(self, f):
        f.write(self.ftype.value)
        f.write(self.r0)
        f.write(self.g0)
        f.write(self.b0)
        f.write(self.iv0)
        f.write(self.iv1)
        f.write(self.iv2)
        f.write(self.iv3)
        f.write(self.in0)


@dataclass
class PolyG4:
    ftype: c_ubyte = PolyType.G4
    r0: c_ubyte = c_ubyte(0)
    g0: c_ubyte = c_ubyte(0)
    b0: c_ubyte = c_ubyte(0)
    r1: c_ubyte = c_ubyte(0)
    g1: c_ubyte = c_ubyte(0)
    b1: c_ubyte = c_ubyte(0)
    r2: c_ubyte = c_ubyte(0)
    g2: c_ubyte = c_ubyte(0)
    b2: c_ubyte = c_ubyte(0)
    r3: c_ubyte = c_ubyte(0)
    g3: c_ubyte = c_ubyte(0)
    b3: c_ubyte = c_ubyte(0)
    iv0: c_ushort = c_ushort(0)
    iv1: c_ushort = c_ushort(0)
    iv2: c_ushort = c_ushort(0)
    iv3: c_ushort = c_ushort(0)
    in0: c_ushort = c_ushort(0)
    in1: c_ushort = c_ushort(0)
    in2: c_ushort = c_ushort(0)
    in3: c_ushort = c_ushort(0)

    @staticmethod
    def from_pair(face: QuadFace, material: GouraudMaterial) -> "PolyG4":
        p = PolyG4()
        p.r0 = material.r0
        p.g0 = material.g0
        p.b0 = material.b0
        p.r1 = material.r1
        p.g1 = material.g1
        p.b1 = material.b1
        p.r2 = material.r2
        p.g2 = material.g2
        p.b2 = material.b2
        p.r3 = material.r3
        p.g3 = material.g3
        p.b3 = material.b3
        p.iv0 = face.iv0
        p.iv1 = face.iv1
        p.iv2 = face.iv2
        p.iv3 = face.iv3
        p.in0 = face.in0
        p.in1 = face.in1
        p.in2 = face.in2
        p.in3 = face.in3
        return p

    def write(self, f):
        f.write(self.ftype.value)
        f.write(self.r0)
        f.write(self.g0)
        f.write(self.b0)
        f.write(self.r1)
        f.write(self.g1)
        f.write(self.b1)
        f.write(self.r2)
        f.write(self.g2)
        f.write(self.b2)
        f.write(self.r3)
        f.write(self.g3)
        f.write(self.b3)
        f.write(self.iv0)
        f.write(self.iv1)
        f.write(self.iv2)
        f.write(self.iv3)
        f.write(self.in0)
        f.write(self.in1)
        f.write(self.in2)
        f.write(self.in3)


@dataclass
class PolyFT3:
    ftype: c_ubyte = PolyType.FT3
    u0: c_ubyte = c_ubyte(0)
    v0: c_ubyte = c_ubyte(0)
    u1: c_ubyte = c_ubyte(0)
    v1: c_ubyte = c_ubyte(0)
    u2: c_ubyte = c_ubyte(0)
    v2: c_ubyte = c_ubyte(0)
    r0: c_ubyte = c_ubyte(0)
    g0: c_ubyte = c_ubyte(0)
    b0: c_ubyte = c_ubyte(0)
    iv0: c_ushort = c_ushort(0)
    iv1: c_ushort = c_ushort(0)
    iv2: c_ushort = c_ushort(0)
    in0: c_ushort = c_ushort(0)

    @staticmethod
    def from_pair(face: TriangleFace, material: TextureFlatMaterial) -> "PolyFT3":
        p = PolyFT3()
        p.u0 = material.u0
        p.v0 = material.v0
        p.u1 = material.u1
        p.v1 = material.v1
        p.u2 = material.u2
        p.v2 = material.v2
        p.r0 = material.r0
        p.g0 = material.g0
        p.b0 = material.b0
        p.iv0 = face.iv0
        p.iv1 = face.iv1
        p.iv2 = face.iv2
        p.in0 = face.in0
        return p

    def write(self, f):
        f.write(self.ftype.value)
        f.write(self.u0)
        f.write(self.v0)
        f.write(self.u1)
        f.write(self.v1)
        f.write(self.u2)
        f.write(self.v2)
        f.write(self.r0)
        f.write(self.g0)
        f.write(self.b0)
        f.write(self.iv0)
        f.write(self.iv1)
        f.write(self.iv2)
        f.write(self.in0)


@dataclass
class PolyGT3:
    ftype: c_ubyte = PolyType.GT3
    u0: c_ubyte = c_ubyte(0)
    v0: c_ubyte = c_ubyte(0)
    u1: c_ubyte = c_ubyte(0)
    v1: c_ubyte = c_ubyte(0)
    u2: c_ubyte = c_ubyte(0)
    v2: c_ubyte = c_ubyte(0)
    r0: c_ubyte = c_ubyte(0)
    g0: c_ubyte = c_ubyte(0)
    b0: c_ubyte = c_ubyte(0)
    r1: c_ubyte = c_ubyte(0)
    g1: c_ubyte = c_ubyte(0)
    b1: c_ubyte = c_ubyte(0)
    r2: c_ubyte = c_ubyte(0)
    g2: c_ubyte = c_ubyte(0)
    b2: c_ubyte = c_ubyte(0)
    iv0: c_ushort = c_ushort(0)
    iv1: c_ushort = c_ushort(0)
    iv2: c_ushort = c_ushort(0)
    in0: c_ushort = c_ushort(0)
    in1: c_ushort = c_ushort(0)
    in2: c_ushort = c_ushort(0)

    @staticmethod
    def from_pair(face: TriangleFace, material: TextureGouraudMaterial) -> "PolyGT3":
        p = PolyGT3()
        p.u0 = material.u0
        p.v0 = material.v0
        p.u1 = material.u1
        p.v1 = material.v1
        p.u2 = material.u2
        p.v2 = material.v2
        p.r0 = material.r0
        p.g0 = material.g0
        p.b0 = material.b0
        p.r1 = material.r1
        p.g1 = material.g1
        p.b1 = material.b1
        p.r2 = material.r2
        p.g2 = material.g2
        p.b2 = material.b2
        p.iv0 = face.iv0
        p.iv1 = face.iv1
        p.iv2 = face.iv2
        p.in0 = face.in0
        p.in1 = face.in1
        p.in2 = face.in2
        return p

    def write(self, f):
        f.write(self.ftype.value)
        f.write(self.u0)
        f.write(self.v0)
        f.write(self.u1)
        f.write(self.v1)
        f.write(self.u2)
        f.write(self.v2)
        f.write(self.r0)
        f.write(self.g0)
        f.write(self.b0)
        f.write(self.r1)
        f.write(self.g1)
        f.write(self.b1)
        f.write(self.r2)
        f.write(self.g2)
        f.write(self.b2)
        f.write(self.iv0)
        f.write(self.iv1)
        f.write(self.iv2)
        f.write(self.in0)
        f.write(self.in1)
        f.write(self.in2)


@dataclass
class PolyFT4:
    ftype: c_ubyte = PolyType.FT4
    u0: c_ubyte = c_ubyte(0)
    v0: c_ubyte = c_ubyte(0)
    u1: c_ubyte = c_ubyte(0)
    v1: c_ubyte = c_ubyte(0)
    u2: c_ubyte = c_ubyte(0)
    v2: c_ubyte = c_ubyte(0)
    u3: c_ubyte = c_ubyte(0)
    v3: c_ubyte = c_ubyte(0)
    r0: c_ubyte = c_ubyte(0)
    g0: c_ubyte = c_ubyte(0)
    b0: c_ubyte = c_ubyte(0)
    iv0: c_ushort = c_ushort(0)
    iv1: c_ushort = c_ushort(0)
    iv2: c_ushort = c_ushort(0)
    iv3: c_ushort = c_ushort(0)
    in0: c_ushort = c_ushort(0)

    @staticmethod
    def from_pair(face: QuadFace, material: TextureFlatMaterial) -> "PolyFT4":
        p = PolyFT4()
        f.write(self.u0)
        f.write(self.v0)
        f.write(self.u1)
        f.write(self.v1)
        f.write(self.u2)
        f.write(self.v2)
        f.write(self.u3)
        f.write(self.v3)
        f.write(self.r0)
        f.write(self.g0)
        f.write(self.b0)
        f.write(self.iv0)
        f.write(self.iv1)
        f.write(self.iv2)
        f.write(self.iv3)
        f.write(self.in0)
        return p

    def write(self, f):
        f.write(self.ftype.value)
        f.write(self.u0)
        f.write(self.v0)
        f.write(self.u1)
        f.write(self.v1)
        f.write(self.u2)
        f.write(self.v2)
        f.write(self.u3)
        f.write(self.v3)
        f.write(self.r0)
        f.write(self.g0)
        f.write(self.b0)
        f.write(self.iv0)
        f.write(self.iv1)
        f.write(self.iv2)
        f.write(self.iv3)
        f.write(self.in0)


@dataclass
class PolyGT4:
    ftype: c_ubyte = PolyType.GT4
    u0: c_ubyte = c_ubyte(0)
    v0: c_ubyte = c_ubyte(0)
    u1: c_ubyte = c_ubyte(0)
    v1: c_ubyte = c_ubyte(0)
    u2: c_ubyte = c_ubyte(0)
    v2: c_ubyte = c_ubyte(0)
    u3: c_ubyte = c_ubyte(0)
    v3: c_ubyte = c_ubyte(0)
    r0: c_ubyte = c_ubyte(0)
    g0: c_ubyte = c_ubyte(0)
    b0: c_ubyte = c_ubyte(0)
    r1: c_ubyte = c_ubyte(0)
    g1: c_ubyte = c_ubyte(0)
    b1: c_ubyte = c_ubyte(0)
    r2: c_ubyte = c_ubyte(0)
    g2: c_ubyte = c_ubyte(0)
    b2: c_ubyte = c_ubyte(0)
    r3: c_ubyte = c_ubyte(0)
    g3: c_ubyte = c_ubyte(0)
    b3: c_ubyte = c_ubyte(0)
    iv0: c_ushort = c_ushort(0)
    iv1: c_ushort = c_ushort(0)
    iv2: c_ushort = c_ushort(0)
    iv3: c_ushort = c_ushort(0)
    in0: c_ushort = c_ushort(0)
    in1: c_ushort = c_ushort(0)
    in2: c_ushort = c_ushort(0)
    in3: c_ushort = c_ushort(0)

    @staticmethod
    def from_pair(face: QuadFace, material: TextureGouraudMaterial) -> "PolyGT4":
        p = PolyGT4()
        f.write(self.u0)
        f.write(self.v0)
        f.write(self.u1)
        f.write(self.v1)
        f.write(self.u2)
        f.write(self.v2)
        f.write(self.u3)
        f.write(self.v3)
        f.write(self.r0)
        f.write(self.g0)
        f.write(self.b0)
        f.write(self.r1)
        f.write(self.g1)
        f.write(self.b1)
        f.write(self.r2)
        f.write(self.g2)
        f.write(self.b2)
        f.write(self.r3)
        f.write(self.g3)
        f.write(self.b3)
        f.write(self.iv0)
        f.write(self.iv1)
        f.write(self.iv2)
        f.write(self.iv3)
        f.write(self.in0)
        f.write(self.in1)
        f.write(self.in2)
        f.write(self.in3)
        return p

    def write(self, f):
        f.write(self.ftype.value)
        f.write(self.u0)
        f.write(self.v0)
        f.write(self.u1)
        f.write(self.v1)
        f.write(self.u2)
        f.write(self.v2)
        f.write(self.u3)
        f.write(self.v3)
        f.write(self.r0)
        f.write(self.g0)
        f.write(self.b0)
        f.write(self.r1)
        f.write(self.g1)
        f.write(self.b1)
        f.write(self.r2)
        f.write(self.g2)
        f.write(self.b2)
        f.write(self.r3)
        f.write(self.g3)
        f.write(self.b3)
        f.write(self.iv0)
        f.write(self.iv1)
        f.write(self.iv2)
        f.write(self.iv3)
        f.write(self.in0)
        f.write(self.in1)
        f.write(self.in2)
        f.write(self.in3)


Polygon = PolyF3 | PolyG3 | PolyF4 | PolyG4 | PolyFT3 | PolyGT3 | PolyFT4 | PolyGT4


def gen_polygon(face, material) -> Polygon:
    ftype = type(face)
    mtype = type(material.info)

    if ftype == TriangleFace:
        if mtype == FlatMaterial:
            return PolyF3.from_pair(face, material.info)
        elif mtype == GouraudMaterial:
            return PolyG3.from_pair(face, material.info)
        elif mtype == TextureFlatMaterial:
            return PolyFT3.from_pair(face, material.info)
        elif mtype == TextureGouraudMaterial:
            return PolyGT3.from_pair(face, material.info)
        else:
            print(f"Unknown material type {mtype}")
            exit(1)
    elif ftype == QuadFace:
        if mtype == FlatMaterial:
            return PolyF4.from_pair(face, material.info)
        elif mtype == GouraudMaterial:
            return PolyG4.from_pair(face, material.info)
        elif mtype == TextureFlatMaterial:
            return PolyFT4.from_pair(face, material.info)
        elif mtype == TextureGouraudMaterial:
            return PolyGT4.from_pair(face, material.info)
        else:
            print(f"Unknown material type {mtype}")
            exit(1)
    else:
        print(f"Unknown face type {type(face)}")
        exit(1)


@dataclass
class MDLModel:
    num_vertices: c_ushort = c_ushort(0)
    num_normals: c_ushort = c_ushort(0)
    num_polys: c_ushort = c_ushort(0)
    vertices: [SVECTOR] = field(default_factory=list)
    normals: [SVECTOR] = field(default_factory=list)
    polygons: [Polygon] = field(default_factory=list)

    def write(self, f):
        f.write(self.num_vertices)
        f.write(self.num_normals)
        f.write(self.num_polys)
        for v in self.vertices:
            f.write(v.vx)
            f.write(v.vy)
            f.write(v.vz)
        for n in self.normals:
            f.write(n.vx)
            f.write(n.vy)
            f.write(n.vz)
        for p in self.polygons:
            p.write(f)


def convert_model(rsd: RSDModel) -> MDLModel:
    model = MDLModel()
    model.num_vertices = rsd.ply.num_vertices
    model.num_normals = rsd.ply.num_normals
    model.vertices = rsd.ply.vertices
    model.normals = rsd.ply.normals

    pairs = rsd.ply.pair_materials_and_faces(rsd.mat.materials)
    for pair in pairs:
        model.polygons.append(gen_polygon(pair[0], pair[1]))
    model.num_polys = c_ushort(len(model.polygons))

    return model
