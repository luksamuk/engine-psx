#!/usr/bin/env python3
# autogeom.py — auto-generate collision geometry for 16x16 tiles from the
# tileset image's alpha channel, and inject into tiles16.tsx.
#
# The engine consumes geometry as an object-group polygon (or rect) per tile in
# tiles16.tsx; cookcollision.py then linecasts it into height masks for the 4
# modes. For ramp/wall tiles the geometry must follow the tile's artwork.
#
# Strategy: for each tile, read the alpha mask (0/255). For the FLOOR mode,
# each column's fill height = number of solid pixels counting up from the
# bottom. Because the cooker's height-mask generation does the exact same
# scan, the most faithful geometry is the polygon traced along the *surface*
# of the solid alpha: the floor profile (top edge) plus the tile's side/bottom
# borders that are filled.
#
# We emit a polygon hugging the mask's outer boundary; the cooker then
# reproduces masks matching the artwork for all four modes automatically.
import argparse
import xml.etree.ElementTree as ET

import numpy as np
from PIL import Image


def tile_alpha(img, tid, cols):
    a = np.asarray(img, dtype=np.uint8)
    tx, ty = (tid % cols) * 16, (tid // cols) * 16
    return a[ty:ty + 16, tx:tx + 16, 3] > 100  # 16x16 bool: solid?


def to_psx_angle(deg):
    """deg (float) -> 12-bit PSX angle."""
    return int(round((deg % 360) * 4096 / 360)) & 0xFFF


def floor_profile_angle(mask):
    """Estimate the floor angle from the floor height mask, using the
    *largest monotone section* (the ramp proper) rather than the endpoints.
    This mirrors how a level designer would read the tile: the dominant
    slope, not the first-to-last delta (which flattens out on plateau+rise
    geometries like the Surely Wood 61/62/77 tiles).
    Returns degrees (float), or None if the mask is flat/empty."""
    if all(h == mask[0] for h in mask) or all(h == 0 for h in mask):
        return 0.0 if mask[0] != 0 else None
    # find longest monotonic run
    best_len, best = 1, (0, 0)
    start = 0
    for i in range(1, len(mask)):
        if (mask[i] - mask[i - 1]) * (mask[start + 1] - mask[start]) < 0 if \
                mask[start + 1] != mask[start] else False:
            if i - start > best_len:
                best_len, best = i - start, (start, i - 1)
            start = i - 1
    if len(mask) - start > best_len:
        best_len, best = len(mask) - start, (start, len(mask) - 1)
    a, b = best
    if b - a < 2:
        return None
    dy = mask[b] - mask[a]
    dx = b - a
    return _math.degrees(_math.atan2(-dy, dx))


import math as _math


def trace_polygon(mask):
    """Trace the outer boundary of the solid region as a polygon.
    Returns list of (x, y) in tile-local coords, or None if empty.

    Approach: column-wise floor fill from bottom; walk the surface left to
    right, recording the topmost filled pixel per column. Polygon:
      left border (bottom->top), surface (left->right), right border (top->bottom),
      bottom border -> back to start. Only include borders that touch solid.
    """
    H = W = 16
    if not mask.any():
        return None
    # column height and top
    tops = []
    for x in range(W):
        col = mask[:, x]
        if col.any():
            tops.append((x, int(np.argmax(col))))  # first solid row from top
        else:
            tops.append((x, None))
    xs = [x for x, t in tops if t is not None]
    x0, x1 = min(xs), max(xs)

    pts = []
    # left edge (from bottom up to first top)
    first_top = tops[x0][1]
    pts.append((x0, 16))
    pts.append((x0, first_top))
    # surface
    for x in range(x0, x1 + 1):
        t = tops[x][1]
        if t is None:
            continue
        pts.append((x, t))
        t_next = tops[x + 1][1] if x + 1 <= x1 and tops[x + 1][1] is not None else None
        if t_next is not None and t_next != t:
            pts.append((x + 1, t_next))
    # right edge down
    pts.append((x1 + 1, 16))
    # close bottom
    pts = [(float(px), float(py)) for px, py in pts]
    return pts


def floor_mask(mask):
    """16-long floor height mask (fills from bottom), as cookcollision generates."""
    H = W = 16
    out = []
    for x in range(W):
        col = mask[:, x]
        h = 0
        for y in range(H - 1, -1, -1):
            if col[y]:
                h = H - y
                break
        out.append(h)
    return out


def inject_tsx(tsx_path, tiles_geom, tiles_props, out_path):
    """Insert <objectgroup><polygon> elements for the given tile ids, plus
    optional floor_angle property overrides. Text-splice so file formatting
    outside the touched tiles is preserved."""
    src = open(tsx_path).read()
    import re
    next_id = 1 + max(
        [int(m) for m in re.findall(r'<object id="(\d+)"', src)] or [0])

    def object_xml(tid):
        pts = " ".join(f"{x:g},{y:g}" for x, y in tiles_geom[tid])
        nonlocal next_id
        oid = next_id
        next_id += 1
        inner = ""
        if tid in tiles_props:
            inner += "<properties>"
            for k, v in tiles_props[tid].items():
                inner += f'<property name="{k}" type="int" value="{v}"/>'
            inner += "</properties>"
        inner += f'<polygon points="{pts}"/>'
        return (f'<object id="{oid}" x="0" y="0">{inner}</object>')

    for tid in sorted(tiles_geom):
        body = object_xml(tid)
        entry = f'<objectgroup draworder="index" id="2">{body}</objectgroup>'
        m = re.search(
            r'<tile id="%d">(.*?)</tile>' % tid, src, flags=re.DOTALL)
        if m:
            tile_body = m.group(1)
            if "<objectgroup" in tile_body:
                new_body = re.sub(
                    r'<objectgroup.*?</objectgroup>', entry, tile_body,
                    flags=re.DOTALL)
                src = src.replace(m.group(0),
                                  f'<tile id="{tid}">{new_body}</tile>')
            else:
                src = src.replace(m.group(0),
                                  f'<tile id="{tid}">{entry}</tile>')
        else:
            block = f' <tile id="{tid}">{entry}</tile>\n'
            src = src.replace("</tileset>", block + "</tileset>")
    with open(out_path, "w") as f:
        f.write(src)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--image", required=True, help="16x16.png")
    ap.add_argument("--tsx", required=True, help="tiles16.tsx")
    ap.add_argument("--out", required=True, help="output tsx")
    ap.add_argument("--tiles", required=True,
                    help="comma list or ranges, e.g. 138-143,155-160")
    ap.add_argument("--smooth", action="store_true",
                    help="also emit floor_angle override properties for tiles "
                         "whose polygon-derived angle (via cookcollision's "
                         "endpoint estimator) would disagree with the mask's "
                         "dominant-slope angle")
    args = ap.parse_args()

    img = Image.open(args.image).convert("RGBA")
    cols = img.width // 16

    tids = []
    for part in args.tiles.split(","):
        part = part.strip()
        if "-" in part:
            a, b = part.split("-")
            tids += list(range(int(a), int(b) + 1))
        else:
            tids.append(int(part))

    geom = {}
    for tid in tids:
        mask = tile_alpha(img, tid, cols)
        poly = trace_polygon(mask)
        if poly is None:
            print(f"  tile {tid}: empty artwork, skipped")
            continue
        geom[tid] = poly

    props = {}
    if args.smooth:
        for tid, poly in geom.items():
            mask = tile_alpha(img, tid, cols)
            fm = floor_mask(mask)
            dom = floor_profile_angle(fm)
            if dom is None:
                continue
            # what would cookcollision produce from the polygon's endpoints?
            h0, h1 = fm[0], fm[-1]
            if h0 == h1 or dom == 0.0:
                continue
            end_deg = _math.degrees(_math.atan2(-(h1 - h0), 15))
            # if dominant-slope and endpoint angles disagree by >8deg,
            # the endpoint estimator would fire the push sensor; override.
            diff = abs(dom - end_deg)
            if 8 < diff < 180:
                props[tid] = {"floor_angle": to_psx_angle(end_deg)}
                print(f"  tile {tid}: overriding floor_angle "
                      f"mask-dominant={dom:.1f} -> end-to-end={end_deg:.1f} (psx {to_psx_angle(end_deg)})")

    inject_tsx(args.tsx, geom, props, args.out)
    print(f"Injected geometry for {len(geom)} tiles -> {args.out}")


if __name__ == "__main__":
    main()
