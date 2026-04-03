/* cookcollision.c - Cook 16x16 tile collision - Uses yyjson */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "yyjson.h"
#include "psxtypes.h"

#define MAX_VERTICES 16

typedef struct { float x, y; } Vec2;
typedef struct { int id; Vec2 v[MAX_VERTICES]; int nv; int fa, ca, ra, la; } Tile;
typedef struct { int h[16]; s32 a; } Mask;
typedef struct { Mask fl, ce, rw, lw; } TileMask;

int pip(float px, float py, Vec2 *verts, int n) {
    if (n < 3) return 0;
    bool inside = false;
    for (int i = 0, j = n-1; i < n; j = i++) {
        float yi = verts[i].y, yj = verts[j].y;
        float xi = verts[i].x, xj = verts[j].x;
        if (((yi > py) != (yj > py)) && (px < (xj-xi) * (py-yi) / (yj-yi) + xi))
            inside = !inside;
    }
    return inside;
}

s32 to_psx(float r) {
    float d = r * 180.0f / M_PI;
    while (d < 0) d += 360;
    while (d >= 360) d -= 360;
    return (s32)(d / 360.0f * 4096 + 0.5f);  // Round to nearest
}

void get_mask(Mask *m, int dir, Vec2 *v, int n, int pre, int has) {
    int hm[16], lp = 0;
    for (int p = 0; p < 16; p++) {
        int found = 0;
        for (int h = 15; h >= 1; h--) {
            float x, y;
            if (dir == 0) { x = p; y = 16 - h; }
            else if (dir == 1) { x = 15 - p; y = h; }
            else if (dir == 2) { x = h; y = p; }
            else { x = 16 - h; y = 15 - p; }
            if (pip(x, y, v, n)) {
                found = 1; hm[p] = h;
                if (p > 0 && hm[p-1] != h) lp = p;
                break;
            }
        }
        if (!found) hm[p] = 0;
    }
    memcpy(m->h, hm, sizeof(hm));
    if (has) { m->a = pre; return; }
    int d = hm[0] - hm[lp];
    float vx = 0, vy = 0;
    if (dir == 0) { vx = 16; vy = d; }
    else if (dir == 1) { vx = -16; vy = -d; }
    else if (dir == 2) { vx = -d; vy = 16; }
    else { vx = d; vy = -16; }
    float len = sqrtf(vx*vx + vy*vy);
    if (len > 0.0001f) { vx /= len; vy /= len; }
    // Calculate angle relative to +X axis (right), matching Python behavior
    float ang = -atan2f(vy, vx);
    if (ang < 0) ang += 2 * M_PI;
    m->a = to_psx(ang);
}

