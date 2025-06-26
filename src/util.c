#include "util.h"
#include <inline_c.h>
#include <psxcd.h>
#include <psxgpu.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int
RotAverageNclip3(
    SVECTOR *a, SVECTOR *b, SVECTOR *c,
    uint32_t *xy0, uint32_t *xy1, uint32_t *xy2,
    int *otz)
{
    int nclip = 0;

    gte_ldv0(a);
    gte_ldv1(b);
    gte_ldv2(c);
    gte_rtpt();
    gte_nclip();
    gte_stopz(&nclip);
    if(nclip <= 0) goto exit;
    gte_stsxy3(xy0, xy1, xy2);
    gte_avsz3();
    gte_stotz(otz);
exit:
    return nclip;
}

int
RotAverageNclip4(
    SVECTOR *a, SVECTOR *b, SVECTOR *c, SVECTOR *d,
    uint32_t *xy0, uint32_t *xy1, uint32_t *xy2, uint32_t *xy3,
    int *otz)
{
    int nclip = 0;

    gte_ldv0(a);
    gte_ldv1(b);
    gte_ldv2(c);
    gte_rtpt();
    gte_nclip();
    gte_stopz(&nclip);
    if(nclip <= 0) goto exit;
    gte_stsxy0(xy0);
    gte_ldv0(d);
    gte_rtps();
    gte_stsxy3(xy1, xy2, xy3);
    gte_avsz4();
    gte_stotz(otz);
exit:
    return nclip;
}

int
RotTransPers(SVECTOR *v, uint32_t *xy0)
{
    int otz = 0;
    gte_ldv0(v);
    gte_rtps();
    gte_stsxy0(xy0);
    gte_stotz(&otz);
    return otz;
}

void
CrossProduct0(VECTOR *v0, VECTOR *v1, VECTOR *out)
{
    gte_ldopv1(v0);
    gte_ldopv2(v1);
    gte_op0();
    gte_stlvnl(out);
}

void
CrossProduct12(VECTOR *v0, VECTOR *v1, VECTOR *out)
{
    gte_ldopv1(v0);
    gte_ldopv2(v1);
    gte_op12();
    gte_stlvnl(out);
}

uint8_t *
file_read(const char *filename, uint32_t *length)
{
    CdlFILE filepos;
    int numsectors;
    uint8_t *buffer;
    buffer = NULL;

    if(CdSearchFile(&filepos, filename) == NULL) {
        printf("File %s not found!\n", filename);
        return NULL;
    }

    numsectors = (filepos.size + 2047) / 2048;
    buffer = (uint8_t *) malloc(2048 * numsectors);
    if(!buffer) {
        printf("Error allocating %d sectors.\n", numsectors);
        return NULL;
    }

    CdControl(CdlSetloc, (uint8_t *) &filepos.pos, 0);
    CdRead(numsectors, (uint32_t *) buffer, CdlModeSpeed);
    CdReadSync(0, 0);

    *length = filepos.size;
    return buffer;
}

void
load_clut_only(TIM_IMAGE *tim)
{
    if(tim->mode & 0x8) {
        LoadImage(tim->crect, tim->caddr);
        DrawSync(0);
    }
}

void
load_texture(uint8_t *data, TIM_IMAGE *tim)
{
    GetTimInfo((const uint32_t *)data, tim);
    LoadImage(tim->prect, tim->paddr);
    DrawSync(0);
    load_clut_only(tim);
}

uint16_t *
_clut_color_address(TIM_IMAGE *tim, uint32_t n)
{
    return (uint16_t *)(tim->caddr) + n;
}

uint16_t
clut_get_color(TIM_IMAGE *tim, uint32_t n)
{
    return *_clut_color_address(tim, n);
}

void
clut_set_color(TIM_IMAGE *tim, uint32_t n, uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t *c = _clut_color_address(tim, n);
    uint8_t stp = (*c & 0x8000) >> 15;
    *c = (stp << 15)
        | RGB_TO_CLUT(b) << 10
        | RGB_TO_CLUT(g) << 5
        | RGB_TO_CLUT(r);
}

void
clut_print_color(TIM_IMAGE *tim, uint32_t n)
{
    uint16_t color = clut_get_color(tim, n);
    uint8_t r = (color & 0x001f);
    uint8_t g = (color & 0x03e0) >> 5;
    uint8_t b = (color & 0x7c00) >> 10;
    uint8_t stp = (color & 0x8000) >> 15;
    printf("Color #%02d: (%2d, %2d, %2d) (#%02x%02x%02x) STP: %d\n",
           n,
           r,
           g,
           b,
           CLUT_TO_RGB(r),
           CLUT_TO_RGB(g),
           CLUT_TO_RGB(b),
           stp);
}

void
clut_print_all_colors(TIM_IMAGE *tim)
{
    // Get number of colors by pointer arithmetic.
    // Number of colors is 64b before start of color array
    uint32_t clut_length = *(tim->caddr - 2);
    uint32_t rectsize = (tim->crect->w * tim->crect->h * 2048);
    uint32_t num_colors = clut_length / rectsize;
    printf("Number of colors: %d\n", num_colors);
    for(uint32_t i = 0; i < num_colors; i++) {
        clut_print_color(tim, i);
    }
}

void
clut_set_glow_color(TIM_IMAGE *tim, uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t clut_length = *(tim->caddr - 2);
    uint32_t rectsize = (tim->crect->w * tim->crect->h * 2048);
    uint32_t num_colors = clut_length / rectsize;
    for(uint32_t i = 0; i < num_colors; i++) {
        uint16_t color = clut_get_color(tim, i);
        uint8_t cr = (color & 0x001f);
        uint8_t cg = (color & 0x03e0) >> 5;
        uint8_t cb = (color & 0x7c00) >> 10;
        uint8_t stp = (color & 0x8000) >> 15;
        if(!cr && !cg && !cb && stp) {
            clut_set_color(tim, i, r, g, b);
            printf("Setting glow color %d: #%02x%02x%02x => #%02x%02x%02x\n",
                   i,
                   CLUT_TO_RGB(cr),
                   CLUT_TO_RGB(cg),
                   CLUT_TO_RGB(cb),
                   r,
                   g,
                   b);
            return;
        }
    }
    printf("Could not set glow color\n");
}

uint8_t
get_byte(uint8_t *bytes, uint32_t *b)
{
    return (uint8_t) bytes[(*b)++];
}

uint16_t
get_short_be(uint8_t *bytes, uint32_t *b)
{
    uint16_t value = 0;
    value |= bytes[(*b)++] << 8;
    value |= bytes[(*b)++];
    return value;
}

uint16_t
get_short_le(uint8_t *bytes, uint32_t *b)
{
    uint16_t value = 0;
    value |= bytes[(*b)++];
    value |= bytes[(*b)++] << 8;
    return value;
}

uint32_t
get_long_be(uint8_t *bytes, uint32_t *b)
{
    uint32_t value = 0;
    value |= bytes[(*b)++] << 24;
    value |= bytes[(*b)++] << 16;
    value |= bytes[(*b)++] << 8;
    value |= bytes[(*b)++];
    return value;
}


uint32_t
adler32(const char *s)
{
    uint32_t a = 0x0001, b = 0x0000;
    const char *i = s;
    while(*i != '\0') {
        a = (a + *i) % 0xfff1;
        b = (a + b) % 0xfff1;
        i++;
    }
    return (b << 16) | a;
}

int32_t
div12(int32_t a, int32_t b)
{
    return ((a << 12) / b) & ~(uint32_t)0xfff;
}

int32_t
floor12(int32_t a)
{
    return a & ~(uint32_t)0xfff;
}
