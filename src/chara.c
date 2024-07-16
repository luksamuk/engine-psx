#include "chara.h"
#include "util.h"

#include <stdlib.h>
#include <stdio.h>

Chara *
load_chara(const char *filename, TIM_IMAGE *tim)
{
    uint8_t *bytes;
    uint32_t b, length;

    bytes = file_read(filename, &length);
    if(bytes == NULL) {
        printf("Error reading %s from the CD.\n", bytes);
        return NULL;
    }

    b = 0;

    Chara *chara = (Chara *)malloc(sizeof(Chara));
    chara->width = get_short_be(bytes, &b);
    chara->height = get_short_be(bytes, &b);
    chara->numframes = get_short_be(bytes, &b);
    chara->numanims = get_short_be(bytes, &b);

    chara->frames = (CharaFrame *)malloc(chara->numframes * sizeof(CharaFrame));
    for(uint16_t i = 0; i < chara->numframes; i++) {
        chara->frames[i].x = get_byte(bytes, &b);
        chara->frames[i].y = get_byte(bytes, &b);
        chara->frames[i].cols = get_byte(bytes, &b);
        chara->frames[i].rows = get_byte(bytes, &b);
        chara->frames[i].width = get_short_be(bytes, &b);
        chara->frames[i].height = get_short_be(bytes, &b);
        uint16_t numtiles = chara->frames[i].cols * chara->frames[i].rows;
        chara->frames[i].tiles = (uint16_t *)malloc(numtiles * sizeof(uint16_t));
        for(uint16_t j = 0; j < numtiles; j++) {
            chara->frames[i].tiles[j] = (uint16_t) get_short_be(bytes, &b);
        }
    }

    chara->anims = (CharaAnim *)malloc(chara->numanims * sizeof(CharaAnim));
    for(uint16_t i = 0; i < chara->numanims; i++) {
        for(int j = 0; j < 15; j++) {
            chara->anims[i].name[j] = get_byte(bytes, &b);
        }
        chara->anims[i].name[15] = '\0';
        chara->anims[i].start = get_byte(bytes, &b);
        chara->anims[i].end = get_byte(bytes, &b);
    }

    /*chara->crectx = tim->crect->x;
    chara->crecty = tim->crect->y;
    chara->prectx = tim->prect->x;
    chara->precty = tim->prect->y;*/
    printf("%d %d\n", tim->prect->x, tim->prect->y);
    chara->crectx = 320;
    chara->crecty = 256;
    chara->prectx = 320;
    chara->precty = 0; // why not loading correctly???

    return chara;
    free(bytes);
}

void
free_chara(Chara *chara)
{
    if(chara->frames != NULL) {
        for(unsigned short i = 0; i < chara->numframes; i++) {
            free(chara->frames[i].tiles);
        }
        free(chara->frames);
        free(chara->anims);
        free(chara);
    }
}

#include <psxgpu.h>
#include "render.h"
#include <inline_c.h>

#define ADVANCE_TIMER 60

/* static int16_t sx = 50, sy = 50; */
static int16_t advance = ADVANCE_TIMER;
static int16_t framenum = 0;

static uint8_t flip_x = 1;

/* static SVECTOR c_rot    = { 0 }; */
static SVECTOR  pos    = { SCREEN_XRES >> 1, SCREEN_YRES >> 1, 0 };
/* static VECTOR  c_scale  = { -ONE, ONE, ONE }; */
/* static MATRIX  c_world  = { 0 }; */

void
chara_render_test(Chara *chara)
{
    CharaFrame *frame = &chara->frames[framenum];

    int16_t left = frame->x >> 3;
    int16_t right = 7 - (frame->width >> 3) - left;

    for(uint16_t row = 0; row < frame->rows; row++) {
        for(uint16_t colx = 0; colx < frame->cols; colx++) {
            uint16_t col = flip_x ? frame->cols - colx - 1 : colx;
            uint16_t idx = (row * frame->cols) + col;
            idx = frame->tiles[idx];
            if(idx == 0) continue;

            // Get upper left UV from tile index on tileset
            uint16_t v0idx = idx >> 5; // divide by 32
            uint16_t u0idx = idx - (v0idx << 5);

            uint8_t
                u0 = (u0idx << 3),
                v0 = (v0idx << 3);

            SVECTOR xy0 = {
                pos.vx + (colx << 3) + frame->x,
                pos.vy + (row << 3) + frame->y - (chara->height >> 1),
                0, 0,
            };

            // I don't know why, but it works
            uint8_t tw, th;
            tw = (u0 < 248 ? 8 : 7);
            th = (v0 < 248 ? 8 : 7);

            if(flip_x) {
                if(u0 > 0) { u0--; }
                xy0.vx += (right << 4);
            } else {
                xy0.vx -= (left << 3);
            }
            xy0.vx -= (chara->width >> 1);
            

            POLY_FT4 *poly = (POLY_FT4 *) get_next_prim();
            increment_prim(sizeof(POLY_FT4));
            setPolyFT4(poly);
            setRGB0(poly, 128, 128, 128);
            setTPage(poly, 0, 0, chara->prectx, chara->precty);
            setClut(poly, chara->crectx, chara->crecty);
            setXYWH(poly, xy0.vx, xy0.vy, 8, 8);
            
            if(flip_x) {
                setUV4(
                    poly,
                    u0 + tw, v0,
                    u0,      v0,
                    u0 + tw, v0 + th,
                    u0, v0 + th);
            } else {
                setUVWH(poly, u0, v0, tw, th);
            }
            sort_prim(poly, 1);
        }
    }
}
