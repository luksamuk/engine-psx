/*
 * buildprl.c - Convert parallax TOML to binary PRL format
 * Uses official toml.c by CK Tan (MIT License)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "toml.h"
#include "psxtypes.h"

#define MAX_STRIPS 256

typedef struct {
    u8 u0, v0;
    u16 width, height;
    u8 bg_index, single;
    s32 scrollx, speedx;
    s16 y0;
} ParallaxStrip;

typedef struct {
    int num_strips;
    ParallaxStrip strips[MAX_STRIPS];
} ParallaxData;

static inline s32 float_to_fixed12(float f) {
    return (s32)(f * 4096.0f);
}

int parse_parallax(const char *filename, ParallaxData *prl) {
    FILE *f = fopen(filename, "r");
    if (!f) { fprintf(stderr, "Error: cannot open %s\n", filename); return -1; }
    
    char errbuf[256];
    toml_table_t *root = toml_parse_file(f, errbuf, sizeof(errbuf));
    fclose(f);
    
    if (!root) { fprintf(stderr, "Error: TOML parse failed: %s\n", errbuf); return -1; }
    
    int num_strips = 0;
    // Iterate through keys in root table
    for (int i = 0; ; i++) {
        const char *key = toml_key_in(root, i);
        if (!key) break;
        
        toml_table_t *strip = toml_table_in(root, key);
        if (!strip) continue;
        if (num_strips >= MAX_STRIPS) break;
        
        ParallaxStrip *s = &prl->strips[num_strips];
        memset(s, 0, sizeof(ParallaxStrip));
        
        toml_datum_t u0 = toml_int_in(strip, "u0");
        toml_datum_t v0 = toml_int_in(strip, "v0");
        toml_datum_t width = toml_int_in(strip, "width");
        toml_datum_t height = toml_int_in(strip, "height");
        toml_datum_t y0 = toml_int_in(strip, "y0");
        
        toml_datum_t scrollx = toml_double_in(strip, "scrollx");
        toml_datum_t speedx = toml_double_in(strip, "speedx");
        toml_datum_t single = toml_bool_in(strip, "single");
        
        int v0_full = v0.ok ? (int)v0.u.i : 0;
        s->u0 = u0.ok ? (u8)u0.u.i : 0;
        s->v0 = v0_full % 256;
        s->width = width.ok ? (u16)width.u.i : 0;
        s->height = height.ok ? (u16)height.u.i : 0;
        s->y0 = y0.ok ? (s16)y0.u.i : 0;
        s->single = single.ok ? (single.u.b ? 1 : 0) : 0;
        s->scrollx = scrollx.ok ? float_to_fixed12((float)scrollx.u.d) : 0;
        s->speedx = speedx.ok ? float_to_fixed12((float)speedx.u.d) : 0;
        s->bg_index = v0_full / 256;
        
        printf("Strip %s: u0=%d v0=%d (%d:%d) %dx%d y0=%d\n", 
               key, s->u0, s->v0, s->bg_index, s->v0,
               s->width, s->height, s->y0);
        
        num_strips++;
    }
    
    prl->num_strips = num_strips;
    toml_free(root);
    printf("Total strips: %d\n", num_strips);
    return 0;
}

void write_parallax(const char *filename, const ParallaxData *prl) {
    FILE *f = fopen(filename, "wb");
    if (!f) { fprintf(stderr, "Error: cannot create %s\n", filename); return; }
    
    write_u8(f, (u8)prl->num_strips);
    for (int i = 0; i < prl->num_strips; i++) {
        const ParallaxStrip *s = &prl->strips[i];
        write_u8(f, s->u0); write_u8(f, s->v0);
        write_u16_be(f, s->width); write_u16_be(f, s->height);
        write_u8(f, s->bg_index); write_u8(f, s->single);
        write_s32_be(f, s->scrollx); write_s32_be(f, s->speedx);
        write_s16_be(f, s->y0);
    }
    fclose(f);
    printf("Created %s\n", filename);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <parallax.toml>\n", argv[0]);
        fprintf(stderr, "Output: PRL.PRL in same directory as input\n");
        return 1;
    }
    
    char *copy = strdup(argv[1]);
    char *dir = dirname(copy);
    char output[1024];
    snprintf(output, sizeof(output), "%s/PRL.PRL", strlen(dir) > 0 ? dir : ".");
    free(copy);
    
    ParallaxData prl = {0};
    if (parse_parallax(argv[1], &prl) != 0) return 1;
    write_parallax(output, &prl);
    return 0;
}
