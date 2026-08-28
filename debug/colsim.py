#!/usr/bin/env python3
# colsim.py — offline frame-accurate simulation of the engine-psx player
# ground/ceiling/push collision pipeline on real level data.
# Mirrors src/collision.c:linecast + relevant parts of src/player.c.
# Angles: PSX 12-bit (0..0x1000); rsin(0x400)=4096; positions 20.12 fixed.
import math

from colkit import parse_col, parse_map128, parse_lvl

SNAP = 16
REACH = SNAP + 16
PUSH_RADIUS = 10
WIDTH_NORMAL = 9

FLOOR, RWALL, CEILING, LWALL = 0, 1, 2, 3

GSMODE_FLOOR_RIGHT = 0x01F0
GSMODE_CEIL_MIN = 0x0610
GSMODE_CEIL_MAX = 0x09F0
GSMODE_FLOOR_LEFT = 0x0E10
PSMODE_RWALL_MIN = 0x01B0
PSMODE_RWALL_MAX = 0x05B0
PSMODE_LWALL_MIN = 0x0A50
PSMODE_LWALL_MAX = 0x0E50

X_ACCEL = 0x000C0
X_FRICTION = 0x000C0
X_TOP_SPD = 0x06000
Y_GRAVITY = 0x00380
X_SLOPE_NORMAL = 0x00200
X_SLOPE_MIN_SPD = 0x000D0
X_MAX_SPD = 0x10000
X_MAX_SLIP_SPD = 0x02800  # 2.5px/f

LANDING_FLAT_RIGHT = 0x0105
LANDING_FLAT_LEFT = 0x0F11
LANDING_SLOPE_RIGHT = 0x0200
LANDING_SLOPE_LEFT = 0x0E0B
CEILING_FLAT_MIN = 0x0600
CEILING_FLAT_MAX = 0x0A00


def rsin(a):
    return int(round(math.sin(a * math.pi / 2048.0) * 4096))


def rcos(a):
    return int(round(math.cos(a * math.pi / 2048.0) * 4096))


def clamp12(v):
    return max(-X_MAX_SPD, min(X_MAX_SPD, v))


class World:
    def __init__(self, level_dir, lvl="Z2"):
        self.col = parse_col(f"{level_dir}/MAP16.COL")
        self.m128 = parse_map128(f"{level_dir}/MAP128.MAP")
        self.lw, self.lh, self.ltiles = parse_lvl(f"{level_dir}/{lvl}.LVL")[0]

    def _tile(self, lx, ly):
        cx, cy = lx >> 7, ly >> 7
        if cx < 0 or cy < 0 or cx >= self.lw or cy >= self.lh:
            return 0, 0
        chunk = self.ltiles[cy * self.lw + cx]
        if chunk <= 0:
            return 0, 0
        px, py = (lx & 0x7F) >> 4, (ly & 0x7F) >> 4
        return self.m128[chunk * 64 + (py << 3) + px]

    def _h_angle(self, piece, direction, lx, ly):
        m = self.col.get(piece)
        if m is None:
            return 0, 0
        if direction == FLOOR:
            return m["floor"][1][lx & 0xF], m["floor"][0]
        if direction == RWALL:
            return m["rwall"][1][15 - (ly & 0xF)], m["rwall"][0]
        if direction == CEILING:
            return m["ceiling"][1][15 - (lx & 0xF)], m["ceiling"][0]
        return m["lwall"][1][ly & 0xF], m["lwall"][0]

    @staticmethod
    def _tip(direction, lx, ly):
        if direction == FLOOR:
            return 15 - (ly & 0xF)
        if direction == CEILING:
            return ly & 0xF
        if direction == LWALL:
            return lx & 0xF
        return 15 - (lx & 0xF)

    def linecast(self, vx, vy, direction, magnitude, floor_direction=FLOOR):
        """Exact port of src/collision.c:linecast(). Returns (hit, coord, angle)."""
        lx, ly = vx, vy
        if direction == FLOOR:
            ly += magnitude
        elif direction == CEILING:
            ly -= magnitude
        elif direction == RWALL:
            lx += magnitude
        else:
            lx -= magnitude
        ev = (0, 0, 0)
        for _ in range((magnitude >> 4) + 1):
            piece, props = self._tile(lx, ly)
            if (piece > 0 and props != 2 and not (props & 4)
                    and not (direction != floor_direction and props == 1)):
                h, angle = self._h_angle(piece, direction, lx, ly)
                if h > 0:
                    th = self._tip(direction, lx, ly)
                    if direction == floor_direction or h >= th:
                        cx, cy = lx >> 7, ly >> 7
                        px, py = (lx & 0x7F) >> 4, (ly & 0x7F) >> 4
                        if direction in (FLOOR, RWALL):
                            ax = (cy << 7) + (py << 4) if direction == FLOOR \
                                else (cx << 7) + (px << 4)
                            coord = ax + (16 - h)
                        else:
                            ax = (cy << 7) + (py << 4) if direction == CEILING \
                                else (cx << 7) + (px << 4)
                            coord = ax + h
                        ev = (1, coord, angle)
            if direction == FLOOR:
                ly = max(ly - 16, vy)
            elif direction == RWALL:
                lx = max(lx - 16, vx)
            elif direction == CEILING:
                ly = min(ly + 16, vy)
            else:
                lx = min(lx + 16, vx)
        return ev


