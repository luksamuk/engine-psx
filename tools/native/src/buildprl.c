/*
 * buildprl.c - Convert parallax TOML to binary PRL format
 * Replaces buildprl.py
 * 
 * Binary layout:
 * - num_strips (u8)
 * For each strip:
 *   - u0 (u8), v0 (u8)
 *   - width (u16 BE), height (u16 BE)
 *   - bg_index (u8)
 *   - single (u8)
 *   - scrollx (s32 BE - 20.12 fixed)
 *   - speedx (s32 BE - 20.12 fixed)
 *   - y0 (s16 BE)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "toml.h"
#include "psxtypes.h"

#define MAX_STRIPS 256

typedef struct {
    u8 u0;
    u8 v0;
    u16 width;
    u16 height;
    u8 bg_index;
    u8 single;
    s32 scrollx;  // 20.12 fixed point
    s32 speedx;   // 20.12 fixed point
    s16 y0;
} ParallaxStrip;

typedef struct {
    int num_strips;
    ParallaxStrip strips[MAX_STRIPS];
} ParallaxData;

static inline s32 float_to_fixed12(float f) {
    return (s32)(f * 4096.0f);
}

void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s <parallax.toml>\n", prog);
    fprintf(stderr, "Output: PRL.PRL in same directory as input\n");
}

const char* get_output_path(const char *input_path) {
    static char output_path[1024];
    char *copy = strdup(input_path);
    char *dir = dirname(copy);
    
    if (strlen(dir) > 0) {
        snprintf(output_path, sizeof(output_path), "%s/PRL.PRL", dir);
    } else {
        snprintf(output_path, sizeof(output_path), "PRL.PRL");
    }
    
    free(copy);
    return output_path;
}

int parse_parallax(const char *filename, ParallaxData *prl) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: cannot open %s\n", filename);
        return -1;
    }
    
    // Read entire file
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(size + 1);
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    
    char errbuf[256];
    toml_table_t *root = toml_parse(buf, errbuf, sizeof(errbuf));
    
    if (!root) {
        fprintf(stderr, "Error: TOML parse failed: %s\n", errbuf);
        free(buf);
        return -1;
    }
    
    // Iterate through all tables (each is a strip)
    int num_strips = 0;
    toml_table_t *tab = root->next;  // Skip root, get first section
    
    while (tab && num_strips < MAX_STRIPS) {
        ParallaxStrip *s = &prl->strips[num_strips];
        memset(s, 0, sizeof(ParallaxStrip));
        
        // Read values (use .ok to check if found)
        toml_datum_t u0 = toml_int_in(tab, "u0");
        toml_datum_t v0_orig = toml_int_in(tab, "v0");
        toml_datum_t width = toml_int_in(tab, "width");
        toml_datum_t height = toml_int_in(tab, "height");
        toml_datum_t y0 = toml_int_in(tab, "y0");
        toml_datum_t single = toml_bool_in(tab, "single");
        
        // Floats
        toml_datum_t scrollx = toml_double_in(tab, "scrollx");
        toml_datum_t speedx = toml_double_in(tab, "speedx");
        
        // Use defaults if not found
        int v0_full = v0_orig.ok ? (int)v0_orig.u.i : 0;
        s->u0 = u0.ok ? (u8)u0.u.i : 0;
        s->v0 = v0_full % 256;
        s->width = width.ok ? (u16)width.u.i : 0;
        s->height = height.ok ? (u16)height.u.i : 0;
        s->y0 = y0.ok ? (s16)y0.u.i : 0;
        s->single = single.ok ? (single.u.b ? 1 : 0) : 0;
        
        // Floats default to 0
        s->scrollx = scrollx.ok ? float_to_fixed12((float)scrollx.u.d) : 0;
        s->speedx = speedx.ok ? float_to_fixed12((float)speedx.u.d) : 0;
        
        // Calculate bg_index from original v0 (NOT modulated)
        s->bg_index = v0_full / 256;
        
        printf("Strip %s: u0=%d v0=%d (%d:%d) %dx%d y0=%d scrollx=%d speedx=%d\n", 
               tab->name ? tab->name : "?", s->u0, s->v0, 
               s->bg_index, s->v0, s->width, s->height, s->y0,
               s->scrollx, s->speedx);
        
        tab = tab->next;
        num_strips++;
    }
    
    prl->num_strips = num_strips;
    
    toml_free(root);
    free(buf);
    
    printf("Total strips: %d\n", num_strips);
    return 0;
}

int write_parallax(const char *filename, const ParallaxData *prl) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Error: cannot create %s\n", filename);
        return -1;
    }
    
    // Write number of strips
    write_u8(f, (u8)prl->num_strips);
    
    // Write each strip
    for (int i = 0; i < prl->num_strips; i++) {
        const ParallaxStrip *s = &prl->strips[i];
        write_u8(f, s->u0);
        write_u8(f, s->v0);
        write_u16_be(f, s->width);
        write_u16_be(f, s->height);
        write_u8(f, s->bg_index);
        write_u8(f, s->single);
        write_s32_be(f, s->scrollx);
        write_s32_be(f, s->speedx);
        write_s16_be(f, s->y0);
    }
    
    fclose(f);
    printf("Created %s (%d bytes)\n", filename, (int)(1 + prl->num_strips * 18));
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *input = argv[1];
    const char *output = get_output_path(input);
    
    ParallaxData prl = {0};
    
    if (parse_parallax(input, &prl) != 0) {
        return 1;
    }
    
    if (write_parallax(output, &prl) != 0) {
        return 1;
    }
    
    return 0;
}
