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

#define ADVANCE_TIMER 15

static short sx = 50, sy = 50;
static short advance = ADVANCE_TIMER;
static short framenum = 0;

/* static SVECTOR c_rot    = { 0 }; */
/* static VECTOR  c_pos    = { 50, 50, 0 }; */
/* static VECTOR  c_scale  = { ONE, ONE, ONE }; */
/* static MATRIX  c_world  = { 0 }; */

void
chara_render_test(Chara *chara)
{
    int changed = 0;
    advance--;
    if(advance < 0) {
        advance = ADVANCE_TIMER;
        framenum = (framenum + 1) % chara->numframes;
        changed = 1;
    }

    /* RotMatrix(&c_rot, &c_world); */
    /* TransMatrix(&c_world, &c_pos); */
    /* ScaleMatrix(&c_world, &c_scale); */
    /* gte_SetRotMatrix(&c_world); */
    /* gte_SetTransMatrix(&c_world); */

    uint16_t SEVEN = ONE * 7;
    
    CharaFrame *frame = &chara->frames[framenum];

    for(uint16_t row = 0; row < frame->rows; row++) {
        for(uint16_t col = 0; col < frame->cols; col++) {
            uint16_t idx = (row * frame->cols) + col;
            idx = frame->tiles[idx];
            if(idx == 0) continue;

            // Get upper left UV from frame index on tileset
            uint16_t v0idx = idx >> 5; // divide by 32
            uint16_t u0idx = idx - (v0idx << 5);

            uint8_t u0, v0;
            u0 = (char)u0idx * 8;  v0 = (char)v0idx * 8;

            uint16_t x0 = sx + (col * 8) + frame->x;
            uint16_t y0 = sy + (row * 8) + frame->y;

            SPRT *sprt = (SPRT *) get_next_prim();
            increment_prim(sizeof(SPRT));
            setSprt8(sprt);
            setXY0(sprt, x0, y0);
            setWH(sprt, SEVEN, SEVEN);
            setRGB0(sprt, 128, 128, 128);
            setUV0(sprt, u0, v0);
            setClut(sprt, chara->crectx, chara->crecty);
            sort_prim(sprt, 1);
        }
    }

    // Sort tpage
    DR_TPAGE *tpri = (DR_TPAGE *) get_next_prim();
    increment_prim(sizeof(DR_TPAGE));
    setDrawTPage(tpri, 0, 0, getTPage(0, 0, chara->prectx, chara->precty));
    sort_prim(tpri, OT_LENGTH - 1);
}