def sensor_dist(direction, anchor_axis, coord):
    if direction in (FLOOR, RWALL):
        return coord - (anchor_axis + SNAP)
    return (anchor_axis - SNAP) - coord


class Player:
    def __init__(self, x, y):
        self.px, self.py = x << 12, y << 12
        self.vx = self.vy = self.vz = 0
        self.angle = 0
        self.grnd = 1
        self.gsmode = self.psmode = FLOOR
        self.push = 0
        self.ctrllock = 0

    def resolve_modes(self):
        if not self.grnd:
            self.gsmode = self.psmode = FLOOR
            return
        a = self.angle
        if a <= GSMODE_FLOOR_RIGHT or a >= GSMODE_FLOOR_LEFT:
            self.gsmode = FLOOR
        elif a < GSMODE_CEIL_MIN:
            self.gsmode = RWALL
        elif a <= GSMODE_CEIL_MAX:
            self.gsmode = CEILING
        else:
            self.gsmode = LWALL
        if a < PSMODE_RWALL_MIN or a > PSMODE_LWALL_MAX:
            self.psmode = FLOOR
        elif a <= PSMODE_RWALL_MAX:
            self.psmode = RWALL
        elif a < PSMODE_LWALL_MIN:
            self.psmode = CEILING
        else:
            self.psmode = LWALL

    def collide_lr(self, w, hold_right):
        """Push sensors (player.c::_player_update_collision_lr).
        Fires only on cardinal angles. hold_right: input direction held."""
        self.push = 0
        if (self.angle % 0x400) != 0:
            return
        anchorx = self.px >> 12
        anchory = (self.py >> 12) - 8
        if self.psmode == FLOOR:
            if self.grnd and self.angle == 0:
                anchory += 8
        else:
            anchory = self.py >> 12
        ldir, rdir = {FLOOR: (LWALL, RWALL), RWALL: (FLOOR, CEILING),
                      CEILING: (RWALL, LWALL), LWALL: (CEILING, FLOOR)}[self.psmode]
        spd = self.vz if self.grnd else self.vx
        if spd == 0:
            return
        axis = anchory if self.psmode in (FLOOR, CEILING) else anchorx
        if self.psmode in (FLOOR, CEILING):
            axis = anchorx if self.psmode == FLOOR else anchorx
        # engine: anchor axis used by push is along the cast-normal axis:
        # FLOOR mode sensors cast along X, axis on X; RWALL/LWALL cast along Y.

        def accept(coord, cast_axis):
            return abs(coord - cast_axis) <= PUSH_RADIUS

        if spd < 0:
            h = w.linecast(anchorx, anchory, ldir, PUSH_RADIUS, self.gsmode)
            if h[0]:
                cax = anchory if ldir in (FLOOR, CEILING) else anchorx
                if accept(h[1], cax):
                    if self.grnd:
                        self.vz = 0
                        self.push = 1
                    else:
                        self.vx = 0
                    # engine per-branch position snap (player.c 597-653)
                    m = self.psmode
                    if m == RWALL:
                        self.py = (h[1] - PUSH_RADIUS) << 12
                    elif m == LWALL:
                        self.py = (h[1] + PUSH_RADIUS) << 12
                    elif m == CEILING:
                        self.px = (h[1] - PUSH_RADIUS) << 12
                    else:
                        self.px = (h[1] + PUSH_RADIUS) << 12
        if spd > 0:
            h = w.linecast(anchorx, anchory, rdir, PUSH_RADIUS, self.gsmode)
            if h[0]:
                cax = anchory if rdir in (FLOOR, CEILING) else anchorx
                if accept(h[1], cax):
                    if self.grnd:
                        self.vz = 0
                        self.push = 1
                    else:
                        self.vx = 0
                    m = self.psmode
                    if m == RWALL:
                        self.py = (h[1] + PUSH_RADIUS) << 12
                    elif m == LWALL:
                        self.py = (h[1] - PUSH_RADIUS) << 12
                    elif m == CEILING:
                        self.px = (h[1] + PUSH_RADIUS) << 12
                    else:
                        self.px = (h[1] - PUSH_RADIUS) << 12

    def collide_tb(self, w):
        px, py = self.px >> 12, self.py >> 12
        grndir = self.gsmode
        ceildir = {FLOOR: CEILING, CEILING: FLOOR,
                   RWALL: LWALL, LWALL: RWALL}[grndir]
        lat = WIDTH_NORMAL
        ax_l, ax_r, ay_l, ay_r = px, px, py, py
        if grndir == RWALL:
            ay_l += lat
            ay_r -= lat - 1
        elif grndir == LWALL:
            ay_l -= lat
            ay_r += lat - 1
        elif grndir == CEILING:
            ax_l += lat
            ax_r -= lat - 1
        else:
            ax_l -= lat
            ax_r += lat - 1
        gaxis = py if grndir in (FLOOR, CEILING) else px
        caxis = py if ceildir in (FLOOR, CEILING) else px

        def accept_ground(dist):
            if self.grnd:
                mx = min(4 + abs(self.vz >> 12), 14)
                return -14 <= dist <= mx
            if self.vy < 0:
                return False
            return -(self.vy >> 12) - 8 <= dist <= 0

        g1 = w.linecast(ax_l, ay_l, grndir, REACH, self.gsmode)
        if g1[0] and not accept_ground(sensor_dist(grndir, gaxis, g1[1])):
            g1 = (0, 0, 0)
        g2 = w.linecast(ax_r, ay_r, grndir, REACH, self.gsmode)
        if g2[0] and not accept_ground(sensor_dist(grndir, gaxis, g2[1])):
            g2 = (0, 0, 0)

        if not self.grnd:
            c1 = w.linecast(ax_l, ay_l, ceildir, REACH, self.gsmode)
            if c1[0] and not (-SNAP <= sensor_dist(ceildir, caxis, c1[1]) <= 0):
                c1 = (0, 0, 0)
            c2 = w.linecast(ax_r, ay_r, ceildir, REACH, self.gsmode)
            if c2[0] and not (-SNAP <= sensor_dist(ceildir, caxis, c2[1]) <= 0):
                c2 = (0, 0, 0)

            # landing?
            if (g1[0] or g2[0]) and self.vy >= 0:
                win = _win(g1, g2, grndir, gaxis)
                self.angle = win[2]
                # landing speed transfer (flat ground only matters here)
                if self.angle >= LANDING_FLAT_LEFT or self.angle <= LANDING_FLAT_RIGHT:
                    self.vz = self.vx
                elif self.angle <= LANDING_SLOPE_RIGHT or self.angle >= LANDING_SLOPE_LEFT:
                    self.vz = ((self.vy * 2048) >> 12) * -sgn(rsin(self.angle))
                else:
                    self.vz = self.vy * -sgn(rsin(self.angle))
                self.py = (win[1] - SNAP) << 12
                self.grnd = 1
            # ceiling bump / ceiling landing
            if (c1[0] or c2[0]) and self.vy < 0:
                win = _win(c1, c2, ceildir, caxis)
                mostly_h = abs(self.vx) > abs(self.vy)
                if (not mostly_h
                        and (win[2] < CEILING_FLAT_MIN or win[2] > CEILING_FLAT_MAX)):
                    self.angle = win[2]
                    self.vz = self.vy * -sgn(rsin(self.angle))
                    self.grnd = 1
                else:
                    self.py = (win[1] + SNAP) << 12
                    self.vy = 0
        else:
            if not (g1[0] or g2[0]):
                self.grnd = 0
                self.gsmode = self.psmode = FLOOR
            else:
                win = _win(g1, g2, grndir, gaxis)
                self.angle = win[2]
                if self.gsmode == RWALL:
                    self.px = (win[1] - SNAP) << 12
                elif self.gsmode == LWALL:
                    self.px = (win[1] + SNAP) << 12
                elif self.gsmode == CEILING:
                    self.py = (win[1] + SNAP) << 12
                else:
                    self.py = (win[1] - SNAP) << 12
        return (g1, g2)


