#!/bin/env python

import sys
import toml
from os.path import realpath, dirname, basename
from dataclasses import dataclass, field
from ctypes import c_ubyte, c_byte, c_short, c_ushort, c_int
from pprint import pp

c_short = c_short.__ctype_be__
c_ushort = c_ushort.__ctype_be__
c_int = c_int.__ctype_be__


def tofixed(value: float, scale: int) -> int:
    return int(value * (1 << scale))


def tofixed12(value: float) -> int:
    return tofixed(value, 12)


@dataclass
class ParallaxStrip:
    u0: int = 0
    v0: int = 0
    width: int = 0
    height: int = 0
    bg_index: int = 0
    single: bool = False
    scrollx: float = 0
    speedx: float = 0
    y0: int = 0

    def write_to(self, f):
        f.write(c_ubyte(self.u0))
        f.write(c_ubyte(self.v0))
        f.write(c_ushort(self.width))
        f.write(c_ushort(self.height))
        f.write(c_ubyte(self.bg_index))
        f.write(c_ubyte(int(self.single)))
        f.write(c_int(tofixed12(self.scrollx)))
        f.write(c_int(tofixed12(self.speedx)))
        f.write(c_short(self.y0))


@dataclass
class Parallax:
    strips: [ParallaxStrip] = field(default_factory=list)

    def write(self, out_path):
        with open(out_path, "wb") as f:
            f.write(c_ubyte(len(self.strips)))
            for s in self.strips:
                s.write_to(f)


def parse(data) -> Parallax:
    p = Parallax()
    for name, strip_data in data.items():
        strip = ParallaxStrip()
        strip.single = strip_data.get("single", False)
        strip.scrollx = strip_data.get("scrollx", 0)
        strip.speedx = strip_data.get("speedx", 0)
        strip.y0 = strip_data.get("y0")
        strip.width = strip_data.get("width")
        strip.height = strip_data.get("height")

        v0 = strip_data.get("v0")
        strip.bg_index = int(v0 / 256)
        strip.u0 = strip_data.get("u0")
        strip.v0 = v0 % 256

        p.strips.append(strip)
    return p


def main():
    toml_path = realpath(sys.argv[1])
    data = toml.load(toml_path)
    parallax = parse(data)

    # /assets/levels/R2/parallax.toml -> /assets/levels/R2/PRL.PRL
    toml_dir = dirname(toml_path)
    out_path = toml_dir + "/PRL.PRL"
    parallax.write(out_path)


if __name__ == "__main__":
    main()
