#include "chara.h"
#include "util.h"

#include <stdlib.h>
#include <stdio.h>

Chara *
load_chara(const char *filename, TIM_IMAGE *tim)
{
    char *bytes;
    unsigned long b, length;

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
    for(unsigned int i = 0; i < chara->numframes; i++) {
        chara->frames[i].x = get_byte(bytes, &b);
        chara->frames[i].y = get_byte(bytes, &b);
        chara->frames[i].cols = get_byte(bytes, &b);
        chara->frames[i].rows = get_byte(bytes, &b);
        short numtiles = (short)chara->frames[i].cols * (short)chara->frames[i].rows;
        chara->frames[i].tiles = (unsigned short *)malloc(numtiles * sizeof(unsigned short));
        for(short j = 0; j < numtiles; j++) {
            chara->frames[i].tiles[j] = get_short_be(bytes, &b);
        }
    }

    chara->anims = (CharaAnim *)malloc(chara->numanims * sizeof(CharaAnim));
    for(unsigned int i = 0; i < chara->numanims; i++) {
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

#define ADVANCE_TIMER 30

static short sx = 50, sy = 50;
static short advance = ADVANCE_TIMER;
static short framenum = 0;

void
chara_render_test(Chara *chara)
{
    advance--;
    if(advance < 0) {
        advance = ADVANCE_TIMER;
        framenum = (framenum + 1) % chara->numframes;
    }
    
    CharaFrame *frame = &chara->frames[framenum];

    for(unsigned short row = 0; row < frame->rows; row++) {
        for(unsigned short col = 0; col < frame->cols; col++) {
            unsigned short idx = (row * frame->cols) + col;
            idx = frame->tiles[idx];
            if(idx == 0) continue;

            // Get upper left UV from frame index on tileset
            unsigned char v0idx = idx >> 5; // divide by 32
            unsigned char u0idx = idx - (v0idx << 5);

            unsigned char u0, v0;
            u0 = u0idx * 8;  v0 = v0idx * 8;
            
            /* unsigned char u1, v1, u2, v2, u3, v3; */
            /* u1 = u0 + 7;     v1 = v0; */
            /* u2 = u0;         v2 = v0 + 7; */
            /* u3 = u0 + 7;     v3 = v0 + 7; */
            
            /*printf("%d =>  uv0 = (%d %d), uv1 = (%d %d), uv2 = (%d %d), uv3 = (%d %d)\n",
              idx, u0, v0, u1, v1, u2, v2, u3, v3);*/

            short x0 = sx + (col * 7) + frame->x;
            short y0 = sy + (row * 7) + frame->y;

            SPRT_8 *sprt = (SPRT_8 *) get_next_prim();
            increment_prim(sizeof(SPRT_8));
            setSprt8(sprt);
            setXY0(sprt, x0, y0);
            setRGB0(sprt, 128, 128, 128);
            setUV0(sprt, u0, v0);
            setClut(sprt, chara->crectx, chara->crecty);
            sort_prim(sprt, 1);

            /* POLY_FT4 *poly = (POLY_FT4 *) get_next_prim(); */
            /* increment_prim(sizeof(POLY_FT4)); */
            /* setPolyFT4(poly); */
            /* setRGB0(poly, 128, 128, 128); */
            /* poly->tpage = getTPage(0, 0, chara->prectx, chara->precty); */
            /* setClut(poly, chara->crectx, chara->crecty); */
            /* setUV4(poly, u0, v0, u1, v1, u2, v2, u3, v3); */
            /* setXY4(poly, */
            /*        x0,     y0, */
            /*        x0 + 7, y0, */
            /*        x0,     y0 + 7, */
            /*        x0 + 7, y0 + 7); */
            /* sort_prim(poly, 1); */
        }
    }

    // Sort tpage
    DR_TPAGE *tpri = (DR_TPAGE *) get_next_prim();
    increment_prim(sizeof(DR_TPAGE));
    setDrawTPage(tpri, 0, 0, getTPage(0, 0, chara->prectx, chara->precty));
    sort_prim(tpri, OT_LENGTH - 1);
}