int parse(const char *fn, Tile **ts, int *nt) {
    FILE *f = fopen(fn, "r");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(sz+1);
    fread(buf, 1, sz, f);
    buf[sz] = 0;
    fclose(f);
    
    yyjson_doc *doc = yyjson_read(buf, sz, 0);
    free(buf);
    if (!doc) return -1;
    
    yyjson_val *arr = yyjson_obj_get(yyjson_doc_get_root(doc), "tiles");
    if (!arr || !yyjson_is_arr(arr)) { yyjson_doc_free(doc); return -1; }
    
    size_t n = yyjson_arr_size(arr);
    *ts = calloc(n, sizeof(Tile));
    int vc = 0;
    
    yyjson_val *t;
    size_t ti, tm;
    yyjson_arr_foreach(arr, ti, tm, t) {
        Tile *tc = &(*ts)[vc];
        tc->id = yyjson_get_int(yyjson_obj_get(t, "id"));
        yyjson_val *og = yyjson_obj_get(t, "objectgroup");
        if (!og) continue;
        yyjson_val *objs = yyjson_obj_get(og, "objects");
        if (!objs || !yyjson_is_arr(objs) || yyjson_arr_size(objs) == 0) continue;
        yyjson_val *o = yyjson_arr_get_first(objs);
        
        yyjson_val *props = yyjson_obj_get(o, "properties");
        if (yyjson_is_arr(props)) {
            yyjson_val *p; size_t pi, pm;
            yyjson_arr_foreach(props, pi, pm, p) {
                const char *n = yyjson_get_str(yyjson_obj_get(p, "name"));
                const char *v = yyjson_get_str(yyjson_obj_get(p, "value"));
                if (!n || !v) continue;
                int a = strtol(v, NULL, 16);
                if (strcmp(n, "floor_angle") == 0) tc->fa = a;
                else if (strcmp(n, "ceil_angle") == 0) tc->ca = a;
                else if (strcmp(n, "rwall_angle") == 0) tc->ra = a;
                else if (strcmp(n, "lwall_angle") == 0) tc->la = a;
            }
        }
        
        float ox = yyjson_get_num(yyjson_obj_get(o, "x"));
        float oy = yyjson_get_num(yyjson_obj_get(o, "y"));
        yyjson_val *poly = yyjson_obj_get(o, "polygon");
        if (poly && yyjson_is_arr(poly)) {
            tc->nv = yyjson_arr_size(poly);
            if (tc->nv > MAX_VERTICES) tc->nv = MAX_VERTICES;
            for (int i = 0; i < tc->nv; i++) {
                yyjson_val *v = yyjson_arr_get(poly, i);
                tc->v[i].x = yyjson_get_num(yyjson_obj_get(v, "x")) + ox;
                tc->v[i].y = yyjson_get_num(yyjson_obj_get(v, "y")) + oy;
            }
        } else {
            tc->nv = 4;
            float w = yyjson_get_num(yyjson_obj_get(o, "width"));
            float h = yyjson_get_num(yyjson_obj_get(o, "height"));
            tc->v[0] = (Vec2){ox, oy};
            tc->v[1] = (Vec2){ox+w, oy};
            // Bug compatibility: Python uses 'x + height' instead of 'y + height'
            tc->v[2] = (Vec2){ox+w, ox+h};
            tc->v[3] = (Vec2){ox, oy+h};
        }
        vc++;
    }
    *nt = vc;
    yyjson_doc_free(doc);
    return 0;
}

void proc(Tile *ts, int nt, TileMask **ms) {
    *ms = calloc(nt, sizeof(TileMask));
    for (int i = 0; i < nt; i++) {
        get_mask(&(*ms)[i].fl, 0, ts[i].v, ts[i].nv, ts[i].fa, ts[i].fa != 0);
        get_mask(&(*ms)[i].ce, 1, ts[i].v, ts[i].nv, ts[i].ca, ts[i].ca != 0);
        get_mask(&(*ms)[i].rw, 3, ts[i].v, ts[i].nv, ts[i].ra, ts[i].ra != 0);
        get_mask(&(*ms)[i].lw, 2, ts[i].v, ts[i].nv, ts[i].la, ts[i].la != 0);
    }
}

void wmask(FILE *f, int *h) {
    for (int i = 0; i < 16; i += 2)
        write_u8(f, ((h[i] & 0xF) << 4) | (h[i+1] & 0xF));
}

void write(const char *fn, Tile *ts, TileMask *ms, int nt) {
    FILE *f = fopen(fn, "wb");
    if (!f) return;
    write_u16_be(f, nt);
    for (int i = 0; i < nt; i++) {
        write_u16_be(f, ts[i].id);
        write_s32_be(f, ms[i].fl.a); wmask(f, ms[i].fl.h);
        write_s32_be(f, ms[i].rw.a); wmask(f, ms[i].rw.h);
        write_s32_be(f, ms[i].ce.a); wmask(f, ms[i].ce.h);
        write_s32_be(f, ms[i].lw.a); wmask(f, ms[i].lw.h);
    }
    fclose(f);
    printf("Created %s (%d tiles)\n", fn, nt);
}

int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr, "Usage: %s in.json out.COL\n", argv[0]); return 1; }
    Tile *ts; int nt;
    if (parse(argv[1], &ts, &nt) != 0) return 1;
    printf("Parsed %d tiles\n", nt);
    TileMask *ms;
    proc(ts, nt, &ms);
    write(argv[2], ts, ms, nt);
    free(ts); free(ms);
    return 0;
}
