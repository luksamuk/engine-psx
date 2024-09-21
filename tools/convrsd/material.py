from dataclasses import dataclass, field

from common import *


@dataclass
class FlatMaterial:
    r0: c_ubyte = c_ubyte(0)
    g0: c_ubyte = c_ubyte(0)
    b0: c_ubyte = c_ubyte(0)

    def __repr__(self):
        return f"<F #{self.r0.value:02X}{self.g0.value:02X}{self.b0.value:02X}>"


@dataclass
class GouraudMaterial:
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

    def __repr__(self):
        rgb0 = f"#{self.r0.value:02X}{self.g0.value:02X}{self.b0.value:02X}"
        rgb1 = f"#{self.r1.value:02X}{self.g1.value:02X}{self.b1.value:02X}"
        rgb2 = f"#{self.r2.value:02X}{self.g2.value:02X}{self.b2.value:02X}"
        rgb3 = f"#{self.r3.value:02X}{self.g3.value:02X}{self.b3.value:02X}"
        return f"<G {rgb0}, {rgb1}, {rgb2}, {rgb3}>"


@dataclass
class TextureMaterial:
    u0: c_ubyte = c_ubyte(0)
    v0: c_ubyte = c_ubyte(0)
    u1: c_ubyte = c_ubyte(0)
    v1: c_ubyte = c_ubyte(0)
    u2: c_ubyte = c_ubyte(0)
    v2: c_ubyte = c_ubyte(0)
    u3: c_ubyte = c_ubyte(0)
    v3: c_ubyte = c_ubyte(0)

    def __repr__(self):
        uv0 = f"({self.u0.value:02X}, {self.v0.value:02X})"
        uv1 = f"({self.u1.value:02X}, {self.v1.value:02X})"
        uv2 = f"({self.u2.value:02X}, {self.v2.value:02X})"
        uv3 = f"({self.u3.value:02X}, {self.v3.value:02X})"
        return f"<T {uv0} {uv1} {uv2} {uv3}>"


@dataclass
class TextureFlatMaterial:
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

    def __repr__(self):
        uv0 = f"({self.u0.value:02X}, {self.v0.value:02X})"
        uv1 = f"({self.u1.value:02X}, {self.v1.value:02X})"
        uv2 = f"({self.u2.value:02X}, {self.v2.value:02X})"
        uv3 = f"({self.u3.value:02X}, {self.v3.value:02X})"
        rgb0 = f"#{self.r0.value:02X}{self.g0.value:02X}{self.b0.value:02X}"
        return f"<FT {rgb0} {{{uv0} {uv1} {uv2} {uv3}}}"


@dataclass
class TextureGouraudMaterial:
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

    def __repr__(self):
        uv0 = f"({self.u0.value:02X}, {self.v0.value:02X})"
        uv1 = f"({self.u1.value:02X}, {self.v1.value:02X})"
        uv2 = f"({self.u2.value:02X}, {self.v2.value:02X})"
        uv3 = f"({self.u3.value:02X}, {self.v3.value:02X})"
        rgb0 = f"#{self.r0.value:02X}{self.g0.value:02X}{self.b0.value:02X}"
        rgb1 = f"#{self.r1.value:02X}{self.g1.value:02X}{self.b1.value:02X}"
        rgb2 = f"#{self.r2.value:02X}{self.g2.value:02X}{self.b2.value:02X}"
        rgb3 = f"#{self.r3.value:02X}{self.g3.value:02X}{self.b3.value:02X}"
        return (
            f"<GT {{{rgb0} {uv0}}} {{{rgb1} {uv1}}} {{{rgb2} {uv2}}} {{{rgb3} {uv3}}}>"
        )


MaterialInfo = (
    FlatMaterial
    | GouraudMaterial
    | TextureMaterial
    | TextureFlatMaterial
    | TextureGouraudMaterial
)


@dataclass
class Material:
    ipolygon: c_ushort = c_ushort(0)
    flag: c_ubyte = c_ubyte(0)
    shading: ShadingType = ShadingType.FLAT
    info: MaterialInfo | None = None

    def __repr__(self):
        return f"<Material (Face {self.ipolygon.value}): {self.shading} ST({self.flag.value:02X}); {self.info}>"
