from dataclasses import dataclass, field
from common import *


@dataclass
class TriangleFace:
    iv0: c_ushort = c_ushort(0)
    iv1: c_ushort = c_ushort(0)
    iv2: c_ushort = c_ushort(0)
    __unused0: c_ushort = c_ushort(0)
    in0: c_ushort = c_ushort(0)
    in1: c_ushort = c_ushort(0)
    in2: c_ushort = c_ushort(0)
    __unused1: c_ushort = c_ushort(0)

    def __repr__(self):
        return f"<Triangle iv0:{self.iv0.value}, iv1:{self.iv1.value}, iv2:{self.iv2.value}, in0:{self.in0.value}, in1:{self.in1.value}, in2{self.in2.value}>"


@dataclass
class QuadFace:
    iv0: c_ushort = c_ushort(0)
    iv1: c_ushort = c_ushort(0)
    iv2: c_ushort = c_ushort(0)
    iv3: c_ushort = c_ushort(0)
    in0: c_ushort = c_ushort(0)
    in1: c_ushort = c_ushort(0)
    in2: c_ushort = c_ushort(0)
    in3: c_ushort = c_ushort(0)

    def __repr__(self):
        return f"<Quad iv0:{self.iv0.value}, iv1:{self.iv1.value}, iv2:{self.iv2.value}, iv3:{self.iv3.value}, in0:{self.in0.value}, in1:{self.in1.value}, in2{self.in2.value}, in3:{self.in3.value}>"


@dataclass
class LineFace:
    iv0: c_ushort = c_ushort(0)
    iv1: c_ushort = c_ushort(0)
    __unused0: c_ushort = c_ushort(0)
    __unused1: c_ushort = c_ushort(0)
    __unused2: c_ushort = c_ushort(0)
    __unused3: c_ushort = c_ushort(0)
    __unused4: c_ushort = c_ushort(0)
    __unused5: c_ushort = c_ushort(0)

    def __repr__(self):
        return f"<Line iv0:{self.iv0.value}, iv1:{self.iv1.value}>"


@dataclass
class SpriteFace:
    iv0: c_ushort = c_ushort(0)
    iv1: c_ushort = c_ushort(0)
    iw: c_ushort = c_ushort(0)
    ih: c_ushort = c_ushort(0)
    __unused0: c_ushort = c_ushort(0)
    __unused1: c_ushort = c_ushort(0)
    __unused2: c_ushort = c_ushort(0)
    __unused3: c_ushort = c_ushort(0)

    def __repr__(self):
        return f"<Sprite iv0:{self.iv0.value}, iv1:{self.iv1.value}, iw:{self.iv2.value}, ih:{self.iv3.value}>"


Face = TriangleFace | QuadFace | LineFace | SpriteFace
