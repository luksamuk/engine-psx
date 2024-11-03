#include "chara.h"
#include "util.h"
#include "render.h"
#include <psxgpu.h>
#include <inline_c.h>
#include <stdlib.h>
#include <stdio.h>

extern uint8_t level_fade;

void
load_chara(Chara *chara, const char *filename, TIM_IMAGE *tim)
{
    uint8_t *bytes;
    uint32_t b, length;

    bytes = file_read(filename, &length);
    if(bytes == NULL) {
        printf("Error reading CHARA file %s from the CD.\n", filename);
        return;
    }

    b = 0;

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
        for(int j = 0; j < 16; j++) {
            chara->anims[i].name[j] = get_byte(bytes, &b);
        }
        chara->anims[i].name[15] = '\0';
        for(int j = 14; j >= 0; j--) {
            if(chara->anims[i].name[j] != ' ')
                break;
            chara->anims[i].name[j] = '\0';
        }
        chara->anims[i].hname = adler32(chara->anims[i].name);
        chara->anims[i].start = get_byte(bytes, &b);
        chara->anims[i].end = get_byte(bytes, &b);
    }

    for(uint16_t i = 0; i < chara->numanims; i++) {
        printf("Animation: %s (h: %08x) [%d -> %d]\n",
               chara->anims[i].name,
               chara->anims[i].hname,
               chara->anims[i].start, chara->anims[i].end);
    }

    /* chara->crectx = tim->crect->x; */
    /* chara->crecty = tim->crect->y; */
    /* chara->prectx = tim->prect->x; */
    /* chara->precty = tim->prect->y; */
    /* printf("prect: %d %d %d %d, %x\n", */
    /*        tim->prect->x, tim->prect->y, */
    /*        tim->prect->w, tim->prect->h, */
    /*        getTPage(tim->mode & 0x3, 1, tim->prect->x, tim->prect->y)); */
    /* printf("crect: %d %d %d %d, %x\n", */
    /*        tim->crect->x, tim->crect->y, */
    /*        tim->crect->w, tim->crect->h, */
    /*        getClut(tim->crect->x, tim->crect->y)); */
    chara->crectx = 0;
    chara->crecty = 480;
    chara->prectx = 320;
    chara->precty = 0; // why not loading correctly???

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
    }
}



void
chara_render_frame(Chara *chara, int16_t framenum, int16_t vx, int16_t vy, uint8_t flipx)
{
    CharaFrame *frame = &chara->frames[framenum];

    int16_t left = frame->x >> 3;
    int16_t right = 7 - (frame->width >> 3) - left;

    for(uint16_t row = 0; row < frame->rows; row++) {
        for(uint16_t colx = 0; colx < frame->cols; colx++) {
            uint16_t col = flipx ? frame->cols - colx - 1 : colx;
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
                vx + (colx << 3) + (flipx ? (right << 3) : frame->x) - (chara->width >> 1),
                vy + (row << 3) + frame->y - (chara->height >> 1),
                0, 0,
            };

            // I don't know why, but it works
            uint8_t tw, th;
            tw = (u0 < 248 ? 8 : 7);
            th = (v0 < 248 ? 8 : 7);
            if(flipx) {
                if (u0 > 0) u0--;
                if (u0 + tw >= 254) tw--;
            }

            // Use a textured quad since we need to reverse
            // our UVs manually
            POLY_FT4 *poly = (POLY_FT4 *) get_next_prim();
            increment_prim(sizeof(POLY_FT4));
            setPolyFT4(poly);
            setRGB0(poly, level_fade, level_fade, level_fade);
            setTPage(poly, 1, 1, chara->prectx, chara->precty); // 8-bit CLUT
            setClut(poly, chara->crectx, chara->crecty);
            setXYWH(poly, xy0.vx, xy0.vy, 8, 8);
            
            if(flipx) {
                setUV4(
                    poly,
                    u0 + tw, v0,
                    u0,      v0,
                    u0 + tw, v0 + th,
                    u0, v0 + th);
            } else {
                setUVWH(poly, u0, v0, tw, th);
            }
            // 3 = sprite and background layer
            // 4 = player sprite layer
            sort_prim(poly, 4);
        }
    }
}

