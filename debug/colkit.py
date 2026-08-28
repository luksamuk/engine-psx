#!/usr/bin/env python3
# colkit.py — offline collision inspection for engine-psx levels.
# Parses tiles16.tsx (Tiled), MAP16.COL, MAP128.MAP, Z*.LVL and renders
# collision geometry + masks + metadata as images/CSV for analysis.
#
# Big-endian formats (see tools/cookcollision.py, chunkgen.py, cooklvl.py):
#   MAP16.COL: u16 numtiles; per tile: u16 id, then 4 masks (floor, rwall,
#              ceil, lwall), each: i32 psx_angle + 8 bytes (16 nibbles).
#   MAP128.MAP: u16 tilew, u16 ntiles, u16 framerows; frames: per chunk 64x
#              (u16 index, u8 props). props: 0 SOLID, 1 ONEWAY, 2 NONE, 4 FRONT.
#   Z*.LVL:    u8 num_layers, u8 unused; per layer: u8 w, u8 h, w*h u16(BE).
import json
import math
import struct
import sys
import xml.etree.ElementTree as ET

import numpy as np
from PIL import Image, ImageDraw


# ---------------------------------------------------------------- tsx parse
def parse_tsx(path):
    """Parse tiles16.tsx -> {tile_id: {points, predef{prop:int}, kind}}."""
    root = ET.parse(path).getroot()
    imagesize = None
    img = root.find("image")
    if img is not None:
        imagesize = (int(img.get("width")), int(img.get("height")))
    tiles = {}
    for tile in root.iter("tile"):
        tid = int(tile.get("id"))
        grp = tile.find("objectgroup")
        if grp is None:
            continue
        obj = grp.find("object")
        if obj is None:
            continue
        x = float(obj.get("x", 0))
        y = float(obj.get("y", 0))
        predef = {}
        props = obj.find("properties")
        if props is not None:
            for p in props.iter("property"):
                predef[p.get("name")] = int(p.get("value"), 16)
        poly = obj.find("polygon")
        if poly is not None:
            pts = []
            for pair in poly.get("points").split():
                px, py = pair.split(",")
                pts.append((float(px) + x, float(py) + y))
            kind = "polygon"
        else:
            w = float(obj.get("width", 16))
            h = float(obj.get("height", 16))
            pts = [(x, y), (x + w, y), (x + w, y + h), (x, y + h)]
            kind = "rect"
        tiles[tid] = {"points": pts, "predef": predef, "kind": kind}
    return tiles, imagesize


# ---------------------------------------------------------------- col parse
def parse_col(path):
    data = open(path, "rb").read()
    n = struct.unpack(">H", data[0:2])[0]
    off = 2
    out = {}
    for _ in range(n):
        tid = struct.unpack(">H", data[off:off + 2])[0]
        off += 2
        masks = {}
        for name in ("floor", "rwall", "ceiling", "lwall"):
            angle = struct.unpack(">i", data[off:off + 4])[0]
            raw = data[off + 4:off + 12]
            off += 12
            heights = []
            for i in range(16):
                byte = raw[i >> 1]
                heights.append((byte >> 4) & 0xF if i % 2 == 0 else byte & 0xF)
            masks[name] = (angle, heights)
        out[tid] = masks
    return out


# ---------------------------------------------------------------- map128
def parse_map128(path):
    data = open(path, "rb").read()
    tilew = struct.unpack(">H", data[0:2])[0]
    ntiles = struct.unpack(">H", data[2:4])[0]
    rows = struct.unpack(">H", data[4:6])[0]
    assert tilew == 128 and rows == 8
    frames = []
    off = 6
    for _ in range(ntiles * 64):
        index, props = struct.unpack(">HB", data[off:off + 3])
        frames.append((index, props))
        off += 3
    return frames


# ---------------------------------------------------------------- lvl
def parse_lvl(path):
    data = open(path, "rb").read()
    nlayers = data[0]
    off = 2
    layers = []
    for _ in range(nlayers):
        w, h = data[off], data[off + 1]
        off += 2
        tiles = struct.unpack(">%dH" % (w * h), data[off:off + 2 * w * h])
        off += 2 * w * h
        layers.append((w, h, tiles))
    return layers


# ---------------------------------------------------------- raster helpers
PX = 4  # zoom for tile rendering


def tile_image(tsx_entry, masks=None, tid=None):
    """Render one 16x16 tile: geometry + 4 mask bars + angle text info."""
    W, H = 16 * PX, 16 * PX
    img = Image.new("RGB", (W, H), (18, 18, 24))
    d = ImageDraw.Draw(img)
    pts = [(x * PX, y * PX) for x, y in tsx_entry["points"]]
    d.polygon(pts, fill=(40, 90, 40), outline=(80, 220, 90))
    for x in range(1, 16):
        d.line([(x * PX, 0), (x * PX, H)], fill=(30, 30, 40))
    for y in range(1, 16):
        d.line([(0, y * PX), (W, y * PX)], fill=(30, 30, 40))
    colors = {"floor": (250, 80, 80), "rwall": (80, 160, 250),
              "ceiling": (250, 200, 60), "lwall": (250, 100, 250)}
    if masks is not None:
        # floor: heights from bottom; ceiling: fill from top; etc.
        fa, fh = masks["floor"]
        for c, h in enumerate(fh):
            if h:
                d.rectangle([c * PX, (16 - h) * PX, (c + 1) * PX - 1, H - 1],
                            fill=colors["floor"])
        ca, ch = masks["ceiling"]
        for ci, h in enumerate(ch):
            cc = 15 - ci
            if h:
                d.rectangle([cc * PX, 0, (cc + 1) * PX - 1, (h + 1) * PX - 1],
                            fill=colors["ceiling"])
        ra, rh = masks["rwall"]
        for ri, h in enumerate(rh):
            rc = 15 - ri
            if h:
                d.rectangle([(16 - h) * PX, rc * PX, W - 1, (rc + 1) * PX - 1],
                            fill=colors["rwall"])
        la, lh = masks["lwall"]
        for rc, h in enumerate(lh):
            if h:
                d.rectangle([0, rc * PX, (h + 1) * PX - 1, (rc + 1) * PX - 1],
                            fill=colors["lwall"])
    return img


def psx_to_deg(a):
    return a * 360.0 / 4096.0


def describe_tile(tid, tsx_entry, masks):
    lines = [f"tile {tid} ({tsx_entry['kind'] if tsx_entry else 'no-tsx'})"]
    if tsx_entry:
        lines.append("  points: " +
                     str([(round(x, 1), round(y, 1)) for x, y in tsx_entry["points"]]))
        if tsx_entry["predef"]:
            pd = {k: f"{v} ({psx_to_deg(v):.0f}°)" for k, v in tsx_entry["predef"].items()}
            lines.append("  predef: " + str(pd))
    if masks:
        for name in ("floor", "rwall", "ceiling", "lwall"):
            ang, hts = masks[name]
            if any(hts):
                lines.append(f"  {name:7s} angle={ang:5d} ({psx_to_deg(ang):6.1f}°) mask={hts}")
    return "\n".join(lines)
