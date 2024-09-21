#!/bin/env python

import os
import sys

from pprint import pp as pprint
from dataclasses import dataclass, field

from fixedpoint import tofixed12
from common import *
from face import *
from material import *

from import_types import *
from export_types import *


# ------------------------------------------


def parse_rsd(filename: str) -> RSDModel:
    if not os.path.isfile(filename):
        print(f"File {filename} does not exist")
        exit(1)

    filename = os.path.abspath(filename)
    rsd = RSDModel()
    with open(filename, "r") as f:
        # Read magic
        while True:
            rsd.magic = f.readline().strip()
            if not rsd.magic:
                print(f"Unexpected EOF at {filename}")
                exit(1)
            if rsd.magic[0] == "#":
                continue
            if rsd.magic[:4] != "@RSD":
                print(f"Could not parse RSD file {filename}")
                exit(1)
            break

        ply_filename = None
        mat_filename = None
        # Read other lines
        while True:
            buffer = f.readline().strip()
            if not buffer:
                break
            elif buffer[:3] == "PLY":
                ply_filename = buffer[4:]
            elif buffer[:3] == "MAT":
                mat_filename = buffer[4:]
            elif buffer[:4] == "MTEX":
                # TODO
                pass

    if ply_filename:
        ply_filename = os.path.dirname(filename) + "/" + ply_filename
        rsd.ply = parse_ply(ply_filename.strip())
    if mat_filename:
        mat_filename = os.path.dirname(filename) + "/" + mat_filename
        rsd.mat = parse_mat(mat_filename.strip())
    return rsd


# ------------------------------------------


def _parse_face(buffer: str) -> Face:
    buffer = buffer.split()
    ftype = FaceType(int(buffer[0]))

    if ftype == FaceType.TRIANGLE:
        v = TriangleFace()
        v.iv0 = c_ushort(int(buffer[1]))
        v.iv1 = c_ushort(int(buffer[2]))
        v.iv2 = c_ushort(int(buffer[3]))
        v.in0 = c_ushort(int(buffer[5]))
        v.in1 = c_ushort(int(buffer[6]))
        v.in2 = c_ushort(int(buffer[7]))
        return v
    elif ftype == FaceType.QUAD:
        v = QuadFace()
        v.iv0 = c_ushort(int(buffer[1]))
        v.iv1 = c_ushort(int(buffer[2]))
        v.iv2 = c_ushort(int(buffer[3]))
        v.iv3 = c_ushort(int(buffer[4]))
        v.in0 = c_ushort(int(buffer[5]))
        v.in1 = c_ushort(int(buffer[6]))
        v.in2 = c_ushort(int(buffer[7]))
        v.in3 = c_ushort(int(buffer[8]))
        return v
    elif ftype == FaceType.LINE:
        v = LineFace()
        v.iv0 = c_ushort(int(buffer[1]))
        v.iv1 = c_ushort(int(buffer[2]))
        return v
    elif ftype == FaceType.SPRITE:
        v = SpriteFace()
        v.iv0 = c_ushort(int(buffer[1]))
        v.iv1 = c_ushort(int(buffer[2]))
        v.iw = c_ushort(int(buffer[3]))
        v.ih = c_ushort(int(buffer[4]))
        return v
    else:
        print(f"Unknown face type {ftype}")
        exit(1)


def parse_ply(filename: str) -> PlyData:
    if not os.path.isfile(filename):
        print(f"File {filename} does not exist")
        exit(1)

    ply = PlyData()
    with open(filename, "r") as f:
        # Read magic
        while True:
            ply.magic = f.readline().strip()
            if not ply.magic:
                print(f"Unexpected EOF at {filename}")
                exit(1)
            if ply.magic[0] == "#":
                continue
            if ply.magic[:4] != "@PLY":
                print(f"Could not parse PLY file {filename}")
                exit(1)
            break

        step = 0
        vertices = 0
        normals = 0
        faces = 0
        while True:
            buffer = f.readline().strip()
            if not buffer:
                break
            elif buffer[0] == "#":
                continue

            if step == 0:
                numbers = buffer.split()
                ply.num_vertices = c_ushort(int(numbers[0]))
                ply.num_normals = c_ushort(int(numbers[1]))
                ply.num_faces = c_ushort(int(numbers[2]))
                vertices = ply.num_vertices.value
                normals = ply.num_normals.value
                faces = ply.num_faces.value
                step += 1
            elif step == 1:
                if vertices > 0:
                    values = buffer.split()
                    # print(f"Buffered values: {values}")
                    values = [float(x) for x in values]
                    # print(f"Float values: {values}")
                    values = [tofixed12(x) for x in values]
                    # print(f"Fixed values: {values}")
                    # print("---------------")
                    vertex = VECTOR()
                    vertex.vx = c_int(values[0])
                    vertex.vy = c_int(values[1])
                    vertex.vz = c_int(values[2])
                    ply.vertices.append(vertex)
                    vertices -= 1
                elif normals > 0:
                    values = buffer.split()
                    values = [tofixed12(float(x)) for x in values]
                    normal = VECTOR()
                    normal.vx = c_int(values[0])
                    normal.vy = c_int(values[1])
                    normal.vz = c_int(values[2])
                    ply.normals.append(normal)
                    normals -= 1
                elif faces > 0:
                    ply.faces.append(_parse_face(buffer))
                    faces -= 1
                else:
                    step += 1
            else:
                break
    assert ply.num_vertices.value == len(ply.vertices)
    assert ply.num_normals.value == len(ply.normals)
    assert ply.num_faces.value == len(ply.faces)
    return ply


# ------------------------------------------