def _win(e1, e2, direction, axis):
    if e1[0] and e2[0]:
        d1 = sensor_dist(direction, axis, e1[1])
        d2 = sensor_dist(direction, axis, e2[1])
        return e2 if d2 < d1 else e1
    return e1 if e1[0] else e2


def sgn(x):
    return (x > 0) - (x < 0)


def step(p, w, hold_right):
    """One frame: physics input then movement then collision (same order as C).
    engine order: resolve_modes, collide_lr, collide_tb happen *before* physics
    in player_update (collision operates on previous pos), then velocity update
    and position += vel.
    """
    p.resolve_modes()
    p.collide_lr(w, hold_right)
    p.collide_tb(w)
    if p.ctrllock > 0 and p.grnd:
        p.ctrllock -= 1

    if p.grnd:
        if hold_right:
            if p.vz < 0:
                p.vz += 0x00800  # skid decel
            elif p.vz < X_TOP_SPD:
                p.vz += X_ACCEL
        # slope factor
        if abs(p.vz) >= X_SLOPE_MIN_SPD:
            p.vz -= (X_SLOPE_NORMAL * rsin(p.angle)) >> 12
        # slip/fall
        if p.ctrllock == 0 and abs(p.vz) < X_MAX_SLIP_SPD \
                and 0x18E <= p.angle <= 0xE7D:
            p.ctrllock = 30
            if 0x311 <= p.angle <= 0xD05:
                p.grnd = 0
            p.vz += -0x800 if p.angle < 0x800 else 0x800
        p.vz = clamp12(p.vz)
        p.vx = (p.vz * rcos(p.angle)) >> 12
        p.vy = (p.vz * -rsin(p.angle)) >> 12
    else:
        if hold_right and p.vx < X_TOP_SPD:
            p.vx += X_ACCEL * 2  # air accel is same 0xC0; keep simple
        p.vy += Y_GRAVITY
    p.px += p.vx
    p.py += p.vy