def _parse_material(buffer: str) -> Material:
    buffer = buffer.split()

    material = Material()
    material.ipolygon = c_ushort(int(buffer[0]))
    material.flag = c_ubyte(int(buffer[1]))
    material.shading = ShadingType(buffer[2])

    mtype = MaterialType(buffer[3])
    if mtype == MaterialType.Flat:
        material.info = FlatMaterial()
        material.info.r0 = c_ubyte(int(buffer[4]))
        material.info.g0 = c_ubyte(int(buffer[5]))
        material.info.b0 = c_ubyte(int(buffer[6]))
    elif mtype == MaterialType.Gouraud:
        material.info = GouraudMaterial()
        material.info.r0 = c_ubyte(int(buffer[4]))
        material.info.g0 = c_ubyte(int(buffer[5]))
        material.info.b0 = c_ubyte(int(buffer[6]))
        material.info.r1 = c_ubyte(int(buffer[5]))
        material.info.g1 = c_ubyte(int(buffer[6]))
        material.info.b1 = c_ubyte(int(buffer[7]))
        material.info.r2 = c_ubyte(int(buffer[8]))
        material.info.g2 = c_ubyte(int(buffer[9]))
        material.info.b2 = c_ubyte(int(buffer[10]))
        material.info.r3 = c_ubyte(int(buffer[11]))
        material.info.g3 = c_ubyte(int(buffer[12]))
        material.info.b3 = c_ubyte(int(buffer[13]))
    elif mtype == MaterialType.Texture:
        m = TextureMaterial()
        material.info.u0 = c_ubyte(int(buffer[4]))
        material.info.v0 = c_ubyte(int(buffer[5]))
        material.info.u1 = c_ubyte(int(buffer[6]))
        material.info.v1 = c_ubyte(int(buffer[7]))
        material.info.u2 = c_ubyte(int(buffer[8]))
        material.info.v2 = c_ubyte(int(buffer[9]))
        material.info.u3 = c_ubyte(int(buffer[10]))
        material.info.v3 = c_ubyte(int(buffer[11]))
    elif mtype == MaterialType.TextureFlat:
        material.info = TextureFlatMaterial()
        material.info.u0 = c_ubyte(int(buffer[4]))
        material.info.v0 = c_ubyte(int(buffer[5]))
        material.info.u1 = c_ubyte(int(buffer[6]))
        material.info.v1 = c_ubyte(int(buffer[7]))
        material.info.u2 = c_ubyte(int(buffer[8]))
        material.info.v2 = c_ubyte(int(buffer[9]))
        material.info.u3 = c_ubyte(int(buffer[10]))
        material.info.v3 = c_ubyte(int(buffer[11]))
        material.info.r0 = c_ubyte(int(buffer[12]))
        material.info.g0 = c_ubyte(int(buffer[13]))
        material.info.b0 = c_ubyte(int(buffer[14]))
    elif mtype == MaterialType.TextureGouraud:
        material.info = TextureGouraudMaterial()
        material.info.u0 = c_ubyte(int(buffer[4]))
        material.info.v0 = c_ubyte(int(buffer[5]))
        material.info.u1 = c_ubyte(int(buffer[6]))
        material.info.v1 = c_ubyte(int(buffer[7]))
        material.info.u2 = c_ubyte(int(buffer[8]))
        material.info.v2 = c_ubyte(int(buffer[9]))
        material.info.u3 = c_ubyte(int(buffer[10]))
        material.info.v3 = c_ubyte(int(buffer[11]))
        material.info.r0 = c_ubyte(int(buffer[12]))
        material.info.g0 = c_ubyte(int(buffer[13]))
        material.info.b0 = c_ubyte(int(buffer[14]))
        material.info.r1 = c_ubyte(int(buffer[15]))
        material.info.g1 = c_ubyte(int(buffer[16]))
        material.info.b1 = c_ubyte(int(buffer[17]))
        material.info.r2 = c_ubyte(int(buffer[18]))
        material.info.g2 = c_ubyte(int(buffer[19]))
        material.info.b2 = c_ubyte(int(buffer[20]))
        material.info.r3 = c_ubyte(int(buffer[21]))
        material.info.g3 = c_ubyte(int(buffer[22]))
        material.info.b3 = c_ubyte(int(buffer[23]))
    else:
        print(f"Unhandled material type {mtype}")
        exit(1)

    return material


def parse_mat(filename: str) -> MatData:
    if not os.path.isfile(filename):
        print(f"File {filename} does not exist")
        exit(1)

    mat = MatData()
    with open(filename, "r") as f:
        # Read magic
        while True:
            mat.magic = f.readline().strip()
            if not mat.magic:
                print(f"Unexpected EOF at {filename}")
                exit(1)
            if mat.magic[0] == "#":
                continue
            if mat.magic[:4] != "@MAT":
                print(f"Could not parse MAT file {filename}")
                exit(1)
            break

        step = 0
        items = 0
        while True:
            buffer = f.readline().strip()
            if not buffer:
                break
            elif buffer[0] == "#":
                continue

            if step == 0:
                mat.num_items = c_ushort(int(buffer))
                items = mat.num_items.value
                step += 1
            elif step == 1:
                if items > 0:
                    mat.materials.append(_parse_material(buffer))
                    items -= 1
                else:
                    step += 1
            else:
                break
    assert mat.num_items.value == len(mat.materials)
    return mat


def main():
    filename = sys.argv[1]
    rsd = parse_rsd(filename.strip())
    mdl = convert_model(rsd)
    out_filename = os.path.splitext(filename)[0] + ".mdl"
    with open(out_filename, "wb") as f:
        mdl.write(f)


if __name__ == "__main__":
    main()
